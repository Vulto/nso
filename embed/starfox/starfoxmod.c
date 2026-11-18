/* Star Fox SNES recomp module for NSO. This replaces the desktop main/window,
 * presentation loop and state menu while leaving the game and SNES runner
 * sources untouched. */
#include "nsomod.h"

#include <pthread.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "config.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "snes/dsp.h"
#include "spc_player.h"
#include "starfox_rtl.h"
#include "starfox_spc_player.h"
#include "types.h"
#include "util.h"

extern const RtlGameInfo kStarFoxGameInfo;

#define FB_W 256
#define FB_H 224
#define NTSC_FPS (21477272.0 / 357366.0)
#define AUDIO_RING_FRAMES 4096
#define STATE_HEADER_SIZE 56
#define STATE_MAX_SIZE (64u * 1024u * 1024u)

/* Globals normally supplied by src/main.c. The desktop renderer and window
 * globals are intentionally absent; Star Fox runs in the NSO-owned frame. */
bool g_debug_flag;
bool g_want_dump_memmap_flags;
bool g_new_ppu = true;
bool g_other_image = true;
struct SpcPlayer *g_spc_player;
int g_got_mismatch_count;
bool g_ws_active;
int g_ws_extra;

/* StarFoxDrawPpuFrame walks scanlines 0 through 224 inclusive. The desktop
 * host reserves 240 rows for this PPU scratch surface even though the
 * presented SNES image is only 224 rows tall. */
static uint8_t g_pixels[FB_W * 240 * 4];

static struct {
  bool live;
  bool guard;
  jmp_buf fatal;
  uint64_t rom_hash;
  int sample_rate;
  double sample_accumulator;
  int16_t audio_tmp[2048 * 2];
  int16_t audio_ring[AUDIO_RING_FRAMES * 2];
  size_t audio_head, audio_count;
} M;

static pthread_mutex_t g_apu_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_audio_mutex = PTHREAD_MUTEX_INITIALIZER;

void NORETURN Die(const char *error) {
  (void)error;
  if (M.guard) longjmp(M.fatal, 1);
  abort();
}

void Warning(const char *error) { (void)error; }
void RtlApuLock(void) { pthread_mutex_lock(&g_apu_mutex); }
void RtlApuUnlock(void) { pthread_mutex_unlock(&g_apu_mutex); }

static uint64_t hash_bytes(const uint8_t *data, size_t size) {
  uint64_t hash = UINT64_C(14695981039346656037);
  for (size_t i = 0; i < size; i++) {
    hash ^= data[i];
    hash *= UINT64_C(1099511628211);
  }
  return hash;
}

static uint32_t crc32_bytes(const uint8_t *data, size_t size) {
  uint32_t crc = UINT32_C(0xffffffff);
  for (size_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8; bit++)
      crc = (crc >> 1) ^ (UINT32_C(0xedb88320) & (0u - (crc & 1u)));
  }
  return ~crc;
}

static void put_u32(uint8_t *dst, uint32_t value) {
  for (unsigned i = 0; i < 4; i++) dst[i] = (uint8_t)(value >> (8 * i));
}

static uint32_t get_u32(const uint8_t *src) {
  uint32_t value = 0;
  for (unsigned i = 0; i < 4; i++) value |= (uint32_t)src[i] << (8 * i);
  return value;
}

static void put_u64(uint8_t *dst, uint64_t value) {
  for (unsigned i = 0; i < 8; i++) dst[i] = (uint8_t)(value >> (8 * i));
}

static uint64_t get_u64(const uint8_t *src) {
  uint64_t value = 0;
  for (unsigned i = 0; i < 8; i++) value |= (uint64_t)src[i] << (8 * i);
  return value;
}

static bool read_file(const char *path, uint8_t **out, size_t *size) {
  FILE *file = fopen(path, "rb");
  long length;
  uint8_t *bytes;
  if (!file) return false;
  if (fseek(file, 0, SEEK_END) != 0 || (length = ftell(file)) <= 0 ||
      fseek(file, 0, SEEK_SET) != 0) {
    fclose(file);
    return false;
  }
  bytes = (uint8_t *)malloc((size_t)length);
  if (!bytes) {
    fclose(file);
    return false;
  }
  if (fread(bytes, 1, (size_t)length, file) != (size_t)length) {
    free(bytes);
    fclose(file);
    return false;
  }
  fclose(file);
  *out = bytes;
  *size = (size_t)length;
  return true;
}

static void prepare_save_dirs(void) {
  mkdir("saves", 0755);
  mkdir("saves/snes", 0755);
  mkdir("saves/snes/starfox", 0755);
  RtlSetSaveRoot("saves/snes/starfox");
}

static void release_machine(bool write_sram) {
  if (g_snes) {
    if (write_sram) RtlTryWriteSram();
    snes_free(g_snes);
    g_snes = NULL;
    g_snes_cpu = NULL;
    g_ppu = NULL;
    g_dma = NULL;
  }
  if (g_spc_player) {
    if (g_spc_player->dsp) dsp_free(g_spc_player->dsp);
    free(g_spc_player);
    g_spc_player = NULL;
  }
  RtlSetSaveRoot(NULL);
}

static bool starfox_init(const NsomodConfig *cfg) {
  uint8_t *rom = NULL;
  size_t rom_size = 0;
  if (M.live || !cfg || !cfg->data_path || !read_file(cfg->data_path, &rom, &rom_size))
    return false;

  memset(&M, 0, sizeof(M));
  M.guard = true;
  if (setjmp(M.fatal)) {
    M.guard = false;
    free(rom);
    release_machine(false);
    return false;
  }

  memset(&g_config, 0, sizeof(g_config));
  g_config.new_renderer = true;
  g_config.no_sprite_limits = true;
  g_config.has_keyboard_controls = 1;
  g_ws_active = false;
  g_ws_extra = 0;
  g_new_ppu = true;
  g_spc_player = NULL;

  RtlRegisterGame(&kStarFoxGameInfo);
  if (!SnesInit(rom, (int)rom_size)) {
    free(rom);
    M.guard = false;
    release_machine(false);
    return false;
  }
  free(rom);
  g_spc_player = StarFoxSpcPlayer_Create();
  if (!g_spc_player) {
    M.guard = false;
    release_machine(false);
    return false;
  }
  g_spc_player->initialize(g_spc_player);
  PpuBeginDrawing(g_ppu, g_pixels, FB_W * 4, kPpuRenderFlags_NewRenderer |
                  kPpuRenderFlags_NoSpriteLimits);

  M.rom_hash = hash_bytes(g_snes->cart->rom, g_snes->cart->romImageSize);
  M.sample_rate = cfg->sample_rate >= 8000 && cfg->sample_rate <= 192000
                      ? cfg->sample_rate : 0;
  if (M.sample_rate) RtlSetAudioOutputRate(M.sample_rate);
  prepare_save_dirs();
  RtlReadSram();
  M.guard = false;
  M.live = true;
  return true;
}

static uint32_t pad_bits(const NsomodInput *p) {
  if (!p) return 0;
  return (uint32_t)p->b | (uint32_t)p->y << 1 | (uint32_t)p->select << 2 |
         (uint32_t)p->start << 3 | (uint32_t)p->up << 4 |
         (uint32_t)p->down << 5 | (uint32_t)p->left << 6 |
         (uint32_t)p->right << 7 | (uint32_t)p->a << 8 |
         (uint32_t)p->x << 9 | (uint32_t)p->l << 10 | (uint32_t)p->r << 11;
}

static void queue_audio(void) {
  M.sample_accumulator += (double)M.sample_rate / NTSC_FPS;
  int samples = (int)M.sample_accumulator;
  if (samples > 2048) samples = 2048;
  M.sample_accumulator -= samples;
  /* RtlRenderAudio acquires/releases the runner's APU lock itself. */
  RtlRenderAudio(M.audio_tmp, samples, 2);

  pthread_mutex_lock(&g_audio_mutex);
  for (int i = 0; i < samples; i++) {
    size_t at = (M.audio_head + M.audio_count) % AUDIO_RING_FRAMES;
    if (M.audio_count == AUDIO_RING_FRAMES)
      M.audio_head = (M.audio_head + 1) % AUDIO_RING_FRAMES;
    else
      M.audio_count++;
    M.audio_ring[at * 2] = M.audio_tmp[i * 2];
    M.audio_ring[at * 2 + 1] = M.audio_tmp[i * 2 + 1];
  }
  pthread_mutex_unlock(&g_audio_mutex);
}

static NsomodFrame starfox_frame(const NsomodInput *input) {
  static uint8_t rgb[FB_W * FB_H * 3];
  NsomodFrame frame = { rgb, FB_W, FB_H };
  if (!M.live) return frame;

  M.guard = true;
  if (setjmp(M.fatal)) {
    M.guard = false;
    M.live = false;
    return frame;
  }
  uint32_t inputs = pad_bits(input) | (UINT32_C(1) << 30);
  StarFoxEnhancedPreFrame(inputs);
  RtlRunFrame(inputs);
  StarFoxEnhancedPostFrame(inputs);
  PpuBeginDrawing(g_ppu, g_pixels, FB_W * 4, kPpuRenderFlags_NewRenderer |
                  kPpuRenderFlags_NoSpriteLimits);
  PpuSetExtraSpace(g_ppu, 0);
  StarFoxDrawPpuFrame();
  if (M.sample_rate) queue_audio();
  M.guard = false;

  for (size_t i = 0; i < FB_W * FB_H; i++) {
    rgb[i * 3] = g_pixels[i * 4 + 2];
    rgb[i * 3 + 1] = g_pixels[i * 4 + 1];
    rgb[i * 3 + 2] = g_pixels[i * 4];
  }
  return frame;
}

static size_t starfox_audio_pull(int16_t *out, size_t max_frames) {
  size_t count = 0;
  if (!out) return 0;
  pthread_mutex_lock(&g_audio_mutex);
  while (count < max_frames && M.audio_count) {
    out[count * 2] = M.audio_ring[M.audio_head * 2];
    out[count * 2 + 1] = M.audio_ring[M.audio_head * 2 + 1];
    M.audio_head = (M.audio_head + 1) % AUDIO_RING_FRAMES;
    M.audio_count--;
    count++;
  }
  pthread_mutex_unlock(&g_audio_mutex);
  return count;
}

static bool starfox_state_save(const char *path) {
  static const uint8_t magic[8] = { 'S','F','N','S','O','S','T',0 };
  uint8_t header[STATE_HEADER_SIZE] = { 0 };
  size_t size;
  uint8_t *payload;
  FILE *file;
  if (!M.live || !path) return false;
  size = RtlSaveSnapshotToMemory(NULL, 0);
  if (size < 8 || size > STATE_MAX_SIZE) return false;
  payload = (uint8_t *)malloc(size);
  if (!payload) return false;
  if (RtlSaveSnapshotToMemory(payload, size) != size) {
    free(payload);
    return false;
  }
  memcpy(header, magic, sizeof(magic));
  put_u32(header + 8, 1);
  put_u64(header + 12, (uint64_t)size);
  put_u32(header + 20, crc32_bytes(payload, size));
  put_u64(header + 24, M.rom_hash);
  file = fopen(path, "wb");
  bool ok = file && fwrite(header, 1, sizeof(header), file) == sizeof(header) &&
            fwrite(payload, 1, size, file) == size && fflush(file) == 0;
  if (file && fclose(file) != 0) ok = false;
  free(payload);
  return ok;
}

static bool starfox_state_load(const char *path) {
  static const uint8_t magic[8] = { 'S','F','N','S','O','S','T',0 };
  uint8_t header[STATE_HEADER_SIZE];
  uint8_t *payload = NULL;
  size_t payload_size = 0;
  bool ok = false;
  if (!M.live || !path) return false;
  FILE *file = fopen(path, "rb");
  if (!file) return false;
  if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
      memcmp(header, magic, sizeof(magic)) != 0 || get_u32(header + 8) != 1 ||
      get_u64(header + 24) != M.rom_hash) {
    fclose(file);
    return false;
  }
  uint64_t length = get_u64(header + 12);
  if (length < 8 || length > STATE_MAX_SIZE) {
    fclose(file);
    return false;
  }
  payload_size = (size_t)length;
  payload = (uint8_t *)malloc(payload_size);
  if (!payload) {
    fclose(file);
    return false;
  }
  ok = fread(payload, 1, payload_size, file) == payload_size &&
       fgetc(file) == EOF && !ferror(file) &&
       crc32_bytes(payload, payload_size) == get_u32(header + 20) &&
       StarFoxSnapshotPreflight(payload, payload_size);
  fclose(file);
  if (ok) ok = RtlLoadSnapshotFromMemory(payload, payload_size);
  free(payload);
  return ok;
}

static void starfox_shutdown(void) {
  M.live = false;
  release_machine(true);
  pthread_mutex_lock(&g_audio_mutex);
  M.audio_head = M.audio_count = 0;
  pthread_mutex_unlock(&g_audio_mutex);
}

static const NsomodApi API = {
  .version = NSOMOD_API_VERSION,
  .fb_w = FB_W,
  .fb_h = FB_H,
  .fps = NTSC_FPS,
  .channels = 2,
  .init = starfox_init,
  .frame = starfox_frame,
  .audio_pull = starfox_audio_pull,
  .state_save = starfox_state_save,
  .state_load = starfox_state_load,
  .shutdown = starfox_shutdown,
};

__attribute__((visibility("default")))
const NsomodApi *nsomod_get_api(void) { return &API; }
