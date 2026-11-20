/* n64pad.h - N64 controller rumble, and the controller's Switch-mode layout.
 *
 * The 8BitDo 64 / N64 Mod Kit has no rumble in D mode (2dc8:2869, plain HID,
 * no output protocol; 8BitDo: "D-input has no vibration"). In S mode it is a
 * Switch Online N64 controller (057e:2019) handled by the kernel's
 * hid-nintendo, whose evdev node has FF_RUMBLE (Bluetooth and USB).
 *  - Rumble: raylib's desktop (GLFW) SetGamepadVibration is a no-op, so an
 *    embedded game's Rumble Pak (nsomod.h nsomod_rumble) is played here as an
 *    evdev FF_RUMBLE effect on the first gamepad that has one (an N64 first).
 *    Each N64Pad_Rumble call keeps it on for N64PAD_HOLD_MS, so it stops by
 *    itself when frames stop (pause menu, quit). External ports rumble through
 *    their own SDL; with the toggle off they get nso-norumble.so (norumble.c).
 *  - Layout: GLFW has no mapping for 057e:2019 (its GUID carries the device's
 *    version; the 8BitDo in S mode on USB is 057e:2009 with the N64 layout, which
 *    GLFW's Pro Controller entry misreads), so N64Pad_Poll registers one per
 *    connected device and NSO treats it as an N64 pad. Its C buttons
 *    are buttons, not D mode's right stick: N64Pad_RightAxes rebuilds the stick
 *    NSO and the games read from them.
 *  - SDL ports (Perfect Dark): SDL's HIDAPI Switch driver reads this pad as a Pro Controller, and the
 *    Pro report's right stick is a constant 0 here (the C buttons are buttons), which SDL calibrates
 *    to a full deflection: the game turns left forever. So NSO starts such a port with that HIDAPI
 *    driver off and N64Pad_SdlMapping in SDL_GAMECONTROLLERCONFIG (C buttons as the right stick, like
 *    D mode). main.c does it for a game whose env has N64PAD_SDL.
 * The string helpers are pure and unit-tested (input_test.c); the device
 * part is src/n64pad.c. */
#ifndef NSO_N64PAD_H
#define NSO_N64PAD_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N64PAD_SWITCH_VENDOR 0x057e
#define N64PAD_SWITCH_PRODUCT 0x2019      /* Switch Online N64 controller */
#define N64PAD_SWITCH_PRO_PRODUCT 0x2009  /* Pro Controller id: the 8BitDo in S mode over USB */
#define N64PAD_HOLD_MS 150

/* hid-nintendo's N64 buttons in GLFW's order (BTN_A..BTN_MODE = b0..b12): A b0,
 * B b1, C-right b2, C-down b3, C-left b4, Capture b5, L b6, R b7, Z b8, ZR b9,
 * C-up b10, Start b11, Home b12; D-pad hat, stick a0/a1. A / B land on NSO_BTN_A
 * / NSO_BTN_B as in D mode (nso.h), C-up / C-down on NSO_BTN_X / NSO_BTN_Y, and
 * C-left / C-right on the stick clicks; Z = left trigger, ZR = right trigger. */
#define N64PAD_SWITCH_LAYOUT \
    "a:b1,b:b0,y:b10,x:b3,leftstick:b4,rightstick:b2,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1," \
    "guide:b12,leftshoulder:b6,rightshoulder:b7,lefttrigger:b8,righttrigger:b9,start:b11," \
    "leftx:a0,lefty:a1,platform:Linux,"

/* The same layout for SDL (game controller mappings, evdev button numbers as above): D mode's
 * (nso.h NSO_PAD_MAPPINGS) with the right stick made of the four C buttons, half axis each. */
#define N64PAD_SDL_LAYOUT \
    "a:b1,b:b0,dpdown:h0.4,dpleft:h0.8,dpright:h0.2,dpup:h0.1,guide:b12,leftshoulder:b6,lefttrigger:b8," \
    "leftx:a0,lefty:a1,rightshoulder:b7,righttrigger:b9,start:b11," \
    "-rightx:b4,+rightx:b2,-righty:b10,+righty:b3,platform:Linux,"

/* A Switch-mode N64 controller: 057e:2019, or 057e:2009 (the 8BitDo in S mode on
 * USB calls itself a Pro Controller) that hid-nintendo set up with the N64 layout
 * (BTN_C = C-right, no right stick; a real Pro Controller has ABS_RX, no BTN_C). */
static inline bool N64Pad_IsSwitchN64(unsigned vendor, unsigned product, bool hasBtnC, bool hasAbsRx)
{
    if (vendor != N64PAD_SWITCH_VENDOR) return false;
    return product == N64PAD_SWITCH_PRODUCT || (product == N64PAD_SWITCH_PRO_PRODUCT && hasBtnC && !hasAbsRx);
}

/* GLFW's Linux joystick GUID (input_id fields, little-endian). out: 33 bytes. */
static inline void N64Pad_Guid(char *out, unsigned bus, unsigned vendor, unsigned product, unsigned version)
{
    snprintf(out, 33, "%02x%02x0000%02x%02x0000%02x%02x0000%02x%02x0000",
             bus & 0xff, (bus >> 8) & 0xff, vendor & 0xff, (vendor >> 8) & 0xff,
             product & 0xff, (product >> 8) & 0xff, version & 0xff, (version >> 8) & 0xff);
}

/* One SDL-format mapping line for the Switch-mode N64 controller with this GUID. */
static inline int N64Pad_SwitchMapping(char *out, size_t cap, const char *guid)
{
    return snprintf(out, cap, "%s,N64 Controller," N64PAD_SWITCH_LAYOUT "\n", guid);
}

/* One SDL mapping line (SDL_GAMECONTROLLERCONFIG) for the pad with this GUID. */
static inline int N64Pad_SdlMapping(char *out, size_t cap, const char *guid)
{
    return snprintf(out, cap, "%s,N64 Controller," N64PAD_SDL_LAYOUT "\n", guid);
}

/* Bit `bit` of a sysfs capabilities file ("hex words, most significant first"). */
static inline bool N64Pad_CapBit(const char *text, int bit)
{
    unsigned long long w[16];
    int n = 0;
    const char *p = text;
    while (n < 16) {
        char *end;
        while (*p == ' ') p++;
        if (!*p || *p == '\n') break;
        w[n] = strtoull(p, &end, 16);
        if (end == p) break;
        p = end;
        n++;
    }
    int word = bit / 64;
    return word < n && ((w[n - 1 - word] >> (bit % 64)) & 1ULL);
}

/* src/n64pad.c (Linux; elsewhere no-ops). */
/* Once a second: GLFW mappings for new Switch-mode N64 pads, the rumble device. */
void N64Pad_Poll(void);
/* Whether an evdev name (GetGamepadName) is a Switch-mode N64 pad's. */
bool N64Pad_IsSwitchName(const char *name);
/* For an SDL port: the SDL_GAMECONTROLLERCONFIG text (mapping lines) for the Switch-mode N64 pads
 * connected now, 0 when there is none. */
int N64Pad_SdlConfig(char *out, size_t cap);
/* The N64 Rumble setting (on by default). */
void N64Pad_SetEnabled(bool on);
/* level 0..1: the motor this frame (0 stops it), held N64PAD_HOLD_MS. */
void N64Pad_Rumble(float level);

/* n64pad.c includes <linux/input.h>, whose KEY_* macros clash with raylib's. */
#ifndef N64PAD_NO_RAYLIB
#include "raylib.h"
/* Right stick, -1..1, y down; on a Switch-mode N64 pad, its C buttons. */
static inline void N64Pad_RightAxes(int pad, float *rx, float *ry)
{
    *rx = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_X);
    *ry = GetGamepadAxisMovement(pad, GAMEPAD_AXIS_RIGHT_Y);
    if (!IsGamepadAvailable(pad) || !N64Pad_IsSwitchName(GetGamepadName(pad))) return;
    *rx = (float)(IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_THUMB) - IsGamepadButtonDown(pad, GAMEPAD_BUTTON_LEFT_THUMB));
    *ry = (float)(IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_LEFT) - IsGamepadButtonDown(pad, GAMEPAD_BUTTON_RIGHT_FACE_UP));
}
#endif

#endif /* NSO_N64PAD_H */
