/* padmap.h - controller configuration, NES first (Settings > Controller).
 *
 * One mapping for every NES game NSO starts: Up, Down, Left, Right, Select,
 * Start, B, A, each with a keyboard key and a gamepad button. config/nso.cfg:
 *   nes.map.<button>.key = SDL key name ("Z", "Left Shift", "Return"), or "none"
 *   nes.map.<button>.pad = SDL controller button ("a", "dpup", "lefttrigger+"), or "none"
 *   nes.map.<button>.js  = the raw Linux joystick control ("b3", "a2+", "a2-"), or "none"
 *   nes.map.joydev       = Mednafen's id of the pad the raw controls were read from
 * <button> is up|down|left|right|select|start|b|a. Without any nes.map.* key the
 * mapping is the default (arrows, Z = B, X = A, Enter = Start, Right Shift = Select,
 * the pad as NSO reads it) and nothing is handed to the games.
 *
 * key and pad are raylib values (embedded games and the on-screen controller read
 * them through raylib). The same controls are written for the games NSO does not
 * run itself: Mednafen takes keys by SDL scancode and a pad by the raw js index
 * (+ the device id), the NESRecomp ports take SDL names in their config.ini.
 *
 * SNES is the same with twelve buttons (snes.map.<button>.*): up|down|left|right|select|
 * start|y|b|x|a|l|r. One PadSys descriptor per system carries its names and defaults; the
 * helpers take it, and PadMap_Nes* are the NES shorthands. Besides Mednafen (snes.input.*)
 * the SNES map reaches the zelda3 / sm ports (zelda3.ini, sm.ini: Controls lists under
 * [KeyMap] and [GamepadMap]) and the snesrecomp ports (keybinds.ini [player1], keyboard only).
 * N64 (n64.map.<button>.*): up|down|left|right (D-pad), stickup|stickdown|stickleft|stickright,
 * start|a|b|z|l|r, cup|cdown|cleft|cright. It reaches the games NSO runs itself (embed.h); no N64
 * port reads it (their SDL controller setups are their own) and there is no Mednafen N64 module.
 * GameCube (gamecube.map.<button>.*): up|down|left|right (D-pad), stickup|stickdown|stickleft|stickright,
 * start|a|b|x|y|z|l|r, lanalog|ranalog (the analog L / R triggers), cup|cdown|cleft|cright (C-stick).
 * The Settings menu stores the map the same way; it is not handed to an emulator yet.
 * Pure helpers (no window): unit-tested in padmap_test.c. */
#ifndef NSO_PADMAP_H
#define NSO_PADMAP_H

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "raylib.h"
#include "lang.h"

enum { NES_BTN_UP, NES_BTN_DOWN, NES_BTN_LEFT, NES_BTN_RIGHT, NES_BTN_SELECT, NES_BTN_START,
       NES_BTN_B, NES_BTN_A, NES_BTN_COUNT };
enum { SNES_BTN_UP, SNES_BTN_DOWN, SNES_BTN_LEFT, SNES_BTN_RIGHT, SNES_BTN_SELECT, SNES_BTN_START,
       SNES_BTN_Y, SNES_BTN_B, SNES_BTN_X, SNES_BTN_A, SNES_BTN_L, SNES_BTN_R,
       SNES_BTN_ZL, SNES_BTN_ZR, SNES_BTN_COUNT };   /* ZL / ZR: the Switch Online pad's, no SNES game reads them */
/* N64: the D-pad, the analog stick as four directions (digital; the real stick stays analog
 * in the games), Start, A, B, Z, L, R and the four C buttons. */
enum { N64_BTN_UP, N64_BTN_DOWN, N64_BTN_LEFT, N64_BTN_RIGHT,
       N64_BTN_SUP, N64_BTN_SDOWN, N64_BTN_SLEFT, N64_BTN_SRIGHT,
       N64_BTN_START, N64_BTN_A, N64_BTN_B, N64_BTN_Z, N64_BTN_L, N64_BTN_R,
       N64_BTN_CUP, N64_BTN_CDOWN, N64_BTN_CLEFT, N64_BTN_CRIGHT, N64_BTN_COUNT };
/* GameCube: D-pad, main stick, Start, A, B, X, Y, Z, L / R digital, L / R analog, C-stick. */
enum { GC_BTN_UP, GC_BTN_DOWN, GC_BTN_LEFT, GC_BTN_RIGHT,
       GC_BTN_SUP, GC_BTN_SDOWN, GC_BTN_SLEFT, GC_BTN_SRIGHT,
       GC_BTN_START, GC_BTN_A, GC_BTN_B, GC_BTN_X, GC_BTN_Y, GC_BTN_Z,
       GC_BTN_L, GC_BTN_R, GC_BTN_LA, GC_BTN_RA,
       GC_BTN_CUP, GC_BTN_CDOWN, GC_BTN_CLEFT, GC_BTN_CRIGHT, GC_BTN_COUNT };
#define PAD_MAX_BTN 22
enum { PAD_JS_NONE = 0, PAD_JS_BUTTON = 1, PAD_JS_AXIS_NEG = 2, PAD_JS_AXIS_POS = 3 };

typedef struct {
    int key;       /* raylib KEY_*, 0 = none */
    int pad;       /* raylib GAMEPAD_BUTTON_*, -1 = none */
    int jsKind;    /* PAD_JS_* */
    int jsIdx;
} PadBind;

typedef struct {
    PadBind b[PAD_MAX_BTN];
    char joyId[40];      /* 32 hex digits, "" = unknown */
    bool custom;         /* some <id>.map.* key is in the config: hand the mapping to the games */
} PadSysMap;
typedef PadSysMap NesPadMap;

/* One controller system: config prefix and Mednafen module (id), its buttons (config names, in
 * the order they are asked for; the first gameCount are the console's own, the games and ports
 * get only those), the defaults, which of them are A and B (the holds) and ZR (-1 if none). */
typedef struct {
    const char *id;
    int count, gameCount;
    const char *const *names;
    const PadBind *def;
    int btnA, btnB, btnZR;
} PadSys;

static const char *const PADMAP_NES_NAMES[NES_BTN_COUNT] = { "up", "down", "left", "right", "select", "start", "b", "a" };
static const char *const PADMAP_SNES_NAMES[SNES_BTN_COUNT] = { "up", "down", "left", "right", "select", "start", "y", "b", "x", "a", "l", "r", "zl", "zr" };
static const char *const PADMAP_N64_NAMES[N64_BTN_COUNT] = { "up", "down", "left", "right", "stickup", "stickdown", "stickleft", "stickright",
                                                             "start", "a", "b", "z", "l", "r", "cup", "cdown", "cleft", "cright" };
static const char *const PADMAP_GC_NAMES[GC_BTN_COUNT] = { "up", "down", "left", "right", "stickup", "stickdown", "stickleft", "stickright",
                                                            "start", "a", "b", "x", "y", "z", "l", "r", "lanalog", "ranalog",
                                                            "cup", "cdown", "cleft", "cright" };

/* Keys that can be bound: raylib key, SDL scancode (= USB HID usage, what Mednafen
 * stores), SDL name (what the NESRecomp ports store). Escape, Backspace (rewind),
 * End and the function keys are NSO's or the emulator's own. */
typedef struct { int key; int scan; const char *name; } PadKeyInfo;
static const PadKeyInfo PADMAP_KEYS[] = {
    { KEY_A, 4, "A" }, { KEY_B, 5, "B" }, { KEY_C, 6, "C" }, { KEY_D, 7, "D" }, { KEY_E, 8, "E" },
    { KEY_F, 9, "F" }, { KEY_G, 10, "G" }, { KEY_H, 11, "H" }, { KEY_I, 12, "I" }, { KEY_J, 13, "J" },
    { KEY_K, 14, "K" }, { KEY_L, 15, "L" }, { KEY_M, 16, "M" }, { KEY_N, 17, "N" }, { KEY_O, 18, "O" },
    { KEY_P, 19, "P" }, { KEY_Q, 20, "Q" }, { KEY_R, 21, "R" }, { KEY_S, 22, "S" }, { KEY_T, 23, "T" },
    { KEY_U, 24, "U" }, { KEY_V, 25, "V" }, { KEY_W, 26, "W" }, { KEY_X, 27, "X" }, { KEY_Y, 28, "Y" },
    { KEY_Z, 29, "Z" },
    { KEY_ONE, 30, "1" }, { KEY_TWO, 31, "2" }, { KEY_THREE, 32, "3" }, { KEY_FOUR, 33, "4" },
    { KEY_FIVE, 34, "5" }, { KEY_SIX, 35, "6" }, { KEY_SEVEN, 36, "7" }, { KEY_EIGHT, 37, "8" },
    { KEY_NINE, 38, "9" }, { KEY_ZERO, 39, "0" },
    { KEY_ENTER, 40, "Return" }, { KEY_TAB, 43, "Tab" }, { KEY_SPACE, 44, "Space" },
    { KEY_MINUS, 45, "-" }, { KEY_EQUAL, 46, "=" }, { KEY_LEFT_BRACKET, 47, "[" },
    { KEY_RIGHT_BRACKET, 48, "]" }, { KEY_BACKSLASH, 49, "\\" }, { KEY_SEMICOLON, 51, ";" },
    { KEY_APOSTROPHE, 52, "'" }, { KEY_GRAVE, 53, "`" }, { KEY_COMMA, 54, "," },
    { KEY_PERIOD, 55, "." }, { KEY_SLASH, 56, "/" },
    { KEY_INSERT, 73, "Insert" }, { KEY_HOME, 74, "Home" }, { KEY_PAGE_UP, 75, "PageUp" },
    { KEY_DELETE, 76, "Delete" }, { KEY_PAGE_DOWN, 78, "PageDown" },
    { KEY_RIGHT, 79, "Right" }, { KEY_LEFT, 80, "Left" }, { KEY_DOWN, 81, "Down" }, { KEY_UP, 82, "Up" },
    { KEY_KP_DIVIDE, 84, "Keypad /" }, { KEY_KP_MULTIPLY, 85, "Keypad *" },
    { KEY_KP_SUBTRACT, 86, "Keypad -" }, { KEY_KP_ADD, 87, "Keypad +" }, { KEY_KP_ENTER, 88, "Keypad Enter" },
    { KEY_KP_1, 89, "Keypad 1" }, { KEY_KP_2, 90, "Keypad 2" }, { KEY_KP_3, 91, "Keypad 3" },
    { KEY_KP_4, 92, "Keypad 4" }, { KEY_KP_5, 93, "Keypad 5" }, { KEY_KP_6, 94, "Keypad 6" },
    { KEY_KP_7, 95, "Keypad 7" }, { KEY_KP_8, 96, "Keypad 8" }, { KEY_KP_9, 97, "Keypad 9" },
    { KEY_KP_0, 98, "Keypad 0" }, { KEY_KP_DECIMAL, 99, "Keypad ." },
    { KEY_LEFT_CONTROL, 224, "Left Ctrl" }, { KEY_LEFT_SHIFT, 225, "Left Shift" },
    { KEY_LEFT_ALT, 226, "Left Alt" }, { KEY_RIGHT_CONTROL, 228, "Right Ctrl" },
    { KEY_RIGHT_SHIFT, 229, "Right Shift" }, { KEY_RIGHT_ALT, 230, "Right Alt" },
};
#define PADMAP_KEY_COUNT ((int)(sizeof(PADMAP_KEYS) / sizeof(PADMAP_KEYS[0])))

static inline const PadKeyInfo *PadMap_KeyInfo(int key)
{
    for (int i = 0; i < PADMAP_KEY_COUNT; i++) if (PADMAP_KEYS[i].key == key) return &PADMAP_KEYS[i];
    return NULL;
}

static inline bool PadMap_KeyBindable(int key) { return PadMap_KeyInfo(key) != NULL; }

static inline int PadMap_KeyByName(const char *name)
{
    for (int i = 0; name && i < PADMAP_KEY_COUNT; i++) {
        const char *a = PADMAP_KEYS[i].name, *b = name;
        while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) { a++; b++; }
        if (!*a && !*b) return PADMAP_KEYS[i].key;
    }
    return 0;
}

/* Gamepad buttons (raylib's standard layout): SDL name, and the Nintendo label NSO shows. */
typedef struct { int btn; const char *sdl; const char *label; } PadBtnInfo;
static const PadBtnInfo PADMAP_PADS[] = {
    { GAMEPAD_BUTTON_LEFT_FACE_UP, "dpup", "D-Pad Up" }, { GAMEPAD_BUTTON_LEFT_FACE_DOWN, "dpdown", "D-Pad Down" },
    { GAMEPAD_BUTTON_LEFT_FACE_LEFT, "dpleft", "D-Pad Left" }, { GAMEPAD_BUTTON_LEFT_FACE_RIGHT, "dpright", "D-Pad Right" },
    { GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, "b", "A" }, { GAMEPAD_BUTTON_RIGHT_FACE_DOWN, "a", "B" },
    { GAMEPAD_BUTTON_RIGHT_FACE_UP, "y", "X" }, { GAMEPAD_BUTTON_RIGHT_FACE_LEFT, "x", "Y" },
    { GAMEPAD_BUTTON_LEFT_TRIGGER_1, "leftshoulder", "L" }, { GAMEPAD_BUTTON_RIGHT_TRIGGER_1, "rightshoulder", "R" },
    { GAMEPAD_BUTTON_LEFT_TRIGGER_2, "lefttrigger+", "ZL" }, { GAMEPAD_BUTTON_RIGHT_TRIGGER_2, "righttrigger+", "ZR" },
    { GAMEPAD_BUTTON_MIDDLE_LEFT, "back", "-" }, { GAMEPAD_BUTTON_MIDDLE_RIGHT, "start", "+" },
    { GAMEPAD_BUTTON_LEFT_THUMB, "leftstick", "L Stick" }, { GAMEPAD_BUTTON_RIGHT_THUMB, "rightstick", "R Stick" },
};
#define PADMAP_PAD_COUNT ((int)(sizeof(PADMAP_PADS) / sizeof(PADMAP_PADS[0])))

static inline const PadBtnInfo *PadMap_PadInfo(int btn)
{
    for (int i = 0; i < PADMAP_PAD_COUNT; i++) if (PADMAP_PADS[i].btn == btn) return &PADMAP_PADS[i];
    return NULL;
}

/* The Guide button opens NSO's menu, so it cannot be a game button. */
static inline bool PadMap_PadBindable(int btn) { return PadMap_PadInfo(btn) != NULL; }

static inline int PadMap_PadByName(const char *name)
{
    for (int i = 0; name && i < PADMAP_PAD_COUNT; i++)
        if (strcmp(PADMAP_PADS[i].sdl, name) == 0) return PADMAP_PADS[i].btn;
    return -1;
}

static const PadBind PADMAP_NES_DEF[NES_BTN_COUNT] = {
    { KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP, 0, 0 }, { KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0, 0 },
    { KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT, 0, 0 }, { KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, 0, 0 },
    { KEY_RIGHT_SHIFT, GAMEPAD_BUTTON_MIDDLE_LEFT, 0, 0 }, { KEY_ENTER, GAMEPAD_BUTTON_MIDDLE_RIGHT, 0, 0 },
    { KEY_Z, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0, 0 }, { KEY_X, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, 0, 0 },
};
/* The keys of the smw port's defaults (embed.h): X = A, Z = B, S = X, A = Y, C = L, V = R. */
static const PadBind PADMAP_SNES_DEF[SNES_BTN_COUNT] = {
    { KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP, 0, 0 }, { KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0, 0 },
    { KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT, 0, 0 }, { KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, 0, 0 },
    { KEY_RIGHT_SHIFT, GAMEPAD_BUTTON_MIDDLE_LEFT, 0, 0 }, { KEY_ENTER, GAMEPAD_BUTTON_MIDDLE_RIGHT, 0, 0 },
    { KEY_A, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, 0, 0 }, { KEY_Z, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0, 0 },
    { KEY_S, GAMEPAD_BUTTON_RIGHT_FACE_UP, 0, 0 }, { KEY_X, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, 0, 0 },
    { KEY_C, GAMEPAD_BUTTON_LEFT_TRIGGER_1, 0, 0 }, { KEY_V, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, 0, 0 },
    { 0, GAMEPAD_BUTTON_LEFT_TRIGGER_2, 0, 0 }, { 0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2, 0, 0 },
};
/* N64 defaults = what the embedded games read without a map (embed.h): the arrows are the stick,
 * the pad's D-pad the D-pad, I / K / J / L the C buttons (the analog sticks and the C buttons of a
 * Switch-mode N64 pad are read on top of these, they are no bindings). */
static const PadBind PADMAP_N64_DEF[N64_BTN_COUNT] = {
    { 0, GAMEPAD_BUTTON_LEFT_FACE_UP, 0, 0 }, { 0, GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0, 0 },
    { 0, GAMEPAD_BUTTON_LEFT_FACE_LEFT, 0, 0 }, { 0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, 0, 0 },
    { KEY_UP, -1, 0, 0 }, { KEY_DOWN, -1, 0, 0 }, { KEY_LEFT, -1, 0, 0 }, { KEY_RIGHT, -1, 0, 0 },
    { KEY_ENTER, GAMEPAD_BUTTON_MIDDLE_RIGHT, 0, 0 },
    { KEY_X, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, 0, 0 }, { KEY_Z, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0, 0 },
    { KEY_LEFT_SHIFT, GAMEPAD_BUTTON_LEFT_TRIGGER_2, 0, 0 },
    { KEY_C, GAMEPAD_BUTTON_LEFT_TRIGGER_1, 0, 0 }, { KEY_V, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, 0, 0 },
    { KEY_I, -1, 0, 0 }, { KEY_K, -1, 0, 0 }, { KEY_J, -1, 0, 0 }, { KEY_L, -1, 0, 0 },
};
static const PadSys PADSYS_NES = { "nes", NES_BTN_COUNT, NES_BTN_COUNT, PADMAP_NES_NAMES, PADMAP_NES_DEF, NES_BTN_A, NES_BTN_B, -1 };
static const PadSys PADSYS_SNES = { "snes", SNES_BTN_COUNT, SNES_BTN_R + 1, PADMAP_SNES_NAMES, PADMAP_SNES_DEF, SNES_BTN_A, SNES_BTN_B, SNES_BTN_ZR };

static const PadSys PADSYS_N64 = { "n64", N64_BTN_COUNT, N64_BTN_COUNT, PADMAP_N64_NAMES, PADMAP_N64_DEF, N64_BTN_A, N64_BTN_B, -1 };

static const PadBind PADMAP_GC_DEF[GC_BTN_COUNT] = {
    { 0, GAMEPAD_BUTTON_LEFT_FACE_UP, 0, 0 }, { 0, GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0, 0 },
    { 0, GAMEPAD_BUTTON_LEFT_FACE_LEFT, 0, 0 }, { 0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, 0, 0 },
    { KEY_UP, -1, 0, 0 }, { KEY_DOWN, -1, 0, 0 }, { KEY_LEFT, -1, 0, 0 }, { KEY_RIGHT, -1, 0, 0 },
    { KEY_ENTER, GAMEPAD_BUTTON_MIDDLE_RIGHT, 0, 0 },
    { KEY_X, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, 0, 0 }, { KEY_Z, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0, 0 },
    { KEY_S, GAMEPAD_BUTTON_RIGHT_FACE_UP, 0, 0 }, { KEY_A, GAMEPAD_BUTTON_RIGHT_FACE_LEFT, 0, 0 },
    { KEY_Q, -1, 0, 0 },
    { KEY_C, GAMEPAD_BUTTON_LEFT_TRIGGER_1, 0, 0 }, { KEY_V, GAMEPAD_BUTTON_RIGHT_TRIGGER_1, 0, 0 },
    { 0, GAMEPAD_BUTTON_LEFT_TRIGGER_2, 0, 0 }, { 0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2, 0, 0 },
    { KEY_I, -1, 0, 0 }, { KEY_K, -1, 0, 0 }, { KEY_J, -1, 0, 0 }, { KEY_L, -1, 0, 0 },
};
static const PadSys PADSYS_GC = { "gamecube", GC_BTN_COUNT, GC_BTN_COUNT, PADMAP_GC_NAMES, PADMAP_GC_DEF, GC_BTN_A, GC_BTN_B, -1 };

static inline void PadMap_Defaults(const PadSys *sys, PadSysMap *m)
{
    memset(m, 0, sizeof(*m));
    memcpy(m->b, sys->def, sizeof(PadBind) * (size_t)sys->count);
}

static inline void PadMap_NesDefaults(NesPadMap *m) { PadMap_Defaults(&PADSYS_NES, m); }

/* "b3" / "a2+" / "a2-" / "none". */
static inline void PadMap_JsText(const PadBind *b, char *out, size_t n)
{
    if (b->jsKind == PAD_JS_BUTTON) snprintf(out, n, "b%d", b->jsIdx);
    else if (b->jsKind == PAD_JS_AXIS_NEG || b->jsKind == PAD_JS_AXIS_POS)
        snprintf(out, n, "a%d%c", b->jsIdx, b->jsKind == PAD_JS_AXIS_POS ? '+' : '-');
    else snprintf(out, n, "none");
}

static inline bool PadMap_JsParse(const char *s, int *kind, int *idx)
{
    char *end;
    long v;
    *kind = PAD_JS_NONE;
    *idx = 0;
    if (!s || !strcmp(s, "none") || !*s) return true;
    if (s[0] != 'b' && s[0] != 'a') return false;
    v = strtol(s + 1, &end, 10);
    if (end == s + 1 || v < 0 || v > 255) return false;
    if (s[0] == 'b') { if (*end) return false; *kind = PAD_JS_BUTTON; }
    else {
        if ((*end != '+' && *end != '-') || end[1]) return false;
        *kind = *end == '+' ? PAD_JS_AXIS_POS : PAD_JS_AXIS_NEG;
    }
    *idx = (int)v;
    return true;
}

/* Config text -> mapping; a missing or unreadable key keeps its default. */
static inline void PadMap_Load(const PadSys *sys, PadSysMap *m, const char *text)
{
    char key[64], v[64];
    PadMap_Defaults(sys, m);
    for (int i = 0; text && i < sys->count; i++) {
        PadBind *b = &m->b[i];
        snprintf(key, sizeof(key), "%s.map.%s.key", sys->id, sys->names[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int k = PadMap_KeyByName(v);
            m->custom = true;
            if (!v[0] || !strcmp(v, "none")) b->key = 0;
            else if (k) b->key = k;
        }
        snprintf(key, sizeof(key), "%s.map.%s.pad", sys->id, sys->names[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int p = PadMap_PadByName(v);
            m->custom = true;
            if (!v[0] || !strcmp(v, "none")) b->pad = -1;
            else if (p >= 0) b->pad = p;
        }
        snprintf(key, sizeof(key), "%s.map.%s.js", sys->id, sys->names[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int kind, idx;
            m->custom = true;
            if (PadMap_JsParse(v, &kind, &idx)) { b->jsKind = kind; b->jsIdx = idx; }
        }
    }
    snprintf(key, sizeof(key), "%s.map.joydev", sys->id);
    if (text && Cfg_Get(text, key, v, sizeof(v)) && strlen(v) == 32) memcpy(m->joyId, v, 33);
}

static inline void PadMap_NesLoad(NesPadMap *m, const char *text) { PadMap_Load(&PADSYS_NES, m, text); }

static inline bool PadMap_Save(const PadSys *sys, const PadSysMap *m, const char *cfgPath)
{
    char key[64], v[64];
    bool ok = true;
    for (int i = 0; i < sys->count; i++) {
        const PadBind *b = &m->b[i];
        const PadKeyInfo *ki = PadMap_KeyInfo(b->key);
        const PadBtnInfo *pi = PadMap_PadInfo(b->pad);
        snprintf(key, sizeof(key), "%s.map.%s.key", sys->id, sys->names[i]);
        ok &= Cfg_SaveKey(cfgPath, key, ki ? ki->name : "none");
        snprintf(key, sizeof(key), "%s.map.%s.pad", sys->id, sys->names[i]);
        ok &= Cfg_SaveKey(cfgPath, key, pi ? pi->sdl : "none");
        snprintf(key, sizeof(key), "%s.map.%s.js", sys->id, sys->names[i]);
        PadMap_JsText(b, v, sizeof(v));
        ok &= Cfg_SaveKey(cfgPath, key, v);
    }
    snprintf(key, sizeof(key), "%s.map.joydev", sys->id);
    ok &= Cfg_SaveKey(cfgPath, key, m->joyId);
    return ok;
}

static inline bool PadMap_NesSave(const NesPadMap *m, const char *cfgPath) { return PadMap_Save(&PADSYS_NES, m, cfgPath); }

/* Back to the default: every <id>.map.* key leaves the config. */
static inline bool PadMap_Clear(const PadSys *sys, const char *cfgPath)
{
    static char a[16384], b[16384];
    char key[64];
    size_t n = 0;
    FILE *f = fopen(cfgPath, "r");
    bool ok = true;
    if (!f) return true;
    n = fread(a, 1, sizeof(a) - 1, f);
    fclose(f);
    a[n] = '\0';
    for (int i = 0; i < sys->count * 3 + 1; i++) {
        if (i == sys->count * 3) snprintf(key, sizeof(key), "%s.map.joydev", sys->id);
        else snprintf(key, sizeof(key), "%s.map.%s.%s", sys->id, sys->names[i / 3], i % 3 == 0 ? "key" : i % 3 == 1 ? "pad" : "js");
        if (!Cfg_Remove(a, key, b, sizeof(b))) return false;
        memcpy(a, b, strlen(b) + 1);
    }
    f = fopen(cfgPath, "w");
    if (!f) return false;
    ok = fputs(a, f) >= 0;
    return (fclose(f) == 0) && ok;
}

static inline bool PadMap_NesClear(const char *cfgPath) { return PadMap_Clear(&PADSYS_NES, cfgPath); }

/* ---- Mednafen ---- */

/* The value of one nes.input.port1.gamepad.<button> setting:
 * "keyboard 0x0 <scancode> || joystick 0x<id> button_<n>" (abs_<n>+ / abs_<n>- for
 * an axis or hat); "" when nothing is bound. */
static inline void PadMap_MednafenValue(const PadBind *b, const char *joyId, char *out, size_t n)
{
    const PadKeyInfo *ki = PadMap_KeyInfo(b->key);
    size_t used = 0;
    out[0] = '\0';
    if (ki) used = (size_t)snprintf(out, n, "keyboard 0x0 %d", ki->scan);
    if (b->jsKind != PAD_JS_NONE && joyId && strlen(joyId) == 32 && used < n) {
        char ctl[24];
        if (b->jsKind == PAD_JS_BUTTON) snprintf(ctl, sizeof(ctl), "button_%d", b->jsIdx);
        else snprintf(ctl, sizeof(ctl), "abs_%d%c", b->jsIdx, b->jsKind == PAD_JS_AXIS_POS ? '+' : '-');
        snprintf(out + used, n - used, "%sjoystick 0x%s %s", used ? " || " : "", joyId, ctl);
    }
}

/* Command-line words for the custom mapping (16 words: option, value, ...) into buf;
 * 0 when the mapping is the default or buf is too small. */
static inline int PadMap_MednafenWordsSys(const PadSys *sys, const PadSysMap *m, char *buf, size_t cap, char **words, int max)
{
    size_t used = 0;
    int n = 0;
    if (!m->custom || max < sys->gameCount * 2) return 0;
    for (int i = 0; i < sys->gameCount; i++) {
        char opt[64], val[160];
        snprintf(opt, sizeof(opt), "-%s.input.port1.gamepad.%s", sys->id, sys->names[i]);
        PadMap_MednafenValue(&m->b[i], m->joyId, val, sizeof(val));
        if (used + strlen(opt) + strlen(val) + 2 > cap) return 0;
        words[n++] = buf + used;
        memcpy(buf + used, opt, strlen(opt) + 1);
        used += strlen(opt) + 1;
        words[n++] = buf + used;
        memcpy(buf + used, val, strlen(val) + 1);
        used += strlen(val) + 1;
    }
    return n;
}

static inline int PadMap_MednafenWords(const NesPadMap *m, char *buf, size_t cap, char **words, int max)
{
    return PadMap_MednafenWordsSys(&PADSYS_NES, m, buf, cap, words, max);
}

/* ---- NESRecomp ports: config.ini [Keyboard.Player1] / [Gamepad.Player1] ---- */

static inline bool PadMap_IniIsSection(const char *line, size_t len, const char *name)
{
    size_t nl = strlen(name);
    if (len < nl + 2 || line[0] != '[') return false;
    for (size_t i = 0; i < nl; i++) if (tolower((unsigned char)line[1 + i]) != tolower((unsigned char)name[i])) return false;
    return line[1 + nl] == ']';
}

static inline bool PadMap_IniAdd(char *out, size_t n, size_t *used, const char *fmt, const char *a, const char *b)
{
    int w = snprintf(out + *used, n - *used, fmt, a, b);
    if (w < 0 || (size_t)w >= n - *used) return false;
    *used += (size_t)w;
    return true;
}

static inline const char *PadMap_IniValue(const PadSysMap *m, int sec, int btn, char *tmp, size_t n)
{
    if (sec == 0) {
        const PadKeyInfo *ki = PadMap_KeyInfo(m->b[btn].key);
        snprintf(tmp, n, "%s", ki ? ki->name : "none");
    } else {
        const PadBtnInfo *pi = PadMap_PadInfo(m->b[btn].pad);
        snprintf(tmp, n, "%s", pi ? pi->sdl : "none");
    }
    return tmp;
}

/* config.ini text with player 1's eight keyboard and gamepad bindings set from m; every
 * other line stays. A missing section or key is added. false if out is too small. */
static inline bool PadMap_IniMergeSys(const PadSys *sys, const char *const *SEC, int nsec, const char *in, const PadSysMap *m, char *out, size_t n)
{
    bool done[2][PAD_MAX_BTN] = { { false } }, seen[2] = { false, false };
    size_t used = 0;
    int sec = -1;
    const char *p = in ? in : "";
    char tmp[64];
    if (!n) return false;
    out[0] = '\0';
#define PADMAP_FLUSH(s) do { if ((s) >= 0) for (int i_ = 0; i_ < sys->gameCount; i_++) if (!done[s][i_]) { \
        done[s][i_] = true; \
        if (!PadMap_IniAdd(out, n, &used, "%s = %s\n", sys->names[i_], PadMap_IniValue(m, s, i_, tmp, sizeof(tmp)))) return false; } } while (0)
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        size_t tl = len;
        while (tl && (p[tl - 1] == '\r' || p[tl - 1] == ' ' || p[tl - 1] == '\t')) tl--;
        if (tl && p[0] == '[') {
            int ns = -1;
            PADMAP_FLUSH(sec);
            for (int s = 0; s < nsec; s++) if (PadMap_IniIsSection(p, tl, SEC[s])) { ns = s; seen[s] = true; }
            sec = ns;
        } else if (sec >= 0) {
            const char *eq = memchr(p, '=', len);
            if (eq) {
                const char *ks = p, *ke = eq;
                int hit = -1;
                while (ks < ke && (*ks == ' ' || *ks == '\t')) ks++;
                while (ke > ks && (ke[-1] == ' ' || ke[-1] == '\t')) ke--;
                for (int i = 0; i < sys->gameCount; i++) {
                    const char *nm = sys->names[i], *kc = ks;
                    while (*nm && kc < ke && tolower((unsigned char)*nm) == tolower((unsigned char)*kc)) { nm++; kc++; }
                    if (!*nm && kc == ke) hit = i;
                }
                if (hit >= 0) {
                    if (!done[sec][hit]) {
                        done[sec][hit] = true;
                        if (!PadMap_IniAdd(out, n, &used, "%s = %s\n", sys->names[hit], PadMap_IniValue(m, sec, hit, tmp, sizeof(tmp)))) return false;
                    }
                    p = nl ? nl + 1 : p + len;
                    continue;
                }
            }
        }
        {
            int w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
            if (w < 0 || (size_t)w >= n - used) return false;
            used += (size_t)w;
        }
        p = nl ? nl + 1 : p + len;
    }
    PADMAP_FLUSH(sec);
    for (int s = 0; s < nsec; s++) {
        if (seen[s]) continue;
        if (!PadMap_IniAdd(out, n, &used, "[%s]\n%s", SEC[s], "")) return false;
        PADMAP_FLUSH(s);
    }
#undef PADMAP_FLUSH
    return true;
}

static inline bool PadMap_IniMerge(const char *in, const NesPadMap *m, char *out, size_t n)
{
    static const char *const SEC[2] = { "Keyboard.Player1", "Gamepad.Player1" };
    return PadMap_IniMergeSys(&PADSYS_NES, SEC, 2, in, m, out, n);
}

/* ---- snesrecomp ports: keybinds.ini [player1] (SDL key names, no pad) ---- */

static inline bool PadMap_SnesKeybinds(const char *in, const PadSysMap *m, char *out, size_t n)
{
    static const char *const SEC[1] = { "player1" };
    return PadMap_IniMergeSys(&PADSYS_SNES, SEC, 1, in, m, out, n);
}

/* ---- zelda3 / sm: zelda3.ini, sm.ini ---- */

/* The port's name for a standard pad button (its GamepadMap list), "" for none. */
static inline const char *PadMap_ZeldaPadName(int pad)
{
    static const struct { const char *sdl, *z; } T[] = {
        { "dpup", "DpadUp" }, { "dpdown", "DpadDown" }, { "dpleft", "DpadLeft" }, { "dpright", "DpadRight" },
        { "a", "A" }, { "b", "B" }, { "x", "X" }, { "y", "Y" }, { "leftshoulder", "Lb" }, { "rightshoulder", "Rb" },
        { "lefttrigger+", "L2" }, { "righttrigger+", "R2" }, { "back", "Back" }, { "start", "Start" },
        { "leftstick", "L3" }, { "rightstick", "R3" },
    };
    const PadBtnInfo *pi = PadMap_PadInfo(pad);
    for (size_t i = 0; pi && i < sizeof(T) / sizeof(T[0]); i++) if (!strcmp(T[i].sdl, pi->sdl)) return T[i].z;
    return "";
}

static inline bool PadMap_HasWord(const char *p, const char *word)
{
    while (*word) { if (tolower((unsigned char)*p) != tolower((unsigned char)*word)) return false; p++; word++; }
    return true;
}

/* The line "Controls = Up, Down, Left, Right, Select, Start, A, B, X, Y, L, R" of one list
 * (keys when pad is false); a letter is written lower case as the port's own defaults are. */
static inline void PadMap_ZeldaList(const PadSysMap *m, bool pad, char *out, size_t n)
{
    static const int order[12] = { SNES_BTN_UP, SNES_BTN_DOWN, SNES_BTN_LEFT, SNES_BTN_RIGHT, SNES_BTN_SELECT,
                                   SNES_BTN_START, SNES_BTN_A, SNES_BTN_B, SNES_BTN_X, SNES_BTN_Y, SNES_BTN_L, SNES_BTN_R };
    size_t used = 0;
    out[0] = '\0';
    for (int i = 0; i < 12; i++) {
        const PadBind *b = &m->b[order[i]];
        char name[40] = "";
        int w;
        if (pad) snprintf(name, sizeof(name), "%s", PadMap_ZeldaPadName(b->pad));
        else {
            const PadKeyInfo *ki = PadMap_KeyInfo(b->key);
            if (ki) snprintf(name, sizeof(name), "%s", ki->name);
            if (name[0] >= 'A' && name[0] <= 'Z' && !name[1]) name[0] = (char)(name[0] - 'A' + 'a');
        }
        w = snprintf(out + used, used < n ? n - used : 0, "%s%s", i ? ", " : "", name);
        if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; return; }
        used += (size_t)w;
    }
}

/* zelda3.ini / sm.ini text with its [KeyMap] and [GamepadMap] "Controls =" lines set from m
 * (the commented examples stay); a missing line or section is added. false if out is too small. */
static inline bool PadMap_ZeldaMerge(const char *in, const PadSysMap *m, char *out, size_t n)
{
    static const char *const SEC[2] = { "KeyMap", "GamepadMap" };
    bool done[2] = { false, false }, seen[2] = { false, false };
    char list[2][256];
    size_t used = 0;
    int sec = -1;
    const char *p = in ? in : "";
    if (!n) return false;
    out[0] = '\0';
    PadMap_ZeldaList(m, false, list[0], sizeof(list[0]));
    PadMap_ZeldaList(m, true, list[1], sizeof(list[1]));
#define ZFLUSH(s) do { if ((s) >= 0 && !done[s]) { int w_ = snprintf(out + used, n - used, "Controls = %s\n", list[s]); \
        if (w_ < 0 || (size_t)w_ >= n - used) { return false; } \
        used += (size_t)w_; done[s] = true; } } while (0)
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        size_t tl = len;
        bool replaced = false;
        while (tl && (p[tl - 1] == '\r' || p[tl - 1] == ' ' || p[tl - 1] == '\t')) tl--;
        if (tl && p[0] == '[') {
            int ns = -1;
            ZFLUSH(sec);
            for (int s = 0; s < 2; s++) if (PadMap_IniIsSection(p, tl, SEC[s])) { ns = s; seen[s] = true; }
            sec = ns;
        } else if (sec >= 0 && !done[sec] && tl >= 8 && PadMap_HasWord(p, "Controls")) {
            const char *q = p + 8;
            while (q < p + len && (*q == ' ' || *q == '\t')) q++;
            if (q < p + len && *q == '=') {
                int w = snprintf(out + used, n - used, "Controls = %s\n", list[sec]);
                if (w < 0 || (size_t)w >= n - used) return false;
                used += (size_t)w;
                done[sec] = true;
                replaced = true;
            }
        }
        if (!replaced) {
            int w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
            if (w < 0 || (size_t)w >= n - used) return false;
            used += (size_t)w;
        }
        p = nl ? nl + 1 : p + len;
    }
    ZFLUSH(sec);
    for (int s = 0; s < 2; s++) {
        int w;
        if (seen[s]) continue;
        w = snprintf(out + used, n - used, "[%s]\nControls = %s\n", SEC[s], list[s]);
        if (w < 0 || (size_t)w >= n - used) return false;
        used += (size_t)w;
    }
#undef ZFLUSH
    return true;
}

#endif /* NSO_PADMAP_H */
