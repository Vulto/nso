/* smwmod.c - libsmwmod.so: snesrev's Super Mario World reimplementation
 * (repos/snes/smw) without SDL, for NSO (embed/nsomod.h). Takes the place of
 * the port's src/main.c (window, keyboard, audio device, timer, smw.ini); the
 * port's other sources are compiled as they are (never edited).
 *
 * Native only (RM_MINE): no ROM is loaded and nothing is emulated alongside.
 * smw_assets.dat, made once from the user's ROM by the port's restool.py,
 * is read from the port's directory (cfg->data_path). Files the core opens
 * by relative path ("saves/smw.srm", ...) resolve there too, through the
 * link-time wraps below (Makefile: -Wl,--wrap=fopen,--wrap=rename), the
 * same files the port uses when it runs on its own.
 *
 *   main()            -> smw_init()   (LoadAssets, SnesInit, SPC player, SRAM)
 *   main loop body    -> smw_frame()  (RtlRunFrame + draw_ppu_frame, no pacing)
 *   AudioCallback()   -> per frame RtlRenderAudio into a ring, audio_pull()
 *   Die()             -> longjmp back to the API call; the module goes dead
 */
#include "../nsomod.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "assets/smw_assets.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "types.h"
#include "smw_rtl.h"
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "config.h"
#include "util.h"
#include "smw_spc_player.h"

#define FB_W 256
#define FB_H 224
#define NTSC_FPS (21477272.0 / 357366.0)   /* 60.0988 */
#define RING 4096                           /* stereo frames, ~85 ms at 48 kHz */

/* ---- what the port's main.c defines for the rest of the port ---- */
bool g_debug_flag;
bool g_want_dump_memmap_flags;
bool g_new_ppu = true;
bool g_other_image = true;
struct SpcPlayer *g_spc_player;
int g_got_mismatch_count;
Config g_config;
const uint8 *g_asset_ptrs[kNumberOfAssets];
uint32 g_asset_sizes[kNumberOfAssets];

static struct {
    bool live, guard;
    jmp_buf jb;
    char dir[2048];
    uint8 *assets;
    uint8 px[FB_W * 4 * 240], px_theirs[FB_W * 4 * 240];
    uint8 rgb[FB_W * FB_H * 3];
    int rate;
    double acc;
    int16 tmp[2 * 2048];
    int16 ring[2 * RING];
    size_t head, count;
} M;

void NORETURN Die(const char *error) {
    (void)error;
    if (M.guard) longjmp(M.jb, 1);
    abort();
}
void Warning(const char *error) { (void)error; }
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}
void MkDir(const char *s) {
    char p[2304];
    snprintf(p, sizeof p, "%s/%s", M.dir, s);
    mkdir(p, 0755);
}
void RtlDrawPpuFrame(uint8 *pixel_buffer, size_t pitch, uint32 render_flags) {
    (void)render_flags;
    g_rtl_game_info->draw_ppu_frame();
    for (size_t y = 0; y < FB_H; y++) memcpy(pixel_buffer + y * pitch, M.px + y * FB_W * 4, FB_W * 4);
}
MemBlk FindInAssetArray(int asset, int idx) {
    return FindIndexInMemblk((MemBlk){ g_asset_ptrs[asset], g_asset_sizes[asset] }, idx);
}
const uint8 *FindPtrInAsset(int asset, uint32 addr) {
    return FindAddrInMemblk((MemBlk){ g_asset_ptrs[asset], g_asset_sizes[asset] }, addr);
}

/* ---- relative paths of the core -> the port's directory ---- */
FILE *__real_fopen(const char *path, const char *mode);
int __real_rename(const char *from, const char *to);
static const char *in_dir(const char *p, char *buf, size_t n) {
    if (!p || p[0] == '/' || !M.dir[0]) return p;
    snprintf(buf, n, "%s/%s", M.dir, p);
    return buf;
}
FILE *__wrap_fopen(const char *path, const char *mode) {
    char b[2304];
    return __real_fopen(in_dir(path, b, sizeof b), mode);
}
int __wrap_rename(const char *from, const char *to) {
    char a[2304], b[2304];
    return __real_rename(in_dir(from, a, sizeof a), in_dir(to, b, sizeof b));
}

/* ---- the core's console messages: dropped ---- */
#include <stdarg.h>
int __wrap_printf(const char *fmt, ...) { (void)fmt; return 0; }
int __wrap_puts(const char *s) { (void)s; return 0; }
int __wrap_putchar(int c) { return c; }
int __wrap_fprintf(FILE *f, const char *fmt, ...) {
    if (f == stdout || f == stderr) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}

/* main.c LoadAssets(), smw_assets.dat only (no .bps patching of a ROM). */
static bool load_assets(void) {
    static const char sig[] = { kAssets_Sig };
    size_t len = 0;
    M.assets = ReadWholeFile("smw_assets.dat", &len);
    if (!M.assets) M.assets = ReadWholeFile("assets/smw_assets.dat", &len);
    if (!M.assets || len < 16 + 32 + 32 + 8 + kNumberOfAssets * 4 || memcmp(M.assets, sig, 48) != 0 ||
        *(uint32 *)(M.assets + 80) != kNumberOfAssets)
        return false;
    uint32 off = 88 + kNumberOfAssets * 4 + *(uint32 *)(M.assets + 84);
    for (size_t i = 0; i < kNumberOfAssets; i++) {
        uint32 size = *(uint32 *)(M.assets + 88 + i * 4);
        off = (off + 3) & ~3u;
        if ((uint64)off + size > len) return false;
        g_asset_sizes[i] = size;
        g_asset_ptrs[i] = M.assets + off;
        off += size;
    }
    return true;
}

static void smw_shutdown(void);

static bool smw_init(const NsomodConfig *cfg) {
    if (M.live || !cfg || !cfg->data_path) return false;
    memset(&M, 0, sizeof M);
    snprintf(M.dir, sizeof M.dir, "%s", cfg->data_path);
    M.guard = true;
    if (setjmp(M.jb)) { M.guard = false; M.live = true; smw_shutdown(); return false; }

    memset(&g_config, 0, sizeof g_config);   /* the port's smw.ini as NSO ships it */
    g_config.new_renderer = true;
    g_config.no_sprite_limits = true;
    g_config.has_keyboard_controls = 1;
    if (!load_assets() || !SnesInit(NULL, 0)) Die("assets");
    g_spc_player = SmwSpcPlayer_Create();
    g_spc_player->initialize(g_spc_player);
    PpuBeginDrawing(g_snes->ppu, M.px_theirs, FB_W * 4, 0);
    PpuBeginDrawing(g_my_ppu, M.px, FB_W * 4, 0);
    MkDir("saves");
    RtlReadSram();

    M.rate = cfg->sample_rate > 0 ? cfg->sample_rate : 0;
    M.guard = false;
    M.live = true;
    return true;
}

/* SNES pad bits as main.c's kKbdRemap: B Y Sel Start Up Down Left Right A X L R. */
static uint32 pad_bits(const NsomodInput *p) {
    if (!p) return 0;
    return (uint32)p->b | p->y << 1 | p->select << 2 | p->start << 3 | p->up << 4 | p->down << 5 |
           p->left << 6 | p->right << 7 | p->a << 8 | p->x << 9 | p->l << 10 | p->r << 11;
}

static void push_audio(void) {
    M.acc += M.rate / NTSC_FPS;
    int n = (int)M.acc;
    if (n > 2048) n = 2048;
    M.acc -= n;
    RtlRenderAudio(M.tmp, n, 2);
    for (int i = 0; i < n; i++) {
        size_t at = (M.head + M.count) % RING;
        if (M.count == RING) M.head = (M.head + 1) % RING;   /* full: drop the oldest */
        else M.count++;
        M.ring[2 * at] = M.tmp[2 * i];
        M.ring[2 * at + 1] = M.tmp[2 * i + 1];
    }
}

static NsomodFrame smw_frame(const NsomodInput *p1) {
    NsomodFrame f = { M.rgb, FB_W, FB_H };
    if (!M.live) return f;
    M.guard = true;
    if (setjmp(M.jb)) { M.guard = false; M.live = false; return f; }
    RtlRunFrame(pad_bits(p1) | 1u << 30);   /* bit 30: pad 1 connected */
    g_rtl_game_info->draw_ppu_frame();
    if (M.rate) push_audio();
    M.guard = false;
    for (int i = 0; i < FB_W * FB_H; i++) {   /* XRGB8888 (B, G, R, x in memory) */
        M.rgb[3 * i] = M.px[4 * i + 2];
        M.rgb[3 * i + 1] = M.px[4 * i + 1];
        M.rgb[3 * i + 2] = M.px[4 * i];
    }
    return f;
}

static size_t smw_audio_pull(int16_t *out, size_t max) {
    size_t n = 0;
    for (; n < max && M.count; n++, M.count--, M.head = (M.head + 1) % RING) {
        out[2 * n] = M.ring[2 * M.head];
        out[2 * n + 1] = M.ring[2 * M.head + 1];
    }
    return n;
}

static bool smw_state_save(const char *path) {
    if (!M.live || !path) return false;
    M.guard = true;
    if (setjmp(M.jb)) { M.guard = false; M.live = false; return false; }
    bool ok = RtlSaveSnapshot(path, false);
    M.guard = false;
    return ok;
}

static bool smw_state_load(const char *path) {
    if (!M.live || !path || access(path, R_OK) != 0) return false;
    M.guard = true;
    if (setjmp(M.jb)) { M.guard = false; M.live = false; return false; }
    bool ok = RtlLoadStateFile(path);
    M.guard = false;
    return ok;
}

static void smw_shutdown(void) {
    if (!M.live) return;
    if (g_snes) { snes_free(g_snes); g_snes = NULL; }
    if (g_my_ppu) { ppu_free(g_my_ppu); g_my_ppu = NULL; }
    free(M.assets);
    memset(&M, 0, sizeof M);
}

static const NsomodApi API = {
    NSOMOD_API_VERSION, FB_W, FB_H, NTSC_FPS, 2,
    smw_init, smw_frame, smw_audio_pull, smw_state_save, smw_state_load, smw_shutdown,
};

__attribute__((visibility("default")))
const NsomodApi *nsomod_get_api(void) { return &API; }
