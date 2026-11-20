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

/* The N64 controller after its reference picture (774 x 723): the body's outline (the smoothed lower line of the picture), the two shoulder rims
 * (thin closed slivers with rounded caps lying on each top edge), the D-pad with a triangle in each arm, Start, B and A, the C
 * diamond (one circle through the four buttons, each with a triangle), the stick in its octagonal
 * gate, and, apart from the body, Z in its own rectangle (2 : 3) with a leader line to where the trigger
 * sits (behind the middle prong). The outline and the rims are traced from the picture's own lines
 * (centre lines, in the picture's pixels); the rest is placed by measurement. Lit buttons are filled white;
 * the stick's three circles shift together with a tilt. (x, y) is the box's top-left, w its width, th
 * the line weight (the NES controller's). The box is the picture's pixels 10..752 x 10..710. */
#define PADN64_W 742.0f
#define PADN64_H 700.0f

static const Vector2 n64Body[750] = {
    {388.8,707.8}, {384.9,708.4}, {380.9,708.6}, {376.9,708.5}, {372.9,707.9}, {369.1,706.9}, {365.4,705.3}, {361.9,703.4}, {358.6,701.1}, {355.6,698.5}, {352.7,695.7}, {350.1,692.6}, {347.7,689.5}, {345.5,686.1}, {343.4,682.7}, {341.4,679.2}, {339.6,675.7}, {337.9,672.1}, {336.2,668.4}, {334.7,664.7}, {333.2,661.0}, {331.8,657.3}, {330.4,653.6}, {328.9,649.8}, {327.5,646.1}, {326.1,642.4}, {324.7,638.6}, {323.5,634.8}, {322.3,631.0}, {321.1,627.2}, {320.0,623.3}, {318.9,619.5}, {317.8,615.6}, {316.7,611.8}, {315.6,607.9}, {314.6,604.1}, {313.6,600.2}, {312.6,596.3}, {311.6,592.5}, {310.7,588.6}, {309.7,584.7}, {308.8,580.8}, {308.0,576.9}, {307.1,573.0}, {306.3,569.1}, {305.5,565.1}, {304.8,561.2}, {304.0,557.3}, {303.3,553.4}, {302.6,549.4}, {302.0,545.5}, {301.3,541.5}, {300.7,537.6}, {300.0,533.6}, {299.3,529.7}, {298.6,525.8}, {297.9,521.8}, {297.2,517.9}, {296.5,514.0}, {295.9,510.0}, {295.2,506.1}, {294.5,502.1}, {293.8,498.2}, {293.1,494.3}, {292.3,490.3}, {291.6,486.4}, {290.9,482.5}, {290.2,478.5}, {289.5,474.6}, {288.8,470.7}, {288.2,466.7}, {287.5,462.8}, {286.8,458.8}, {286.2,454.9}, {285.6,450.9}, {285.0,447.0}, {284.4,443.0}, {283.7,439.1}, {283.0,435.1}, {282.3,431.2}, {281.5,427.3}, {280.7,423.4}, {279.9,419.4}, {279.1,415.5}, {278.2,411.6}, {277.3,407.8}, {276.2,403.9}, {274.9,400.1}, {273.3,396.4}, {271.6,392.9}, {269.5,389.4}, {267.2,386.1}, {264.7,383.1}, {261.9,380.2}, {258.9,377.6}, {255.6,375.3}, {252.2,373.2}, {248.7,371.3}, {245.0,369.6}, {241.3,368.1}, {237.6,366.8}, {233.8,365.6}, {229.9,364.5}, {226.1,363.5}, {222.2,362.6}, {218.2,361.8}, {214.3,361.1}, {210.4,360.4}, {206.4,359.9}, {202.4,359.4}, {198.5,358.9}, {194.5,358.5}, {190.5,358.2}, {186.5,358.0}, {182.5,358.4}, {178.6,359.2}, {174.9,360.7}, {171.4,362.6}, {168.2,365.0}, {165.5,367.9}, {163.2,371.2}, {161.4,374.8}, {159.9,378.5}, {158.6,382.2}, {157.4,386.1}, {156.2,389.9}, {155.0,393.7}, {153.9,397.5}, {152.7,401.4}, {151.5,405.2}, {150.3,409.0}, {149.2,412.8}, {148.0,416.7}, {146.9,420.5}, {145.8,424.3}, {144.6,428.2}, {143.5,432.0}, {142.3,435.8}, {141.1,439.6}, {140.0,443.5}, {138.8,447.3}, {137.6,451.1}, {136.3,454.9}, {135.1,458.7}, {133.8,462.5}, {132.5,466.3}, {131.2,470.0}, {129.9,473.8}, {128.6,477.6}, {127.3,481.4}, {125.9,485.1}, {124.5,488.9}, {123.1,492.6}, {121.7,496.4}, {120.3,500.1}, {118.8,503.8}, {117.3,507.5}, {115.8,511.2}, {114.2,514.9}, {112.5,518.5}, {110.8,522.1}, {109.0,525.7}, {107.1,529.2}, {105.0,532.6}, {102.9,536.0}, {100.6,539.3}, {98.1,542.4}, {95.5,545.4}, {92.6,548.2}, {89.4,550.6}, {86.0,552.7}, {82.4,554.4}, {78.6,555.7}, {74.7,556.4}, {70.7,556.6}, {66.7,556.5}, {62.8,555.9}, {58.9,554.7}, {55.4,552.9}, {52.0,550.7}, {49.0,548.1}, {46.2,545.2}, {43.7,542.1}, {41.4,538.8}, {39.3,535.4}, {37.4,531.9}, {35.7,528.3}, {34.1,524.6}, {32.7,520.9}, {31.4,517.1}, {30.2,513.3}, {29.0,509.5}, {27.9,505.6}, {26.9,501.8}, {25.9,497.9}, {24.9,494.0}, {23.9,490.2}, {22.9,486.3}, {22.0,482.4}, {21.0,478.5}, {20.2,474.6}, {19.3,470.7}, {18.6,466.8}, {17.9,462.8}, {17.2,458.9}, {16.5,455.0}, {15.9,451.0}, {15.4,447.0}, {14.9,443.1}, {14.4,439.1}, {14.1,435.1}, {13.7,431.1}, {13.4,427.2}, {13.1,423.2}, {12.9,419.2}, {12.6,415.2}, {12.4,411.2}, {12.2,407.2}, {12.1,403.2}, {12.0,399.2}, {12.0,395.2}, {12.0,391.2}, {12.0,387.2}, {12.0,383.2}, {12.0,379.2}, {12.0,375.2}, {12.0,371.2}, {12.0,367.2}, {12.1,363.2}, {12.2,359.2}, {12.4,355.2}, {12.6,351.2}, {12.9,347.3}, {13.2,343.3}, {13.4,339.3}, {13.8,335.3}, {14.1,331.3}, {14.5,327.3}, {14.9,323.3}, {15.4,319.4}, {15.9,315.4}, {16.5,311.5}, {17.1,307.5}, {17.7,303.6}, {18.4,299.6}, {19.0,295.7}, {19.8,291.7}, {20.5,287.8}, {21.3,283.9}, {22.1,280.0}, {22.9,276.1}, {23.7,272.1}, {24.5,268.2}, {25.4,264.3}, {26.2,260.4}, {27.1,256.5}, {27.9,252.6}, {28.7,248.7}, {29.4,244.7}, {30.0,240.8}, {30.6,236.8}, {31.2,232.9}, {31.7,228.9}, {32.2,225.0}, {32.7,221.0}, {33.2,217.0}, {33.7,213.1}, {34.2,209.1}, {34.8,205.1}, {35.4,201.2}, {36.2,197.3}, {37.0,193.3}, {38.0,189.5}, {39.0,185.6}, {40.0,181.7}, {41.2,177.9}, {42.4,174.1}, {43.8,170.4}, {45.3,166.7}, {46.9,163.0}, {48.6,159.3}, {50.3,155.8}, {52.2,152.2}, {54.1,148.7}, {56.2,145.3}, {58.3,141.9}, {60.5,138.6}, {62.9,135.3}, {65.3,132.2}, {67.9,129.1}, {70.5,126.1}, {73.3,123.2}, {76.2,120.5}, {79.2,117.8}, {82.2,115.2}, {85.4,112.7}, {88.6,110.4}, {91.9,108.1}, {95.2,105.9}, {98.6,103.8}, {102.1,101.8}, {105.6,99.9}, {109.1,98.0}, {112.7,96.2}, {116.3,94.5}, {120.0,92.9}, {123.7,91.4}, {127.4,90.1}, {131.2,88.8}, {135.0,87.6}, {138.9,86.4}, {142.7,85.4}, {146.6,84.4}, {150.5,83.5}, {154.4,82.6}, {158.3,81.7}, {162.2,80.9}, {166.1,80.2}, {170.1,79.4}, {174.0,78.7}, {177.9,78.0}, {181.9,77.4}, {185.8,76.8}, {189.8,76.2}, {193.7,75.7}, {197.7,75.1}, {201.7,74.6}, {205.6,74.0}, {209.6,73.6}, {213.6,73.1}, {217.6,72.7}, {221.5,72.1}, {225.4,71.3}, {229.1,69.9}, {232.5,67.7}, {235.3,64.9}, {237.5,61.5}, {239.3,57.9}, {240.9,54.3}, {242.5,50.6}, {244.3,47.0}, {246.4,43.6}, {248.9,40.5}, {251.7,37.7}, {254.9,35.3}, {258.4,33.3}, {261.9,31.5}, {265.6,29.8}, {269.3,28.4}, {273.1,27.0}, {276.9,25.8}, {280.7,24.6}, {284.5,23.5}, {288.4,22.4}, {292.2,21.4}, {296.1,20.5}, {300.0,19.5}, {303.9,18.7}, {307.8,17.8}, {311.8,17.1}, {315.7,16.4}, {319.6,15.8}, {323.6,15.3}, {327.6,14.9}, {331.6,14.4}, {335.5,14.0}, {339.5,13.7}, {343.5,13.3}, {347.5,13.1}, {351.5,12.9}, {355.5,12.7}, {359.5,12.5}, {363.5,12.3}, {367.5,12.1}, {371.5,12.0}, {375.5,12.0}, {379.5,12.0}, {383.5,12.0}, {387.4,12.1}, {391.4,12.1}, {395.4,12.3}, {399.4,12.5}, {403.4,12.8}, {407.4,12.9}, {411.4,13.1}, {415.4,13.4}, {419.4,13.7}, {423.4,14.1}, {427.3,14.5}, {431.3,15.0}, {435.3,15.5}, {439.2,16.0}, {443.2,16.6}, {447.2,17.2}, {451.1,17.7}, {455.1,18.3}, {459.0,18.9}, {463.0,19.6}, {466.9,20.4}, {470.8,21.2}, {474.7,22.1}, {478.6,23.1}, {482.4,24.1}, {486.3,25.2}, {490.1,26.2}, {494.0,27.4}, {497.8,28.6}, {501.5,30.0}, {505.2,31.5}, {508.9,33.1}, {512.5,34.8}, {516.0,36.8}, {519.2,39.1}, {522.1,41.9}, {524.6,45.0}, {526.6,48.4}, {528.2,52.1}, {529.7,55.8}, {531.1,59.6}, {532.8,63.2}, {535.0,66.5}, {537.9,69.3}, {541.4,71.1}, {545.2,72.2}, {549.2,72.9}, {553.2,73.3}, {557.1,73.7}, {561.1,74.2}, {565.1,74.8}, {569.0,75.3}, {573.0,75.9}, {576.9,76.5}, {580.9,77.1}, {584.8,77.7}, {588.8,78.4}, {592.7,79.1}, {596.6,79.8}, {600.6,80.6}, {604.5,81.4}, {608.4,82.2}, {612.3,83.0}, {616.2,83.9}, {620.1,84.8}, {624.0,85.8}, {627.8,86.9}, {631.7,88.0}, {635.5,89.2}, {639.3,90.4}, {643.1,91.6}, {646.8,93.0}, {650.6,94.5}, {654.2,96.1}, {657.9,97.8}, {661.4,99.6}, {664.9,101.5}, {668.3,103.6}, {671.6,105.8}, {674.9,108.1}, {678.1,110.5}, {681.3,113.0}, {684.4,115.5}, {687.4,118.2}, {690.3,120.9}, {693.0,123.8}, {695.7,126.8}, {698.3,129.8}, {700.8,132.9}, {703.2,136.1}, {705.6,139.4}, {707.8,142.7}, {709.9,146.1}, {711.9,149.5}, {713.9,153.0}, {715.7,156.5}, {717.5,160.1}, {719.1,163.8}, {720.6,167.5}, {722.1,171.2}, {723.4,175.0}, {724.6,178.8}, {725.8,182.6}, {726.9,186.5}, {728.0,190.3}, {728.9,194.2}, {729.7,198.1}, {730.4,202.1}, {731.0,206.0}, {731.6,210.0}, {732.2,213.9}, {732.7,217.9}, {733.1,221.9}, {733.5,225.9}, {733.9,229.8}, {734.3,233.8}, {734.8,237.8}, {735.3,241.7}, {735.8,245.7}, {736.4,249.7}, {737.0,253.6}, {737.6,257.6}, {738.3,261.5}, {739.0,265.4}, {739.7,269.4}, {740.4,273.3}, {741.1,277.2}, {741.8,281.2}, {742.5,285.1}, {743.2,289.1}, {743.8,293.0}, {744.4,297.0}, {745.0,300.9}, {745.5,304.9}, {746.0,308.9}, {746.4,312.8}, {746.8,316.8}, {747.3,320.8}, {747.7,324.7}, {748.1,328.7}, {748.5,332.7}, {748.8,336.7}, {749.1,340.7}, {749.3,344.7}, {749.6,348.7}, {749.8,352.7}, {749.9,356.7}, {750.0,360.6}, {750.0,364.6}, {750.1,368.6}, {750.3,372.6}, {750.5,376.6}, {750.7,380.6}, {750.8,384.6}, {750.8,388.6}, {750.7,392.6}, {750.5,396.6}, {750.2,400.6}, {750.1,404.6}, {750.0,408.6}, {749.9,412.6}, {749.7,416.6}, {749.4,420.6}, {749.1,424.6}, {748.7,428.5}, {748.4,432.5}, {747.9,436.5}, {747.5,440.5}, {746.9,444.4}, {746.3,448.4}, {745.7,452.3}, {745.0,456.3}, {744.3,460.2}, {743.6,464.1}, {742.8,468.1}, {742.0,472.0}, {741.2,475.9}, {740.3,479.8}, {739.4,483.7}, {738.5,487.6}, {737.6,491.5}, {736.7,495.4}, {735.8,499.3}, {734.8,503.1}, {733.8,507.0}, {732.7,510.8}, {731.5,514.7}, {730.2,518.5}, {728.9,522.2}, {727.4,525.9}, {725.8,529.6}, {724.0,533.2}, {722.1,536.7}, {719.9,540.0}, {717.5,543.2}, {714.8,546.2}, {711.9,548.9}, {708.7,551.4}, {705.3,553.5}, {701.7,555.2}, {697.9,556.5}, {694.0,557.2}, {690.0,557.5}, {686.0,557.3}, {682.1,556.5}, {678.3,555.3}, {674.7,553.6}, {671.3,551.4}, {668.2,549.0}, {665.3,546.3}, {662.6,543.3}, {660.0,540.2}, {657.7,536.9}, {655.6,533.6}, {653.6,530.1}, {651.7,526.6}, {650.0,522.9}, {648.4,519.3}, {646.8,515.6}, {645.3,511.9}, {643.8,508.2}, {642.4,504.5}, {640.9,500.8}, {639.5,497.0}, {638.0,493.3}, {636.6,489.6}, {635.2,485.8}, {633.9,482.1}, {632.5,478.3}, {631.2,474.5}, {629.8,470.7}, {628.5,467.0}, {627.2,463.2}, {626.0,459.4}, {624.8,455.6}, {623.6,451.7}, {622.5,447.9}, {621.3,444.1}, {620.1,440.3}, {619.0,436.4}, {617.8,432.6}, {616.7,428.8}, {615.5,425.0}, {614.4,421.1}, {613.2,417.3}, {612.1,413.5}, {611.0,409.6}, {609.9,405.8}, {608.8,401.9}, {607.7,398.1}, {606.6,394.2}, {605.6,390.4}, {604.5,386.5}, {603.3,382.7}, {602.0,378.9}, {600.5,375.2}, {598.6,371.7}, {596.4,368.4}, {593.6,365.5}, {590.5,363.0}, {587.0,361.0}, {583.3,359.6}, {579.4,358.8}, {575.4,358.5}, {571.4,358.6}, {567.4,358.9}, {563.4,359.4}, {559.5,359.9}, {555.5,360.4}, {551.6,361.0}, {547.6,361.6}, {543.7,362.2}, {539.7,362.9}, {535.8,363.7}, {531.9,364.6}, {528.0,365.5}, {524.2,366.6}, {520.3,367.8}, {516.6,369.2}, {513.0,370.9}, {509.5,372.8}, {506.2,375.0}, {503.0,377.5}, {500.0,380.2}, {497.3,383.1}, {494.8,386.2}, {492.6,389.5}, {490.5,393.0}, {488.7,396.5}, {487.2,400.2}, {485.9,404.0}, {484.7,407.8}, {483.7,411.7}, {482.8,415.6}, {481.9,419.5}, {481.0,423.4}, {480.2,427.3}, {479.4,431.2}, {478.7,435.2}, {477.9,439.1}, {477.2,443.0}, {476.5,446.9}, {475.7,450.9}, {475.0,454.8}, {474.3,458.7}, {473.6,462.7}, {472.8,466.6}, {472.1,470.5}, {471.5,474.5}, {470.8,478.4}, {470.1,482.4}, {469.5,486.3}, {468.8,490.3}, {468.1,494.2}, {467.4,498.1}, {466.7,502.1}, {466.0,506.0}, {465.4,509.9}, {464.7,513.9}, {464.1,517.8}, {463.5,521.8}, {462.9,525.7}, {462.2,529.7}, {461.5,533.6}, {460.8,537.6}, {460.1,541.5}, {459.4,545.4}, {458.7,549.4}, {458.0,553.3}, {457.2,557.2}, {456.5,561.2}, {455.7,565.1}, {454.9,569.0}, {454.2,572.9}, {453.4,576.8}, {452.6,580.8}, {451.8,584.7}, {451.0,588.6}, {450.1,592.5}, {449.3,596.4}, {448.4,600.3}, {447.6,604.2}, {446.8,608.1}, {445.9,612.0}, {444.9,615.9}, {443.8,619.8}, {442.7,623.6}, {441.6,627.4}, {440.5,631.3}, {439.3,635.1}, {438.1,638.9}, {436.8,642.7}, {435.5,646.5}, {434.1,650.2}, {432.6,653.9}, {431.1,657.6}, {429.5,661.3}, {427.9,665.0}, {426.1,668.6}, {424.3,672.1}, {422.4,675.6}, {420.4,679.1}, {418.4,682.5}, {416.3,685.9}, {414.0,689.2}, {411.5,692.3}, {408.9,695.3}, {406.0,698.2}, {403.0,700.8}, {399.8,703.1}, {396.3,705.1}, {392.7,706.7} };
static const Vector2 n64RimLU[28] = {
    {83.0,92.0}, {88.7,89.9}, {94.3,87.3}, {100.0,84.6}, {105.7,82.0}, {111.3,79.6}, {117.0,77.4}, {122.7,75.4}, {128.3,73.6}, {134.0,72.0}, {139.7,70.4}, {145.3,68.9}, {151.0,67.5}, {156.7,66.3}, {162.3,65.2}, {168.0,64.2}, {173.7,63.3}, {179.3,62.5}, {185.0,61.7}, {190.7,60.8}, {196.3,59.9}, {202.0,58.9}, {207.7,58.0}, {213.3,57.0}, {219.0,56.2}, {224.7,55.4}, {230.3,54.9}, {236.0,54.5} };
static const Vector2 n64RimLL[28] = {
    {83.0,114.6}, {88.7,110.3}, {94.3,106.5}, {100.0,103.0}, {105.7,99.8}, {111.3,96.8}, {117.0,94.2}, {122.7,91.8}, {128.3,89.7}, {134.0,87.9}, {139.7,86.2}, {145.3,84.7}, {151.0,83.3}, {156.7,82.1}, {162.3,80.9}, {168.0,79.8}, {173.7,78.8}, {179.3,77.8}, {185.0,76.9}, {190.7,76.1}, {196.3,75.3}, {202.0,74.5}, {207.7,73.8}, {213.3,73.1}, {219.0,72.5}, {224.7,71.5}, {230.3,69.2}, {236.0,63.9} };
static const Vector2 n64RimRU[28] = {
    {535.0,55.3}, {540.4,55.6}, {545.8,56.2}, {551.2,56.9}, {556.6,57.8}, {562.0,58.6}, {567.4,59.4}, {572.9,60.3}, {578.3,61.2}, {583.7,62.1}, {589.1,63.0}, {594.5,64.0}, {599.9,65.0}, {605.3,66.1}, {610.7,67.1}, {616.1,68.2}, {621.5,69.4}, {626.9,70.7}, {632.3,72.2}, {637.7,73.8}, {643.1,75.6}, {648.6,77.5}, {654.0,79.6}, {659.4,82.0}, {664.8,84.6}, {670.2,87.3}, {675.6,89.9}, {681.0,92.0} };
static const Vector2 n64RimRL[28] = {
    {535.0,66.6}, {540.4,70.7}, {545.8,72.4}, {551.2,73.1}, {556.6,73.7}, {562.0,74.3}, {567.4,75.1}, {572.9,75.9}, {578.3,76.7}, {583.7,77.5}, {589.1,78.4}, {594.5,79.4}, {599.9,80.5}, {605.3,81.5}, {610.7,82.7}, {616.1,83.8}, {621.5,85.2}, {626.9,86.6}, {632.3,88.2}, {637.7,89.9}, {643.1,91.7}, {648.6,93.7}, {654.0,96.0}, {659.4,98.5}, {664.8,101.5}, {670.2,104.9}, {675.6,108.6}, {681.0,112.7} };

/* A rounded triangle pointing along (ux, uy) from (cx, cy): apex at distance ap, base at ba, half-width hf
 * (reference units), outline th, filled when on. */
static void PadN64Tri(PadView v, float cx, float cy, float ux, float uy, float ap, float ba, float hf, float rad, float th, bool on, Color c) {
    Vector2 tri[3] = {
        PadV(v, cx + ux * ap, cy + uy * ap),
        PadV(v, cx + ux * ba - uy * hf, cy + uy * ba + ux * hf),
        PadV(v, cx + ux * ba + uy * hf, cy + uy * ba - ux * hf) };
    PadRoundPolyG(tri, 3, rad * v.s, th, on, c);
}

/* A shoulder rim: a thin closed shape lying on the body's top edge. Its outer line is chain u, its inner line is
 * the body's own edge (chain l, same x positions), closed at both ends by a short cap; the four corners are
 * rounded (reference units). Filled between the two lines when lit. */
static Vector2 PadLerpTo(Vector2 a, Vector2 b, float d) {
    float l = hypotf(b.x - a.x, b.y - a.y), f = l > 0.001f ? fminf(0.45f, d / l) : 0.0f;
    return (Vector2){ a.x + (b.x - a.x) * f, a.y + (b.y - a.y) * f };
}

static void PadN64Rim(PadView v, const Vector2 *u, const Vector2 *l, int n, bool lit, float th, Color c) {
    Vector2 path[128], sc[128];
    const float r = 5.0f;
    int m = 0;
    m = PadBend(path, m, PadLerpTo(u[0], l[0], r), u[0], u[1]);
    for (int i = 2; i < n - 1; i++) path[m++] = u[i];
    m = PadBend(path, m, u[n - 2], u[n - 1], PadLerpTo(u[n - 1], l[n - 1], r));
    m = PadBend(path, m, PadLerpTo(l[n - 1], u[n - 1], r), l[n - 1], l[n - 2]);
    for (int i = n - 3; i >= 1; i--) path[m++] = l[i];
    m = PadBend(path, m, l[1], l[0], PadLerpTo(l[0], u[0], r));
    for (int i = 0; i < m; i++) sc[i] = PadV(v, path[i].x, path[i].y);
    if (lit)
        for (int i = 0; i + 1 < n; i++) {
            Vector2 a0 = PadV(v, u[i].x, u[i].y), a1 = PadV(v, u[i + 1].x, u[i + 1].y);
            Vector2 b0 = PadV(v, l[i].x, l[i].y), b1 = PadV(v, l[i + 1].x, l[i + 1].y);
            PadTri(a0, a1, b1, c);
            PadTri(a0, b1, b0, c);
        }
    PadLoop(sc, m, th, c);
}

static void DrawN64Controller(float x, float y, float w, float th, const bool lit[PAD_MAX_BTN], Color c) {
    float s = w / PADN64_W;
    PadView v = { x + 60.0f * s, y + 155.0f * s, s };   /* PadV(rx, ry) = (x + (rx - 10) s, y + (ry - 10) s) */
    float thM = th * 0.8f, thL = fmaxf(1.0f, th * 0.55f);
    enum { BN = (int)(sizeof(n64Body) / sizeof(n64Body[0])) };
    Vector2 loop[BN];

    for (int i = 0; i < BN; i++) loop[i] = PadV(v, n64Body[i].x, n64Body[i].y);
    PadLoop(loop, BN, th, c);
    PadN64Rim(v, n64RimLU, n64RimLL, 28, lit[N64_BTN_L], th, c);
    PadN64Rim(v, n64RimRU, n64RimRL, 28, lit[N64_BTN_R], th, c);

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
            if (!lit[N64_BTN_CUP + d]) PadN64Tri(v, ccx, ccy, ux, uy, 12.3f, -6.2f, 9.5f, 2.5f, thL, false, c);   /* its centroid on the circle's centre */
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
        const float x0 = 531.0f, y0 = 578.0f, x1 = 589.0f, y1 = 683.0f;
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
