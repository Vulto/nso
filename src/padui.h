/* padui.h - Settings > Controller (NES): the controller drawn in white line art
 * (raylib primitives, after the reference picture), a lit white button for every
 * pressed control, and the eight NES buttons to remap. Included by main.c after the
 * scrape page; uses its statics. The screen is a third page below Image scraping:
 * the Settings page scrolls down to it like it does to the scrape rows.
 *
 * A: pick the button, then press the new key or pad button (Esc cancels, so does
 * waiting six seconds). A key sets the key, a pad button the pad control. The pad
 * button is read twice: as raylib's standard button (embedded games, the ports) and
 * as the raw js control (Mednafen), see padmap.h. Saved at once in config/nso.cfg. */
#ifndef NSO_PADUI_H
#define NSO_PADUI_H

#include "padmap.h"
#include "jspad.h"

#define PADUI_PAGE_Y 1.9f          /* top of the page, in screen heights below the Settings page */
#define PADUI_LISTEN_SEC 6.0
#define PADUI_PAIR_SEC 0.06        /* how long a pad press waits for its other reading */

static NesPadMap nesPad;
static bool padOpen = false;
static bool padListen = false;
static int padSel = 0;             /* NES_BTN_*, NES_BTN_COUNT = Restore defaults */
static double padListenT = 0.0;
static int padPendStd = -1, padPendKind = PAD_JS_NONE, padPendIdx = 0;
static char padPendDev[40];
static double padPendT = 0.0;

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

static void PadUi_ListenEnd(void) {
    if (padListen) SetExitKey(KEY_ESCAPE);
    padListen = false;
}

static void PadUi_Open(void) {
    padOpen = true;
    padSel = 0;
    padListen = false;
    JsPad_Open();
}

static void PadUi_Close(void) {
    PadUi_ListenEnd();
    padOpen = false;
    JsPad_Close();
}

static void PadUi_ListenBegin(void) {
    int k, idx;
    char dev[40];
    padListen = true;
    padListenT = GetTime();
    padPendStd = -1;
    padPendKind = PAD_JS_NONE;
    padPendT = 0.0;
    SetExitKey(KEY_NULL);   /* Esc cancels here, it must not close NSO */
    while (GetKeyPressed() > 0) { }
    JsPad_Update();
    while (JsPad_TakePress(&k, &idx, dev, sizeof(dev))) { }
}

static bool PadUi_Lit(int btn) {
    const PadBind *b = &nesPad.b[btn];
    return (b->key && IsKeyDown(b->key))
        || (b->pad >= 0 && IsGamepadAvailable(0) && IsGamepadButtonDown(0, b->pad))
        || (b->jsKind != PAD_JS_NONE && JsPad_Down(b->jsKind, b->jsIdx));
}

/* One frame of listening: a key, or a pad button (its standard and raw readings, which can
 * land a frame apart). */
static void PadUi_ListenPoll(void) {
    int key, k, idx;
    char dev[40];
    double now = GetTime();
    if (now - padListenT > PADUI_LISTEN_SEC) { PadUi_ListenEnd(); return; }
    while ((key = GetKeyPressed()) > 0) {
        if (key == KEY_ESCAPE) { PadUi_ListenEnd(); return; }
        if (PadMap_KeyBindable(key)) {
            nesPad.b[padSel].key = key;
            Pad_SaveNes();
            Sfx_Play(SFX_CLICK);
            PadUi_ListenEnd();
            return;
        }
    }
    {
        int std = GetGamepadButtonPressed();
        if (std >= 0 && PadMap_PadBindable(std) && padPendStd < 0) { padPendStd = std; if (padPendT == 0.0) padPendT = now; }
        if (padPendKind == PAD_JS_NONE && JsPad_TakePress(&k, &idx, dev, sizeof(dev))) {
            padPendKind = k;
            padPendIdx = idx;
            snprintf(padPendDev, sizeof(padPendDev), "%s", dev);
            if (padPendT == 0.0) padPendT = now;
        }
    }
    if (padPendT != 0.0 && ((padPendStd >= 0 && padPendKind != PAD_JS_NONE) || now - padPendT > PADUI_PAIR_SEC)) {
        PadBind *b = &nesPad.b[padSel];
        b->pad = padPendStd;
        b->jsKind = padPendKind;
        b->jsIdx = padPendKind != PAD_JS_NONE ? padPendIdx : 0;
        if (padPendKind != PAD_JS_NONE) snprintf(nesPad.joyId, sizeof(nesPad.joyId), "%s", padPendDev);
        padPendT = 0.0;
        padPendStd = -1;
        padPendKind = PAD_JS_NONE;
        Pad_SaveNes();
        Sfx_Play(SFX_CLICK);
        PadUi_ListenEnd();
    }
}

static void PadUi_Input(bool up, bool down, bool ok, bool back, bool left) {
    JsPad_Update();
    if (padListen) { PadUi_ListenPoll(); return; }
    if (back || left) { PadUi_Close(); Sfx_Play(SFX_CLICK); return; }
    if (up && padSel > 0) { padSel--; Sfx_Play(SFX_PLUCK); }
    if (down && padSel < NES_BTN_COUNT) { padSel++; Sfx_Play(SFX_PLUCK); }
    if (ok) {
        Sfx_Play(SFX_CLICK);
        if (padSel == NES_BTN_COUNT) {
            char cfg[2112];
            Lang_ConfigPath(cfg, sizeof(cfg));
            PadMap_NesDefaults(&nesPad);
            if (!PadMap_NesClear(cfg)) fprintf(stderr, "NSO: cannot save %s\n", cfg);
        } else {
            PadUi_ListenBegin();
        }
    }
}

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

/* "Key: Z    Pad: B" of one button; a pad control only the raw reading knows is "Button 3". */
static void PadUi_BindText(int btn, char *out, size_t n) {
    const PadBind *b = &nesPad.b[btn];
    const PadKeyInfo *ki = PadMap_KeyInfo(b->key);
    const PadBtnInfo *pi = PadMap_PadInfo(b->pad);
    char pad[40];
    if (pi) snprintf(pad, sizeof(pad), "%s", pi->label);
    else if (b->jsKind == PAD_JS_BUTTON) snprintf(pad, sizeof(pad), "#%d", b->jsIdx);
    else if (b->jsKind != PAD_JS_NONE) snprintf(pad, sizeof(pad), "%c%d", b->jsKind == PAD_JS_AXIS_POS ? '+' : '-', b->jsIdx);
    else snprintf(pad, sizeof(pad), "-");
    snprintf(out, n, "%s: %s    %s: %s", T(S_PC_KEY), ki ? ki->name : "-", T(S_PC_PAD), pad);
}

static void PadUi_Draw(Font font, Color bg) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float hx = sw * 0.449f, bx = sw * 0.485f, bw = sw * 0.387f;
    float oy = sh * PADUI_PAGE_Y;
    float th = fmaxf(2.0f, sh * 0.0028f);
    float hsize = (float)font.baseSize * 0.85f, osize = (float)font.baseSize * 0.85f;
    float bh = sh * 0.044f, step = sh * 0.0505f, top = oy + sh * 0.485f;
    bool lit[NES_BTN_COUNT];
    char line[160];

    for (int i = 0; i < NES_BTN_COUNT; i++) lit[i] = padOpen && PadUi_Lit(i);
    DrawFitText(font, T(S_PC_ENTRY), (Vector2){ hx, oy + sh * 0.092f }, hsize, sw - hx, WHITE);
    DrawNesController(bx, oy + sh * 0.13f, bw, lit, WHITE);
    for (int i = 0; i <= NES_BTN_COUNT; i++) {
        float y = top + step * (float)i;
        bool cur = padOpen && padSel == i;
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        Color fg = cur ? bg : WHITE;
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        if (i == NES_BTN_COUNT) {
            DrawFitText(font, T(S_PC_DEFAULTS), (Vector2){ bx + sw * 0.0225f, y }, osize, bw - sw * 0.03f, fg);
            continue;
        }
        DrawFitText(font, T((StrId)(S_PC_UP + i)), (Vector2){ bx + sw * 0.0225f, y }, osize, bw * 0.28f, fg);
        if (cur && padListen) snprintf(line, sizeof(line), "%s", T(S_PC_LISTEN));
        else PadUi_BindText(i, line, sizeof(line));
        {
            float maxW = bw * 0.66f, size = osize;
            Vector2 m = MeasureTextEx(font, line, size, 1.0f);
            if (m.x > maxW) { size *= maxW / m.x; m = MeasureTextEx(font, line, size, 1.0f); }
            DrawFitText(font, line, (Vector2){ bx + bw - m.x - sw * 0.012f, y }, size, m.x + 1.0f, fg);
        }
    }
    DrawFitText(font, padListen ? T(S_PC_HINT_LISTEN) : T(S_PC_HINT), (Vector2){ hx, oy + sh * 0.945f },
                (float)font.baseSize * 0.75f, sw - hx - sw * 0.02f, WHITE);
}

#endif /* NSO_PADUI_H */
