/* embed.h - games that run inside NSO's window instead of as a child process.
 *
 * A GameEntry with embedRomKey set names an in-process module (dir/binary,
 * e.g. embed/smb/build/libsmbmod.so; API in embed/nsomod.h). NSO dlopen()s
 * it, calls one game frame per console frame from its own loop, uploads the
 * RGB framebuffer (NES 256x240, SNES 256x224, N64 640x480) to a texture drawn per
 * "<system>.screen", feeds the samples (mono or stereo) to a raylib
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

/* "<system>.screen" values, same order as main.c SCREEN_MODE_KEYS. */
enum { EMBED_SCREEN_43 = 0, EMBED_SCREEN_PIXEL = 1, EMBED_SCREEN_CRT = 2 };
enum { EMBED_MENU_CONTINUE = 0, EMBED_MENU_LOAD, EMBED_MENU_SAVE, EMBED_MENU_RESTART, EMBED_MENU_QUIT,
       EMBED_MENU_COUNT };
/* Embed_Draw texts: the items, then the slot label ("%d"), empty slot, date (strftime). */
enum { EMBED_TXT_SLOT = EMBED_MENU_COUNT, EMBED_TXT_NODATA, EMBED_TXT_DATE, EMBED_TXT_COUNT };
#define EMBED_SLOTS 4

/* Where a fw x fh picture goes on a sw x sh screen.
 * Pixel Perfect: largest integer scale, square pixels (1080p: 4x, NES 1024x960).
 * 4:3 and CRT: full height at 4:3 (1080p: 1440x1080). Centred, whole pixels. */
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
/* One NSO frame: input, game frames, audio. false = the player chose Quit. */
bool Embed_Update(float dt);
/* blur/blurDirLoc: main.c's separable 5-tap shader (id 0 = downscale only). */
void Embed_Draw(Font font, int screenMode, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc);
/* After Embed_Update returned false: the frozen frame at t (0..1) of the way into card. */
void Embed_DrawBack(int screenMode, Rectangle card, float t);
void Embed_DrawZoom(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim);
float Embed_MenuOpacity(void);
void Embed_Stop(void);

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

/* Screen dimmed by dim, then tex (id 0: a black panel) at p of the way from card to game. */
void Embed_DrawZoom(Texture2D tex, Rectangle game, Rectangle card, float p, float alpha, float dim)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, dim));
    Rectangle r = Embed_ZoomRect(card, game, p);
    if (tex.id) DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height }, r,
                               (Vector2){ 0 }, 0.0f, Fade(WHITE, alpha));
    else DrawRectangleRec(r, Fade(BLACK, alpha));
}

#if defined(_WIN32)
/* Not built for Windows yet: embedded cards stay unlaunchable there. */
bool Embed_Start(const char *modulePath, const char *dataPath, const char *stateDir, const char *stateLoad)
{ (void)modulePath; (void)dataPath; (void)stateDir; (void)stateLoad; return false; }
bool Embed_Update(float dt) { (void)dt; return false; }
void Embed_Draw(Font font, int screenMode, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc)
{ (void)font; (void)screenMode; (void)text; (void)blur; (void)blurDirLoc; }
void Embed_DrawBack(int screenMode, Rectangle card, float t) { (void)screenMode; (void)card; (void)t; }
float Embed_MenuOpacity(void) { return 0.0f; }
void Embed_Stop(void) {}
#else
#include <dlfcn.h>

#define EMBED_SAMPLE_RATE 48000
#define EMBED_AUDIO_CHUNK 1024
/* Menu backdrop: the frozen frame at half resolution plus a black
 * margin (so the blur bleeds into the bars), blurred once per opening. */
#define EMBED_BLUR_W (embed.fbw / 2)
#define EMBED_BLUR_H (embed.fbh / 2)
#define EMBED_BLUR_PAD 16

static struct {
    bool active;
    void *so;
    const NsomodApi *api;
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
    RenderTexture2D blurA, blurB;
    bool blurReady;
    bool quitting;   /* To Game Selection: false once the menu exit ends */
    char stateDir[1100];                   /* "" = no suspend points */
    const uint8_t *last;                   /* last frame (thumbnail), module-owned */
    bool slotsFocus;
    int slotSel, loadSlot;                 /* loadSlot: 1..EMBED_SLOTS once the menu exit ends */
    float blackT;                          /* Embed_LoadBlack countdown */
    long slotTime[EMBED_SLOTS];            /* .sav mtime, 0 = empty */
    Texture2D thumb[EMBED_SLOTS];
} embed;

bool Embed_Start(const char *modulePath, const char *dataPath, const char *stateDir, const char *stateLoad)
{
    if (embed.active) return false;
    memset(&embed, 0, sizeof(embed));
    embed.so = dlopen(modulePath, RTLD_NOW | RTLD_LOCAL);
    if (!embed.so) return false;
    nsomod_get_api_fn get = (nsomod_get_api_fn)dlsym(embed.so, NSOMOD_ENTRY);
    embed.api = get ? get() : NULL;
    snprintf(embed.data, sizeof(embed.data), "%s", dataPath);
    snprintf(embed.module, sizeof(embed.module), "%s", modulePath);
    embed.cfg = (NsomodConfig){ .data_path = embed.data, .sample_rate = EMBED_SAMPLE_RATE };
    if (!embed.api || embed.api->version != NSOMOD_API_VERSION || embed.api->fb_w <= 0 || embed.api->fb_h <= 0
        || (embed.api->channels != 1 && embed.api->channels != 2) || !embed.api->init(&embed.cfg)) {
        dlclose(embed.so);
        memset(&embed, 0, sizeof(embed));
        return false;
    }
    if (stateLoad && stateLoad[0] && embed.api->state_load) embed.api->state_load(stateLoad);
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

    SetExitKey(KEY_NULL);   /* ESC must not close NSO mid-game */
    embed.active = true;
    return true;
}

void Embed_Stop(void)
{
    if (!embed.active) return;
    if (embed.so) { embed.api->shutdown(); dlclose(embed.so); }
    if (embed.audio) UnloadAudioStream(embed.stream);
    if (embed.ownAudioDevice) CloseAudioDevice();
    UnloadTexture(embed.tex);
    if (embed.blurA.id) { UnloadRenderTexture(embed.blurA); UnloadRenderTexture(embed.blurB); }
    for (int i = 0; i < EMBED_SLOTS; i++) if (embed.thumb[i].id) UnloadTexture(embed.thumb[i]);
    SetExitKey(KEY_ESCAPE);
    memset(&embed, 0, sizeof(embed));
}

/* Keyboard: arrows, X = A, Z = B, Enter = Start, Right Shift = Select;
 * SNES also S = X, A = Y, C = L, V = R (the smw port's own defaults);
 * N64 also Left Shift = Z, I / J / K / L = C buttons (right stick).
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
        float rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
        float ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
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
    embed.api->shutdown();
    dlclose(embed.so);
    embed.so = dlopen(embed.module, RTLD_NOW | RTLD_LOCAL);
    nsomod_get_api_fn get = embed.so ? (nsomod_get_api_fn)dlsym(embed.so, NSOMOD_ENTRY) : NULL;
    const NsomodApi *api = get ? get() : NULL;
    if (!api || api->version != NSOMOD_API_VERSION || api->fb_w != embed.fbw
        || api->fb_h != embed.fbh || api->channels != embed.channels || !api->init(&embed.cfg)) {
        if (embed.so) dlclose(embed.so);
        embed.so = NULL;
        return false;
    }
    embed.api = api;
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
    char sav[1200], tmp[1200], bmp[1200];
    if (!Embed_SlotPath(sav, sizeof(sav), i, ".sav") || !Embed_SlotPath(tmp, sizeof(tmp), i, ".sav.tmp")
        || !Embed_SlotPath(bmp, sizeof(bmp), i, ".bmp")) return;
    if (!embed.api->state_save(tmp) || rename(tmp, sav) != 0) { remove(tmp); return; }
    if (embed.last) {
        Image img = { .data = (void *)embed.last, .width = embed.fbw, .height = embed.fbh,
                      .mipmaps = 1, .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8 };
        ExportImage(img, bmp);
    }
    Embed_ScanSlot(i);
}

/* After the menu exit: load the slot, show its first frame under the black. */
static void Embed_LoadSlot(int i)
{
    char sav[1200];
    if (!Embed_SlotPath(sav, sizeof(sav), i, ".sav") || !embed.api->state_load(sav)) return;
    NsomodInput none = { 0 };
    NsomodFrame f = embed.api->frame(&none);
    UpdateTexture(embed.tex, f.rgb24);
    embed.last = f.rgb24;
    embed.blackT = EMBED_LOAD_SEC;
}

/* Opening pauses game and audio; both resume once the exit animation ends. */
static void Embed_SetMenu(bool open)
{
    embed.menuOpen = open;
    if (!open) return;
    embed.menuSel = EMBED_MENU_CONTINUE;
    embed.slotsFocus = false;
    embed.blurReady = false;
    for (int i = 0; i < EMBED_SLOTS; i++) Embed_ScanSlot(i);
    if (embed.audio) PauseAudioStream(embed.stream);
    Sfx_Play(SFX_AUDIO);
}

/* Slot list (Load / Create focused right of the divider). */
static void Embed_SlotsInput(bool gp)
{
    bool load = (embed.menuSel == EMBED_MENU_LOAD);
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_LEFT) || Nso_PadBackPressed(0)
        || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT))) {
        embed.slotsFocus = false;
        return;
    }
    if ((IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))) && embed.slotSel > 0) {
        embed.slotSel--;
        Sfx_Play(SFX_NAVIGATE);
    }
    if ((IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)))
        && embed.slotSel + 1 < EMBED_SLOTS) {
        embed.slotSel++;
        Sfx_Play(SFX_NAVIGATE);
    }
    if (!(IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || Nso_PadConfirmPressed(0))) return;
    Sfx_Play(SFX_CLICK);
    if (!load) Embed_SaveSlot(embed.slotSel);
    else if (embed.slotTime[embed.slotSel]) { embed.loadSlot = embed.slotSel + 1; Embed_SetMenu(false); }
}

bool Embed_Update(float dt)
{
    if (!embed.active) return false;
    bool gp = IsGamepadAvailable(0);
    bool toggle = IsKeyPressed(KEY_END) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE));

    float st = dt / EMBED_MENU_ANIM_SEC;
    if (embed.menuOpen) embed.menuT = fminf(1.0f, embed.menuT + st);
    else if (embed.menuT > 0.0f) {   /* exit animation: still paused */
        embed.menuT = fmaxf(0.0f, embed.menuT - st);
        embed.acc = 0.0;
        if (embed.menuT <= 0.0f && embed.quitting) return false;
        if (embed.menuT <= 0.0f && embed.loadSlot) { Embed_LoadSlot(embed.loadSlot - 1); embed.loadSlot = 0; }
        if (embed.menuT <= 0.0f && embed.audio && embed.blackT <= 0.0f) ResumeAudioStream(embed.stream);
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
        if (toggle) { Embed_SetMenu(false); return true; }
        if (embed.slotsFocus) { Embed_SlotsInput(gp); return true; }
        if (IsKeyPressed(KEY_B) || Nso_PadBackPressed(0)) { Embed_SetMenu(false); return true; }
        if (IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP))) {
            embed.menuSel = (embed.menuSel + EMBED_MENU_COUNT - 1) % EMBED_MENU_COUNT;
            Sfx_Play(SFX_NAVIGATE);
        }
        if (IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN))) {
            embed.menuSel = (embed.menuSel + 1) % EMBED_MENU_COUNT;
            Sfx_Play(SFX_NAVIGATE);
        }
        bool slots = embed.menuSel == EMBED_MENU_LOAD || embed.menuSel == EMBED_MENU_SAVE;
        bool right = IsKeyPressed(KEY_RIGHT) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
        bool ok = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || Nso_PadConfirmPressed(0);
        if (slots && (ok || right)) {
            if (embed.stateDir[0]) { embed.slotsFocus = true; embed.slotSel = 0; Sfx_Play(SFX_CLICK); }
        } else if (ok) {
            Sfx_Play(SFX_CLICK);
            if (embed.menuSel == EMBED_MENU_QUIT) embed.quitting = true;
            if (embed.menuSel == EMBED_MENU_RESTART) {
                if (!Embed_Reload()) return false;
            }
            Embed_SetMenu(false);
        }
        return true;
    }
    if (toggle) { Embed_SetMenu(true); return true; }

    int n = Embed_PaceFrames(&embed.acc, dt, embed.fps);
    if (n > 0) {
        NsomodInput p1 = Embed_ReadPad();
        NsomodFrame f = { 0 };
        for (int i = 0; i < n; i++) f = embed.api->frame(&p1);
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
    SetTextureFilter(embed.tex, TEXTURE_FILTER_POINT);
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

void Embed_Draw(Font font, int screenMode, const char *const text[EMBED_TXT_COUNT], Shader blur, int blurDirLoc)
{
    if (!embed.active) return;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float a = Embed_MenuOpacity();
    ClearBackground(BLACK);
    Rectangle dst = Embed_DestRect(screenMode, sw, sh, embed.fbw, embed.fbh);
    DrawTexturePro(embed.tex, (Rectangle){ 0, 0, (float)embed.fbw, (float)embed.fbh }, dst,
                   (Vector2){ 0 }, 0.0f, WHITE);
    if (screenMode == EMBED_SCREEN_CRT) {   /* a dark band under every NES line */
        float lh = dst.height / embed.fbh;
        for (int y = 0; y < embed.fbh; y++)
            DrawRectangleRec((Rectangle){ dst.x, dst.y + y * lh + lh * 0.6f, dst.width, fmaxf(1.0f, lh * 0.4f) },
                             Fade(BLACK, 0.35f * (1.0f - a)));
    }
    if (embed.blackT > 0.0f) DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, Embed_LoadBlack(embed.blackT)));
    if (a <= 0.0f) return;

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
    float k = sh / 1080.0f, ox = roundf(-28.0f * k * (1.0f - a));
    float fs = roundf(54.0f * k), divX = roundf(978.0f * k) + ox;
    DrawRectangleRec((Rectangle){ divX, roundf(40.0f * k), fmaxf(1.0f, roundf(2.0f * k)), roundf(1000.0f * k) },
                     Fade(panel, a));
    for (int i = 0; i < EMBED_MENU_COUNT; i++) {
        float cy = roundf((300.0f + 92.0f * i) * k);
        bool sel = (i == embed.menuSel);
        if (sel) {   /* grey with black text while the slots have the focus */
            float bx = roundf(240.0f * k) + ox;
            DrawRectangleRec((Rectangle){ bx, cy - roundf(36.0f * k), divX - bx, roundf(72.0f * k) },
                             Fade(embed.slotsFocus ? (Color){ 192, 191, 200, 255 } : panel, a));
        }
        Vector2 m = MeasureTextEx(font, text[i], fs, 0.0f);
        DrawTextEx(font, text[i], (Vector2){ roundf(889.0f * k) + ox - m.x, roundf(cy - m.y * 0.5f) },
                   fs, 0.0f, Fade(!sel ? WHITE : embed.slotsFocus ? BLACK : selInk, a));
    }
    if (embed.stateDir[0] && (embed.menuSel == EMBED_MENU_LOAD || embed.menuSel == EMBED_MENU_SAVE))
        Embed_DrawSlots(font, text, k, ox, a, panel, selInk);
}

void Embed_DrawBack(int screenMode, Rectangle card, float t)
{
    if (!embed.active) return;
    Rectangle a = Embed_DestRect(screenMode, (float)GetScreenWidth(), (float)GetScreenHeight(),
                                 embed.fbw, embed.fbh);
    float q = 1.0f - t;
    Embed_DrawZoom(embed.tex, a, card, 1.0f - Embed_BackEase(t), q * q, 0.35f * q);
}
#endif /* _WIN32 */

#endif /* EMBED_IMPLEMENTATION_ONCE */
#endif /* EMBED_IMPLEMENTATION */
