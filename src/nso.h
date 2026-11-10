/* nso.h - STB-style shared types and platform helpers */
#ifndef NSO_H
#define NSO_H

#include "raylib.h"

/* USA = landscape 4:3, JAPAN = portrait 3:4 */
typedef enum {
    SLOT_ORIENT_USA = 0,
    SLOT_ORIENT_JAPAN = 1,
} SlotOrientation;

/* Game-only: use platform default orientation */
#define SLOT_ORIENT_INHERIT (-1)

typedef struct {
    const char *id;
    const char *name;
    const char *imagePath;
    Texture2D texture;
    Color bg;               /* card fill, sampled from the art's edge */
    SlotOrientation orient; /* system-wide default for this platform */
} PlatformEntry;

typedef struct {
    const char *title;
    const char *boxartPath;
    const char *dir;      /* NULL = display-only */
    const char *binary;   /* NULL = display-only */
    const char *stateDir; /* NULL = no save states; else NSO-relative dir */
    int orient;           /* SLOT_ORIENT_INHERIT, SLOT_ORIENT_USA, or SLOT_ORIENT_JAPAN */
    const struct NsoRomImport *romImport; /* NULL = none; auto-import in launch.h */
} GameEntry;

void Nso_PlatformInit(PlatformEntry *p);
void Nso_PlatformUnload(PlatformEntry *p);

/* Resolve per-game orient (game overrides platform when not INHERIT). */
static inline SlotOrientation Nso_ResolveOrient(const PlatformEntry *plat,
                                                const GameEntry *game)
{
    if (game && game->orient != SLOT_ORIENT_INHERIT) {
        if (game->orient == (int)SLOT_ORIENT_JAPAN) {
            return SLOT_ORIENT_JAPAN;
        }
        return SLOT_ORIENT_USA;
    }
    if (plat) {
        return plat->orient;
    }
    return SLOT_ORIENT_USA;
}

static inline float Nso_OrientAspect(SlotOrientation o)
{
    return (o == SLOT_ORIENT_JAPAN) ? (3.0f / 4.0f) : (4.0f / 3.0f);
}

/* Nintendo face buttons. Raylib's RIGHT_FACE_* names follow Xbox positions. */
#define NSO_BTN_A GAMEPAD_BUTTON_RIGHT_FACE_RIGHT /* confirm */
#define NSO_BTN_B GAMEPAD_BUTTON_RIGHT_FACE_DOWN  /* back */
#define NSO_BTN_X GAMEPAD_BUTTON_RIGHT_FACE_UP
#define NSO_BTN_Y GAMEPAD_BUTTON_RIGHT_FACE_LEFT

/* 1 = confirm, -1 = back, 0 = neither. Start also confirms. */
static inline int Nso_PadConfirmBack(int button)
{
    if (button == NSO_BTN_A || button == GAMEPAD_BUTTON_MIDDLE_RIGHT) {
        return 1;
    }
    if (button == NSO_BTN_B) {
        return -1;
    }
    return 0;
}

static inline bool Nso_PadConfirmPressed(int pad)
{
    return IsGamepadButtonPressed(pad, NSO_BTN_A)
        || IsGamepadButtonPressed(pad, GAMEPAD_BUTTON_MIDDLE_RIGHT);
}

static inline bool Nso_PadBackPressed(int pad)
{
    return IsGamepadButtonPressed(pad, NSO_BTN_B);
}

#endif /* NSO_H */

#ifdef NSO_IMPLEMENTATION
#ifndef NSO_IMPLEMENTATION_ONCE
#define NSO_IMPLEMENTATION_ONCE

const char *GetDataPath(const char *relativePath);   /* main.c: base dir + rel */

void Nso_PlatformInit(PlatformEntry *p)
{
    if (!p || !p->imagePath) {
        return;
    }
    /* Art is not shipped in the repo: no file -> no texture (name is drawn). */
    if (!FileExists(GetDataPath(p->imagePath))) {
        return;
    }
    Image img = LoadImage(GetDataPath(p->imagePath));
    if (!img.data) {
        return;
    }
    /* Card colour = art background just inside the square centre crop. */
    p->bg = GetImageColor(img, (img.width - img.height) / 2 + 4, 4);
    p->texture = LoadTextureFromImage(img);
    UnloadImage(img);
    /* Mipmaps keep the 1080px art clean when shrunk onto a small card. */
    GenTextureMipmaps(&p->texture);
    SetTextureFilter(p->texture, TEXTURE_FILTER_TRILINEAR);
}

void Nso_PlatformUnload(PlatformEntry *p)
{
    if (!p) {
        return;
    }
    if (p->texture.id) {
        UnloadTexture(p->texture);
    }
    p->texture = (Texture2D){0};
}

#endif /* NSO_IMPLEMENTATION_ONCE */
#endif /* NSO_IMPLEMENTATION */
