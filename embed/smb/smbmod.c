/* smbmod.c - libsmbmod.so: smbcore (SMB game logic) + SMB Vanilla's own
 * software rasterizer, no SDL. Takes the place of platform/main_sdl.c;
 * SMB Vanilla's sources are compiled as they are (never edited).
 *
 *   main()      -> smbmod_init()  (no SDL_Init / CreateWindow / INI)
 *   sdl_tick()  -> smbmod_frame() (no PollEvent / blit; the host paces frames)
 *   joy1/joy2, update_pattern_tables, update_palette, draw_tile, apu_*
 *               -> the cb_* callbacks below
 */
#include "smbmod.h"
#include "../nsomod.h"
#include "smbmod_audio.h"

#include "smbcore/api.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"   /* the core's inline stubs */
#include "smbcore/interface.h"   /* struct SMB_state: the PPU flags a state file needs */
#pragma GCC diagnostic pop
#include "render_raster.h"
#include "palette_builtin.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SMB_RAM_SAVE  0x800
#define SMB_PPU_SAVE  0x4000

static struct {
    bool live;
    struct SMB_assets *assets;
    struct SMB_state  *state;
    struct SMBraster  *raster;
    SmbmodAudio       *audio;
    struct SMB_buttons p1, p2;
    uint8_t games_beaten;
    uint8_t fb[SMBMOD_FB_W * SMBMOD_FB_H * 3];
} M;

/* ---- SMB_state_options callbacks (smbcore/api.h) ---- */
static void cb_joy1(void *u, struct SMB_buttons *b) { (void)u; *b = M.p1; }
static void cb_joy2(void *u, struct SMB_buttons *b) { (void)u; *b = M.p2; }
static void cb_patterns(void *u, const u8 *chr) { (void)u; SMBraster_update_pattern_tables(M.raster, chr); }
static void cb_palette(void *u, const u8 *idx)  { (void)u; SMBraster_update_palette(M.raster, idx); }
static void cb_tile(void *u, const struct SMB_tile t) { (void)u; SMBraster_draw_tile(M.raster, t); }
static void cb_apu_w(void *u, u16 a, u8 d) { (void)u; if (M.audio) smbmod_audio_write_register(M.audio, a, d); }
static void cb_apu_end(void *u) { (void)u; if (M.audio) smbmod_audio_end_frame(M.audio); }
static u8   cb_gb_load(void *u) { (void)u; return M.games_beaten; }
static bool cb_gb_save(void *u, u8 g) { (void)u; M.games_beaten = g; return true; }

static struct SMB_buttons to_buttons(const SmbmodInput *in) {
    struct SMB_buttons b = {0};
    if (!in) return b;
    b.u = in->up; b.d = in->down; b.l = in->left; b.r = in->right;
    b.a = in->a; b.b = in->b; b.select = in->select; b.start = in->start;
    return b;
}

static void smbmod_shutdown(void);

static bool smbmod_init(const SmbmodConfig *cfg) {
    if (M.live || !cfg || !cfg->rom_path) return false;
    memset(&M, 0, sizeof M);

    M.state  = malloc(SMB_state_size());        /* ~90 KB: RAM 64K + PPU 16K + CHR 8K */
    M.assets = malloc(SMB_assets_size());
    M.raster = malloc(SMBraster_size());
    if (!M.state || !M.assets || !M.raster || !SMBraster_init(M.raster)) goto fail;

    SMBraster_provide_palette_lookup(M.raster,
        cfg->palette_rgb ? cfg->palette_rgb : &PALETTE_BUILTIN[0][0]);
    SMBraster_set_buffer(M.raster, M.fb, SMBMOD_FB_W);

    if (cfg->sample_rate > 0) M.audio = smbmod_audio_new(cfg->sample_rate);   /* optional */
    M.games_beaten = cfg->smb2j_games_beaten;

    /* smbcore validates the ROM (detect_game); it is only read. */
    if (!SMB_assets_init_from_rom_file(M.assets, cfg->rom_path)) { free(M.assets); M.assets = NULL; goto fail; }

    struct SMB_state_options o = {0};
    o.assets = M.assets;
    o.update_pattern_tables = cb_patterns;
    o.update_palette = cb_palette;
    o.draw_tile = cb_tile;
    o.apu_write_register = cb_apu_w;
    o.apu_end_frame = cb_apu_end;
    o.joy1 = cb_joy1;
    o.joy2 = cb_joy2;
    o.smb2j_load_games_beaten = cb_gb_load;
    o.smb2j_save_games_beaten = cb_gb_save;
    if (!SMB_state_init(M.state, &o)) goto fail;
    if (cfg->start_world > 0 && cfg->start_level > 0)
        SMB_start_on_level(M.state, (u8)cfg->start_world, (u8)cfg->start_level);

    M.live = true;
    return true;
fail:
    M.live = true;      /* let shutdown free what was allocated */
    smbmod_shutdown();
    return false;
}

static int smbmod_which_game(void) { return M.live ? SMB_which_game(M.state) : 0; }

static SmbmodFrame smbmod_frame(const SmbmodInput *p1, const SmbmodInput *p2) {
    SmbmodFrame f = { M.fb, SMBMOD_FB_W, SMBMOD_FB_H };
    if (!M.live) return f;
    M.p1 = to_buttons(p1);
    M.p2 = to_buttons(p2);
    SMBraster_clear(M.raster);   /* as main_sdl.c sdl_tick */
    SMB_tick(M.state);           /* one NMI = one frame, drawn through cb_tile */
    return f;
}

static size_t smbmod_audio_pull_api(int16_t *out, size_t max) {
    return (M.live && M.audio) ? smbmod_audio_pull(M.audio, out, max) : 0;
}

static size_t smbmod_state_size(void) { return SMB_RAM_SAVE + SMB_PPU_SAVE; }

static bool smbmod_state_save(void *buf, size_t size) {
    if (!M.live || size < smbmod_state_size()) return false;
    memcpy(buf, SMB_ram(M.state), SMB_RAM_SAVE);
    memcpy((uint8_t *)buf + SMB_RAM_SAVE, SMB_ppuram(M.state), SMB_PPU_SAVE);
    return true;
}

static bool smbmod_state_load(const void *buf, size_t size) {
    if (!M.live || size < smbmod_state_size()) return false;
    memcpy(SMB_ram(M.state), buf, SMB_RAM_SAVE);
    memcpy(SMB_ppuram(M.state), (const uint8_t *)buf + SMB_RAM_SAVE, SMB_PPU_SAVE);
    SMB_ram_finishwrite(M.state);
    return true;
}

static void smbmod_shutdown(void) {
    if (!M.live) return;
    if (M.assets) { SMB_assets_fini(M.assets); free(M.assets); }
    if (M.raster) { SMBraster_fini(M.raster); free(M.raster); }
    if (M.audio) smbmod_audio_free(M.audio);
    free(M.state);
    memset(&M, 0, sizeof M);   /* smbcore's SMB_STATE dangles until the next init sets it */
}

static const SmbmodApi API = {
    SMBMOD_API_VERSION, smbmod_init, smbmod_which_game, smbmod_frame,
    smbmod_audio_pull_api, smbmod_state_size, smbmod_state_save, smbmod_state_load,
    smbmod_shutdown,
};

__attribute__((visibility("default")))
const SmbmodApi *smbmod_get_api(void) { return &API; }

/* ---- nsomod.h: what NSO (src/embed.h) loads ---- */
static bool nso_init(const NsomodConfig *cfg) {
    if (!cfg) return false;
    SmbmodConfig c = { .rom_path = cfg->data_path, .sample_rate = cfg->sample_rate };
    return smbmod_init(&c);
}

static NsomodFrame nso_frame(const NsomodInput *p1) {
    SmbmodInput in = { 0 };
    if (p1) in = (SmbmodInput){ p1->up, p1->down, p1->left, p1->right, p1->a, p1->b, p1->select, p1->start };
    SmbmodFrame f = smbmod_frame(&in, NULL);
    return (NsomodFrame){ f.rgb24, f.width, f.height };
}

/* Suspend point file: magic, CPU RAM 0x800 + PPU RAM 0x4000 (what SMB
 * Vanilla's own dump_ram/dump_ppuram keep), then the PPU flags the core holds
 * outside RAM. Everything else is rebuilt from RAM by SMB_ram_finishwrite.
 * shortcut: SMB1 only; SMB2J also lives in FDS RAM $6000-$DFFF, save it to add 2J. */
#define NSO_STATE_MAGIC "NSOSMB1\x01"
typedef struct {
    char magic[8];
    uint8_t ram[SMB_RAM_SAVE], ppuram[SMB_PPU_SAVE];
    uint16_t scroll_x, scroll_y;
    uint8_t screen_on;
} NsoSmbState;

static bool nso_state_save(const char *path) {
    if (!M.live || !path || M.state->which_game != GAME_SMB1) return false;
    static NsoSmbState s;
    memcpy(s.magic, NSO_STATE_MAGIC, sizeof s.magic);
    memcpy(s.ram, SMB_ram(M.state), SMB_RAM_SAVE);
    memcpy(s.ppuram, SMB_ppuram(M.state), SMB_PPU_SAVE);
    s.scroll_x = M.state->ppu.scroll_x;
    s.scroll_y = M.state->ppu.scroll_y;
    s.screen_on = M.state->ppu.screen_on;
    FILE *f = fopen(path, "wb");
    if (!f) return false;
    bool ok = fwrite(&s, sizeof s, 1, f) == 1;
    return fclose(f) == 0 && ok;
}

static bool nso_state_load(const char *path) {
    if (!M.live || !path || M.state->which_game != GAME_SMB1) return false;
    static NsoSmbState s;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fread(&s, sizeof s, 1, f) == 1 && fgetc(f) == EOF;
    fclose(f);
    if (!ok || memcmp(s.magic, NSO_STATE_MAGIC, sizeof s.magic) != 0) return false;
    memcpy(SMB_ram(M.state), s.ram, SMB_RAM_SAVE);
    memcpy(SMB_ppuram(M.state), s.ppuram, SMB_PPU_SAVE);
    M.state->ppu.scroll_x = s.scroll_x;
    M.state->ppu.scroll_y = s.scroll_y;
    M.state->ppu.screen_on = s.screen_on != 0;
    M.state->reset_occurred = true;   /* loaded before the first tick: no smb1_Reset over it */
    SMB_ram_finishwrite(M.state);
    /* the next frame is cleared to the backdrop before the core pushes the palette */
    SMBraster_update_palette(M.raster, SMB_ppuram(M.state) + 0x3F00);
    if (M.audio) smbmod_audio_reset(M.audio);
    return true;
}

static const NsomodApi NSO_API = {
    NSOMOD_API_VERSION, SMBMOD_FB_W, SMBMOD_FB_H, SMBMOD_NES_FPS, 1,
    nso_init, nso_frame, smbmod_audio_pull_api, nso_state_save, nso_state_load, smbmod_shutdown,
};

__attribute__((visibility("default")))
const NsomodApi *nsomod_get_api(void) { return &NSO_API; }
