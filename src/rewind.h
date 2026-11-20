/* rewind.h - Mednafen's game rewind (NES and SNES) inside NSO.
 *
 * Mednafen runs with -srwautoenable 1 (600 frames of history). Its rewind key (Backspace) works as
 * is on a keyboard; on a pad NSO holds ZL+ZR (a tap of the same chord opens the Suspend Menu,
 * as on Switch Online) and injects that key with XTest. A small patch in the Mednafen tree
 * (src/drivers/main.cpp, NSO_REWIND_SHM) publishes every emulated frame, the number of frames
 * that can still be rewound, and a thumbnail of the game every second (60 frames) to a shared file. While the rewind
 * key is held NSO hides the emulator's window (X Shape: it keeps the keyboard focus), draws the
 * live frame itself, shrunk over the gray panel with the thumbnail strip, per rewindanim.h.
 * With "Show controls in game" a black caption bar is drawn over the bottom edge of the game (the
 * window stays full size): at game start for a few seconds, and while rewinding. Linux only. */
#ifndef NSO_REWIND_H
#define NSO_REWIND_H
#include <stdbool.h>
#include "raylib.h"

/* scanlines: the Scanlines toggle (with crtgeom, the CRT shader's beam scanlines); crtgeom: draw the live
 * frame with shaders/crtgeom.glsl (crtgeom.h) like Mednafen does, so rewinding does not change the look. */
void Rewind_Begin(const char *shmPath, const char *menuDir, bool pixel, bool scanlines, bool crtgeom, bool showControls);
void Rewind_End(void);
/* Back to the plain game window at once (key released, window shown): before the Suspend Menu grabs it. */
void Rewind_Reset(void);
/* Once a frame; inGame = an external game is on screen (not its Suspend Menu). */
void Rewind_Frame(float dt, bool inGame, bool zlzrHeld);
/* True once after a short tap of ZL+ZR: open the Suspend Menu. */
bool Rewind_MenuTap(void);
bool Rewind_Session(void);
void Rewind_Draw(Font font, const char *capHold, const char *capSuspend, const char *capBack, const char *capOk);

#ifdef REWIND_IMPLEMENTATION
#ifndef REWIND_IMPLEMENTATION_ONCE
#define REWIND_IMPLEMENTATION_ONCE
#include "rewindanim.h"
#include "crtgeom.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if defined(__linux__)
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#define Font X11Font
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/shape.h>
#undef Font

enum { RW_MAGIC = 0x3157524E, RW_FW = 256, RW_FH = 240, RW_SLOTS = 64, RW_HEAD = 4096,
       RW_FRAME_BYTES = RW_FW * RW_FH * 4, RW_HOLD_MS = 250 };
typedef struct {   /* same layout as Mednafen's NsoRwHeader */
    uint32_t magic, version, seq, enabled, w, h, rewinding, history, maxhist, frame, stride, nslots, head;
    uint32_t slot_frame[RW_SLOTS];
} RewindShm;

static struct {
    bool session, pixel, scanlines, crtgeom, showControls, texOk, hidden, injected, menuTap, filterPoint;
    char shmPath[1300], menuDir[1200];
    void *map;
    size_t mapSize;
    const volatile RewindShm *shm;
    Display *dpy;
    Window child;
    RewindAnim anim;
    Texture2D liveTex, slotTex[RW_SLOTS];
    uint32_t slotLoaded[RW_SLOTS], seenSeq;
    float chordHeld, retry, startT, pressAlpha;
    int picX, picY, picW, picH;
} rw;

static int Rewind_XError(Display *d, XErrorEvent *e) { (void)d; (void)e; return 0; }

bool Rewind_Session(void) { return rw.session; }

static void Rewind_SetHidden(bool hide)
{
    if (!rw.dpy || !rw.child || rw.hidden == hide) return;
    int (*old)(Display *, XErrorEvent *) = XSetErrorHandler(Rewind_XError);
    if (hide) XShapeCombineRectangles(rw.dpy, rw.child, ShapeBounding, 0, 0, NULL, 0, ShapeSet, Unsorted);
    else XShapeCombineMask(rw.dpy, rw.child, ShapeBounding, 0, 0, None, ShapeSet);
    XSync(rw.dpy, False);
    XSetErrorHandler(old);
    rw.hidden = hide;
}

static void Rewind_Key(bool down)
{
    if (!rw.dpy || rw.injected == down) return;
    KeyCode code = XKeysymToKeycode(rw.dpy, XK_BackSpace);
    if (!code) return;
    XTestFakeKeyEvent(rw.dpy, code, down, CurrentTime);
    XSync(rw.dpy, False);
    rw.injected = down;
}

static void Rewind_Unload(void)
{
    if (rw.texOk) {
        UnloadTexture(rw.liveTex);
        for (int i = 0; i < RW_SLOTS; i++) UnloadTexture(rw.slotTex[i]);
        rw.texOk = false;
    }
}

void Rewind_End(void)
{
    if (!rw.session) return;
    Rewind_Key(false);
    Rewind_SetHidden(false);
    Rewind_Unload();
    if (rw.map) munmap(rw.map, rw.mapSize);
    if (rw.dpy) XCloseDisplay(rw.dpy);
    memset(&rw, 0, sizeof(rw));
}

void Rewind_Begin(const char *shmPath, const char *menuDir, bool pixel, bool scanlines, bool crtgeom, bool showControls)
{
    Rewind_End();
    if (!shmPath || !menuDir) return;
    memset(&rw, 0, sizeof(rw));
    snprintf(rw.shmPath, sizeof(rw.shmPath), "%s", shmPath);
    snprintf(rw.menuDir, sizeof(rw.menuDir), "%s", menuDir);
    rw.pixel = pixel;
    rw.scanlines = scanlines;
    rw.crtgeom = crtgeom;
    rw.showControls = showControls;
    rw.session = true;
    RewindAnim_Init(&rw.anim);
}

void Rewind_Reset(void)
{
    if (!rw.session) return;
    Rewind_Key(false);
    rw.chordHeld = 0.0f;
    RewindAnim_Init(&rw.anim);
    Rewind_SetHidden(false);
}

static void Rewind_Open(void)
{
    if (!rw.map) {
        int fd = open(rw.shmPath, O_RDONLY);
        struct stat st;
        size_t want = RW_HEAD + (size_t)RW_FRAME_BYTES * (1 + RW_SLOTS);
        if (fd >= 0 && fstat(fd, &st) == 0 && (size_t)st.st_size >= want) {
            void *m = mmap(NULL, want, PROT_READ, MAP_SHARED, fd, 0);
            if (m != MAP_FAILED) {
                const RewindShm *h = m;
                if (h->magic == RW_MAGIC) { rw.map = m; rw.mapSize = want; rw.shm = m; }
                else munmap(m, want);
            }
        }
        if (fd >= 0) close(fd);
    }
    if (!rw.dpy) rw.dpy = XOpenDisplay(NULL);
    if (!rw.child) {
        char path[1300];
        snprintf(path, sizeof(path), "%s/nso-menu-window", rw.menuDir);
        FILE *f = fopen(path, "rb");
        unsigned long id = 0;
        if (f) { if (fscanf(f, "%lu", &id) != 1) id = 0; fclose(f); }
        rw.child = (Window)id;
    }
}

static void Rewind_Textures(void)
{
    if (rw.texOk) return;
    Image img = GenImageColor(RW_FW, RW_FH, BLACK);
    rw.liveTex = LoadTextureFromImage(img);
    for (int i = 0; i < RW_SLOTS; i++) rw.slotTex[i] = LoadTextureFromImage(img);
    UnloadImage(img);
    rw.texOk = true;
    memset(rw.slotLoaded, 0, sizeof(rw.slotLoaded));
    rw.seenSeq = 0;
}

void Rewind_Frame(float dt, bool inGame, bool zlzrHeld)
{
    if (!rw.session) return;
    if (!inGame) {
        Rewind_Key(false);
        rw.chordHeld = 0.0f;
        if (rw.anim.phase) RewindAnim_Init(&rw.anim);
        Rewind_SetHidden(false);
        return;
    }
    if (rw.child) rw.startT += dt;   /* the game's window exists: the start caption's clock */
    if (!rw.map || !rw.dpy || !rw.child) {
        rw.retry -= dt;
        if (rw.retry <= 0.0f) { rw.retry = 0.25f; Rewind_Open(); }
    }
    /* ZL+ZR: held past RW_HOLD_MS = rewind (Backspace into the emulator), a shorter tap = Suspend Menu */
    if (zlzrHeld) {
        rw.chordHeld += dt;
        if (rw.chordHeld * 1000.0f >= (float)RW_HOLD_MS) Rewind_Key(true);
    } else {
        if (rw.injected) Rewind_Key(false);
        else if (rw.chordHeld > 0.0f) rw.menuTap = true;
        rw.chordHeld = 0.0f;
    }
    bool rewinding = rw.shm && rw.shm->enabled && rw.shm->rewinding;
    bool was = RewindAnim_Active(&rw.anim);
    if (!was && rewinding) rw.pressAlpha = RewindAnim_StartAlpha(rw.startT);   /* caption 1 only if it is showing */
    RewindAnim_Step(&rw.anim, rewinding, dt);
    if (!was && RewindAnim_Active(&rw.anim)) {
        XWindowAttributes at;
        int (*old)(Display *, XErrorEvent *) = XSetErrorHandler(Rewind_XError);
        if (rw.dpy && rw.child && XGetWindowAttributes(rw.dpy, rw.child, &at) && at.width > 0) {
            rw.picX = at.x; rw.picY = at.y; rw.picW = at.width; rw.picH = at.height;
        } else {
            rw.picW = GetScreenWidth(); rw.picH = GetScreenHeight(); rw.picX = rw.picY = 0;
        }
        XSetErrorHandler(old);
        Rewind_Textures();
    }
    Rewind_SetHidden(!RewindAnim_RealWindowShows(&rw.anim));
    if (RewindAnim_Active(&rw.anim) && rw.shm && rw.texOk) {
        const uint8_t *base = (const uint8_t *)rw.map + RW_HEAD;
        uint32_t seq = __atomic_load_n(&rw.shm->seq, __ATOMIC_ACQUIRE);
        if (seq != rw.seenSeq) {
            rw.seenSeq = seq;
            if (rw.shm->w && rw.shm->h)   /* packed rows of the frame's own width */
                UpdateTextureRec(rw.liveTex, (Rectangle){ 0, 0, (float)rw.shm->w, (float)rw.shm->h }, base);
        }
    }
}

bool Rewind_MenuTap(void)
{
    bool t = rw.menuTap;
    rw.menuTap = false;
    return t;
}

/* ---- drawing ---- */
typedef struct { int kind; const char *text; float gap; } RwItem;   /* kind: 0 text, 1 ZL, 2 ZR, 3 plus, 4 B, 5 A */

static float Rw_ItemW(Font f, float k, RwItem it)
{
    switch (it.kind) {
    case 1: case 2: return 28.0f * k;
    case 4: case 5: return 24.0f * k;
    default: return MeasureTextEx(f, it.kind == 3 ? "+" : it.text, 21.0f * k, 0.0f).x;
    }
}

/* Items laid out right-aligned at xr, vertically centred on cy. */
static void Rw_Row(Font f, float k, const RwItem *it, int n, float xr, float cy, float alpha)
{
    if (alpha <= 0.003f) return;
    float total = 0.0f, x;
    for (int i = 0; i < n; i++) total += Rw_ItemW(f, k, it[i]) + (i ? it[i].gap * k : 0.0f);
    x = xr - total;
    Color ink = Fade((Color){ 235, 235, 235, 255 }, alpha);
    for (int i = 0; i < n; i++) {
        if (i) x += it[i].gap * k;
        float w = Rw_ItemW(f, k, it[i]);
        switch (it[i].kind) {
        case 1: case 2: {
            Rectangle r = { x, cy - 11.0f * k, w, 22.0f * k };
            DrawRectangleRounded(r, 0.3f, 6, Fade(WHITE, alpha));
            const char *t = it[i].kind == 1 ? "ZL" : "ZR";
            Vector2 m = MeasureTextEx(f, t, 14.0f * k, 0.0f);
            DrawTextEx(f, t, (Vector2){ x + (w - m.x) * 0.5f, cy - m.y * 0.5f }, 14.0f * k, 0.0f, Fade(BLACK, alpha));
            break; }
        case 4: case 5: {
            DrawCircle((int)(x + w * 0.5f), (int)cy, w * 0.5f, Fade((Color){ 232, 232, 232, 255 }, alpha));
            const char *t = it[i].kind == 4 ? "B" : "A";
            Vector2 m = MeasureTextEx(f, t, 16.0f * k, 0.0f);
            DrawTextEx(f, t, (Vector2){ x + (w - m.x) * 0.5f, cy - m.y * 0.5f }, 16.0f * k, 0.0f, Fade(BLACK, alpha));
            break; }
        default: {
            const char *t = it[i].kind == 3 ? "+" : it[i].text;
            Vector2 m = MeasureTextEx(f, t, 21.0f * k, 0.0f);
            DrawTextEx(f, t, (Vector2){ x, cy - m.y * 0.5f }, 21.0f * k, 0.0f, ink);
            break; }
        }
        x += w;
    }
}

void Rewind_Draw(Font font, const char *capHold, const char *capSuspend, const char *capBack, const char *capOk)
{
    if (!rw.session) return;
    const float W = (float)GetScreenWidth(), H = (float)GetScreenHeight(), k = H / 720.0f;
    RewindLook look = RewindAnim_Look(&rw.anim);
    bool active = RewindAnim_Active(&rw.anim) && rw.texOk;
    const volatile RewindShm *s = rw.shm;
    if (active && s && s->w && s->h) {
        float pw = (float)rw.picW, ph = (float)rw.picH, aspect = pw / ph;
        float q = (1.0f - look.scale) / (1.0f - rewind_table[92 - REWIND_F0].scale);
        if (q > 1.0f) q = 1.0f;
        /* the shrunken game ends just above the panel (the reference: 0.95 of the screen height at its
         * 74.9% width), so a window taller than that shrinks a little further */
        float fit = 0.95f * H / ph;
        if (fit > 1.0f) fit = 1.0f;
        float sc = look.scale * (1.0f + (fit - 1.0f) * q);
        float w = pw * sc, h = ph * sc;
        float x = (float)rw.picX + pw * 0.5f - w * 0.5f, y = (float)rw.picY * (1.0f - q);
        bool point = rw.pixel && look.scale > 0.999f;
        if (point != rw.filterPoint) {
            SetTextureFilter(rw.liveTex, point ? TEXTURE_FILTER_POINT : TEXTURE_FILTER_BILINEAR);
            rw.filterPoint = point;
        }
        Rectangle src = { 0, 0, (float)s->w, (float)s->h };
        bool crt = rw.crtgeom && CrtGeom_Ready();
        if (RewindAnim_RealWindowShows(&rw.anim) == false) {
            if (crt) CrtGeom_Draw(rw.liveTex, src, (Rectangle){ x, y, w, h }, rw.scanlines, WHITE);
            else DrawTexturePro(rw.liveTex, src, (Rectangle){ x, y, w, h }, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }
        if (rw.scanlines && !crt && look.scale > 0.9f && !RewindAnim_RealWindowShows(&rw.anim)) {
            /* Mednafen's scanlines (90%): the lower half of every source row, only while the
             * picture is close to its real size where the lines are still clean */
            float row = h / (float)s->h, a = 0.9f * fminf(1.0f, (look.scale - 0.9f) / 0.1f);
            for (int r = 0; r < (int)s->h; r++)
                DrawRectangle((int)x, (int)(y + ((float)r + 0.5f) * row), (int)w, (int)(row * 0.5f + 0.5f), Fade(BLACK, a));
        }
        /* gray panel */
        float top = look.panelTop * H;
        if (look.panelAlpha > 0.003f)
            DrawRectangle(0, (int)top, (int)W, (int)(H - top) + 1,
                          Fade((Color){ REWIND_PANEL_GRAY, REWIND_PANEL_GRAY, REWIND_PANEL_GRAY, 255 }, look.panelAlpha));
        /* thumbnails: the selected (live) frame centred, every frame that can go back to its left */
        if (look.thumbAlpha > 0.003f) {
            float th = REWIND_THUMB_H * k * look.thumbScale, tw = th * aspect;
            float step = tw + REWIND_GAP * k, cx = W * 0.5f, ty = top + REWIND_THUMB_DY * k;
            const uint8_t *base = (const uint8_t *)rw.map + RW_HEAD;
            int frame = (int)s->frame, stride = (int)s->stride ? (int)s->stride : 60;
            for (int i = 0; i < RW_SLOTS; i++) {
                uint32_t sf = s->slot_frame[i];
                if (!sf || (int)sf > frame + stride || frame - (int)sf >= (int)s->maxhist) continue;
                float tx = RewindStrip_X(cx, step, frame, (int)sf, stride) - tw * 0.5f;
                if (tx + tw < 0.0f || tx > W) continue;
                float a = RewindStrip_Alpha(frame, (int)sf, stride) * look.thumbAlpha * 0.7f;
                if (a <= 0.003f) continue;
                if (rw.slotLoaded[i] != sf) {
                    UpdateTextureRec(rw.slotTex[i], (Rectangle){ 0, 0, (float)s->w, (float)s->h },
                                     base + (size_t)RW_FRAME_BYTES * (1 + i));
                    rw.slotLoaded[i] = sf;
                    SetTextureFilter(rw.slotTex[i], TEXTURE_FILTER_BILINEAR);
                }
                DrawTexturePro(rw.slotTex[i], src, (Rectangle){ tx, ty, tw, th }, (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, a));
            }
            float bw = 2.0f * k;
            Rectangle sel = { cx - tw * 0.5f, ty, tw, th };
            DrawRectangleRec((Rectangle){ sel.x - bw, sel.y - bw, sel.width + 2 * bw, sel.height + 2 * bw }, Fade(WHITE, look.thumbAlpha));
            DrawTexturePro(rw.liveTex, src, sel, (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, look.thumbAlpha));
        }
    }
    if (!rw.showControls) return;
    float barH = REWIND_BAR_H * H, cy = H - barH * 0.5f, xr = W - 39.0f * k;
    if (!RewindAnim_Active(&rw.anim)) {   /* not rewinding: only the start-of-game caption */
        look.bar = look.cap1 = RewindAnim_StartAlpha(rw.startT);
        look.cap2 = 0.0f;
    } else if (rw.anim.f < (float)REWIND_FSPLIT) {   /* leaving or entering: caption 1 only if it was showing */
        look.bar *= rw.pressAlpha;
        look.cap1 *= rw.pressAlpha;
    }
    if (look.bar > 0.003f) DrawRectangle(0, (int)(H - barH), (int)W, (int)barH + 1, Fade(BLACK, look.bar));
    const RwItem hold[] = { { 1, NULL, 0 }, { 3, NULL, 9 }, { 2, NULL, 9 }, { 0, capHold, 9 } };
    Rw_Row(font, k, hold, 4, xr, cy, look.cap1);
    const RwItem second[] = { { 1, NULL, 0 }, { 3, NULL, 9 }, { 2, NULL, 9 }, { 0, capSuspend, 9 },
                              { 4, NULL, 34 }, { 0, capBack, 9 }, { 5, NULL, 34 }, { 0, capOk, 9 } };
    Rw_Row(font, k, second, 8, xr, cy, look.cap2);
}

#else   /* not Linux: no rewind */
void Rewind_Begin(const char *a, const char *b, bool c, bool d, bool e, bool f) { (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; }
void Rewind_Reset(void) {}
void Rewind_End(void) {}
void Rewind_Frame(float dt, bool a, bool b) { (void)dt; (void)a; (void)b; }
bool Rewind_MenuTap(void) { return false; }
bool Rewind_Session(void) { return false; }
void Rewind_Draw(Font f, const char *a, const char *b, const char *c, const char *d) { (void)f; (void)a; (void)b; (void)c; (void)d; }
#endif
#endif
#endif
#endif
