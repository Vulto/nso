/* sortpanel.h - "Change order of titles" panel (grid T / right stick right).
 * Included by main.c after the Y Move code; uses its statics. The chosen mode
 * is "<id>.sort" in config/nso.cfg; "custom" is the Y Move order file. Play
 * stats (last played, total seconds) live in config/played-<id>.txt. */
#ifndef NSO_SORTPANEL_H
#define NSO_SORTPANEL_H

static SortMode sortMode[sizeof(systems) / sizeof(systems[0])];
static bool sortOpen = false;
static int sortRow = 0;
static float sortW = 0.0f, sortTarget = 0.0f;
static enum { SORT_IDLE = 0, SORT_TO_PEAK, SORT_TO_REST, SORT_TO_OFF } sortPhase = SORT_IDLE;
static Texture2D sortDisc, sortRing;

static bool SortPanel_Active(void) { return sortOpen || sortW > 0.5f || sortPhase != SORT_IDLE; }

static void SortPanel_Path(int sys, char *dst, size_t n) {
    snprintf(dst, n, "%s/config/played-%s.txt", GetBaseDirectory(), systems[sys].platform->id);
}

/* Release dates (YYYYMM) scraped from libretro-database: config/dates-<id>.txt. */
static void SortPanel_DatesPath(int sys, char *dst, size_t n) {
    snprintf(dst, n, "%s/config/dates-%s.txt", GetBaseDirectory(), systems[sys].platform->id);
}

static int SortPanel_DatesLoad(int sys, const char *const *titles, SortKeys *keys, int count) {
    char path[2112];
    SortPanel_DatesPath(sys, path, sizeof(path));
    char *text = FileExists(path) ? LoadFileText(path) : NULL;
    if (!text) return 0;
    Sort_DatesParse(text, titles, keys, count);
    UnloadFileText(text);
    return 1;
}

/* Games of the shown system that still have no date. */
static int SortPanel_DatesMissing(void) {
    int count = *systems[currentSystem].gameCount, missing = 0;
    if (count <= 0 || count > MAX_GAMES) return 0;
    const char *titles[MAX_GAMES];
    SortKeys keys[MAX_GAMES] = { 0 };
    for (int i = 0; i < count; i++) titles[i] = systems[currentSystem].games[i].title;
    SortPanel_DatesLoad(currentSystem, titles, keys, count);
    for (int i = 0; i < count; i++) if (keys[i].release == 0) missing++;
    return missing;
}

static void SortPanel_Refresh(void);

/* Fill the missing dates of the shown system from the two downloaded metadat
 * files (same Scrape_MatchLoose as the covers), then keep the list sorted. */
static void SortPanel_DatesScrape(const char *yearText, const char *monthText) {
    int sys = currentSystem, count = *systems[sys].gameCount;
    char **yn = NULL, **mn = NULL;
    int *yv = NULL, *mv = NULL, ny = 0, nm = 0;
    if (count <= 0 || count > MAX_GAMES) return;
    if (Scrape_ParseDat(yearText, "releaseyear", &yn, &yv, &ny) != 0) return;
    if (Scrape_ParseDat(monthText, "releasemonth", &mn, &mv, &nm) != 0) {
        Scrape_FreeNames(yn, ny);
        free(yv);
        return;
    }
    const char *titles[MAX_GAMES];
    SortKeys keys[MAX_GAMES] = { 0 };
    for (int i = 0; i < count; i++) titles[i] = systems[sys].games[i].title;
    SortPanel_DatesLoad(sys, titles, keys, count);
    for (int i = 0; i < count; i++) {
        if (keys[i].release != 0) continue;
        const char *hit = Scrape_MatchLoose(titles[i], (const char **)yn, ny);
        for (int a = 0; hit && a < ny; a++) {
            if (yn[a] != hit) continue;
            int month = 0;
            for (int b = 0; b < nm; b++) if (strcmp(mn[b], hit) == 0) { month = mv[b]; break; }
            if (yv[a] > 0) keys[i].release = (long long)yv[a] * 100 + month;
            break;
        }
    }
    static char buf[MAX_GAMES * 300];
    char path[2112];
    SortPanel_DatesPath(sys, path, sizeof(path));
    if (Sort_DatesFormat(buf, sizeof(buf), titles, keys, count) != 0 || !SaveFileText(path, buf))
        fprintf(stderr, "NSO: cannot save %s\n", path);
    Scrape_FreeNames(yn, ny);
    Scrape_FreeNames(mn, nm);
    free(yv);
    free(mv);
    SortPanel_Refresh();
}

/* Reorder system `sys` by `mode`. live = it is the shown system, so its cover
 * textures and tile scales travel along and the cursor stays on its game. */
static void SortPanel_Apply(int sys, SortMode mode, bool live) {
    GameEntry *games = systems[sys].games;
    int count = *systems[sys].gameCount, shown = count;
    if (count <= 0 || count > MAX_GAMES) return;
    const char *titles[MAX_GAMES];
    int perm[MAX_GAMES];
    for (int i = 0; i < count; i++) titles[i] = games[i].title;
    if (mode == SORT_CUSTOM) {
        char path[2112];
        Order_Path(sys, path, sizeof(path));
        char *text = FileExists(path) ? LoadFileText(path) : NULL;
        Order_Perm(text ? text : "", titles, count, perm);
        if (text) UnloadFileText(text);
    } else {
        SortKeys keys[MAX_GAMES] = { 0 };   /* release 0 = unknown: no date scraped yet */
        char path[2112];
        SortPanel_Path(sys, path, sizeof(path));
        char *text = FileExists(path) ? LoadFileText(path) : NULL;
        if (text) { Sort_StatsParse(text, titles, keys, count); UnloadFileText(text); }
        if (mode == SORT_RELEASE) SortPanel_DatesLoad(sys, titles, keys, count);
        Sort_Perm(mode, titles, keys, count, perm);
    }
    if (hideOn[sys]) {   /* playable titles first (same order inside each group), the rest hidden */
        bool ok[MAX_GAMES];
        int order[MAX_GAMES];
        for (int i = 0; i < count; i++) { order[i] = perm[i]; ok[i] = GameIsLaunchable(&games[i]); }
        shown = Hide_Perm(ok, order, count, perm);
    }
    hideShown[sys] = shown;
    hideReady[sys] = hideOn[sys];
    GameEntry tmp[MAX_GAMES];
    for (int k = 0; k < count; k++) tmp[k] = games[perm[k]];
    memcpy(games, tmp, sizeof(GameEntry) * (size_t)count);
    if (!live) return;
    Texture2D art[MAX_GAMES];
    float scale[MAX_GAMES];
    for (int k = 0; k < count; k++) { art[k] = gameBoxarts[perm[k]]; scale[k] = animScales[perm[k]]; }
    memcpy(gameBoxarts, art, sizeof(Texture2D) * (size_t)count);
    memcpy(animScales, scale, sizeof(float) * (size_t)count);
    for (int k = 0; k < count; k++) if (perm[k] == GameIdx) { GameIdx = k; break; }
    for (int k = shown; k < count; k++) {   /* hidden covers are not kept on the GPU */
        if (gameBoxarts[k].id != 0) UnloadTexture(gameBoxarts[k]);
        gameBoxarts[k] = (Texture2D){ 0 };
    }
    GameIdx = Hide_Clamp(GameIdx, shown);
}

static void Hide_Save(int sys) {
    char dir[2048], cfg[2112], key[64];
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    MakeDirectory(dir);
    Lang_ConfigPath(cfg, sizeof(cfg));
    snprintf(key, sizeof(key), "%s.hide_unplayable", systems[sys].platform->id);
    if (!Cfg_SaveKey(cfg, key, Hide_CfgValue(hideOn[sys]))) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

/* Settings > Hide unplayable: flip the shown system, re-list it at once. */
static void Hide_Toggle(void) {
    hideOn[currentSystem] = !hideOn[currentSystem];
    Hide_Save(currentSystem);
    SortPanel_Apply(currentSystem, sortMode[currentSystem], true);
}

/* Something may have been installed: list the shown system again. */
static void Hide_Resync(bool live) {
    if (hideOn[currentSystem]) SortPanel_Apply(currentSystem, sortMode[currentSystem], live);
}

static void SortPanel_Save(int sys) {
    char cfg[2112], key[64];
    Lang_ConfigPath(cfg, sizeof(cfg));
    snprintf(key, sizeof(key), "%s.sort", systems[sys].platform->id);
    if (!Cfg_SaveKey(cfg, key, Sort_ModeName(sortMode[sys]))) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

/* New dates arrived: re-sort the shown system if it is sorted by them. */
static void SortPanel_Refresh(void) {
    if (sortMode[currentSystem] == SORT_RELEASE) SortPanel_Apply(currentSystem, SORT_RELEASE, true);
}

/* Startup, right after Order_LoadAll (the custom order is already applied). */
static void SortPanel_LoadAll(void) {
    char cfg[2112], v[32];
    Lang_ConfigPath(cfg, sizeof(cfg));
    char *text = FileExists(cfg) ? LoadFileText(cfg) : NULL;
    if (!text) return;
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        char key[64];
        snprintf(key, sizeof(key), "%s.sort", systems[s].platform->id);
        if (Cfg_Get(text, key, v, sizeof(v))) sortMode[s] = Sort_ModeParse(v);
        snprintf(key, sizeof(key), "%s.hide_unplayable", systems[s].platform->id);
        if (Cfg_Get(text, key, v, sizeof(v))) hideOn[s] = Hide_CfgParse(v);
        if (sortMode[s] != SORT_CUSTOM || hideOn[s]) SortPanel_Apply(s, sortMode[s], false);
    }
    UnloadFileText(text);
}

/* A Y Move was saved: the list is now the user's own order. */
static void SortPanel_SetCustom(void) {
    if (sortMode[currentSystem] == SORT_CUSTOM) return;
    sortMode[currentSystem] = SORT_CUSTOM;
    SortPanel_Save(currentSystem);
}

/* One call per frame: records last played + play time when a game ends. */
static void SortPanel_Play(bool inGame) {
    static bool playing = false;
    static int sys = 0;
    static const char *title = NULL;
    static double clock0 = 0.0;
    if (inGame && !playing) {
        playing = true;
        sys = currentSystem;
        title = launchingGameIdx < ActiveGameCount() ? ActiveGames()[launchingGameIdx].title : NULL;
        clock0 = GetTime();
    } else if (!inGame && playing) {
        playing = false;
        int count = *systems[sys].gameCount;
        if (!title || count > MAX_GAMES) return;
        static char buf[MAX_GAMES * 300];
        const char *titles[MAX_GAMES];
        SortKeys keys[MAX_GAMES] = { 0 };
        char path[2112];
        for (int i = 0; i < count; i++) titles[i] = systems[sys].games[i].title;
        SortPanel_Path(sys, path, sizeof(path));
        char *text = FileExists(path) ? LoadFileText(path) : NULL;
        if (text) { Sort_StatsParse(text, titles, keys, count); UnloadFileText(text); }
        for (int i = 0; i < count; i++) {
            if (titles[i] && strcmp(titles[i], title) == 0) {
                keys[i].lastPlayed = (long long)time(NULL);
                keys[i].playSecs += (long long)(GetTime() - clock0);
                break;
            }
        }
        if (Sort_StatsFormat(buf, sizeof(buf), titles, keys, count) != 0 || !SaveFileText(path, buf))
            fprintf(stderr, "NSO: cannot save %s\n", path);
    }
}

static void SortPanel_Open(void) {
    sortOpen = true;
    sortRow = (int)sortMode[currentSystem];
    sortPhase = SORT_TO_PEAK;
    sortTarget = SidebarOpenWidth() * (1.0f + SIDEBAR_OVERSHOOT);
    InfoBar_Request(INFO_BAR_SIDEBAR);   /* same "Back / OK" hints */
}

static void SortPanel_Close(void) {
    sortOpen = false;
    sortPhase = SORT_TO_PEAK;   /* same bounce as the options menu, then off */
    sortTarget = SidebarOpenWidth() * (1.0f + SIDEBAR_OVERSHOOT);
    InfoBar_Request(INFO_BAR_MOSAIC);
}

static void SortPanel_Input(bool gpUp, bool gpDown, bool gpConfirm, bool gpBack) {
    if (!sortOpen) return;
    if (IsKeyPressed(KEY_B) || gpBack) { SortPanel_Close(); return; }
    if (IsKeyPressed(KEY_UP) || gpUp) { sortRow = (sortRow + SORT_COUNT - 1) % SORT_COUNT; Sfx_Play(SFX_PLUCK); }
    if (IsKeyPressed(KEY_DOWN) || gpDown) { sortRow = (sortRow + 1) % SORT_COUNT; Sfx_Play(SFX_PLUCK); }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm) {
        Sfx_Play(SFX_CLICK);
        sortMode[currentSystem] = (SortMode)sortRow;
        SortPanel_Save(currentSystem);
        SortPanel_Apply(currentSystem, sortMode[currentSystem], true);
        SortPanel_Close();
    }
}

/* Same width animation as the options menu: lerp to a peak past full width,
 * settle; closing goes to the peak, then off the screen. */
static void SortPanel_Update(void) {
    float full = SidebarOpenWidth();
    if (sortPhase == SORT_TO_PEAK) {
        if (fabsf(sortW - sortTarget) < 1.0f) {
            sortW = sortTarget;
            sortPhase = sortOpen ? SORT_TO_REST : SORT_TO_OFF;
            sortTarget = sortOpen ? full : 0.0f;
        }
    } else if (sortPhase == SORT_TO_REST) {
        sortTarget = full;
        if (fabsf(sortW - full) < 1.0f) { sortW = full; sortPhase = SORT_IDLE; }
    } else if (sortPhase == SORT_TO_OFF) {
        sortTarget = 0.0f;
        if (sortW < 1.0f) { sortW = 0.0f; sortPhase = SORT_IDLE; }
    } else if (sortOpen) {
        sortTarget = full;
    }
    sortW += (sortTarget - sortW) * SIDEBAR_WIDTH_LERP;
    if (fabsf(sortW - sortTarget) < 1.0f) sortW = sortTarget;
}

/* White disc / ring with analytic edge coverage (4x4 samples per texel),
 * drawn scaled with trilinear filtering: smooth with or without MSAA. */
static Texture2D SortPanel_CircleTex(float inner) {
    enum { N = 128, SS = 4 };
    Image img = GenImageColor(N, N, BLANK);
    Color *px = img.data;
    float R = N * 0.5f - 1.0f;
    for (int y = 0; y < N; y++) {
        for (int x = 0; x < N; x++) {
            int hit = 0;
            for (int sy = 0; sy < SS; sy++) {
                for (int sx = 0; sx < SS; sx++) {
                    float dx = (float)x + ((float)sx + 0.5f) / SS - N * 0.5f;
                    float dy = (float)y + ((float)sy + 0.5f) / SS - N * 0.5f;
                    float d = sqrtf(dx * dx + dy * dy);
                    if (d <= R && d >= inner * R) hit++;
                }
            }
            px[y * N + x] = (Color){ 255, 255, 255, (unsigned char)(255 * hit / (SS * SS)) };
        }
    }
    Texture2D t = LoadTextureFromImage(img);
    UnloadImage(img);
    GenTextureMipmaps(&t);
    SetTextureFilter(t, TEXTURE_FILTER_TRILINEAR);
    return t;
}

static void SortPanel_Circle(Texture2D t, Vector2 c, float r, Color col) {
    float d = 2.0f * r * 128.0f / 126.0f;   /* texture radius is 63 of 64 */
    DrawTexturePro(t, (Rectangle){ 0, 0, 128, 128 }, (Rectangle){ c.x - d * 0.5f, c.y - d * 0.5f, d, d },
                   (Vector2){ 0 }, 0.0f, col);
}

static void SortPanel_Draw(Font font) {
    SortPanel_Update();
    if (sortW < 1.0f) return;
    if (!sortDisc.id) { sortDisc = SortPanel_CircleTex(0.0f); sortRing = SortPanel_CircleTex(0.82f); }
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float px = sw - sortW;
    const Color bg = { 247, 247, 247, 255 }, band = { 224, 224, 224, 255 }, head = { 120, 120, 120, 255 },
                ink = { 70, 70, 70, 255 }, red = { 235, 75, 55, 255 }, grey = { 150, 150, 150, 255 };
    static const StrId labels[SORT_COUNT] = { S_SORT_CUSTOM, S_SORT_TITLE, S_SORT_RELEASE, S_SORT_PLAYED, S_SORT_TIME };
    DrawRectangle((int)px, 0, (int)sortW + 1, (int)sh, bg);
    DrawFitText(font, T(S_SORT_HEAD), (Vector2){ px + sw * 0.0732f - sw * 0.0312f, sh * 0.0934f }, (float)font.baseSize * 0.85f,
                sw - px - sw * 0.05f, head);
    float bh = sh * 0.066f, r = sh * 0.0165f;
    for (int k = 0; k < SORT_COUNT; k++) {
        float y = sh * (0.1574f + 0.0849f * (float)k);
        bool cur = (k == sortRow), on = (k == (int)sortMode[currentSystem]);
        if (cur) DrawRectangle((int)px, (int)roundf(y - bh * 0.5f), (int)sortW + 1, (int)roundf(bh), band);
        Vector2 c = { px + sw * 0.0518f, y };
        SortPanel_Circle(sortRing, c, r, on ? red : grey);
        if (on) SortPanel_Circle(sortDisc, c, r * 0.52f, red);
        DrawFitText(font, T(labels[k]), (Vector2){ px + sw * 0.0732f, y }, (float)font.baseSize * 1.2f,
                    sw - px - sw * 0.08f, cur ? red : ink);
    }
}

static void SortPanel_Unload(void) {
    if (sortDisc.id) UnloadTexture(sortDisc);
    if (sortRing.id) UnloadTexture(sortRing);
    sortDisc = sortRing = (Texture2D){ 0 };
}

#endif /* NSO_SORTPANEL_H */
