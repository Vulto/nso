/* nso.h - STB-style shared types and platform helpers */
#ifndef NSO_H
#define NSO_H

#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

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
    const char *embedRomKey; /* NULL = spawn binary; else dir/binary is an in-process
                                module (embed.h), its game data path (ROM file
                                or port dir) from this config key */
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

#define NSO_PAD_DEADZONE 0.5f

/* 1 = crossed +deadzone this frame, -1 = crossed -deadzone, 0 = none. */
static inline int Nso_PadAxisEdge(float prev, float now)
{
    if (now > NSO_PAD_DEADZONE && prev <= NSO_PAD_DEADZONE) {
        return 1;
    }
    if (now < -NSO_PAD_DEADZONE && prev >= -NSO_PAD_DEADZONE) {
        return -1;
    }
    return 0;
}

/* After a connect, ignore edges until the axis has sat inside the deadzone.
 * First samples are often 0 on a 0..255 stick (GLFW -1.0), which would look
 * like a C-left/left press if prev starts at 0. */
#define NSO_PAD_SETTLE_SEC 0.30f

typedef struct {
    float prev;
    bool armed;
} NsoPadAxisArm;

static inline void Nso_PadAxisArmReset(NsoPadAxisArm *a)
{
    if (!a) {
        return;
    }
    a->prev = 0.0f;
    a->armed = false;
}

static inline int Nso_PadAxisEdgeArmed(NsoPadAxisArm *a, float now)
{
    int e;
    if (!a) {
        return 0;
    }
    if (!a->armed) {
        if (now <= NSO_PAD_DEADZONE && now >= -NSO_PAD_DEADZONE) {
            a->armed = true;
        }
        a->prev = now;
        return 0;
    }
    e = Nso_PadAxisEdge(a->prev, now);
    a->prev = now;
    return e;
}

/* Per-controller hint profile. Only N64 is implemented; more can be added. */
typedef enum {
    NSO_PAD_PROFILE_DEFAULT = 0,
    NSO_PAD_PROFILE_N64,
} NsoPadProfile;

static inline bool Nso_PadNameIsN64(const char *name)
{
    return name && (strstr(name, "N64") || strstr(name, "8BitDo 64"));
}

static inline NsoPadProfile Nso_PadProfileFromName(const char *name)
{
    if (Nso_PadNameIsN64(name)) {
        return NSO_PAD_PROFILE_N64;
    }
    return NSO_PAD_PROFILE_DEFAULT;
}

/* NSO_PAD_STYLE=n64 or =default overrides auto-detection (tests). */
static inline bool Nso_PadStyleEnvOverride(NsoPadProfile *out)
{
    const char *f = getenv("NSO_PAD_STYLE");
    if (!f || !f[0] || !out) {
        return false;
    }
    if (strcmp(f, "n64") == 0) {
        *out = NSO_PAD_PROFILE_N64;
        return true;
    }
    if (strcmp(f, "default") == 0) {
        *out = NSO_PAD_PROFILE_DEFAULT;
        return true;
    }
    return false;
}

/* Live profile: N64 if any connected pad's name matches, unless NSO_PAD_STYLE overrides. */
#define NSO_PAD_MAX 4

static inline NsoPadProfile Nso_PadProfileFromConnected(void)
{
    NsoPadProfile env;
    if (Nso_PadStyleEnvOverride(&env)) {
        return env;
    }
    for (int i = 0; i < NSO_PAD_MAX; i++) {
        if (IsGamepadAvailable(i) && Nso_PadNameIsN64(GetGamepadName(i))) {
            return NSO_PAD_PROFILE_N64;
        }
    }
    return NSO_PAD_PROFILE_DEFAULT;
}

/* Tracks a live profile change so the hint bar can close, then reopen.
 * fade is 1 when the current labels are fully shown, 0 at the mid-switch.
 * notice is set on a transition to N64 (not on the first snap); the UI clears it. */
typedef struct {
    NsoPadProfile live;
    NsoPadProfile drawn;
    float fade;
    bool inited;
    bool closing;
    bool notice;
} NsoPadWatch;

static inline void Nso_PadWatchUpdate(NsoPadWatch *w, NsoPadProfile now,
                                      float dt, float fadeSec)
{
    float step;
    if (!w) {
        return;
    }
    if (!w->inited) {
        w->live = now;
        w->drawn = now;
        w->fade = 1.0f;
        w->closing = false;
        w->notice = false;
        w->inited = true;
        return;
    }
    if (now != w->live) {
        if (now == NSO_PAD_PROFILE_N64) {
            w->notice = true;
        }
        w->live = now;
        if (w->fade > 0.001f) {
            w->closing = true;
        } else {
            w->drawn = now;
            w->closing = false;
        }
    }
    step = (fadeSec > 0.0f) ? (dt / fadeSec) : 1.0f;
    if (w->closing) {
        w->fade -= step;
        if (w->fade <= 0.0f) {
            w->fade = 0.0f;
            w->closing = false;
            w->drawn = w->live;
        }
    } else if (w->drawn == w->live && w->fade < 1.0f) {
        w->fade += step;
        if (w->fade > 1.0f) {
            w->fade = 1.0f;
        }
    }
}

/* Player slot = raylib gamepad index + 1 (clamped 1..4). LEDs 1..N are on. */
static inline int Nso_PadPlayerFromIndex(int gamepadIndex)
{
    if (gamepadIndex < 0) {
        return 0;
    }
    int p = gamepadIndex + 1;
    return p > 4 ? 4 : p;
}

static inline bool Nso_PadPlayerLedOn(int player, int led /* 1..4 */)
{
    return player > 0 && led >= 1 && led <= 4 && led <= player;
}

typedef enum {
    NSO_PAD_LINK_NONE = 0,
    NSO_PAD_LINK_USB,
    NSO_PAD_LINK_BT,
} NsoPadLink;

/* Hint-bar profile from the evdev link of the N64 pad. USB keeps the default
 * bar; only Bluetooth switches to N64 labels. */
static inline NsoPadProfile Nso_PadProfileFromLink(NsoPadLink link)
{
    if (link == NSO_PAD_LINK_BT) {
        return NSO_PAD_PROFILE_N64;
    }
    return NSO_PAD_PROFILE_DEFAULT;
}

/* True if s contains mac (colons, hyphens or no separators, any case). */
static inline bool Nso_PadTextHasMac(const char *s, const char *mac)
{
    char a[32], b[32];
    size_t i, na = 0, nb = 0;
    if (!s || !mac || !mac[0]) {
        return false;
    }
    for (i = 0; mac[i] && na + 1 < sizeof a; i++) {
        if (mac[i] != ':' && mac[i] != '-') {
            a[na++] = (char)tolower((unsigned char)mac[i]);
        }
    }
    a[na] = 0;
    for (i = 0; s[i] && nb + 1 < sizeof b; i++) {
        if (s[i] != ':' && s[i] != '-') {
            b[nb++] = (char)tolower((unsigned char)s[i]);
        }
    }
    b[nb] = 0;
    return na && strstr(b, a) != NULL;
}

/* Classify an 8BitDo N64 entry in a /proc/bus/input/devices dump.
 * If mac is set, a device with a different Uniq is skipped; an empty Uniq is kept. */
static inline NsoPadLink Nso_PadLinkFromProc(const char *text, const char *mac)
{
    const char *p = text;
    while (p && *p) {
        const char *end = strstr(p, "\n\n");
        size_t n = end ? (size_t)(end - p) : strlen(p);
        char name[128] = { 0 }, sysfs[256] = { 0 }, phys[128] = { 0 }, uniq[64] = { 0 };
        int bus = -1;
        const char *line = p, *stop = p + n;
        while (line < stop) {
            const char *nl = memchr(line, 10, (size_t)(stop - line));
            size_t ln = nl ? (size_t)(nl - line) : (size_t)(stop - line);
            if (ln >= 8 && strncmp(line, "N: Name=", 8) == 0) {
                const char *s = line + 8, *e;
                if (*s == '"') {
                    s++;
                }
                e = s;
                while (e < line + ln && *e && *e != '"') {
                    e++;
                }
                if ((size_t)(e - s) >= sizeof name) {
                    e = s + sizeof name - 1;
                }
                memcpy(name, s, (size_t)(e - s));
                name[e - s] = 0;
            } else if (ln >= 9 && strncmp(line, "S: Sysfs=", 9) == 0) {
                size_t L = ln - 9;
                if (L >= sizeof sysfs) L = sizeof sysfs - 1;
                memcpy(sysfs, line + 9, L);
                sysfs[L] = 0;
            } else if (ln >= 8 && strncmp(line, "P: Phys=", 8) == 0) {
                size_t L = ln - 8;
                if (L >= sizeof phys) L = sizeof phys - 1;
                memcpy(phys, line + 8, L);
                phys[L] = 0;
            } else if (ln >= 8 && strncmp(line, "U: Uniq=", 8) == 0) {
                size_t L = ln - 8;
                if (L >= sizeof uniq) L = sizeof uniq - 1;
                memcpy(uniq, line + 8, L);
                uniq[L] = 0;
            } else if (ln >= 7 && strncmp(line, "I: Bus=", 7) == 0) {
                bus = (int)strtol(line + 7, NULL, 16);
            }
            line = nl ? nl + 1 : stop;
        }
        if (Nso_PadNameIsN64(name)
            && !(mac && mac[0] && uniq[0] && !Nso_PadTextHasMac(uniq, mac))) {
            if (bus == 3 || strstr(sysfs, "/usb") || strncmp(phys, "usb-", 4) == 0) {
                return NSO_PAD_LINK_USB;
            }
            if (bus == 5 || strstr(sysfs, "uhid") || strchr(uniq, ':')) {
                return NSO_PAD_LINK_BT;
            }
        }
        p = end ? end + 2 : NULL;
    }
    return NSO_PAD_LINK_NONE;
}

/* Capacity 0..100, or -1. Ignores system packs (BAT*, AC, ucsi*). Matches hid-* or mac. */
static inline int Nso_PadBatteryParseCapacity(const char *text)
{
    char *end = NULL;
    long v;
    if (!text || !text[0]) {
        return -1;
    }
    v = strtol(text, &end, 10);
    if (end == text || v < 0 || v > 100) {
        return -1;
    }
    return (int)v;
}

static inline int Nso_PadBatteryRead(const char *root, const char *mac)
{
    DIR *dir;
    struct dirent *de;
    int best = -1;
    if (!root || !root[0]) {
        root = "/sys/class/power_supply";
    }
    dir = opendir(root);
    if (!dir) {
        return -1;
    }
    while ((de = readdir(dir)) != NULL) {
        char path[512], buf[1024];
        FILE *f;
        size_t n;
        int cap, hidish, mac_hit;
        if (de->d_name[0] == '.') {
            continue;
        }
        if (strncmp(de->d_name, "BAT", 3) == 0 || strcmp(de->d_name, "AC") == 0
            || strncmp(de->d_name, "ADP", 3) == 0 || strncmp(de->d_name, "ucsi", 4) == 0) {
            continue;
        }
        snprintf(path, sizeof path, "%s/%s/uevent", root, de->d_name);
        buf[0] = 0;
        f = fopen(path, "r");
        if (f) {
            n = fread(buf, 1, sizeof buf - 1, f);
            buf[n] = 0;
            fclose(f);
        }
        hidish = strstr(de->d_name, "hid") || strstr(de->d_name, "HID")
            || strstr(buf, "hid-") || strstr(buf, "HID_UNIQ")
            || strstr(buf, "HID_PHYS");
        mac_hit = Nso_PadTextHasMac(de->d_name, mac) || Nso_PadTextHasMac(buf, mac);
        if (mac && mac[0] && !mac_hit) {
            continue;
        }
        if (!hidish && !mac_hit) {
            continue;
        }
        snprintf(path, sizeof path, "%s/%s/capacity", root, de->d_name);
        f = fopen(path, "r");
        if (!f) {
            continue;
        }
        n = fread(buf, 1, sizeof buf - 1, f);
        buf[n] = 0;
        fclose(f);
        cap = Nso_PadBatteryParseCapacity(buf);
        if (cap >= 0) {
            best = cap;
            if (mac_hit) {
                break;
            }
        }
    }
    closedir(dir);
    return best;
}

/* N64 hints use the same font as the stock bar; only X/Y labels change.
 * 🅧 (Details / delete) -> "C-Up", 🅨 (Move / copy) -> "C-Down".
 * Arrow glyphs are not in the UI font atlas, so "C-Up" / "C-Down" are used
 * rather than "C↑". A and B keep 🅐 / 🅑. Default profile returns text as-is. */
static inline const char *Nso_HintTextForProfile(NsoPadProfile profile,
                                                 const char *text,
                                                 char *buf,
                                                 size_t bufsz)
{
    if (!text || profile != NSO_PAD_PROFILE_N64 || !buf || bufsz < 2) {
        return text;
    }
    size_t o = 0;
    for (const unsigned char *p = (const unsigned char *)text; *p; ) {
        const char *rep = NULL;
        size_t skip = 1;
        if (p[0] == 0xF0 && p[1] == 0x9F && p[2] == 0x85 && p[3] == 0xA7) {
            rep = "C-Up";
            skip = 4;
        } else if (p[0] == 0xF0 && p[1] == 0x9F && p[2] == 0x85 && p[3] == 0xA8) {
            rep = "C-Down";
            skip = 4;
        }
        if (rep) {
            size_t rl = strlen(rep);
            if (o + rl >= bufsz) {
                break;
            }
            memcpy(buf + o, rep, rl);
            o += rl;
            p += skip;
        } else {
            if (o + 1 >= bufsz) {
                break;
            }
            buf[o++] = (char)*p++;
        }
    }
    buf[o] = '\0';
    return buf;
}

/* 8BitDo N64 Mod Kit (Bluetooth 05..., USB 03...). SDL's gamecontrollerdb
 * entry with a/b swapped so N64 A (b0) lands on NSO_BTN_A and N64 B (b1) on
 * NSO_BTN_B; C buttons stay the right stick (SM64), Z = left trigger,
 * star/home = guide (pause menu). Passed to SetGamepadMappings after InitWindow. */
#define NSO_PAD_MAPPINGS \
    "05000000c82d00006928000000010000,8BitDo N64,a:b1,b:b0,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1," \
    "guide:b12,leftshoulder:b6,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,righttrigger:b9," \
    "rightx:a2,righty:a3,start:b11,platform:Linux,\n" \
    "03000000c82d00006928000011010000,8BitDo N64,a:b1,b:b0,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1," \
    "guide:b12,leftshoulder:b6,lefttrigger:b8,leftx:a0,lefty:a1,rightshoulder:b7,righttrigger:b9," \
    "rightx:a2,righty:a3,start:b11,platform:Linux,\n"

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
