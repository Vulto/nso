/* statuspanel.h - "Port status" overlay (grid S key). Included by main.c after
 * the X Details card and drawn on the same card frame (Details_DrawFrame):
 * same fade, dimming, colors and text style. Left column: headline, segmented
 * bar and legend (counts per type, status.h). Right column: every catalog game
 * with its type and whether it is playable, scrollable. B or S closes it. */
#ifndef NSO_STATUSPANEL_H
#define NSO_STATUSPANEL_H

#define STATUS_IMPLEMENTATION
#include "status.h"

#define STATUS_MAX_ROWS 1024

typedef struct {
    const GameEntry *game;
    const char *system;
    StatusType type;
    bool playable;
} StatusRow;

static StatusRow statusRows[STATUS_MAX_ROWS];
static int statusRowCount = 0, statusTop = 0;
static int statusCounts[ST_COUNT], statusTotal = 0, statusPlayable = 0;

static const Color statusColors[ST_COUNT] = {
    { 66, 133, 244, 255 },    /* recompilation: blue */
    { 52, 168, 83, 255 },     /* native source port: green */
    { 155, 89, 182, 255 },    /* emulated: purple */
    { 245, 150, 40, 255 },    /* not available: orange */
};

static const char *Status_TypeName(StatusType t) {
    static const char *const en[ST_COUNT] = { "Recompilation", "Native source port", "Emulated", "Not available" };
    static const char *const pt[ST_COUNT] = { "Recompilação", "Port nativo (código-fonte)", "Emulado", "Indisponível" };
    return (Lang_Get() == LANG_PT ? pt : en)[t];
}

static const char *Status_Word(int id) {
    static const char *const en[] = { "Port status", "playable", "Total", "Playable", "Not playable",
                                      "%d (%d%% of total)", "    \xF0\x9F\x85\x91 Back   " };
    static const char *const pt[] = { "Status dos ports", "jogáveis", "Total", "Jogável", "Não jogável",
                                      "%d (%d%% do total)", "    \xF0\x9F\x85\x91 Voltar   " };
    return (Lang_Get() == LANG_PT ? pt : en)[id];
}

static void Status_Open(void) {
    statusRowCount = 0;
    statusTotal = statusPlayable = 0;
    statusTop = 0;
    memset(statusCounts, 0, sizeof(statusCounts));
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        for (int i = 0; i < *systems[s].gameCount && statusRowCount < STATUS_MAX_ROWS; i++) {
            const GameEntry *g = &systems[s].games[i];
            StatusRow *r = &statusRows[statusRowCount++];
            r->game = g;
            r->system = systems[s].platform->name;
            r->type = Status_Classify(g);
            r->playable = GameIsLaunchable(g);
            statusCounts[r->type]++;
            statusTotal++;
            if (r->playable) statusPlayable++;
        }
    }
    statusOpen = true;
    InfoBar_Request(INFO_BAR_NONE);
}

static void Status_Close(void) {
    statusOpen = false;
    InfoBar_Request(INFO_BAR_MOSAIC);
}

static void Status_Update(float dt) {
    float target = statusOpen ? 1.0f : 0.0f;
    float step = dt * 1000.0f / DETAILS_FADE_DELTA_MS
               * (statusOpen ? DETAILS_FADE_IN_STEP : DETAILS_FADE_OUT_STEP);
    if (statusT < target) statusT = fminf(statusT + step, target);
    else if (statusT > target) statusT = fmaxf(statusT - step, target);
}

/* Rows that fit the list column; shared by the input and the draw code. */
static int Status_Visible(void) {
    float sh = (float)GetScreenHeight();
    int n = (int)((sh * 0.905f - sh * 0.06f) / (sh * 0.046f));
    return n < 1 ? 1 : n;
}

static void Status_HandleInput(bool gpUp, bool gpDown, bool gpBack) {
    if (!statusOpen) return;   /* sliding out */
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_S) || gpBack) {
        Status_Close();
        return;
    }
    int vis = Status_Visible(), maxTop = statusRowCount > vis ? statusRowCount - vis : 0;
    int d = 0;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) || gpDown) d = 1;
    if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || gpUp) d = -1;
    if (IsKeyPressed(KEY_PAGE_DOWN) || IsKeyPressedRepeat(KEY_PAGE_DOWN)) d = vis - 1;
    if (IsKeyPressed(KEY_PAGE_UP) || IsKeyPressedRepeat(KEY_PAGE_UP)) d = -(vis - 1);
    if (IsKeyPressed(KEY_HOME)) d = -statusRowCount;
    if (IsKeyPressed(KEY_END)) d = statusRowCount;
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) d -= (int)(wheel * 3.0f);
    statusTop += d;
    if (statusTop < 0) statusTop = 0;
    if (statusTop > maxTop) statusTop = maxTop;
}

static void Status_Bold(Font font, const char *text, Vector2 pos, float size, Color c) {
    DrawTextEx(font, text, pos, size, 2.0f, c);
    DrawTextEx(font, text, (Vector2){ pos.x + 1.2f, pos.y }, size, 2.0f, c);
}

/* Text ending at x. */
static void Status_TextR(Font font, const char *text, float x, float y, float size, float maxW, Color c) {
    float s = size;
    while (s > size * 0.6f && MeasureTextEx(font, text, s, 2.0f).x > maxW) s -= 1.0f;
    DrawTextEx(font, text, (Vector2){ x - MeasureTextEx(font, text, s, 2.0f).x, y }, s, 2.0f, c);
}

static void Status_Draw(Font font) {
    if (statusT <= 0.0f) return;
    float a = statusT;
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    const Color ink = { 44, 49, 50, 255 }, meta = { 92, 97, 98, 255 };
    const Color light = { 128, 134, 136, 255 }, track = { 214, 220, 222, 255 };
    const Color line = { 214, 220, 222, 255 }, inner = { 250, 252, 252, 255 };
    Rectangle card;
    float lw, rad;
    Details_DrawFrame(a, &card, &lw, &rad);

    /* Left column: heading, then one rounded card with headline, bar, legend. */
    float lx = card.x + lw * 0.07f, lcw = lw * 0.86f;
    Details_Text(font, Status_Word(0), (Vector2){ lx, card.y + sh * 0.03f }, sh * 0.030f, lcw, Fade(meta, a));
    float py = card.y + sh * 0.085f, pad = lcw * 0.055f;
    float legendRow = sh * 0.062f;
    Rectangle pc = { lx, py, lcw, sh * 0.19f + legendRow * ST_COUNT };
    DrawRectangleRounded(pc, 2.0f * rad / pc.height * 2.0f, 12, Fade(inner, a));

    float ix = pc.x + pad, iw = pc.width - 2.0f * pad, y = pc.y + sh * 0.025f;
    int pct = Status_Percent(statusPlayable, statusTotal);
    char big[16], rest[48], tot[16];
    snprintf(big, sizeof(big), "%d%%", pct);
    snprintf(rest, sizeof(rest), " %s", Status_Word(1));
    snprintf(tot, sizeof(tot), "%d", statusTotal);
    float bs = sh * 0.052f, ss = sh * 0.026f;
    Status_Bold(font, big, (Vector2){ ix, y }, bs, Fade(ink, a));
    float bw = MeasureTextEx(font, big, bs, 2.0f).x;
    DrawTextEx(font, rest, (Vector2){ ix + bw, y + bs - ss * 1.15f }, ss, 2.0f, Fade(light, a));
    Vector2 tv = MeasureTextEx(font, tot, ss * 1.2f, 2.0f);
    Status_Bold(font, tot, (Vector2){ ix + iw - tv.x, y + bs - ss * 1.4f }, ss * 1.2f, Fade(ink, a));
    const char *tl = Status_Word(2);
    Vector2 tlv = MeasureTextEx(font, tl, ss, 2.0f);
    DrawTextEx(font, tl, (Vector2){ ix + iw - tv.x - tlv.x - 10.0f, y + bs - ss * 1.15f }, ss, 2.0f, Fade(light, a));

    /* Segmented bar: gray track, one rounded segment per type, small gaps. */
    float by = y + bs + sh * 0.035f, bh = sh * 0.022f, gap = sh * 0.004f;
    Rectangle tr = { ix, by, iw, bh };
    DrawRectangleRounded(tr, 1.0f, 12, Fade(track, a));
    int used = 0;
    for (int t = 0; t < ST_COUNT; t++) if (statusCounts[t] > 0) used++;
    float room = iw - gap * (float)(used > 0 ? used - 1 : 0), x = ix;
    for (int t = 0; t < ST_COUNT; t++) {
        if (statusCounts[t] <= 0 || statusTotal <= 0) continue;
        float w = room * (float)statusCounts[t] / (float)statusTotal;
        DrawRectangleRounded((Rectangle){ x, by, w, bh }, 1.0f, 12, Fade(statusColors[t], a));
        x += w + gap;
    }

    /* Legend: dot + name, "<count> (<x>% of total)" at the right, dividers. */
    float ly = by + bh + sh * 0.025f, ds = sh * 0.011f, ts = sh * 0.023f;
    for (int t = 0; t < ST_COUNT; t++) {
        float ry = ly + legendRow * (float)t, cy = ry + legendRow * 0.5f;
        DrawCircleV((Vector2){ ix + ds, cy }, ds, Fade(statusColors[t], a));
        float nx = ix + ds * 3.0f;
        char cnt[48];
        snprintf(cnt, sizeof(cnt), Status_Word(5), statusCounts[t], Status_Percent(statusCounts[t], statusTotal));
        Status_TextR(font, cnt, ix + iw, cy - ts * 0.5f, ts, iw * 0.5f, Fade(light, a));
        float cw = MeasureTextEx(font, cnt, ts, 2.0f).x;
        Details_Text(font, Status_TypeName((StatusType)t), (Vector2){ nx, cy - ts * 0.5f }, ts,
                     iw - ds * 3.0f - cw - 12.0f, Fade(ink, a));
        if (t < ST_COUNT - 1)
            DrawLineEx((Vector2){ nx, ry + legendRow }, (Vector2){ ix + iw, ry + legendRow }, 1.0f, Fade(line, a));
    }

    /* Right column: scrollable per-game list. */
    float rx = card.x + lw + sw * 0.025f, rr = card.x + card.width - sw * 0.025f;
    float rowH = sh * 0.046f, top = card.y + sh * 0.03f, fs = sh * 0.026f;
    int vis = Status_Visible();
    float typeX = rx + (rr - rx) * 0.60f, sysX = rx + (rr - rx) * 0.44f;
    for (int k = 0; k < vis && statusTop + k < statusRowCount; k++) {
        const StatusRow *r = &statusRows[statusTop + k];
        float ry = top + rowH * (float)k, ty = ry + (rowH - fs) * 0.5f;
        DrawCircleV((Vector2){ rx + fs * 0.4f, ry + rowH * 0.5f }, fs * 0.3f, Fade(statusColors[r->type], a));
        Details_Text(font, r->game->title, (Vector2){ rx + fs * 1.1f, ty }, fs, sysX - rx - fs * 1.5f, Fade(ink, a));
        Details_Text(font, r->system, (Vector2){ sysX, ty }, fs * 0.9f, typeX - sysX - 10.0f, Fade(light, a));
        Details_Text(font, Status_TypeName(r->type), (Vector2){ typeX, ty }, fs * 0.9f, rr - typeX - sw * 0.1f, Fade(meta, a));
        Status_TextR(font, Status_Word(r->playable ? 3 : 4), rr, ty, fs * 0.9f, sw * 0.095f,
                     Fade(r->playable ? statusColors[ST_NATIVE] : light, a));
        if (k < vis - 1)
            DrawLineEx((Vector2){ rx + fs * 1.1f, ry + rowH }, (Vector2){ rr, ry + rowH }, 1.0f, Fade(line, a * 0.7f));
    }

    DrawInfoBarEx(font, Status_Word(6), a, 0.0f);
}

#endif /* NSO_STATUSPANEL_H */
