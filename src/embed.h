/* embed.h - games that run inside NSO's window instead of as a child process.
 *
 * A GameEntry with embedRomKey set names an in-process module (dir/binary,
 * e.g. embed/smb/build/libsmbmod.so; API in embed/nsomod.h). NSO dlopen()s
 * it, calls one game frame per console frame from its own loop, uploads the
 * RGB framebuffer (NES 256x240, SNES 256x224, N64 640x480) to a texture drawn per
 * the system's screen settings, feeds the samples (mono or stereo) to a raylib
 * AudioStream, and draws a pause menu on top (End or
 * the Guide button) laid out like Switch Online's: items right-aligned left
 * of a divider over the blurred, dimmed, frozen game frame. Load / Create
 * Suspend Point list four slots right of the divider, kept as the grid's
 * Save States files ("<stateDir>/Suspend point N.sav" + .bmp thumbnail).
 *
 * The inline helpers are pure (no raylib calls) and unit-tested in
 * embed_test.c; the rest is the EMBED_IMPLEMENTATION part main.c includes. */
#ifndef NSO_EMBED_H
#define NSO_EMBED_H

#include <math.h>
#include <stdbool.h>
#include "raylib.h"
#include "../embed/nsomod.h"
#include "padmap.h"
#include "crtgeom.h"

/* Legacy "<system>.screen" values for N64/GameCube embeds (not shown in Settings).
 * NES/SNES use EmbedScreen (Pixel Perfect + Scanlines toggles) instead. */
enum { EMBED_SCREEN_43 = 0, EMBED_SCREEN_PIXEL = 1, EMBED_SCREEN_CRT = 2 };
typedef struct { bool pixel; bool scanlines; bool fill43; bool crtgeom; } EmbedScreen;   /* crtgeom: shaders/crtgeom.glsl */
enum { EMBED_MENU_CONTINUE = 0, EMBED_MENU_LOAD, EMBED_MENU_SAVE, EMBED_MENU_RESTART, EMBED_MENU_CRT,
       EMBED_MENU_QUIT, EMBED_MENU_COUNT };
/* Embed_Draw texts: the items, then the slot label ("%d"), empty slot, date (strftime), loading, and the
 * CRT Geometry sub-screen's twelve slider labels (crtparams.h order) and its reset row. */
enum { EMBED_TXT_SLOT = EMBED_MENU_COUNT, EMBED_TXT_NODATA, EMBED_TXT_DATE,
       EMBED_TXT_LOADING, EMBED_TXT_CRT0, EMBED_TXT_CRT_RESET = EMBED_TXT_CRT0 + CRTP_COUNT, EMBED_TXT_COUNT };
#define EMBED_SLOTS 4
enum { EMBED_EXTERNAL_NONE, EMBED_EXTERNAL_RESUME, EMBED_EXTERNAL_QUIT, EMBED_EXTERNAL_RESTART };

/* Where a fw x fh picture goes on a sw x sh screen.
 * Pixel Perfect: largest integer scale, square pixels (1080p: 4x, NES 1024x960).
 * 4:3 and CRT: full height at 4:3 (1080p: 1440x1080). Centred, whole pixels.
 * Used for N64/GameCube embeds. */
static inline Rectangle Embed_DestRect(int mode, float sw, float sh, int fw, int fh)
{
    float w, h;
    if (mode == EMBED_SCREEN_PIXEL) {
        int s = (int)fminf(sw / fw, sh / fh);
        if (s < 1) s = 1;
        w = (float)(fw * s);
        h = (float)(fh * s);
    } else {
        h = sh;
        w = floorf(h * 4.0f / 3.0f);
        if (w > sw) { w = sw; h = floorf(w * 3.0f / 4.0f); }
    }
    return (Rectangle){ floorf((sw - w) * 0.5f), floorf((sh - h) * 0.5f), w, h };
}

/* NES/SNES Game Screen: Pixel Perfect on = largest integer scale, square
 * pixels; off = as large as fits (non-integer), same aspect. Centred, whole
 * pixels. */
static inline Rectangle Embed_DestRectFit(bool pixel, float sw, float sh, int fw, int fh)
{
    float w, h;
    if (pixel) {
        int s = (int)fminf(sw / (float)fw, sh / (float)fh);
        if (s < 1) s = 1;
        w = (float)(fw * s);
        h = (float)(fh * s);
    } else {
        float s = fminf(sw / (float)fw, sh / (float)fh);
        if (s < 1.0f) s = 1.0f;
        w = floorf((float)fw * s);
        h = floorf((float)fh * s);
    }
    return (Rectangle){ floorf((sw - w) * 0.5f), floorf((sh - h) * 0.5f), w, h };
}

static inline Rectangle Embed_DestForScreen(EmbedScreen sc, float sw, float sh, int fw, int fh)
{
    if (sc.fill43) return Embed_DestRect(EMBED_SCREEN_43, sw, sh, fw, fh);
    return Embed_DestRectFit(sc.pixel, sw, sh, fw, fh);
}

/* NSO draws at 60 FPS, the console ran at fps (NES/SNES ~60.0988): frames to
 * run this tick from an accumulator (seconds). At most 2 per tick; a longer
 * stall is dropped instead of fast-forwarding. */
static inline int Embed_PaceFrames(double *acc, double dt, double fps)
{
    const double step = 1.0 / fps;
    if (dt > 0.1) dt = 0.1;
    *acc += dt;
    int n = 0;
    while (*acc >= step && n < 2) { *acc -= step; n++; }
    if (*acc >= step) *acc = 0.0;
    return n;
}

/* Pause menu entry/exit, t = linear 0..1 over EMBED_MENU_ANIM_SEC:
 * Switch Online's ease-out (fitted to its 60 fps capture). Exit runs it backwards. */
#define EMBED_MENU_ANIM_SEC 0.22f
static inline float Embed_MenuEase(float t)
{
    t = fminf(1.0f, fmaxf(0.0f, t));
    return 1.0f - powf(1.0f - t, 1.6f);
}

/* Card <-> game zoom (Switch Online capture). Back to the grid (frames
 * 5848-5861): after the menu exit the frozen frame shrinks into the selected
 * card while fading. Launch, every game (frames 5519-5533): a dark panel
 * grows out of the card to the game picture, same ease, grid dimming. */
#define EMBED_BACK_SEC 0.35f
static inline float Embed_BackEase(float t)
{
    t = fminf(1.0f, fmaxf(0.0f, t));
    return 1.0f - powf(1.0f - t, 4.0f);
}

/* Loading a suspend point (frames 144-159): after the menu exit the screen
 * cuts to black, holds, then fades back in. Black's opacity, t = seconds left. */
#define EMBED_LOAD_SEC 0.40f
#define EMBED_LOAD_FADE_SEC 0.07f
static inline float Embed_LoadBlack(float t)
{
    return fminf(1.0f, fmaxf(0.0f, t / EMBED_LOAD_FADE_SEC));
}

/* p = 0: card, 1: game rect. */
static inline Rectangle Embed_ZoomRect(Rectangle card, Rectangle game, float p)
{
    return (Rectangle){ card.x + (game.x - card.x) * p, card.y + (game.y - card.y) * p,
                        card.width + (game.width - card.width) * p, card.height + (game.height - card.height) * p };
}

/* dataPath: the module's game data (nsomod.h); stateDir: where suspend points go
 * (NULL: none); stateLoad: a state file to resume, or NULL. */
bool Embed_Start(const char *modulePath, const char *dataPath, const char *stateDir, const char *stateLoad);
/* The player's NES controller mapping (padmap.h) for the NES game about to start; NULL = the
 * default controls. */
void Embed_SetPadMap(const PadSysMap *map, int kind);   /* kind: 0 NES, 1 SNES, 2 N64 */
/* One NSO frame: input, game frames, audio. false = the player chose Quit. */
bool Embed_Update(float dt);
/* blur/blurDirLoc: main.c's separable 5-tap shader (id 0 = downscale only). */
void Embed_Draw(Font font, EmbedScreen screen, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc);
/* After Embed_Update returned false: the frozen frame at t (0..1) of the way into card. */
void Embed_DrawBack(EmbedScreen screen, Rectangle card, float t);
void Embed_DrawExternalResumeFrame(EmbedScreen screen);
void Embed_DrawZoom(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim);
/* The same, the game frame drawn with CRT Geometry (crt) and its beam scanlines (scan). */
void Embed_DrawZoomCrt(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim, bool crt, bool scan);
float Embed_MenuOpacity(void);
/* The CRT Geometry sub-screen of the pause menu is open (main.c switches the info bar). */
bool Embed_CrtOpen(void);
void Embed_Stop(void);
/* Full teardown: Embed_Stop plus discarding a keep_loaded module (switch game / quit NSO). */
void Embed_Unload(void);
bool Embed_StartExternal(const char *stateDir, unsigned long childWindow, bool stateSlots);
void Embed_ExternalSetChildVisible(bool visible);
/* Raise a parked external child's window again (the overlay is already gone). */
void Embed_ShowExternalWindow(unsigned long window);
/* Result is 0 when no child-menu result is pending, otherwise resume/quit/restart.
 * EMBED_EXTERNAL_QUIT now means "To Game Selection": keep the child paused; main parks it. */
int Embed_ExternalResult(void);
bool Embed_IsActive(void);
bool Embed_NeedsFastExit(void);
bool Embed_IsParked(void);
bool Embed_IsExternal(void);
unsigned long Embed_ExternalWindow(void);
const char *Embed_StateDir(void);
/* Pause audio and stop ticking; keep the module/session for an instant resume. */
void Embed_Park(void);
void Embed_Unpark(void);
/* Write an action for a parked external child (menuDir from Embed_StateDir before Stop). */
bool Embed_WriteMenuAction(const char *menuDir, const char *action, int slot);
/* True once To Game Selection was chosen, including while the menu is still closing. */
bool Embed_IsLeaving(void);
/* Override-redirect black window raised above a hosted child, opacity 0..1.
 * With no compositor the window is mapped only at full opacity, so a GL fade stays visible. */
void Embed_ExitOverlaySet(float opacity);
void Embed_ExitOverlayDestroy(void);
/* True when there is no hosted child, or it is unmapped or destroyed. */
bool Embed_ExternalChildGone(void);

#endif /* NSO_EMBED_H */

#ifdef EMBED_IMPLEMENTATION
#ifndef EMBED_IMPLEMENTATION_ONCE
#define EMBED_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "nso.h"
#include "savestate.h"
#include "sfx.h"
#include "n64pad.h"

bool Rewind_LiveFrame(Texture2D *tex, Rectangle *src);   /* rewind.h */

/* Screen dimmed by dim, then tex (id 0: a black panel) at p of the way from card to game. */
void Embed_DrawZoomCrt(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim, bool crt, bool scan)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, dim));
    Rectangle r = Embed_ZoomRect(card, game, p);
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    if (tex.id && crt) CrtGeom_Draw(tex, src, r, scan, Fade(WHITE, alpha));
    else if (tex.id) DrawTexturePro(tex, src, r, (Vector2){ 0 }, 0.0f, Fade(WHITE, alpha));
    else DrawRectangleRec(r, Fade(BLACK, alpha));
}

void Embed_DrawZoom(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim)
{
    Embed_DrawZoomCrt(tex, game, card, p, alpha, dim, false, false);
}

static const PadSysMap *embedPadMap = NULL;   /* a custom controller mapping (padmap.h), NULL = the defaults */
static int embedPadKind = 0;                   /* ... of this system: 0 NES, 1 SNES, 2 N64 */
void Embed_SetPadMap(const PadSysMap *map, int kind) { embedPadMap = map; embedPadKind = kind; }

#if defined(_WIN32)
/* Not built for Windows yet: embedded cards stay unlaunchable there. */
bool Embed_Start(const char *modulePath, const char *dataPath, const char *stateDir, const char *stateLoad)
{ (void)modulePath; (void)dataPath; (void)stateDir; (void)stateLoad; return false; }
bool Embed_StartExternal(const char *stateDir, unsigned long childWindow, bool stateSlots)
{ (void)stateDir; (void)childWindow; (void)stateSlots; return false; }
void Embed_ExternalSetChildVisible(bool visible) { (void)visible; }
void Embed_ShowExternalWindow(unsigned long window) { (void)window; }
int Embed_ExternalResult(void) { return EMBED_EXTERNAL_NONE; }
bool Embed_Update(float dt) { (void)dt; return false; }
void Embed_Draw(Font font, EmbedScreen screen, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc)
{ (void)font; (void)screen; (void)text; (void)blur; (void)blurDirLoc; }
void Embed_DrawBack(EmbedScreen screen, Rectangle card, float t) { (void)screen; (void)card; (void)t; }
void Embed_DrawZoomCrt(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim, bool crt, bool scan)
{ (void)tex; (void)game; (void)card; (void)p; (void)alpha; (void)dim; (void)crt; (void)scan; }
void Embed_DrawExternalResumeFrame(EmbedScreen screen) { (void)screen; }
float Embed_MenuOpacity(void) { return 0.0f; }
bool Embed_CrtOpen(void) { return false; }
void Embed_Stop(void) {}
void Embed_Unload(void) {}
bool Embed_IsActive(void) { return false; }
bool Embed_NeedsFastExit(void) { return false; }
bool Embed_IsParked(void) { return false; }
bool Embed_IsExternal(void) { return false; }
unsigned long Embed_ExternalWindow(void) { return 0; }
const char *Embed_StateDir(void) { return ""; }
void Embed_Park(void) {}
void Embed_Unpark(void) {}
bool Embed_WriteMenuAction(const char *menuDir, const char *action, int slot)
{ (void)menuDir; (void)action; (void)slot; return false; }
bool Embed_IsLeaving(void) { return false; }
void Embed_ExitOverlaySet(float opacity) { (void)opacity; }
void Embed_ExitOverlayDestroy(void) {}
bool Embed_ExternalChildGone(void) { return true; }
#else
#include <dlfcn.h>
#include <pthread.h>
#include <stdatomic.h>
#if defined(__linux__)
#define Font X11Font
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#undef Font
#endif

#define EMBED_SAMPLE_RATE 48000
#define EMBED_AUDIO_CHUNK 1024
/* Menu backdrop: the frozen frame at half resolution plus a black
 * margin (so the blur bleeds into the bars), blurred once per opening. */
#define EMBED_BLUR_W (embed.fbw / 2)
#define EMBED_BLUR_H (embed.fbh / 2)
#define EMBED_BLUR_PAD 16

static struct {
    bool active;
    bool external;
    bool externalResumeRequested;
    bool externalRestartRequested;
    bool externalLoadRequested;
    bool externalSlots;
    int externalResult;
    float externalScanTimer;
    unsigned long externalWindow;
    unsigned long externalParent;
    void *externalDisplay;
    unsigned char *externalFrameRGB;
    NsoPadAxisArm navArmX, navArmY;   /* left stick as menu d-pad (edge, like the d-pad buttons) */
    int navX, navY;                   /* this frame: -1 / 0 / +1 */
    void *so;
    bool reusedRetainedSo;
    bool moduleInitialized;
    const NsomodApi *api;
    nsomod_rumble_fn rumble;               /* optional (nsomod.h) */
    NsomodConfig cfg;
    char data[2048], module[2048];
    int fbw, fbh, channels;   /* the module's, kept even if a reload fails */
    double fps;
    Texture2D tex;
    bool audio, ownAudioDevice;
    AudioStream stream;
    int16_t pend[2 * EMBED_AUDIO_CHUNK];   /* up to stereo */
    size_t pendN;                          /* sample frames */
    double acc;
    bool menuOpen;
    int menuSel;
    float menuT;
    bool crtOpen, crtWait;                 /* CRT Geometry sub-screen; crtWait: a held key must be let go first */
    float crtT, crtRepT, crtHold;          /* its slide (0..1), the Left/Right repeat clock, the A hold (s) */
    int crtSel, crtRepDir;
    bool crtHoldDone;
    signed char crtSteps[CRTP_COUNT];      /* the game's steps being edited (-1 = unset: inherits) */
    RenderTexture2D blurA, blurB;
    bool blurReady;
    bool quitting;   /* To Game Selection: false once the menu exit ends */
    bool parked;     /* left running behind the grid; audio paused, no ticks */
    char stateDir[1100];                   /* "" = no suspend points */
    const uint8_t *last;                   /* last frame (thumbnail), module-owned */
    bool slotsFocus;
    int slotSel, loadSlot;                 /* loadSlot: 1..EMBED_SLOTS once the menu exit ends */
    float blackT;                          /* Embed_LoadBlack countdown */
    long slotTime[EMBED_SLOTS];            /* .sav mtime, 0 = empty */
    char statusText[96];
    float statusTimer;
    Texture2D thumb[EMBED_SLOTS];
    pthread_t loadThread;
    bool loadThreadStarted;
    _Atomic bool loadDone, loadOk;
    char loadPath[1200];
} embed;
static void *embedRetainedSo;
/* A keep_loaded engine leaves worker threads whose static destructors block at exit(). */
static bool embedPersistentSeen;
static char embedRetainedPath[2048];

static bool Embed_BeginLoad(const char *path);
static void Embed_SetMenu(bool open);

static void Embed_DiscardRetainedModule(void)
{
    if (!embedRetainedSo) return;
    nsomod_get_api_fn get = (nsomod_get_api_fn)dlsym(embedRetainedSo, NSOMOD_ENTRY);
    const NsomodApi *api = get ? get() : NULL;
    if (api && api->keep_loaded) embedPersistentSeen = true;
    if (api && api->keep_loaded && api->shutdown) api->shutdown();
    dlclose(embedRetainedSo);
    embedRetainedSo = NULL;
    embedRetainedPath[0] = '\0';
}

static void Embed_SetStatus(const char *message)
{
    snprintf(embed.statusText, sizeof(embed.statusText), "%s", message ? message : "");
    embed.statusTimer = 2.5f;
}

bool Embed_Start(const char *modulePath, const char *dataPath, const char *stateDir, const char *stateLoad)
{
    if (embed.active) return false;
    memset(&embed, 0, sizeof(embed));
    if (embedRetainedSo && strcmp(embedRetainedPath, modulePath) != 0)
        Embed_DiscardRetainedModule();
    if (embedRetainedSo && strcmp(embedRetainedPath, modulePath) == 0) {
        embed.so = embedRetainedSo;
        embed.reusedRetainedSo = true;
    } else {
        embed.so = dlopen(modulePath, RTLD_NOW | RTLD_LOCAL);
    }
    if (!embed.so) return false;
    nsomod_get_api_fn get = (nsomod_get_api_fn)dlsym(embed.so, NSOMOD_ENTRY);
    embed.api = get ? get() : NULL;
    embed.rumble = (nsomod_rumble_fn)dlsym(embed.so, NSOMOD_RUMBLE_ENTRY);
    snprintf(embed.data, sizeof(embed.data), "%s", dataPath);
    snprintf(embed.module, sizeof(embed.module), "%s", modulePath);
    embed.cfg = (NsomodConfig){ .data_path = embed.data, .sample_rate = EMBED_SAMPLE_RATE };
    if (!embed.api || embed.api->version != NSOMOD_API_VERSION || embed.api->fb_w <= 0 || embed.api->fb_h <= 0
        || (embed.api->channels != 1 && embed.api->channels != 2) || !embed.api->init(&embed.cfg)) {
        if (embed.so && !embed.reusedRetainedSo) dlclose(embed.so);
        memset(&embed, 0, sizeof(embed));
        return false;
    }
    embed.moduleInitialized = true;
    if (stateLoad && stateLoad[0] && !embed.api->state_load) {
        embed.api->shutdown();
        embed.moduleInitialized = false;
        if (!embed.reusedRetainedSo) dlclose(embed.so);
        memset(&embed, 0, sizeof(embed));
        return false;
    }
    embed.fbw = embed.api->fb_w;
    embed.fbh = embed.api->fb_h;
    embed.channels = embed.api->channels;
    embed.fps = embed.api->fps;
    if (stateDir && embed.api->state_save && embed.api->state_load
        && (DirectoryExists(stateDir) || MakeDirectory(stateDir) == 0))
        snprintf(embed.stateDir, sizeof(embed.stateDir), "%s", stateDir);

    Image img = GenImageColor(embed.fbw, embed.fbh, BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    embed.tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(embed.tex, TEXTURE_FILTER_POINT);

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        embed.ownAudioDevice = IsAudioDeviceReady();
    }
    if (IsAudioDeviceReady()) {
        SetAudioStreamBufferSizeDefault(EMBED_AUDIO_CHUNK);
        embed.stream = LoadAudioStream(EMBED_SAMPLE_RATE, 16, (unsigned int)embed.channels);
        SetAudioStreamBufferSizeDefault(0);
        embed.audio = IsAudioStreamValid(embed.stream);
        if (embed.audio) PlayAudioStream(embed.stream);
    }
    atomic_init(&embed.loadDone, false);
    atomic_init(&embed.loadOk, false);

    SetExitKey(KEY_NULL);   /* ESC must not close NSO mid-game */
    embed.active = true;
    if (stateLoad && stateLoad[0] && !Embed_BeginLoad(stateLoad)) {
        Embed_Stop();
        return false;
    }
    return true;
}

void Embed_Stop(void)
{
    if (!embed.active) return;
    N64Pad_Rumble(0.0f);
    if (embed.loadThreadStarted) {
        pthread_join(embed.loadThread, NULL);
        embed.loadThreadStarted = false;
    }
    if (embed.so) {
        if (embed.api && embed.api->keep_loaded) {
            /* A persistent module may keep its live session while NSO is at
               Game Selection. Re-entry calls init() to resume that session. */
            embedRetainedSo = embed.so;
            embedPersistentSeen = true;
            snprintf(embedRetainedPath, sizeof(embedRetainedPath), "%s", embed.module);
        } else {
            if (embed.moduleInitialized && embed.api) embed.api->shutdown();
            embed.moduleInitialized = false;
            dlclose(embed.so);
            if (embed.so == embedRetainedSo) {
                embedRetainedSo = NULL;
                embedRetainedPath[0] = '\0';
            }
        }
    }
    if (embed.audio) UnloadAudioStream(embed.stream);
    if (embed.ownAudioDevice) CloseAudioDevice();
    UnloadTexture(embed.tex);
    if (embed.blurA.id) { UnloadRenderTexture(embed.blurA); UnloadRenderTexture(embed.blurB); }
    for (int i = 0; i < EMBED_SLOTS; i++) if (embed.thumb[i].id) UnloadTexture(embed.thumb[i]);
#if defined(__linux__)
    if (embed.externalDisplay) XCloseDisplay((Display *)embed.externalDisplay);
#endif
    free(embed.externalFrameRGB);
    SetExitKey(KEY_ESCAPE);
    memset(&embed, 0, sizeof(embed));
}

void Embed_Unload(void)
{
    Embed_Stop();
    Embed_DiscardRetainedModule();
}

bool Embed_IsActive(void) { return embed.active; }
bool Embed_IsLeaving(void) { return embed.active && embed.quitting; }
bool Embed_NeedsFastExit(void) { return embedPersistentSeen; }
bool Embed_IsParked(void) { return embed.active && embed.parked; }
bool Embed_IsExternal(void) { return embed.active && embed.external; }
unsigned long Embed_ExternalWindow(void) { return embed.externalWindow; }
const char *Embed_StateDir(void) { return embed.stateDir; }

void Embed_Park(void)
{
    if (!embed.active || embed.parked) return;
    N64Pad_Rumble(0.0f);
    if (embed.audio) PauseAudioStream(embed.stream);
    embed.menuOpen = false;
    embed.menuT = 0.0f;
    embed.crtOpen = false;
    embed.crtT = 0.0f;
    embed.quitting = false;
    embed.externalResumeRequested = false;
    embed.externalRestartRequested = false;
    embed.acc = 0.0;
    embed.parked = true;
    SetExitKey(KEY_ESCAPE);   /* at Game Selection ESC quits NSO as usual (the game is closed first) */
}

void Embed_Unpark(void)
{
    if (!embed.active || !embed.parked) return;
    embed.parked = false;
    SetExitKey(KEY_NULL);
    if (embed.audio && embed.blackT <= 0.0f && !embed.loadThreadStarted)
        ResumeAudioStream(embed.stream);
}

bool Embed_WriteMenuAction(const char *menuDir, const char *action, int slot)
{
    char path[1200], tmp[1240];
    int n;
    if (!menuDir || !menuDir[0] || !action || !action[0]) return false;
    n = snprintf(path, sizeof(path), "%s/nso-menu-action", menuDir);
    if (n < 0 || (size_t)n >= sizeof(path)) return false;
    n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fprintf(f, "%s %d\n", action, slot) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok && rename(tmp, path) != 0) ok = false;
    if (!ok) remove(tmp);
    return ok;
}

static bool Embed_BindDown(const PadBind *b)
{
    return (b->key && IsKeyDown(b->key)) || (b->pad >= 0 && IsGamepadAvailable(0) && IsGamepadButtonDown(0, b->pad));
}

/* Keyboard: arrows, X = A, Z = B, Enter = Start, Right Shift = Select (an NES or SNES game
 * with a custom controller mapping, padmap.h, reads its buttons from it);
 * SNES also S = X, A = Y, C = L, V = R (the smw port's own defaults);
 * N64 also Left Shift = Z, I / J / K / L = C buttons (right stick); an N64 custom map
 * (embedPadMap, kind 2) replaces all of these: D-pad, stick directions (full tilt, the analog stick still
 * counts when none is held), Start, A, B, Z, L, R and the C directions.
 * Gamepad: D-pad / left stick, Nintendo A / B / X / Y (nso.h), L1 / R1, Start, Select;
 * N64 also the sticks as analog, ZL / ZR = Z. */
static NsomodInput Embed_ReadPad(void)
{
    NsomodInput in = {
        .up = IsKeyDown(KEY_UP), .down = IsKeyDown(KEY_DOWN),
        .left = IsKeyDown(KEY_LEFT), .right = IsKeyDown(KEY_RIGHT),
        .a = IsKeyDown(KEY_X), .b = IsKeyDown(KEY_Z), .x = IsKeyDown(KEY_S), .y = IsKeyDown(KEY_A),
        .l = IsKeyDown(KEY_C), .r = IsKeyDown(KEY_V),
        .start = IsKeyDown(KEY_ENTER), .select = IsKeyDown(KEY_RIGHT_SHIFT),
        .rx = (float)(IsKeyDown(KEY_L) - IsKeyDown(KEY_J)), .ry = (float)(IsKeyDown(KEY_K) - IsKeyDown(KEY_I)),
        .z = IsKeyDown(KEY_LEFT_SHIFT),
    };
    if (IsGamepadAvailable(0)) {
        float lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        float rx, ry;
        N64Pad_RightAxes(0, &rx, &ry);   /* Switch-mode N64 pad: its C buttons */
        in.lx = lx;
        in.ly = ly;
        if (fabsf(rx) > fabsf(in.rx)) in.rx = rx;
        if (fabsf(ry) > fabsf(in.ry)) in.ry = ry;
        in.z |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_2) || IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
        in.up |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || ly < -0.5f;
        in.down |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || ly > 0.5f;
        in.left |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || lx < -0.5f;
        in.right |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || lx > 0.5f;
        in.a |= IsGamepadButtonDown(0, NSO_BTN_A);
        in.b |= IsGamepadButtonDown(0, NSO_BTN_B);
        in.x |= IsGamepadButtonDown(0, NSO_BTN_X);
        in.y |= IsGamepadButtonDown(0, NSO_BTN_Y);
        in.l |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1);
        in.r |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1);
        in.start |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT);
        in.select |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_LEFT);
    }
    if (embedPadMap && embedPadKind == 2) {   /* N64: D-pad, stick (digital, over the analog), buttons, C buttons */
        const PadSysMap *m = embedPadMap;
        bool gp = IsGamepadAvailable(0);
        float lx = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) : 0.0f;
        float ly = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) : 0.0f;
        float rx = 0.0f, ry = 0.0f;
        float sx = (float)(Embed_BindDown(&m->b[N64_BTN_SRIGHT]) - Embed_BindDown(&m->b[N64_BTN_SLEFT]));
        float sy = (float)(Embed_BindDown(&m->b[N64_BTN_SDOWN]) - Embed_BindDown(&m->b[N64_BTN_SUP]));
        float cx = (float)(Embed_BindDown(&m->b[N64_BTN_CRIGHT]) - Embed_BindDown(&m->b[N64_BTN_CLEFT]));
        float cy = (float)(Embed_BindDown(&m->b[N64_BTN_CDOWN]) - Embed_BindDown(&m->b[N64_BTN_CUP]));
        if (gp) N64Pad_RightAxes(0, &rx, &ry);
        in.up = Embed_BindDown(&m->b[N64_BTN_UP]);
        in.down = Embed_BindDown(&m->b[N64_BTN_DOWN]);
        in.left = Embed_BindDown(&m->b[N64_BTN_LEFT]);
        in.right = Embed_BindDown(&m->b[N64_BTN_RIGHT]);
        in.lx = sx != 0.0f ? sx : lx;
        in.ly = sy != 0.0f ? sy : ly;
        in.rx = cx != 0.0f ? cx : rx;
        in.ry = cy != 0.0f ? cy : ry;
        in.start = Embed_BindDown(&m->b[N64_BTN_START]);
        in.a = Embed_BindDown(&m->b[N64_BTN_A]);
        in.b = Embed_BindDown(&m->b[N64_BTN_B]);
        in.z = Embed_BindDown(&m->b[N64_BTN_Z]);
        in.l = Embed_BindDown(&m->b[N64_BTN_L]);
        in.r = Embed_BindDown(&m->b[N64_BTN_R]);
        in.x = in.y = in.select = false;
    } else if (embedPadMap) {
        const PadSysMap *m = embedPadMap;
        bool gp = IsGamepadAvailable(0);
        float lx = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) : 0.0f;
        float ly = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y) : 0.0f;
        in.up = Embed_BindDown(&m->b[NES_BTN_UP]) || ly < -0.5f;
        in.down = Embed_BindDown(&m->b[NES_BTN_DOWN]) || ly > 0.5f;
        in.left = Embed_BindDown(&m->b[NES_BTN_LEFT]) || lx < -0.5f;
        in.right = Embed_BindDown(&m->b[NES_BTN_RIGHT]) || lx > 0.5f;
        in.select = Embed_BindDown(&m->b[NES_BTN_SELECT]);
        in.start = Embed_BindDown(&m->b[NES_BTN_START]);
        if (embedPadKind == 1) {   /* the SNES order; ZL / ZR are not the games' */
            in.b = Embed_BindDown(&m->b[SNES_BTN_B]);
            in.a = Embed_BindDown(&m->b[SNES_BTN_A]);
            in.y = Embed_BindDown(&m->b[SNES_BTN_Y]);
            in.x = Embed_BindDown(&m->b[SNES_BTN_X]);
            in.l = Embed_BindDown(&m->b[SNES_BTN_L]);
            in.r = Embed_BindDown(&m->b[SNES_BTN_R]);
        } else {
            in.b = Embed_BindDown(&m->b[NES_BTN_B]);
            in.a = Embed_BindDown(&m->b[NES_BTN_A]);
        }
    }
    return in;
}

/* Samples go out in whole chunks; while none is ready raylib plays silence. */
static void Embed_PumpAudio(void)
{
    if (!embed.audio) return;
    for (;;) {
        embed.pendN += embed.api->audio_pull(embed.pend + embed.pendN * embed.channels,
                                             EMBED_AUDIO_CHUNK - embed.pendN);
        if (embed.pendN < EMBED_AUDIO_CHUNK || !IsAudioStreamProcessed(embed.stream)) return;
        UpdateAudioStream(embed.stream, embed.pend, EMBED_AUDIO_CHUNK);
        embed.pendN = 0;
    }
}

/* Reset Game: a fresh copy of the module, so no static of the port survives
 * (smw cannot reset itself without a ROM). */
static bool Embed_Reload(void)
{
    if (!embed.so || !embed.api || !embed.moduleInitialized) return false;
    if (embed.api->keep_loaded) {
        embed.api->shutdown();
        embed.moduleInitialized = false;
        if (!embed.api->init(&embed.cfg)) return false;
        embed.moduleInitialized = true;
        embed.pendN = 0;
        embed.last = NULL;
        return true;
    }
    embed.api->shutdown();
    embed.moduleInitialized = false;
    void *oldSo = embed.so;
    embed.so = NULL;
    embed.api = NULL;
    embed.rumble = NULL;
    N64Pad_Rumble(0.0f);
    if (oldSo == embedRetainedSo) {
        embedRetainedSo = NULL;
        embedRetainedPath[0] = '\0';
    }
    dlclose(oldSo);

    embed.so = dlopen(embed.module, RTLD_NOW | RTLD_LOCAL);
    nsomod_get_api_fn get = embed.so ? (nsomod_get_api_fn)dlsym(embed.so, NSOMOD_ENTRY) : NULL;
    embed.rumble = embed.so ? (nsomod_rumble_fn)dlsym(embed.so, NSOMOD_RUMBLE_ENTRY) : NULL;
    const NsomodApi *api = get ? get() : NULL;
    if (!api || api->version != NSOMOD_API_VERSION || api->fb_w != embed.fbw
        || api->fb_h != embed.fbh || api->channels != embed.channels || !api->init) {
        if (embed.so) dlclose(embed.so);
        embed.so = NULL;
        embed.api = NULL;
        return false;
    }
    embed.api = api;
    if (!api->init(&embed.cfg)) return false;
    embed.moduleInitialized = true;
    embed.pendN = 0;
    embed.last = NULL;   /* the old copy's buffer */
    return true;
}

/* Slot i (0-based) file with ext; false if it does not fit. */
static bool Embed_SlotPath(char *out, size_t n, int i, const char *ext)
{
    char name[32];
    snprintf(name, sizeof(name), "Suspend point %d", i + 1);
    return Ss_Path(out, n, embed.stateDir, name, ext) == 0;
}

static bool Embed_ExternalCommand(const char *action, int slot)
{
    char path[1200], tmp[1240];
    int n = snprintf(path, sizeof(path), "%s/nso-menu-action", embed.stateDir);
    if (!embed.external || n < 0 || (size_t)n >= sizeof(path)) return false;
    n = snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    if (n < 0 || (size_t)n >= sizeof(tmp)) return false;
    FILE *f = fopen(tmp, "wb");
    if (!f) return false;
    bool ok = fprintf(f, "%s %d\n", action, slot) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok && rename(tmp, path) != 0) ok = false;
    if (!ok) remove(tmp);
    return ok;
}

static unsigned char Embed_XChannel(unsigned long pixel, unsigned long mask)
{
    if (!mask) return 0;
    unsigned int shift = (unsigned int)__builtin_ctzl(mask);
    unsigned long maxValue = mask >> shift;
    return (unsigned char)((((pixel & mask) >> shift) * 255UL + maxValue / 2) / maxValue);
}

static int embedXErrorCode;
static int Embed_XErrorTrap(Display *display, XErrorEvent *error)
{
    (void)display;
    embedXErrorCode = error->error_code;
    return 0;
}

bool Embed_StartExternal(const char *stateDir, unsigned long childWindow, bool stateSlots)
{
    if (embed.active || !stateDir || !stateDir[0] || !childWindow) return false;
    memset(&embed, 0, sizeof(embed));
    embed.external = true;
    embed.externalSlots = stateSlots;
    embed.active = true;
    embed.fps = 60.0;
    snprintf(embed.stateDir, sizeof(embed.stateDir), "%s", stateDir);
#if defined(__linux__)
    Display *display = XOpenDisplay(NULL);
    XWindowAttributes attrs = {};
    Window root = 0, parent = 0;
    Window *children = NULL;
    unsigned int childCount = 0;
    void *hostHandle = GetWindowHandle();
    unsigned long expectedParent = hostHandle ? *(unsigned long *)hostHandle : 0;
    XErrorHandler previousHandler = display ? XSetErrorHandler(Embed_XErrorTrap) : NULL;
    embedXErrorCode = 0;
    Status attrsOk = display ? XGetWindowAttributes(display, (Window)childWindow, &attrs) : 0;
    Status treeOk = attrsOk ? XQueryTree(display, (Window)childWindow, &root, &parent, &children, &childCount) : 0;
    if (display) XSync(display, False);
    int queryError = embedXErrorCode;
    if (display) XSetErrorHandler(previousHandler);
    if (!display || !attrsOk || !treeOk || queryError != 0
        || attrs.map_state != IsViewable || attrs.width <= 0 || attrs.height <= 0
        || attrs.width > 8192 || attrs.height > 8192 || parent == root
        || expectedParent == 0 || (unsigned long)parent != expectedParent) {
        if (children) XFree(children);
        if (display) XCloseDisplay(display);
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
    if (children) XFree(children);
    previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    XImage *shot = XGetImage(display, (Window)childWindow, 0, 0, (unsigned int)attrs.width,
                             (unsigned int)attrs.height, AllPlanes, ZPixmap);
    XSync(display, False);
    int captureError = embedXErrorCode;
    XSetErrorHandler(previousHandler);
    if (!shot || captureError != 0) {
        if (shot) XDestroyImage(shot);
        XCloseDisplay(display);
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
    embed.fbw = attrs.width;
    embed.fbh = attrs.height;
    Image img = GenImageColor(embed.fbw, embed.fbh, BLACK);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
    if (!img.data) {
        XDestroyImage(shot);
        XCloseDisplay(display);
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
    unsigned char *pixels = (unsigned char *)img.data;
    for (int y = 0; y < embed.fbh; y++) {
        for (int x = 0; x < embed.fbw; x++) {
            unsigned long px = XGetPixel(shot, x, y);
            size_t i = ((size_t)y * (size_t)embed.fbw + (size_t)x) * 3;
            pixels[i + 0] = Embed_XChannel(px, shot->red_mask);
            pixels[i + 1] = Embed_XChannel(px, shot->green_mask);
            pixels[i + 2] = Embed_XChannel(px, shot->blue_mask);
        }
    }
    XDestroyImage(shot);
    size_t frameBytes = (size_t)embed.fbw * (size_t)embed.fbh * 3;
    embed.externalFrameRGB = (unsigned char *)malloc(frameBytes);
    if (!embed.externalFrameRGB) {
        UnloadImage(img);
        XCloseDisplay(display);
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
    memcpy(embed.externalFrameRGB, img.data, frameBytes);
    embed.externalWindow = childWindow;
    embed.externalParent = (unsigned long)parent;
    embed.externalDisplay = display;
    SetExitKey(KEY_NULL);
    embed.tex = LoadTextureFromImage(img);
    UnloadImage(img);
    SetTextureFilter(embed.tex, crtSession.rawFrames ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);   /* rawFrames: the CRT shader sees 480 lines */
    if (!embed.tex.id) {
        XCloseDisplay(display);
        embed.externalDisplay = NULL;
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
    previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    XUnmapWindow(display, (Window)childWindow);
    XSetInputFocus(display, parent, RevertToParent, CurrentTime);
    XSync(display, False);
    int hideError = embedXErrorCode;
    XSetErrorHandler(previousHandler);
    if (hideError != 0) {
        previousHandler = XSetErrorHandler(Embed_XErrorTrap);
        embedXErrorCode = 0;
        XMapRaised(display, (Window)childWindow);
        XSetInputFocus(display, (Window)childWindow, RevertToParent, CurrentTime);
        XSync(display, False);
        XSetErrorHandler(previousHandler);
        Embed_ExternalCommand("resume", 0);
        Embed_Stop();
        return false;
    }
#else
    Embed_ExternalCommand("resume", 0);
    Embed_Stop();
    return false;
#endif
    Embed_SetMenu(true);
    embed.menuT = 0.0f;
    return true;
}

int Embed_ExternalResult(void)
{
    return embed.externalResult;
}

void Embed_ExternalSetChildVisible(bool visible)
{
#if defined(__linux__)
    Display *display = (Display *)embed.externalDisplay;
    if (!embed.external || !display || !embed.externalWindow) return;
    XErrorHandler previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    if (visible) {
        XMapRaised(display, (Window)embed.externalWindow);
        XSetInputFocus(display, (Window)embed.externalWindow, RevertToParent, CurrentTime);
    } else {
        XUnmapWindow(display, (Window)embed.externalWindow);
        if (embed.externalParent) XSetInputFocus(display, (Window)embed.externalParent, RevertToParent, CurrentTime);
    }
    XSync(display, False);
    int errorCode = embedXErrorCode;
    XSetErrorHandler(previousHandler);
    if (errorCode != 0) fprintf(stderr, "NSO: X11 error %d while changing hosted game visibility\n", errorCode);
#else
    (void)visible;
#endif
}

void Embed_ShowExternalWindow(unsigned long window)
{
#if defined(__linux__)
    if (!window) return;
    Display *display = XOpenDisplay(NULL);
    if (!display) return;
    XErrorHandler previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    XMapRaised(display, (Window)window);
    XSync(display, False);
    if (embedXErrorCode == 0) XSetInputFocus(display, (Window)window, RevertToParent, CurrentTime);
    XSync(display, False);
    XSetErrorHandler(previousHandler);
    XCloseDisplay(display);
#else
    (void)window;
#endif
}

#if defined(__linux__)
static Display *exitOvDpy;
static Window exitOvWin;
static bool exitOvMapped;

static Window Embed_HostWindow(void)
{
    void *hostHandle = GetWindowHandle();
    return hostHandle ? (Window)*(unsigned long *)hostHandle : 0;
}

static bool Embed_CompositorActive(Display *dpy)
{
    if (!dpy) return false;
    Atom cm = XInternAtom(dpy, "_NET_WM_CM_S0", False);
    return XGetSelectionOwner(dpy, cm) != None;
}

static void Embed_ExitOverlaySetOpacity(Display *dpy, Window win, float opacity)
{
    unsigned long val;
    Atom atom;
    if (opacity <= 0.0f) val = 0;
    else if (opacity >= 1.0f) val = 0xffffffffUL;
    else val = (unsigned long)(opacity * 65535.0f) * 0x10001UL;
    atom = XInternAtom(dpy, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(dpy, win, atom, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&val, 1);
}

static Atom Embed_ExitOverlayAtom(Display *dpy)
{
    return XInternAtom(dpy, "NSO_EXIT_OVERLAY", False);
}

static void Embed_ExitOverlayMark(Display *dpy, Window win)
{
    unsigned long one = 1;
    Atom atom = Embed_ExitOverlayAtom(dpy);
    XChangeProperty(dpy, win, atom, XA_CARDINAL, 32, PropModeReplace,
                    (unsigned char *)&one, 1);
}

/* Keep (or return) keyboard focus on NSO so the overlay never steals it
 * under a focus-follows-mouse WM, and so Destroy does not leave NSO unfocused. */
static void Embed_ExitOverlayFocusHost(Display *dpy)
{
    Window parent = Embed_HostWindow();
    if (!dpy || !parent) return;
    XSetInputFocus(dpy, parent, RevertToParent, CurrentTime);
}

/* Destroy any leftover overlay from a previous exit (failed destroy, or a
 * previous Display connection) so a second leave cannot stack another one. */
static void Embed_ExitOverlaySweep(Display *dpy, Window parent)
{
    Window root = 0, p = 0, *children = NULL;
    unsigned int n = 0;
    Atom atom;
    if (!dpy || !parent) return;
    atom = Embed_ExitOverlayAtom(dpy);
    if (!XQueryTree(dpy, parent, &root, &p, &children, &n) || !children) return;
    for (unsigned int i = 0; i < n; i++) {
        Atom actualType = None;
        int actualFormat = 0;
        unsigned long nitems = 0, bytes = 0;
        unsigned char *data = NULL;
        if (children[i] == exitOvWin) continue;
        if (XGetWindowProperty(dpy, children[i], atom, 0, 1, False, XA_CARDINAL,
                               &actualType, &actualFormat, &nitems, &bytes, &data) == Success
            && actualType == XA_CARDINAL) {
            Embed_ExitOverlaySetOpacity(dpy, children[i], 0.0f);
            XUnmapWindow(dpy, children[i]);
            XDestroyWindow(dpy, children[i]);
        }
        if (data) XFree(data);
    }
    XFree(children);
}
#endif

void Embed_ExitOverlaySet(float opacity)
{
#if defined(__linux__)
    Window parent = Embed_HostWindow();
    int sw, sh;
    XErrorHandler previousHandler;
    bool compositor;
    if (!parent) return;
    opacity = fminf(1.0f, fmaxf(0.0f, opacity));
    if (!exitOvDpy) {
        exitOvDpy = XOpenDisplay(NULL);
        if (!exitOvDpy) return;
    }
    sw = GetScreenWidth();
    sh = GetScreenHeight();
    if (sw < 1) sw = 1;
    if (sh < 1) sh = 1;
    previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    Embed_ExitOverlaySweep(exitOvDpy, parent);
    if (!exitOvWin) {
        XSetWindowAttributes swa = { 0 };
        swa.override_redirect = True;
        swa.background_pixel = BlackPixel(exitOvDpy, DefaultScreen(exitOvDpy));
        swa.border_pixel = 0;
        swa.event_mask = 0;
        swa.do_not_propagate_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask
            | ButtonReleaseMask | PointerMotionMask;
        exitOvWin = XCreateWindow(exitOvDpy, parent, 0, 0,
                                  (unsigned)sw, (unsigned)sh, 0,
                                  CopyFromParent, InputOutput, CopyFromParent,
                                  CWOverrideRedirect | CWBackPixel | CWBorderPixel
                                  | CWEventMask | CWDontPropagate, &swa);
        if (exitOvWin) {
            Embed_ExitOverlayMark(exitOvDpy, exitOvWin);
            Embed_ExitOverlaySetOpacity(exitOvDpy, exitOvWin, 0.0f);
        }
    } else {
        XMoveResizeWindow(exitOvDpy, exitOvWin, 0, 0, (unsigned)sw, (unsigned)sh);
    }
    if (!exitOvWin) {
        XSetErrorHandler(previousHandler);
        return;
    }
    compositor = Embed_CompositorActive(exitOvDpy);
    Embed_ExitOverlaySetOpacity(exitOvDpy, exitOvWin, opacity);
    /* No compositor: an unmapped-to-mapped window is instantly opaque, so keep
     * it hidden until the fade reaches 1 and a GL fade can run underneath. */
    if (!compositor && opacity < 1.0f) {
        if (exitOvMapped) {
            XUnmapWindow(exitOvDpy, exitOvWin);
            exitOvMapped = false;
        }
    } else if (!exitOvMapped) {
        XMapRaised(exitOvDpy, exitOvWin);
        exitOvMapped = true;
    } else {
        XRaiseWindow(exitOvDpy, exitOvWin);
    }
    Embed_ExitOverlayFocusHost(exitOvDpy);
    XSync(exitOvDpy, False);
    XSetErrorHandler(previousHandler);
#else
    (void)opacity;
#endif
}

void Embed_ExitOverlayDestroy(void)
{
#if defined(__linux__)
    Window parent = Embed_HostWindow();
    Display *dpy = exitOvDpy;
    XErrorHandler previousHandler;
    bool opened = false;
    if (!dpy) {
        dpy = XOpenDisplay(NULL);
        opened = (dpy != NULL);
    }
    if (dpy) {
        previousHandler = XSetErrorHandler(Embed_XErrorTrap);
        embedXErrorCode = 0;
        /* Drop opacity to 0 and sync before unmap so picom's unmap fade has
         * nothing visible to leave behind as a ghost rectangle. */
        if (exitOvWin) {
            Embed_ExitOverlaySetOpacity(dpy, exitOvWin, 0.0f);
            XSync(dpy, False);
            if (exitOvMapped) XUnmapWindow(dpy, exitOvWin);
            XSync(dpy, False);
            XDestroyWindow(dpy, exitOvWin);
            XSync(dpy, False);
        }
        exitOvWin = 0;
        exitOvMapped = false;
        Embed_ExitOverlaySweep(dpy, parent);
        Embed_ExitOverlayFocusHost(dpy);
        XSync(dpy, False);
        XSetErrorHandler(previousHandler);
        if (exitOvDpy) {
            XCloseDisplay(exitOvDpy);
            exitOvDpy = NULL;
        } else if (opened) {
            XCloseDisplay(dpy);
        }
    } else {
        exitOvWin = 0;
        exitOvMapped = false;
    }
#endif
}

bool Embed_ExternalChildGone(void)
{
#if defined(__linux__)
    Display *display;
    XWindowAttributes attrs = { 0 };
    XErrorHandler previousHandler;
    Status ok;
    int err;
    if (!embed.external || !embed.externalWindow) return true;
    display = (Display *)embed.externalDisplay;
    if (!display) return true;
    previousHandler = XSetErrorHandler(Embed_XErrorTrap);
    embedXErrorCode = 0;
    ok = XGetWindowAttributes(display, (Window)embed.externalWindow, &attrs);
    XSync(display, False);
    err = embedXErrorCode;
    XSetErrorHandler(previousHandler);
    if (!ok || err != 0) return true;
    return attrs.map_state != IsViewable;
#else
    return true;
#endif
}

static void Embed_ScanSlot(int i)
{
    char p[1200];
    if (embed.thumb[i].id) UnloadTexture(embed.thumb[i]);
    embed.thumb[i] = (Texture2D){ 0 };
    embed.slotTime[i] = 0;
    if (!embed.stateDir[0] || !Embed_SlotPath(p, sizeof(p), i, ".sav") || !FileExists(p)) return;
    embed.slotTime[i] = GetFileModTime(p);
    if (Embed_SlotPath(p, sizeof(p), i, ".bmp") && FileExists(p)) {
        embed.thumb[i] = LoadTexture(p);
        SetTextureFilter(embed.thumb[i], TEXTURE_FILTER_BILINEAR);
    }
}

/* Create Suspend Point: the state goes to a temp file first so a failed save
 * never clobbers the slot; the thumbnail is the frozen frame. */
static void Embed_SaveSlot(int i)
{
    if (embed.external) {
        char bmp[1200];
        if (!embed.externalFrameRGB || !Embed_SlotPath(bmp, sizeof(bmp), i, ".bmp")) return;
        Image frame = { .data = embed.externalFrameRGB, .width = embed.fbw, .height = embed.fbh,
                        .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
        if (!ExportImage(frame, bmp)) return;
        if (!Embed_ExternalCommand("save", i)) { remove(bmp); return; }
        Embed_ScanSlot(i);
        return;
    }
    char sav[1200], tmp[1200], bmp[1200];
    if (!embed.moduleInitialized || !embed.api || !embed.api->state_save) {
        Embed_SetStatus("Game cannot create suspend points");
        return;
    }
    if (!Embed_SlotPath(sav, sizeof(sav), i, ".sav") || !Embed_SlotPath(tmp, sizeof(tmp), i, ".sav.tmp")
        || !Embed_SlotPath(bmp, sizeof(bmp), i, ".bmp")) {
        Embed_SetStatus("Could not create suspend point path");
        return;
    }
    if (!embed.api->state_save(tmp) || rename(tmp, sav) != 0) {
        remove(tmp);
        Embed_SetStatus("Suspend point save failed");
        return;
    }
    if (embed.last) {
        Image img = { .data = (void *)embed.last, .width = embed.fbw, .height = embed.fbh,
                      .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
        ExportImage(img, bmp);
    }
    Embed_ScanSlot(i);
    Embed_SetStatus("Suspend point saved");
}

/* After the menu exit: load the slot, show its first frame under the black. */
static void *Embed_LoadWorker(void *unused)
{
    (void)unused;
    bool ok = embed.api->state_load(embed.loadPath);
    atomic_store_explicit(&embed.loadOk, ok, memory_order_relaxed);
    atomic_store_explicit(&embed.loadDone, true, memory_order_release);
    return NULL;
}

static bool Embed_BeginLoad(const char *path)
{
    if (!path || !path[0] || !embed.moduleInitialized || !embed.api || !embed.api->state_load
        || embed.loadThreadStarted) return false;
    snprintf(embed.loadPath, sizeof(embed.loadPath), "%s", path);
    atomic_store_explicit(&embed.loadDone, false, memory_order_relaxed);
    atomic_store_explicit(&embed.loadOk, false, memory_order_relaxed);
    if (embed.audio) PauseAudioStream(embed.stream);
    if (pthread_create(&embed.loadThread, NULL, Embed_LoadWorker, NULL) != 0) {
        if (embed.audio) ResumeAudioStream(embed.stream);
        return false;
    }
    embed.loadThreadStarted = true;
    return true;
}

static void Embed_LoadSlot(int i)
{
    if (embed.external) {
        Embed_ExternalCommand("load", i);
        embed.externalResumeRequested = true;
        embed.externalLoadRequested = true;
        return;
    }
    char sav[1200];
    if (!Embed_SlotPath(sav, sizeof(sav), i, ".sav") || !Embed_BeginLoad(sav)) return;
}

static bool Embed_FinishLoad(void)
{
    if (!embed.loadThreadStarted) return false;
    if (!atomic_load_explicit(&embed.loadDone, memory_order_acquire)) return false;
    pthread_join(embed.loadThread, NULL);
    embed.loadThreadStarted = false;
    if (!atomic_load_explicit(&embed.loadOk, memory_order_relaxed)) {
        if (embed.audio) ResumeAudioStream(embed.stream);
        Embed_SetStatus("Suspend point load failed");
        return true;
    }
    if (!embed.moduleInitialized || !embed.api) {
        Embed_SetStatus("Game module stopped during load");
        return true;
    }
    NsomodInput none = { 0 };
    NsomodFrame f = embed.api->frame(&none);
    UpdateTexture(embed.tex, f.rgb24);
    embed.last = f.rgb24;
    embed.blackT = EMBED_LOAD_SEC;
    Embed_SetStatus("Suspend point loaded");
    return true;
}

/* Opening pauses game and audio; both resume once the exit animation ends. */
static void Embed_SetMenu(bool open)
{
    embed.menuOpen = open;
    if (!open) return;
    embed.menuSel = EMBED_MENU_CONTINUE;
    embed.crtOpen = false;
    embed.crtT = 0.0f;
    embed.slotsFocus = false;
    embed.blurReady = false;
    if (!embed.external || embed.externalSlots)
        for (int i = 0; i < EMBED_SLOTS; i++) Embed_ScanSlot(i);
    if (embed.audio) PauseAudioStream(embed.stream);
    Sfx_Play(SFX_AUDIO);
}

static bool Embed_MenuItemAvailable(int item)
{
    if (item == EMBED_MENU_CRT) return crtSession.active;   /* CRT Geometry on for this game and adjustable */
    return !embed.external || embed.externalSlots
        || (item != EMBED_MENU_LOAD && item != EMBED_MENU_SAVE);
}

static int Embed_NextMenuItem(int item, int direction)
{
    do {
        item = (item + direction + EMBED_MENU_COUNT) % EMBED_MENU_COUNT;
    } while (!Embed_MenuItemAvailable(item));
    return item;
}

/* Left stick edges for menu navigation: crossing the deadzone is one press,
 * exactly like a d-pad button (no auto-repeat). Armed only after the stick
 * has been centred, so a stick held when the menu opens does not move it. */
static void Embed_NavUpdate(void)
{
    float lx = 0.0f, ly = 0.0f;
    if (IsGamepadAvailable(0)) {
        lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    }
    embed.navX = Nso_PadAxisEdgeArmed(&embed.navArmX, lx);
    embed.navY = Nso_PadAxisEdgeArmed(&embed.navArmY, ly);
}

/* Slot list (Load / Create focused right of the divider). */
static void Embed_SlotsInput(bool gp)
{
    bool load = (embed.menuSel == EMBED_MENU_LOAD);
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_LEFT) || Nso_PadBackPressed(0)
        || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) || embed.navX < 0) {
        embed.slotsFocus = false;
        return;
    }
    if ((IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) || embed.navY < 0) && embed.slotSel > 0) {
        embed.slotSel--;
        Sfx_Play(SFX_NAVIGATE);
    }
    if ((IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) || embed.navY > 0)
        && embed.slotSel + 1 < EMBED_SLOTS) {
        embed.slotSel++;
        Sfx_Play(SFX_NAVIGATE);
    }
    if (!(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || Nso_PadConfirmPressed(0))) return;
    Sfx_Play(SFX_CLICK);
    if (!load) Embed_SaveSlot(embed.slotSel);
    else if (embed.slotTime[embed.slotSel]) { embed.loadSlot = embed.slotSel + 1; Embed_SetMenu(false); }
}

/* ---- CRT Geometry sub-screen: sliders only. Back / End commit: the steps are saved per game and (Mednafen)
 * written to the run file the emulator re-reads when it resumes. ---- */
#define EMBED_CRT_SLIDER0 0
#define EMBED_CRT_ROWS (CRTP_COUNT + 1)   /* sliders, reset */
#define EMBED_CRT_HOLD_SEC 0.9f           /* the 0.9 s hold convention (padui.h) */
#define EMBED_CRT_REPEAT_DELAY 0.35f
#define EMBED_CRT_REPEAT_SEC 0.04f

static void Embed_CrtOpenScreen(void)
{
    memcpy(embed.crtSteps, crtSession.game, sizeof(embed.crtSteps));
    embed.crtOpen = true;
    embed.crtWait = true;
    embed.crtSel = 0;
    embed.crtRepDir = 0;
    embed.crtRepT = 0.0f;
    embed.crtHold = 0.0f;
    embed.crtHoldDone = true;   /* until the key that opened it is let go */
    Sfx_Play(SFX_CLICK);
}

static void Embed_CrtLeave(void)
{
    if (memcmp(embed.crtSteps, crtSession.game, sizeof(embed.crtSteps)) != 0) {
        CrtSession_Commit(embed.crtSteps);
        CrtGeom_SetParams(crtSession.val, crtSession.angleSet);
    }
    embed.crtOpen = false;
}

static void Embed_CrtStep(int dir)
{
    int param = embed.crtSel - EMBED_CRT_SLIDER0;
    if (param < 0 || param >= CRTP_COUNT) return;
    int cur = CrtSteps_Shown(embed.crtSteps, crtSession.def, param);
    int next = CrtStep_Clamp(cur + dir);
    if (next == cur) return;
    embed.crtSteps[param] = (signed char)next;
    Sfx_Play(SFX_NAVIGATE);
}

static void Embed_CrtInput(float dt, bool gp, bool end)
{
    if (IsKeyPressed(KEY_B) || Nso_PadBackPressed(0) || end) { Embed_CrtLeave(); return; }
    if (IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) || embed.navY < 0) {
        embed.crtSel = (embed.crtSel + EMBED_CRT_ROWS - 1) % EMBED_CRT_ROWS;
        Sfx_Play(SFX_NAVIGATE);
        embed.crtHold = 0.0f;
    }
    if (IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) || embed.navY > 0) {
        embed.crtSel = (embed.crtSel + 1) % EMBED_CRT_ROWS;
        Sfx_Play(SFX_NAVIGATE);
        embed.crtHold = 0.0f;
    }
    /* Left / Right: one step per press, then auto-repeat while held */
    float lx = gp ? GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X) : 0.0f;
    bool l = IsKeyDown(KEY_LEFT) || (gp && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) || lx < -NSO_PAD_DEADZONE;
    bool r = IsKeyDown(KEY_RIGHT) || (gp && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) || lx > NSO_PAD_DEADZONE;
    int dir = (r ? 1 : 0) - (l ? 1 : 0);
    if (dir == 0) { embed.crtWait = false; embed.crtRepDir = 0; }
    else if (!embed.crtWait) {
        if (dir != embed.crtRepDir) {
            embed.crtRepDir = dir;
            embed.crtRepT = EMBED_CRT_REPEAT_DELAY;
            Embed_CrtStep(dir);
        } else {
            embed.crtRepT -= dt;
            while (embed.crtRepT <= 0.0f) { Embed_CrtStep(dir); embed.crtRepT += EMBED_CRT_REPEAT_SEC; }
        }
    }
    /* Hold A / Enter: this slider back to inherit its default; on the last row, all of them */
    bool down = IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_A) || (gp && IsGamepadButtonDown(0, NSO_BTN_A));
    if (!down) { embed.crtHold = 0.0f; embed.crtHoldDone = false; }
    else if (!embed.crtHoldDone) {
        embed.crtHold += dt;
        if (embed.crtHold >= EMBED_CRT_HOLD_SEC) {
            if (embed.crtSel >= EMBED_CRT_SLIDER0 + CRTP_COUNT) memset(embed.crtSteps, -1, sizeof(embed.crtSteps));
            else embed.crtSteps[embed.crtSel - EMBED_CRT_SLIDER0] = -1;
            embed.crtHoldDone = true;
            embed.crtHold = 0.0f;
            Sfx_Play(SFX_CLICK);
        }
    }
}

bool Embed_CrtOpen(void) { return embed.active && embed.crtOpen; }

bool Embed_Update(float dt)
{
    if (!embed.active) return false;
    Embed_NavUpdate();
    if (embed.statusTimer > 0.0f) embed.statusTimer = fmaxf(0.0f, embed.statusTimer - dt);
    if (embed.external) {
        embed.externalScanTimer += dt;
        if (embed.externalScanTimer >= 0.35f) {
            embed.externalScanTimer = 0.0f;
            if (!embed.external || embed.externalSlots)
                for (int i = 0; i < EMBED_SLOTS; i++) Embed_ScanSlot(i);
        }
    }
    if (embed.loadThreadStarted) {
        Embed_FinishLoad();
        if (embed.loadThreadStarted) { embed.acc = 0.0; return true; }
    }
    bool gp = IsGamepadAvailable(0);
    /* Hosted child games open this menu themselves. The opening End/Guide
     * press may also reach Raylib after focus moves to NSO; don't consume that
     * same press as an immediate close. Continue/Back remain the close actions. */
    bool toggle = !embed.external
        && (IsKeyPressed(KEY_END) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE)));

    float st = dt / EMBED_MENU_ANIM_SEC;
    embed.crtT = embed.crtOpen ? fminf(1.0f, embed.crtT + st) : fmaxf(0.0f, embed.crtT - st);
    if (embed.menuOpen) embed.menuT = fminf(1.0f, embed.menuT + st);
    else if (embed.menuT > 0.0f) {   /* exit animation: still paused */
        embed.menuT = fmaxf(0.0f, embed.menuT - st);
        embed.acc = 0.0;
        if (embed.menuT <= 0.0f && embed.loadSlot) { Embed_LoadSlot(embed.loadSlot - 1); embed.loadSlot = 0; }
        if (embed.menuT <= 0.0f && embed.external) {
            if (embed.quitting) {
                /* Keep the child paused; main parks it and may quit later. */
                embed.externalResult = EMBED_EXTERNAL_QUIT;
                return false;
            }
            if (embed.externalRestartRequested) {
                Embed_ExternalCommand("restart", 0);
                embed.externalResult = EMBED_EXTERNAL_RESTART;
                return false;
            }
            if (embed.externalResumeRequested) {
                if (!embed.externalLoadRequested) Embed_ExternalCommand("resume", 0);
                embed.externalResult = EMBED_EXTERNAL_RESUME;
                return false;
            }
        }
        if (embed.menuT <= 0.0f && embed.quitting) return false;
        if (embed.menuT <= 0.0f && embed.audio && embed.blackT <= 0.0f && !embed.loadThreadStarted)
            ResumeAudioStream(embed.stream);
        return true;
    }
    if (embed.blackT > 0.0f) {   /* suspend point loading: still paused */
        embed.blackT = fmaxf(0.0f, embed.blackT - dt);
        embed.acc = 0.0;
        if (embed.blackT <= 0.0f && embed.audio) ResumeAudioStream(embed.stream);
        return true;
    }

    if (embed.menuOpen) {   /* paused: no game frames */
        embed.acc = 0.0;
        if (embed.crtOpen) { Embed_CrtInput(dt, gp, toggle); return true; }
        if (toggle) {
            if (embed.external) embed.externalResumeRequested = true;
            Embed_SetMenu(false);
            return true;
        }
        if (embed.slotsFocus) { Embed_SlotsInput(gp); return true; }
        if (IsKeyPressed(KEY_B) || Nso_PadBackPressed(0)) {
            if (embed.external) embed.externalResumeRequested = true;
            Embed_SetMenu(false);
            return true;
        }
        if (IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) || embed.navY < 0) {
            embed.menuSel = Embed_NextMenuItem(embed.menuSel, -1);
            Sfx_Play(SFX_NAVIGATE);
        }
        if (IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) || embed.navY > 0) {
            embed.menuSel = Embed_NextMenuItem(embed.menuSel, 1);
            Sfx_Play(SFX_NAVIGATE);
        }
        bool slots = embed.menuSel == EMBED_MENU_LOAD || embed.menuSel == EMBED_MENU_SAVE;
        bool right = IsKeyPressed(KEY_RIGHT) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) || embed.navX > 0;
        bool ok = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || Nso_PadConfirmPressed(0);
        if (embed.menuSel == EMBED_MENU_CRT && (ok || right)) {
            Embed_CrtOpenScreen();
        } else if (slots && Embed_MenuItemAvailable(embed.menuSel) && (ok || right)) {
            if (embed.stateDir[0]) { embed.slotsFocus = true; embed.slotSel = 0; Sfx_Play(SFX_CLICK); }
        } else if (ok) {
            Sfx_Play(SFX_CLICK);
            if (embed.menuSel == EMBED_MENU_QUIT) embed.quitting = true;
            if (embed.menuSel == EMBED_MENU_RESTART) {
                if (embed.external) embed.externalRestartRequested = true;
                else if (!Embed_Reload()) return false;
            }
            if (embed.external && embed.menuSel == EMBED_MENU_CONTINUE) embed.externalResumeRequested = true;
            Embed_SetMenu(false);
        }
        return true;
    }
    if (toggle) { Embed_SetMenu(true); return true; }

    if (embed.external) return true;

    int n = Embed_PaceFrames(&embed.acc, dt, embed.fps);
    if (n > 0) {
        NsomodInput p1 = Embed_ReadPad();
        NsomodFrame f = { 0 };
        for (int i = 0; i < n; i++) f = embed.api->frame(&p1);
        if (embed.rumble) N64Pad_Rumble(embed.rumble());   /* held N64PAD_HOLD_MS: stops when frames stop */
        UpdateTexture(embed.tex, f.rgb24);
        embed.last = f.rgb24;
    }
    Embed_PumpAudio();
    return true;
}

float Embed_MenuOpacity(void) { return embed.active ? Embed_MenuEase(embed.menuT) : 0.0f; }

static void Embed_BlurPass(RenderTexture2D dst, RenderTexture2D src, Shader sh, int loc, float dx, float dy)
{
    float dir[2] = { dx / dst.texture.width, dy / dst.texture.height };
    BeginTextureMode(dst);
    ClearBackground(BLACK);
    BeginShaderMode(sh);
    SetShaderValue(sh, loc, dir, SHADER_UNIFORM_VEC2);
    DrawTexturePro(src.texture, (Rectangle){ 0, 0, (float)src.texture.width, -(float)src.texture.height },
                   (Rectangle){ 0, 0, (float)dst.texture.width, (float)dst.texture.height }, (Vector2){ 0 }, 0.0f, WHITE);
    EndShaderMode();
    EndTextureMode();
}

/* Gaussian-ish, sigma ~ 3.7 half-res texels (~30 px at 1080p): 3 H/V pairs, wider each time. */
static void Embed_BuildBlur(Shader sh, int loc)
{
    int w = EMBED_BLUR_W + 2 * EMBED_BLUR_PAD, h = EMBED_BLUR_H + 2 * EMBED_BLUR_PAD;
    if (!embed.blurA.id) {
        embed.blurA = LoadRenderTexture(w, h);
        embed.blurB = LoadRenderTexture(w, h);
        RenderTexture2D *rt[2] = { &embed.blurA, &embed.blurB };
        for (int i = 0; i < 2; i++) {
            SetTextureFilter(rt[i]->texture, TEXTURE_FILTER_BILINEAR);
            SetTextureWrap(rt[i]->texture, TEXTURE_WRAP_CLAMP);
        }
    }
    SetTextureFilter(embed.tex, TEXTURE_FILTER_BILINEAR);
    BeginTextureMode(embed.blurA);
    ClearBackground(BLACK);
    DrawTexturePro(embed.tex, (Rectangle){ 0, 0, (float)embed.fbw, (float)embed.fbh },
                   (Rectangle){ EMBED_BLUR_PAD, EMBED_BLUR_PAD, EMBED_BLUR_W, EMBED_BLUR_H }, (Vector2){ 0 }, 0.0f, WHITE);
    EndTextureMode();
    SetTextureFilter(embed.tex, embed.external && crtSession.rawFrames ? TEXTURE_FILTER_BILINEAR : TEXTURE_FILTER_POINT);
    if (sh.id > 0 && loc >= 0)
        for (int k = 1; k <= 3; k++) {
            Embed_BlurPass(embed.blurB, embed.blurA, sh, loc, (float)k, 0.0f);
            Embed_BlurPass(embed.blurA, embed.blurB, sh, loc, 0.0f, (float)k);
        }
    embed.blurReady = true;
}

/* Right of the divider at 1080p (frames 124-142): four 626 x 168 boxes, 210
 * apart, thumbnail left; the focused one white with red text. */
static void Embed_DrawSlots(Font font, const char *const text[EMBED_TXT_COUNT], float k, float ox, float a,
                            Color panel, Color selInk)
{
    float fs = roundf(40.0f * k), bw = roundf(2.0f * k);
    for (int i = 0; i < EMBED_SLOTS; i++) {
        Rectangle box = { roundf(1071.0f * k) + ox, roundf((141.0f + 210.0f * i) * k), roundf(626.0f * k), roundf(168.0f * k) };
        Rectangle th = { box.x, box.y, roundf(216.0f * k), box.height };
        bool sel = embed.slotsFocus && i == embed.slotSel;
        if (sel) DrawRectangleRec((Rectangle){ th.x + th.width, box.y, box.width - th.width, box.height }, Fade(panel, a));
        if (embed.thumb[i].id)
            DrawTexturePro(embed.thumb[i], (Rectangle){ 0, 0, (float)embed.thumb[i].width, (float)embed.thumb[i].height },
                           th, (Vector2){ 0 }, 0.0f, Fade(WHITE, a));
        else {
            DrawRectangleRec(th, Fade((Color){ 65, 67, 67, 255 }, a));
            if (!embed.slotTime[i]) {
                Vector2 m = MeasureTextEx(font, text[EMBED_TXT_NODATA], roundf(32.0f * k), 0.0f);
                DrawTextEx(font, text[EMBED_TXT_NODATA], (Vector2){ roundf(th.x + (th.width - m.x) * 0.5f),
                           roundf(th.y + (th.height - m.y) * 0.5f) }, roundf(32.0f * k), 0.0f, Fade(WHITE, a));
            }
        }
        DrawRectangleLinesEx(box, bw, Fade(panel, a));
        char line[64];
        Color ink = Fade(sel ? selInk : WHITE, a);
        float tx = th.x + th.width + roundf(53.0f * k);
        snprintf(line, sizeof(line), text[EMBED_TXT_SLOT], i + 1);
        Vector2 m = MeasureTextEx(font, line, fs, 0.0f);
        if (!embed.slotTime[i]) {   /* label alone, centred */
            DrawTextEx(font, line, (Vector2){ tx, roundf(box.y + (box.height - m.y) * 0.5f) }, fs, 0.0f, ink);
            continue;
        }
        DrawTextEx(font, line, (Vector2){ tx, roundf(box.y + 65.0f * k - m.y * 0.5f) }, fs, 0.0f, ink);
        time_t t = (time_t)embed.slotTime[i];
        struct tm tm;
        if (localtime_r(&t, &tm) && strftime(line, sizeof(line), text[EMBED_TXT_DATE], &tm))
            DrawTextEx(font, line, (Vector2){ tx, roundf(box.y + 105.0f * k - m.y * 0.5f) }, fs, 0.0f, ink);
    }
}

/* CRT Geometry applies when the toggle is on and the shader built. Its beam scanlines follow
 * EmbedScreen.scanlines (NES/SNES: the Scanlines toggle; N64/GameCube: main.c sets it with
 * CRT Geometry, they have no Scanlines toggle). The plain overlay lines are replaced by it. */
static bool Embed_UseCrt(EmbedScreen sc)
{
    /* a hosted Mednafen game's picture already comes out of Mednafen's own CRT shader */
    return sc.crtgeom && CrtGeom_Ready() && (!embed.external || crtSession.rawFrames);
}
static bool Embed_CrtScan(EmbedScreen sc) { return sc.scanlines; }

static void DrawTexturePro_OrCrt(bool crt, EmbedScreen sc, Rectangle dst)
{
    Rectangle src = { 0, 0, (float)embed.fbw, (float)embed.fbh };
    if (crt) CrtGeom_Draw(embed.tex, src, dst, Embed_CrtScan(sc), WHITE);
    else DrawTexturePro(embed.tex, src, dst, (Vector2){ 0 }, 0.0f, WHITE);
}

/* The CRT Geometry sub-screen at 1080p, scaled by height: the sliders on the left (the menu's white bar and
 * coral ink on the selected row, fill bars with a tick at the middle = the stock value), a live preview of
 * the paused picture on the right. a: the sub-screen's opacity. */
static void Embed_DrawCrt(Font font, EmbedScreen screen, const char *const text[EMBED_TXT_COUNT], float k, float divX,
                          float a, Color panel, Color selInk)
{
    float ox = roundf(-28.0f * k * (1.0f - a));
    float fs = roundf(34.0f * k), pitch = roundf(62.0f * k), barH = roundf(56.0f * k);
    float bx = roundf(150.0f * k) + ox, top = roundf(70.0f * k);
    for (int i = 0; i < EMBED_CRT_ROWS; i++) {
        float cy = top + pitch * (float)i;
        bool sel = (i == embed.crtSel);
        Color ink = sel ? selInk : WHITE;
        if (sel) DrawRectangleRec((Rectangle){ bx, cy - barH * 0.5f, divX - bx, barH }, Fade(panel, a));
        const char *label = (i < EMBED_CRT_SLIDER0 + CRTP_COUNT)
            ? text[EMBED_TXT_CRT0 + (i - EMBED_CRT_SLIDER0)]
            : text[EMBED_TXT_CRT_RESET];
        Vector2 m = MeasureTextEx(font, label, fs, 0.0f);
        DrawTextEx(font, label, (Vector2){ bx + roundf(34.0f * k), roundf(cy - m.y * 0.5f) }, fs, 0.0f, Fade(ink, a));
        if (i < EMBED_CRT_SLIDER0 + CRTP_COUNT) {
            int step = CrtSteps_Shown(embed.crtSteps, crtSession.def, i - EMBED_CRT_SLIDER0);
            float x0 = roundf(640.0f * k) + ox, w = roundf(300.0f * k), h = roundf(16.0f * k), bw = fmaxf(1.0f, roundf(2.0f * k));
            Rectangle box = { x0, roundf(cy - h * 0.5f), w, h };
            DrawRectangleLinesEx(box, bw, Fade(ink, a));
            float inner = w - 4.0f * bw;
            DrawRectangleRec((Rectangle){ x0 + 2.0f * bw, box.y + 2.0f * bw, inner * (float)step / (float)CRTP_STEPS, h - 4.0f * bw },
                             Fade(ink, a));
            float mid = roundf(x0 + w * 0.5f);   /* the stock value: a notch above and below the bar */
            DrawRectangleRec((Rectangle){ mid - bw * 0.5f, box.y - roundf(7.0f * k), bw, roundf(7.0f * k) }, Fade(ink, a * 0.8f));
            DrawRectangleRec((Rectangle){ mid - bw * 0.5f, box.y + h, bw, roundf(7.0f * k) }, Fade(ink, a * 0.8f));
        }
        if (sel && embed.crtHold > 0.0f && !embed.crtHoldDone)
            DrawRectangleRec((Rectangle){ bx, cy + barH * 0.5f - roundf(5.0f * k), (divX - bx) * (embed.crtHold / EMBED_CRT_HOLD_SEC),
                             roundf(5.0f * k) }, Fade(selInk, a));
    }
    /* preview: the picture as it will look, CRT Geometry with the values being edited */
    Rectangle area = { roundf(1040.0f * k) + ox, roundf(240.0f * k), roundf(800.0f * k), roundf(600.0f * k) };
    Texture2D tex = embed.tex;
    Rectangle src = { 0, 0, (float)embed.fbw, (float)embed.fbh };
    bool raw = !embed.external || crtSession.rawFrames;   /* the capture of a rawFrames port is unfiltered */
    if (!raw && Rewind_LiveFrame(&tex, &src)) raw = true;
    Rectangle fit = Embed_DestForScreen(screen, area.width, area.height, (int)src.width, (int)src.height);
    if (fit.width > area.width || fit.height > area.height) {   /* a window-sized capture: shrink it into the box */
        float sc = fminf(area.width / src.width, area.height / src.height);
        fit = (Rectangle){ 0, 0, floorf(src.width * sc), floorf(src.height * sc) };
        fit.x = floorf((area.width - fit.width) * 0.5f);
        fit.y = floorf((area.height - fit.height) * 0.5f);
    }
    Rectangle dst = { area.x + fit.x, area.y + fit.y, fit.width, fit.height };
    DrawRectangleRec(area, Fade(BLACK, a));
    if (raw && CrtGeom_Ready()) {
        float v[CRTP_COUNT];
        bool angleSet;
        CrtSteps_Resolve(embed.crtSteps, crtSession.def, v, &angleSet);
        CrtGeom_SetParams(v, angleSet);
        CrtGeom_Draw(tex, src, dst, Embed_CrtScan(screen), Fade(WHITE, a));
        CrtGeom_SetParams(crtSession.val, crtSession.angleSet);
    } else {
        DrawTexturePro(tex, src, dst, (Vector2){ 0 }, 0.0f, Fade(WHITE, a));
    }
    DrawRectangleLinesEx(area, fmaxf(1.0f, roundf(2.0f * k)), Fade(panel, a));
}

void Embed_Draw(Font font, EmbedScreen screen, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc)
{
    if (!embed.active) return;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float k = sh / 1080.0f;
    float a = Embed_MenuOpacity();
    ClearBackground(BLACK);
    Rectangle dst = Embed_DestForScreen(screen, sw, sh, embed.fbw, embed.fbh);
    bool crt = Embed_UseCrt(screen);
    DrawTexturePro_OrCrt(crt, screen, dst);
    if (screen.scanlines && !crt) {   /* a dark band under every NES line */
        float lh = dst.height / embed.fbh;
        for (int y = 0; y < embed.fbh; y++)
            DrawRectangleRec((Rectangle){ dst.x, dst.y + y * lh + lh * 0.6f, dst.width, fmaxf(1.0f, lh * 0.4f) },
                             Fade(BLACK, 0.35f * (1.0f - a)));
    }
    if (embed.blackT > 0.0f) DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, Embed_LoadBlack(embed.blackT)));
    if (embed.loadThreadStarted) {
        const float fs = fmaxf(24.0f, sh * 0.045f);
        Vector2 size = MeasureTextEx(font, text[EMBED_TXT_LOADING], fs, 0.0f);
        DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.55f));
        DrawTextEx(font, text[EMBED_TXT_LOADING], (Vector2){ (sw - size.x) * 0.5f,
                   (sh - size.y) * 0.5f }, fs, 0.0f, WHITE);
        if (embed.statusTimer > 0.0f) {
            Vector2 status = MeasureTextEx(font, embed.statusText, fs * 0.55f, 0.0f);
            DrawTextEx(font, embed.statusText, (Vector2){ (sw - status.x) * 0.5f, sh - 70.0f * k },
                       fs * 0.55f, 0.0f, WHITE);
        }
        return;
    }
    if (a <= 0.0f) {
        if (embed.statusTimer > 0.0f) {
            float fs = fmaxf(20.0f, sh * 0.026f);
            Vector2 status = MeasureTextEx(font, embed.statusText, fs, 0.0f);
            DrawRectangle((int)((sw - status.x) * 0.5f - 20.0f), (int)(sh - 88.0f * k),
                          (int)(status.x + 40.0f), (int)(52.0f * k), Fade(BLACK, 0.72f));
            DrawTextEx(font, embed.statusText, (Vector2){ (sw - status.x) * 0.5f, sh - 76.0f * k },
                       fs, 0.0f, WHITE);
        }
        return;
    }

    /* Backdrop: blurred frame cross-faded in, then dimmed to ~65%. */
    if (!embed.blurReady) Embed_BuildBlur(blur, blurDirLoc);
    float px = dst.width / EMBED_BLUR_W * EMBED_BLUR_PAD, py = dst.height / EMBED_BLUR_H * EMBED_BLUR_PAD;
    DrawTexturePro(embed.blurA.texture,
                   (Rectangle){ 0, 0, (float)embed.blurA.texture.width, -(float)embed.blurA.texture.height },
                   (Rectangle){ dst.x - px, dst.y - py, dst.width + 2.0f * px, dst.height + 2.0f * py },
                   (Vector2){ 0 }, 0.0f, Fade(WHITE, a));
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.35f * a));

    /* Switch Online layout at 1080p, scaled by height; slides in 28 px from the left.
     * Text at 1.5x the 36 px atlas: the reference's widths with this font. */
    const Color panel = { 240, 245, 244, 255 }, selInk = { 246, 80, 59, 255 };
    float ox = roundf(-28.0f * k * (1.0f - a));
    float fs = roundf(54.0f * k), divX = roundf(978.0f * k) + ox;
    DrawRectangleRec((Rectangle){ divX, roundf(40.0f * k), fmaxf(1.0f, roundf(2.0f * k)), roundf(1000.0f * k) },
                     Fade(panel, a));
    int visibleRow = 0;
    const float ce = Embed_MenuEase(embed.crtT), aMain = a * (1.0f - ce);   /* the main list gives way to the sub-screen */
    for (int i = 0; i < EMBED_MENU_COUNT && aMain > 0.0f; i++) {
        if (!Embed_MenuItemAvailable(i)) continue;
        float cy = roundf((300.0f + 92.0f * visibleRow++) * k);
        bool sel = (i == embed.menuSel);
        if (sel) {   /* grey with black text while the slots have the focus */
            float bx = roundf(240.0f * k) + ox;
            DrawRectangleRec((Rectangle){ bx, cy - roundf(36.0f * k), divX - bx, roundf(72.0f * k) },
                             Fade(embed.slotsFocus ? (Color){ 192, 191, 200, 255 } : panel, aMain));
        }
        Vector2 m = MeasureTextEx(font, text[i], fs, 0.0f);
        DrawTextEx(font, text[i], (Vector2){ roundf(889.0f * k) + ox - m.x, roundf(cy - m.y * 0.5f) },
                   fs, 0.0f, Fade(!sel ? WHITE : embed.slotsFocus ? BLACK : selInk, aMain));
    }
    if (embed.stateDir[0] && (!embed.external || embed.externalSlots) && aMain > 0.0f
        && (embed.menuSel == EMBED_MENU_LOAD || embed.menuSel == EMBED_MENU_SAVE))
        Embed_DrawSlots(font, text, k, ox, aMain, panel, selInk);
    if (ce > 0.0f) Embed_DrawCrt(font, screen, text, k, divX, a * ce, panel, selInk);
    if (embed.statusTimer > 0.0f) {
        float statusFs = fmaxf(20.0f, 28.0f * k);
        Vector2 status = MeasureTextEx(font, embed.statusText, statusFs, 0.0f);
        DrawRectangle((int)((sw - status.x) * 0.5f - 20.0f), (int)(sh - 88.0f * k),
                      (int)(status.x + 40.0f), (int)(52.0f * k), Fade(BLACK, 0.72f * a));
        DrawTextEx(font, embed.statusText, (Vector2){ (sw - status.x) * 0.5f, sh - 76.0f * k },
                   statusFs, 0.0f, Fade(WHITE, a));
    }
}

void Embed_DrawBack(EmbedScreen screen, Rectangle card, float t)
{
    if (!embed.active) return;
    Rectangle a = Embed_DestForScreen(screen, (float)GetScreenWidth(), (float)GetScreenHeight(),
                                 embed.fbw, embed.fbh);
    float q = 1.0f - t;
    Embed_DrawZoomCrt(embed.tex, a, card, 1.0f - Embed_BackEase(t), q * q, 0.35f * q,
                      Embed_UseCrt(screen), Embed_CrtScan(screen));
}

void Embed_DrawExternalResumeFrame(EmbedScreen screen)
{
    if (!embed.active || !embed.external) return;
    Rectangle dst = Embed_DestForScreen(screen, (float)GetScreenWidth(), (float)GetScreenHeight(),
                                   embed.fbw, embed.fbh);
    ClearBackground(BLACK);
    DrawTexturePro_OrCrt(Embed_UseCrt(screen), screen, dst);
}
#endif /* _WIN32 */

#endif /* EMBED_IMPLEMENTATION_ONCE */
#endif /* EMBED_IMPLEMENTATION */
