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

/* The N64 controller after its reference picture (774 x 723): the body's outline, the two shoulder rims
 * (the double line along each top edge), the D-pad with a triangle in each arm, Start, B and A, the C
 * diamond (a rounded diamond tangent to four circles, each with a triangle), the stick in its octagonal
 * gate, and, apart from the body, Z in its own rectangle (2 : 3) with a leader line to where the trigger
 * sits (behind the middle prong). The outline and the rims are traced from the picture's own lines
 * (centre lines, in the picture's pixels); the rest is placed by measurement. Lit buttons are filled white;
 * the stick's three circles shift together with a tilt. (x, y) is the box's top-left, w its width, th
 * the line weight (the NES controller's). The box is the picture's pixels 10..752 x 10..710. */
#define PADN64_W 742.0f
#define PADN64_H 700.0f

static const Vector2 n64Body[178] = {
    {383.0,709.0}, {376.0,708.7}, {367.2,706.6}, {359.7,702.9}, {352.6,696.9}, {345.6,686.9}, {342.6,681.7}, {331.1,654.9}, {325.4,639.0}, {314.2,600.0}, {308.3,576.0}, {294.3,499.0}, {293.7,493.0}, {279.4,410.0}, {275.4,398.3}, {271.7,390.8}, {268.5,386.0}, {261.0,378.5}, {255.4,374.0}, {249.8,370.6}, {241.5,366.9}, {235.0,364.6}, {221.0,361.4}, {184.0,356.1}, {180.0,356.6}, {175.2,358.3}, {169.5,362.0}, {163.6,368.2}, {158.6,379.3}, {154.8,393.9}, {141.2,438.0}, {127.5,479.8}, {115.6,510.8}, {108.9,525.4}, {102.2,536.7}, {98.8,541.3}, {90.9,549.4}, {84.6,553.2}, {79.6,555.2}, {70.0,556.9}, {59.2,554.6}, {52.9,551.5}, {49.4,549.1}, {43.6,542.9}, {40.8,538.7}, {33.5,522.8}, {24.0,490.0}, {16.3,452.0}, {12.1,409.0}, {12.0,391.0}, {11.1,384.0}, {12.0,376.0}, {12.1,356.0}, {14.2,332.0}, {15.8,323.0}, {16.3,316.0}, {21.1,288.0}, {31.6,238.0}, {34.3,212.0}, {36.3,199.0}, {38.5,189.0}, {44.8,169.1}, {53.8,150.3}, {66.6,131.1}, {71.6,125.1}, {79.3,117.6}, {80.0,114.0}, {80.2,101.0}, {80.9,98.4}, {82.1,96.6}, {90.3,90.8}, {103.6,84.1}, {113.1,80.1}, {134.0,73.2}, {150.0,69.3}, {171.0,65.3}, {238.8,54.6}, {241.3,52.6}, {245.6,43.2}, {250.7,38.2}, {265.4,30.8}, {285.0,23.8}, {308.0,18.5}, {334.0,14.3}, {362.0,12.0}, {389.0,12.0}, {397.0,13.0}, {416.0,13.2}, {455.0,18.3}, {475.0,22.2}, {487.0,25.5}, {500.9,30.1}, {512.6,34.8}, {520.1,38.5}, {522.4,40.1}, {524.8,42.7}, {529.1,51.3}, {531.1,52.8}, {536.1,55.1}, {619.9,70.3}, {640.0,75.5}, {663.1,84.4}, {667.7,86.8}, {682.4,96.1}, {684.1,98.4}, {684.8,101.0}, {684.9,107.0}, {684.2,113.0}, {684.7,115.6}, {686.5,118.0}, {691.9,122.6}, {703.4,136.1}, {714.2,153.3}, {717.2,159.4}, {721.9,171.1}, {727.5,190.0}, {730.7,205.0}, {732.9,220.0}, {736.2,252.0}, {739.4,272.0}, {741.7,283.0}, {745.9,310.0}, {749.9,352.0}, {750.0,365.0}, {751.0,373.0}, {750.0,387.0}, {750.9,404.0}, {747.9,434.0}, {742.5,468.0}, {738.5,487.0}, {732.9,509.0}, {727.5,524.8}, {724.5,531.8}, {716.9,544.4}, {711.9,549.4}, {705.7,553.2}, {698.0,556.5}, {692.0,557.0}, {682.0,556.5}, {677.6,554.9}, {669.7,550.8}, {662.6,543.9}, {659.2,539.3}, {655.1,532.4}, {643.4,506.8}, {631.1,472.9}, {615.2,422.9}, {602.4,377.3}, {600.4,372.3}, {598.4,369.1}, {591.1,361.4}, {588.6,359.6}, {586.0,358.6}, {580.0,357.2}, {571.0,357.2}, {544.0,361.2}, {531.0,363.8}, {519.1,367.2}, {511.8,370.3}, {503.5,376.0}, {495.0,384.5}, {492.0,388.5}, {488.3,395.1}, {485.2,402.1}, {483.8,407.0}, {476.3,442.0}, {468.3,487.0}, {458.7,546.0}, {448.7,597.0}, {441.8,625.0}, {434.9,646.9}, {429.2,661.6}, {422.5,675.1}, {416.4,685.9}, {407.8,697.3}, {401.7,702.2}, {394.1,706.6}, {390.0,707.9} };
static const Vector2 n64RimLA[16] = {
    {79.3,108.9}, {79.8,96.9}, {89.4,89.8}, {100.3,84.4}, {111.3,79.4}, {122.7,75.3}, {134.3,71.8}, {145.9,68.6}, {157.8,66.2}, {169.7,64.0}, {181.6,62.1}, {193.6,60.2}, {205.5,58.1}, {217.4,56.2}, {229.4,54.8}, {239.7,58.1} };
static const Vector2 n64RimLB[16] = {
    {79.3,108.9}, {89.1,111.6}, {99.3,105.4}, {109.7,99.8}, {120.4,94.7}, {131.7,90.9}, {143.0,87.2}, {154.6,84.6}, {166.2,82.0}, {177.8,79.9}, {189.6,78.0}, {201.3,76.4}, {213.1,75.0}, {224.8,73.5}, {235.2,68.4}, {239.7,58.1} };
static const Vector2 n64RimRA[16] = {
    {531.3,59.1}, {541.0,55.1}, {552.4,57.0}, {563.7,58.9}, {575.0,60.9}, {586.4,62.6}, {597.8,64.5}, {609.0,67.0}, {620.3,69.1}, {631.5,71.9}, {642.5,75.2}, {653.4,78.9}, {663.8,84.0}, {674.0,89.1}, {683.4,95.8}, {684.7,106.9} };
static const Vector2 n64RimRB[16] = {
    {531.3,59.1}, {535.0,69.4}, {545.2,73.7}, {556.4,75.2}, {567.7,77.0}, {578.8,78.8}, {590.1,80.7}, {601.2,82.7}, {612.4,85.0}, {623.5,87.4}, {634.4,90.6}, {645.2,94.2}, {655.6,98.7}, {665.5,104.2}, {675.3,110.0}, {684.7,106.9} };

/* A rounded triangle pointing along (ux, uy) from (cx, cy): apex at distance ap, base at ba, half-width hf
 * (reference units), outline th, filled when on. */
static void PadN64Tri(PadView v, float cx, float cy, float ux, float uy, float ap, float ba, float hf, float rad, float th, bool on, Color c) {
    Vector2 tri[3] = {
        PadV(v, cx + ux * ap, cy + uy * ap),
        PadV(v, cx + ux * ba - uy * hf, cy + uy * ba + ux * hf),
        PadV(v, cx + ux * ba + uy * hf, cy + uy * ba - ux * hf) };
    PadRoundPolyG(tri, 3, rad * v.s, th, on, c);
}

/* A shoulder rim: the sliver between the two lines along the shoulder, as an upper chain a (the body's own
 * outline) and a lower chain b (n points each, sharing their ends); b is drawn, and filled up to a when lit. */
static void PadN64Rim(PadView v, const Vector2 *a, const Vector2 *b, int n, bool lit, float th, Color c) {
    Vector2 sa[16], sb[16];
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
}

static void DrawN64Controller(float x, float y, float w, float th, const bool lit[PAD_MAX_BTN], Color c) {
    float s = w / PADN64_W;
    PadView v = { x + 60.0f * s, y + 155.0f * s, s };   /* PadV(rx, ry) = (x + (rx - 10) s, y + (ry - 10) s) */
    float thM = th * 0.8f, thL = fmaxf(1.0f, th * 0.55f);
    enum { BN = (int)(sizeof(n64Body) / sizeof(n64Body[0])) };
    Vector2 loop[BN];

    for (int i = 0; i < BN; i++) loop[i] = PadV(v, n64Body[i].x, n64Body[i].y);
    PadLoop(loop, BN, th, c);
    PadN64Rim(v, n64RimLA, n64RimLB, 16, lit[N64_BTN_L], th, c);
    PadN64Rim(v, n64RimRA, n64RimRB, 16, lit[N64_BTN_R], th, c);

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

    /* C buttons: a rounded diamond tangent to four circles, each with a triangle */
    {
        const float cx = 626.5f, cy = 208.5f, hx = 45.5f, hy = 42.5f, r = 22.0f;
        static const int cdx[4] = { 0, 0, -1, 1 }, cdy[4] = { -1, 1, 0, 0 };
        /* the four sides: up -> right -> down -> left, each offset r outward from the line between the circles */
        static const int ord[4] = { 0, 3, 1, 2 };
        for (int i = 0; i < 4; i++) {
            int a = ord[i], b = ord[(i + 1) % 4];
            float ax = cx + (float)cdx[a] * hx, ay = cy + (float)cdy[a] * hy, bx = cx + (float)cdx[b] * hx, by = cy + (float)cdy[b] * hy;
            float dx = bx - ax, dy = by - ay, l = hypotf(dx, dy), nx = dy / l, ny = -dx / l;   /* outward normal */
            DrawLineEx(PadV(v, ax + nx * r, ay + ny * r), PadV(v, bx + nx * r, by + ny * r), thM, c);
        }
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

    /* Z: its own rectangle (2 : 3) beside the middle prong, a leader line from it to the trigger behind it */
    {
        const float x0 = 525.0f, y0 = 588.0f, x1 = 595.0f, y1 = 693.0f, ym = (y0 + y1) * 0.5f, xm = (x0 + x1) * 0.5f;
        Rectangle zr = PadR(v, x0, y0, x1, y1);
        Vector2 dot = PadV(v, 381.0f, ym);
        Vector2 zz[4] = { PadV(v, xm - 15.0f, ym - 21.0f), PadV(v, xm + 15.0f, ym - 21.0f), PadV(v, xm - 15.0f, ym + 21.0f), PadV(v, xm + 15.0f, ym + 21.0f) };
        float rr = 2.0f * 8.0f * s / fminf(zr.width, zr.height);
        if (lit[N64_BTN_Z]) DrawRectangleRounded(zr, rr, 24, c);
        PadRRect(v, x0, y0, x1, y1, 8.0f, thM, c);
        DrawLineEx((Vector2){ dot.x + 5.0f * s, dot.y }, PadV(v, x0, ym), thL, c);
        PadCircle(dot, 5.0f * s, thL, c);
        if (lit[N64_BTN_Z]) DrawCircleV(dot, 5.0f * s, c);
        else PadPoly(zz, 4, thM, c);
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
