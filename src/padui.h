/* padui.h - Settings > Controller (NES, SNES and N64): the controller drawn in white line art
 * (raylib primitives, after the reference picture) with a lit white button for every
 * pressed control, and Mednafen-style sequential mapping. Included by main.c after the
 * scrape page; uses its statics. The screen is a third page below Image scraping:
 * the Settings page scrolls down to it like it does to the scrape rows.
 *
 * The same screen serves both systems (the PadSys of padmap.h picks the buttons, the drawing, the
 * config keys): SNES adds Y, X, L, R and the Switch Online pad's ZL / ZR (drawn smaller beside L / R
 * as tabs on the top edge, mappable and lit, but no SNES game reads them).
 *
 * Entering the screen shows the controller, lighting live on every press (only the button bound to the
 * pressed control lights, also when that control is the one that holds to remap / go back). There are no buttons: the bottom-right hint bar lists "Hold A: Remap   Hold ZR:
 * Reset to default   Hold B: Back". Every hold here (A, B, ZR, skip a prompt) lasts exactly
 * PADUI_HOLD_SEC, 0.9 seconds. Holding A starts the mapping (a ring fills on the bar's A glyph);
 * holding ZR (the pad's, or Left Shift, N64's ZL/ZR = Z key in embed.h) deletes nes.map.* and the live controller shows the defaults at once (ignored if that control is mapped to a
 * button). The mapping asks for one button at a time ("Press: Up", Down, Left, Right, Select,
 * Start, B, A) while that button blinks on the drawn controller. A key sets the button's key,
 * a pad button its pad control; a short press assigns on release and advances, holding it 0.9
 * seconds skips the prompt and keeps that button's current mapping. Esc (or ten seconds without
 * input) cancels and keeps the old map. After the last button the map is saved (config/nso.cfg,
 * nes.map.*). Leaving the screen takes holding B for 0.9 seconds (a ring fills on its B glyph):
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
#define PADUI_HOLD_SEC 0.9         /* every hold on this screen: B back, A remap, ZR reset, skip a prompt */

static NesPadMap nesPad, snesPad, n64Pad;    /* the map of each system (padmap.h) */
static const PadSys *padSys = &PADSYS_NES;   /* the system whose screen is open */
static PadSysMap *padMapP = &nesPad;
#define PADCUR (*padMapP)
static float padExitHeld = 0.0f;   /* seconds B has been held on the idle screen */
static float padRing = 0.0f;       /* the ring on the B glyph, 0..1: follows the hold, drains on release */
static float padRemapHeld = 0.0f, padRingA = 0.0f;   /* the same for A (remap) */
static float padResetHeld = 0.0f, padRingZ = 0.0f;   /* and for ZR / Left Shift (reset) */
static bool padResetWait = true;   /* ZR held on entering must be released first */
static bool padRemapWait = true;   /* an A held when the screen opened (or a mapping ended) must be released first */
static bool padExitGuard = false;  /* B is still held after leaving: Settings must not take it as Back */
static bool padExitWait = true;    /* a B held when the screen opened (or a mapping ended) must be released first */
static PadSysMap padWork;          /* the map being entered; replaces the system's map only when complete */
static bool padOpen = false;
static bool padListen = false;     /* mapping: prompting for button padStep */
static int padStep = 0;            /* the button (index into padSys->names) being asked for */
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

static void Pad_LoadMaps(void) {
    static char text[16384];
    char cfg[2112];
    size_t n = 0;
    Lang_ConfigPath(cfg, sizeof(cfg));
    FILE *f = fopen(cfg, "r");
    if (f) { n = fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[n] = '\0';
    PadMap_Load(&PADSYS_NES, &nesPad, text);
    PadMap_Load(&PADSYS_SNES, &snesPad, text);
    PadMap_Load(&PADSYS_N64, &n64Pad, text);
}

static void Pad_SaveMap(void) {
    char cfg[2112];
    Lang_ConfigPath(cfg, sizeof(cfg));
    PADCUR.custom = true;
    if (!PadMap_Save(padSys, padMapP, cfg)) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

/* Esc must not close NSO while this screen is open: leaving takes a 0.9 second hold of B. */
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
    padWork = PADCUR;
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
static void PadUi_Select(int kind) {   /* the system of a screen: 0 NES, 1 SNES, 2 N64 */
    padSys = kind == 2 ? &PADSYS_N64 : kind == 1 ? &PADSYS_SNES : &PADSYS_NES;
    padMapP = kind == 2 ? &n64Pad : kind == 1 ? &snesPad : &nesPad;
}

static void PadUi_Open(int kind) {
    PadUi_Select(kind);
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

/* N64: the analog stick and the C buttons (a Switch-mode pad's buttons or another pad's right stick) are
 * read by the games whatever the map says; they light their directions too. */
static bool PadUi_N64Analog(int btn) {
    float lx, ly, rx, ry;
    if (padSys != &PADSYS_N64 || !IsGamepadAvailable(0)) return false;
    lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
    ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
    N64Pad_RightAxes(0, &rx, &ry);
    switch (btn) {
        case N64_BTN_SUP: return ly < -0.5f;
        case N64_BTN_SDOWN: return ly > 0.5f;
        case N64_BTN_SLEFT: return lx < -0.5f;
        case N64_BTN_SRIGHT: return lx > 0.5f;
        case N64_BTN_CUP: return ry < -0.5f;
        case N64_BTN_CDOWN: return ry > 0.5f;
        case N64_BTN_CLEFT: return rx < -0.5f;
        case N64_BTN_CRIGHT: return rx > 0.5f;
        default: return false;
    }
}

static bool PadUi_Lit(int btn) {
    const PadBind *b = &PADCUR.b[btn];
    return PadUi_N64Analog(btn) || (b->key && IsKeyDown(b->key))
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
    if (++padStep >= padSys->count) {
        PADCUR = padWork;
        Pad_SaveMap();
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
    const PadBind *b = &PADCUR.b[padSys->btnB];
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
    const PadBind *b = &PADCUR.b[padSys->btnA];
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
 * puts the defaults back. A control that is mapped to a button of the pad (other than the SNES ZR
 * itself) never resets. */
static void PadUi_ResetTick(void) {
    float dt = GetFrameTime();
    bool zr = IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
    bool shift = IsKeyDown(KEY_LEFT_SHIFT);
    bool down;
    for (int i = 0; i < padSys->count; i++) {   /* the system's own ZR button (SNES) is what this resets with */
        if (i == padSys->btnZR) continue;
        if (PADCUR.b[i].pad == GAMEPAD_BUTTON_RIGHT_TRIGGER_2) zr = false;
        if (PADCUR.b[i].key == KEY_LEFT_SHIFT) shift = false;
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
            PadMap_Defaults(padSys, padMapP);
            if (!PadMap_Clear(padSys, cfg)) fprintf(stderr, "NSO: cannot save %s\n", cfg);
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
    DrawRectangleRoundedLinesEx(r, m > 0.0f ? fminf(1.0f, 2.0f * radius * v.s / m) : 0.0f, 24, th, c);
}

/* A circle outline of thickness th: enough segments to look round at any scale. */
static void PadCircle(Vector2 ctr, float r, float th, Color c) {
    DrawRing(ctr, fmaxf(0.0f, r - th * 0.5f), r + th * 0.5f, 0.0f, 360.0f, 120, c);
}

/* A triangle drawn with both windings: backface culling cannot hide it. */
static void PadTri(Vector2 a, Vector2 b, Vector2 c2, Color c) {
    DrawTriangle(a, b, c2, c);
    DrawTriangle(a, c2, b, c);
}

/* An open polyline with round joins and round caps. */
static void PadPoly(const Vector2 *p, int n, float th, Color c) {
    for (int i = 0; i + 1 < n; i++) DrawLineEx(p[i], p[i + 1], th, c);
    for (int i = 0; i < n; i++) DrawCircleV(p[i], th * 0.5f, c);
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
        if (lit[NES_BTN_SELECT]) DrawRectangleRounded(sp, 1.0f, 24, c);
        if (lit[NES_BTN_START]) DrawRectangleRounded(tp, 1.0f, 24, c);
        DrawRectangleRoundedLinesEx(sp, 1.0f, 24, th, c);
        DrawRectangleRoundedLinesEx(tp, 1.0f, 24, th, c);
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
    PadCircle(PadV(v, dcx, dcy), 6.5f * v.s, th, c);
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
        PadCircle(bc, r, th, c);
        PadCircle(ac, r, th, c);
    }
}

/* A slanted capsule from a to b (radius r, screen units): outline, filled white when on. */
static void PadCapsule(Vector2 a, Vector2 b, float r, float th, bool on, Color c) {
    Vector2 pts[66];
    float ang = atan2f(b.y - a.y, b.x - a.x);
    int n = 0;
    if (on) {
        PadTri((Vector2){ a.x + sinf(ang) * r, a.y - cosf(ang) * r }, (Vector2){ b.x + sinf(ang) * r, b.y - cosf(ang) * r },
               (Vector2){ b.x - sinf(ang) * r, b.y + cosf(ang) * r }, c);
        PadTri((Vector2){ a.x + sinf(ang) * r, a.y - cosf(ang) * r }, (Vector2){ b.x - sinf(ang) * r, b.y + cosf(ang) * r },
               (Vector2){ a.x - sinf(ang) * r, a.y + cosf(ang) * r }, c);
        DrawCircleV(a, r, c);
        DrawCircleV(b, r, c);
    }
    for (int i = 0; i <= 32; i++) {   /* the far end, then the near one */
        float t = ang - PI * 0.5f + PI * (float)i / 32.0f;
        pts[n++] = (Vector2){ b.x + cosf(t) * r, b.y + sinf(t) * r };
    }
    for (int i = 0; i <= 32; i++) {
        float t = ang + PI * 0.5f + PI * (float)i / 32.0f;
        pts[n++] = (Vector2){ a.x + cosf(t) * r, a.y + sinf(t) * r };
    }
    PadLoop(pts, n, th, c);
}

/* A closed polygon with each corner rounded (radius rad[i], screen units; 90 degree corners). */
static void PadRoundPoly(const Vector2 *p, const float *rad, int n, float th, Color c) {
    Vector2 out[12 * 12];
    int k = 0;
    for (int i = 0; i < n; i++) {
        Vector2 pv = p[(i + n - 1) % n], nx = p[(i + 1) % n], q = p[i];
        float l1 = hypotf(pv.x - q.x, pv.y - q.y), l2 = hypotf(nx.x - q.x, nx.y - q.y);
        float r = fminf(rad[i], fminf(l1, l2) * 0.5f);
        Vector2 d1 = { (pv.x - q.x) / l1, (pv.y - q.y) / l1 }, d2 = { (nx.x - q.x) / l2, (nx.y - q.y) / l2 };
        Vector2 a = { q.x + d1.x * r, q.y + d1.y * r }, b = { q.x + d2.x * r, q.y + d2.y * r };
        Vector2 ctr = { q.x + (d1.x + d2.x) * r, q.y + (d1.y + d2.y) * r };
        float a0 = atan2f(a.y - ctr.y, a.x - ctr.x), a1 = atan2f(b.y - ctr.y, b.x - ctr.x), d = a1 - a0;
        while (d > PI) d -= 2.0f * PI;
        while (d < -PI) d += 2.0f * PI;
        for (int j = 0; j <= 10; j++) {
            float t = a0 + d * (float)j / 10.0f;
            out[k++] = (Vector2){ ctr.x + cosf(t) * r, ctr.y + sinf(t) * r };
        }
    }
    PadLoop(out, k, th, c);
}

/* A closed convex polygon with every corner rounded by radius r (screen units, any corner angle);
 * filled white first when fill is set. */
static void PadRoundPolyG(const Vector2 *p, int n, float r, float th, bool fill, Color c) {
    Vector2 out[8 * 12];
    int k = 0;
    for (int i = 0; i < n; i++) {
        Vector2 pv = p[(i + n - 1) % n], nx = p[(i + 1) % n], q = p[i];
        float l1 = hypotf(pv.x - q.x, pv.y - q.y), l2 = hypotf(nx.x - q.x, nx.y - q.y);
        Vector2 d1 = { (pv.x - q.x) / l1, (pv.y - q.y) / l1 }, d2 = { (nx.x - q.x) / l2, (nx.y - q.y) / l2 };
        float half = acosf(fminf(1.0f, fmaxf(-1.0f, d1.x * d2.x + d1.y * d2.y))) * 0.5f;   /* half the corner's angle */
        float tl = r / tanf(half), cd = r / sinf(half);
        Vector2 bis = { d1.x + d2.x, d1.y + d2.y };
        float bl = hypotf(bis.x, bis.y);
        Vector2 ctr = { q.x + bis.x / bl * cd, q.y + bis.y / bl * cd };
        Vector2 a = { q.x + d1.x * tl, q.y + d1.y * tl }, b = { q.x + d2.x * tl, q.y + d2.y * tl };
        float a0 = atan2f(a.y - ctr.y, a.x - ctr.x), a1 = atan2f(b.y - ctr.y, b.x - ctr.x), d = a1 - a0;
        while (d > PI) d -= 2.0f * PI;
        while (d < -PI) d += 2.0f * PI;
        for (int j = 0; j <= 10; j++) {
            float tt = a0 + d * (float)j / 10.0f;
            out[k++] = (Vector2){ ctr.x + cosf(tt) * r, ctr.y + sinf(tt) * r };
        }
    }
    if (fill) for (int i = 1; i + 1 < k; i++) PadTri(out[0], out[i], out[i + 1], c);
    PadLoop(out, k, th, c);
}

/* A quadratic round-off of a corner: from a through the corner q to b, appended to a path. */
static int PadBend(Vector2 *path, int m, Vector2 a, Vector2 q, Vector2 b) {
    for (int i = 0; i <= 8; i++) {
        float u = (float)i / 8.0f;
        path[m++] = (Vector2){ (1 - u) * (1 - u) * a.x + 2 * u * (1 - u) * q.x + u * u * b.x,
                               (1 - u) * (1 - u) * a.y + 2 * u * (1 - u) * q.y + u * u * b.y };
    }
    return m;
}

/* The SNES body's upper edge at x (reference units): the lobes' arcs, flat between them. */
static float PadSnesTopY(float x) {
    const float R = 185.0f, cy = 342.0f;
    float cx = x < 210.0f ? 210.0f : (x > 716.0f ? 716.0f : x);
    float dx = x - cx;
    return cy - sqrtf(fmaxf(0.0f, R * R - dx * dx));
}

/* A shoulder tab: the open outline ref[0..n) (reference units, ends on the body's edge), its outline
 * line, and when lit the strip between it and the body's edge filled white. */
static void PadSnesTab(PadView v, const Vector2 *ref, int n, bool lit, float th, Color c) {
    Vector2 sc[64];
    for (int i = 0; i < n; i++) sc[i] = PadV(v, ref[i].x, ref[i].y);
    if (lit)
        for (int i = 0; i + 1 < n; i++) {
            Vector2 ia = PadV(v, ref[i].x, PadSnesTopY(ref[i].x)), ib = PadV(v, ref[i + 1].x, PadSnesTopY(ref[i + 1].x));
            PadTri(sc[i], sc[i + 1], ib, c);
            PadTri(sc[i], ib, ia, c);
        }
    PadPoly(sc, n, th, c);
}

/* The width of the SNES drawing's box and its height, in the reference picture's units. */
#define PADSNES_W 876.0f
#define PADSNES_H 382.0f

static float PadMirrorX(float x) { return 926.0f - x; }   /* the pad is symmetric about x = 463 */

/* The SNES controller (Switch Online pad: ZL / ZR too) in white line art after the reference picture
 * (the top view, the box 25..901 x 146..528): the raised shoulder tabs on its upper edge (L and R at the
 * lobes, the smaller ZL and ZR inboard of them), the D-pad in its ring, Select / Start as slanted pills
 * and the four face buttons in two capsule wells. (x, y) is the top-left of the box, w its width;
 * lit[b] fills that button white. Line weights: the outer lines weigh the same as the NES
 * controller's (1.3 per 340 px of width), the inner ones are lighter, as in the reference. */
static void DrawSnesController(float x, float y, float w, const bool lit[PAD_MAX_BTN], Color c) {
    float s = w / PADSNES_W;
    PadView v = { x + 45.0f * s, y + 19.0f * s, s };   /* PadV(rx, ry) = (x + (rx - 25) s, y + (ry - 146) s) */
    float th = fmaxf(1.5f, 1.3f * w / 340.0f);          /* the NES outer line */
    float thM = th * 0.8f, thL = fmaxf(1.0f, th * 0.55f);   /* details, hairlines */
    Vector2 pts[300];
    int n = 0;
    const float lcx = 210.0f, rcx = 716.0f, cy = 342.0f, R = 185.0f;
    const float aA = 123.6f * DEG2RAD;   /* where the lobe's circle meets the bottom edge's corner */

    /* body: two lobes joined by the band; the bottom edge between them is flat with rounded corners */
    for (int i = 0; i <= 90; i++) {   /* right lobe, from the top clockwise to below */
        float t = -PI * 0.5f + (aA + PI * 0.5f) * (float)i / 90.0f;
        pts[n++] = PadV(v, rcx + cosf(t) * R, cy + sinf(t) * R);
    }
    {
        Vector2 A = { rcx + cosf(aA) * R, cy + sinf(aA) * R }, Pc = { 596.0f, 482.0f }, B = { 574.0f, 482.0f };
        for (int i = 1; i <= 12; i++) {   /* round the corner into the flat bottom */
            float u = (float)i / 12.0f;
            pts[n++] = PadV(v, (1 - u) * (1 - u) * A.x + 2 * u * (1 - u) * Pc.x + u * u * B.x,
                               (1 - u) * (1 - u) * A.y + 2 * u * (1 - u) * Pc.y + u * u * B.y);
        }
        for (int i = 11; i >= 0; i--) {   /* and out of it on the left (the mirror image) */
            float u = (float)i / 12.0f;
            pts[n++] = PadV(v, PadMirrorX((1 - u) * (1 - u) * A.x + 2 * u * (1 - u) * Pc.x + u * u * B.x),
                               (1 - u) * (1 - u) * A.y + 2 * u * (1 - u) * Pc.y + u * u * B.y);
        }
    }
    for (int i = 1; i <= 90; i++) {   /* left lobe, from below up to the top */
        float t0 = PI - aA, t = t0 + (1.5f * PI - t0) * (float)i / 90.0f;
        pts[n++] = PadV(v, lcx + cosf(t) * R, cy + sinf(t) * R);
    }
    PadLoop(pts, n, th, c);

    /* shoulder tabs on the upper edge: a short flat stretch with the curve of the lobe, rounded at both
     * ends, which drop to the body */
    for (int side = 0; side < 2; side++) {
        const float Ro = R + 11.0f, th0 = 245.0f * DEG2RAD, rl = 8.0f, rr = 6.0f;
        Vector2 path[64];
        int m = 0;
        float x0 = lcx + cosf(th0) * Ro, yc = cy + sinf(th0) * Ro, thA = th0 + rl / Ro;
        path[m++] = (Vector2){ x0, PadSnesTopY(x0) };
        m = PadBend(path, m, (Vector2){ x0, yc + rl }, (Vector2){ x0, yc }, (Vector2){ lcx + cosf(thA) * Ro, cy + sinf(thA) * Ro });
        for (int i = 1; i <= 20; i++) {
            float tt = thA + (1.5f * PI - thA) * (float)i / 20.0f;
            path[m++] = (Vector2){ lcx + cosf(tt) * Ro, cy + sinf(tt) * Ro };
        }
        m = PadBend(path, m, (Vector2){ 315.0f - rr, cy - Ro }, (Vector2){ 315.0f, cy - Ro }, (Vector2){ 315.0f, cy - Ro + rr });
        path[m++] = (Vector2){ 315.0f, PadSnesTopY(315.0f) };
        if (side) for (int i = 0; i < m; i++) path[i].x = PadMirrorX(path[i].x);
        PadSnesTab(v, path, m, lit[side ? SNES_BTN_R : SNES_BTN_L], th, c);
    }

    /* ZL / ZR: smaller tabs on the flat top edge between the L and R tabs, inboard of them, rounded the same */
    for (int side = 0; side < 2; side++) {
        const float xa = 351.0f, xb = 413.0f, top = cy - R - 11.0f, edge = cy - R, rz = 6.0f;
        Vector2 path[32];
        int m = 0;
        path[m++] = (Vector2){ xa, edge };
        m = PadBend(path, m, (Vector2){ xa, top + rz }, (Vector2){ xa, top }, (Vector2){ xa + rz, top });
        m = PadBend(path, m, (Vector2){ xb - rz, top }, (Vector2){ xb, top }, (Vector2){ xb, top + rz });
        path[m++] = (Vector2){ xb, edge };
        if (side) for (int i = 0; i < m; i++) path[i].x = PadMirrorX(path[i].x);
        PadSnesTab(v, path, m, lit[side ? SNES_BTN_ZR : SNES_BTN_ZL], th, c);
    }

    /* D-pad: its ring, then the plus-shaped pad with small rounded corners; a rounded triangle in each arm, pointing outward (filled when lit) */
    PadCircle(PadV(v, 210, 342), 105.0f * s, thL, c);
    {
        const float hw = 27.0f, hl = 73.0f, cx = 214.5f, cyy = 342.5f;
        static const float px[12] = { -1, 1, 1, 2, 2, 1, 1, -1, -1, -2, -2, -1 }, py[12] = { -2, -2, -1, -1, 1, 1, 2, 2, 1, 1, -1, -1 };
        float rad[12];
        Vector2 cr[12];
        for (int i = 0; i < 12; i++) {
            float ax = px[i], ay = py[i];
            cr[i] = PadV(v, cx + (fabsf(ax) == 2 ? (ax > 0 ? hl : -hl) : (ax > 0 ? hw : -hw)),
                            cyy + (fabsf(ay) == 2 ? (ay > 0 ? hl : -hl) : (ay > 0 ? hw : -hw)));
            rad[i] = ((fabsf(ax) == 2 || fabsf(ay) == 2) ? 5.0f : 2.5f) * s;
        }
        PadRoundPoly(cr, rad, 12, thM, c);
    }
    for (int d = 0; d < 4; d++) {   /* the direction marks: Up, Down, Left, Right */
        static const int dx[4] = { 0, 0, -1, 1 }, dy[4] = { -1, 1, 0, 0 };
        const float apex = 61.5f, base = 41.5f, half = 12.0f, cx = 214.5f, cyy = 342.5f;
        float ux = (float)dx[d], uy = (float)dy[d];
        Vector2 tri[3] = {
            PadV(v, cx + ux * apex, cyy + uy * apex),
            PadV(v, cx + ux * base - uy * half, cyy + uy * base + ux * half),
            PadV(v, cx + ux * base + uy * half, cyy + uy * base - ux * half) };
        PadRoundPolyG(tri, 3, 3.0f * s, thM, lit[d], c);
    }

    /* Select and Start: slanted pills */
    PadCapsule(PadV(v, 363, 387), PadV(v, 403, 351), 10.0f * s, thM, lit[SNES_BTN_SELECT], c);
    PadCapsule(PadV(v, 460, 387), PadV(v, 500, 351), 10.0f * s, thM, lit[SNES_BTN_START], c);

    /* the face: its ring, two capsule wells (Y with X, B with A), the four buttons with a ring each */
    PadCircle(PadV(v, rcx, cy), 161.5f * s, thL, c);
    {
        Vector2 xc = PadV(v, 716.5f, 277.5f), ac = PadV(v, 792.5f, 341.0f), bc = PadV(v, 716.5f, 405.0f), yc = PadV(v, 640.5f, 341.0f);
        float r = 35.5f * s;
        PadCapsule(yc, xc, 43.0f * s, thL, false, c);
        PadCapsule(bc, ac, 43.0f * s, thL, false, c);
        if (lit[SNES_BTN_X]) DrawCircleV(xc, r, c);
        if (lit[SNES_BTN_A]) DrawCircleV(ac, r, c);
        if (lit[SNES_BTN_B]) DrawCircleV(bc, r, c);
        if (lit[SNES_BTN_Y]) DrawCircleV(yc, r, c);
        PadCircle(xc, r, thM, c);
        PadCircle(ac, r, thM, c);
        PadCircle(bc, r, thM, c);
        PadCircle(yc, r, thM, c);
    }
}

/* ---- N64 ---- */

/* The N64 controller after its reference picture (774 x 723): the body's outline (a smoothed curve through the traced centre line), the two shoulder rims
 * (closed shapes along each top edge), the D-pad with a triangle in each arm, Start, B and A, the C
 * diamond (one circle through the four buttons, each with a triangle), the stick in its octagonal
 * gate, and, apart from the body, Z in its own rectangle (2 : 3) with a leader line to where the trigger
 * sits (behind the middle prong). The outline and the rims are traced from the picture's own lines
 * (centre lines, in the picture's pixels); the rest is placed by measurement. Lit buttons are filled white;
 * the stick's three circles shift together with a tilt. (x, y) is the box's top-left, w its width, th
 * the line weight (the NES controller's). The box is the picture's pixels 10..752 x 10..710. */
#define PADN64_W 742.0f
#define PADN64_H 700.0f

static const Vector2 n64Body[750] = {
    {387.9,707.5}, {383.9,708.1}, {379.9,708.2}, {375.9,707.9}, {372.0,707.2}, {368.2,706.1}, {364.5,704.6}, {360.9,702.7}, {357.6,700.4}, {354.6,697.8}, {351.9,694.9}, {349.3,691.8}, {347.1,688.6}, {344.9,685.2}, {343.0,681.7}, {341.2,678.1}, {339.4,674.5}, {337.8,670.9}, {336.2,667.2}, {334.7,663.5}, {333.2,659.8}, {331.7,656.1}, {330.3,652.3}, {328.9,648.6}, {327.6,644.8}, {326.3,641.0}, {325.0,637.2}, {323.8,633.4}, {322.7,629.6}, {321.5,625.7}, {320.4,621.9}, {319.3,618.0}, {318.2,614.2}, {317.1,610.4}, {316.1,606.5}, {315.0,602.6}, {314.0,598.8}, {313.0,594.9}, {312.1,591.0}, {311.1,587.1}, {310.2,583.2}, {309.2,579.3}, {308.4,575.4}, {307.5,571.5}, {306.7,567.6}, {306.0,563.7}, {305.3,559.8}, {304.6,555.8}, {303.9,551.9}, {303.2,547.9}, {302.5,544.0}, {301.8,540.1}, {301.1,536.1}, {300.4,532.2}, {299.7,528.2}, {299.0,524.3}, {298.3,520.4}, {297.6,516.4}, {296.9,512.5}, {296.2,508.6}, {295.5,504.6}, {294.8,500.7}, {294.1,496.7}, {293.5,492.8}, {292.8,488.8}, {292.1,484.9}, {291.5,481.0}, {290.8,477.0}, {290.1,473.1}, {289.4,469.1}, {288.7,465.2}, {288.1,461.3}, {287.4,457.3}, {286.7,453.4}, {286.1,449.4}, {285.4,445.5}, {284.7,441.5}, {284.0,437.6}, {283.3,433.7}, {282.6,429.7}, {282.0,425.8}, {281.3,421.8}, {280.6,417.9}, {279.8,414.0}, {278.9,410.1}, {277.9,406.2}, {276.6,402.4}, {275.1,398.7}, {273.4,395.1}, {271.5,391.6}, {269.3,388.3}, {266.8,385.1}, {264.1,382.1}, {261.2,379.4}, {258.2,376.8}, {255.0,374.4}, {251.6,372.3}, {248.1,370.3}, {244.5,368.6}, {240.8,367.0}, {237.1,365.7}, {233.2,364.5}, {229.4,363.5}, {225.5,362.6}, {221.6,361.8}, {217.6,361.0}, {213.7,360.4}, {209.7,359.7}, {205.8,359.1}, {201.8,358.5}, {197.9,357.9}, {193.9,357.5}, {189.9,357.1}, {185.9,357.1}, {181.9,357.5}, {178.1,358.4}, {174.4,360.0}, {171.0,362.1}, {168.0,364.7}, {165.3,367.7}, {163.0,371.0}, {161.1,374.5}, {159.5,378.1}, {158.2,381.9}, {157.0,385.7}, {155.8,389.5}, {154.7,393.4}, {153.6,397.2}, {152.4,401.0}, {151.3,404.9}, {150.1,408.7}, {148.9,412.5}, {147.7,416.3}, {146.5,420.1}, {145.3,424.0}, {144.1,427.8}, {142.9,431.6}, {141.8,435.4}, {140.6,439.2}, {139.3,443.0}, {138.1,446.9}, {136.9,450.7}, {135.6,454.5}, {134.4,458.3}, {133.1,462.1}, {131.9,465.9}, {130.6,469.7}, {129.4,473.5}, {128.1,477.3}, {126.9,481.0}, {125.5,484.8}, {124.2,488.6}, {122.8,492.3}, {121.3,496.1}, {119.9,499.8}, {118.4,503.5}, {116.9,507.2}, {115.3,510.9}, {113.7,514.5}, {112.0,518.2}, {110.3,521.8}, {108.5,525.3}, {106.6,528.9}, {104.5,532.3}, {102.3,535.6}, {100.0,538.9}, {97.5,542.0}, {94.7,544.9}, {91.7,547.6}, {88.5,549.9}, {85.1,552.0}, {81.5,553.6}, {77.6,554.8}, {73.7,555.5}, {69.7,555.7}, {65.7,555.4}, {61.8,554.5}, {58.1,553.2}, {54.5,551.4}, {51.2,549.2}, {48.1,546.6}, {45.3,543.7}, {42.9,540.6}, {40.7,537.2}, {38.7,533.8}, {37.0,530.2}, {35.4,526.5}, {33.9,522.8}, {32.6,519.0}, {31.4,515.2}, {30.1,511.4}, {29.0,507.6}, {27.8,503.7}, {26.7,499.9}, {25.7,496.0}, {24.7,492.1}, {23.7,488.3}, {22.9,484.4}, {22.0,480.4}, {21.2,476.5}, {20.3,472.6}, {19.5,468.7}, {18.7,464.8}, {18.0,460.9}, {17.3,456.9}, {16.7,453.0}, {16.1,449.0}, {15.7,445.0}, {15.2,441.1}, {14.8,437.1}, {14.4,433.1}, {14.0,429.1}, {13.6,425.1}, {13.3,421.2}, {12.9,417.2}, {12.6,413.2}, {12.4,409.2}, {12.2,405.2}, {12.1,401.2}, {11.9,397.2}, {11.8,393.2}, {11.7,389.2}, {11.6,385.2}, {11.7,381.2}, {11.7,377.2}, {11.8,373.2}, {11.9,369.2}, {12.0,365.2}, {12.1,361.2}, {12.3,357.2}, {12.5,353.2}, {12.8,349.2}, {13.1,345.3}, {13.5,341.3}, {13.9,337.3}, {14.3,333.3}, {14.8,329.3}, {15.3,325.4}, {15.8,321.4}, {16.3,317.4}, {16.9,313.5}, {17.5,309.5}, {18.1,305.6}, {18.8,301.6}, {19.4,297.7}, {20.1,293.8}, {20.8,289.8}, {21.6,285.9}, {22.4,282.0}, {23.3,278.1}, {24.2,274.2}, {25.0,270.3}, {25.8,266.3}, {26.6,262.4}, {27.4,258.5}, {28.2,254.6}, {29.0,250.7}, {29.7,246.7}, {30.5,242.8}, {31.1,238.9}, {31.7,234.9}, {32.2,230.9}, {32.7,227.0}, {33.2,223.0}, {33.6,219.0}, {34.1,215.1}, {34.6,211.1}, {35.2,207.1}, {35.8,203.2}, {36.5,199.2}, {37.3,195.3}, {38.2,191.4}, {39.1,187.5}, {40.2,183.7}, {41.3,179.8}, {42.6,176.0}, {43.9,172.3}, {45.4,168.5}, {46.9,164.9}, {48.5,161.2}, {50.3,157.6}, {52.1,154.0}, {54.0,150.5}, {56.0,147.1}, {58.1,143.7}, {60.3,140.3}, {62.6,137.0}, {64.9,133.8}, {67.4,130.6}, {69.9,127.5}, {72.4,124.4}, {74.8,121.2}, {76.9,117.8}, {78.5,114.2}, {79.5,110.3}, {80.4,106.4}, {81.4,102.5}, {83.0,98.9}, {85.4,95.7}, {88.3,93.0}, {91.6,90.7}, {95.1,88.6}, {98.6,86.8}, {102.2,85.0}, {105.8,83.4}, {109.5,81.8}, {113.2,80.3}, {117.0,78.9}, {120.7,77.6}, {124.5,76.3}, {128.4,75.1}, {132.2,74.0}, {136.0,72.9}, {139.9,71.9}, {143.8,70.9}, {147.7,70.0}, {151.6,69.1}, {155.5,68.3}, {159.4,67.5}, {163.3,66.7}, {167.3,66.0}, {171.2,65.3}, {175.2,64.7}, {179.1,64.0}, {183.0,63.4}, {187.0,62.7}, {190.9,62.1}, {194.9,61.5}, {198.8,60.8}, {202.8,60.2}, {206.7,59.6}, {210.7,59.0}, {214.7,58.3}, {218.6,57.7}, {222.5,57.0}, {226.5,56.4}, {230.4,55.5}, {234.2,54.4}, {237.8,52.6}, {240.9,50.2}, {243.6,47.2}, {246.2,44.1}, {248.9,41.2}, {252.0,38.6}, {255.2,36.3}, {258.7,34.3}, {262.3,32.5}, {265.9,30.9}, {269.6,29.3}, {273.4,27.9}, {277.1,26.6}, {280.9,25.3}, {284.7,24.1}, {288.6,23.0}, {292.4,22.0}, {296.3,21.0}, {300.2,20.1}, {304.1,19.3}, {308.1,18.6}, {312.0,17.9}, {316.0,17.2}, {319.9,16.6}, {323.9,16.0}, {327.8,15.4}, {331.8,14.8}, {335.7,14.3}, {339.7,13.9}, {343.7,13.5}, {347.7,13.2}, {351.7,12.9}, {355.7,12.7}, {359.7,12.4}, {363.6,12.2}, {367.6,12.1}, {371.6,12.0}, {375.6,12.0}, {379.6,12.0}, {383.6,12.1}, {387.6,12.2}, {391.6,12.4}, {395.6,12.6}, {399.6,12.8}, {403.6,12.9}, {407.6,13.1}, {411.6,13.3}, {415.6,13.5}, {419.6,13.9}, {423.6,14.3}, {427.5,14.8}, {431.5,15.3}, {435.5,15.7}, {439.4,16.2}, {443.4,16.8}, {447.4,17.3}, {451.3,17.9}, {455.3,18.5}, {459.2,19.2}, {463.2,19.9}, {467.1,20.7}, {471.0,21.5}, {474.9,22.4}, {478.8,23.4}, {482.6,24.4}, {486.5,25.6}, {490.3,26.7}, {494.1,28.0}, {497.9,29.3}, {501.6,30.6}, {505.4,32.0}, {509.1,33.6}, {512.7,35.3}, {516.2,37.2}, {519.5,39.4}, {522.6,42.0}, {525.3,44.9}, {528.1,47.8}, {531.0,50.6}, {534.2,52.9}, {537.8,54.6}, {541.6,55.8}, {545.5,56.8}, {549.4,57.6}, {553.4,58.3}, {557.3,59.0}, {561.2,59.7}, {565.2,60.4}, {569.1,61.1}, {573.1,61.8}, {577.0,62.4}, {581.0,63.1}, {584.9,63.8}, {588.8,64.6}, {592.8,65.3}, {596.7,66.0}, {600.6,66.7}, {604.6,67.5}, {608.5,68.2}, {612.4,69.0}, {616.3,69.8}, {620.2,70.7}, {624.1,71.6}, {628.0,72.5}, {631.9,73.5}, {635.7,74.6}, {639.6,75.7}, {643.4,76.9}, {647.1,78.3}, {650.9,79.7}, {654.6,81.1}, {658.3,82.7}, {661.9,84.3}, {665.5,86.1}, {669.0,88.0}, {672.5,90.1}, {675.7,92.4}, {678.8,95.0}, {681.3,98.0}, {683.2,101.6}, {684.3,105.4}, {685.0,109.4}, {686.1,113.2}, {687.7,116.9}, {689.8,120.2}, {692.3,123.4}, {694.9,126.4}, {697.6,129.4}, {700.1,132.5}, {702.6,135.6}, {704.9,138.9}, {707.2,142.2}, {709.3,145.6}, {711.4,149.0}, {713.4,152.5}, {715.2,156.0}, {717.0,159.6}, {718.6,163.3}, {720.1,167.0}, {721.5,170.7}, {722.8,174.5}, {724.1,178.3}, {725.2,182.1}, {726.3,186.0}, {727.3,189.8}, {728.2,193.7}, {729.1,197.6}, {729.9,201.5}, {730.6,205.5}, {731.2,209.4}, {731.8,213.4}, {732.3,217.4}, {732.8,221.3}, {733.2,225.3}, {733.7,229.3}, {734.1,233.3}, {734.5,237.2}, {735.0,241.2}, {735.4,245.2}, {735.9,249.1}, {736.5,253.1}, {737.1,257.1}, {737.7,261.0}, {738.4,265.0}, {739.0,268.9}, {739.7,272.8}, {740.4,276.8}, {741.1,280.7}, {741.8,284.7}, {742.5,288.6}, {743.1,292.5}, {743.8,296.5}, {744.4,300.4}, {744.9,304.4}, {745.5,308.4}, {746.0,312.3}, {746.5,316.3}, {746.9,320.3}, {747.3,324.3}, {747.6,328.2}, {748.0,332.2}, {748.4,336.2}, {748.7,340.2}, {749.1,344.2}, {749.4,348.2}, {749.6,352.2}, {749.9,356.2}, {750.1,360.1}, {750.2,364.1}, {750.4,368.1}, {750.5,372.1}, {750.5,376.1}, {750.4,380.1}, {750.3,384.1}, {750.3,388.1}, {750.3,392.1}, {750.4,396.1}, {750.5,400.1}, {750.4,404.1}, {750.2,408.1}, {750.0,412.1}, {749.6,416.1}, {749.2,420.1}, {748.8,424.1}, {748.4,428.0}, {748.0,432.0}, {747.4,436.0}, {746.9,439.9}, {746.3,443.9}, {745.7,447.8}, {745.1,451.8}, {744.4,455.7}, {743.8,459.7}, {743.1,463.6}, {742.4,467.6}, {741.6,471.5}, {740.9,475.4}, {740.1,479.3}, {739.2,483.2}, {738.4,487.1}, {737.4,491.0}, {736.5,494.9}, {735.5,498.8}, {734.5,502.7}, {733.4,506.5}, {732.2,510.4}, {731.0,514.2}, {729.7,517.9}, {728.4,521.7}, {726.9,525.4}, {725.3,529.1}, {723.5,532.7}, {721.6,536.2}, {719.5,539.6}, {717.1,542.8}, {714.5,545.8}, {711.5,548.5}, {708.3,550.9}, {704.8,552.8}, {701.2,554.4}, {697.3,555.6}, {693.4,556.2}, {689.4,556.4}, {685.4,556.1}, {681.5,555.3}, {677.7,554.0}, {674.1,552.3}, {670.7,550.2}, {667.5,547.8}, {664.6,545.0}, {662.0,542.0}, {659.6,538.8}, {657.4,535.5}, {655.3,532.1}, {653.4,528.5}, {651.6,525.0}, {650.0,521.3}, {648.3,517.7}, {646.7,514.0}, {645.2,510.3}, {643.6,506.6}, {642.1,502.9}, {640.7,499.2}, {639.4,495.4}, {638.0,491.7}, {636.7,487.9}, {635.3,484.1}, {634.0,480.4}, {632.6,476.6}, {631.3,472.8}, {630.0,469.1}, {628.7,465.3}, {627.5,461.5}, {626.2,457.7}, {625.0,453.8}, {623.8,450.0}, {622.6,446.2}, {621.4,442.4}, {620.2,438.6}, {619.1,434.8}, {617.9,430.9}, {616.7,427.1}, {615.5,423.3}, {614.3,419.5}, {613.2,415.6}, {612.1,411.8}, {611.0,407.9}, {609.9,404.1}, {608.8,400.2}, {607.8,396.4}, {606.8,392.5}, {605.7,388.7}, {604.6,384.8}, {603.3,381.0}, {601.9,377.3}, {600.1,373.7}, {598.0,370.3}, {595.6,367.1}, {592.8,364.3}, {589.6,361.9}, {586.1,360.0}, {582.3,358.7}, {578.4,357.9}, {574.4,357.7}, {570.4,357.7}, {566.4,358.1}, {562.4,358.5}, {558.5,359.1}, {554.5,359.7}, {550.6,360.3}, {546.6,360.9}, {542.7,361.6}, {538.8,362.4}, {534.8,363.2}, {530.9,364.1}, {527.1,365.1}, {523.2,366.2}, {519.5,367.5}, {515.8,369.1}, {512.2,370.8}, {508.7,372.9}, {505.5,375.2}, {502.4,377.7}, {499.5,380.4}, {496.7,383.4}, {494.2,386.5}, {491.9,389.7}, {489.9,393.2}, {488.0,396.7}, {486.4,400.4}, {485.0,404.1}, {483.9,408.0}, {482.8,411.8}, {481.9,415.7}, {481.1,419.6}, {480.3,423.5}, {479.5,427.5}, {478.7,431.4}, {477.8,435.3}, {477.0,439.2}, {476.3,443.1}, {475.5,447.1}, {474.8,451.0}, {474.1,454.9}, {473.4,458.9}, {472.7,462.8}, {472.0,466.8}, {471.4,470.7}, {470.7,474.6}, {470.0,478.6}, {469.3,482.5}, {468.6,486.5}, {468.0,490.4}, {467.3,494.3}, {466.6,498.3}, {465.9,502.2}, {465.2,506.2}, {464.6,510.1}, {463.9,514.1}, {463.3,518.0}, {462.7,522.0}, {462.0,525.9}, {461.4,529.9}, {460.7,533.8}, {460.0,537.7}, {459.3,541.7}, {458.6,545.6}, {457.9,549.5}, {457.1,553.5}, {456.4,557.4}, {455.7,561.3}, {454.9,565.3}, {454.2,569.2}, {453.4,573.1}, {452.6,577.0}, {451.8,580.9}, {450.9,584.9}, {450.2,588.8}, {449.4,592.7}, {448.6,596.6}, {447.8,600.5}, {446.9,604.4}, {446.0,608.3}, {445.0,612.2}, {444.0,616.1}, {443.0,620.0}, {442.0,623.8}, {440.9,627.7}, {439.8,631.5}, {438.6,635.3}, {437.4,639.1}, {436.1,642.9}, {434.8,646.7}, {433.4,650.5}, {432.0,654.2}, {430.5,657.9}, {428.9,661.6}, {427.3,665.2}, {425.5,668.8}, {423.7,672.4}, {421.9,675.9}, {419.9,679.4}, {417.9,682.9}, {415.7,686.2}, {413.4,689.5}, {411.0,692.7}, {408.3,695.7}, {405.4,698.4}, {402.3,700.9}, {399.0,703.2}, {395.5,705.0}, {391.7,706.5} };
static const Vector2 n64RimLA[24] = {
    {86.7,94.4}, {91.2,90.9}, {97.2,87.5}, {104.3,84.1}, {110.7,81.3}, {117.2,78.9}, {124.7,76.3}, {131.3,74.2}, {137.9,72.4}, {144.6,70.7}, {151.3,69.2}, {158.1,67.8}, {164.8,66.5}, {171.6,65.3}, {178.4,64.1}, {185.1,63.0}, {192.9,61.8}, {199.7,60.7}, {206.5,59.6}, {213.3,58.6}, {220.1,57.5}, {225.9,56.5}, {232.4,55.0}, {237.3,52.9} };
static const Vector2 n64RimLB[24] = {
    {83.0,103.1}, {89.8,101.1}, {96.7,98.5}, {103.5,95.5}, {110.3,92.5}, {117.1,89.6}, {124.0,87.0}, {130.8,84.6}, {137.6,82.5}, {144.4,80.5}, {151.3,78.8}, {158.1,77.3}, {164.9,75.9}, {171.7,74.7}, {178.6,73.5}, {185.4,72.4}, {192.2,71.3}, {199.0,70.2}, {205.9,69.1}, {212.7,68.0}, {219.5,66.7}, {226.3,65.2}, {233.2,63.6}, {240.0,62.2} };
static const Vector2 n64RimRA[24] = {
    {535.5,53.5}, {541.6,55.8}, {548.3,57.4}, {554.1,58.4}, {560.8,59.7}, {567.6,60.8}, {574.4,62.0}, {580.2,63.0}, {587.0,64.2}, {593.7,65.5}, {599.5,66.5}, {606.3,67.8}, {613.0,69.1}, {619.7,70.5}, {626.4,72.1}, {632.1,73.6}, {638.8,75.5}, {645.3,77.6}, {651.8,80.0}, {658.2,82.6}, {665.4,86.0}, {671.4,89.4}, {677.6,93.9}, {681.3,97.9} };
static const Vector2 n64RimRB[24] = {
    {535.0,65.3}, {541.5,66.1}, {548.0,67.1}, {554.6,68.2}, {561.1,69.3}, {567.6,70.3}, {574.1,71.4}, {580.7,72.5}, {587.2,73.6}, {593.7,74.8}, {600.2,76.0}, {606.7,77.3}, {613.3,78.7}, {619.8,80.2}, {626.3,81.9}, {632.8,83.8}, {639.3,85.9}, {645.9,88.2}, {652.4,90.8}, {658.9,93.5}, {665.4,96.0}, {672.0,98.1}, {678.5,99.4}, {685.0,99.9} };

/* A rounded triangle pointing along (ux, uy) from (cx, cy): apex at distance ap, base at ba, half-width hf
 * (reference units), outline th, filled when on. */
static void PadN64Tri(PadView v, float cx, float cy, float ux, float uy, float ap, float ba, float hf, float rad, float th, bool on, Color c) {
    Vector2 tri[3] = {
        PadV(v, cx + ux * ap, cy + uy * ap),
        PadV(v, cx + ux * ba - uy * hf, cy + uy * ba + ux * hf),
        PadV(v, cx + ux * ba + uy * hf, cy + uy * ba - ux * hf) };
    PadRoundPolyG(tri, 3, rad * v.s, th, on, c);
}

/* A shoulder rim, a closed shape: its upper line is the body's own outline (chain a), its lower line is chain b,
 * joined by a short cap at each end (n points per chain, in step); filled between the two lines when lit. */
static void PadN64Rim(PadView v, const Vector2 *a, const Vector2 *b, int n, bool lit, float th, Color c) {
    Vector2 sa[32], sb[32];
    for (int i = 0; i < n; i++) {
        sa[i] = PadV(v, a[i].x, a[i].y);
        sb[i] = PadV(v, b[i].x, b[i].y);
    }
    if (lit)
        for (int i = 0; i + 1 < n; i++) {
            PadTri(sa[i], sa[i + 1], sb[i + 1], c);
            PadTri(sa[i], sb[i + 1], sb[i], c);
        }
    PadPoly(sb, n, th, c);   /* the upper line is the body's outline, already drawn */
    DrawLineEx(sa[0], sb[0], th, c);
    DrawLineEx(sa[n - 1], sb[n - 1], th, c);
    DrawCircleV(sa[0], th * 0.5f, c);
    DrawCircleV(sa[n - 1], th * 0.5f, c);
}

static void DrawN64Controller(float x, float y, float w, float th, const bool lit[PAD_MAX_BTN], Color c) {
    float s = w / PADN64_W;
    PadView v = { x + 60.0f * s, y + 155.0f * s, s };   /* PadV(rx, ry) = (x + (rx - 10) s, y + (ry - 10) s) */
    float thM = th * 0.8f, thL = fmaxf(1.0f, th * 0.55f);
    enum { BN = (int)(sizeof(n64Body) / sizeof(n64Body[0])) };
    Vector2 loop[BN];

    for (int i = 0; i < BN; i++) loop[i] = PadV(v, n64Body[i].x, n64Body[i].y);
    PadLoop(loop, BN, th, c);
    PadN64Rim(v, n64RimLA, n64RimLB, 24, lit[N64_BTN_L], th, c);
    PadN64Rim(v, n64RimRA, n64RimRB, 24, lit[N64_BTN_R], th, c);

    /* D-pad: the plus, a triangle in each arm pointing outward */
    {
        const float cx = 158.5f, cy = 233.0f, hw = 21.0f, hl = 60.0f;
        static const float px[12] = { -1, 1, 1, 2, 2, 1, 1, -1, -1, -2, -2, -1 }, py[12] = { -2, -2, -1, -1, 1, 1, 2, 2, 1, 1, -1, -1 };
        float rad[12];
        Vector2 cr[12];
        static const int ddx[4] = { 0, 0, -1, 1 }, ddy[4] = { -1, 1, 0, 0 };
        for (int i = 0; i < 12; i++) {
            float ax = px[i], ay = py[i];
            cr[i] = PadV(v, cx + (fabsf(ax) == 2 ? (ax > 0 ? hl : -hl) : (ax > 0 ? hw : -hw)),
                            cy + (fabsf(ay) == 2 ? (ay > 0 ? hl : -hl) : (ay > 0 ? hw : -hw)));
            rad[i] = ((fabsf(ax) == 2 || fabsf(ay) == 2) ? 7.0f : 3.0f) * s;
        }
        PadRoundPoly(cr, rad, 12, thM, c);
        PadCircle(PadV(v, cx, cy), 9.0f * s, thL, c);   /* the small empty circle in the middle */
        for (int d = 0; d < 4; d++)
            PadN64Tri(v, cx, cy, (float)ddx[d], (float)ddy[d], 46.0f, 30.0f, 9.5f, 2.5f, thL, lit[N64_BTN_UP + d], c);
    }

    /* Start, B and A */
    {
        Vector2 sc = PadV(v, 383.0f, 254.5f), bc = PadV(v, 514.0f, 253.0f), ac = PadV(v, 571.0f, 303.5f);
        float r = 30.0f * s;
        if (lit[N64_BTN_START]) DrawCircleV(sc, r, c);
        if (lit[N64_BTN_B]) DrawCircleV(bc, r, c);
        if (lit[N64_BTN_A]) DrawCircleV(ac, r, c);
        PadCircle(sc, r, thM, c);
        PadCircle(bc, r, thM, c);
        PadCircle(ac, r, thM, c);
    }

    /* C buttons: one circle through the middle of the four buttons (hidden behind them), each button with a triangle */
    {
        const float cx = 626.5f, cy = 208.5f, hx = 45.5f, hy = 42.5f, r = 22.0f, rr = 54.5f;
        static const int cdx[4] = { 0, 0, -1, 1 }, cdy[4] = { -1, 1, 0, 0 };
        Vector2 run[130];
        int m = 0;
        for (int i = 0; i <= 360; i++) {
            float a = (float)i * DEG2RAD, px = cx + cosf(a) * rr, py = cy + sinf(a) * rr;
            bool hidden = false;
            for (int d = 0; d < 4; d++) {
                float ddx = px - (cx + (float)cdx[d] * hx), ddy = py - (cy + (float)cdy[d] * hy);
                if (ddx * ddx + ddy * ddy < r * r) hidden = true;
            }
            if (!hidden) {
                if (m < 130) run[m++] = PadV(v, px, py);
            } else if (m > 0) {
                PadPoly(run, m, thM, c);
                m = 0;
            }
        }
        if (m > 0) PadPoly(run, m, thM, c);
        for (int d = 0; d < 4; d++) {
            float ux = (float)cdx[d], uy = (float)cdy[d];
            float ccx = cx + ux * hx, ccy = cy + uy * hy;
            Vector2 cc = PadV(v, ccx, ccy);
            if (lit[N64_BTN_CUP + d]) DrawCircleV(cc, r * s, c);
            PadCircle(cc, r * s, thM, c);
            if (!lit[N64_BTN_CUP + d]) PadN64Tri(v, ccx, ccy, ux, uy, 10.0f, -8.5f, 9.5f, 2.5f, thL, false, c);
        }
    }

    /* the stick: a ring, the octagonal gate (both fixed) and three circles that move together with the tilt */
    {
        const float cx = 382.0f, cy = 399.5f;
        Vector2 oct[8];
        float tx = (float)(lit[N64_BTN_SRIGHT] - lit[N64_BTN_SLEFT]), ty = (float)(lit[N64_BTN_SDOWN] - lit[N64_BTN_SUP]);
        float tl = hypotf(tx, ty), amt = 10.0f;
        Vector2 mc = PadV(v, cx, cy);
        PadCircle(mc, 73.0f * s, thM, c);
        for (int i = 0; i < 8; i++) {
            float a = 45.0f * (float)i * DEG2RAD;
            oct[i] = PadV(v, cx + cosf(a) * 47.0f, cy + sinf(a) * 47.0f);
        }
        PadRoundPolyG(oct, 8, 4.0f * s, thM, false, c);
        if (tl > 0.0f) { mc.x += tx / tl * amt * s; mc.y += ty / tl * amt * s; }
        PadCircle(mc, 34.0f * s, thL, c);
        PadCircle(mc, 22.5f * s, thL, c);
        if (tl > 0.0f) DrawCircleV(mc, 12.0f * s, c);
        PadCircle(mc, 12.0f * s, thM, c);
    }

    /* Z: its own empty rectangle (2 : 3 before it was narrowed) beside the middle prong; filled when pressed */
    {
        const float x0 = 531.0f, y0 = 588.0f, x1 = 589.0f, y1 = 693.0f;
        Rectangle zr = PadR(v, x0, y0, x1, y1);
        float rr = 2.0f * 8.0f * s / fminf(zr.width, zr.height);
        if (lit[N64_BTN_Z]) DrawRectangleRounded(zr, rr, 24, c);
        PadRRect(v, x0, y0, x1, y1, 8.0f, thM, c);
    }
}

/* The prompt's name of the step-th button of the open system. */
static StrId PadUi_Label(int step) {
    static const StrId nes[NES_BTN_COUNT] = { S_PC_UP, S_PC_DOWN, S_PC_LEFT, S_PC_RIGHT, S_PC_SELECT, S_PC_START, S_PC_B, S_PC_A };
    static const StrId snes[SNES_BTN_COUNT] = { S_PC_UP, S_PC_DOWN, S_PC_LEFT, S_PC_RIGHT, S_PC_SELECT, S_PC_START,
                                                S_PC_Y, S_PC_B, S_PC_X, S_PC_A, S_PC_L, S_PC_R, S_PC_ZL, S_PC_ZR };
    static const StrId n64[N64_BTN_COUNT] = { S_PC_DP_UP, S_PC_DP_DOWN, S_PC_DP_LEFT, S_PC_DP_RIGHT, S_PC_ST_UP, S_PC_ST_DOWN,
                                              S_PC_ST_LEFT, S_PC_ST_RIGHT, S_PC_START, S_PC_A, S_PC_B, S_PC_Z, S_PC_L, S_PC_R,
                                              S_PC_C_UP, S_PC_C_DOWN, S_PC_C_LEFT, S_PC_C_RIGHT };
    return padSys == &PADSYS_N64 ? n64[step] : padSys == &PADSYS_SNES ? snes[step] : nes[step];
}

/* The drawing is rendered PADUI_SS times larger into a transparent texture, mipmapped and drawn down
 * with trilinear filtering, like the sidebar icons: smooth edges on any background. It is rendered
 * again only when the set of lit buttons (or the size, or the system) changes. */
#define PADUI_SS 4
#define PADUI_MARGIN 6.0f   /* logical pixels round the drawing for the weight of its lines */
static RenderTexture2D padRT;
static int padRTW = 0, padRTH = 0;
static unsigned padRTKey = 0xffffffffu;
static bool padLit[PAD_MAX_BTN];

/* Where the drawing goes: its width, the box's height, the left edge and the centre line. */
static void PadUi_Geom(float *bw, float *boxH, float *bx, float *cy) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float full = sw * 0.387f;   /* the NES / SNES width; the N64's box is narrower (it is tall), same centre */
    *bw = padSys == &PADSYS_N64 ? sw * 0.30f : full;
    *boxH = padSys == &PADSYS_N64 ? PADN64_H * *bw / PADN64_W : padSys == &PADSYS_SNES ? PADSNES_H * *bw / PADSNES_W : 149.0f * *bw / 340.0f;
    *bx = sw * 0.485f + (full - *bw) * 0.5f;
    *cy = sh * PADUI_PAGE_Y + sh * 0.5f;   /* the controller sits in the middle of the page */
}

/* Before the page is drawn (outside its translation): which buttons are lit, and the texture. */
static void PadUi_Prepare(void) {
    float bw, boxH, bx, cy;
    bool snes;
    unsigned key = 0;
    int W, H;
    if (!padOpen) {   /* the page is leaving: it keeps showing the current system's pad, nothing lit */
        PadUi_Select(SysIsN64(currentSystem) ? 2 : SysIsSnes(currentSystem) ? 1 : 0);
    }
    snes = padSys == &PADSYS_SNES;
    PadUi_Geom(&bw, &boxH, &bx, &cy);
    for (int i = 0; i < PAD_MAX_BTN; i++) padLit[i] = false;
    if (padOpen && padListen) padLit[padStep] = ((int)(GetTime() * PADUI_BLINK_HZ * 2.0) & 1) == 0;   /* blinks */
    else for (int i = 0; i < padSys->count; i++) padLit[i] = padOpen && PadUi_Lit(i);
    /* Only a button's own bindings light it. The controls that hold to remap / go back (the pad's A / B,
     * Enter, the A / B keys) are NSO's, not the game's: with a remapped pad they are other buttons' controls. */
    for (int i = 0; i < PAD_MAX_BTN; i++) if (padLit[i]) key |= 1u << i;
    key |= (unsigned)(padSys == &PADSYS_N64 ? 2 : snes ? 1 : 0) << 20;
    W = (int)ceilf(bw) + 2 * (int)PADUI_MARGIN;
    H = (int)ceilf(boxH) + 2 * (int)PADUI_MARGIN;
    if (!padRT.id || W != padRTW || H != padRTH) {
        if (padRT.id) UnloadRenderTexture(padRT);
        padRT = LoadRenderTexture(W * PADUI_SS, H * PADUI_SS);
        padRTW = W;
        padRTH = H;
        padRTKey = 0xffffffffu;
    }
    if (key == padRTKey) return;
    padRTKey = key;
    BeginTextureMode(padRT);
    ClearBackground((Color){ 255, 255, 255, 0 });   /* same RGB: no dark fringe when filtered */
    if (padSys == &PADSYS_N64)   /* the same line weight as the NES controller's */
        DrawN64Controller(PADUI_MARGIN * PADUI_SS, PADUI_MARGIN * PADUI_SS, bw * PADUI_SS,
                          fmaxf(1.5f, 1.3f * (float)GetScreenWidth() * 0.387f * PADUI_SS / 340.0f), padLit, WHITE);
    else if (snes) DrawSnesController(PADUI_MARGIN * PADUI_SS, PADUI_MARGIN * PADUI_SS, bw * PADUI_SS, padLit, WHITE);
    else {   /* the NES body's top-left is the reference (71, 166), the origin (70, 165) */
        float sc = bw * PADUI_SS / 340.0f;
        DrawNesController(PADUI_MARGIN * PADUI_SS - sc, PADUI_MARGIN * PADUI_SS - sc, bw * PADUI_SS, padLit, WHITE);
    }
    EndTextureMode();
    GenTextureMipmaps(&padRT.texture);
    SetTextureFilter(padRT.texture, TEXTURE_FILTER_TRILINEAR);
}

static void PadUi_Draw(Font font, Color bg) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float hx = sw * 0.449f, bw, boxH, bx, cy;
    float oy = sh * PADUI_PAGE_Y;
    (void)bg;
    PadUi_Geom(&bw, &boxH, &bx, &cy);
    DrawFitText(font, T(padSys == &PADSYS_N64 ? S_PC_ENTRY_N64 : padSys == &PADSYS_SNES ? S_PC_ENTRY_SNES : S_PC_ENTRY), (Vector2){ hx, oy + sh * 0.092f }, (float)font.baseSize * 0.85f, sw - hx, WHITE);
    if (padRT.id) {
        float ts = (float)padRT.texture.width, th = (float)padRT.texture.height;
        DrawTexturePro(padRT.texture, (Rectangle){ 0, 0, ts, -th },   /* RT is upside down */
                       (Rectangle){ roundf(bx - PADUI_MARGIN), roundf(cy - boxH * 0.5f - PADUI_MARGIN), (float)padRTW, (float)padRTH },
                       (Vector2){ 0 }, 0.0f, WHITE);
    }
    if (padOpen && padListen) {   /* the one prompt under the controller: "Press: A", and the skip cue */
        char line[96];
        float y = cy + boxH * 0.5f + sh * 0.07f;
        float size = (float)font.baseSize * 1.5f;
        snprintf(line, sizeof(line), "%s: %s", T(S_PC_PRESS), T(PadUi_Label(padStep)));
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
