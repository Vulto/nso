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
enum { PAD_JS_NONE = 0, PAD_JS_BUTTON = 1, PAD_JS_AXIS_NEG = 2, PAD_JS_AXIS_POS = 3 };

typedef struct {
    int key;       /* raylib KEY_*, 0 = none */
    int pad;       /* raylib GAMEPAD_BUTTON_*, -1 = none */
    int jsKind;    /* PAD_JS_* */
    int jsIdx;
} PadBind;

typedef struct {
    PadBind b[NES_BTN_COUNT];
    char joyId[40];      /* 32 hex digits, "" = unknown */
    bool custom;         /* some nes.map.* key is in the config: hand the mapping to the games */
} NesPadMap;

static const char *const PADMAP_NES_NAMES[NES_BTN_COUNT] = { "up", "down", "left", "right", "select", "start", "b", "a" };

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

static inline void PadMap_NesDefaults(NesPadMap *m)
{
    static const PadBind def[NES_BTN_COUNT] = {
        { KEY_UP, GAMEPAD_BUTTON_LEFT_FACE_UP, 0, 0 }, { KEY_DOWN, GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0, 0 },
        { KEY_LEFT, GAMEPAD_BUTTON_LEFT_FACE_LEFT, 0, 0 }, { KEY_RIGHT, GAMEPAD_BUTTON_LEFT_FACE_RIGHT, 0, 0 },
        { KEY_RIGHT_SHIFT, GAMEPAD_BUTTON_MIDDLE_LEFT, 0, 0 }, { KEY_ENTER, GAMEPAD_BUTTON_MIDDLE_RIGHT, 0, 0 },
        { KEY_Z, GAMEPAD_BUTTON_RIGHT_FACE_DOWN, 0, 0 }, { KEY_X, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT, 0, 0 },
    };
    memset(m, 0, sizeof(*m));
    memcpy(m->b, def, sizeof(def));
}

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
static inline void PadMap_NesLoad(NesPadMap *m, const char *text)
{
    char key[64], v[64];
    PadMap_NesDefaults(m);
    for (int i = 0; text && i < NES_BTN_COUNT; i++) {
        PadBind *b = &m->b[i];
        snprintf(key, sizeof(key), "nes.map.%s.key", PADMAP_NES_NAMES[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int k = PadMap_KeyByName(v);
            m->custom = true;
            if (!v[0] || !strcmp(v, "none")) b->key = 0;
            else if (k) b->key = k;
        }
        snprintf(key, sizeof(key), "nes.map.%s.pad", PADMAP_NES_NAMES[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int p = PadMap_PadByName(v);
            m->custom = true;
            if (!v[0] || !strcmp(v, "none")) b->pad = -1;
            else if (p >= 0) b->pad = p;
        }
        snprintf(key, sizeof(key), "nes.map.%s.js", PADMAP_NES_NAMES[i]);
        if (Cfg_Get(text, key, v, sizeof(v))) {
            int kind, idx;
            m->custom = true;
            if (PadMap_JsParse(v, &kind, &idx)) { b->jsKind = kind; b->jsIdx = idx; }
        }
    }
    if (text && Cfg_Get(text, "nes.map.joydev", v, sizeof(v)) && strlen(v) == 32) memcpy(m->joyId, v, 33);
}

static inline bool PadMap_NesSave(const NesPadMap *m, const char *cfgPath)
{
    char key[64], v[64];
    bool ok = true;
    for (int i = 0; i < NES_BTN_COUNT; i++) {
        const PadBind *b = &m->b[i];
        const PadKeyInfo *ki = PadMap_KeyInfo(b->key);
        const PadBtnInfo *pi = PadMap_PadInfo(b->pad);
        snprintf(key, sizeof(key), "nes.map.%s.key", PADMAP_NES_NAMES[i]);
        ok &= Cfg_SaveKey(cfgPath, key, ki ? ki->name : "none");
        snprintf(key, sizeof(key), "nes.map.%s.pad", PADMAP_NES_NAMES[i]);
        ok &= Cfg_SaveKey(cfgPath, key, pi ? pi->sdl : "none");
        snprintf(key, sizeof(key), "nes.map.%s.js", PADMAP_NES_NAMES[i]);
        PadMap_JsText(b, v, sizeof(v));
        ok &= Cfg_SaveKey(cfgPath, key, v);
    }
    ok &= Cfg_SaveKey(cfgPath, "nes.map.joydev", m->joyId);
    return ok;
}

/* Back to the default: every nes.map.* key leaves the config. */
static inline bool PadMap_NesClear(const char *cfgPath)
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
    for (int i = 0; i < NES_BTN_COUNT * 3 + 1; i++) {
        if (i == NES_BTN_COUNT * 3) snprintf(key, sizeof(key), "nes.map.joydev");
        else snprintf(key, sizeof(key), "nes.map.%s.%s", PADMAP_NES_NAMES[i / 3], i % 3 == 0 ? "key" : i % 3 == 1 ? "pad" : "js");
        if (!Cfg_Remove(a, key, b, sizeof(b))) return false;
        memcpy(a, b, strlen(b) + 1);
    }
    f = fopen(cfgPath, "w");
    if (!f) return false;
    ok = fputs(a, f) >= 0;
    return (fclose(f) == 0) && ok;
}

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
static inline int PadMap_MednafenWords(const NesPadMap *m, char *buf, size_t cap, char **words, int max)
{
    size_t used = 0;
    int n = 0;
    if (!m->custom || max < NES_BTN_COUNT * 2) return 0;
    for (int i = 0; i < NES_BTN_COUNT; i++) {
        char opt[64], val[160];
        snprintf(opt, sizeof(opt), "-nes.input.port1.gamepad.%s", PADMAP_NES_NAMES[i]);
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

static inline const char *PadMap_IniValue(const NesPadMap *m, int sec, int btn, char *tmp, size_t n)
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
static inline bool PadMap_IniMerge(const char *in, const NesPadMap *m, char *out, size_t n)
{
    static const char *const SEC[2] = { "Keyboard.Player1", "Gamepad.Player1" };
    bool done[2][NES_BTN_COUNT] = { { false } }, seen[2] = { false, false };
    size_t used = 0;
    int sec = -1;
    const char *p = in ? in : "";
    char tmp[64];
    if (!n) return false;
    out[0] = '\0';
#define PADMAP_FLUSH(s) do { if ((s) >= 0) for (int i_ = 0; i_ < NES_BTN_COUNT; i_++) if (!done[s][i_]) { \
        done[s][i_] = true; \
        if (!PadMap_IniAdd(out, n, &used, "%s = %s\n", PADMAP_NES_NAMES[i_], PadMap_IniValue(m, s, i_, tmp, sizeof(tmp)))) return false; } } while (0)
    while (*p) {
        const char *nl = strchr(p, '\n');
        size_t len = nl ? (size_t)(nl - p) : strlen(p);
        size_t tl = len;
        while (tl && (p[tl - 1] == '\r' || p[tl - 1] == ' ' || p[tl - 1] == '\t')) tl--;
        if (tl && p[0] == '[') {
            int ns = -1;
            PADMAP_FLUSH(sec);
            for (int s = 0; s < 2; s++) if (PadMap_IniIsSection(p, tl, SEC[s])) { ns = s; seen[s] = true; }
            sec = ns;
        } else if (sec >= 0) {
            const char *eq = memchr(p, '=', len);
            if (eq) {
                const char *ks = p, *ke = eq;
                int hit = -1;
                while (ks < ke && (*ks == ' ' || *ks == '\t')) ks++;
                while (ke > ks && (ke[-1] == ' ' || ke[-1] == '\t')) ke--;
                for (int i = 0; i < NES_BTN_COUNT; i++) {
                    const char *nm = PADMAP_NES_NAMES[i], *kc = ks;
                    while (*nm && kc < ke && tolower((unsigned char)*nm) == tolower((unsigned char)*kc)) { nm++; kc++; }
                    if (!*nm && kc == ke) hit = i;
                }
                if (hit >= 0) {
                    if (!done[sec][hit]) {
                        done[sec][hit] = true;
                        if (!PadMap_IniAdd(out, n, &used, "%s = %s\n", PADMAP_NES_NAMES[hit], PadMap_IniValue(m, sec, hit, tmp, sizeof(tmp)))) return false;
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
    for (int s = 0; s < 2; s++) {
        if (seen[s]) continue;
        if (!PadMap_IniAdd(out, n, &used, "[%s]\n%s", SEC[s], "")) return false;
        PADMAP_FLUSH(s);
    }
#undef PADMAP_FLUSH
    return true;
}

#endif /* NSO_PADMAP_H */
