/* padui.h - Settings > Controller (NES): the controller drawn in white line art
 * (raylib primitives, after the reference picture) with a lit white button for every
 * pressed control, and Mednafen-style sequential mapping. Included by main.c after the
 * scrape page; uses its statics. The screen is a third page below Image scraping:
 * the Settings page scrolls down to it like it does to the scrape rows.
 *
 * Entering the screen shows the controller, lighting live on every press (A and B light it too
 * while held, as the holds that remap / go back). There are no buttons: the bottom-right hint bar lists "Hold A: Remap   Hold ZR:
 * Reset to default   Hold B: Back". Every hold here (A, B, ZR, skip a prompt) lasts exactly
 * PADUI_HOLD_SEC, two seconds. Holding A starts the mapping (a ring fills on the bar's A glyph);
 * holding ZR (the pad's, or Left Shift, N64's ZL/ZR = Z key in embed.h) deletes nes.map.* and the live controller shows the defaults at once (ignored if that control is mapped to a
 * button). The mapping asks for one button at a time ("Press: Up", Down, Left, Right, Select,
 * Start, B, A) while that button blinks on the drawn controller. A key sets the button's key,
 * a pad button its pad control; a short press assigns on release and advances, holding it two
 * seconds skips the prompt and keeps that button's current mapping. Esc (or ten seconds without
 * input) cancels and keeps the old map. After the last button the map is saved (config/nso.cfg,
 * nes.map.*). Leaving the screen takes holding B for two seconds (a ring fills on its B glyph):
 * short B, Left, Esc and the menu keys do not leave, and Esc does not close NSO while the
 * screen is open. A or B held on entering, or when a mapping ends, count after release.
 * During the mapping every button is just an input (hold to skip).
 *
 * Capture is edge-triggered only (IsKeyPressed / IsGamepadButtonPressed, and a js button
 * going down or an axis leaving its rest after the step started): raylib's
 * GetGamepadButtonPressed repeats while a button is held, so it is not used. The frame the
 * mapping starts reads nothing, and before the first press and after every assignment all
 * inputs (keys, pad buttons, js buttons and axes) must be released, so the press that
 * opened the mapping, or a held button, never lands on a prompt. The pad button is read
 * twice: as raylib's standard button (embedded games, the ports) and as the raw js control
 * (Mednafen), see padmap.h. */
#ifndef NSO_PADUI_H
#define NSO_PADUI_H

#include "padmap.h"
#include "jspad.h"

#define PADUI_PAGE_Y 1.9f          /* top of the page, in screen heights below the Settings page */
#define PADUI_LISTEN_SEC 10.0      /* idle seconds that cancel the mapping */
#define PADUI_PAIR_SEC 0.06        /* how long a pad press waits for its other reading */
#define PADUI_BLINK_HZ 3.0
#define PADUI_HOLD_SEC 2.0         /* every hold on this screen: B back, A remap, ZR reset, skip a prompt */

static NesPadMap nesPad;
static float padExitHeld = 0.0f;   /* seconds B has been held on the idle screen */
static float padRing = 0.0f;       /* the ring on the B glyph, 0..1: follows the hold, drains on release */
static float padRemapHeld = 0.0f, padRingA = 0.0f;   /* the same for A (remap) */
static float padResetHeld = 0.0f, padRingZ = 0.0f;   /* and for ZR / Left Shift (reset) */
static bool padResetWait = true;   /* ZR held on entering must be released first */
static bool padRemapWait = true;   /* an A held when the screen opened (or a mapping ended) must be released first */
static bool padExitGuard = false;  /* B is still held after leaving: Settings must not take it as Back */
static bool padExitWait = true;    /* a B held when the screen opened (or a mapping ended) must be released first */
static NesPadMap padWork;          /* the map being entered; replaces nesPad only when complete */
static bool padOpen = false;
static bool padListen = false;     /* mapping: prompting for button padStep */
static int padStep = 0;            /* NES_BTN_* being asked for */
static double padListenT = 0.0;
static bool padWaitRelease = false; /* mapping waits for every input to be released */
static int padPendStd = -1, padPendKind = PAD_JS_NONE, padPendIdx = 0;
static char padPendDev[40];
static double padPendT = 0.0;
/* The press being judged: held past PADUI_HOLD_SEC it skips the prompt, released sooner it assigns. */
enum { PADCAND_NONE, PADCAND_KEY, PADCAND_PAD };
static int padCand = PADCAND_NONE, padCandKey = 0, padCandStd = -1, padCandJsKind = PAD_JS_NONE, padCandJsIdx = 0;
static char padCandDev[40];
static double padCandT = 0.0;

static void Pad_LoadNes(void) {
    static char text[16384];
    char cfg[2112];
    size_t n = 0;
    Lang_ConfigPath(cfg, sizeof(cfg));
    FILE *f = fopen(cfg, "r");
    if (f) { n = fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[n] = '\0';
    PadMap_NesLoad(&nesPad, text);
}

static void Pad_SaveNes(void) {
    char cfg[2112];
    Lang_ConfigPath(cfg, sizeof(cfg));
    nesPad.custom = true;
    if (!PadMap_NesSave(&nesPad, cfg)) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

/* Esc must not close NSO while this screen is open: leaving takes a two second hold of B. */
static void PadUi_ListenEnd(void) {
    padListen = false;
    padExitWait = true;
    padExitHeld = 0.0f;
    padRemapWait = true;
    padRemapHeld = 0.0f;
    padResetWait = true;
    padResetHeld = 0.0f;
}

static void PadUi_ListenBegin(void) {
    int k, idx;
    char dev[40];
    padWork = nesPad;
    padStep = 0;
    padListen = true;
    padListenT = GetTime();
    padWaitRelease = true;
    padPendStd = -1;
    padPendKind = PAD_JS_NONE;
    padPendT = 0.0;
    padCand = PADCAND_NONE;
    SetExitKey(KEY_NULL);   /* Esc cancels here, it must not close NSO */
    JsPad_Update();
    while (JsPad_TakePress(&k, &idx, dev, sizeof(dev))) { }
}

static void PadUi_Close(void) {
    PadUi_ListenEnd();
    padOpen = false;
    padRing = 0.0f;
    padRingA = 0.0f;
    padRingZ = 0.0f;
    SetExitKey(KEY_ESCAPE);
    JsPad_Close();
}

/* Entering the screen shows the live controller; "Map Controls" starts the mapping. */
static void PadUi_Open(void) {
    padOpen = true;
    SetExitKey(KEY_NULL);
    padExitWait = true;
    padExitHeld = 0.0f;
    padRemapWait = true;
    padRemapHeld = 0.0f;
    padResetWait = true;
    padResetHeld = 0.0f;
    padRingA = 0.0f;
    padRingZ = 0.0f;
    padListen = false;
    JsPad_Open();
}

static bool PadUi_Lit(int btn) {
    const PadBind *b = &nesPad.b[btn];
    return (b->key && IsKeyDown(b->key))
        || (b->pad >= 0 && IsGamepadAvailable(0) && IsGamepadButtonDown(0, b->pad))
        || (b->jsKind != PAD_JS_NONE && JsPad_Down(b->jsKind, b->jsIdx));
}

/* The current button is done (assigned, or skipped keeping its map): next one, or finished
 * (saved, shown live). Everything must be released again before the next prompt reads. */
static void PadUi_Assigned(double now, bool skipped) {
    Sfx_Play(skipped ? SFX_PLUCK : SFX_CLICK);
    padCand = PADCAND_NONE;
    padListenT = now;
    padWaitRelease = true;
    padPendT = 0.0;
    padPendStd = -1;
    padPendKind = PAD_JS_NONE;
    if (++padStep >= NES_BTN_COUNT) {
        nesPad = padWork;
        Pad_SaveNes();
        PadUi_ListenEnd();
    }
}

/* Is any key, pad button or raw js control held. */
static bool PadUi_AnyHeld(void) {
    for (int i = 0; i < PADMAP_KEY_COUNT; i++) if (IsKeyDown(PADMAP_KEYS[i].key)) return true;
    for (int i = 0; i < PADMAP_PAD_COUNT; i++)
        if (IsGamepadAvailable(0) && IsGamepadButtonDown(0, PADMAP_PADS[i].btn)) return true;
    return JsPad_AnyDown();
}

/* One frame of mapping: Esc cancels; once everything is released, a key press, or a pad
 * button press (its standard and raw readings, which can land a frame apart), assigns. */
static void PadUi_ListenPoll(void) {
    int k, idx;
    char dev[40];
    double now = GetTime();
    if (IsKeyPressed(KEY_ESCAPE)) { PadUi_ListenEnd(); return; }
    if (padWaitRelease) {   /* nothing is read until all inputs are up; waiting does not time out */
        padListenT = now;
        while (JsPad_TakePress(&k, &idx, dev, sizeof(dev))) { }
        if (!PadUi_AnyHeld()) padWaitRelease = false;   /* the next frame listens */
        return;
    }
    if (padCand != PADCAND_NONE) {   /* a press is being judged: short = assign on release, long = skip */
        bool held = padCand == PADCAND_KEY ? IsKeyDown(padCandKey)
                  : (padCandStd >= 0 && IsGamepadAvailable(0) && IsGamepadButtonDown(0, padCandStd))
                    || (padCandJsKind != PAD_JS_NONE && JsPad_Down(padCandJsKind, padCandJsIdx));
        if (held) {
            if (now - padCandT >= PADUI_HOLD_SEC) PadUi_Assigned(now, true);   /* keeps the old mapping */
            return;
        }
        if (padCand == PADCAND_KEY) padWork.b[padStep].key = padCandKey;
        else {
            PadBind *b = &padWork.b[padStep];
            b->pad = padCandStd;
            b->jsKind = padCandJsKind;
            b->jsIdx = padCandJsKind != PAD_JS_NONE ? padCandJsIdx : 0;
            if (padCandJsKind != PAD_JS_NONE) snprintf(padWork.joyId, sizeof(padWork.joyId), "%s", padCandDev);
        }
        PadUi_Assigned(now, false);
        return;
    }
    if (now - padListenT > PADUI_LISTEN_SEC) { PadUi_ListenEnd(); return; }
    for (int i = 0; i < PADMAP_KEY_COUNT; i++) {
        if (IsKeyPressed(PADMAP_KEYS[i].key)) {
            padCand = PADCAND_KEY;
            padCandKey = PADMAP_KEYS[i].key;
            padCandT = now;
            return;
        }
    }
    {
        for (int i = 0; i < PADMAP_PAD_COUNT && padPendStd < 0; i++)
            if (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, PADMAP_PADS[i].btn)) { padPendStd = PADMAP_PADS[i].btn; if (padPendT == 0.0) padPendT = now; }
        if (padPendKind == PAD_JS_NONE && JsPad_TakePress(&k, &idx, dev, sizeof(dev))) {
            padPendKind = k;
            padPendIdx = idx;
            snprintf(padPendDev, sizeof(padPendDev), "%s", dev);
            if (padPendT == 0.0) padPendT = now;
        }
    }
    if (padPendT != 0.0 && ((padPendStd >= 0 && padPendKind != PAD_JS_NONE) || now - padPendT > PADUI_PAIR_SEC)) {
        padCand = PADCAND_PAD;   /* both readings are in: judge the press */
        padCandStd = padPendStd;
        padCandJsKind = padPendKind;
        padCandJsIdx = padPendIdx;
        snprintf(padCandDev, sizeof(padCandDev), "%s", padPendDev);
        padCandT = padPendT;
        padPendT = 0.0;
        padPendStd = -1;
        padPendKind = PAD_JS_NONE;
    }
}

/* Is B held: NSO's back (the B key, the pad's B) or the control mapped to the NES B. */
static bool PadUi_BDown(void) {
    const PadBind *b = &nesPad.b[NES_BTN_B];
    if (IsKeyDown(KEY_B) || (b->key && IsKeyDown(b->key))) return true;
    if (IsGamepadAvailable(0) && (IsGamepadButtonDown(0, NSO_BTN_B) || (b->pad >= 0 && IsGamepadButtonDown(0, b->pad)))) return true;
    return b->jsKind != PAD_JS_NONE && JsPad_Down(b->jsKind, b->jsIdx);
}

/* The one way out: B held for PADUI_HOLD_SEC. Short B, Left, Esc and the menu keys do nothing. */
static void PadUi_ExitTick(void) {
    float dt = GetFrameTime();
    bool down = PadUi_BDown();
    if (padExitWait) {
        if (!down) padExitWait = false;
        padExitHeld = 0.0f;
    } else if (down) {
        padExitHeld += dt;
        padRing = fminf(1.0f, padExitHeld / PADUI_HOLD_SEC);
        if (padExitHeld >= PADUI_HOLD_SEC) { Sfx_Play(SFX_CLICK); PadUi_Close(); padExitGuard = true; }
        return;
    } else {
        padExitHeld = 0.0f;
    }
    padRing = fmaxf(0.0f, padRing - dt * 4.0f);   /* smoothly resets on release */
}

/* Is A held: NSO's confirm (Enter, the A key, the pad's A) or the control mapped to the NES A. */
static bool PadUi_ADown(void) {
    const PadBind *b = &nesPad.b[NES_BTN_A];
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_ENTER) || (b->key && IsKeyDown(b->key))) return true;
    if (IsGamepadAvailable(0) && (IsGamepadButtonDown(0, NSO_BTN_A) || (b->pad >= 0 && IsGamepadButtonDown(0, b->pad)))) return true;
    return b->jsKind != PAD_JS_NONE && JsPad_Down(b->jsKind, b->jsIdx);
}

/* Holding A for PADUI_HOLD_SEC starts the mapping; a short A only lights the controller. */
static bool PadUi_RemapTick(void) {
    float dt = GetFrameTime();
    bool down = PadUi_ADown();
    if (padRemapWait) {
        if (!down) padRemapWait = false;
        padRemapHeld = 0.0f;
    } else if (down) {
        padRemapHeld += dt;
        padRingA = fminf(1.0f, padRemapHeld / PADUI_HOLD_SEC);
        if (padRemapHeld >= PADUI_HOLD_SEC) {
            Sfx_Play(SFX_CLICK);
            padRingA = 0.0f;
    padRingZ = 0.0f;
            PadUi_ListenBegin();   /* reads nothing this frame; the gate waits for A to be released */
            return true;
        }
        return false;
    } else {
        padRemapHeld = 0.0f;
    }
    padRingA = fmaxf(0.0f, padRingA - dt * 4.0f);
    return false;
}

/* Holding ZR (pad), or Left Shift on the keyboard (the N64 ZL / ZR = Z key), for PADUI_HOLD_SEC
 * puts the defaults back. A control that is mapped to a NES button never resets. */
static void PadUi_ResetTick(void) {
    float dt = GetFrameTime();
    bool zr = IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT);
    bool down;
    for (int i = 0; i < NES_BTN_COUNT; i++) {
        if (nesPad.b[i].pad == GAMEPAD_BUTTON_RIGHT_TRIGGER_2) zr = false;
        if (nesPad.b[i].key == KEY_LEFT_SHIFT) shift = false;
    }
    down = zr || shift;
    if (padResetWait) {
        if (!down) padResetWait = false;
        padResetHeld = 0.0f;
    } else if (down) {
        padResetHeld += dt;
        padRingZ = fminf(1.0f, padResetHeld / PADUI_HOLD_SEC);
        if (padResetHeld >= PADUI_HOLD_SEC) {
            char cfg[2112];
            Sfx_Play(SFX_CLICK);
            Lang_ConfigPath(cfg, sizeof(cfg));
            PadMap_NesDefaults(&nesPad);
            if (!PadMap_NesClear(cfg)) fprintf(stderr, "NSO: cannot save %s\n", cfg);
            padResetHeld = 0.0f;
            padRingZ = 0.0f;
            padResetWait = true;   /* once per hold: release before it can fire again */
        }
        return;
    } else {
        padResetHeld = 0.0f;
    }
    padRingZ = fmaxf(0.0f, padRingZ - dt * 4.0f);
}

static void PadUi_Input(bool up, bool down, bool ok, bool back, bool left) {
    (void)up; (void)down; (void)ok; (void)back; (void)left;
    JsPad_Update();
    if (padListen) {   /* every button is just an input here: hold it to skip the prompt */
        padRing = fmaxf(0.0f, padRing - GetFrameTime() * 4.0f);
        padRingA = fmaxf(0.0f, padRingA - GetFrameTime() * 4.0f);
        padRingZ = fmaxf(0.0f, padRingZ - GetFrameTime() * 4.0f);
        PadUi_ListenPoll();
        return;
    }
    PadUi_ExitTick();
    if (!padOpen) return;
    if (PadUi_RemapTick()) return;
    PadUi_ResetTick();
}

/* The bottom-right hint bar text of this screen (NULL when it is not open) and the fill of
 * the ring drawn on its B glyph; main.c's DrawInfoBarEx uses both. */
static const char *PadUi_BarText(void) {
    if (!padOpen) return NULL;
    return padListen ? T(S_PC_BAR_MAP) : T(S_PC_BAR);
}
/* True while the B that left the screen is still held (it is not Back on the Settings page). */
static bool PadUi_SwallowBack(void) {
    if (padExitGuard && !PadUi_BDown()) padExitGuard = false;
    return padExitGuard;
}
/* The fill of the ring on the bar's glyph: 0 = B (leave), 1 = A (remap), 2 = ZR (reset). */
static float PadUi_RingProgress(int which) { return padOpen ? (which == 2 ? padRingZ : which == 1 ? padRingA : padRing) : 0.0f; }

/* ---- the controller ---- */

typedef struct { float x, y, s; } PadView;   /* screen origin of the reference (70,165) and its scale */

static Vector2 PadV(PadView v, float rx, float ry) { return (Vector2){ v.x + (rx - 70.0f) * v.s, v.y + (ry - 165.0f) * v.s }; }

static Rectangle PadR(PadView v, float x0, float y0, float x1, float y1) {
    Vector2 a = PadV(v, x0, y0), b = PadV(v, x1, y1);
    return (Rectangle){ a.x, a.y, b.x - a.x, b.y - a.y };
}

static void PadRRect(PadView v, float x0, float y0, float x1, float y1, float radius, float th, Color c) {
    Rectangle r = PadR(v, x0, y0, x1, y1);
    float m = fminf(r.width, r.height);
    DrawRectangleRoundedLinesEx(r, m > 0.0f ? fminf(1.0f, 2.0f * radius * v.s / m) : 0.0f, 6, th, c);
}

static void PadLoop(const Vector2 *p, int n, float th, Color c) {
    for (int i = 0; i < n; i++) {
        Vector2 a = p[i], b = p[(i + 1) % n];
        DrawLineEx(a, b, th, c);
        DrawCircleV(b, th * 0.5f, c);
    }
}

/* The D-pad's plus shape: arm half-width hw, half-length hl, arm ends cut by ch. */
static int PadCross(PadView v, float cx, float cy, float hw, float hl, float ch, Vector2 *out) {
    float px[12] = { -hw, hw, hw, hl, hl, hw, hw, -hw, -hw, -hl, -hl, -hw };
    float py[12] = { -hl, -hl, -hw, -hw, hw, hw, hl, hl, hw, hw, -hw, -hw };
    static const bool end[12] = { 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0 };
    int n = 0;
    for (int i = 0; i < 12; i++) {
        if (!end[i]) { out[n++] = PadV(v, cx + px[i], cy + py[i]); continue; }
        int pi = (i + 11) % 12, ni = (i + 1) % 12;
        float dx1 = px[pi] - px[i], dy1 = py[pi] - py[i], dx2 = px[ni] - px[i], dy2 = py[ni] - py[i];
        float l1 = sqrtf(dx1 * dx1 + dy1 * dy1), l2 = sqrtf(dx2 * dx2 + dy2 * dy2);
        out[n++] = PadV(v, cx + px[i] + dx1 / l1 * ch, cy + py[i] + dy1 / l1 * ch);
        out[n++] = PadV(v, cx + px[i] + dx2 / l2 * ch, cy + py[i] + dy2 / l2 * ch);
    }
    return n;
}

/* dir 0 up, 1 down, 2 left, 3 right. */
static void PadArrow(PadView v, float cx, float cy, int dir, float th, Color c) {
    static const float dx[4] = { 0, 0, -1, 1 }, dy[4] = { -1, 1, 0, 0 };
    float ux = dx[dir], uy = dy[dir], nx = -uy, ny = ux;
    const float tip = 25.0f, shoulder = 20.0f, base = 14.0f, half = 5.0f;
    Vector2 p[5] = {
        PadV(v, cx + ux * tip, cy + uy * tip),
        PadV(v, cx + ux * shoulder + nx * half, cy + uy * shoulder + ny * half),
        PadV(v, cx + ux * base + nx * half, cy + uy * base + ny * half),
        PadV(v, cx + ux * base - nx * half, cy + uy * base - ny * half),
        PadV(v, cx + ux * shoulder - nx * half, cy + uy * shoulder - ny * half),
    };
    PadLoop(p, 5, th, c);
}

/* The NES controller in white line art in a box of width w at (x, y); lit[b] fills that
 * button white. */
static void DrawNesController(float x, float y, float w, const bool lit[NES_BTN_COUNT], Color c) {
    PadView v = { x, y, w / 340.0f };
    float th = fmaxf(1.5f, 1.3f * v.s);
    const float dcx = 131.0f, dcy = 253.0f;
    Vector2 cross[20];
    int n;

    /* body */
    PadRRect(v, 71, 166, 408, 313, 7, th, c);
    {
        Rectangle in = PadR(v, 80, 190, 399, 302);
        DrawRectangleLinesEx(in, th, c);
    }
    PadRRect(v, 180, 190, 277, 206, 2, th, c);
    PadRRect(v, 180, 210, 277, 226, 2, th, c);
    PadRRect(v, 180, 231, 277, 247, 2, th, c);
    PadRRect(v, 180, 292, 277, 302, 2, th, c);
    PadRRect(v, 180, 252, 277, 286, 3, th, c);
    PadRRect(v, 185, 256, 272, 282, 3, th, c);

    /* Select and Start: the pills */
    {
        Rectangle sp = PadR(v, 193, 264, 220, 276), tp = PadR(v, 237, 264, 265, 276);
        if (lit[NES_BTN_SELECT]) DrawRectangleRounded(sp, 1.0f, 8, c);
        if (lit[NES_BTN_START]) DrawRectangleRounded(tp, 1.0f, 8, c);
        DrawRectangleRoundedLinesEx(sp, 1.0f, 8, th, c);
        DrawRectangleRoundedLinesEx(tp, 1.0f, 8, th, c);
    }

    /* D-pad: a lit arm is filled white */
    if (lit[NES_BTN_UP]) DrawRectangleRec(PadR(v, dcx - 9.5f, dcy - 29.0f, dcx + 9.5f, dcy - 7.0f), c);
    if (lit[NES_BTN_DOWN]) DrawRectangleRec(PadR(v, dcx - 9.5f, dcy + 7.0f, dcx + 9.5f, dcy + 29.0f), c);
    if (lit[NES_BTN_LEFT]) DrawRectangleRec(PadR(v, dcx - 29.0f, dcy - 9.5f, dcx - 7.0f, dcy + 9.5f), c);
    if (lit[NES_BTN_RIGHT]) DrawRectangleRec(PadR(v, dcx + 7.0f, dcy - 9.5f, dcx + 29.0f, dcy + 9.5f), c);
    n = PadCross(v, dcx, dcy, 14.0f, 34.0f, 3.0f, cross);
    PadLoop(cross, n, th, c);
    n = PadCross(v, dcx, dcy, 10.0f, 29.0f, 2.5f, cross);
    PadLoop(cross, n, th, c);
    DrawCircleLinesV(PadV(v, dcx, dcy), 6.5f * v.s, c);
    if (!lit[NES_BTN_UP]) PadArrow(v, dcx, dcy, 0, th, c);
    if (!lit[NES_BTN_DOWN]) PadArrow(v, dcx, dcy, 1, th, c);
    if (!lit[NES_BTN_LEFT]) PadArrow(v, dcx, dcy, 2, th, c);
    if (!lit[NES_BTN_RIGHT]) PadArrow(v, dcx, dcy, 3, th, c);

    /* B and A */
    PadRRect(v, 290, 250, 327, 288, 3, th, c);
    PadRRect(v, 334, 250, 371, 288, 3, th, c);
    {
        Vector2 bc = PadV(v, 308.5f, 269.0f), ac = PadV(v, 352.5f, 269.0f);
        float r = 14.5f * v.s;
        if (lit[NES_BTN_B]) DrawCircleV(bc, r, c);
        if (lit[NES_BTN_A]) DrawCircleV(ac, r, c);
        DrawRing(bc, r - th * 0.5f, r + th * 0.5f, 0.0f, 360.0f, 48, c);
        DrawRing(ac, r - th * 0.5f, r + th * 0.5f, 0.0f, 360.0f, 48, c);
    }
}

static void PadUi_Draw(Font font, Color bg) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float hx = sw * 0.449f, bx = sw * 0.485f, bw = sw * 0.387f;
    float oy = sh * PADUI_PAGE_Y;
    float sc = bw / 340.0f;                      /* the reference's scale; the body is 147 tall, centred on ref y 239.5 */
    float cy = oy + sh * 0.5f;                   /* the controller sits in the middle of the page */
    bool lit[NES_BTN_COUNT];
    (void)bg;

    for (int i = 0; i < NES_BTN_COUNT; i++) lit[i] = false;
    if (padOpen && padListen) lit[padStep] = ((int)(GetTime() * PADUI_BLINK_HZ * 2.0) & 1) == 0;   /* blinks */
    else for (int i = 0; i < NES_BTN_COUNT; i++) lit[i] = padOpen && PadUi_Lit(i);
    if (padOpen && !padListen) {   /* the A / B that hold to remap / go back still light the pad while held */
        if (PadUi_ADown()) lit[NES_BTN_A] = true;
        if (PadUi_BDown()) lit[NES_BTN_B] = true;
    }
    DrawFitText(font, T(S_PC_ENTRY), (Vector2){ hx, oy + sh * 0.092f }, (float)font.baseSize * 0.85f, sw - hx, WHITE);
    DrawNesController(bx, cy - (239.5f - 165.0f) * sc, bw, lit, WHITE);
    if (padOpen && padListen) {   /* the one prompt under the controller: "Press: A", and the skip cue */
        char line[96];
        float y = cy + 74.0f * sc + sh * 0.07f;
        float size = (float)font.baseSize * 1.5f;
        snprintf(line, sizeof(line), "%s: %s", T(S_PC_PRESS), T((StrId)(S_PC_UP + padStep)));
        Vector2 m = MeasureTextEx(font, line, size, 1.0f);
        if (m.x > bw) { size *= bw / m.x; m = MeasureTextEx(font, line, size, 1.0f); }
        DrawFitText(font, line, (Vector2){ bx + (bw - m.x) * 0.5f, y }, size, m.x + 1.0f, WHITE);
        {
            float hs = (float)font.baseSize * 0.75f;
            Vector2 hm = MeasureTextEx(font, T(S_PC_SKIP), hs, 1.0f);
            if (hm.x > bw) { hs *= bw / hm.x; hm = MeasureTextEx(font, T(S_PC_SKIP), hs, 1.0f); }
            DrawFitText(font, T(S_PC_SKIP), (Vector2){ bx + (bw - hm.x) * 0.5f, y + sh * 0.055f }, hs, hm.x + 1.0f, WHITE);
        }
    }
}

#endif /* NSO_PADUI_H */
