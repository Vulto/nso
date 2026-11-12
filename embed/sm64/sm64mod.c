/* sm64mod.c - libsm64mod.so: sm64ex (repos/n64/sm64ex, Super Mario 64
 * decompiled to C, not an emulator) for NSO (embed/nsomod.h). Takes the place
 * of the port's src/pc/pc_main.c and its SDL window, audio and controller
 * backends; every other source is compiled as it is (never edited).
 *
 * Two libraries (Makefile): libsm64game.so holds the game (engine, audio,
 * assets built from the user's ROM, main pool), this one the renderer
 * (gfx_pc + gfx_opengl), fs, config and this adapter. Suspend points are the
 * game library's writable segment, so renderer state never goes in a state.
 *
 *   main_func()           -> sm64_init()   (fs, config, pool, gfx, audio, thread5)
 *   produce_one_frame()   -> sm64_frame()  (one 30 Hz game frame, no pacing)
 *   SDL window / GL       -> a GLX pbuffer of our own, read back as RGB24
 *   SDL audio             -> 32 kHz from the game, resampled into a ring
 *   SDL / keyboard pads   -> osContGetReadData() from NsomodInput
 *   sys_fatal()           -> longjmp back to the API call; the module goes dead
 */
#define _GNU_SOURCE   /* dladdr, dl_iterate_phdr */
#include "../nsomod.h"

#include <dlfcn.h>
#include <link.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <zlib.h>

#include <GL/gl.h>
#include <GL/glx.h>

#include "sm64.h"
#include "game/memory.h"
#include "audio/external.h"
#include "pc/gfx/gfx_pc.h"
#include "pc/gfx/gfx_opengl.h"
#include "pc/gfx/gfx_window_manager_api.h"
#include "pc/configfile.h"
#include "pc/controller/controller_api.h"
#include "pc/fs/fs.h"
#include "pc/platform.h"
#include "reloc.h"

#define FB_W 640                /* gfx_screen_config.h DESIRED_SCREEN_* */
#define FB_H 480
#define GAME_FPS 30.0
#define GAME_RATE 32000         /* create_next_audio_buffer() */
#define SAMPLES_HIGH 544        /* pc_main.c (US) */
#define SAMPLES_LOW 528
#define RING 16384              /* stereo frames, ~340 ms at 48 kHz */
#define RING_WANT 3200          /* ~2 game frames queued */

extern u64 gNsoMainPool[DEFAULT_POOL_SIZE / sizeof(u64)];
extern void gfx_run(Gfx *commands);
extern void thread5_game_loop(void *arg);
extern void game_loop_one_iteration(void);
extern void thread6_rumble_loop(void *arg);
extern void create_next_audio_buffer(s16 *samples, u32 num_samples);
extern void sound_init(void);

/* One loaded library: its whole mapping (for pointer fix-ups) and its
 * writable, non-RELRO ranges (.data/.bss: what a state holds). */
typedef struct {
    const char *name;
    uintptr_t lo, hi;
    uintptr_t rw[4][2];
    int nrw;
    uint8_t id[20];
} Lib;

static struct {
    bool live, guard, inited;
    jmp_buf jb;
    char dir[2048];
    Display *dpy;
    GLXPbuffer pb;
    GLXContext ctx;
    Display *was_dpy;
    GLXDrawable was_draw, was_read;
    GLXContext was_ctx;
    NsomodInput in;
    uint8_t px[FB_W * FB_H * 3], rgb[FB_W * FB_H * 3];
    int rate;
    double phase;
    int16_t prev[2];
    int16_t ring[2 * RING];
    size_t head, count;
} M;

/* ---- what pc_main.c defines for the rest of the port ---- */
void dispatch_audio_sptask(UNUSED struct SPTask *spTask) {}
void set_vblank_handler(UNUSED s32 index, UNUSED struct VblankHandler *handler, UNUSED OSMesgQueue *queue,
                        UNUSED OSMesg *msg) {}
void send_display_list(struct SPTask *spTask) {
    if (M.inited) gfx_run((Gfx *)spTask->task.t.data_ptr);
}
void game_deinit(void) {}
void game_exit(void) {}   /* "Exit Game": NSO's End menu leaves instead */

/* ---- controller_entry_point.c: the pad is NSO's ---- */
s32 osContInit(UNUSED OSMesgQueue *mq, u8 *bits, UNUSED OSContStatus *status) { *bits = 1; return 0; }
s32 osContStartReadData(UNUSED OSMesgQueue *mq) { return 0; }
s32 osMotorStart(UNUSED void *pfs) { return 0; }
s32 osMotorStop(UNUSED void *pfs) { return 0; }
u32 osMotorInit(UNUSED OSMesgQueue *mq, UNUSED void *pfs, UNUSED s32 port) { return 0; }
u32 controller_get_raw_key(void) { return VK_INVALID; }
void controller_reconfigure(void) {}
void controller_rumble_play(UNUSED float str, UNUSED float time) {}
void controller_rumble_stop(void) {}
void controller_shutdown(void) {}

static s8 axis(float v, bool neg, bool pos) {
    if (v > 0.15f || v < -0.15f) return (s8)(v * 80.0f);   /* analog past a small dead zone */
    return (s8)(pos ? 80 : neg ? -80 : 0);
}
void osContGetReadData(OSContPad *pad) {
    const NsomodInput *p = &M.in;
    pad->button = (p->a ? A_BUTTON : 0) | (p->b ? B_BUTTON : 0) | (p->z ? Z_TRIG : 0) | (p->start ? START_BUTTON : 0) |
                  (p->l ? L_TRIG : 0) | (p->r ? R_TRIG : 0) | (p->ry < -0.5f ? U_CBUTTONS : 0) |
                  (p->ry > 0.5f ? D_CBUTTONS : 0) | (p->rx < -0.5f ? L_CBUTTONS : 0) | (p->rx > 0.5f ? R_CBUTTONS : 0);
    bool analog = p->lx > 0.15f || p->lx < -0.15f || p->ly > 0.15f || p->ly < -0.15f;
    pad->stick_x = analog ? axis(p->lx, 0, 0) : axis(0, p->left, p->right);
    pad->stick_y = analog ? axis(-p->ly, 0, 0) : axis(0, p->down, p->up);   /* N64 y is up */
    pad->ext_stick_x = pad->ext_stick_y = 0;
    pad->errnum = 0;
}

/* ---- window API for gfx_pc: no window, the pbuffer is FB_W x FB_H ---- */
static void w_init(UNUSED const char *title) {}
static void w_keys(UNUSED kb_callback_t d, UNUSED kb_callback_t u, UNUSED void (*all)(void)) {}
static void w_loop(UNUSED void (*iter)(void)) {}
static void w_dims(uint32_t *w, uint32_t *h) { *w = FB_W; *h = FB_H; }
static void w_nop(void) {}
static bool w_start(void) { return true; }
static double w_time(void) { return 0.0; }
static struct GfxWindowManagerAPI wapi = { w_init, w_keys, w_loop, w_dims, w_nop, w_start, w_nop, w_nop, w_time, w_nop };

/* ---- the port's console output: dropped; fatal errors: back to NSO ---- */
int __wrap_printf(UNUSED const char *fmt, ...) { return 0; }
int __wrap_puts(UNUSED const char *s) { return 0; }
int __wrap_putchar(int c) { return c; }
int __wrap_fprintf(FILE *f, const char *fmt, ...) {
    if (f == stdout || f == stderr) return 0;
    va_list ap;
    va_start(ap, fmt);
    int n = vfprintf(f, fmt, ap);
    va_end(ap);
    return n;
}
__attribute__((noreturn)) void __wrap_sys_fatal(UNUSED const char *fmt, ...) {
    if (M.guard) longjmp(M.jb, 1);
    abort();
}
/* The options menu would rewrite the user's sm64config.txt (shared with
 * their stand-alone port, which may keep keys this one drops). */
void __wrap_configfile_save(UNUSED const char *name) {}

/* ---- our GL context, current only inside the module's calls ---- */
static bool gl_open(void) {
    M.dpy = XOpenDisplay(NULL);
    if (!M.dpy) return false;
    const int fa[] = { GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT, GLX_RENDER_TYPE, GLX_RGBA_BIT, GLX_RED_SIZE, 8,
                       GLX_GREEN_SIZE, 8, GLX_BLUE_SIZE, 8, GLX_DEPTH_SIZE, 24, None };
    int n = 0;
    GLXFBConfig *fc = glXChooseFBConfig(M.dpy, DefaultScreen(M.dpy), fa, &n);
    if (!fc || n < 1) { if (fc) XFree(fc); return false; }
    const int pa[] = { GLX_PBUFFER_WIDTH, FB_W, GLX_PBUFFER_HEIGHT, FB_H, None };
    M.pb = glXCreatePbuffer(M.dpy, fc[0], pa);
    M.ctx = glXCreateNewContext(M.dpy, fc[0], GLX_RGBA_TYPE, NULL, True);
    XFree(fc);
    return M.pb && M.ctx;
}
static void gl_push(void) {
    M.was_ctx = glXGetCurrentContext();
    M.was_dpy = glXGetCurrentDisplay();
    M.was_draw = glXGetCurrentDrawable();
    M.was_read = glXGetCurrentReadDrawable();
    glXMakeContextCurrent(M.dpy, M.pb, M.pb, M.ctx);
}
static void gl_pop(void) {
    if (M.was_ctx) glXMakeContextCurrent(M.was_dpy, M.was_draw, M.was_read, M.was_ctx);
    else glXMakeContextCurrent(M.dpy, None, None, NULL);
}
static void gl_close(void) {
    if (!M.dpy) return;
    if (M.ctx) glXDestroyContext(M.dpy, M.ctx);
    if (M.pb) glXDestroyPbuffer(M.dpy, M.pb);
    XCloseDisplay(M.dpy);
}

static void sm64_shutdown(void);

static bool sm64_init(const NsomodConfig *cfg) {
    if (M.live || !cfg || !cfg->data_path) return false;
    memset(&M, 0, sizeof M);
    snprintf(M.dir, sizeof M.dir, "%s", cfg->data_path);
    M.rate = cfg->sample_rate > 0 ? cfg->sample_rate : 0;
    if (!gl_open()) { gl_close(); memset(&M, 0, sizeof M); return false; }
    gl_push();
    M.guard = true;
    if (setjmp(M.jb)) { gl_pop(); M.guard = false; M.live = true; sm64_shutdown(); return false; }

    /* main_func(): the user's data dir (EEPROM save, sm64config.txt) is the write path. */
    fs_init(sys_ropaths, FS_BASEDIR, M.dir);
    configfile_load(configfile_name());
    main_pool_init(gNsoMainPool, gNsoMainPool + sizeof gNsoMainPool / sizeof gNsoMainPool[0]);
    gEffectsMemoryPool = mem_pool_init(0x4000, MEMORY_POOL_LEFT);
    gfx_init(&wapi, &gfx_opengl_api, "");
    audio_init();
    sound_init();
    thread5_game_loop(NULL);
    M.inited = true;

    M.guard = false;
    gl_pop();
    M.live = true;
    return true;
}

/* 32 kHz stereo -> M.rate, linear, into the ring (full: drop the oldest). */
static void push_audio(const s16 *in, u32 n) {
    const double step = (double)GAME_RATE / M.rate;
    for (u32 i = 0; i < n; i++) {
        for (; M.phase < 1.0; M.phase += step) {
            size_t at = (M.head + M.count) % RING;
            if (M.count == RING) M.head = (M.head + 1) % RING;
            else M.count++;
            for (int c = 0; c < 2; c++)
                M.ring[2 * at + c] = (int16_t)(M.prev[c] + (in[2 * i + c] - M.prev[c]) * M.phase);
        }
        M.phase -= 1.0;
        M.prev[0] = in[2 * i];
        M.prev[1] = in[2 * i + 1];
    }
}

static NsomodFrame sm64_frame(const NsomodInput *p1) {
    NsomodFrame f = { M.rgb, FB_W, FB_H };
    if (!M.live) return f;
    M.in = p1 ? *p1 : (NsomodInput){ 0 };
    gl_push();
    M.guard = true;
    if (setjmp(M.jb)) { gl_pop(); M.guard = false; M.live = false; return f; }

    /* produce_one_frame() */
    gfx_start_frame();
    const f32 master = (f32)configMasterVolume / 127.0f;
    set_sequence_player_volume(SEQ_PLAYER_LEVEL, (f32)configMusicVolume / 127.0f * master);
    set_sequence_player_volume(SEQ_PLAYER_SFX, (f32)configSfxVolume / 127.0f * master);
    set_sequence_player_volume(SEQ_PLAYER_ENV, (f32)configEnvVolume / 127.0f * master);
    game_loop_one_iteration();
    thread6_rumble_loop(NULL);
    u32 n = M.count < RING_WANT ? SAMPLES_HIGH : SAMPLES_LOW;
    s16 buf[SAMPLES_HIGH * 2 * 2];
    for (int i = 0; i < 2; i++) create_next_audio_buffer(buf + i * (n * 2), n);
    if (M.rate) push_audio(buf, 2 * n);
    gfx_end_frame();

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, FB_W, FB_H, GL_RGB, GL_UNSIGNED_BYTE, M.px);
    M.guard = false;
    gl_pop();
    for (int y = 0; y < FB_H; y++)   /* GL rows go bottom-up */
        memcpy(M.rgb + y * FB_W * 3, M.px + (FB_H - 1 - y) * FB_W * 3, FB_W * 3);
    return f;
}

static size_t sm64_audio_pull(int16_t *out, size_t max) {
    size_t n = 0;
    for (; n < max && M.count; n++, M.count--, M.head = (M.head + 1) % RING) {
        out[2 * n] = M.ring[2 * M.head];
        out[2 * n + 1] = M.ring[2 * M.head + 1];
    }
    return n;
}

/* ---- suspend points: the game library's .data/.bss, gzip'd ----
 * The libraries load at another address every run, so a state also records
 * where both were; on load, every 4-aligned word that pointed into either
 * mapping is moved by the same distance. */
static int lib_cb(struct dl_phdr_info *info, UNUSED size_t sz, void *data) {
    Lib *L = data;
    if (!info->dlpi_name || strcmp(info->dlpi_name, L->name) != 0) return 0;
    uintptr_t relro_lo = 0, relro_hi = 0;
    L->lo = UINTPTR_MAX;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
        uintptr_t a = info->dlpi_addr + ph->p_vaddr;
        if (ph->p_type == PT_GNU_RELRO) { relro_lo = a; relro_hi = a + ph->p_memsz; }
        if (ph->p_type == PT_LOAD) {
            if (a < L->lo) L->lo = a;
            if (a + ph->p_memsz > L->hi) L->hi = a + ph->p_memsz;
        }
        if (ph->p_type == PT_NOTE) {   /* NT_GNU_BUILD_ID: a state fits one build */
            const uint8_t *p = (const uint8_t *)a, *end = p + ph->p_memsz;
            while (p + sizeof(ElfW(Nhdr)) <= end) {
                const ElfW(Nhdr) *nh = (const ElfW(Nhdr) *)p;
                const uint8_t *desc = p + sizeof *nh + ((nh->n_namesz + 3) & ~3u);
                if (nh->n_type == NT_GNU_BUILD_ID && nh->n_namesz == 4 && !memcmp(p + sizeof *nh, "GNU", 4))
                    memcpy(L->id, desc, nh->n_descsz < 20 ? nh->n_descsz : 20);
                p = desc + ((nh->n_descsz + 3) & ~3u);
            }
        }
    }
    for (int i = 0; i < info->dlpi_phnum && L->nrw < 4; i++) {
        const ElfW(Phdr) *ph = &info->dlpi_phdr[i];
        if (ph->p_type != PT_LOAD || !(ph->p_flags & PF_W)) continue;
        uintptr_t a = info->dlpi_addr + ph->p_vaddr, b = a + ph->p_memsz;
        if (relro_hi > a && relro_hi <= b && relro_lo <= a) a = relro_hi;   /* RELRO opens the segment */
        if (a < b) { L->rw[L->nrw][0] = a; L->rw[L->nrw][1] = b - a; L->nrw++; }
    }
    return 1;
}
static bool lib_find(Lib *L, const void *sym) {
    Dl_info di;
    memset(L, 0, sizeof *L);
    if (!dladdr(sym, &di) || !di.dli_fname) return false;
    L->name = di.dli_fname;
    return dl_iterate_phdr(lib_cb, L) && L->nrw > 0;
}

typedef struct {
    char magic[8];
    uint8_t id[20], mod_id[20];   /* both builds: the game points into this library too */
    uint32_t nrw;
    uint64_t game_lo, game_size, mod_lo, mod_size;
    uint64_t rw[4][2];   /* offset from game_lo, size */
} StateHead;
static const char MAGIC[8] = "NSOSM64";

static bool state_head(StateHead *h, Lib *g, Lib *m) {
    if (!lib_find(g, gNsoMainPool) || !lib_find(m, (const void *)sm64_frame)) return false;
    memset(h, 0, sizeof *h);
    memcpy(h->magic, MAGIC, 8);
    memcpy(h->id, g->id, 20);
    memcpy(h->mod_id, m->id, 20);
    h->nrw = (uint32_t)g->nrw;
    h->game_lo = g->lo; h->game_size = g->hi - g->lo;
    h->mod_lo = m->lo; h->mod_size = m->hi - m->lo;
    for (int i = 0; i < g->nrw; i++) { h->rw[i][0] = g->rw[i][0] - g->lo; h->rw[i][1] = g->rw[i][1]; }
    return true;
}

static bool sm64_state_save(const char *path) {
    StateHead h;
    Lib g, m;
    if (!M.live || !path || !state_head(&h, &g, &m)) return false;
    gzFile z = gzopen(path, "wb1");
    if (!z) return false;
    bool ok = gzwrite(z, &h, sizeof h) == (int)sizeof h;
    for (int i = 0; ok && i < g.nrw; i++)
        ok = gzwrite(z, (const void *)g.rw[i][0], (unsigned)g.rw[i][1]) == (int)g.rw[i][1];
    return gzclose(z) == Z_OK && ok;
}

static bool sm64_state_load(const char *path) {
    StateHead now, h;
    Lib g, m;
    if (!M.live || !path || access(path, R_OK) != 0 || !state_head(&now, &g, &m)) return false;
    gzFile z = gzopen(path, "rb");
    if (!z) return false;
    bool ok = gzread(z, &h, sizeof h) == (int)sizeof h && !memcmp(h.magic, MAGIC, 8) && !memcmp(h.id, now.id, 20) &&
              !memcmp(h.mod_id, now.mod_id, 20) &&
              h.nrw == now.nrw && h.game_size == now.game_size && h.mod_size == now.mod_size &&
              !memcmp(h.rw, now.rw, sizeof h.rw);
    size_t total = 0;
    for (int i = 0; ok && i < g.nrw; i++) total += g.rw[i][1];
    uint8_t *buf = ok ? malloc(total) : NULL;
    size_t at = 0;
    for (int i = 0; buf && i < g.nrw && ok; i++) {   /* all read before any byte of the game changes */
        ok = gzread(z, buf + at, (unsigned)g.rw[i][1]) == (int)g.rw[i][1];
        at += g.rw[i][1];
    }
    gzclose(z);
    if (!buf || !ok) { free(buf); return false; }
    at = 0;
    const uint64_t lo[2] = { h.game_lo, h.mod_lo }, size[2] = { h.game_size, h.mod_size };
    const uint64_t d[2] = { now.game_lo - h.game_lo, now.mod_lo - h.mod_lo };
    for (int i = 0; i < g.nrw; i++) {
        memcpy((void *)g.rw[i][0], buf + at, g.rw[i][1]);
        at += g.rw[i][1];
        Sm64_Relocate((uint8_t *)g.rw[i][0], g.rw[i][1], lo, size, d);
    }
    free(buf);
    M.count = M.head = 0;   /* the old position's sound */
    return true;
}

static void sm64_shutdown(void) {
    if (M.ctx && M.inited) {
        gl_push();
        gfx_shutdown();
        gl_pop();
    }
    gl_close();
    memset(&M, 0, sizeof M);
}

static const NsomodApi API = {
    NSOMOD_API_VERSION, FB_W, FB_H, GAME_FPS, 2,
    sm64_init, sm64_frame, sm64_audio_pull, sm64_state_save, sm64_state_load, sm64_shutdown,
};

const NsomodApi *nsomod_get_api(void) { return &API; }
