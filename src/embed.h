/* embed.h - games that run inside NSO's window instead of as a child process.
 *
 * A GameEntry with embedRomKey set names an in-process module (dir/binary,
 * e.g. embed/smb/build/libsmbmod.so, see embed/smb/Makefile). NSO dlopen()s
 * it, calls one game frame per NES frame from its own loop, uploads the
 * 256x240 RGB framebuffer to a texture drawn per "<system>.screen", feeds the
 * samples to a raylib AudioStream, and draws a pause menu on top.
 *
 * The inline helpers are pure (no raylib calls) and unit-tested in
 * embed_test.c; the rest is the EMBED_IMPLEMENTATION part main.c includes. */
#ifndef NSO_EMBED_H
#define NSO_EMBED_H

#include <math.h>
#include <stdbool.h>
#include "raylib.h"
#include "../embed/smb/smbmod.h"

/* "<system>.screen" values, same order as main.c SCREEN_MODE_KEYS. */
enum { EMBED_SCREEN_43 = 0, EMBED_SCREEN_PIXEL = 1, EMBED_SCREEN_CRT = 2 };
enum { EMBED_MENU_CONTINUE = 0, EMBED_MENU_RESTART, EMBED_MENU_QUIT, EMBED_MENU_COUNT };

/* Where the 256x240 picture goes on a sw x sh screen.
 * Pixel Perfect: largest integer scale, square pixels (1080p: 4x, 1024x960).
 * 4:3 and CRT: full height at 4:3 (1080p: 1440x1080). Centred, whole pixels. */
static inline Rectangle Embed_DestRect(int mode, float sw, float sh)
{
    float w, h;
    if (mode == EMBED_SCREEN_PIXEL) {
        int s = (int)fminf(sw / SMBMOD_FB_W, sh / SMBMOD_FB_H);
        if (s < 1) s = 1;
        w = (float)(SMBMOD_FB_W * s);
        h = (float)(SMBMOD_FB_H * s);
    } else {
        h = sh;
        w = floorf(h * 4.0f / 3.0f);
        if (w > sw) { w = sw; h = floorf(w * 3.0f / 4.0f); }
    }
    return (Rectangle){ floorf((sw - w) * 0.5f), floorf((sh - h) * 0.5f), w, h };
}

/* NSO draws at 60 FPS, the NES ran at ~60.0988: frames to run this tick from
 * an accumulator (seconds). At most 2 per tick; a longer stall is dropped
 * instead of fast-forwarding. */
static inline int Embed_PaceFrames(double *acc, double dt)
{
    const double step = 1.0 / SMBMOD_NES_FPS;
    if (dt > 0.1) dt = 0.1;
    *acc += dt;
    int n = 0;
    while (*acc >= step && n < 2) { *acc -= step; n++; }
    if (*acc >= step) *acc = 0.0;
    return n;
}

bool Embed_Start(const char *modulePath, const char *romPath);
/* One NSO frame: input, game frames, audio. false = the player chose Quit. */
bool Embed_Update(float dt);
void Embed_Draw(Font font, int screenMode, const char *const items[EMBED_MENU_COUNT]);
float Embed_MenuOpacity(void);
void Embed_Stop(void);

#endif /* NSO_EMBED_H */

#ifdef EMBED_IMPLEMENTATION
#ifndef EMBED_IMPLEMENTATION_ONCE
#define EMBED_IMPLEMENTATION_ONCE

#include <string.h>
#include "nso.h"

#if defined(_WIN32)
/* Not built for Windows yet: embedded cards stay unlaunchable there. */
bool Embed_Start(const char *modulePath, const char *romPath) { (void)modulePath; (void)romPath; return false; }
bool Embed_Update(float dt) { (void)dt; return false; }
void Embed_Draw(Font font, int screenMode, const char *const items[EMBED_MENU_COUNT]) { (void)font; (void)screenMode; (void)items; }
float Embed_MenuOpacity(void) { return 0.0f; }
void Embed_Stop(void) {}
#else
#include <dlfcn.h>

#define EMBED_SAMPLE_RATE 48000
#define EMBED_AUDIO_CHUNK 1024
/* Pause menu fade: picom's default fade, as the Details card (main.c). */
#define EMBED_FADE_IN_STEP 0.028f
#define EMBED_FADE_OUT_STEP 0.03f
#define EMBED_FADE_DELTA_MS 10.0f

static struct {
    bool active;
    void *so;
    const SmbmodApi *api;
    SmbmodConfig cfg;
    char rom[2048];
    Texture2D tex;
    bool audio, ownAudioDevice;
    AudioStream stream;
    int16_t pend[EMBED_AUDIO_CHUNK];
    size_t pendN;
    double acc;
    bool menuOpen;
    int menuSel;
    float menuT;
} embed;

bool Embed_Start(const char *modulePath, const char *romPath)
{
    if (embed.active) return false;
    memset(&embed, 0, sizeof(embed));
    embed.so = dlopen(modulePath, RTLD_NOW | RTLD_LOCAL);
    if (!embed.so) return false;
    smbmod_get_api_fn get = (smbmod_get_api_fn)dlsym(embed.so, SMBMOD_ENTRY);
    embed.api = get ? get() : NULL;
    snprintf(embed.rom, sizeof(embed.rom), "%s", romPath);
    embed.cfg = (SmbmodConfig){ .rom_path = embed.rom, .sample_rate = EMBED_SAMPLE_RATE };
    if (!embed.api || embed.api->version != SMBMOD_API_VERSION || !embed.api->init(&embed.cfg)) {
        dlclose(embed.so);
        memset(&embed, 0, sizeof(embed));
        return false;
    }

    Image img = GenImageColor(SMBMOD_FB_W, SMBMOD_FB_H, BLACK);
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
        embed.stream = LoadAudioStream(EMBED_SAMPLE_RATE, 16, 1);
        SetAudioStreamBufferSizeDefault(0);
        embed.audio = IsAudioStreamValid(embed.stream);
        if (embed.audio) PlayAudioStream(embed.stream);
    }

    SetExitKey(KEY_NULL);   /* ESC opens the pause menu instead of closing NSO */
    embed.active = true;
    return true;
}

void Embed_Stop(void)
{
    if (!embed.active) return;
    embed.api->shutdown();
    if (embed.audio) UnloadAudioStream(embed.stream);
    if (embed.ownAudioDevice) CloseAudioDevice();
    UnloadTexture(embed.tex);
    dlclose(embed.so);
    SetExitKey(KEY_ESCAPE);
    memset(&embed, 0, sizeof(embed));
}

/* Keyboard: arrows, X = A, Z = B, Enter = Start, Right Shift = Select.
 * Gamepad: D-pad / left stick, Nintendo A / B (nso.h), Start, Select. */
static SmbmodInput Embed_ReadPad(void)
{
    SmbmodInput in = {
        .up = IsKeyDown(KEY_UP), .down = IsKeyDown(KEY_DOWN),
        .left = IsKeyDown(KEY_LEFT), .right = IsKeyDown(KEY_RIGHT),
        .a = IsKeyDown(KEY_X), .b = IsKeyDown(KEY_Z),
        .start = IsKeyDown(KEY_ENTER), .select = IsKeyDown(KEY_RIGHT_SHIFT),
    };
    if (IsGamepadAvailable(0)) {
        float lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
        float ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        in.up |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || ly < -0.5f;
        in.down |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || ly > 0.5f;
        in.left |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || lx < -0.5f;
        in.right |= IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || lx > 0.5f;
        in.a |= IsGamepadButtonDown(0, NSO_BTN_A);
        in.b |= IsGamepadButtonDown(0, NSO_BTN_B);
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
        embed.pendN += embed.api->audio_pull(embed.pend + embed.pendN, EMBED_AUDIO_CHUNK - embed.pendN);
        if (embed.pendN < EMBED_AUDIO_CHUNK || !IsAudioStreamProcessed(embed.stream)) return;
        UpdateAudioStream(embed.stream, embed.pend, EMBED_AUDIO_CHUNK);
        embed.pendN = 0;
    }
}

static void Embed_SetMenu(bool open)
{
    embed.menuOpen = open;
    embed.menuSel = EMBED_MENU_CONTINUE;
    if (!embed.audio) return;
    if (open) PauseAudioStream(embed.stream);
    else ResumeAudioStream(embed.stream);
}

bool Embed_Update(float dt)
{
    if (!embed.active) return false;
    bool gp = IsGamepadAvailable(0);
    bool toggle = IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_F1)
        || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE));

    float st = dt * 1000.0f / EMBED_FADE_DELTA_MS;
    embed.menuT = embed.menuOpen ? fminf(1.0f, embed.menuT + st * EMBED_FADE_IN_STEP)
                                 : fmaxf(0.0f, embed.menuT - st * EMBED_FADE_OUT_STEP);

    if (embed.menuOpen) {   /* paused: no game frames */
        embed.acc = 0.0;
        if (toggle || IsKeyPressed(KEY_B) || Nso_PadBackPressed(0)) { Embed_SetMenu(false); return true; }
        if (IsKeyPressed(KEY_UP) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP)))
            embed.menuSel = (embed.menuSel + EMBED_MENU_COUNT - 1) % EMBED_MENU_COUNT;
        if (IsKeyPressed(KEY_DOWN) || (gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)))
            embed.menuSel = (embed.menuSel + 1) % EMBED_MENU_COUNT;
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || Nso_PadConfirmPressed(0)) {
            if (embed.menuSel == EMBED_MENU_QUIT) return false;
            if (embed.menuSel == EMBED_MENU_RESTART) {
                embed.api->shutdown();
                if (!embed.api->init(&embed.cfg)) return false;
                embed.pendN = 0;
            }
            Embed_SetMenu(false);
        }
        return true;
    }
    if (toggle) { Embed_SetMenu(true); return true; }

    int n = Embed_PaceFrames(&embed.acc, dt);
    if (n > 0) {
        SmbmodInput p1 = Embed_ReadPad();
        SmbmodFrame f = { 0 };
        for (int i = 0; i < n; i++) f = embed.api->frame(&p1, NULL);
        UpdateTexture(embed.tex, f.rgb24);
    }
    Embed_PumpAudio();
    return true;
}

float Embed_MenuOpacity(void) { return embed.active ? embed.menuT : 0.0f; }

void Embed_Draw(Font font, int screenMode, const char *const items[EMBED_MENU_COUNT])
{
    if (!embed.active) return;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    ClearBackground(BLACK);
    Rectangle dst = Embed_DestRect(screenMode, sw, sh);
    DrawTexturePro(embed.tex, (Rectangle){ 0, 0, SMBMOD_FB_W, SMBMOD_FB_H }, dst,
                   (Vector2){ 0 }, 0.0f, WHITE);
    if (screenMode == EMBED_SCREEN_CRT) {   /* a dark band under every NES line */
        float lh = dst.height / SMBMOD_FB_H;
        for (int y = 0; y < SMBMOD_FB_H; y++)
            DrawRectangleRec((Rectangle){ dst.x, dst.y + y * lh + lh * 0.6f, dst.width, fmaxf(1.0f, lh * 0.4f) },
                             Fade(BLACK, 0.35f));
    }

    float a = embed.menuT;
    if (a <= 0.0f) return;
    /* Details card colours (main.c Details_Draw): light panel, dark ink. */
    const Color ink = { 44, 49, 50, 255 }, panel = { 240, 245, 244, 255 }, sel = { 0, 190, 255, 255 };
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.40f * a));
    float fs = (float)font.baseSize, rowH = roundf(sh * 0.074f), gap = roundf(sh * 0.012f);
    float pw = roundf(sw * 0.26f), ph = rowH * EMBED_MENU_COUNT + gap * (EMBED_MENU_COUNT + 1);
    Rectangle card = { roundf((sw - pw) * 0.5f), roundf((sh - ph) * 0.5f), pw, ph };
    DrawRectangleRounded(card, 2.0f * sh * 0.012f / fminf(card.width, card.height), 12, Fade(panel, a));
    for (int i = 0; i < EMBED_MENU_COUNT; i++) {
        Rectangle r = { card.x + gap, card.y + gap + i * (rowH + gap), card.width - 2.0f * gap, rowH };
        if (i == embed.menuSel) DrawRectangleRoundedLinesEx(r, 0.25f, 8, 4.0f, Fade(sel, a));
        Vector2 m = MeasureTextEx(font, items[i], fs, 2.0f);
        DrawTextEx(font, items[i], (Vector2){ r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f },
                   fs, 2.0f, Fade(ink, a));
    }
}
#endif /* _WIN32 */

#endif /* EMBED_IMPLEMENTATION_ONCE */
#endif /* EMBED_IMPLEMENTATION */
