/* statuspanel.h - status + scraper overlay (grid S key). Included by main.c
 * after the X Details card and drawn on the same card frame (Details_DrawFrame):
 * same fade, dimming, colors and text style. Four tabs (Y cycles them).
 * Statistics is the original port-status content. Emulated holds the image
 * scraping page (moved from the sidebar), scoped to ROMs in roms/<System>/.
 * PC Ports and Decomps are placeholders in this step. B or S closes it. */
#ifndef NSO_STATUSPANEL_H
#define NSO_STATUSPANEL_H

#define STATUS_IMPLEMENTATION
#include "status.h"

#define STATUS_MAX_ROWS 1024

typedef struct {
    const GameEntry *game;
    StatusType type;
    bool playable;
} StatusRow;

typedef enum {
    STATUS_TAB_PORTS = 0,
    STATUS_TAB_DECOMPS,
    STATUS_TAB_EMU,
    STATUS_TAB_STATS,
    STATUS_TAB_COUNT
} StatusTab;

/* One left action ("Download all box arts"), not in the focus cycle. */
enum { STATUS_EMU_ACTIONS = 1 };

static StatusRow statusRows[STATUS_MAX_ROWS];
static int statusRowCount = 0, statusTop = 0;
static int statusCounts[ST_COUNT], statusTotal = 0, statusPlayable = 0;
static const char *statusSystemName = "";
static int statusTab = STATUS_TAB_STATS;
static int statusEmuRow = 0;
static int statusEmuRoms[MAX_GAMES];
static int statusEmuRomN = 0;
static int statusEmuListTop = 0;
static int statusEmuNeedRefresh = 1;
static int statusEmuSawBusy = 0;
static Texture2D statusEmuPrevTex;
static int statusEmuPrevIdx = -1;

/* Release builds only run emulated games, so the Statistics tab hides the
 * Recompilation and Native source port counts (and those games' rows). The
 * code stays; build without NSO_RELEASE, or with -DNSO_SHOW_ALL_SYSTEMS, to
 * see everything. */
#if defined(NSO_RELEASE) && !defined(NSO_SHOW_ALL_SYSTEMS)
#define STATUS_TYPE_SHOWN(t) ((t) != ST_RECOMP && (t) != ST_NATIVE)
#else
#define STATUS_TYPE_SHOWN(t) 1
#endif
static int Status_ShownTypes(void) {
    int n = 0;
    for (int t = 0; t < ST_COUNT; t++) if (STATUS_TYPE_SHOWN(t)) n++;
    return n;
}

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
    static const char *const en[] = { "Port status", "status", "Playable", "Not playable",
                                      "%d (%d%% of total)", "    \xF0\x9F\x85\x91 Back   " };
    static const char *const pt[] = { "Status dos ports", "status", "Jogável", "Não jogável",
                                      "%d (%d%% do total)", "    \xF0\x9F\x85\x91 Voltar   " };
    return (Lang_Get() == LANG_PT ? pt : en)[id];
}

static void Status_Open(void) {
    statusRowCount = 0;
    statusTotal = statusPlayable = 0;
    statusTop = 0;
    statusTab = STATUS_TAB_STATS;
    statusEmuRow = 0;
    statusEmuListTop = 0;
    statusEmuNeedRefresh = 1;
    memset(statusCounts, 0, sizeof(statusCounts));
    const System *sys = ActiveSystem();
    statusSystemName = sys->platform->name ? sys->platform->name : "";
    for (int i = 0; i < *sys->gameCount && statusRowCount < STATUS_MAX_ROWS; i++) {
        const GameEntry *g = &sys->games[i];
        StatusType gt = Status_Classify(g);
        if (!STATUS_TYPE_SHOWN(gt)) continue;
        StatusRow *r = &statusRows[statusRowCount++];
        r->game = g;
        r->type = gt;
        r->playable = GameIsLaunchable(g);
        statusCounts[r->type]++;
        statusTotal++;
        if (r->playable) statusPlayable++;
    }
    statusOpen = true;
    InfoBar_Request(INFO_BAR_STATUS);
}

static void Status_EmuUnloadPrev(void) {
    if (statusEmuPrevTex.id != 0) UnloadTexture(statusEmuPrevTex);
    statusEmuPrevTex = (Texture2D){ 0 };
    statusEmuPrevIdx = -1;
}

static void Status_EmuRefreshRoms(void) {
    int last;
    statusEmuRomN = ScrapeMenu_ListRoms(statusEmuRoms, MAX_GAMES);
    last = statusEmuRomN > 0 ? statusEmuRomN - 1 : 0;
    if (statusEmuRow > last) statusEmuRow = last;
    if (statusEmuRow < 0) statusEmuRow = 0;
}

static int Status_EmuFocusCatalog(void) {
    if (gsOpen) return GameSel_FocusCatalog();
    if (ScrapeMenu_Choosing()) return ScrapeMenu_ChoiceGame();
    if (statusEmuRow >= 0 && statusEmuRow < statusEmuRomN)
        return statusEmuRoms[statusEmuRow];
    return -1;
}

static Texture2D Status_EmuArt(int catalog) {
    Texture2D t;
    if (ScrapeMenu_Choosing()) {
        t = ScrapeMenu_ChoicePreview();
        if (t.id != 0) return t;
    }
    if (catalog < 0) return (Texture2D){ 0 };
    t = GetBoxart(catalog);
    if (t.id != 0) return t;
    if (catalog >= ActiveGameTotal()) return (Texture2D){ 0 };
    if (statusEmuPrevIdx == catalog && statusEmuPrevTex.id != 0)
        return statusEmuPrevTex;
    Status_EmuUnloadPrev();
    statusEmuPrevIdx = catalog;
    {
        const char *rel = ActiveGames()[catalog].boxartPath;
        Image img;
        if (!rel || !rel[0]) return (Texture2D){ 0 };
        img = LoadBoxartImage(GetDataPath(rel));
        if (!img.data) return (Texture2D){ 0 };
        statusEmuPrevTex = LoadTextureFromImage(img);
        UnloadImage(img);
    }
    return statusEmuPrevTex;
}

static void Status_Close(void) {
    if (gsOpen) {
        if (GameSel_Busy()) GameSel_Cancel();
        gsOpen = false;
    }
    if (scrapeBusy) ScrapeMenu_Cancel();
    Status_EmuUnloadPrev();
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

/* Room taken by the tab bar at the top of the card. */
static float Status_TabH(float sh) { return sh * 0.058f; }
static float Status_ContentOff(float sh) { return sh * 0.012f + Status_TabH(sh) + sh * 0.010f; }

/* Card height: 36px shorter than the Details card at 1920x1080, scaled. */
static float Status_CardHeight(float sh) {
    return roundf(sh * 0.905f) - roundf(36.0f * sh / 1080.0f);
}

/* Rows that fit the list column; shared by the input and the draw code. */
static int Status_Visible(void) {
    float sh = (float)GetScreenHeight();
    float top = sh * 0.047f + Status_ContentOff(sh);
    float bot = sh * 0.047f + Status_CardHeight(sh) - sh * 0.018f;
    int n = (int)((bot - top) / (sh * 0.046f));
    return n < 1 ? 1 : n;
}

static bool Status_YPressed(void) {
    return IsKeyPressed(KEY_Y) || padCDown
        || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, NSO_BTN_Y));
}

static bool Status_XPressed(void) {
    return IsKeyPressed(KEY_X) || padCUp
        || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, NSO_BTN_X));
}

static void Status_CycleTab(void) {
    statusTab = (statusTab + 1) % STATUS_TAB_COUNT;
    if (statusTab == STATUS_TAB_EMU) statusEmuNeedRefresh = 1;
    Sfx_Play(SFX_PLUCK);
}

static void Status_HandleInput(bool gpUp, bool gpDown, bool gpBack,
                               bool gpConfirm, bool gpLeft, bool gpRight) {
    bool back = IsKeyPressed(KEY_B) || gpBack;
    bool ok = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm;
    bool up = IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP) || gpUp;
    bool down = IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN) || gpDown;
    bool left = IsKeyPressed(KEY_LEFT) || gpLeft;
    bool right = IsKeyPressed(KEY_RIGHT) || gpRight;
    bool y = Status_YPressed();
    if (!statusOpen) return;   /* sliding out */

    /* The box-art picker comes first: Select games stays open (gsOpen) while
     * its download pauses for a choice, and the picker must get the keys. */
    if (ScrapeMenu_Choosing()) {
        if (ScrapeMenu_HandleChoice(ok, back, up || left, down || right)
            && (ok || back || up || down || left || right))
            Sfx_Play((ok || back) ? SFX_CLICK : SFX_PLUCK);
        return;
    }
    if (gsOpen) {
        GameSel_Input(up, down, ok, back, left);
        return;
    }
    if (scrapeBusy) {
        if (back) { ScrapeMenu_Cancel(); Sfx_Play(SFX_CLICK); return; }
        if (y) Status_CycleTab();
        return;
    }
    if (back) {
        Status_Close();
        return;
    }
    if (y) {
        Status_CycleTab();
        return;
    }

    if (statusTab == STATUS_TAB_EMU) {
        if (statusEmuRomN > 0) {
            if (up && statusEmuRow > 0) { statusEmuRow--; Sfx_Play(SFX_PLUCK); }
            if (down && statusEmuRow < statusEmuRomN - 1) { statusEmuRow++; Sfx_Play(SFX_PLUCK); }
        }
        if (ok || Status_XPressed()) {
            Sfx_Play(SFX_CLICK);
            ScrapeMenu_StartFromRoms(0);
        }
        return;
    }
    if (statusTab != STATUS_TAB_STATS) return;

    int vis = Status_Visible(), maxTop = statusRowCount > vis ? statusRowCount - vis : 0;
    int d = 0;
    if (down) d = 1;
    if (up) d = -1;
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

static void Status_ChoiceLabel(const char *file, char *dst, size_t n) {
    char *dot;
    if (!dst || n == 0) return;
    snprintf(dst, n, "%s", file ? file : "");
    dot = strrchr(dst, '.');
    if (dot && (strcmp(dot, ".png") == 0 || strcmp(dot, ".jpg") == 0
                || strcmp(dot, ".jpeg") == 0 || strcmp(dot, ".PNG") == 0))
        *dot = '\0';
}

static void Status_DrawTabs(Font font, Rectangle card, float lw, float a, Color ink, Color meta, Color line) {
    static const StrId ids[STATUS_TAB_COUNT] = {
        S_TAB_PORTS, S_TAB_DECOMPS, S_TAB_EMU, S_TAB_STATS
    };
    float sh = (float)GetScreenHeight();
    float tabH = Status_TabH(sh);
    float y = card.y + sh * 0.012f;
    float inset = sh * 0.01f;
    float x0 = card.x + lw + inset;
    float barW = card.width - lw - 2.0f * inset;
    float tabW = barW / (float)STATUS_TAB_COUNT;
    float ts = sh * 0.024f;
    for (int i = 0; i < STATUS_TAB_COUNT; i++) {
        const char *lab = T(ids[i]);
        float x = x0 + tabW * (float)i;
        Color c = Fade(i == statusTab ? ink : meta, a);
        float tw = MeasureTextEx(font, lab, ts, 1.0f).x;
        float tx = x + (tabW - tw) * 0.5f;
        DrawTextEx(font, lab, (Vector2){ tx, y + (tabH - ts) * 0.45f }, ts, 1.0f, c);
        if (i == statusTab) {
            float uw = fminf(tw + sh * 0.016f, tabW * 0.72f);
            DrawRectangleRec((Rectangle){ x + (tabW - uw) * 0.5f, y + tabH - sh * 0.004f,
                                          uw, sh * 0.0035f }, Fade(ink, a));
        }
    }
    DrawLineEx((Vector2){ x0, y + tabH },
               (Vector2){ x0 + barW, y + tabH },
               1.0f, Fade(line, a));
}

static void Status_DrawStats(Font font, Rectangle card, float lw, float rad, float a,
                             Color ink, Color meta, Color light, Color track, Color line, Color inner) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float content = Status_ContentOff(sh);
    float lx = card.x + lw * 0.07f, lcw = lw * 0.86f;
    char heading[96];
    if (statusSystemName[0])
#if defined(NSO_RELEASE) && !defined(NSO_SHOW_ALL_SYSTEMS)
        snprintf(heading, sizeof(heading), "%s %s", statusSystemName, T(S_TAB_STATS));   /* "NES Port status" is about source ports */
#else
        snprintf(heading, sizeof(heading), "%s %s", statusSystemName, Status_Word(0));
#endif
    else
        snprintf(heading, sizeof(heading), "%s", Status_Word(0));
    Details_Text(font, heading, (Vector2){ lx, card.y + content }, sh * 0.030f, lcw, Fade(meta, a));
    float py = card.y + content + sh * 0.055f, pad = lcw * 0.055f;
    float legendRow = sh * 0.062f;
    Rectangle pc = { lx, py, lcw, sh * 0.19f + legendRow * Status_ShownTypes() };
    DrawRectangleRounded(pc, 2.0f * rad / pc.height * 2.0f, 12, Fade(inner, a));

    float ix = pc.x + pad, iw = pc.width - 2.0f * pad, y = pc.y + sh * 0.025f;
    int pct = Status_Percent(statusPlayable, statusTotal);
    char big[16], rest[48];
    snprintf(big, sizeof(big), "%d%%", pct);
    snprintf(rest, sizeof(rest), " %s", Status_Word(1));
    float bs = sh * 0.052f, ss = sh * 0.026f;
    Status_Bold(font, big, (Vector2){ ix, y }, bs, Fade(ink, a));
    float bw = MeasureTextEx(font, big, bs, 2.0f).x;
    DrawTextEx(font, rest, (Vector2){ ix + bw, y + bs - ss * 1.15f }, ss, 2.0f, Fade(light, a));

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

    float ly = by + bh + sh * 0.025f, ds = sh * 0.011f, ts = sh * 0.023f;
    int shownN = Status_ShownTypes(), shownI = 0;
    for (int t = 0; t < ST_COUNT; t++) {
        if (!STATUS_TYPE_SHOWN(t)) continue;
        float ry = ly + legendRow * (float)shownI, cy = ry + legendRow * 0.5f;
        DrawCircleV((Vector2){ ix + ds, cy }, ds, Fade(statusColors[t], a));
        float nx = ix + ds * 3.0f;
        char cnt[48];
        snprintf(cnt, sizeof(cnt), Status_Word(4), statusCounts[t], Status_Percent(statusCounts[t], statusTotal));
        Status_TextR(font, cnt, ix + iw, cy - ts * 0.5f, ts, iw * 0.5f, Fade(light, a));
        float cw = MeasureTextEx(font, cnt, ts, 2.0f).x;
        Details_Text(font, Status_TypeName((StatusType)t), (Vector2){ nx, cy - ts * 0.5f }, ts,
                     iw - ds * 3.0f - cw - 12.0f, Fade(ink, a));
        shownI++;
        if (shownI < shownN)
            DrawLineEx((Vector2){ nx, ry + legendRow }, (Vector2){ ix + iw, ry + legendRow }, 1.0f, Fade(line, a));
    }

    float rx = card.x + lw + sw * 0.025f, rr = card.x + card.width - sw * 0.025f;
    float rowH = sh * 0.046f, top = card.y + content, fs = sh * 0.026f;
    int vis = Status_Visible();
    float typeX = rx + (rr - rx) * 0.60f;
    for (int k = 0; k < vis && statusTop + k < statusRowCount; k++) {
        const StatusRow *r = &statusRows[statusTop + k];
        float ry = top + rowH * (float)k, ty = ry + (rowH - fs) * 0.5f;
        DrawCircleV((Vector2){ rx + fs * 0.4f, ry + rowH * 0.5f }, fs * 0.3f, Fade(statusColors[r->type], a));
        Details_Text(font, r->game->title, (Vector2){ rx + fs * 1.1f, ty }, fs, typeX - rx - fs * 1.5f, Fade(ink, a));
        Details_Text(font, Status_TypeName(r->type), (Vector2){ typeX, ty }, fs * 0.9f, rr - typeX - sw * 0.1f, Fade(meta, a));
        Status_TextR(font, Status_Word(r->playable ? 2 : 3), rr, ty, fs * 0.9f, sw * 0.095f,
                     Fade(r->playable ? statusColors[ST_NATIVE] : light, a));
        if (k < vis - 1)
            DrawLineEx((Vector2){ rx + fs * 1.1f, ry + rowH }, (Vector2){ rr, ry + rowH }, 1.0f, Fade(line, a * 0.7f));
    }
}

static void Status_DrawPlaceholder(Font font, Rectangle card, float lw, float a, Color ink, Color meta) {
    float sh = (float)GetScreenHeight();
    float content = Status_ContentOff(sh);
    float lx = card.x + lw * 0.07f, lcw = lw * 0.86f;
    const char *head = T(statusTab == STATUS_TAB_PORTS ? S_TAB_PORTS : S_TAB_DECOMPS);
    Details_Text(font, head, (Vector2){ lx, card.y + content }, sh * 0.030f, lcw, Fade(meta, a));
    Details_Text(font, T(S_TAB_PLACEHOLDER), (Vector2){ lx, card.y + content + sh * 0.055f },
                 sh * 0.028f, card.width - lw * 0.14f, Fade(ink, a));
}

static void Status_DrawEmuPreview(Font font, Rectangle card, float lw, float a,
                                 Color ink, Color inner, float artTop) {
    float sh = (float)GetScreenHeight();
    float lx = card.x + lw * 0.07f, lcw = lw * 0.86f;
    float bot = card.y + card.height - sh * 0.018f;
    int cat = Status_EmuFocusCatalog();
    const char *title = NULL;
    Texture2D tex;
    float artH, pad;
    Rectangle artBox;

    artH = bot - artTop - sh * 0.070f;
    if (artH < sh * 0.16f) artH = bot - artTop;
    if (artH < 1.0f) return;
    pad = lcw * 0.055f;
    artBox = (Rectangle){ lx, artTop, lcw, artH };
    DrawRectangleRounded(artBox, 0.04f, 12, Fade(inner, a));
    tex = Status_EmuArt(cat);
    if (tex.id != 0 && tex.width > 0 && tex.height > 0) {
        float ox, oy, dw, dh;
        Rectangle slot = { artBox.x + pad, artBox.y + pad,
                           artBox.width - 2.0f * pad, artBox.height - 2.0f * pad };
        CoverDestInSlot(slot, tex, 1.0f, 0.0f, &ox, &oy, &dw, &dh);
        DrawTexturePro(tex,
                       (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                       (Rectangle){ ox, oy, dw, dh },
                       (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, a));
    }
    if (cat >= 0 && cat < ActiveGameTotal() && ActiveGames()[cat].title)
        title = ActiveGames()[cat].title;
    if (title && title[0] && artTop + artH + sh * 0.036f < bot + sh * 0.004f)
        Details_Text(font, title, (Vector2){ lx, artTop + artH + sh * 0.012f },
                     sh * 0.024f, lcw, Fade(ink, a));
}

static void Status_DrawEmu(Font font, Rectangle card, float lw, float a,
                           Color ink, Color meta, Color line, Color inner) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float content = Status_ContentOff(sh);
    float top = card.y + content;
    float bot = card.y + card.height - sh * 0.018f;
    float lx = card.x + lw * 0.07f, lcw = lw * 0.86f;
    float rx = card.x + lw + sw * 0.025f, rr = card.x + card.width - sw * 0.025f;
    float rw = rr - rx;
    int choosing = ScrapeMenu_Choosing();
    float artTop;
    float rowH = sh * 0.070f, bh = sh * 0.056f;

    if (scrapeBusy) statusEmuSawBusy = 1;
    if ((statusEmuNeedRefresh || statusEmuSawBusy) && !scrapeBusy && !gsOpen) {
        Status_EmuRefreshRoms();
        Status_EmuUnloadPrev();
        statusEmuNeedRefresh = 0;
        statusEmuSawBusy = 0;
    }

    /* Align the remaining left action with the first row of the right-hand
     * list (same y, row height and box size). Preview starts below it. */
    artTop = top;
    if (!gsOpen && !choosing) {
        char status[192] = "";
        float y = top + sh * 0.036f;
        Rectangle box = { lx, y - bh * 0.5f, lcw, bh };
        DrawRectangleRoundedLinesEx(box, 0.3f, 6, 1.5f, Fade(line, a));
        Details_Text(font, T(S_EMU_DOWNLOAD_ALL), (Vector2){ lx + sh * 0.018f, y - sh * 0.014f },
                     sh * 0.026f, lcw - sh * 0.03f, Fade(ink, a));
        artTop = top + sh * 0.036f + rowH * (float)STATUS_EMU_ACTIONS + sh * 0.01f;
        Msg_Format(&scrapeMsg, status, sizeof(status));
        if (status[0]) {
            Details_Text(font, status, (Vector2){ lx, artTop }, sh * 0.024f, lcw, Fade(meta, a));
            artTop += sh * 0.040f;
        }
        artTop += sh * 0.012f;
    }

    Status_DrawEmuPreview(font, card, lw, a, ink, inner, artTop);

    if (gsOpen) {
        Rectangle area = { rx, top, rw, bot - top };
        GameSel_DrawInCard(font, area, ink, meta, line, inner, ink, a);
        return;
    }

    if (choosing) {
        float fs = sh * 0.026f, row = sh * 0.052f;
        int n = ScrapeMenu_ChoiceCount();
        int cur = ScrapeMenu_ChoiceCur();
        const char *title = ScrapeMenu_ChoiceTitle();
        Details_Text(font, T(S_EMU_CHOOSE), (Vector2){ rx, top }, sh * 0.030f, rw, Fade(meta, a));
        if (title && title[0])
            Details_Text(font, title, (Vector2){ rx, top + sh * 0.040f }, sh * 0.026f, rw, Fade(ink, a));
        for (int i = 0; i < n; i++) {
            char lab[256];
            float y = top + sh * 0.090f + row * (float)i;
            bool on = (i == cur);
            Rectangle box = { rx, y, rw, row * 0.88f };
            Color tc = Fade(on ? (Color){ 255, 255, 255, 255 } : ink, a);
            if (on) DrawRectangleRounded(box, 0.3f, 6, Fade(ink, a));
            Status_ChoiceLabel(ScrapeMenu_ChoiceName(i), lab, sizeof(lab));
            Details_Text(font, lab, (Vector2){ rx + sh * 0.016f, y + (row * 0.88f - fs) * 0.5f },
                         fs, rw - sh * 0.03f, tc);
        }
        return;
    }

    {
        float listY = top, thW;
        int vis, k, focus;

        if (statusEmuRomN == 0) {
            char miss[192];
            ScrapeMenu_NoRomsText(miss, sizeof(miss));
            Details_Text(font, miss, (Vector2){ rx, listY + sh * 0.008f },
                         sh * 0.026f, rw, Fade(meta, a));
            return;
        }

        vis = (int)((bot - listY - sh * 0.01f) / rowH);
        if (vis < 1) vis = 1;
        focus = statusEmuRow;
        if (focus >= 0) {
            if (focus < statusEmuListTop) statusEmuListTop = focus;
            if (focus >= statusEmuListTop + vis) statusEmuListTop = focus - vis + 1;
        }
        if (statusEmuListTop < 0) statusEmuListTop = 0;
        if (statusEmuRomN > vis && statusEmuListTop > statusEmuRomN - vis)
            statusEmuListTop = statusEmuRomN - vis;

        thW = bh * 0.72f;
        for (k = 0; k < vis && statusEmuListTop + k < statusEmuRomN; k++) {
            int idx = statusEmuRoms[statusEmuListTop + k];
            float y = listY + sh * 0.036f + rowH * (float)k;
            bool cur = !scrapeBusy && statusEmuRow == statusEmuListTop + k;
            Rectangle box = { rx, y - bh * 0.5f, rw, bh };
            Color tc = Fade(cur ? (Color){ 255, 255, 255, 255 } : ink, a);
            Texture2D tex;
            const char *name = (idx >= 0 && idx < ActiveGameTotal() && ActiveGames()[idx].title)
                               ? ActiveGames()[idx].title : "";
            if (cur) DrawRectangleRounded(box, 0.3f, 6, Fade(ink, a));
            else DrawRectangleRoundedLinesEx(box, 0.3f, 6, 1.5f, Fade(line, a));
            tex = GetBoxart(idx);
            if (tex.id != 0 && tex.width > 0 && tex.height > 0) {
                float ox, oy, dw, dh;
                Rectangle slot = { rx + sh * 0.010f, y - bh * 0.38f, thW, bh * 0.76f };
                CoverDestInSlot(slot, tex, 1.0f, 0.0f, &ox, &oy, &dw, &dh);
                DrawTexturePro(tex,
                               (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                               (Rectangle){ ox, oy, dw, dh },
                               (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, a));
            }
            Details_Text(font, name,
                         (Vector2){ rx + sh * 0.016f + thW, y - sh * 0.014f },
                         sh * 0.026f, rw - thW - sh * 0.032f, tc);
        }
    }
}

static void Status_Draw(Font font) {
    if (statusT <= 0.0f) return;
    float a = statusT;
    const Color ink = { 44, 49, 50, 255 }, meta = { 92, 97, 98, 255 };
    const Color light = { 128, 134, 136, 255 }, track = { 214, 220, 222, 255 };
    const Color line = { 214, 220, 222, 255 }, inner = { 250, 252, 252, 255 };
    Rectangle card;
    float lw, rad;
    Details_DrawFrameH(a, &card, &lw, &rad, Status_CardHeight((float)GetScreenHeight()));
    Status_DrawTabs(font, card, lw, a, ink, meta, line);

    if (statusTab == STATUS_TAB_STATS)
        Status_DrawStats(font, card, lw, rad, a, ink, meta, light, track, line, inner);
    else if (statusTab == STATUS_TAB_EMU)
        Status_DrawEmu(font, card, lw, a, ink, meta, line, inner);
    else
        Status_DrawPlaceholder(font, card, lw, a, ink, meta);

    InfoBar_Draw(font);
}

#endif /* NSO_STATUSPANEL_H */
