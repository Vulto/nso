/* mm3mod.c - libmm3mod.so: Mega Man 3 (mstan's Megaman3NESRecomp,
 * repos/nes/mm3-recomp) inside NSO (embed/nsomod.h). NESRecomp turns the
 * user's ROM into native C at build time (CMakeLists.txt); this file takes
 * the place of the port's window (nesrecomp/runner/cyc/cyc_sdl.c) and drives
 * the cycle runtime the way cyc_host.c's own loop does:
 *
 *   init()    ROM -> same image the code was generated from? -> power on
 *   frame()   controller -> cyc_run_frame -> cyc_frame_argb (256x240)
 *   audio     cyc_audio_read, mono int16
 *   states    the runtime's own cycle state files
 *
 * Stock game: none of the port's widescreen, co-op or other extras. */
#include "../nsomod.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cyc_core.h"
#include "cyc_recomp.h"
#include "cyc_run.h"
#include "cyc_session.h"
#include "cyc_state.h"
#include "../../common/nes_cart.h"

#define FB_W 256
#define FB_H 240
#define NES_FPS (1789773.0 / 29780.5)   /* NTSC, 60.0988 */

static struct {
    bool live, audio;
    uint8_t *image;                      /* cyc_load_ines() reads it; kept while live */
    uint8_t rgb[FB_W * FB_H * 3];
} M;

static void mm3_shutdown(void) {
    free(M.image);
    memset(&M, 0, sizeof M);
}

static uint8_t *read_file(const char *path, size_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    uint8_t *buf = NULL;
    long n = fseek(f, 0, SEEK_END) == 0 ? ftell(f) : -1;
    if (n > 0 && fseek(f, 0, SEEK_SET) == 0 && (buf = malloc((size_t)n)) &&
        fread(buf, 1, (size_t)n, f) != (size_t)n) {
        free(buf);
        buf = NULL;
    }
    fclose(f);
    *size = buf ? (size_t)n : 0;
    return buf;
}

static bool mm3_init(const NsomodConfig *cfg) {
    if (M.live || !cfg || !cfg->data_path) return false;
    memset(&M, 0, sizeof M);
    size_t size = 0;
    NesCartInfo cart;
    M.image = read_file(cfg->data_path, &size);
    /* cyc_host.c's checks: a loadable cartridge, the board and PRG the
     * native code was compiled from. */
    if (!M.image || !nes_cart_image(M.image, size, &cart) || !cyc_load_ines(M.image, size) ||
        !nes_cart_identity_compatible_region(&cart, cyc_native_cart_hash, cyc_cart_region() == CYC_REGION_PAL) ||
        cyc_prg_hash() != cyc_native_prg_hash) {
        mm3_shutdown();
        return false;
    }
    cyc_set_console((CycConsole)cyc_native_console);
    if (!cyc_set_region(cyc_cart_region())) { mm3_shutdown(); return false; }
    cyc_power_on(0);
    cyc_run_power_on();
    if (!cyc_session_start()) { mm3_shutdown(); return false; }
    M.audio = cfg->sample_rate > 0 && cyc_audio_enable(cfg->sample_rate);
    M.live = true;
    return true;
}

static NsomodFrame mm3_frame(const NsomodInput *p1) {
    if (M.live) {
        /* NES bit order, MSB first: A B Select Start Up Down Left Right */
        uint8_t buttons[2] = { 0, 0 };
        if (p1)
            buttons[0] = (uint8_t)(p1->a << 7 | p1->b << 6 | p1->select << 5 | p1->start << 4 |
                                   p1->up << 3 | p1->down << 2 | p1->left << 1 | p1->right);
        cyc_session_input(buttons);
        cyc_set_controller(0, buttons[0]);
        cyc_set_controller(1, buttons[1]);
        cyc_session_frame_begin();
        cyc_run_frame();
        cyc_session_frame_end();
        const uint32_t *px = cyc_frame_argb();   /* 0xAARRGGBB */
        for (int i = 0; i < FB_W * FB_H; i++) {
            M.rgb[i * 3 + 0] = (uint8_t)(px[i] >> 16);
            M.rgb[i * 3 + 1] = (uint8_t)(px[i] >> 8);
            M.rgb[i * 3 + 2] = (uint8_t)px[i];
        }
    }
    return (NsomodFrame){ M.rgb, FB_W, FB_H };
}

static size_t mm3_audio_pull(int16_t *out, size_t max) {
    return M.live && M.audio ? cyc_audio_read(out, max) : 0;
}

static bool mm3_state_save(const char *path) {
    char err[256];
    return M.live && path && cyc_state_save_file(path, err, sizeof err);
}

static bool mm3_state_load(const char *path) {
    char err[256];
    if (!M.live || !path || !cyc_state_load_file(path, err, sizeof err)) return false;
    cyc_session_state_loaded();
    return true;
}

static const NsomodApi NSO_API = {
    NSOMOD_API_VERSION, FB_W, FB_H, NES_FPS, 1,
    mm3_init, mm3_frame, mm3_audio_pull, mm3_state_save, mm3_state_load, mm3_shutdown,
};

__attribute__((visibility("default")))
const NsomodApi *nsomod_get_api(void) { return &NSO_API; }
