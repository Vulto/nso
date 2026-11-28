/* gamesel_ui.h - "Select games" panel (system sidebar > Settings > Image
 * scraping > Select games). Included by main.c after the scrape page; uses its
 * statics. Lists the ROMs in roms/<folder> (read only), one check per
 * game; for SNES, GameCube and Mega Drive it lists the wiki artbox titles
 * (assets/data/<system>-titles.txt) and can filter them by Released in USA /
 * Released in Japan / all.
 * "Download and install" fetches the checked games' covers + dates (the
 * scrape, limited to them — or, in wiki-list mode with nothing checked, the
 * current region group) while the ports of the games with a pipeline (gamesel.h)
 * download two at a time and build one at a time through
 * tools/game_install.sh (log: logs/install/<name>.log). A built game gets its
 * config key and is playable at once. B cancels a run. */
#ifndef NSO_GAMESEL_UI_H
#define NSO_GAMESEL_UI_H

#define GS_MAX_ROWS SYSLIST_MAX
#define GS_FETCH_PARALLEL 2

static bool gsOpen = false;
static int gsSys = -1;
static GsRow gsRows[GS_MAX_ROWS];
static char gsRomRel[GS_MAX_ROWS][320];   /* roms/<folder>/<file> */
static char gsLabel[GS_MAX_ROWS][160];
static int gsCount = 0;
static int gsCursor = 0;                  /* 0 = Start, 1.. = rows */
static int gsTop = 0;                     /* first row shown */
static NsoProc gsFetch[GS_MAX_ROWS];
static NsoProc gsBuild = -1;
static int gsBuildRow = -1;
static bool gsCoverRun = false;
static bool gsWasRunning = false;
static NsoMsg gsMsg = { -1, 0, "" };
static SysListGame gsWiki[SYSLIST_MAX];
static int gsWikiN = 0;
static int gsWikiOf[SYSLIST_MAX];   /* visible row -> gsWiki index, or -1 */
static SysListRegion gsRegion = SYSLIST_ALL;
static bool gsWikiMode = false;
static bool gsCoverOnly = false;   /* Emulated tab: scrape covers, no install */

static int GameSel_StartCursor(void) { return gsWikiMode ? 1 : 0; }
static int GameSel_FirstGame(void) { return GameSel_StartCursor() + 1; }

static int GameSel_FocusCatalog(void)
{
    int i = gsCursor - GameSel_FirstGame();
    if (i < 0 || i >= gsCount) return -1;
    return gsRows[i].game;
}

static int GameSel_CatalogOf(const char *title, const char *alias)
{
    int gc = ActiveGameTotal();
    GameEntry *games = ActiveGames();
    char nt[256], na[256], ng[256];
    Gs_Norm(title ? title : "", nt, sizeof(nt));
    if (alias && alias[0]) Gs_Norm(alias, na, sizeof(na));
    else na[0] = '\0';
    for (int i = 0; i < gc; i++) {
        if (!games[i].title) continue;
        if (strcmp(games[i].title, title) == 0) return i;
        if (alias && alias[0] && strcmp(games[i].title, alias) == 0) return i;
        Gs_Norm(games[i].title, ng, sizeof(ng));
        if (nt[0] && strcmp(ng, nt) == 0) return i;
        if (na[0] && strcmp(ng, na) == 0) return i;
    }
    return -1;
}

static void GameSel_CoverDest(const SysListGame *g, int catalog, char *dst, size_t n)
{
    const char *sysId = ActiveSystem()->platform->id;
    if (catalog >= 0) {
        const char *rel = ActiveGames()[catalog].boxartPath;
        if (rel && rel[0]) { snprintf(dst, n, "%s", rel); return; }
    }
    char slug[SYSLIST_NAME];
    SysList_Slug(g && g->title[0] ? g->title : "untitled", slug, sizeof(slug));
    if (!slug[0]) snprintf(slug, sizeof(slug), "untitled");
    snprintf(dst, n, "assets/images/systems/%s/%s.png", sysId ? sysId : "snes", slug);
}

static void GameSel_RebuildWikiRows(void)
{
    int idx[SYSLIST_MAX];
    int n, i;
    const char *sysId = ActiveSystem()->platform->id;
    GameEntry *games = ActiveGames();
    n = SysList_Filter(gsWiki, gsWikiN, gsRegion, idx, SYSLIST_MAX);
    if (n > GS_MAX_ROWS) n = GS_MAX_ROWS;
    for (i = 0; i < n; i++) {
        const SysListGame *g = &gsWiki[idx[i]];
        GsRow *r = &gsRows[i];
        r->game = GameSel_CatalogOf(g->title, g->alias[0] ? g->alias : NULL);
        r->pipe = r->game >= 0 ? Gs_FindPipeline(sysId, games[r->game].title) : -1;
        r->on = false;
        r->st = GS_IDLE;
        gsWikiOf[i] = idx[i];
        gsRomRel[i][0] = '\0';
        snprintf(gsLabel[i], sizeof(gsLabel[i]), "%s", g->title);
    }
    gsCount = n;
    if (gsCursor > GameSel_StartCursor() + gsCount)
        gsCursor = GameSel_StartCursor() + gsCount;
}

static int GsCmpName(const void *a, const void *b) {
    return strcmp(*(const char *const *)a, *(const char *const *)b);
}

/* Scan the system's ROM folder and match each ROM to a catalog title.
 * SNES / GameCube / Mega Drive: show the wiki artbox list (filterable by region). */
static void GameSel_Open(void) {
    const char *folder = Gs_RomFolder(ActiveSystem()->platform->id);
    char dir[2048];
    static char names[GS_MAX_ROWS][256];
    const char *sorted[GS_MAX_ROWS] = { 0 }, *bases[GS_MAX_ROWS] = { 0 }, *titles[MAX_GAMES] = { 0 };
    static char baseBuf[GS_MAX_ROWS][256];
    int matched[GS_MAX_ROWS], n = 0, gc = ActiveGameTotal();   /* hidden titles are the ones to install */
    GameEntry *games = ActiveGames();

    gsOpen = true;
    gsSys = currentSystem;
    gsCount = 0;
    gsCursor = 0;
    gsTop = 0;
    gsWikiMode = false;
    gsCoverOnly = false;
    gsWikiN = 0;
    Msg_Clear(&gsMsg);
    for (int i = 0; i < GS_MAX_ROWS; i++) gsFetch[i] = NsoProcNone();

    {
        const char *id = ActiveSystem()->platform->id;
        const char *rel = NULL;
        if (id && strcmp(id, "snes") == 0) rel = "assets/data/snes-titles.txt";
        else if (id && strcmp(id, "gamecube") == 0) rel = "assets/data/gamecube-titles.txt";
        else if (id && strcmp(id, "megadrive") == 0) rel = "assets/data/megadrive-titles.txt";
        if (rel) {
            char path[2112];
            snprintf(path, sizeof(path), "%s", GetDataPath(rel));
            char *text = FileExists(path) ? LoadFileText(path) : NULL;
            if (text) {
                int got = SysList_Parse(text, gsWiki, SYSLIST_MAX);
                UnloadFileText(text);
                if (got > 0) {
                    gsWikiN = got;
                    gsWikiMode = true;
                    GameSel_RebuildWikiRows();
                    return;
                }
            }
        }
    }

    if (!folder) return;
    snprintf(dir, sizeof(dir), "%s/%s/%s", GetBaseDirectory(), Gs_RomRoot(), folder);
    if (!DirectoryExists(dir)) return;
    FilePathList files = LoadDirectoryFiles(dir);
    for (unsigned i = 0; i < files.count && n < GS_MAX_ROWS; i++) {
        if (!IsPathFile(files.paths[i])) continue;
        snprintf(names[n], sizeof(names[n]), "%s", GetFileName(files.paths[i]));
        sorted[n] = names[n];
        n++;
    }
    UnloadDirectoryFiles(files);
    qsort(sorted, (size_t)n, sizeof(sorted[0]), GsCmpName);
    for (int i = 0; i < n; i++) {
        Gs_RomBase(sorted[i], baseBuf[i], sizeof(baseBuf[i]));
        bases[i] = baseBuf[i];
    }
    if (gc > MAX_GAMES) gc = MAX_GAMES;
    for (int i = 0; i < gc; i++) titles[i] = games[i].title ? games[i].title : "";
    Gs_MatchAll(bases, n, titles, gc, matched);
    for (int i = 0; i < n; i++) {
        GsRow *r = &gsRows[i];
        r->game = matched[i];
        r->pipe = matched[i] >= 0 ? Gs_RowPipeline(ActiveSystem()->platform->id, bases[i], titles[matched[i]]) : -1;
        r->on = false;
        r->st = GS_IDLE;
        snprintf(gsRomRel[i], sizeof(gsRomRel[i]), "%s/%s/%s", Gs_RomRoot(), folder, sorted[i]);
        snprintf(gsLabel[i], sizeof(gsLabel[i]), "%s", matched[i] >= 0 ? titles[matched[i]] : bases[i]);
    }
    gsCount = n;
}

/* ROM-folder list only (no wiki titles). Used by the Emulated tab. */
static void GameSel_OpenRoms(void) {
    int idx[GS_MAX_ROWS];
    int n, i, gc = ActiveGameTotal();
    GameEntry *games = ActiveGames();

    gsOpen = true;
    gsSys = currentSystem;
    gsCount = 0;
    gsCursor = 0;
    gsTop = 0;
    gsWikiMode = false;
    gsCoverOnly = true;
    gsWikiN = 0;
    Msg_Clear(&gsMsg);
    for (i = 0; i < GS_MAX_ROWS; i++) gsFetch[i] = NsoProcNone();

    if (gc > MAX_GAMES) gc = MAX_GAMES;
    n = ScrapeMenu_ListRoms(idx, GS_MAX_ROWS);
    n = ScrapeMenu_AddCatalogCovers(idx, n, GS_MAX_ROWS);
    for (i = 0; i < n; i++) {
        int cat = idx[i];
        GsRow *r = &gsRows[i];
        r->game = cat;
        r->pipe = (cat >= 0 && cat < gc && games[cat].title)
            ? Gs_FindPipeline(ActiveSystem()->platform->id, games[cat].title) : -1;
        r->on = false;
        r->st = GS_IDLE;
        if (cat >= 0 && cat < gc && games[cat].rom && games[cat].rom[0])
            snprintf(gsRomRel[i], sizeof(gsRomRel[i]), "%s", games[cat].rom);
        else
            gsRomRel[i][0] = '\0';
        snprintf(gsLabel[i], sizeof(gsLabel[i]), "%s",
                 (cat >= 0 && cat < gc && games[cat].title) ? games[cat].title : "");
    }
    gsCount = n;
}

/* Kill a step and everything it started (its own session: setsid). */
static void GameSel_Kill(NsoProc p) {
#ifndef _WIN32
    if (p > 0) kill(-p, SIGTERM);
#else
    NsoSpawnKill(p);
#endif
}

static NsoProc GameSel_Spawn(char **args) {
    char *argv[12];
    int k = 0;
#ifndef _WIN32
    argv[k++] = "setsid";
#endif
    argv[k++] = "sh";
    argv[k++] = "tools/game_install.sh";
    for (int i = 0; args[i] && k < 11; i++) argv[k++] = args[i];
    argv[k] = NULL;
    return NsoSpawnArgv(GetBaseDirectory(), argv);
}

static void GameSel_LogPath(const GamePipeline *p, char *dst, size_t n) {
    snprintf(dst, n, "logs/install/%s.log", p->name);
}

static bool GameSel_StartFetch(int r) {
    const GamePipeline *p = &gs_pipelines[gsRows[r].pipe];
    char log[256];
    GameSel_LogPath(p, log, sizeof(log));
    char *args[] = { "fetch", log, (char *)p->dir, (char *)p->url, (char *)(p->pin ? p->pin : "-"), NULL };
    gsFetch[r] = GameSel_Spawn(args);
    return NsoProcOk(gsFetch[r]);
}

static bool GameSel_StartBuild(int r) {
    const GamePipeline *p = &gs_pipelines[gsRows[r].pipe];
    char log[256], rom[2400];
    GameSel_LogPath(p, log, sizeof(log));
    snprintf(rom, sizeof(rom), "%s/%s", GetBaseDirectory(), gsRomRel[r]);
    char *args[] = { "build", log, (char *)p->build, (char *)p->binary, rom, NULL };
    gsBuild = GameSel_Spawn(args);
    gsBuildRow = r;
    return NsoProcOk(gsBuild);
}

/* A built game: point its config key at the data unless it already points
 * at something that exists (the user's own setup wins). */
static void GameSel_Configure(int r) {
    const GamePipeline *p = &gs_pipelines[gsRows[r].pipe];
    const char *value = Gs_CfgValue(p, gsRomRel[r]);
    char cfg[2112], cur[1024] = "", path[2400];
    if (!p->cfgKey || !value) return;
    Lang_ConfigPath(cfg, sizeof(cfg));
    char *text = FileExists(cfg) ? LoadFileText(cfg) : NULL;
    bool have = text && Cfg_Get(text, p->cfgKey, cur, sizeof(cur)) && cur[0];
    if (text) UnloadFileText(text);
    if (have) {
        if (cur[0] == '/') snprintf(path, sizeof(path), "%s", cur);
        else snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), cur);
        if (FileExists(path) || DirectoryExists(path)) return;
    }
    if (strcmp(value, gsRomRel[r]) != 0) {   /* a directory of ours: make it */
        snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), value);
        if (!DirectoryExists(path)) MakeDirectory(path);
    }
    snprintf(path, sizeof(path), "%s/config", GetBaseDirectory());
    MakeDirectory(path);
    Cfg_SaveKey(cfg, p->cfgKey, value);
}

/* A wiki title with a downloaded cover counts as ready and is added to that
 * system's games grid (display-only; Hide unplayable can still hide it). */
static void GameSel_FinishCovers(void)
{
    if (gsWikiMode) {
        int added = 0;
        const char *sysId = ActiveSystem()->platform->id;
        for (int i = 0; i < gsCount; i++) {
            char dest[256], full[2304];
            const SysListGame *g;
            if (gsRows[i].st != GS_COVER) continue;
            if (gsWikiOf[i] < 0 || gsWikiOf[i] >= gsWikiN) {
                gsRows[i].st = GS_FAILED;
                continue;
            }
            g = &gsWiki[gsWikiOf[i]];
            GameSel_CoverDest(g, gsRows[i].game, dest, sizeof(dest));
            if (dest[0] && strlen(dest) < 1024)
                snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), dest);
            else
                full[0] = '\0';
            if (full[0] && FileExists(full) && GetFileLength(full) > 0) {
                gsRows[i].st = GS_READY;
                if (Wiki_AddCover(sysId, g->title,
                                  g->alias[0] ? g->alias : NULL, dest) >= 0)
                    added = 1;
            } else {
                gsRows[i].st = GS_FAILED;
            }
        }
        if (added) {
            Hide_Resync(true);
            CoverLoader_Start();
        }
    } else {
        Gs_CoversDone(gsRows, gsCount);
    }
}

static void GameSel_Start(void) {
    int checked = Gs_Checked(gsRows, gsCount);
    bool group = gsWikiMode && checked == 0 && gsCount > 0;
    if (gsCoverOnly) {
        if (scrapeBusy || checked == 0) return;
        Msg_Clear(&gsMsg);
        for (int i = 0; i < MAX_GAMES; i++) scrapeOnly[i] = false;
        scrapeListCount = 0;
        bool any = false;
        for (int i = 0; i < gsCount; i++)
            if (gsRows[i].on && gsRows[i].game >= 0 && gsRows[i].game < MAX_GAMES) {
                scrapeOnly[gsRows[i].game] = true;
                any = true;
            }
        gsCoverRun = false;
        if (any) {
            char cfgDir[2112];
            snprintf(cfgDir, sizeof(cfgDir), "%s/config", GetBaseDirectory());
            MakeDirectory(cfgDir);
            scrapeOnlyOn = true;
            ScrapeMenu_Start(0);
            gsCoverRun = scrapeBusy;
            if (!gsCoverRun) { scrapeOnlyOn = false; }
        }
        if (!gsCoverRun) GameSel_FinishCovers();
        gsWasRunning = true;
        return;
    }
    if (Gs_Running(gsRows, gsCount) || scrapeBusy || (checked == 0 && !group)) return;
    if (!group) Gs_Start(gsRows, gsCount);
    else {
        for (int i = 0; i < gsCount; i++) gsRows[i].st = GS_COVER;
    }
    Msg_Clear(&gsMsg);
    for (int i = 0; i < MAX_GAMES; i++) scrapeOnly[i] = false;
    scrapeListCount = 0;
    bool any = false;
    if (gsWikiMode) {
        for (int i = 0; i < gsCount && scrapeListCount < SYSLIST_MAX; i++) {
            const SysListGame *g;
            if (!group && !gsRows[i].on) continue;
            if (gsWikiOf[i] < 0 || gsWikiOf[i] >= gsWikiN) continue;
            g = &gsWiki[gsWikiOf[i]];
            snprintf(scrapeListTitle[scrapeListCount], sizeof(scrapeListTitle[0]), "%s", g->title);
            snprintf(scrapeListAlias[scrapeListCount], sizeof(scrapeListAlias[0]), "%s", g->alias);
            snprintf(scrapeListFile[scrapeListCount], sizeof(scrapeListFile[0]), "%s",
                     SysList_Boxart(g, gsRegion));
            GameSel_CoverDest(g, gsRows[i].game, scrapeListDest[scrapeListCount],
                              sizeof(scrapeListDest[0]));
            scrapeListCount++;
            any = true;
        }
    } else {
        for (int i = 0; i < gsCount; i++)
            if (gsRows[i].on && gsRows[i].game >= 0 && gsRows[i].game < MAX_GAMES) {
                scrapeOnly[gsRows[i].game] = true;
                any = true;
            }
    }
    gsCoverRun = false;
    if (any) {
        char cfgDir[2112];   /* the dates file goes there */
        snprintf(cfgDir, sizeof(cfgDir), "%s/config", GetBaseDirectory());
        MakeDirectory(cfgDir);
        scrapeOnlyOn = !gsWikiMode;
        ScrapeMenu_Start(0);
        gsCoverRun = scrapeBusy;
        if (!gsCoverRun) { scrapeOnlyOn = false; scrapeListCount = 0; }
    }
    if (!gsCoverRun) GameSel_FinishCovers();
    gsWasRunning = true;
}

static void GameSel_Cancel(void) {
    Gs_Cancel(gsRows, gsCount);
    for (int i = 0; i < gsCount; i++) GameSel_Kill(gsFetch[i]);
    GameSel_Kill(gsBuild);
    if (gsCoverRun) ScrapeMenu_Cancel();
}

/* Every frame: reap steps, start the next ones, finish the cover pass. */
static void GameSel_Poll(void) {
    int code = 1;
    for (int i = 0; i < gsCount; i++) {
        if (NsoProcOk(gsFetch[i]) && NsoSpawnExited(&gsFetch[i], &code)) Gs_FetchDone(&gsRows[i], code == 0);
    }
    if (NsoProcOk(gsBuild) && NsoSpawnExited(&gsBuild, &code)) {
        int r = gsBuildRow;
        gsBuildRow = -1;
        if (r >= 0 && r < gsCount && gsRows[r].st == GS_BUILDING) {
            Gs_BuildDone(&gsRows[r], code == 0);
            if (gsRows[r].st == GS_READY) GameSel_Configure(r);
        }
    }
    if (gsCoverRun && !scrapeBusy) {
        gsCoverRun = false;
        scrapeOnlyOn = false;
        GameSel_FinishCovers();
    }
    for (int r; (r = Gs_NextFetch(gsRows, gsCount, GS_FETCH_PARALLEL)) >= 0;) {
        gsRows[r].st = GS_DOWNLOADING;
        if (!GameSel_StartFetch(r)) Gs_FetchDone(&gsRows[r], false);
    }
    if (!NsoProcOk(gsBuild)) {
        int r = Gs_NextBuild(gsRows, gsCount);
        if (r >= 0) {
            gsRows[r].st = GS_BUILDING;
            if (!GameSel_StartBuild(r)) { gsBuildRow = -1; Gs_BuildDone(&gsRows[r], false); }
        }
    }
    if (gsWasRunning && !Gs_Running(gsRows, gsCount)) {
        int ready = 0;
        for (int i = 0; i < gsCount; i++) if (gsRows[i].st == GS_READY) ready++;
        gsWasRunning = false;
        Msg_Set(&gsMsg, S_GS_DONE, ready, NULL);
    }
}

static bool GameSel_Busy(void) {
    bool procs = NsoProcOk(gsBuild);
    for (int i = 0; i < gsCount && !procs; i++) procs = NsoProcOk(gsFetch[i]);
    return Gs_Running(gsRows, gsCount) || procs || gsCoverRun;
}

/* Panel focus: Up/Down move, A checks a game or starts, B cancels a run or
 * goes back to the Settings panel. */
static void GameSel_CycleRegion(int dir)
{
    gsRegion = (SysListRegion)((gsRegion + dir + 3) % 3);
    GameSel_RebuildWikiRows();
}

static void GameSel_Input(bool up, bool down, bool ok, bool back, bool left) {
    int startCur = GameSel_StartCursor();
    int lastCur = startCur + gsCount;
    bool right = IsKeyPressed(KEY_RIGHT)
        || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT));
    if (GameSel_Busy()) {
        if (back) { GameSel_Cancel(); Sfx_Play(SFX_CLICK); return; }
        if (up && gsCursor > 0) { gsCursor--; Sfx_Play(SFX_PLUCK); }
        if (down && gsCursor < lastCur) { gsCursor++; Sfx_Play(SFX_PLUCK); }
        if (ok && !Gs_Running(gsRows, gsCount) && gsCursor > startCur)
            Gs_Toggle(gsRows, gsCount, gsCursor - GameSel_FirstGame());
        return;
    }
    if (gsWikiMode && gsCursor == 0 && (left || right || ok)) {
        Sfx_Play(SFX_CLICK);
        GameSel_CycleRegion(left ? -1 : 1);
        return;
    }
    if (back || left) { gsOpen = false; Sfx_Play(SFX_CLICK); return; }
    if (up && gsCursor > 0) { gsCursor--; Sfx_Play(SFX_PLUCK); }
    if (down && gsCursor < lastCur) { gsCursor++; Sfx_Play(SFX_PLUCK); }
    if (ok) {
        Sfx_Play(SFX_CLICK);
        if (gsCursor == startCur) GameSel_Start();
        else if (gsCursor > startCur) Gs_Toggle(gsRows, gsCount, gsCursor - GameSel_FirstGame());
    }
}

/* Gs_NeedsMet callback: an NSO-relative file exists. */
static bool GsRelExists(const char *rel, void *u) {
    char path[2400];
    (void)u;
    snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), rel);
    return FileExists(path);
}

static StrId GameSel_StateStr(int r) {
    switch (gsRows[r].st) {
    case GS_QUEUED: return S_GS_QUEUED;
    case GS_DOWNLOADING: return S_GS_DOWNLOADING;
    case GS_WAIT_BUILD: return S_GS_WAIT_BUILD;
    case GS_BUILDING: return S_GS_BUILDING;
    case GS_COVER: return S_GS_COVER;
    case GS_READY: return S_GS_READY;
    case GS_NO_INSTALLER: return S_GS_NO_INSTALLER;
    case GS_FAILED: return S_GS_FAILED;
    case GS_CANCELLED: return S_GS_CANCELLED;
    default: break;
    }
    if (gsRows[r].pipe < 0) return S_RAW;
    {
        const GamePipeline *p = &gs_pipelines[gsRows[r].pipe];
        char path[2400];
        snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), p->binary);
        return FileExists(path) && Gs_NeedsMet(p, GsRelExists, NULL) ? S_GS_INSTALLED : S_GS_INSTALLER;
    }
}

/* Same look as the Settings panel: white box = cursor, checkbox on the left.
 * Unused after Select games moved into the status card; kept behind #if 0 so
 * the layout is not lost. */
#if 0
static void GameSel_Draw(Font font, Color red) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float hx = sw * 0.449f, cx = sw * 0.46f, bx = sw * 0.485f, bw = sw * 0.387f, bh = sh * 0.066f;
    float th = fmaxf(2.0f, sh * 0.0028f), r = sh * 0.016f;
    float hsize = (float)font.baseSize * 0.85f, osize = (float)font.baseSize * 1.2f;
    float ssize = (float)font.baseSize * 0.75f;
    float step = sh * 0.076f;
    float startY = gsWikiMode ? sh * 0.248f : sh * 0.172f;
    float top = startY + sh * 0.100f;
    int visible = (int)((sh * 0.86f - top) / step) + 1;
    char line[200];

    DrawFitText(font, T(S_GS_SELECT), (Vector2){ hx, sh * 0.092f }, hsize, sw - hx, WHITE);
    if (gsWikiMode) {
        float y = sh * 0.172f;   /* same first-row slot as Start on other systems */
        bool cur = gsCursor == 0;
        StrId fid = gsRegion == SYSLIST_USA ? S_GS_FILTER_USA
                    : gsRegion == SYSLIST_JP ? S_GS_FILTER_JP : S_GS_FILTER_ALL;
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        DrawFitText(font, T(fid), (Vector2){ bx + sw * 0.0225f, y }, osize, bw - sw * 0.03f, cur ? red : WHITE);
    }
    {   /* Start row */
        float y = startY;
        bool cur = gsCursor == GameSel_StartCursor();
        int n = Gs_Checked(gsRows, gsCount);
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        snprintf(line, sizeof(line), T(S_GS_START), n > 0 ? n : (gsWikiMode ? gsCount : 0));
        DrawFitText(font, line, (Vector2){ bx + sw * 0.0225f, y }, osize, bw - sw * 0.03f, cur ? red : WHITE);
    }
    if (gsCount == 0 && !gsWikiMode) {
        DrawFitText(font, T(S_GS_NO_ROMS), (Vector2){ hx, top }, hsize, sw - hx - sw * 0.02f, WHITE);
    }
    {
        int first = GameSel_FirstGame();
        int gameCur = gsCursor - first;
        if (gameCur < gsTop) gsTop = gameCur > 0 ? gameCur : 0;
        if (gameCur >= gsTop + visible) gsTop = gameCur - visible + 1;
        if (gsTop < 0) gsTop = 0;
    }
    for (int k = 0; k < visible && gsTop + k < gsCount; k++) {
        int i = gsTop + k;
        float y = top + step * (float)k;
        bool cur = gsCursor == i + GameSel_FirstGame();
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
        if (gsRows[i].on) {
            DrawRectangleRec(cb, red);
            DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f }, th * 1.3f, WHITE);
            DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f }, th * 1.3f, WHITE);
        } else {
            DrawRectangleLinesEx(cb, th, WHITE);
        }
        StrId sid = GameSel_StateStr(i);
        float stW = 0.0f;
        if (sid != S_RAW) {
            const char *st = T(sid);
            stW = MeasureTextEx(font, st, ssize, 1.0f).x;
            DrawFitText(font, st, (Vector2){ bx + bw - stW - sw * 0.012f, y }, ssize, stW + 1.0f, cur ? red : WHITE);
        }
        DrawFitText(font, gsLabel[i], (Vector2){ bx + sw * 0.0225f, y }, osize,
                    bw - sw * 0.035f - stW, cur ? red : WHITE);
    }
    {   /* footer: cover-pass or install result only; actions stay on the info bar */
        char status[192] = "";
        if (gsCoverRun) Msg_Format(&scrapeMsg, status, sizeof(status));
        else Msg_Format(&gsMsg, status, sizeof(status));
        if (status[0])
            DrawFitText(font, status, (Vector2){ hx, sh * 0.935f }, ssize, sw - hx - sw * 0.02f, WHITE);
    }
}
#endif /* 0: sidebar GameSel_Draw */

/* Select games drawn inside the status card (same rows and cursor as the
 * sidebar page, ink/line colors of the status menu). */
static void GameSel_DrawInCard(Font font, Rectangle area, Color ink, Color meta,
                               Color line, Color inner, Color curFill, float a)
{
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float hx = area.x, bx = area.x + sw * 0.018f, bw = area.width - sw * 0.030f;
    float cx = bx + sh * 0.018f, bh = sh * 0.052f, r = sh * 0.013f;
    float th = fmaxf(2.0f, sh * 0.0024f);
    float hsize = sh * 0.026f, osize = sh * 0.028f, ssize = sh * 0.020f;
    float step = sh * 0.058f;
    float startY = area.y + sh * 0.055f + (gsWikiMode ? step : 0.0f);
    float top = startY + step;
    int visible = (int)((area.y + area.height - top - sh * 0.04f) / step);
    char linebuf[200];
    Color inkA = Fade(ink, a), metaA = Fade(meta, a), lineA = Fade(line, a);
    Color innerA = Fade(inner, a), fillA = Fade(curFill, a);

    if (visible < 1) visible = 1;
    DrawTextEx(font, T(S_GS_SELECT), (Vector2){ hx, area.y }, hsize, 1.0f, metaA);
    if (gsWikiMode) {
        float y = area.y + sh * 0.055f;
        bool cur = gsCursor == 0;
        StrId fid = gsRegion == SYSLIST_USA ? S_GS_FILTER_USA
                    : gsRegion == SYSLIST_JP ? S_GS_FILTER_JP : S_GS_FILTER_ALL;
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRounded(box, 0.3f, 6, fillA);
        else DrawRectangleRoundedLinesEx(box, 0.3f, 6, th, lineA);
        DrawFitText(font, T(fid), (Vector2){ bx + sw * 0.016f, y }, osize, bw - sw * 0.03f,
                    cur ? (Color){ 255, 255, 255, (unsigned char)(255 * a) } : inkA);
    }
    {
        float y = startY;
        bool cur = gsCursor == GameSel_StartCursor();
        int n = Gs_Checked(gsRows, gsCount);
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        Color tc = cur ? (Color){ 255, 255, 255, (unsigned char)(255 * a) } : inkA;
        if (cur) DrawRectangleRounded(box, 0.3f, 6, fillA);
        else DrawRectangleRoundedLinesEx(box, 0.3f, 6, th, lineA);
        snprintf(linebuf, sizeof(linebuf), T(gsCoverOnly ? S_EMU_START_COVERS : S_GS_START),
                 n > 0 ? n : (gsWikiMode ? gsCount : 0));
        DrawFitText(font, linebuf, (Vector2){ bx + sw * 0.016f, y }, osize, bw - sw * 0.03f, tc);
    }
    if (gsCount == 0 && !gsWikiMode) {
        if (gsCoverOnly) {
            char miss[192];
            ScrapeMenu_NoRomsText(miss, sizeof(miss));
            DrawFitText(font, miss, (Vector2){ hx, top }, hsize, area.width - sw * 0.02f, metaA);
        } else {
            DrawFitText(font, T(S_GS_NO_ROMS), (Vector2){ hx, top }, hsize, area.width - sw * 0.02f, metaA);
        }
    }
    {
        int first = GameSel_FirstGame();
        int gameCur = gsCursor - first;
        if (gameCur < gsTop) gsTop = gameCur > 0 ? gameCur : 0;
        if (gameCur >= gsTop + visible) gsTop = gameCur - visible + 1;
        if (gsTop < 0) gsTop = 0;
    }
    for (int k = 0; k < visible && gsTop + k < gsCount; k++) {
        int i = gsTop + k;
        float y = top + step * (float)k;
        bool cur = gsCursor == i + GameSel_FirstGame();
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        Color tc = cur ? (Color){ 255, 255, 255, (unsigned char)(255 * a) } : inkA;
        if (cur) DrawRectangleRounded(box, 0.3f, 6, fillA);
        else DrawRectangleRoundedLinesEx(box, 0.3f, 6, th, lineA);
        Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
        if (gsRows[i].on) {
            DrawRectangleRec(cb, Fade(ink, a));
            DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f },
                       th * 1.3f, (Color){ 255, 255, 255, (unsigned char)(255 * a) });
            DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f },
                       th * 1.3f, (Color){ 255, 255, 255, (unsigned char)(255 * a) });
        } else {
            DrawRectangleLinesEx(cb, th, cur ? (Color){ 255, 255, 255, (unsigned char)(255 * a) } : lineA);
        }
        StrId sid = GameSel_StateStr(i);
        float stW = 0.0f;
        float labelX = bx + sw * 0.034f;
        if (sid != S_RAW) {
            const char *st = T(sid);
            stW = MeasureTextEx(font, st, ssize, 1.0f).x;
            DrawFitText(font, st, (Vector2){ bx + bw - stW - sw * 0.012f, y }, ssize, stW + 1.0f,
                        cur ? tc : metaA);
        }
        DrawFitText(font, gsLabel[i], (Vector2){ labelX, y }, osize,
                    bw - sw * 0.05f - stW, tc);
    }
    {
        char status[192] = "";
        if (gsCoverRun) Msg_Format(&scrapeMsg, status, sizeof(status));
        else Msg_Format(&gsMsg, status, sizeof(status));
        if (status[0])
            DrawFitText(font, status, (Vector2){ hx, area.y + area.height - sh * 0.028f },
                        ssize, area.width, metaA);
    }
    (void)innerA;
}

#endif /* NSO_GAMESEL_UI_H */
