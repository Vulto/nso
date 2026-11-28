#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <pthread.h>
#ifndef _WIN32
#include <unistd.h>
#endif
#ifdef __linux__
#include <fcntl.h>
#include <sys/inotify.h>
#endif

#include "menu_launcher.h"

#include "launch.h"
#include "update.h"
#include "raylib.h"
#include "rlgl.h"   /* blend factors: erase inside the sidebar icon texture */
#ifdef NSO_RELEASE
#include "assetpack.h"
#endif

#define SCRAPE_IMPLEMENTATION
#include "scrape.h"
#define GAMESEL_IMPLEMENTATION
#include "gamesel.h"
#include "sneslist.h"

#define SAVESTATE_IMPLEMENTATION
#include "savestate.h"
#define LANG_IMPLEMENTATION
#include "lang.h"
#define REPOS_IMPLEMENTATION
#include "repos.h"
#define COVERS_IMPLEMENTATION
#include "covers.h"
#include <time.h>

#define NSO_IMPLEMENTATION
#include "nso.h"
#define NES_IMPLEMENTATION
#include "systems/nes.h"
#define SNES_IMPLEMENTATION
#include "systems/snes.h"
#define N64_IMPLEMENTATION
#include "systems/n64.h"
#define GAMECUBE_IMPLEMENTATION
#include "systems/gamecube.h"
#define MEGADRIVE_IMPLEMENTATION
#include "systems/megadrive.h"
#define SFX_IMPLEMENTATION
#include "sfx.h"
#define ENTERANIM_IMPLEMENTATION
#include "enteranim.h"
#define CRTPARAMS_IMPLEMENTATION
#include "crtparams.h"
#define CRTGEOM_IMPLEMENTATION
#include "crtgeom.h"
#define EMBED_IMPLEMENTATION
#include "embed.h"
#define REWIND_IMPLEMENTATION
#include "rewind.h"

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define FPS 60

/* Grid capacity, derived from the catalogs: every static entry plus the
 * wiki-live display rows one system can add (SYSLIST_MAX). Adding games to a
 * system header raises this automatically. */
#define NSO_COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define MAX_GAMES ((int)(NSO_COUNT(nes_games) + NSO_COUNT(snes_games) \
                       + NSO_COUNT(n64_games) + NSO_COUNT(gamecube_games) \
                       + NSO_COUNT(megadrive_games) + SYSLIST_MAX))

#define ORDER_IMPLEMENTATION
#include "order.h"
#include "gridlayout.h"
#define SORT_IMPLEMENTATION
#include "sort.h"
#define HIDE_IMPLEMENTATION
#include "hide.h"
#define SYSSTATUS_IMPLEMENTATION
#include "sysstatus.h"
#define FAV_IMPLEMENTATION
#include "fav.h"
#define VERSION_IMPLEMENTATION
#include "version.h"

typedef struct {
    PlatformEntry *platform;
    GameEntry     *games;
    const int     *gameCount;
} System;

static System systems[] = {
    { &nes_platform,      nes_games,      &nes_game_count },
    { &snes_platform,     snes_games,     &snes_game_count },
#if !defined(NSO_RELEASE) || defined(NSO_SHOW_ALL_SYSTEMS)
    /* The bundled Mednafen has no N64 or GameCube core, so the release build
     * hides these two systems (all their code stays; build with
     * -DNSO_SHOW_ALL_SYSTEMS to show them). */
    { &n64_platform,      n64_games,      &n64_game_count },
    { &gamecube_platform, gamecube_games, &gamecube_game_count },
#endif
    { &megadrive_platform, megadrive_games, &megadrive_game_count },
};

static const int SYSTEM_COUNT = (int)(sizeof(systems) / sizeof(systems[0]));

/* Hide unplayable / Favorites filter (hide.h, fav.h, per system): the games
 * array keeps every title; hidden ones are moved to the back and only the
 * first hideShown are shown while hideReady (a filter is on). */
static bool hideOn[sizeof(systems) / sizeof(systems[0])];
static bool hideReady[sizeof(systems) / sizeof(systems[0])];
static int hideShown[sizeof(systems) / sizeof(systems[0])];
static bool favOn[sizeof(systems) / sizeof(systems[0])];
static char *favText[sizeof(systems) / sizeof(systems[0])];
static bool showHidden[sizeof(systems) / sizeof(systems[0])];
static char *hiddenText[sizeof(systems) / sizeof(systems[0])];
static void Hide_Toggle(void);
static void Hide_Resync(bool live);

static Texture2D gameBoxarts[MAX_GAMES];
/* Cached even after the GPU texture is unloaded, so the mosaic does not jump. */
static float boxartMetaW[MAX_GAMES];
static float boxartMetaH[MAX_GAMES];
/* Covers are keyed by game/group, not by slot index, so a hide or filter
 * rebuild can move textures and size metadata with their titles. */
static char coverSlotKey[MAX_GAMES][COVER_KEY_MAX];
static int coverSlotN;

static void Cover_RememberGame(int index, const GameEntry *g)
{
    if (index < 0 || index >= MAX_GAMES) return;
    Covers_Key(g ? g->title : NULL, g ? g->boxartPath : NULL, g ? g->rom : NULL,
               coverSlotKey[index], sizeof(coverSlotKey[index]));
    if (coverSlotN <= index) coverSlotN = index + 1;
}

static void CoverSize_Note(int index, int w, int h);

static void BoxartMetaRemember(int index, int w, int h)
{
    if (index < 0 || index >= MAX_GAMES) return;
    if (w > 0 && h > 0) {
        boxartMetaW[index] = (float)w;
        boxartMetaH[index] = (float)h;
        CoverSize_Note(index, w, h);
    }
}

static const int ASSET_MAX_BOXART_W = 512;
static const int ASSET_MAX_BOXART_H = 384;
static const float SLIDE_SPEED = 100.0f;
static const float SIDEBAR_RAIL_FRAC = 0.06f; // side bar size 
static const float SIDEBAR_OPEN_FRAC = 0.385f;   /* video: ~38.6% */
static const float SIDEBAR_OVERSHOOT = 0.08f; /* past target, then settle */
static const float SIDEBAR_WIDTH_LERP = 0.44f;
/* 3 full rows + ~10% of a fourth so the user sees more games below */
static const float MOSAIC_VISIBLE_ROWS = 3.1f;
static const float MOSAIC_GAP = 1.0f; /* px between covers, and between grid and sidebar */
static const float MOSAIC_GROW_DELAY_SEC = 0.3f;
static const float INFO_BAR_FADE_SEC = 0.4f;
static const float INFO_BAR_SLIDE_PX = 13.0f;
static const float MOSAIC_SELECT_SCALE = 1.18f;
static const float MOSAIC_LIFT_SCALE = 0.92f;   /* Y Move: lifted tile (video: ~0.92 of its cell) */
/* Details fade = picom's default fade (the user runs `picom -fc`, no config):
 * linear, fade-in-step 0.028 / fade-out-step 0.03 every fade-delta 10 ms,
 * i.e. ~0.36 s in and ~0.33 s out, time-based. */
static const float DETAILS_FADE_IN_STEP = 0.028f;
static const float DETAILS_FADE_OUT_STEP = 0.03f;
static const float DETAILS_FADE_DELTA_MS = 10.0f;


static char baseDir[1024] = {0};
static int baseDirInitialized = 0;

/* Active system index (systems menu + mosaic) */
static int currentSystem = 0;

/* Systems menu: plain gray backdrop, cards in a centred W/2 x H/2 box. */
static const Color MENU_BG = { 28, 28, 28, 255 };
static const float MENU_CARD_ROUNDNESS = 0.10f;

typedef struct {
    int screenW;
    int screenH;
    int originX;   /* left inset reserved for sidebar rail */
    int contentW;  /* usable width for game slots */
    float totalH;
    Rectangle slots[MAX_GAMES];
    Rectangle items[MAX_GAMES];
    int itemRows[MAX_GAMES];
    float selectScale[MAX_GAMES]; /* selected grow target (1.18, or full size if larger) */
    int rowCount;
} GridLayout;

static float SidebarRailWidth(void) {
    return (float)GetScreenWidth() * SIDEBAR_RAIL_FRAC;
}

static float SidebarOpenWidth(void) {
    return (float)GetScreenWidth() * SIDEBAR_OPEN_FRAC;
}

static System *ActiveSystem(void) {
    if (currentSystem < 0 || currentSystem >= SYSTEM_COUNT) {
        return &systems[0];
    }
    return &systems[currentSystem];
}

/* Every title of the active system, shown or not. */
static int ActiveGameTotal(void) {
    return *ActiveSystem()->gameCount;
}

/* Titles in the grid: all of them, or the filtered prefix (hide unplayable
 * and/or Favorites). */
static int ActiveGameCount(void) {
    int s = (int)(ActiveSystem() - systems);
    return hideReady[s] ? hideShown[s] : ActiveGameTotal();
}

static GameEntry *ActiveGames(void) {
    return ActiveSystem()->games;
}

static SlotOrientation ActivePlatformOrient(void) {
    PlatformEntry *p = ActiveSystem()->platform;
    return p ? p->orient : SLOT_ORIENT_USA;
}

static SlotOrientation GameOrientAt(int gameIdx) {
    GameEntry *games = ActiveGames();
    int count = ActiveGameCount();
    if (gameIdx < 0 || gameIdx >= count || !games) {
        return ActivePlatformOrient();
    }
    return Nso_ResolveOrient(ActiveSystem()->platform, &games[gameIdx]);
}

/* Fit an aspect-correct (w/h) rect inside a cell. */
static void FitAspectInCell(float cellW, float cellH, float aspect,
                            float *outW, float *outH) {
    if (aspect <= 0.0f) aspect = 4.0f / 3.0f;
    if (cellW / cellH > aspect) {
        *outH = cellH;
        *outW = cellH * aspect;
    } else {
        *outW = cellW;
        *outH = cellW / aspect;
    }
}

/* Cover sizes read from file headers (no decode) by a background thread at
 * startup, for every system's covers, at the size the loader will give them.
 * So a grid is laid out with each cover's real shape on its first frame and
 * slots do not jump as textures arrive. Keyed by full path; the table is
 * built on the main thread before the thread starts, the thread only fills
 * sizes (state 0 -> 1/2). */
#define COVER_SIZE_CAP 8192
static struct {
    char *path[COVER_SIZE_CAP];
    int w[COVER_SIZE_CAP], h[COVER_SIZE_CAP];
    atomic_int state[COVER_SIZE_CAP];   /* 0 pending, 1 sized, 2 no header */
    int hash[COVER_SIZE_CAP * 2];       /* index + 1, 0 = empty */
    int n;
    pthread_t thread;
    bool running;
    atomic_bool cancel;
} coverSize;

static unsigned CoverSize_Hash(const char *p)
{
    unsigned h = 2166136261u;
    while (*p) { h ^= (unsigned char)*p++; h *= 16777619u; }
    return h;
}

static int CoverSize_Find(const char *path, int add)
{
    if (!path || !path[0]) return -1;
    unsigned m = COVER_SIZE_CAP * 2 - 1, k = CoverSize_Hash(path) & m;
    for (;;) {
        int e = coverSize.hash[k];
        if (e == 0) break;
        if (strcmp(coverSize.path[e - 1], path) == 0) return e - 1;
        k = (k + 1) & m;
    }
    if (!add || coverSize.n >= COVER_SIZE_CAP) return -1;
    char *dup = strdup(path);
    if (!dup) return -1;
    int i = coverSize.n++;
    coverSize.path[i] = dup;
    coverSize.hash[k] = i + 1;
    return i;
}

static void *CoverSizeThread(void *arg)
{
    (void)arg;
    static unsigned char head[65536];
    for (int i = 0; i < coverSize.n; i++) {
        if (atomic_load(&coverSize.cancel)) break;
        int w = 0, h = 0, ok = 0;
        FILE *f = fopen(coverSize.path[i], "rb");
        if (f) {
            size_t n = fread(head, 1, 32, f);
            if (n >= 2 && head[0] == 0xFF && head[1] == 0xD8) {
                n += fread(head + n, 1, sizeof(head) - n, f);
            }
            fclose(f);
            ok = Grid_ImageHeaderSize(head, n, &w, &h);
        }
        if (ok) {
            Grid_BoxartFitSize(w, h, ASSET_MAX_BOXART_W, ASSET_MAX_BOXART_H,
                               &coverSize.w[i], &coverSize.h[i]);
        }
        atomic_store(&coverSize.state[i], ok ? 1 : 2);
    }
    return NULL;
}

static void CoverSize_Start(void)
{
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        const GameEntry *games = systems[s].games;
        int count = systems[s].gameCount ? *systems[s].gameCount : 0;
        if (count > MAX_GAMES) count = MAX_GAMES;
        for (int i = 0; games && i < count; i++) {
            const char *rel = games[i].boxartPath;
            if (rel && rel[0]) CoverSize_Find(GetDataPath(rel), 1);
        }
    }
    atomic_store(&coverSize.cancel, false);
    coverSize.running = pthread_create(&coverSize.thread, NULL, CoverSizeThread, NULL) == 0;
}

static void CoverSize_Stop(void)
{
    atomic_store(&coverSize.cancel, true);
    if (coverSize.running) {
        pthread_join(coverSize.thread, NULL);
        coverSize.running = false;
    }
    for (int i = 0; i < coverSize.n; i++) { free(coverSize.path[i]); coverSize.path[i] = NULL; }
    memset(coverSize.hash, 0, sizeof coverSize.hash);
    coverSize.n = 0;
}

/* A cover was loaded (or replaced on disk): keep its header size current so
 * the next visit lays it out at this size. Main thread; entries the size
 * thread has not reached yet (state 0) are left to it. */
static void CoverSize_Note(int index, int w, int h)
{
    if (index < 0 || index >= ActiveGameCount()) return;
    const char *rel = ActiveGames()[index].boxartPath;
    int c = (rel && rel[0]) ? CoverSize_Find(GetDataPath(rel), 0) : -1;
    if (c < 0 || atomic_load(&coverSize.state[c]) == 0) return;
    coverSize.w[c] = w;
    coverSize.h[c] = h;
    atomic_store(&coverSize.state[c], 1);
}

/* Known cover aspect: loaded size, else the header size; 0 = not known yet.
 * meta -1 = no header size (texture size arrives with the cover). */
static float GridItemKnownAspect(int index) {
    if (index < 0 || index >= MAX_GAMES) return 0.0f;
    if (boxartMetaW[index] == 0.0f && index < ActiveGameCount()) {
        const char *rel = ActiveGames()[index].boxartPath;
        int c = (rel && rel[0]) ? CoverSize_Find(GetDataPath(rel), 0) : -1;
        int st = c >= 0 ? atomic_load(&coverSize.state[c]) : 2;
        if (st == 1) BoxartMetaRemember(index, coverSize.w[c], coverSize.h[c]);
        else if (st == 2) { boxartMetaW[index] = -1.0f; boxartMetaH[index] = -1.0f; }
    }
    if (boxartMetaW[index] > 0.0f && boxartMetaH[index] > 0.0f) {
        return boxartMetaW[index] / boxartMetaH[index];
    }
    if (gameBoxarts[index].id != 0
        && gameBoxarts[index].width > 0
        && gameBoxarts[index].height > 0) {
        return (float)gameBoxarts[index].width / (float)gameBoxarts[index].height;
    }
    return 0.0f;
}

static float GridItemAspect(int index) {
    float a = GridItemKnownAspect(index);
    return a > 0.0f ? a : Nso_OrientAspect(GameOrientAt(index));
}

GridLayout GetGridLayout(void) {
    GridLayout g;
    g.screenW = GetScreenWidth();
    g.screenH = GetScreenHeight();
    g.originX = (int)SidebarRailWidth();
    if (g.originX < 0) g.originX = 0;
    g.contentW = g.screenW - g.originX;
    if (g.contentW < 1) g.contentW = 1;

    int count = ActiveGameCount();
    if (count > MAX_GAMES) count = MAX_GAMES;
    if (count <= 0) {
        g.totalH = 0.0f;
        g.rowCount = 0;
        return g;
    }

    /* Per-system base slot, ±10% flex to the cover aspect, 1px MOSAIC_GAP
     * between covers and between the grid and the sidebar. Full rows fill
     * the width; a short last row stays at the standard scale. */
    const char *sysId = (ActiveSystem()->platform && ActiveSystem()->platform->id)
        ? ActiveSystem()->platform->id : "";
    float aspects[MAX_GAMES];
    GridLaidItem laid[MAX_GAMES];
    /* A cover not sized yet takes the system's base shape (Grid_Pack's
     * fallback), not a 4:3 guess; SNES keeps the per-game orientation. */
    int mixSys = Grid_Lookup(sysId)->mix;
    for (int i = 0; i < count; i++) {
        aspects[i] = mixSys ? GridItemAspect(i) : GridItemKnownAspect(i);
    }
    Grid_Pack(sysId, g.screenW, g.screenH, g.originX, g.contentW,
              aspects, count, MOSAIC_GAP, MOSAIC_VISIBLE_ROWS,
              laid, MAX_GAMES, &g.totalH, &g.rowCount);
    for (int i = 0; i < count; i++) {
        Rectangle slot = { laid[i].x, laid[i].y, laid[i].w, laid[i].h };
        g.slots[i] = slot;
        g.items[i] = slot;
        g.itemRows[i] = laid[i].row;
        float target = MOSAIC_SELECT_SCALE;
        if (laid[i].fullScale > target) target = laid[i].fullScale;
        g.selectScale[i] = target;
    }
    return g;
}

float GridMaxScroll(const GridLayout *g, int itemCount) {
    if (!g || itemCount <= 0) {
        return 0.0f;
    }

    return fmaxf(0.0f, g->totalH - (float)g->screenH);
}

float GridTargetViewportY(const GridLayout *g, int selectedIndex, int itemCount,
                          float currentViewportY) {
    if (!g || itemCount <= 0) {
        return 0.0f;
    }

    if (selectedIndex < 0 || selectedIndex >= itemCount) return 0.0f;
    Rectangle item = g->slots[selectedIndex];
    float target = currentViewportY;
    if (item.y < currentViewportY) target = item.y;
    else if (item.y + item.height > currentViewportY + (float)g->screenH)
        target = item.y + item.height - (float)g->screenH;
    return fminf(GridMaxScroll(g, itemCount), fmaxf(0.0f, target));
}

static int GridNavigate(const GridLayout *g, int idx, int count, OrderNav dir) {
    if (!g || count <= 0 || idx < 0 || idx >= count) return idx;
    int row = g->itemRows[idx];
    if (dir == ORDER_NAV_LEFT) return idx > 0 ? idx - 1 : idx;
    if (dir == ORDER_NAV_RIGHT) return idx + 1 < count ? idx + 1 : idx;
    if (g->rowCount <= 1) return idx;
    int targetRow = dir == ORDER_NAV_UP ? (row + g->rowCount - 1) % g->rowCount
                                        : (row + 1) % g->rowCount;
    float center = g->items[idx].x + g->items[idx].width * 0.5f;
    int best = idx;
    float bestDistance = INFINITY;
    for (int i = 0; i < count; i++) {
        if (g->itemRows[i] != targetRow) continue;
        float candidate = g->items[i].x + g->items[i].width * 0.5f;
        float distance = fabsf(candidate - center);
        if (distance < bestDistance) {
            bestDistance = distance;
            best = i;
        }
    }
    return best;
}

float GridClampViewportY(const GridLayout *g, float viewportY, int itemCount) {
    float maxScroll = GridMaxScroll(g, itemCount);
    if (viewportY < 0.0f) {
        return 0.0f;
    }
    if (viewportY > maxScroll) {
        return maxScroll;
    }
    return viewportY;
}

static void InitBaseDirectory(void) {
    if (baseDirInitialized) return;

    const char *env = getenv("NSO_BASE_DIR");
    if (env && env[0] != '\0') {
        strncpy(baseDir, env, sizeof(baseDir) - 1);
        baseDir[sizeof(baseDir) - 1] = '\0';
        baseDirInitialized = 1;
        return;
    }

    const char *appDir = GetApplicationDirectory();
    if (appDir && appDir[0] != '\0') {
        strncpy(baseDir, appDir, sizeof(baseDir) - 1);
        baseDir[sizeof(baseDir) - 1] = '\0';
        baseDirInitialized = 1;
        return;
    }

    const char *cwd = GetWorkingDirectory();
    if (cwd && cwd[0] != '\0') {
        strncpy(baseDir, cwd, sizeof(baseDir) - 1);
        baseDir[sizeof(baseDir) - 1] = '\0';
    } else {
        strcpy(baseDir, ".");
    }

    baseDirInitialized = 1;
}

const char *GetBaseDirectory(void) {
    InitBaseDirectory();
    return baseDir;
}

const char *GetDataPath(const char *relativePath) {
    static char fullPath[2048];

    const char *base = GetBaseDirectory();

    if (!relativePath || relativePath[0] == '\0') {
        snprintf(fullPath, sizeof(fullPath), "%s", base);
    } else {
        snprintf(fullPath, sizeof(fullPath), "%s/%s", base, relativePath);
    }

    return fullPath;
}

static void EnsureDirectoryTree(const char *path);

/* Directory containing this NSO executable (/proc/self/exe), not the CWD. */
static char nsoExeDirBuf[2048];
static int nsoExeDirReady;
static char nsoMednafenPath[2400];
static int nsoMednafenState;   /* 0 unknown, 1 present, -1 missing */

static const char *NsoExeDir(void)
{
    if (nsoExeDirReady) return nsoExeDirBuf;
    nsoExeDirBuf[0] = '\0';
#ifndef _WIN32
    {
        char link[2048];
        ssize_t n = readlink("/proc/self/exe", link, sizeof(link) - 1);
        if (n > 0) {
            char *slash;
            static const char deleted[] = " (deleted)";
            link[n] = '\0';
            if ((size_t)n > sizeof(deleted) - 1
                && strcmp(link + (size_t)n - (sizeof(deleted) - 1), deleted) == 0)
                link[(size_t)n - (sizeof(deleted) - 1)] = '\0';
            slash = strrchr(link, '/');
            if (slash) {
                if (slash == link) slash[1] = '\0';
                else *slash = '\0';
                snprintf(nsoExeDirBuf, sizeof(nsoExeDirBuf), "%s", link);
            }
        }
    }
#endif
    if (!nsoExeDirBuf[0]) {
        const char *app = GetApplicationDirectory();
        if (app && app[0]) {
            size_t L = strlen(app);
            snprintf(nsoExeDirBuf, sizeof(nsoExeDirBuf), "%s", app);
            if (L > 1 && nsoExeDirBuf[L - 1] == '/') nsoExeDirBuf[L - 1] = '\0';
        } else {
            snprintf(nsoExeDirBuf, sizeof(nsoExeDirBuf), "%s", ".");
        }
    }
    nsoExeDirReady = 1;
    return nsoExeDirBuf;
}

static void NsoMednafenRecheck(void)
{
    nsoMednafenState = 0;
}

static int NsoMednafenIsExe(const char *path)
{
    return path && path[0] && FileExists(path) && !DirectoryExists(path);
}

/* First and only emulator: a `mednafen` file next to this binary, then the
 * in-tree dev path <exe>/mednafen/src/mednafen. No PATH / system fallback.
 * The result is cached; call NsoMednafenRecheck() when about to launch. */
static const char *NsoBundledMednafen(void)
{
    const char *dir;
    char cand[2400];
    if (nsoMednafenState == 1) return nsoMednafenPath;
    if (nsoMednafenState == -1) return NULL;
    nsoMednafenPath[0] = '\0';
    dir = NsoExeDir();
    snprintf(cand, sizeof(cand), "%s/mednafen", dir);
    if (NsoMednafenIsExe(cand)) {
        snprintf(nsoMednafenPath, sizeof(nsoMednafenPath), "%s", cand);
        nsoMednafenState = 1;
        return nsoMednafenPath;
    }
    snprintf(cand, sizeof(cand), "%s/mednafen/src/mednafen", dir);
    if (NsoMednafenIsExe(cand)) {
        snprintf(nsoMednafenPath, sizeof(nsoMednafenPath), "%s", cand);
        nsoMednafenState = 1;
        return nsoMednafenPath;
    }
    nsoMednafenState = -1;
    return NULL;
}

static void NsoMednafenDir(char *dst, size_t n)
{
    const char *bin = NsoBundledMednafen();
    char *slash;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!bin) return;
    snprintf(dst, n, "%s", bin);
    slash = strrchr(dst, '/');
    if (!slash) return;
    if (slash == dst) slash[1] = '\0';
    else *slash = '\0';
}

static void NsoEnsureConfigDir(void)
{
    char dir[2112];
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    EnsureDirectoryTree(dir);
}

/* Materialize shaders/crtgeom.glsl on disk so a child Mednafen can load it
 * (the pack hook only serves this process). */
static void NsoEnsureShaderFile(void)
{
    char dest[2048], dir[2048];
    char *text;
    snprintf(dest, sizeof(dest), "%s", GetDataPath("shaders/crtgeom.glsl"));
    if (FileExists(dest) && GetFileLength(dest) > 0) return;
    text = LoadFileText("shaders/crtgeom.glsl");
    if (!text) text = LoadFileText(dest);
    if (!text) return;
    snprintf(dir, sizeof(dir), "%s", GetDataPath("shaders"));
    EnsureDirectoryTree(dir);
    SaveFileText(dest, text);
    UnloadFileText(text);
}

/* Installed = the game's binary exists (built / downloaded). Mednafen games
 * use the bundled emulator next to this executable, cached until launch. */
static bool GameBinaryPresent(const GameEntry *game) {
    char path[2304];
    if (!game) return false;
    if (game->mednafen && game->mednafen[0])
        return NsoBundledMednafen() != NULL;
    if (!game->dir || !game->dir[0] || !game->binary || !game->binary[0]) return false;
    snprintf(path, sizeof(path), "%s/%s/%s", GetBaseDirectory(), game->dir, game->binary);
    return FileExists(path);
}

static bool GameIsLaunchable(const GameEntry *game);

/* Grid tint: games that cannot launch yet are dimmed. */
static Color GameTint(int gameIdx) {
    /* Launchability costs several stat() calls; the grid asks for every
     * visible game each frame, so keep the answer for a second. */
    static const char *tTitle[MAX_GAMES], *tRom[MAX_GAMES];
    static double tAt[MAX_GAMES];
    static bool tVal[MAX_GAMES];
    const GameEntry *g = &ActiveGames()[gameIdx];
    bool launchable;
    if (gameIdx >= 0 && gameIdx < MAX_GAMES) {
        double now = GetTime();
        if (tAt[gameIdx] <= 0.0 || tTitle[gameIdx] != g->title || tRom[gameIdx] != g->rom
            || now - tAt[gameIdx] >= 1.0 || now < tAt[gameIdx]) {
            tVal[gameIdx] = GameIsLaunchable(g);
            tTitle[gameIdx] = g->title;
            tRom[gameIdx] = g->rom;
            tAt[gameIdx] = now > 0.0 ? now : 0.001;
        }
        launchable = tVal[gameIdx];
    } else {
        launchable = GameIsLaunchable(g);
    }
    return launchable ? WHITE : (Color){ 110, 110, 110, 255 };
}

/* gameIdx is local to the active system */
static Image LoadBoxartImage(const char *path);
static bool CoverLoader_Owns(int gameIdx);
static bool coverLoaderHasWork(void);
static void CoverReload_Stamp(const char *ev);
void UnloadBoxart(int gameIdx);

/* Negative cover cache: a game with no cover file used to cost a few stat()
 * calls per frame. The miss is remembered per slot (keyed like the loaded
 * slots, so a re-sort invalidates it) and only re-checked after a scrape or
 * when the covers are unloaded (system change). */
static char boxartMissKey[MAX_GAMES][COVER_KEY_MAX];

static void Boxart_MissReset(void)
{
    memset(boxartMissKey, 0, sizeof boxartMissKey);
}

Texture2D GetBoxart(int gameIdx) {
    int count = ActiveGameCount();
    if (gameIdx < 0 || gameIdx >= count || gameIdx >= MAX_GAMES) {
        return (Texture2D){0};
    }

    if (gameBoxarts[gameIdx].id != 0) {
        char now[COVER_KEY_MAX];
        const GameEntry *g = &ActiveGames()[gameIdx];
        Covers_Key(g->title, g->boxartPath, g->rom, now, sizeof(now));
        if (coverSlotKey[gameIdx][0] && strcmp(coverSlotKey[gameIdx], now) != 0) {
            UnloadBoxart(gameIdx);
        } else {
            Cover_RememberGame(gameIdx, g);
            return gameBoxarts[gameIdx];
        }
    }

    /* The background loader owns this slot; wait for CoverLoader_Poll. */
    if (CoverLoader_Owns(gameIdx)) {
        return (Texture2D){0};
    }

    const char *rel = ActiveGames()[gameIdx].boxartPath;
    Image img;
    if (!rel || !rel[0]) {
        return (Texture2D){0};
    }
    {
        char mk[COVER_KEY_MAX];
        const GameEntry *mg = &ActiveGames()[gameIdx];
        Covers_Key(mg->title, mg->boxartPath, mg->rom, mk, sizeof(mk));
        if (boxartMissKey[gameIdx][0] && strcmp(boxartMissKey[gameIdx], mk) == 0)
            return (Texture2D){0};
        img = LoadBoxartImage(GetDataPath(rel));
        if (!img.data) {
            snprintf(boxartMissKey[gameIdx], sizeof(boxartMissKey[gameIdx]), "%s", mk);
            return (Texture2D){0};
        }
    }

    gameBoxarts[gameIdx] = LoadTextureFromImage(img);
    BoxartMetaRemember(gameIdx, img.width, img.height);
    Cover_RememberGame(gameIdx, &ActiveGames()[gameIdx]);
    UnloadImage(img);

    return gameBoxarts[gameIdx];
}

/* CPU-only cover decode + downscale (no GL, safe on the loader thread). */
static Image LoadBoxartImage(const char *path) {
    if (!path || path[0] == '\0') {
        return (Image){0};
    }

    if (!FileExists(path) || DirectoryExists(path)) {   /* quiet: missing covers are retried every frame */
        return (Image){0};
    }
    /* Decode by content: raylib's LoadImage picks the codec from the file
     * extension, so a download still named *.part failed with "Data format
     * not supported" even for a valid PNG. */
    Image img = (Image){0};
    {
        int fsz = 0;
        unsigned char *fdata = LoadFileData(path, &fsz);
        if (fdata) {
            const char *ext = NULL;
            if (fsz >= 8 && fdata[0] == 0x89 && fdata[1] == 'P' && fdata[2] == 'N' && fdata[3] == 'G') ext = ".png";
            else if (fsz >= 3 && fdata[0] == 0xFF && fdata[1] == 0xD8 && fdata[2] == 0xFF) ext = ".jpg";
            if (ext) img = LoadImageFromMemory(ext, fdata, fsz);
            else img = LoadImage(path);
            UnloadFileData(fdata);
        }
    }
    if (!img.data) {
        return img;
    }

    int nw, nh;
    Grid_BoxartFitSize(img.width, img.height, ASSET_MAX_BOXART_W, ASSET_MAX_BOXART_H, &nw, &nh);
    if (nw != img.width || nh != img.height) {
        ImageResize(&img, nw, nh);
    }
    return img;
}

void UnloadBoxart(int gameIdx) {
    if (gameIdx < 0 || gameIdx >= MAX_GAMES) return;

    if (gameBoxarts[gameIdx].id != 0) {
        UnloadTexture(gameBoxarts[gameIdx]);
        gameBoxarts[gameIdx] = (Texture2D){0};
    }
}

void UnloadAllBoxarts(void) {
    Boxart_MissReset();
    for (int i = 0; i < MAX_GAMES; i++) {
        UnloadBoxart(i);
        coverSlotKey[i][0] = '\0';
        boxartMetaW[i] = 0.0f;
        boxartMetaH[i] = 0.0f;
    }
    coverSlotN = 0;
}

/* Safety net for the visible screen only. While the reload worker is running
 * it owns the queue; Poll uploads in priority order without blocking input. */
void ManageVisibleBoxarts(int gameCount, float viewportY) {
    if (gameCount <= 0) return;
    if (coverLoaderHasWork()) return;

    GridLayout g = GetGridLayout();
    CoversRect slots[MAX_GAMES];
    int n = gameCount;
    if (n > MAX_GAMES) n = MAX_GAMES;
    for (int i = 0; i < n; i++) {
        slots[i].x = g.slots[i].x;
        slots[i].y = g.slots[i].y;
        slots[i].width = g.slots[i].width;
        slots[i].height = g.slots[i].height;
    }
    int vis[MAX_GAMES];
    int vn = Covers_Screen(slots, n, viewportY, (float)g.screenH, vis, MAX_GAMES);
    int budget = 4;
    for (int k = 0; k < vn && budget > 0; k++) {
        int i = vis[k];
        if (gameBoxarts[i].id == 0 && GetBoxart(i).id != 0) budget--;
    }
}

/* ---- Background cover loader (splash, and reload after a game) ----
 * A worker thread decodes/resizes cover images (LoadImage, CPU only); the
 * main thread uploads finished ones with LoadTextureFromImage, since GL
 * textures must be created on the main thread.
 *
 * Decode and upload follow Covers_ReloadOrder: the visible screen first
 * (what the user will see on return), then the first and last screens of
 * the list, then the remaining middle. Visible covers are uploaded as soon
 * as they are ready; later blocks use a per-frame budget so input and
 * frames stay unblocked. */
static struct {
    pthread_t thread;
    bool running;              /* thread started and not yet joined */
    int count;
    int order[MAX_GAMES];
    int orderN;
    int visibleN;              /* prefix of order[] that is the visible screen */
    int prefetchN;             /* prefix that is visible + first + last */
    char *paths[MAX_GAMES];
    Image images[MAX_GAMES];
    atomic_int ready[MAX_GAMES];  /* 1 = images[i] decoded (maybe empty) */
    bool uploaded[MAX_GAMES];
    char keys[MAX_GAMES][COVER_KEY_MAX];
    atomic_bool cancel;
    atomic_uint gen;               /* bumped on cancel/stop; stale work must not apply */
    unsigned batchGen;             /* generation of the in-flight batch */
} coverLoader;

static bool coverLoaderHasWork(void)
{
    return coverLoader.running || coverLoader.count > 0;
}

static bool CoverLoader_Owns(int gameIdx)
{
    if (gameIdx < 0 || gameIdx >= coverLoader.count) return false;
    if (coverLoader.uploaded[gameIdx]) return false;
    return coverLoader.running || coverLoader.count > 0;
}

/* Declared here so the cover loader can aim at the cursor's screen; the
 * later block only comments and related mosaic state. */
static int GameIdx = 0;
static int launchingGameIdx = 0;
static bool isLaunching = false;
static float viewportY = 0.0f;
static float targetViewportY = 0.0f;
static float viewportSlideFromY = 0.0f;
static float viewportSlideT = 1.0f;   /* 0..1; 1 = already at the target */
/* Linear page slide (~15 frames at 60 fps). */
static const float GRID_PAGE_SLIDE_SEC = 15.0f / 60.0f;

static void CoverLoader_BuildOrder(int count)
{
    GridLayout g = GetGridLayout();
    CoversRect slots[MAX_GAMES];
    int n = count;
    if (n > MAX_GAMES) n = MAX_GAMES;
    for (int i = 0; i < n; i++) {
        slots[i].x = g.slots[i].x;
        slots[i].y = g.slots[i].y;
        slots[i].width = g.slots[i].width;
        slots[i].height = g.slots[i].height;
    }
    float viewH = (float)g.screenH;
    if (viewH < 1.0f) viewH = 1.0f;
    float totalH = g.totalH;
    int cursor = GameIdx;
    if (isLaunching && launchingGameIdx >= 0 && launchingGameIdx < n) {
        cursor = launchingGameIdx;
    }
    if (cursor < 0) cursor = 0;
    if (n > 0 && cursor >= n) cursor = n - 1;
    float curY = GridTargetViewportY(&g, cursor, n, viewportY);
    curY = GridClampViewportY(&g, curY, n);

    coverLoader.orderN = Covers_ReloadOrder(slots, n, viewH, totalH, curY,
                                            coverLoader.order, MAX_GAMES);
    int vis[MAX_GAMES], first[MAX_GAMES], lastv[MAX_GAMES];
    int vn = Covers_Screen(slots, n, curY, viewH, vis, MAX_GAMES);
    float lastY = totalH > viewH ? totalH - viewH : 0.0f;
    int fn = Covers_Screen(slots, n, 0.0f, viewH, first, MAX_GAMES);
    int ln = Covers_Screen(slots, n, lastY, viewH, lastv, MAX_GAMES);
    unsigned char seen[MAX_GAMES];
    memset(seen, 0, sizeof seen);
    int pref = 0;
    for (int k = 0; k < vn; k++) {
        int i = vis[k];
        if (i >= 0 && i < n && !seen[i]) { seen[i] = 1; pref++; }
    }
    for (int k = 0; k < fn; k++) {
        int i = first[k];
        if (i >= 0 && i < n && !seen[i]) { seen[i] = 1; pref++; }
    }
    for (int k = 0; k < ln; k++) {
        int i = lastv[k];
        if (i >= 0 && i < n && !seen[i]) { seen[i] = 1; pref++; }
    }
    coverLoader.visibleN = vn;
    coverLoader.prefetchN = pref;
}

static void *CoverLoaderThread(void *arg) {
    (void)arg;
    unsigned gen = coverLoader.batchGen;
    int walkN = coverLoader.orderN > 0 ? coverLoader.orderN : coverLoader.count;
    for (int k = 0; k < walkN; k++) {
        if (atomic_load(&coverLoader.cancel) || atomic_load(&coverLoader.gen) != gen) break;
        int i = (coverLoader.orderN > 0) ? coverLoader.order[k] : k;
        if (i < 0 || i >= coverLoader.count) continue;
        if (coverLoader.uploaded[i] || atomic_load(&coverLoader.ready[i])) continue;
        if (!coverLoader.paths[i]) {
            if (atomic_load(&coverLoader.gen) != gen) break;
            atomic_store(&coverLoader.ready[i], 1);
            continue;
        }
        Image img = LoadBoxartImage(coverLoader.paths[i]);
        if (atomic_load(&coverLoader.gen) != gen) {
            if (img.data) UnloadImage(img);
            break;
        }
        coverLoader.images[i] = img;
        atomic_store(&coverLoader.ready[i], 1);
    }
    return NULL;
}

/* Cancel, bump the generation so stale decodes are ignored, join, and free
 * anything not handed to the GPU. */
static void CoverLoader_Stop(void) {
    atomic_fetch_add(&coverLoader.gen, 1);
    atomic_store(&coverLoader.cancel, true);
    if (coverLoader.running) {
        pthread_join(coverLoader.thread, NULL);
        coverLoader.running = false;
    }
    for (int i = 0; i < coverLoader.count; i++) {
        if (atomic_load(&coverLoader.ready[i]) && !coverLoader.uploaded[i]) {
            UnloadImage(coverLoader.images[i]);
        }
        free(coverLoader.paths[i]);
        coverLoader.paths[i] = NULL;
    }
    coverLoader.count = 0;
    coverLoader.orderN = 0;
    coverLoader.visibleN = 0;
    coverLoader.prefetchN = 0;
}

static void CoverLoader_Start(void) {
    CoverLoader_Stop();
    int count = ActiveGameCount();
    if (count > MAX_GAMES) count = MAX_GAMES;
    if (count <= 0) return;
    CoverLoader_BuildOrder(count);
    for (int i = 0; i < count; i++) {
        coverLoader.images[i] = (Image){0};
        Cover_RememberGame(i, &ActiveGames()[i]);
        memcpy(coverLoader.keys[i], coverSlotKey[i], sizeof(coverLoader.keys[i]));
        if (gameBoxarts[i].id != 0) {
            /* Already on the GPU (kept through a game, or still loaded). */
            coverLoader.paths[i] = NULL;
            atomic_store(&coverLoader.ready[i], 1);
            coverLoader.uploaded[i] = true;
        } else {
            const char *rel = ActiveGames()[i].boxartPath;
            coverLoader.paths[i] = (rel && rel[0]) ? strdup(GetDataPath(rel)) : NULL;
            atomic_store(&coverLoader.ready[i], 0);
            coverLoader.uploaded[i] = false;
        }
    }
    coverLoader.count = count;
    coverLoader.batchGen = atomic_load(&coverLoader.gen);
    atomic_store(&coverLoader.cancel, false);
    coverLoader.running = pthread_create(&coverLoader.thread, NULL,
                                         CoverLoaderThread, NULL) == 0;
    if (!coverLoader.running) {
        coverLoader.count = 0; /* fall back: grid loads covers itself */
        coverLoader.orderN = 0;
        for (int i = 0; i < count; i++) { free(coverLoader.paths[i]); coverLoader.paths[i] = NULL; }
    }
}

/* Test harness only: NSO_COVER_RELOAD_LOG=path appends monotonic-ms stamps.
 * No stdout. Absent env = no file I/O. */
static bool coverReloadVisibleLogged = false;

static void CoverReload_Stamp(const char *ev)
{
    const char *path = getenv("NSO_COVER_RELOAD_LOG");
    if (!path || !path[0] || !ev) return;
    FILE *f = fopen(path, "a");
    if (!f) return;
    struct timespec ts;
    long long ms = 0;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        ms = (long long)ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
    fprintf(f, "%lld %s\n", ms, ev);
    fclose(f);
}

/* Upload decoded covers; returns true when every cover is on the GPU.
 * The visible screen has no per-frame cap; first/last then the middle do,
 * so the menu stays usable while the rest fills in. */
/* Set when a grid is about to be shown (splash, or return from a game);
 * cleared once the visible screen's covers are on the GPU. */
static bool mosaicRevealWait = false;
/* Frozen blurred snapshot of the grid as it was before a hide/filter re-list.
 * The new list and covers are built behind it; the grid is shown once ready. */
static bool mosaicBlurHeld = false;
static void MosaicBlur_HoldBegin(void);
static void MosaicBlur_HoldEnd(void)
{
    mosaicBlurHeld = false;
}

/* True once every cover of the screen the user will see is on the GPU (or
 * has no file). The grid is not revealed before this: the exit animation /
 * splash holds its frame instead, so no empty slot is ever shown. */
static bool CoverLoader_VisibleReady(void)
{
    if (coverLoader.count > 0) {
        if (coverLoader.visibleN <= 0) return true;
        for (int k = 0; k < coverLoader.visibleN && k < coverLoader.orderN; k++) {
            int i = coverLoader.order[k];
            if (i >= 0 && i < coverLoader.count && !coverLoader.uploaded[i]) return false;
        }
        return true;
    }
    int count = ActiveGameCount();
    if (count <= 0) return true;
    if (count > MAX_GAMES) count = MAX_GAMES;
    GridLayout g = GetGridLayout();
    CoversRect slots[MAX_GAMES];
    for (int i = 0; i < count; i++) {
        slots[i].x = g.slots[i].x;
        slots[i].y = g.slots[i].y;
        slots[i].width = g.slots[i].width;
        slots[i].height = g.slots[i].height;
    }
    int cursor = GameIdx;
    if (isLaunching && launchingGameIdx >= 0 && launchingGameIdx < count)
        cursor = launchingGameIdx;
    float curY = GridTargetViewportY(&g, cursor, count, viewportY);
    curY = GridClampViewportY(&g, curY, count);
    int vis[MAX_GAMES];
    int vn = Covers_Screen(slots, count, curY, (float)g.screenH, vis, MAX_GAMES);
    for (int k = 0; k < vn; k++) {
        int i = vis[k];
        if (i < 0 || i >= count) continue;
        if (gameBoxarts[i].id != 0) continue;
        const char *rel = ActiveGames()[i].boxartPath;
        const char *path;
        if (!rel || !rel[0]) continue;
        path = GetDataPath(rel);
        if (path && path[0] && FileExists(path) && !DirectoryExists(path)) return false;
    }
    return true;
}

static bool CoverLoader_Poll(void) {
    if (coverLoader.count <= 0) return true;
    if (atomic_load(&coverLoader.gen) != coverLoader.batchGen) return false;
    int visLeft = 0, prefLeft = 0;
    int walkN = coverLoader.orderN > 0 ? coverLoader.orderN : coverLoader.count;
    for (int k = 0; k < walkN; k++) {
        int i = (coverLoader.orderN > 0) ? coverLoader.order[k] : k;
        if (i < 0 || i >= coverLoader.count || coverLoader.uploaded[i]) continue;
        if (k < coverLoader.visibleN) visLeft++;
        else if (k < coverLoader.prefetchN) prefLeft++;
    }
    if (visLeft == 0 && !coverReloadVisibleLogged && coverLoader.visibleN > 0) {
        coverReloadVisibleLogged = true;
        CoverReload_Stamp("reload-visible");
    }
    int budget;
    if (visLeft > 0) budget = visLeft;          /* finish the on-screen block now */
    else if (prefLeft > 0) budget = 12;         /* first and last screens next */
    else budget = 6;                            /* middle, progressively */

    bool done = true;
    for (int k = 0; k < walkN; k++) {
        int i = (coverLoader.orderN > 0) ? coverLoader.order[k] : k;
        if (i < 0 || i >= coverLoader.count) continue;
        if (coverLoader.uploaded[i]) continue;
        if (!atomic_load(&coverLoader.ready[i])) { done = false; continue; }
        if (budget <= 0) { done = false; break; }
        if (atomic_load(&coverLoader.gen) != coverLoader.batchGen) return false;
        {
            int dest = i, n = ActiveGameTotal(), shown = ActiveGameCount(), j;
            char now[COVER_KEY_MAX];
            if (n > MAX_GAMES) n = MAX_GAMES;
            dest = -1;
            for (j = 0; j < n; j++) {
                const GameEntry *g = &ActiveGames()[j];
                Covers_Key(g->title, g->boxartPath, g->rom, now, sizeof(now));
                if (coverLoader.keys[i][0] && strcmp(now, coverLoader.keys[i]) == 0) {
                    dest = j;
                    break;
                }
            }
            if (dest < 0) dest = i;
            if (dest >= shown) {
                if (coverLoader.images[i].data) UnloadImage(coverLoader.images[i]);
                coverLoader.images[i] = (Image){0};
                coverLoader.uploaded[i] = true;
                budget--;
                continue;
            }
            if (coverLoader.images[i].data) {
                if (gameBoxarts[dest].id != 0) UnloadTexture(gameBoxarts[dest]);
                gameBoxarts[dest] = LoadTextureFromImage(coverLoader.images[i]);
                BoxartMetaRemember(dest, coverLoader.images[i].width, coverLoader.images[i].height);
                Cover_RememberGame(dest, &ActiveGames()[dest]);
                UnloadImage(coverLoader.images[i]);
            }
        }
        coverLoader.images[i] = (Image){0};
        coverLoader.uploaded[i] = true;
        budget--;
    }
    if (done) {
        CoverReload_Stamp("reload-done");
        CoverLoader_Stop();
    }
    return done;
}

/* Writable catalog plus display-only rows for wiki titles whose covers have
 * been scraped into assets/images/systems/<id>/. The static *_games[] arrays
 * cannot grow, and those titles are not in them. Used by SNES and GameCube. */
typedef struct {
    const char *id;
    const char *titlesRel;
    GameEntry *srcGames;
    const int *srcCount;
    GameEntry liveGames[MAX_GAMES];
    char liveTitle[MAX_GAMES][SYSLIST_NAME];
    char livePath[MAX_GAMES][256];
    int liveCount;
    int catalogCount;
    int liveSys; /* index in systems[], or -1 if not installed */
} WikiLive;

static WikiLive wikiSnes;
static WikiLive wikiGc;
static WikiLive wikiMd;

static void WikiLive_Setup(WikiLive *w, const char *id, const char *titlesRel,
                           GameEntry *src, const int *srcCount)
{
    if (!w) return;
    w->id = id;
    w->titlesRel = titlesRel;
    w->srcGames = src;
    w->srcCount = srcCount;
    w->liveSys = -1;
    w->liveCount = 0;
    w->catalogCount = 0;
}

static WikiLive *WikiLive_ById(const char *id)
{
    if (id && strcmp(id, "snes") == 0) return &wikiSnes;
    if (id && strcmp(id, "gamecube") == 0) return &wikiGc;
    if (id && strcmp(id, "megadrive") == 0) return &wikiMd;
    return NULL;
}

static int WikiLive_SysIndex(const WikiLive *w)
{
    if (!w || !w->id) return -1;
    for (int i = 0; i < SYSTEM_COUNT; i++)
        if (strcmp(systems[i].platform->id, w->id) == 0) return i;
    return -1;
}

static void WikiLive_Install(WikiLive *w)
{
    int s, n;
    if (!w || !w->srcGames || !w->srcCount) return;
    s = WikiLive_SysIndex(w);
    if (s < 0 || w->liveSys >= 0) return;
    n = *w->srcCount;
    if (n > MAX_GAMES) n = MAX_GAMES;
    memcpy(w->liveGames, w->srcGames, sizeof(GameEntry) * (size_t)n);
    w->liveCount = n;
    w->catalogCount = n;
    w->liveSys = s;
    systems[s].games = w->liveGames;
    systems[s].gameCount = &w->liveCount;
}

static int WikiLive_FindTitle(const WikiLive *w, const char *title)
{
    if (!w || !title || !title[0]) return -1;
    for (int i = 0; i < w->liveCount; i++)
        if (w->liveGames[i].title && strcmp(w->liveGames[i].title, title) == 0)
            return i;
    return -1;
}

static int WikiLive_CatalogOf(const WikiLive *w, const char *title, const char *alias)
{
    const GameEntry *games;
    int n;
    char nt[256], na[256], ng[256];
    if (!w) return -1;
    games = w->liveSys >= 0 ? w->liveGames : w->srcGames;
    n = w->catalogCount > 0 ? w->catalogCount : (w->srcCount ? *w->srcCount : 0);
    if (!games) return -1;
    Gs_Norm(title ? title : "", nt, sizeof(nt));
    if (alias && alias[0]) Gs_Norm(alias, na, sizeof(na));
    else na[0] = '\0';
    for (int i = 0; i < n; i++) {
        if (!games[i].title) continue;
        if (title && strcmp(games[i].title, title) == 0) return i;
        if (alias && alias[0] && strcmp(games[i].title, alias) == 0) return i;
        Gs_Norm(games[i].title, ng, sizeof(ng));
        if (nt[0] && strcmp(ng, nt) == 0) return i;
        if (na[0] && strcmp(ng, na) == 0) return i;
    }
    return -1;
}

static void WikiLive_CoverDest(WikiLive *w, const SysListGame *g, char *dst, size_t n)
{
    int cat;
    GameEntry *games;
    char slug[SYSLIST_NAME];
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!w) return;
    cat = WikiLive_CatalogOf(w, g ? g->title : NULL, g && g->alias[0] ? g->alias : NULL);
    games = w->liveSys >= 0 ? w->liveGames : w->srcGames;
    if (cat >= 0 && games) {
        const char *rel = games[cat].boxartPath;
        if (rel && rel[0]) { snprintf(dst, n, "%s", rel); return; }
    }
    SysList_Slug(g && g->title[0] ? g->title : "untitled", slug, sizeof(slug));
    if (!slug[0]) snprintf(slug, sizeof(slug), "untitled");
    snprintf(dst, n, "assets/images/systems/%s/%s.png", w->id ? w->id : "snes", slug);
}

static void Version_InstallAll(void);
static void Version_Refresh(int sys);

/* Display-only mosaic row: cover exists, no ROM / pipeline. */
static int Wiki_AddCover(const char *sysId, const char *title, const char *alias,
                         const char *rel)
{
    WikiLive *w = WikiLive_ById(sysId);
    char full[2304];
    int i;
    if (!w) return -1;
    if (w->liveSys < 0) WikiLive_Install(w);
    if (w->liveSys < 0 || !title || !title[0] || !rel || !rel[0]) return -1;
    i = WikiLive_FindTitle(w, title);
    if (i >= 0) return i;
    /* Same game already in the catalog under a close spelling, or under the
     * other region's name (wiki "Aero Fighters", catalog "Sonic Wings"). */
    i = WikiLive_CatalogOf(w, title, alias);
    if (i >= 0) return i;
    if (w->liveCount >= MAX_GAMES) return -1;
    if (strlen(rel) >= sizeof(w->livePath[0])) return -1;
    snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), rel);
    if (!FileExists(full) || GetFileLength(full) <= 0) return -1;
    i = w->liveCount;
    snprintf(w->liveTitle[i], sizeof(w->liveTitle[0]), "%s", title);
    snprintf(w->livePath[i], sizeof(w->livePath[0]), "%s", rel);
    memset(&w->liveGames[i], 0, sizeof(w->liveGames[i]));
    w->liveGames[i].title = w->liveTitle[i];
    w->liveGames[i].boxartPath = w->livePath[i];
    w->liveGames[i].orient = SLOT_ORIENT_INHERIT;
    w->liveCount++;
    if (w->liveSys >= 0) Version_Refresh(w->liveSys);
    return i;
}

static void WikiLive_LoadScrapedCovers(WikiLive *w)
{
    char path[2112];
    char *text;
    SysListGame *list;
    int n, i;
    if (!w || !w->titlesRel) return;
    WikiLive_Install(w);
    if (w->liveSys < 0) return;
    snprintf(path, sizeof(path), "%s", GetDataPath(w->titlesRel));
    text = FileExists(path) ? LoadFileText(path) : NULL;
    if (!text) return;
    list = calloc((size_t)SYSLIST_MAX, sizeof(*list));
    if (!list) { UnloadFileText(text); return; }
    n = SysList_Parse(text, list, SYSLIST_MAX);
    UnloadFileText(text);
    if (n < 0) n = 0;
    for (i = 0; i < n && w->liveCount < MAX_GAMES; i++) {
        char dest[256];
        WikiLive_CoverDest(w, &list[i], dest, sizeof(dest));
        Wiki_AddCover(w->id, list[i].title,
                      list[i].alias[0] ? list[i].alias : NULL, dest);
    }
    free(list);
}

static void InitSystems(void) {
    WikiLive_Setup(&wikiSnes, "snes", "assets/data/snes-titles.txt",
                   snes_games, &snes_game_count);
    WikiLive_Setup(&wikiGc, "gamecube", "assets/data/gamecube-titles.txt",
                   gamecube_games, &gamecube_game_count);
    WikiLive_Setup(&wikiMd, "megadrive", "assets/data/megadrive-titles.txt",
                   megadrive_games, &megadrive_game_count);
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        Nso_PlatformInit(systems[i].platform);
    }
    WikiLive_LoadScrapedCovers(&wikiSnes);
    WikiLive_LoadScrapedCovers(&wikiGc);
    WikiLive_LoadScrapedCovers(&wikiMd);
    Version_InstallAll();
}

static void UnloadSystems(void) {
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        Nso_PlatformUnload(systems[i].platform);
    }
}

typedef enum State {
    LAUNCHING,
    SPLASH,     /* full-screen system art fading in before the game grid */
    SYSTEMS,
    MOSAIC,
    EMBED,      /* in-process game drawn in this window (embed.h) */
} State;

static State currentState = SYSTEMS;

/* GameIdx, launchingGameIdx, isLaunching, viewportY: declared with the cover loader. */

/* Y Move (hold): selected game follows the arrows; saved on release. */
static bool gridLifted = false;
static bool gridLiftMoved = false;
static int gridLiftFrom = 0;   /* index the lifted game had on entering move mode (B restores) */
/* Move-mode look (0..1): dark gaps, smaller covers, slot markers, rail dimmed.
 * Same linear picom-default fade as the Details card. */
static float liftT = 0.0f;
static const Color MOVE_BG = { 64, 66, 66, 255 };     /* video's gap grey */
static const float MOVE_OTHERS_SCALE = 0.80f;        /* other covers in move mode */
/* Video timings (60 fps frames 000319..000390): the rail slides out/in over
 * ~8.5 frames with a symmetric ease; the lifted cover glides to its new slot
 * over ~9 frames, ease-in-out. */
static const float MOVE_RAIL_SLIDE_SEC = 0.14f;
static const float MOVE_GLIDE_SEC = 0.15f;
static float railSlideT = 0.0f;   /* 0 = rail in place, 1 = slid off to the left */

/* Keyboard Y, gamepad Y, and C-Down on the games grid: hold this long
 * (same 0.9 s convention as PADUI_HOLD_SEC / EMBED_CRT_HOLD_SEC) to open
 * the sort panel; a shorter tap starts Move. */
#define SORT_HOLD_SEC 0.9f
#define SORT_Y_HINT_CP 0x1F168  /* 🅨 in the mosaic action bar */
static bool sortYHeld = false;
static bool sortYConsumed = false;
static double sortYT0 = 0.0;
static bool padCDownHeld = false;    /* right-stick / C-Down held past the deadzone */

static float SortHold_Progress(bool held, double t0) {
    if (!held) return 0.0f;
    float p = (float)((GetTime() - t0) / (double)SORT_HOLD_SEC);
    if (p < 0.0f) return 0.0f;
    if (p > 1.0f) return 1.0f;
    return p;
}

static float SortY_Progress(void) { return SortHold_Progress(sortYHeld, sortYT0); }

/* X Details card: fades in over the frosted grid (detailsT 0..1). */
static bool detailsOpen = false;
static float detailsT = 0.0f;
/* S Status card: same fade (statusT 0..1), see statuspanel.h. */
static bool statusOpen = false;
static float statusT = 0.0f;

/* viewportY / isLaunching / launchingGameIdx: declared with the cover loader. */

static NsoProc gamePid;
static bool waitingForGame = false;
static bool externalRestartOnExit = false;
#ifndef _WIN32
static volatile sig_atomic_t externalMenuSignal = 0;
static void NsoExternalMenuSignal(int signalNumber) {
    (void)signalNumber;
    externalMenuSignal = 1;
}
static volatile sig_atomic_t externalHomeSignal = 0;
static void NsoExternalHomeSignal(int signalNumber) {
    (void)signalNumber;
    externalHomeSignal = 1;
}
#endif

static bool sidebarOpen = false;
static float sidebarWidth = 0.0f;
static float targetSidebarWidth = 0.0f;
/* True while waiting for selected tile to shrink before expanding the menu. */
static bool sidebarExpandPending = false;

typedef enum {
    SB_WIDTH_IDLE = 0,
    SB_WIDTH_TO_PEAK,  /* past full open width */
    SB_WIDTH_TO_REST,  /* settle to full open */
    SB_WIDTH_TO_RAIL,  /* after close bounce: direct to rail (no min overshoot) */
} SidebarWidthPhase;

static SidebarWidthPhase sbWidthPhase = SB_WIDTH_IDLE;
static float sbWidthRest = 0.0f;  /* full open width during bounce */
static float sbWidthPeak = 0.0f;  /* past full open */
static bool sidebarClosing = false;

typedef enum {
    SCRAPE_IDLE = 0,
    SCRAPE_INDEX,
    SCRAPE_FILE,
    SCRAPE_DATES,   /* release year + month files, then done */
    SCRAPE_SCAN,    /* walk the title list without a curl in flight */
    SCRAPE_CHOOSE,  /* 2+ box-art candidates: wait for the user to pick one */
} ScrapePhase;

static ScrapePhase scrapePhase = SCRAPE_IDLE;
static bool scrapeBusy = false;
static bool scrapeCancel = false;
static bool scrapeReplace = false;
static int scrapeGame = 0;
static int scrapeMisses = 0;
static int scrapeActive = -1;
static NsoProc scrapeProc;
static char **scrapeNames = NULL;
static int scrapeNameCount = 0;
static char scrapeId[32];
static char scrapePart[2200];
static char scrapePart2[2200];   /* second download of the dates phase */
static bool scrapeIndexNext;     /* covers follow the dates phase */
static char scrapeDest[2200];
static int scrapeAliasHops = 0;   /* Named_Boxarts text alias follow, per file */
static NsoMsg scrapeMsg = { -1, 0, "" };
static bool scrapeOnly[MAX_GAMES];   /* with scrapeOnlyOn: covers of these games only (Select games) */
static bool scrapeOnlyOn = false;
static bool scrapeRomOnly = false;     /* next Start() is scoped to ROMs in roms/<System>/ */
static bool scrapeAutoPick = false;    /* Download all: pick the best No-Intro name, no picker */
static void ScrapeMenu_ChoicePrevPoll(void);
static void ScrapeMenu_ChoicePrevStop(void);
static Texture2D ScrapeMenu_ChoicePreview(void);
enum { SCRAPE_CHOICE_MAX = 16 };
static int scrapeChoiceN = 0;
static int scrapeChoiceCur = 0;
static const char *scrapeChoices[SCRAPE_CHOICE_MAX];
static char scrapeChoiceTitle[160];
/* Select games (SNES region list): scrape these titles instead of the catalog. */
static char scrapeListTitle[SNESLIST_MAX][SNESLIST_NAME];
static char scrapeListAlias[SNESLIST_MAX][SNESLIST_NAME];
static char scrapeListDest[SNESLIST_MAX][256];
static char scrapeListFile[SNESLIST_MAX][SNESLIST_NAME]; /* exact Named_Boxarts filename */
static int scrapeListCount = 0;

/* Settings-icon spin / highlight morph from sidebar width progress (rail→open maps to 0→1). */
static float gearSpinAngle = 0.0f;
static const float SIDEBAR_HIGHLIGHT_ROUNDNESS = 0.26f;

/* Blur backdrop RTs + optional shader (recreated on resize). */
static RenderTexture2D mosaicRT = {0};
static RenderTexture2D blurRT_A = {0};
static RenderTexture2D blurRT_B = {0};
static Shader blurShader = {0};
static int blurDirLoc = -1;
static int blurRT_W = 0;
static int blurRT_H = 0;
static bool blurShaderOk = false;

static bool SidebarWantsBlur(void) {
    return sidebarOpen || sidebarClosing || sbWidthPhase != SB_WIDTH_IDLE
        || sidebarWidth > SidebarRailWidth() + 2.0f;
}

static float animScales[MAX_GAMES];


/* Mosaic enter intro: delay → selected grow → info bar available for fade-in */
typedef enum {
    MOSAIC_INTRO_NONE = 0,
    MOSAIC_INTRO_DELAY,
    MOSAIC_INTRO_GROW,
} MosaicIntroPhase;

static MosaicIntroPhase mosaicIntroPhase = MOSAIC_INTRO_NONE;
static float mosaicIntroTimer = 0.0f;
/* Leave-game: fade a full-window black rect over the still-visible game (0.3 s),
 * close or park only once it is fully black, then shrink that rect into the cover. */
#define GAME_FADE_SEC 0.20f    /* black fade in on exit, fade out on enter */
#define GAME_COVER_SEC 0.15f   /* shrink into the cover on exit, grow from it on enter */
typedef enum {
    GAME_EXIT_NONE = 0,
    GAME_EXIT_FADE,    /* black opacity 0→1 over the game / pause menu */
    GAME_EXIT_CLOSE,   /* fully black; unmap/park; wait until gone + covers ready */
    GAME_EXIT_REVEAL,  /* black rectangle shrinks into the cover, grid visible */
} GameExitPhase;
static GameExitPhase gameExitPhase = GAME_EXIT_NONE;
static float gameExitT = 0.0f;
static bool gameExitCloseStarted = false;
static bool gameExitHandoff = false;   /* this frame drew mosaic under full black; drop the X overlay after the swap */
static bool GameExitActive(void) { return gameExitPhase != GAME_EXIT_NONE; }
static void GameExit_Reset(void);
static void GameExit_Begin(void);
static void GameExit_Step(float dt);

/* Open a game: the exit animation run in reverse. */
typedef enum {
    GAME_ENTER_NONE = 0,
    GAME_ENTER_GROW,    /* black rectangle grows from the cover to the full window */
    GAME_ENTER_START,   /* fully black; launch/resume behind the veil; wait for the first frame */
    GAME_ENTER_FADE,    /* black opacity 1→0 once the game is showing */
} GameEnterPhase;
static GameEnterPhase gameEnterPhase = GAME_ENTER_NONE;
static float gameEnterT = 0.0f;
static bool gameEnterStarted = false;
static bool gameEnterHadFrame = false;
static bool gameEnterHandoff = false;   /* first game frame presented under full black; fade after the swap */
static bool gameEnterFailed = false;
static bool GameEnterActive(void) { return gameEnterPhase != GAME_ENTER_NONE; }

static NsoMsg launchMsg = { -1, 0, "" };
static double launchMsgUntil;

static void LaunchMsg_Show(int id)
{
    Msg_Set(&launchMsg, id, 0, NULL);
    launchMsgUntil = GetTime() + 4.0;
}

static void LaunchMsg_Draw(Font font)
{
    char text[192];
    float sw, sh, fs, tw, pad, w, h, x, y;
    if (launchMsg.id < 0 || GetTime() >= launchMsgUntil) return;
    Msg_Format(&launchMsg, text, sizeof(text));
    if (!text[0] || font.texture.id == 0) return;
    sw = (float)GetScreenWidth();
    sh = (float)GetScreenHeight();
    fs = sh * 0.028f;
    tw = MeasureTextEx(font, text, fs, 1.0f).x;
    pad = sh * 0.018f;
    w = tw + pad * 2.0f;
    if (w > sw * 0.9f) w = sw * 0.9f;
    h = fs + pad;
    x = (sw - w) * 0.5f;
    y = sh * 0.86f;
    DrawRectangleRounded((Rectangle){ x, y, w, h }, 0.25f, 8,
                         (Color){ 20, 20, 20, 230 });
    DrawTextEx(font, text, (Vector2){ x + pad, y + (h - fs) * 0.5f },
               fs, 1.0f, WHITE);
}
static void GameEnter_Reset(void);
static void GameEnter_Begin(void);
static void GameEnter_Step(float dt);
static void BeginMosaicEnterIntro(void);
static bool GameMenuDirectory(char *out, size_t cap, const GameEntry *game);
/* NSO menu > Quit of an external game: its process is still shutting down while the card-return
 * animation plays and after it. No black cover and no second enter intro for that wait. */
static bool externalQuitting = false;

/* One game kept alive behind the grid (End → To Game Selection). Re-opening the
 * same title resumes it; opening another asks to close this one first. */
static struct {
    bool parked;
    bool embedded;           /* in-process module (embed.h) */
    bool external;           /* child with the menu-action protocol */
    int sys, game;
    char title[96];
    NsoProc pid;
    char menuDir[1200];
    unsigned long window;
    bool slots;
} live;
static bool liveParking = false;   /* fade/close will park after the screen is fully black */
static int liveConfirm = 0;        /* 0 off, 1 open */
static int liveConfirmSel = 0;     /* 0 Yes, 1 No */
static int liveConfirmGame = -1;   /* grid index to launch after Yes */
static float liveConfirmT = 0.0f;

static bool Live_Busy(void) { return waitingForGame && !live.parked; }

typedef enum {
    HOME_RET_NONE = 0,
    HOME_RET_SYSTEMS,
    HOME_RET_SIDEBAR,
    HOME_RET_DETAILS,
    HOME_RET_STATUS,
    HOME_RET_SORT,
    HOME_RET_LIFT,
    HOME_RET_GAME,
    HOME_RET_MOSAIC,
} HomeReturn;
static HomeReturn homeReturn = HOME_RET_NONE;
static int homeRetSidebarItem = 0, homeRetSetRow = 0;
static bool homeRetPanelFocus = false, homeRetPad = false, homeRetGameSel = false;
static int homeRetSys = 0, homeRetGame = 0;
static float homeRetViewY = 0.0f;
static bool homeResumePending = false;
static int homeEnterGame = -1;
static float homeEnterViewY = 0.0f;
/* Home only: fade the system menu over a snapshot of the previous screen
 * (the game, or mosaic/manage). Other transitions are unchanged. */
#define HOME_XFADE_SEC 0.15f
typedef enum { HOME_XFADE_NONE = 0, HOME_XFADE_IN, HOME_XFADE_OUT } HomeXFadePhase;
static HomeXFadePhase homeXFade = HOME_XFADE_NONE;
static float homeXFadeT = 0.0f;
static Texture2D homeUnderlay;
static RenderTexture2D homeMenuRt;
static bool homeUnmapAfterFrame = false;
static bool homeResumeAfterFrame = false;
static bool homeApplyAfterFrame = false;

static void Live_Clear(void)
{
    if (homeReturn == HOME_RET_GAME) homeReturn = HOME_RET_NONE;
    live.parked = false;
    live.embedded = false;
    live.external = false;
    live.sys = live.game = -1;
    live.title[0] = '\0';
    live.pid = NsoProcNone();
    live.menuDir[0] = '\0';
    live.window = 0;
    live.slots = false;
    liveParking = false;
}

/* Capture identity while the overlay/module is still up; finish after the card anim. */
static void Live_BeginPark(void)
{
    const GameEntry *g = (launchingGameIdx >= 0 && launchingGameIdx < ActiveGameCount())
        ? &ActiveGames()[launchingGameIdx] : NULL;
    live.sys = currentSystem;
    live.game = launchingGameIdx;
    snprintf(live.title, sizeof(live.title), "%s", g && g->title ? g->title : "?");
    live.embedded = Embed_IsActive() && !Embed_IsExternal();
    live.external = Embed_IsExternal();
    if (live.external) {
        live.window = Embed_ExternalWindow();
        snprintf(live.menuDir, sizeof(live.menuDir), "%s", Embed_StateDir());
        live.pid = gamePid;
        live.slots = g && g->externalSlots;
    } else if (!live.embedded && NsoProcOk(gamePid) && g && !g->embedRomKey) {
        /* Running child with no End menu overlay: still park it as an external. */
        live.external = true;
        live.pid = gamePid;
        live.slots = g->externalSlots;
        live.window = 0;
        if (GameMenuDirectory(live.menuDir, sizeof(live.menuDir), g)) {
            char windowPath[1240];
            snprintf(windowPath, sizeof(windowPath), "%s/nso-menu-window", live.menuDir);
            FILE *fp = fopen(windowPath, "rb");
            if (fp) {
                if (fscanf(fp, "%lu", &live.window) != 1) live.window = 0;
                fclose(fp);
            }
        } else {
            live.menuDir[0] = '\0';
        }
    } else {
        live.window = 0;
        live.menuDir[0] = '\0';
        live.pid = NsoProcNone();
        live.slots = false;
    }
    liveParking = true;
}

static void Live_FinishPark(void)
{
    if (live.embedded) {
        Embed_Park();
    } else if (live.external) {
        /* Child stays paused (SIGSTOP / its own bridge). Drop the overlay only. */
        Embed_Stop();
    } else {
        Embed_Park();
    }
    live.parked = true;
    liveParking = false;
    externalQuitting = false;
    if (live.embedded) waitingForGame = false;
    /* external: waitingForGame stays true so process-exit is still watched */
}

/* Close the parked (or still-running overlay) game for good. */
static void Live_Close(void)
{
    liveConfirm = 0;
    liveConfirmT = 0.0f;
    if (live.parked && live.external && live.menuDir[0]) {
        Embed_WriteMenuAction(live.menuDir, "quit", 0);
        if (NsoProcOk(live.pid)) {
            for (int i = 0; i < 40; i++) {
                if (NsoSpawnExited(&live.pid, NULL)) break;
                struct timespec ts = { 0, 50 * 1000 * 1000 };
                nanosleep(&ts, NULL);
            }
            if (NsoProcOk(live.pid)) {
                NsoSpawnKill(live.pid);
                for (int i = 0; i < 20; i++) {
                    if (NsoSpawnExited(&live.pid, NULL)) break;
                    struct timespec ts = { 0, 50 * 1000 * 1000 };
                    nanosleep(&ts, NULL);
                }
                if (NsoProcOk(live.pid)) {
#if !defined(_WIN32)
                    kill(live.pid, SIGKILL);
#endif
                    NsoSpawnExited(&live.pid, NULL);
                }
            }
        }
        if (live.pid == gamePid) { gamePid = NsoProcNone(); waitingForGame = false; }
    }
    Embed_Unload();
    Live_Clear();
}

/* Tear down a parked game without mapping, resuming, or showing a frame.
 * "quit" is not sent: the menu launcher would SIGCONT the child to end it. */
static void Live_CloseHidden(void)
{
    liveConfirm = 0;
    liveConfirmT = 0.0f;
#if !defined(_WIN32)
    if (live.window) Embed_UnmapWindow(live.window);
    if (live.parked && live.external && NsoProcOk(live.pid)) {
        NsoProc pid = live.pid;
        char path[64];
        snprintf(path, sizeof(path), "/proc/%ld/task/%ld/children", (long)pid, (long)pid);
        FILE *f = fopen(path, "r");
        if (f) {
            long child;
            while (fscanf(f, "%ld", &child) == 1) {
                if (child > 1) {
                    kill(-(pid_t)child, SIGKILL);
                    kill((pid_t)child, SIGKILL);
                }
            }
            fclose(f);
        }
        kill(pid, SIGKILL);
        for (int i = 0; i < 20; i++) {
            if (NsoSpawnExited(&pid, NULL)) break;
            struct timespec ts = { 0, 10 * 1000 * 1000 };
            nanosleep(&ts, NULL);
        }
        if (live.pid == gamePid || pid == gamePid) {
            gamePid = NsoProcNone();
            waitingForGame = false;
        }
    }
#endif
    Embed_Unload();
    Live_Clear();
}

static bool Live_IsSame(int sys, int game)
{
    return live.parked && live.sys == sys && live.game == game;
}

static bool liveShowAfterFrame;
static void Live_Resume(void)
{
    if (!live.parked) return;
    if (live.embedded) {
        Embed_Unpark();
        live.parked = false;
        currentState = EMBED;
        return;
    }
    if (live.external && live.menuDir[0]) {
        Embed_WriteMenuAction(live.menuDir, "resume", 0);
        gamePid = live.pid;
        waitingForGame = true;
        live.parked = false;
        currentState = MOSAIC;
        /* Raise the hidden window after one clean host frame (as on Continue). */
        liveShowAfterFrame = true;
        return;
    }
    Live_Clear();
}

static void Live_ConfirmInput(bool gpUp, bool gpDown, bool gpConfirm, bool gpBack)
{
    if (liveConfirm == 0) return;
    if (IsKeyPressed(KEY_UP) || gpUp || IsKeyPressed(KEY_DOWN) || gpDown) {
        liveConfirmSel = 1 - liveConfirmSel;
        Sfx_Play(SFX_NAVIGATE);
    }
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_BACKSPACE) || gpBack) {
        liveConfirm = 0;
        Sfx_Play(SFX_CLICK);
        return;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm) {
        Sfx_Play(SFX_CLICK);
        if (liveConfirmSel == 0) {
            /* Yes: play the same opening animation; the parked game is closed
             * behind full black in GameEnter_StartGame, then the new one launches. */
            int next = liveConfirmGame;
            liveConfirm = 0;
            liveConfirmT = 0.0f;
            if (next >= 0 && next < ActiveGameCount() && GameIsLaunchable(&ActiveGames()[next])) {
                isLaunching = true;
                launchingGameIdx = next;
                GameIdx = next;
                currentState = LAUNCHING;
                GameEnter_Begin();
            }
        } else {
            liveConfirm = 0;
        }
    }
}

/* The same fade rates as Details (DETAILS_FADE_*), once per frame. */
static void Live_ConfirmStep(void)
{
    float st = GetFrameTime() * 1000.0f / DETAILS_FADE_DELTA_MS;
    if (liveConfirm) liveConfirmT = fminf(1.0f, liveConfirmT + st * DETAILS_FADE_IN_STEP);
    else liveConfirmT = fmaxf(0.0f, liveConfirmT - st * DETAILS_FADE_OUT_STEP);
}

static void Live_ConfirmDraw(Font font)
{
    float t = liveConfirmT;
    if (t <= 0.0f) return;
    float e = t * t * (3.0f - 2.0f * t);
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    /* over the blurred grid: the pause menu's dark veil */
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.45f * e));
    const Color panel = { 240, 245, 244, 255 }, selInk = { 246, 80, 59, 255 };
    float k = fminf((float)sw / 1920.0f, (float)sh / 1080.0f);
    float fs = fmaxf(22.0f, roundf(36.0f * k));
    float rowH = fmaxf(40.0f, roundf(56.0f * k));
    float boxW = fminf((float)sw * 0.55f, 720.0f * k);
    float boxH = rowH * 3.4f;
    float bx = ((float)sw - boxW) * 0.5f;
    float by = ((float)sh - boxH) * 0.5f;
    char title[192];
    snprintf(title, sizeof(title), T(S_LIVE_CLOSE), live.title[0] ? live.title : "?");
    float tw = MeasureTextEx(font, title, fs, 0.0f).x;
    DrawTextEx(font, title, (Vector2){ bx + (boxW - tw) * 0.5f, by }, fs, 0.0f, Fade(WHITE, e));
    const char *labels[2] = { T(S_LIVE_YES), T(S_LIVE_NO) };
    for (int i = 0; i < 2; i++) {
        float ry = by + rowH * (1.2f + (float)i);
        bool sel = (i == liveConfirmSel);
        if (sel) DrawRectangleRec((Rectangle){ bx, ry, boxW, rowH }, Fade(panel, e));
        Color ink = Fade(sel ? selInk : WHITE, e);
        float lw = MeasureTextEx(font, labels[i], fs, 0.0f).x;
        DrawTextEx(font, labels[i], (Vector2){ bx + (boxW - lw) * 0.5f, ry + (rowH - fs) * 0.5f },
                   fs, 0.0f, ink);
    }
}

/* Leave mosaic → systems: reverse of enter (info out → shrink → slide; no delay). */
typedef enum {
    MOSAIC_EXIT_NONE = 0,
    MOSAIC_EXIT_INFO,
    MOSAIC_EXIT_SHRINK,
} MosaicExitPhase;

static MosaicExitPhase mosaicExitPhase = MOSAIC_EXIT_NONE;

/* ---- Info bars: one at a time (close fully, then open next) ---- */
typedef enum {
    INFO_BAR_NONE = 0,
    INFO_BAR_MOSAIC,
    INFO_BAR_SIDEBAR,
    INFO_BAR_STATUS,
    /* future: INFO_BAR_ORG, INFO_BAR_DETAILS, ... */
    INFO_BAR_COUNT
} InfoBarId;

static float infoBarOpacity[INFO_BAR_COUNT];
static InfoBarId infoBarActive = INFO_BAR_NONE;
static InfoBarId infoBarWanted = INFO_BAR_NONE;

/* C-up / C-down / C-left this frame (right stick edges on any pad), set in the main loop. */
static bool padCUp, padCDown, padCLeft, padCRight;

static void StepInfoOpacity(float *cur, float target, float dt) {
    if (INFO_BAR_FADE_SEC <= 0.0f) {
        *cur = target;
        return;
    }
    float step = dt / INFO_BAR_FADE_SEC;
    if (*cur < target) {
        *cur += step;
        if (*cur > target) *cur = target;
    } else if (*cur > target) {
        *cur -= step;
        if (*cur < target) *cur = target;
    }
}

static void InfoBar_Request(InfoBarId id) {
    if (id < INFO_BAR_NONE || id >= INFO_BAR_COUNT) {
        id = INFO_BAR_NONE;
    }
    infoBarWanted = id;
}

static void InfoBar_Reset(void) {
    infoBarActive = INFO_BAR_NONE;
    infoBarWanted = INFO_BAR_NONE;
    for (int i = 0; i < INFO_BAR_COUNT; i++) {
        infoBarOpacity[i] = 0.0f;
    }
}

static bool InfoBar_IsHidden(void) {
    if (infoBarWanted != INFO_BAR_NONE || infoBarActive != INFO_BAR_NONE) {
        return false;
    }
    for (int i = 1; i < INFO_BAR_COUNT; i++) {
        if (infoBarOpacity[i] > 0.001f) {
            return false;
        }
    }
    return true;
}

static void InfoBar_Update(float dt) {
    /* Advance queue: only open after previous is fully closed. */
    if (infoBarActive != INFO_BAR_NONE && infoBarWanted != infoBarActive) {
        if (infoBarOpacity[infoBarActive] <= 0.001f) {
            infoBarOpacity[infoBarActive] = 0.0f;
            infoBarActive = infoBarWanted;
        }
    } else if (infoBarActive == INFO_BAR_NONE && infoBarWanted != INFO_BAR_NONE) {
        infoBarActive = infoBarWanted;
    }

    for (int i = 1; i < INFO_BAR_COUNT; i++) {
        float target = 0.0f;
        if ((InfoBarId)i == infoBarActive && infoBarWanted == infoBarActive) {
            target = 1.0f;
        }
        StepInfoOpacity(&infoBarOpacity[i], target, dt);
    }
}

static const char *PadUi_BarText(void);   /* padui.h: the controller screen's own hint bar */
static float PadUi_RingProgress(int which);
static bool PadUi_SwallowBack(void);
static const char *InfoBar_Text(InfoBarId id) {
    switch (id) {
        case INFO_BAR_MOSAIC:  return T(gridLifted ? S_MOVE_INFO : S_MOSAIC_INFO);
        case INFO_BAR_SIDEBAR:
            if (PadUi_BarText()) return PadUi_BarText();
            return scrapeBusy ? T(S_INFO_CANCEL) : T(S_SIDEBAR_INFO);
        case INFO_BAR_STATUS:  return T(S_STATUS_LEGEND);
        default: return NULL;
    }
}

/* Constant scale velocity (units/sec); grow 1→1.18 ≈ 0.10s. No exponential lerp. */
static const float MOSAIC_SCALE_VEL = 1.8f;
/* Outline sits outside the image (draw at -1); keep cursor flush with content edge. */
static const float MOSAIC_CURSOR_PAD = 1.0f;

static bool UpdateAnimScaleToward(float *scale, float target, float dt) {
    float diff = target - *scale;
    float step = MOSAIC_SCALE_VEL * dt;
    if (fabsf(diff) <= step) {
        *scale = target;
        return true;
    }
    *scale += (diff > 0.0f) ? step : -step;
    return false;
}

/*
 * growSelected: true → selected → MOSAIC_SELECT_SCALE; false → all → 1.0.
 * Returns true when the selected tile has reached its target.
 */
static bool UpdateMosaicItemScales(int gameCount, float dt, bool growSelected) {
    bool selectedDone = (gameCount <= 0);
    GridLayout laid = GetGridLayout();
    for (int i = 0; i < gameCount; i++) {
        float target = 1.0f;
        if (i == GameIdx && growSelected) {
            if (gridLifted) {
                target = MOSAIC_LIFT_SCALE;
            } else if (i < MAX_GAMES && laid.selectScale[i] > 0.0f) {
                target = laid.selectScale[i];
            } else {
                target = MOSAIC_SELECT_SCALE;
            }
        }
        bool done = UpdateAnimScaleToward(&animScales[i], target, dt);
        if (i == GameIdx) {
            selectedDone = done;
        }
    }
    return selectedDone;
}

/* ---- transitions ---- */
static float introOpacity = 1.0f;
static int introFrames = 0;

/* System splash: reuses the app-open fade (introFrames/introOpacity, one
 * second), then holds the full-bright art before sliding to the grid. */
/* The splash is the loading screen: covers decode on a worker thread and
 * upload on the main thread (CoverLoader_*). It slides away once
 * loading is done AND the fade finished, after a short minimum hold. */
static float splashTimer = 0.0f;   /* time since the image was 100% shown; it slides after ENTER_MIN_SHOW_SEC */
/* Enter-a-system transition (enteranim.h): seconds since A on the system tile, the
 * wordmark glyph (Nerd Font U+E22A, the Nintendo logo) and the "PC Ports" text. */
static float enterT = 0.0f;
static Font enterLogoFont, enterTextFont;
static bool enterLogoOk = false, enterTextOk = false;
#define ENTER_LOGO_CP 0xE22A
/* Per-system "select jingle": plays once the system's full-screen image is completely shown
 * (the gray transition finished, no overlay), after the click of the A press. Nintendo 64
 * uses the coin, SNES and NES have their own; -1 = none. */
static int SystemSelectJingle(const PlatformEntry *p)
{
    if (p == &n64_platform)  return (int)SFX_COIN;
    if (p == &snes_platform) return (int)SFX_SNES;
    if (p == &nes_platform)  return (int)SFX_NES;
    return -1;
}
static bool enterJinglePending = false;
static int splashJingle = -1;   /* the select jingle started for this entry, -1 if none */


static bool screenSliding = false;
static float slideOffset = 0.0f;
static int slideDirection = 0; /* -1 enter mosaic, +1 back to systems */
/* Leaving a system's menu (slideDirection > 0): seconds into the gray fade-through (enteranim.h)
 * and whether the main menu has already replaced the games grid behind the layer. */
static float leaveT = 0.0f;
static bool leaveSwitched = false;
/* Fade-in clock of the leave-to-main-menu veil; -1 while fading out or holding
 * black until the system-card textures are loaded. */
static float leaveFadeInT = -1.0f;
/* Fade-to-black of that leave, in the spirit of SIDEBAR_WIDTH_LERP 0.44f
 * (snappy settle). Fade-in keeps ENTER_FADE_SEC. */
static const float LEAVE_FADE_OUT_SEC = 0.12f;


/* Shared select-indicator metrics: a black gap, then a pastel gradient ring
 * (same MenuBarRingGrad / GetTime()*3 cycle as the bar cursor). */
static void MenuSelectRingParams(float size, float *gapT, float *ringT)
{
    *gapT = size * 0.015f;
    *ringT = size * 0.02f;
}

/* Bar-icon cursor (Switch home): a filled disc slightly lighter than the
 * pill (#4a4a4a) and a thin ring with a soft pastel gradient around the
 * circumference. The gradient rotates at the same GetTime()*3 rate as
 * MenuGlow. Diameter is computed from the bar height so the circle sits
 * fully inside the pill with a small equal top/bottom margin. Ring width
 * is 2.5% of the diameter plus 3px inward (outer radius stays put; the
 * fill shrinks). */
static const float MENU_BAR_CURSOR_MARGIN = 4.5f;   /* px at 1920, times kHud */
static const float MENU_BAR_CURSOR_RING = 0.025f;   /* fraction of diameter */
static const float MENU_BAR_CURSOR_RING_ADD = 3.0f; /* extra thickness toward center, px at 1920 */
static const Color MENU_BAR_CURSOR_FILL = { 74, 74, 74, 255 };     /* #4a4a4a */
static const Color MENU_BAR_LABEL_COL = { 94, 200, 240, 255 };     /* #5ec8f0 */

/* Pastel stops (Y-down UVs): BR mint, BL violet, TL pink/lavender, TR periwinkle. */
static const Color MENU_BAR_RING_COL[4] = {
    { 184, 232, 224, 255 },
    { 204, 180, 228, 255 },
    { 236, 204, 224, 255 },
    { 184, 204, 242, 255 },
};

static Image menuBarCursorImg;
static Texture2D menuBarCursorTex;
static Color *menuBarCursorPix;
static int menuBarCursorSize;

static void MenuBarCursorTex_Unload(void)
{
    if (menuBarCursorTex.id) UnloadTexture(menuBarCursorTex);
    menuBarCursorTex = (Texture2D){ 0 };
    /* Image.data is the same buffer; UnloadImage would free it. */
    if (menuBarCursorPix) {
        RL_FREE(menuBarCursorPix);
        menuBarCursorPix = NULL;
    }
    menuBarCursorImg = (Image){ 0 };
    menuBarCursorSize = 0;
}

static bool MenuBarCursorTex_Ensure(int size)
{
    if (size < 2) size = 2;
    if (menuBarCursorTex.id && menuBarCursorSize == size && menuBarCursorPix) return true;
    MenuBarCursorTex_Unload();
    menuBarCursorPix = (Color *)RL_CALLOC((size_t)size * (size_t)size, sizeof(Color));
    if (!menuBarCursorPix) return false;
    menuBarCursorImg = (Image){
        .data = menuBarCursorPix,
        .width = size,
        .height = size,
        .mipmaps = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
    };
    menuBarCursorTex = LoadTextureFromImage(menuBarCursorImg);
    if (!menuBarCursorTex.id) {
        MenuBarCursorTex_Unload();
        return false;
    }
    SetTextureFilter(menuBarCursorTex, TEXTURE_FILTER_BILINEAR);
    menuBarCursorSize = size;
    return true;
}

static float MenuBarSmoothstep(float e0, float e1, float x)
{
    float t;
    if (e1 <= e0) return (x >= e1) ? 1.0f : 0.0f;
    t = (x - e0) / (e1 - e0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static Color MenuBarRingGrad(float ang)
{
    /* Y-down: BR mint 0.125, BL violet 0.375, TL pink 0.625, TR periwinkle 0.875. */
    float t = ang * (0.5f / 3.14159265f);
    t -= floorf(t);
    const Color *c = MENU_BAR_RING_COL;
    int i0;
    float u;
    if (t >= 0.125f && t < 0.375f) { i0 = 0; u = (t - 0.125f) / 0.25f; }
    else if (t >= 0.375f && t < 0.625f) { i0 = 1; u = (t - 0.375f) / 0.25f; }
    else if (t >= 0.625f && t < 0.875f) { i0 = 2; u = (t - 0.625f) / 0.25f; }
    else {
        i0 = 3;
        u = ((t >= 0.875f) ? (t - 0.875f) : (t + 0.125f)) / 0.25f;
    }
    Color a = c[i0], b = c[(i0 + 1) & 3];
    return (Color){
        (unsigned char)(a.r + (int)((float)(b.r - a.r) * u + 0.5f)),
        (unsigned char)(a.g + (int)((float)(b.g - a.g) * u + 0.5f)),
        (unsigned char)(a.b + (int)((float)(b.b - a.b) * u + 0.5f)),
        255
    };
}

/* One vertex of the card select ring: same pastel angular gradient, in sync
 * with the bar cursor (GetTime()*3, Y-down atan2). */
static void MenuSelectRingVert(float x, float y, float cx, float cy, float ang0,
                               Rectangle src, float texW, float texH, float u, float v)
{
    Color col = MenuBarRingGrad(atan2f(y - cy, x - cx) + ang0);
    rlColor4ub(col.r, col.g, col.b, col.a);
    rlTexCoord2f((src.x + src.width * u) / texW, (src.y + src.height * v) / texH);
    rlVertex2f(x, y);
}

/* A stroke strip along one edge, subdivided so the gradient stays smooth.
 * (x0,y0)->(x1,y1) is the first side of the quad (raylib's first two verts
 * of DrawRectangleRoundedLinesEx); (x2,y2)->(x3,y3) is the opposite side. */
static void MenuSelectRingStrip(float x0, float y0, float x1, float y1,
                                float x2, float y2, float x3, float y3,
                                float cx, float cy, float ang0,
                                Rectangle src, float tw, float th, int n)
{
    if (n < 1) n = 1;
    for (int i = 0; i < n; i++) {
        float t0 = (float)i / (float)n, t1 = (float)(i + 1) / (float)n;
        float ax0 = x0 + (x1 - x0) * t0, ay0 = y0 + (y1 - y0) * t0;
        float ax1 = x0 + (x1 - x0) * t1, ay1 = y0 + (y1 - y0) * t1;
        float bx0 = x3 + (x2 - x3) * t0, by0 = y3 + (y2 - y3) * t0;
        float bx1 = x3 + (x2 - x3) * t1, by1 = y3 + (y2 - y3) * t1;
        MenuSelectRingVert(ax0, ay0, cx, cy, ang0, src, tw, th, 0.0f, 0.0f);
        MenuSelectRingVert(bx0, by0, cx, cy, ang0, src, tw, th, 0.0f, 1.0f);
        MenuSelectRingVert(bx1, by1, cx, cy, ang0, src, tw, th, 1.0f, 1.0f);
        MenuSelectRingVert(ax1, ay1, cx, cy, ang0, src, tw, th, 1.0f, 0.0f);
    }
}

/* Same geometry as DrawRectangleRoundedLinesEx (stroke inward from rec,
 * 16 corner segments); only the color is the rotating pastel gradient. */
static void DrawMenuSelectRing(Rectangle rec, float roundness, int segments, float thick)
{
    float radius, outerRadius, innerRadius, stepLength, ang0;
    float x0, x1, x2, x3, x4, x5, y0, y1, y2, y3, y4, y5, cx, cy;
    Texture2D tex;
    Rectangle src;
    float tw, th;
    const float angles[4] = { 180.0f, 270.0f, 0.0f, 90.0f };
    Vector2 centers[4];
    int k, i, sideN;

    if (roundness <= 0.0f || thick <= 0.0f) return;
    if (roundness >= 1.0f) roundness = 1.0f;
    if (segments < 1) segments = 16;
    radius = (rec.width > rec.height) ? (rec.height * roundness) / 2.0f
                                      : (rec.width * roundness) / 2.0f;
    if (radius <= 0.0f) return;
    outerRadius = radius;
    innerRadius = outerRadius - thick;
    if (innerRadius < 0.0f) innerRadius = 0.0f;

    stepLength = 90.0f / (float)segments;
    x0 = rec.x + outerRadius;
    x1 = (rec.x + rec.width) - outerRadius;
    x2 = rec.x + rec.width;
    x3 = rec.x;
    x4 = (rec.x + rec.width) - thick;
    x5 = rec.x + thick;
    y0 = rec.y;
    y1 = rec.y + outerRadius;
    y2 = (rec.y + rec.height) - outerRadius;
    y3 = rec.y + rec.height;
    y4 = rec.y + thick;
    y5 = (rec.y + rec.height) - thick;
    centers[0] = (Vector2){ x0, y1 };
    centers[1] = (Vector2){ x1, y1 };
    centers[2] = (Vector2){ x1, y2 };
    centers[3] = (Vector2){ x0, y2 };
    cx = rec.x + rec.width * 0.5f;
    cy = rec.y + rec.height * 0.5f;
    ang0 = (float)GetTime() * 3.0f;
    tex = GetShapesTexture();
    src = GetShapesTextureRectangle();
    tw = (float)tex.width;
    th = (float)tex.height;
    if (tw < 1.0f) tw = 1.0f;
    if (th < 1.0f) th = 1.0f;
    sideN = 16;

    rlSetTexture(tex.id);
    rlBegin(RL_QUADS);
    for (k = 0; k < 4; k++) {
        float angle = angles[k];
        Vector2 center = centers[k];
        for (i = 0; i < segments; i++) {
            float a0 = angle, a1 = angle + stepLength;
            float c0 = cosf(DEG2RAD * a0), s0 = sinf(DEG2RAD * a0);
            float c1 = cosf(DEG2RAD * a1), s1 = sinf(DEG2RAD * a1);
            /* Same vertex order as raylib: inner, inner, outer, outer. */
            MenuSelectRingVert(center.x + c0 * innerRadius, center.y + s0 * innerRadius,
                               cx, cy, ang0, src, tw, th, 0.0f, 0.0f);
            MenuSelectRingVert(center.x + c1 * innerRadius, center.y + s1 * innerRadius,
                               cx, cy, ang0, src, tw, th, 1.0f, 0.0f);
            MenuSelectRingVert(center.x + c1 * outerRadius, center.y + s1 * outerRadius,
                               cx, cy, ang0, src, tw, th, 1.0f, 1.0f);
            MenuSelectRingVert(center.x + c0 * outerRadius, center.y + s0 * outerRadius,
                               cx, cy, ang0, src, tw, th, 0.0f, 1.0f);
            angle += stepLength;
        }
    }
    /* Sides: same edges as raylib's four side quads, split for a smooth grad. */
    MenuSelectRingStrip(x0, y0, x1, y0, x1, y4, x0, y4, cx, cy, ang0, src, tw, th, sideN); /* top: P0-P1, P9-P8 */
    MenuSelectRingStrip(x2, y1, x2, y2, x4, y2, x4, y1, cx, cy, ang0, src, tw, th, sideN); /* right: P2-P3, P11-P10 */
    MenuSelectRingStrip(x0, y5, x1, y5, x1, y3, x0, y3, cx, cy, ang0, src, tw, th, sideN); /* lower: P13-P12, P4-P5 */
    MenuSelectRingStrip(x5, y1, x5, y2, x3, y2, x3, y1, cx, cy, ang0, src, tw, th, sideN); /* left: P15-P14, P6-P7 */
    rlEnd();
    rlSetTexture(0);
}

static void DrawMenuBarCursor(Vector2 c, float diam, float ringW)
{
    float pad = 2.0f;
    float side = diam + 2.0f * pad;
    int size = (int)ceilf(side);
    float cx, cy, outer, inner, angle, aa;
    if (!MenuBarCursorTex_Ensure(size)) return;
    cx = (float)size * 0.5f;
    cy = (float)size * 0.5f;
    outer = diam * 0.5f;
    inner = fmaxf(0.0f, outer - ringW);
    angle = (float)GetTime() * 3.0f;
    aa = 0.75f;   /* ~1px analytical AA */
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            float px = (float)x + 0.5f - cx;
            float py = (float)y + 0.5f - cy;
            float d = sqrtf(px * px + py * py);
            float fillA = 1.0f - MenuBarSmoothstep(inner - aa, inner + aa, d);
            float ringA = MenuBarSmoothstep(inner - aa, inner + aa, d)
                        * (1.0f - MenuBarSmoothstep(outer - aa, outer + aa, d));
            float a = fillA + ringA;
            Color out = { 0, 0, 0, 0 };
            if (a > 0.001f) {
                Color fill = MENU_BAR_CURSOR_FILL;
                Color rg = MenuBarRingGrad(atan2f(py, px) + angle);
                float rf = (fill.r * fillA + rg.r * ringA) / a;
                float gf = (fill.g * fillA + rg.g * ringA) / a;
                float bf = (fill.b * fillA + rg.b * ringA) / a;
                if (a > 1.0f) a = 1.0f;
                out = (Color){
                    (unsigned char)(rf + 0.5f),
                    (unsigned char)(gf + 0.5f),
                    (unsigned char)(bf + 0.5f),
                    (unsigned char)(a * 255.0f + 0.5f)
                };
            }
            menuBarCursorPix[y * size + x] = out;
        }
    }
    UpdateTexture(menuBarCursorTex, menuBarCursorPix);
    /* Default shader only: a custom shader on the backbuffer left later
     * textured draws in the same frame (icons, label, HUD) invisible. */
    DrawTexturePro(menuBarCursorTex,
                   (Rectangle){ 0, 0, (float)size, (float)size },
                   (Rectangle){ c.x - (float)size * 0.5f, c.y - (float)size * 0.5f,
                                (float)size, (float)size },
                   (Vector2){ 0, 0 }, 0.0f, WHITE);
}

/* Draw the systems menu shifted by offsetX (used by the mosaic slide). The cards and the
 * selection ring are laid out by SystemRow below. */
static Font slotFont;   /* menu font, defined (and set in main) below */
/* Nerd Font (Symbols Only): N64 C-Up / C-Down, and the status-legend Y. */
#define HINT_NERD_CUP   0xF0062  /* md-arrow_up_drop_circle */
#define HINT_NERD_CDOWN 0xF004A  /* md-arrow_down_drop_circle */
/* nf-md-alpha_y_circle: this font maps it to U+F0C34 (U+F0B8C is md-dns_outline). */
#define HINT_NERD_Y_CIRCLE 0xF0C34
static Font hintNerdFont;
static bool hintNerdLoaded;
static bool hintYNerdLoaded;
#define FAV_HEART   0xF004  /* nf-fa-heart */
#define FAV_HEART_O 0xF08A  /* nf-fa-heart_o */
#define HIDE_BLOCK  0xF00AD /* nf-md-block_helper */
static Font favNerdFont;
static bool favNerdLoaded;
static bool hideNerdLoaded;


/* Main-menu system cards: twice the original size. The original row was laid out in a box of half
 * the screen (card = (0.5 sw * 0.96 - 3 gaps) / 4, gap 0.0125 sw); the card is now double that, the
 * gap and the selection-ring room keep their old absolute size, and the row stays centred on screen
 * (capped so it always fits the width and the height). */
typedef struct { float x0, y, card, gap; } SystemRowLayout;

static SystemRowLayout SystemRow(float sw, float sh) {
    float gap = sw * 0.5f * 0.025f, pad = sw * 0.5f * 0.02f;
    float old = (sw * 0.5f - 2.0f * pad - gap * (SYSTEM_COUNT - 1)) / SYSTEM_COUNT;
    float card = old * 2.0f;
    float maxW = (sw - 2.0f * pad - gap * (SYSTEM_COUNT - 1)) / SYSTEM_COUNT;   /* fits the width with ring room */
    float maxH = sh - 2.0f * pad;
    if (card > maxW) card = maxW;
    if (card > maxH) card = maxH;
    float rowW = card * SYSTEM_COUNT + gap * (SYSTEM_COUNT - 1);
    return (SystemRowLayout){ (sw - rowW) * 0.5f, (sh - card) * 0.5f, card, gap };
}

/* Geometry of system tile i in DrawSystemMenu (offsetX 0). */
static Rectangle SystemTileRect(int i, float *radius) {
    SystemRowLayout L = SystemRow((float)GetScreenWidth(), (float)GetScreenHeight());
    if (radius) *radius = L.card * MENU_CARD_ROUNDNESS * 0.5f;
    return (Rectangle){ L.x0 + (float)i * (L.card + L.gap), L.y, L.card, L.card };
}

/* Switch-home HUD on the system carousel only. Layout is 1:1 at 1920x1080
 * (measured from a 4K capture of the reference, then divided by 2). */
#define STATUS_NERD_WIFI      0xF05A9  /* md-wifi */
#define STATUS_NERD_WIFI_OFF  0xF05AA  /* md-wifi-off */
#define STATUS_NERD_WIFI_DISC 0xF092F  /* md-wifi-strength-outline */
#define STATUS_NERD_BAT_FULL  0xF240   /* fa-battery-full */
#define STATUS_NERD_BAT_75    0xF241
#define STATUS_NERD_BAT_50    0xF242
#define STATUS_NERD_BAT_25    0xF243
#define STATUS_NERD_BAT_0     0xF244
#define STATUS_NERD_BOLT      0xF0E7   /* fa-bolt, shown when charging */
#define STATUS_NERD_CIRCLE_PLUS 0xF055 /* nf-fa-circle_plus */
#define STATUS_A_GLYPH        0x1F150  /* 🅐, same glyph as the games-menu legend */
#define MENU_BAR_DOWNLOAD     0xF019   /* nf-fa-download */
#define MENU_BAR_POWER        0xF0425  /* nf-md-power */
#define MENU_BAR_ICON_COUNT   2
static Font statusNerdFont;
static bool statusNerdLoaded;
static Font menuBarFont;
static bool menuBarFontLoaded;
/* ---- Self-update (Downloads button). On startup a background curl asks the
 * GitHub API for the latest Vulto/nso release; if its tag is newer than
 * NSO_VERSION the download icon glows. Pressing it (only then) runs a
 * background script that downloads the release's linux tarball to a temp dir
 * inside the install dir, verifies it (sha256 when GitHub reports one, ELF
 * check, nso + mednafen present), swaps the files in by rename with rollback,
 * and NSO re-execs itself. Offline or on any error nothing happens. Only the
 * release build (NSO_RELEASE) does this: a dev tree never overwrites itself.
 * NSO_UPDATE_API overrides the API URL (file:// mock for tests). ---- */
#ifndef NSO_VERSION
#define NSO_VERSION "0.2.1"
#endif
static bool nsoQuit = false;
typedef enum { UPD_IDLE = 0, UPD_CHECKING, UPD_AVAILABLE, UPD_INSTALLING } UpdState;
static UpdState updState = UPD_IDLE;
static char updTag[64];
#if defined(NSO_RELEASE) && !defined(_WIN32)
static NsoProc updProc;
static char updUrl[1024], updDigest[80], updJson[2400];
static bool updAnyUrl;
#endif
static double updFailUntil;
static bool nsoRestart;
static char nsoSelfPath[2400];
static char **nsoArgv;

#if defined(NSO_RELEASE) && !defined(_WIN32)
static const char *Update_Script =
    "set -e\n"
    "url=$1; dest=$2; digest=$3\n"
    "tmp=$(mktemp -d \"$dest/.nso-update.XXXXXX\")\n"
    "done_list=\"\"\n"
    "rollback() { for f in $done_list; do mv -f \"$tmp/bak.$f\" \"$dest/$f\" 2>/dev/null || true; done; }\n"
    "trap 'rm -rf \"$tmp\"' EXIT\n"
    "curl -fsSL --max-time 900 --retry 2 -A nso-updater -o \"$tmp/pkg.tar.gz\" \"$url\"\n"
    "if [ -n \"$digest\" ]; then echo \"$digest  $tmp/pkg.tar.gz\" | sha256sum -c - >/dev/null; fi\n"
    "mkdir \"$tmp/x\"\n"
    "tar --no-same-owner --no-same-permissions -xzf \"$tmp/pkg.tar.gz\" -C \"$tmp/x\"\n"
    "d=$(find \"$tmp/x\" -mindepth 1 -maxdepth 1 -type d | head -n 1)\n"
    "[ -n \"$d\" ]\n"
    "for f in nso mednafen; do\n"
    "  [ -f \"$d/$f\" ] && [ -s \"$d/$f\" ]\n"
    "  [ \"$(head -c 4 \"$d/$f\" | tail -c 3)\" = ELF ]\n"
    "  chmod 755 \"$d/$f\"\n"
    "done\n"
    "for f in mednafen nso-norumble.so nso; do\n"
    "  [ -f \"$d/$f\" ] || continue\n"
    "  if [ -e \"$dest/$f\" ]; then ln \"$dest/$f\" \"$tmp/bak.$f\"; fi\n"
    "  if mv -f \"$d/$f\" \"$dest/$f\"; then done_list=\"$done_list $f\"; else rollback; exit 1; fi\n"
    "done\n"
    "exit 0\n";
#endif

#if defined(NSO_RELEASE) && !defined(_WIN32)
static const char *NsoSelfPath(void)
{
    if (nsoSelfPath[0]) return nsoSelfPath;
#ifndef _WIN32
    {
        ssize_t n = readlink("/proc/self/exe", nsoSelfPath, sizeof(nsoSelfPath) - 1);
        static const char deleted[] = " (deleted)";
        if (n > 0) {
            nsoSelfPath[n] = '\0';
            if ((size_t)n > sizeof(deleted) - 1
                && strcmp(nsoSelfPath + (size_t)n - (sizeof(deleted) - 1), deleted) == 0)
                nsoSelfPath[(size_t)n - (sizeof(deleted) - 1)] = '\0';
        } else {
            nsoSelfPath[0] = '\0';
        }
    }
#endif
    return nsoSelfPath;
}
#endif

static void Update_Start(void)
{
#if defined(NSO_RELEASE) && !defined(_WIN32)
    const char *api = getenv("NSO_UPDATE_API");
    char *argv[13];
    NsoSelfPath();
    updAnyUrl = api && api[0];
    if (!updAnyUrl) api = "https://api.github.com/repos/Vulto/nso/releases/latest";
    snprintf(updJson, sizeof(updJson), "%s", GetDataPath("assets/cache/update-latest.json"));
    argv[0] = "curl";
    argv[1] = "-fsSL";
    argv[2] = "--max-time";
    argv[3] = "10";
    argv[4] = "--create-dirs";
    argv[5] = "-A";
    argv[6] = "nso-updater";
    argv[7] = "-H";
    argv[8] = "Accept: application/vnd.github+json";
    argv[9] = "-o";
    argv[10] = updJson;
    argv[11] = (char *)api;
    argv[12] = NULL;
    updProc = NsoSpawnArgv(NULL, argv);
    updState = NsoProcOk(updProc) ? UPD_CHECKING : UPD_IDLE;
#endif
}

static void Update_Poll(void)
{
#if defined(NSO_RELEASE) && !defined(_WIN32)
    int code = 0;
    if (updState != UPD_CHECKING && updState != UPD_INSTALLING) return;
    if (!NsoProcOk(updProc) || !NsoSpawnExited(&updProc, &code)) return;
    updProc = NsoProcNone();
    if (updState == UPD_CHECKING) {
        updState = UPD_IDLE;
        if (code == 0) {
            char *text = LoadFileText(updJson);
            if (text) {
                if (Update_ParseRelease(text, updTag, sizeof(updTag), updUrl, sizeof(updUrl),
                                        updDigest, sizeof(updDigest), updAnyUrl)
                    && Update_VersionNewer(updTag, NSO_VERSION)) {
                    updState = UPD_AVAILABLE;
                    fprintf(stderr, "NSO: update %s available (running %s)\n", updTag, NSO_VERSION);
                }
                UnloadFileText(text);
            }
        }
        FileRemove(updJson);
    } else if (code == 0) {
        updState = UPD_IDLE;
        nsoRestart = true;
        nsoQuit = true;
    } else {
        updState = UPD_AVAILABLE;
        updFailUntil = GetTime() + 5.0;
    }
#endif
}

/* Downloads button pressed: only does anything when an update is waiting. */
static bool Update_Install(void)
{
#if defined(NSO_RELEASE) && !defined(_WIN32)
    char *argv[8];
    if (updState != UPD_AVAILABLE) return false;
    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = (char *)Update_Script;
    argv[3] = "nso-update";
    argv[4] = updUrl;
    argv[5] = (char *)NsoExeDir();
    argv[6] = updDigest;
    argv[7] = NULL;
    updProc = NsoSpawnArgv(NULL, argv);
    if (!NsoProcOk(updProc)) { updFailUntil = GetTime() + 5.0; return false; }
    updState = UPD_INSTALLING;
    updFailUntil = 0.0;
    return true;
#else
    return false;
#endif
}

static bool Update_Glow(void) { return updState == UPD_AVAILABLE || updState == UPD_INSTALLING; }

static int menuBarFocus = -1;     /* -1 = system cards, 0 = download, 1 = power */
static bool powerDialogOpen = false;
static int powerDialogRow = 2;    /* Cancel is the default */
static Texture2D statusPadN64;
static bool statusPadN64Loaded;

static const Color STATUS_INK     = { 227, 229, 228, 255 };
static const Color STATUS_DIM     = { 129, 131, 130, 255 };
static const Color STATUS_LED_ON  = { 156, 217,  79, 255 };
static const Color STATUS_LED_OFF = { 129, 131, 130, 255 };

static float StatusHudK(float sw, float sh)
{
    float k = sw / 1920.0f;
    float ky = sh / 1080.0f;
    if (ky < k) k = ky;
    if (k < 0.01f) k = 1.0f;
    return k;
}

static void StatusCpUtf8(int cp, char out[5])
{
    if (cp < 0x80) {
        out[0] = (char)cp; out[1] = 0;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        out[2] = 0;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        out[3] = 0;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
        out[4] = 0;
    }
}

static Vector2 StatusMeasureCp(Font font, int cp, float size)
{
    char u[5];
    StatusCpUtf8(cp, u);
    return MeasureTextEx(font, u, size, 0.0f);
}

static float StatusDrawCp(Font font, int cp, float x, float midY, float size, Color c)
{
    Vector2 m = StatusMeasureCp(font, cp, size);
    if (m.x <= 1.0f) return 0.0f;
    DrawTextCodepoint(font, cp, (Vector2){ x, midY - m.y * 0.5f }, size, c);
    return m.x;
}

static float StatusGlyphInkH(Font font, int cp, float size)
{
    int i = GetGlyphIndex(font, cp);
    if (i < 0 || font.recs[i].height <= 0.0f) return 0.0f;
    return font.recs[i].height * (size / (float)font.baseSize);
}

static float StatusGlyphOffY(Font font, int cp, float size)
{
    int i = GetGlyphIndex(font, cp);
    if (i < 0) return 0.0f;
    return (float)font.glyphs[i].offsetY * (size / (float)font.baseSize);
}

/* Draw cp so its ink is the same height as inkH and vertically centred on midY. */
static float StatusDrawCpMatched(Font font, int cp, float x, float midY,
                                 float size, float inkH, Color c)
{
    Vector2 m = StatusMeasureCp(font, cp, size);
    float offY = StatusGlyphOffY(font, cp, size);
    if (m.x <= 1.0f) return 0.0f;
    DrawTextCodepoint(font, cp, (Vector2){ x, midY - offY - inkH * 0.5f }, size, c);
    return m.x;
}

static void StatusDrawText(Font font, const char *s, float x, float midY, float size, Color c)
{
    Vector2 m = MeasureTextEx(font, s, size, 0.0f);
    DrawTextEx(font, s, (Vector2){ x, midY - m.y * 0.5f }, size, 0.0f, c);
}

static int StatusBatteryGlyph(int cap)
{
    if (cap >= 90) return STATUS_NERD_BAT_FULL;
    if (cap >= 65) return STATUS_NERD_BAT_75;
    if (cap >= 40) return STATUS_NERD_BAT_50;
    if (cap >= 15) return STATUS_NERD_BAT_25;
    return STATUS_NERD_BAT_0;
}

static int StatusWifiGlyph(int wifi)
{
    if (wifi == SYSSTATUS_WIFI_ON) return STATUS_NERD_WIFI;
    if (wifi == SYSSTATUS_WIFI_DISCONNECTED) return STATUS_NERD_WIFI_DISC;
    return STATUS_NERD_WIFI_OFF;
}

static int Pad_N64Index(void);
static NsoPadLink Pad_LinkNow(void);

static void DrawSystemMenuHud(float offsetX, float sw, float sh)
{
    const float k = StatusHudK(sw, sh);
    const float fs = 44.0f * k;
    const float midTop = 113.5f * k;
    Font textFont = (slotFont.texture.id != 0) ? slotFont : GetFontDefault();
    SysStatus st;
    float x, gapTw, gapWp, gapPb, iconWifi, iconBat;
    char percent[8];
    Vector2 timeM, pctM, optM, startM, aM, plusM;
    float iconHint, midBot, startX, aX, optX, plusX, plusH, aH0, aSize;
    float bodyW, bodyH, bodyCx, bodyBot, bodyY, sq, gapSq, ledY, led0;

    SysStatus_Poll(&st, GetTime());

    /* Top right: time, Wi-Fi, [percent + battery]. Right-aligned to sw-72. */
    timeM = MeasureTextEx(textFont, st.time, fs, 0.0f);
    gapTw = 29.0f * k;
    gapWp = 31.0f * k;
    gapPb = 22.0f * k;
    iconWifi = 32.0f * k;
    iconBat = 36.0f * k;
    x = sw - 72.0f * k;
    if (st.hasBattery && statusNerdLoaded) {
        int bcp = StatusBatteryGlyph(st.capacity);
        Vector2 bm = StatusMeasureCp(statusNerdFont, bcp, iconBat);
        Vector2 boltM = { 0 };
        if (st.charging) boltM = StatusMeasureCp(statusNerdFont, STATUS_NERD_BOLT, iconBat * 0.72f);
        if (st.charging && boltM.x > 1.0f) {
            x -= boltM.x;
            StatusDrawCp(statusNerdFont, STATUS_NERD_BOLT, x, midTop, iconBat * 0.72f, STATUS_INK);
            x -= 6.0f * k;
        }
        x -= bm.x;
        StatusDrawCp(statusNerdFont, bcp, x, midTop, iconBat, STATUS_INK);
        x -= gapPb;
        snprintf(percent, sizeof(percent), "%d%%", st.capacity);
        pctM = MeasureTextEx(textFont, percent, fs, 0.0f);
        x -= pctM.x;
        StatusDrawText(textFont, percent, x, midTop, fs, STATUS_INK);
        x -= gapWp;
    }
    if (statusNerdLoaded) {
        int wcp = StatusWifiGlyph(st.wifi);
        Vector2 wm = StatusMeasureCp(statusNerdFont, wcp, iconWifi);
        Color wc = (st.wifi == SYSSTATUS_WIFI_ON) ? STATUS_INK : STATUS_DIM;
        x -= wm.x;
        StatusDrawCp(statusNerdFont, wcp, x, midTop, iconWifi, wc);
        x -= gapTw;
    }
    StatusDrawText(textFont, st.time, x - timeM.x, midTop, fs, STATUS_INK);

    /* Bottom right: (circle-plus) Options   (🅐) Start — no key binds.
     * A is the same 🅐 glyph as the games-menu legend, scaled so its ink
     * height and vertical alignment match nf-fa-circle_plus. */
    midBot = sh - 61.0f * k;
    iconHint = 36.0f * k;
    startM = MeasureTextEx(textFont, "Start", fs, 0.0f);
    optM = MeasureTextEx(textFont, "Options", fs, 0.0f);
    plusH = (statusNerdLoaded
             ? StatusGlyphInkH(statusNerdFont, STATUS_NERD_CIRCLE_PLUS, iconHint)
             : iconHint);
    aH0 = StatusGlyphInkH(textFont, STATUS_A_GLYPH, iconHint);
    aSize = (aH0 > 0.1f && plusH > 0.1f) ? iconHint * (plusH / aH0) : iconHint;
    aM = StatusMeasureCp(textFont, STATUS_A_GLYPH, aSize);
    plusM = statusNerdLoaded ? StatusMeasureCp(statusNerdFont, STATUS_NERD_CIRCLE_PLUS, iconHint)
                             : (Vector2){ 0 };
    if (menuBarFocus >= 0) {
        Vector2 okM = MeasureTextEx(textFont, "OK", fs, 0.0f);
        float okX = sw - 97.0f * k - okM.x;
        aX = okX - 15.0f * k - aM.x;
        StatusDrawCpMatched(textFont, STATUS_A_GLYPH, aX, midBot, aSize, plusH, STATUS_INK);
        StatusDrawText(textFont, "OK", okX, midBot, fs, STATUS_INK);
    } else {
        startX = sw - 97.0f * k - startM.x;
        aX = startX - 15.0f * k - aM.x;
        optX = aX - 43.0f * k - optM.x;
        plusX = optX - 16.0f * k - plusM.x;
        if (statusNerdLoaded)
            StatusDrawCpMatched(statusNerdFont, STATUS_NERD_CIRCLE_PLUS, plusX, midBot,
                                iconHint, plusH, STATUS_INK);
        StatusDrawText(textFont, "Options", optX, midBot, fs, STATUS_INK);
        StatusDrawCpMatched(textFont, STATUS_A_GLYPH, aX, midBot, aSize, plusH, STATUS_INK);
        StatusDrawText(textFont, "Start", startX, midBot, fs, STATUS_INK);
    }

    /* Bottom left: N64 icon + P1–P4 squares, only when a real pad is present.
     * Same filter as the input layer (raylib slot with an N64 name, or an
     * N64 entry in /proc/bus/input/devices). Do not use bare IsGamepadAvailable:
     * the cluster used to draw even with every slot empty. */
    if (Pad_N64Index() >= 0 || Pad_LinkNow() != NSO_PAD_LINK_NONE) {
        int player = Nso_PadPlayerFromIndex(Pad_N64Index());
        bodyCx = offsetX + 131.5f * k;
        bodyBot = sh - 36.5f * k;
        if (statusPadN64Loaded) {
            bodyH = (float)statusPadN64.height * k;
            bodyW = (float)statusPadN64.width * k;
            bodyY = bodyBot - bodyH;
            DrawTexturePro(statusPadN64,
                           (Rectangle){ 0, 0, (float)statusPadN64.width, (float)statusPadN64.height },
                           (Rectangle){ bodyCx - bodyW * 0.5f, bodyY, bodyW, bodyH },
                           (Vector2){ 0 }, 0.0f, WHITE);
        } else {
            bodyH = 72.0f * k;
            bodyY = bodyBot - bodyH;
        }
        sq = 12.0f * k;
        gapSq = 5.0f * k;
        ledY = bodyY - 10.0f * k - sq;
        led0 = bodyCx - (4.0f * sq + 3.0f * gapSq) * 0.5f;
        for (int i = 0; i < 4; i++) {
            Color led = Nso_PadPlayerLedOn(player, i + 1) ? STATUS_LED_ON : STATUS_LED_OFF;
            DrawRectangleRec((Rectangle){ led0 + (float)i * (sq + gapSq), ledY, sq, sq }, led);
        }
    }
}


/* Pill under the system cards (Switch-home dock). Sizes are 1:1 at 1920x1080,
 * then scaled slightly down. Width follows the items (slots + equal end pads)
 * and the bar is centred horizontally. Vertically it sits in the middle of the
 * space between the cards and the bottom Options/Start legend, so both gaps match. */
static const Color MENU_BAR_FILL = { 58, 58, 58, 255 };     /* #3a3a3a */
static const Color MENU_BAR_ICON_COL = { 192, 192, 192, 255 }; /* #c0c0c0 */
static const float MENU_BAR_H = 140.0f;
static const float MENU_BAR_ICON = 84.0f;
static const float MENU_BAR_END_PAD = 5.0f;      /* circle edge to bar end, px at 1920 */
static const float MENU_BAR_CIRCLE_GAP = 20.0f;  /* gap between cursor circles, px at 1920 */
static const float MENU_BAR_SCALE = 0.85f;

static void DrawSystemMenuBar(float offsetX, float sw, float sh, SystemRowLayout L)
{
    const float kHud = StatusHudK(sw, sh);
    const float k = kHud * MENU_BAR_SCALE;
    float margin = MENU_BAR_CURSOR_MARGIN * kHud;
    float endPad = MENU_BAR_END_PAD * kHud;
    float gap = MENU_BAR_CIRCLE_GAP * kHud;
    float bh = MENU_BAR_H * k;
    float diam = fmaxf(1.0f, bh - 2.0f * margin);
    /* Horizontal: 5px end pad (circle edge to bar edge) and 20px gap
     * between the two cursor circles, at 1920x1080. Vertical margin and
     * circle size are unchanged. */
    float span = diam + gap;
    float pad = endPad + diam * 0.5f;
    float bw = diam + 2.0f * endPad + (float)(MENU_BAR_ICON_COUNT - 1) * span;
    Font textFont = (slotFont.texture.id != 0) ? slotFont : GetFontDefault();
    float fs = 44.0f * kHud, midBot = sh - 61.0f * kHud, plusH, textH, legendTop, cardsBot;
    float bx, by, midY, icon, firstCx;
    textH = MeasureTextEx(textFont, "Start", fs, 0.0f).y;
    plusH = statusNerdLoaded
          ? StatusGlyphInkH(statusNerdFont, STATUS_NERD_CIRCLE_PLUS, 36.0f * kHud)
          : 36.0f * kHud;
    legendTop = midBot - fmaxf(textH, plusH) * 0.5f;
    cardsBot = L.y + L.card;
    bx = offsetX + (sw - bw) * 0.5f;
    by = cardsBot + (legendTop - cardsBot - bh) * 0.5f;
    midY = by + bh * 0.5f;
    icon = MENU_BAR_ICON * k;
    firstCx = bx + pad;
    const int cps[MENU_BAR_ICON_COUNT] = { MENU_BAR_DOWNLOAD, MENU_BAR_POWER };
    Rectangle bar = { bx, by, bw, bh };

    DrawRectangleRounded(bar, 1.0f, 64, MENU_BAR_FILL);
    if (!menuBarFontLoaded) return;
    {
        float ringW = diam * MENU_BAR_CURSOR_RING + MENU_BAR_CURSOR_RING_ADD * kHud;
        for (int i = 0; i < MENU_BAR_ICON_COUNT; i++) {
            float cx = firstCx + (float)i * span;
            bool sel = (menuBarFocus == i);
            bool glow = (i == 0 && Update_Glow());
            Vector2 m;
            if (glow) {   /* an update is waiting: soft pulsing halo behind the download icon */
                float pulse = 0.55f + 0.45f * sinf((float)GetTime() * 3.0f);
                for (int g = 4; g >= 1; g--)
                    DrawCircleV((Vector2){ cx, midY }, diam * (0.5f + 0.16f * (float)g),
                                Fade(MENU_BAR_LABEL_COL, 0.10f * pulse));
            }
            if (sel) {
                DrawMenuBarCursor((Vector2){ cx, midY }, diam, ringW);
            }
            m = StatusMeasureCp(menuBarFont, cps[i], icon);
            StatusDrawCp(menuBarFont, cps[i], cx - m.x * 0.5f, midY, icon,
                         (sel || glow) ? WHITE : MENU_BAR_ICON_COL);
            if (sel && i == 0 && (Update_Glow() || GetTime() < updFailUntil)) {
                char note[96];
                float nfs = 32.0f * kHud;
                Vector2 nm;
                if (GetTime() < updFailUntil) snprintf(note, sizeof(note), "%s", T(S_UPD_FAILED));
                else snprintf(note, sizeof(note), T(updState == UPD_INSTALLING ? S_UPD_INSTALLING : S_UPD_AVAILABLE), updTag);
                nm = MeasureTextEx(textFont, note, nfs, 0.0f);
                DrawTextEx(textFont, note, (Vector2){ bx + (bw - nm.x) * 0.5f, by - nm.y - 14.0f * kHud },
                           nfs, 0.0f, MENU_BAR_LABEL_COL);
            }
            if (sel) {
                const char *lab = T(i == 1 ? S_BAR_SHUTDOWN : S_BAR_DOWNLOADS);
                float lfs = 36.0f * kHud;
                Vector2 lm = MeasureTextEx(textFont, lab, lfs, 0.0f);
                float ly = midY + diam * 0.5f + 10.0f * kHud;
                DrawTextEx(textFont, lab, (Vector2){ cx - lm.x * 0.5f, ly }, lfs, 0.0f,
                           MENU_BAR_LABEL_COL);
            }
        }
    }
}

static void DrawPowerDialog(float sw, float sh)
{
    const float k = StatusHudK(sw, sh);
    Font font = (slotFont.texture.id != 0) ? slotFont : GetFontDefault();
    const float fs = 44.0f * k, rowH = 72.0f * k, pad = 28.0f * k;
    const float pw = 560.0f * k, ph = pad * 2.0f + rowH * 3.0f;
    float px = (sw - pw) * 0.5f, py = (sh - ph) * 0.5f;
    static const StrId labels[3] = { S_PWR_SHUTDOWN, S_PWR_CLOSE, S_PWR_CANCEL };

    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.45f));
    DrawRectangleRounded((Rectangle){ px, py, pw, ph }, 0.12f, 16, MENU_BG);
    for (int i = 0; i < 3; i++) {
        float y = py + pad + rowH * (float)i;
        bool sel = (powerDialogRow == i);
        const char *s = T(labels[i]);
        Vector2 m;
        if (sel) {
            DrawRectangleRounded((Rectangle){ px + pad * 0.4f, y, pw - pad * 0.8f, rowH },
                                 0.35f, 12, MENU_BAR_FILL);
        }
        m = MeasureTextEx(font, s, fs, 0.0f);
        DrawTextEx(font, s, (Vector2){ px + (pw - m.x) * 0.5f, y + (rowH - m.y) * 0.5f },
                   fs, 0.0f, sel ? WHITE : MENU_BAR_ICON_COL);
    }
}

static void Power_ShutDown(void)
{
#ifndef _WIN32
    pid_t pid = fork();
    if (pid == 0) {
        execlp("doas", "doas", "poweroff", (char *)NULL);
        _exit(127);
    }
#endif
}

/* System-card art. 16:9 banners keep the existing square centre crop; wider
 * art (Mega Drive is 2:1) is fitted in full so the logo is not cropped. */
static void DrawSystemCardArt(const PlatformEntry *p, Rectangle dst, float r, Color tint)
{
    float tw, th, side, k;
    Rectangle src;
    if (!p || p->texture.id == 0) return;
    tw = (float)p->texture.width;
    th = (float)p->texture.height;
    if (th < 1.0f || tw < 1.0f) return;
    if (tw * 9.0f > th * 16.0f + 1.0f) {
        float scale = fminf(dst.width / tw, dst.height / th);
        float w = tw * scale, h = th * scale;
        Rectangle d = { dst.x + (dst.width - w) * 0.5f,
                        dst.y + (dst.height - h) * 0.5f, w, h };
        DrawTexturePro(p->texture, (Rectangle){ 0, 0, tw, th }, d,
                       (Vector2){ 0 }, 0.0f, tint);
        return;
    }
    side = th;
    src = (Rectangle){ (tw - side) * 0.5f, 0.0f, side, side };
    k = side / dst.width;
    DrawTexturePro(p->texture,
                   (Rectangle){ src.x, src.y + r * k, side, side - 2.0f * r * k },
                   (Rectangle){ dst.x, dst.y + r, dst.width, dst.height - 2.0f * r },
                   (Vector2){ 0 }, 0.0f, tint);
    DrawTexturePro(p->texture,
                   (Rectangle){ src.x + r * k, src.y, side - 2.0f * r * k, side },
                   (Rectangle){ dst.x + r, dst.y, dst.width - 2.0f * r, dst.height },
                   (Vector2){ 0 }, 0.0f, tint);
}

static void DrawSystemMenu(float offsetX) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    DrawRectangleRec((Rectangle){ offsetX, 0.0f, sw, sh }, MENU_BG);

    SystemRowLayout L = SystemRow(sw, sh);
    float gap = L.gap, card = L.card;
    float x0 = offsetX + L.x0, y = L.y;
    float r = card * MENU_CARD_ROUNDNESS * 0.5f; /* corner radius in px */

    for (int i = 0; i < SYSTEM_COUNT; i++) {
        const PlatformEntry *p = systems[i].platform;
        Rectangle dst = { x0 + (float)i * (card + gap), y, card, card };

        /* No system art (not shipped in the repo): grey card + system name. */
        DrawRectangleRounded(dst, MENU_CARD_ROUNDNESS, 16,
                             p->texture.id != 0 ? p->bg : GetColor(0x3A3A3AFF));
        if (p->texture.id == 0 && slotFont.texture.id != 0 && p->name) {
            float fs = card * 0.16f;
            Vector2 m = MeasureTextEx(slotFont, p->name, fs, 1.0f);
            if (m.x > card * 0.9f) { fs *= card * 0.9f / m.x; m = MeasureTextEx(slotFont, p->name, fs, 1.0f); }
            DrawTextEx(slotFont, p->name, (Vector2){ dst.x + (card - m.x) * 0.5f, dst.y + (card - m.y) * 0.5f },
                       fs, 1.0f, LIGHTGRAY);
        }

        if (p->texture.id != 0)
            DrawSystemCardArt(p, dst, r, WHITE);

        if (i == currentSystem && menuBarFocus < 0) {
            /* Black gap, then a pastel gradient ring in sync with the bar cursor.
             * Hidden while the dock bar is focused, so only one indicator is on screen. */
            float gapT, ringT;
            Rectangle outer;
            MenuSelectRingParams(card, &gapT, &ringT);
            outer = (Rectangle){ dst.x - gapT, dst.y - gapT, card + 2.0f * gapT, card + 2.0f * gapT };
            DrawRectangleRoundedLinesEx(dst, MENU_CARD_ROUNDNESS, 16, gapT, BLACK);
            DrawMenuSelectRing(outer, 2.0f * (r + gapT) / outer.width, 16, ringT);
        }
    }

    DrawSystemMenuBar(offsetX, sw, sh, L);
    DrawSystemMenuHud(offsetX, sw, sh);
    if (powerDialogOpen) DrawPowerDialog(sw, sh);
}

/* Live pad profile + the existing info-bar close/open (INFO_BAR_FADE_SEC /
 * INFO_BAR_SLIDE_PX) when the hint labels change. fade starts at 1 so a
 * frame before the first Pad_Update stays pixel-identical. */
static NsoPadWatch padWatch = { .fade = 1.0f };
static Texture2D padNoticeTex;
static double padNoticeUntil;
static double padNoticeStart;
static const float PAD_NOTICE_SEC = 4.0f;

static NsoPadLink Pad_LinkNow(void);

static NsoPadProfile Pad_Detect(void) {
    NsoPadProfile env;
    const char *flip = getenv("NSO_PAD_FLIP_AT");
    if (Nso_PadStyleEnvOverride(&env)) return env;
    if (flip && flip[0] && GetTime() >= atof(flip)) return NSO_PAD_PROFILE_N64;
    /* USB and Bluetooth both get N64 labels; no N64 pad keeps the default bar. */
    return Nso_PadProfileFromLink(Pad_LinkNow());
}

static void PadNotice_Show(void) {
    padNoticeStart = GetTime();
    padNoticeUntil = padNoticeStart + (double)PAD_NOTICE_SEC;
}

static void Pad_Update(float dt) {
    static NsoPadLink seen;
    static bool inited;
    NsoPadLink link = Pad_LinkNow();
    bool shown = false;

    Nso_PadWatchUpdate(&padWatch, Pad_Detect(), dt, INFO_BAR_FADE_SEC);
    if (!inited) {
        seen = link;
        inited = true;
    } else if (link != seen) {
        if (link == NSO_PAD_LINK_USB || link == NSO_PAD_LINK_BT) {
            PadNotice_Show();
            shown = true;
        }
        seen = link;
    }
    if (padWatch.notice) {
        padWatch.notice = false;
        if (!shown) PadNotice_Show();
    }
}

static NsoPadProfile Pad_ActiveProfile(void) {
    return padWatch.drawn;
}

/* The user asset also has baked LEDs and a battery; keep only the controller. */
#define PAD_NOTICE_CROP_X 22
#define PAD_NOTICE_CROP_Y 40
#define PAD_NOTICE_CROP_W 76
#define PAD_NOTICE_CROP_H 69
#define PAD_NOTICE_MAC "e4:17:d8:9d:7f:79"

static void PadNotice_Load(void) {
    const char *path = GetDataPath("assets/images/n64_controller.png");
    if (!FileExists(path)) return;
    Image img = LoadImage(path);
    if (!img.data) return;
    if (img.width >= PAD_NOTICE_CROP_X + PAD_NOTICE_CROP_W
        && img.height >= PAD_NOTICE_CROP_Y + PAD_NOTICE_CROP_H) {
        ImageCrop(&img, (Rectangle){
            (float)PAD_NOTICE_CROP_X, (float)PAD_NOTICE_CROP_Y,
            (float)PAD_NOTICE_CROP_W, (float)PAD_NOTICE_CROP_H
        });
    }
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if (img.data && img.format == PIXELFORMAT_UNCOMPRESSED_R8G8B8A8) {
        int n = img.width * img.height;
        unsigned char *px = (unsigned char *)img.data;
        for (int i = 0; i < n; i++, px += 4) {
            int dr = (int)px[0] - 49, dg = (int)px[1] - 51, db = (int)px[2] - 51;
            if (dr * dr + dg * dg + db * db <= 30 * 30) {
                px[3] = 0;
            }
        }
    }
    padNoticeTex = LoadTextureFromImage(img);
    UnloadImage(img);
    if (padNoticeTex.id) {
        GenTextureMipmaps(&padNoticeTex);
        SetTextureFilter(padNoticeTex, TEXTURE_FILTER_BILINEAR);
    }
}

static int Pad_N64Index(void) {
    for (int i = 0; i < NSO_PAD_MAX; i++) {
        if (IsGamepadAvailable(i) && Nso_PadNameIsN64(GetGamepadName(i))) return i;
    }
    return -1;
}

static NsoPadLink Pad_LinkNow(void) {
    /* Opening /proc/bus/input/devices every frame (~80 times/s) is enough
     * to show up on a quiet grid. Cache the result for about a second, and
     * refresh sooner if inotify sees a change under /dev/input (hot-plug). */
    static NsoPadLink cached = NSO_PAD_LINK_NONE;
    static double cachedAt = -1.0;
    static int soon = 0;
#ifdef __linux__
    static int inofd = -2;   /* -2 = not tried, -1 = unavailable */
#endif
    char buf[16384];
    size_t n = 0;
    FILE *f;
    NsoPadLink link;
    double now = GetTime();
    int refresh = 0;
    double hold = soon ? 0.2 : 1.0;
#ifdef __linux__
    if (inofd == -2) {
        inofd = inotify_init();
        if (inofd >= 0) {
            int fl = fcntl(inofd, F_GETFL, 0);
            if (fl >= 0) fcntl(inofd, F_SETFL, fl | O_NONBLOCK);
            fcntl(inofd, F_SETFD, FD_CLOEXEC);
            if (inotify_add_watch(inofd, "/dev/input",
                    IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO | IN_ATTRIB) < 0) {
                close(inofd);
                inofd = -1;
            }
        }
    }
    if (inofd >= 0) {
        char ev[4096];
        ssize_t got = read(inofd, ev, sizeof ev);
        if (got > 0) refresh = 1;
    }
#endif
    if (!refresh && cachedAt >= 0.0 && now >= cachedAt && now - cachedAt < hold)
        return cached;
    f = fopen("/proc/bus/input/devices", "r");
    if (!f) {
        cached = NSO_PAD_LINK_NONE;
        cachedAt = now;
        soon = refresh;
        return cached;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    link = Nso_PadLinkFromProc(buf, PAD_NOTICE_MAC);
    if (link == NSO_PAD_LINK_NONE) link = Nso_PadLinkFromProc(buf, NULL);
    cached = link;
    cachedAt = now;
    soon = refresh;   /* udev may publish /proc a moment after /dev/input */
    return cached;
}

static void PadNotice_DrawLeds(Rectangle row, int player) {
    const Color on = { 176, 245, 91, 255 };
    const Color off = { 144, 146, 146, 255 };
    float gap = row.width * 0.08f;
    float side = (row.width - gap * 3.0f) / 4.0f;
    if (side > row.height) side = row.height;
    float x0 = row.x + (row.width - (side * 4.0f + gap * 3.0f)) * 0.5f;
    float y0 = row.y + (row.height - side) * 0.5f;
    for (int i = 0; i < 4; i++) {
        Rectangle r = { x0 + (float)i * (side + gap), y0, side, side };
        DrawRectangleRec(r, Nso_PadPlayerLedOn(player, i + 1) ? on : off);
    }
}

static void PadNotice_DrawBattery(Rectangle box, int pct) {
    const Color outline = { 80, 84, 86, 255 };
    float bodyW = box.width * 0.82f, nubW = box.width * 0.10f, th = fmaxf(2.0f, box.height * 0.10f);
    float h = box.height * 0.70f;
    float x = box.x + (box.width - (bodyW + nubW)) * 0.5f;
    float y = box.y + (box.height - h) * 0.5f;
    Rectangle body = { x, y, bodyW, h };
    Rectangle nub = { x + bodyW, y + h * 0.28f, nubW, h * 0.44f };
    DrawRectangleLinesEx(body, th, outline);
    DrawRectangleRec(nub, outline);
    if (pct > 0) {
        float inset = th + 1.0f;
        float fw = (bodyW - inset * 2.0f) * ((float)pct / 100.0f);
        if (fw > 0.0f) {
            DrawRectangleRec((Rectangle){ x + inset, y + inset, fw, h - inset * 2.0f },
                             (Color){ 176, 245, 91, 255 });
        }
    }
}

/* USB trident: stem + three branches (circle / bar / square). */
static void PadNotice_DrawUsb(Rectangle box, Color col) {
    float s = fminf(box.width, box.height);
    float cx = box.x + box.width * 0.5f, cy = box.y + box.height * 0.5f;
    float th = fmaxf(2.0f, s * 0.10f);
    float stemH = s * 0.55f, stemY = cy - s * 0.05f;
    DrawRectangleRec((Rectangle){ cx - th * 0.5f, stemY, th, stemH }, col);
    DrawRectangleRec((Rectangle){ cx - s * 0.28f, stemY, s * 0.56f, th }, col);
    DrawCircleV((Vector2){ cx - s * 0.28f, stemY + th * 0.5f }, s * 0.11f, col);
    DrawRectangleRec((Rectangle){ cx + s * 0.22f, stemY - s * 0.04f, s * 0.16f, s * 0.16f }, col);
    DrawRectangleRec((Rectangle){ cx - th * 0.5f, stemY - s * 0.22f, th, s * 0.22f }, col);
    /* triangle tip */
    DrawTriangle((Vector2){ cx, stemY - s * 0.38f },
                 (Vector2){ cx - s * 0.11f, stemY - s * 0.18f },
                 (Vector2){ cx + s * 0.11f, stemY - s * 0.18f }, col);
}

static void PadNotice_Unload(void) {
    if (padNoticeTex.id) UnloadTexture(padNoticeTex);
    padNoticeTex = (Texture2D){ 0 };
}

/* opacity 0..1; ySlide = (1-opacity)*INFO_BAR_SLIDE_PX so close is reverse of open.
 * Far-right aligned; 10px margin between text and rounded border. */

static int Hint_N64NerdCodepoint(int cp) {
    if (cp == 0x1F167) return HINT_NERD_CUP;    /* 🅧 Details → C-Up */
    if (cp == 0x1F168) return HINT_NERD_CDOWN;  /* 🅨 Move    → C-Down */
    return 0;
}

static float Hint_GlyphAdvance(Font font, int cp, float fontSize) {
    float scale = fontSize / (float)font.baseSize;
    int index = GetGlyphIndex(font, cp);
    if (font.glyphs[index].advanceX == 0) return font.recs[index].width * scale;
    return (float)font.glyphs[index].advanceX * scale;
}

static float Hint_GlyphHeight(Font font, int cp, float fontSize) {
    float scale = fontSize / (float)font.baseSize;
    int index = GetGlyphIndex(font, cp);
    return font.recs[index].height * scale;
}

/* Scale the status-legend Y so its ink height matches 🅐 (and 🅑). */
static float Hint_YCircleSize(Font ui, float fontSize) {
    float aH = Hint_GlyphHeight(ui, 0x1F150, fontSize);
    float yH = Hint_GlyphHeight(hintNerdFont, HINT_NERD_Y_CIRCLE, fontSize);
    if (aH <= 0.0f || yH <= 0.0f) return fontSize;
    return fontSize * (aH / yH);
}

static int Hint_TextHasCp(const char *text, int want) {
    const char *p = text;
    if (!p) return 0;
    while (*p) {
        int sz = 0, cp = GetCodepointNext(p, &sz);
        p += sz;
        if (cp == want) return 1;
    }
    return 0;
}

/* N64 bar: same string as default, but 🅧/🅨 are drawn from hintNerdFont.
 * The status-legend Y (nf-md-alpha_y_circle) is also drawn from that font,
 * scaled and aligned to the 🅐/🅑 glyphs. */
static float Hint_N64Width(Font font, const char *text, float fontSize, float spacing) {
    float w = 0.0f;
    const char *p = text;
    while (*p) {
        int sz = 0;
        int cp = GetCodepointNext(p, &sz);
        p += sz;
        if (cp == HINT_NERD_Y_CIRCLE && hintYNerdLoaded) {
            float ys = Hint_YCircleSize(font, fontSize);
            w += Hint_GlyphAdvance(hintNerdFont, HINT_NERD_Y_CIRCLE, ys) + spacing;
            continue;
        }
        int nerd = Hint_N64NerdCodepoint(cp);
        Font f = nerd ? hintNerdFont : font;
        int drawcp = nerd ? nerd : cp;
        w += Hint_GlyphAdvance(f, drawcp, fontSize) + spacing;
    }
    if (w > 0.0f) w -= spacing;
    return w;
}

static void Hint_N64Draw(Font font, const char *text, Vector2 pos, float fontSize, float spacing, float lineH, Color tint) {
    float x = pos.x;
    const char *p = text;
    while (*p) {
        int sz = 0;
        int cp = GetCodepointNext(p, &sz);
        p += sz;
        if (cp == HINT_NERD_Y_CIRCLE && hintYNerdLoaded) {
            float ys = Hint_YCircleSize(font, fontSize);
            float aOff = StatusGlyphOffY(font, 0x1F150, fontSize);
            float yOff = StatusGlyphOffY(hintNerdFont, HINT_NERD_Y_CIRCLE, ys);
            float aH = Hint_GlyphHeight(font, 0x1F150, fontSize);
            float yH = Hint_GlyphHeight(hintNerdFont, HINT_NERD_Y_CIRCLE, ys);
            float y = pos.y + aOff - yOff + (aH - yH) * 0.5f;
            DrawTextCodepoint(hintNerdFont, HINT_NERD_Y_CIRCLE, (Vector2){ x, y }, ys, tint);
            x += Hint_GlyphAdvance(hintNerdFont, HINT_NERD_Y_CIRCLE, ys) + spacing;
            continue;
        }
        int nerd = Hint_N64NerdCodepoint(cp);
        Font f = nerd ? hintNerdFont : font;
        int drawcp = nerd ? nerd : cp;
        float y = pos.y;
        if (nerd) {
            float h = Hint_GlyphHeight(f, drawcp, fontSize);
            y = pos.y + (lineH - h) * 0.5f;
        }
        DrawTextCodepoint(f, drawcp, (Vector2){ x, y }, fontSize, tint);
        x += Hint_GlyphAdvance(f, drawcp, fontSize) + spacing;
    }
}

static void DrawInfoBarEx(Font font, const char *text, float opacity, float ySlide) {
    /* When the hint labels switch, reuse the stock close/open: multiply the
     * caller's opacity by padWatch.fade and add the stock slide on top.
     * At rest fade is 1, so this is a no-op (pixel-identical). */
    opacity *= padWatch.fade;
    ySlide += (1.0f - padWatch.fade) * INFO_BAR_SLIDE_PX;
    if (opacity <= 0.001f || !text) return;

    const float textPad = 10.0f;
    float fontSize = (float)font.baseSize;
    float spacing = 2.0f;
    char hintBuf[256];
    const char *draw = Nso_HintTextForProfile(Pad_ActiveProfile(), text, hintBuf, sizeof hintBuf);
    bool padBar = (text == PadUi_BarText());   /* the controller screen's bar: rings round its A / B glyphs */
    bool n64Hints = !padBar && (Pad_ActiveProfile() == NSO_PAD_PROFILE_N64) && hintNerdLoaded;
    bool yHint = !padBar && hintYNerdLoaded && Hint_TextHasCp(draw, HINT_NERD_Y_CIRCLE);
    bool mixedHints = n64Hints || yHint;
    const char *mix = n64Hints ? text : draw;
    float glyphGap = 0.0f, glyphGapZ = 0.0f;
    Vector2 textSize;
    if (mixedHints) {
        textSize.x = Hint_N64Width(font, mix, fontSize, spacing);
        textSize.y = MeasureTextEx(font, n64Hints ? text : draw, fontSize, spacing).y;
        if (yHint)
            textSize.y = MeasureTextEx(font, "🅐", fontSize, spacing).y;
    } else {
        textSize = MeasureTextEx(font, draw, fontSize, spacing);
    }
    if (padBar) {
        /* room after each ringed A / B glyph / ZR word: the ring's edge ends 3 px short of the ':' */
        float th = fmaxf(2.0f, fontSize * 0.07f);
        float adv = Hint_GlyphAdvance(font, 0x1F151, fontSize);
        float zw = Hint_GlyphAdvance(font, 'Z', fontSize) + spacing + Hint_GlyphAdvance(font, 'R', fontSize);
        float zr = fmaxf(adv * 0.62f, zw * 0.5f + th);
        glyphGap = fmaxf(0.0f, adv * 0.12f + th * 0.5f + 3.0f - spacing);
        glyphGapZ = fmaxf(0.0f, (zr - zw * 0.5f) + th * 0.5f + 3.0f - spacing);
        for (const char *q = draw; *q; ) {
            int sz = 0, cp = GetCodepointNext(q, &sz);
            q += sz;
            if ((cp == 0x1F150 || cp == 0x1F151) && *q == ':') textSize.x += glyphGap;
            if (cp == 'R' && q - sz > draw && q[-sz - 1] == 'Z' && *q == ':') textSize.x += glyphGapZ;
        }
    }
    float barW = textSize.x + textPad * 2.0f;
    if (barW < 40.0f) barW = 40.0f;
    float barH = 55.0f;

    float screenW = (float)GetScreenWidth();
    float baseY = (float)GetScreenHeight() - 75.0f;
    /* Flush to far right; textPad is only inside the rounded rect. */
    float barX = screenW - barW +10;
    if (barX < 0.0f) {
        barX = 0.0f;
    }

    Rectangle rec = {
        .x = barX,
        .y = baseY + ySlide,
        .width = barW,
        .height = barH
    };

    DrawRectangleRounded(
        rec,
        0.35f,
        8,
        Fade(BLACK, 0.8f * opacity)
    );

    Vector2 textPosition = {
        .x = barX + textPad,
        .y = rec.y + (barH - textSize.y) * 0.5f
    };

    if (padBar) {
        float x = textPosition.x, zx = 0.0f;
        float th = fmaxf(2.0f, fontSize * 0.07f);
        float zadv = Hint_GlyphAdvance(font, 'Z', fontSize) + spacing + Hint_GlyphAdvance(font, 'R', fontSize);
        for (const char *q = draw; *q; ) {
            int sz = 0, cp = GetCodepointNext(q, &sz);
            float adv = Hint_GlyphAdvance(font, cp, fontSize);
            bool zrEnd = (cp == 'R' && q > draw && q[-1] == 'Z');
            if (cp == 'Z' && q[sz] == 'R') zx = x;
            q += sz;
            DrawTextCodepoint(font, cp, (Vector2){ x, textPosition.y }, fontSize, Fade(WHITE, opacity));
            if (cp == 0x1F150 || cp == 0x1F151 || zrEnd) {   /* A: remap, B: back, ZR: reset; the ring fills while held */
                float prog = PadUi_RingProgress(zrEnd ? 2 : cp == 0x1F150 ? 1 : 0);
                float r = zrEnd ? fmaxf(Hint_GlyphAdvance(font, 0x1F151, fontSize) * 0.62f, zadv * 0.5f + th) : adv * 0.62f;
                if (prog > 0.001f) {
                    Vector2 c = { zrEnd ? zx + zadv * 0.5f : x + adv * 0.5f, textPosition.y + textSize.y * 0.5f };
                    DrawRing(c, r - th * 0.5f, r + th * 0.5f, -90.0f, -90.0f + 360.0f * prog, 48, Fade(WHITE, opacity));
                }
                if (*q == ':') x += zrEnd ? glyphGapZ : glyphGap;
            }
            x += adv + spacing;
        }
    } else if (mixedHints) {
        Hint_N64Draw(font, mix, textPosition, fontSize, spacing, textSize.y, Fade(WHITE, opacity));
    } else {
        DrawTextEx(font,
                   draw,
                   textPosition,
                   fontSize,
                   spacing,
                   Fade(WHITE, opacity));
    }

    /* Charging arc around the existing 🅨 while Y is held. Layout unchanged. */
    float holdY = padBar ? 0.0f : SortY_Progress();
    if (holdY > 0.001f) {
        const char *walk = n64Hints ? text : draw;
        float x = textPosition.x;
        for (const char *p = walk; p && *p; ) {
            int sz = 0, cp = GetCodepointNext(p, &sz);
            p += sz;
            int nerd = n64Hints ? Hint_N64NerdCodepoint(cp) : 0;
            Font f = nerd ? hintNerdFont : font;
            int drawcp = nerd ? nerd : cp;
            float adv = Hint_GlyphAdvance(f, drawcp, fontSize);
            if (cp == SORT_Y_HINT_CP) {
                float th = fmaxf(2.0f, fontSize * 0.07f);
                float r = adv * 0.62f;
                Vector2 c = { x + adv * 0.5f, textPosition.y + textSize.y * 0.5f };
                DrawRing(c, r - th * 0.5f, r + th * 0.5f, -90.0f, -90.0f + 360.0f * holdY, 48,
                         Fade(WHITE, opacity));
                break;
            }
            x += adv + spacing;
        }
    }
}

static void InfoBar_Draw(Font font) {
    for (int i = 1; i < INFO_BAR_COUNT; i++) {
        const char *text = InfoBar_Text((InfoBarId)i);
        float op = infoBarOpacity[i];
        DrawInfoBarEx(font, text, op, (1.0f - op) * INFO_BAR_SLIDE_PX);
    }
}

static void UnloadBlurResources(void) {
    if (mosaicRT.id) UnloadRenderTexture(mosaicRT);
    if (blurRT_A.id) UnloadRenderTexture(blurRT_A);
    if (blurRT_B.id) UnloadRenderTexture(blurRT_B);
    if (blurShader.id) UnloadShader(blurShader);
    mosaicRT = (RenderTexture2D){0};
    blurRT_A = (RenderTexture2D){0};
    blurRT_B = (RenderTexture2D){0};
    blurShader = (Shader){0};
    blurDirLoc = -1;
    blurRT_W = 0;
    blurRT_H = 0;
    blurShaderOk = false;
}

static void EnsureBlurResources(int screenW, int screenH) {
    if (screenW < 1) screenW = 1;
    if (screenH < 1) screenH = 1;
    if (mosaicRT.id && blurRT_W == screenW && blurRT_H == screenH) {
        return;
    }

    if (mosaicRT.id) UnloadRenderTexture(mosaicRT);
    if (blurRT_A.id) UnloadRenderTexture(blurRT_A);
    if (blurRT_B.id) UnloadRenderTexture(blurRT_B);

    mosaicRT = LoadRenderTexture(screenW, screenH);
    int hw = screenW / 2;
    int hh = screenH / 2;
    if (hw < 1) hw = 1;
    if (hh < 1) hh = 1;
    blurRT_A = LoadRenderTexture(hw, hh);
    blurRT_B = LoadRenderTexture(hw, hh);
    SetTextureFilter(mosaicRT.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blurRT_A.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(blurRT_B.texture, TEXTURE_FILTER_BILINEAR);
    blurRT_W = screenW;
    blurRT_H = screenH;

    if (!blurShader.id) {
        /* Separable 5-tap blur; dir is UV step (texel * axis). */
        const char *blurFs =
            "#version 330\n"
            "in vec2 fragTexCoord;\n"
            "in vec4 fragColor;\n"
            "out vec4 finalColor;\n"
            "uniform sampler2D texture0;\n"
            "uniform vec2 dir;\n"
            "void main() {\n"
            "    vec4 c = vec4(0.0);\n"
            "    c += texture(texture0, fragTexCoord + dir * -2.0) * 0.06136;\n"
            "    c += texture(texture0, fragTexCoord + dir * -1.0) * 0.24477;\n"
            "    c += texture(texture0, fragTexCoord) * 0.38774;\n"
            "    c += texture(texture0, fragTexCoord + dir *  1.0) * 0.24477;\n"
            "    c += texture(texture0, fragTexCoord + dir *  2.0) * 0.06136;\n"
            "    finalColor = c * fragColor;\n"
            "}\n";
        blurShader = LoadShaderFromMemory(NULL, blurFs);
        blurShaderOk = (blurShader.id > 0);
        if (blurShaderOk) {
            blurDirLoc = GetShaderLocation(blurShader, "dir");
        }
    }
}

/* Draw RT texture upright to dest (raylib RTs are flipped on Y). */
static void DrawRT(Texture2D tex, Rectangle dest, Color tint) {
    Rectangle src = { 0.0f, 0.0f, (float)tex.width, -(float)tex.height };
    DrawTexturePro(tex, src, dest, (Vector2){0, 0}, 0.0f, tint);
}

/* strength scales the tap spacing of every pass (1 = the standard frosted look). */
static void DrawBlurredMosaicBackdrop(Color clearColor, float strength) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    EnsureBlurResources(sw, sh);
    if (!mosaicRT.id) return;

    float fw = (float)sw;
    float fh = (float)sh;
    float hw = (float)blurRT_A.texture.width;
    float hh = (float)blurRT_A.texture.height;

    if (blurShaderOk && blurDirLoc >= 0) {
        /* Downsample to half-res */
        BeginTextureMode(blurRT_A);
        ClearBackground(clearColor);
        DrawRT(mosaicRT.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        EndTextureMode();

        /* H blur A -> B */
        BeginTextureMode(blurRT_B);
        ClearBackground(clearColor);
        BeginShaderMode(blurShader);
        {
            float dir[2] = { strength / hw, 0.0f };
            SetShaderValue(blurShader, blurDirLoc, dir, SHADER_UNIFORM_VEC2);
            DrawRT(blurRT_A.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        }
        EndShaderMode();
        EndTextureMode();

        /* V blur B -> A */
        BeginTextureMode(blurRT_A);
        ClearBackground(clearColor);
        BeginShaderMode(blurShader);
        {
            float dir[2] = { 0.0f, strength / hh };
            SetShaderValue(blurShader, blurDirLoc, dir, SHADER_UNIFORM_VEC2);
            DrawRT(blurRT_B.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        }
        EndShaderMode();
        EndTextureMode();

        /* Second pass for stronger frosted look */
        BeginTextureMode(blurRT_B);
        ClearBackground(clearColor);
        BeginShaderMode(blurShader);
        {
            float dir[2] = { 1.5f * strength / hw, 0.0f };
            SetShaderValue(blurShader, blurDirLoc, dir, SHADER_UNIFORM_VEC2);
            DrawRT(blurRT_A.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        }
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(blurRT_A);
        ClearBackground(clearColor);
        BeginShaderMode(blurShader);
        {
            float dir[2] = { 0.0f, 1.5f * strength / hh };
            SetShaderValue(blurShader, blurDirLoc, dir, SHADER_UNIFORM_VEC2);
            DrawRT(blurRT_B.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        }
        EndShaderMode();
        EndTextureMode();

        DrawRT(blurRT_A.texture, (Rectangle){0, 0, fw, fh}, WHITE);
    } else {
        /* Fallback: soft downsample + upscale */
        BeginTextureMode(blurRT_A);
        ClearBackground(clearColor);
        DrawRT(mosaicRT.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        EndTextureMode();
        DrawRT(blurRT_A.texture, (Rectangle){0, 0, fw, fh}, WHITE);
    }
}

/* Progress 0 at rail (or narrower), 1 at full open. Overshoot clamps to 1. */
static float SidebarExpandProgress(void) {
    float rail = SidebarRailWidth();
    float open = SidebarOpenWidth();
    float span = open - rail;
    if (span <= 1.0f) return 0.0f;
    float w = sidebarWidth;
    if (w < rail) w = rail;
    if (w > open) w = open;
    float t = (w - rail) / span;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t;
}

static void UpdateGearSpin(void) {
    gearSpinAngle = SidebarExpandProgress() * 360.0f;
}

static bool SidebarMenuSettled(void)
{
    return sidebarOpen && !sidebarClosing && sbWidthPhase == SB_WIDTH_IDLE;
}

static int SortPanel_DatesMissing(void);
static void SortPanel_DatesScrape(const char *yearText, const char *monthText);

static void ScrapeMenu_ClearNames(void)
{
    Scrape_FreeNames(scrapeNames, scrapeNameCount);
    scrapeNames = NULL;
    scrapeNameCount = 0;
}

static void ScrapeMenu_Idle(int status)
{
    Boxart_MissReset();
    if (scrapePart[0] != '\0') FileRemove(scrapePart);
    if (scrapePart2[0] != '\0') FileRemove(scrapePart2);
    scrapePart2[0] = '\0';
    ScrapeMenu_ClearNames();
    scrapeProc = NsoProcNone();
    scrapePhase = SCRAPE_IDLE;
    scrapeBusy = false;
    scrapeCancel = false;
    scrapePart[0] = '\0';
    scrapeAliasHops = 0;
    if (status >= 0) Msg_Set(&scrapeMsg, status, 0, NULL);
}

static int ScrapeMenu_SpawnCurl(const char *url, const char *outPath)
{
    char *argv[13];
    snprintf(scrapePart, sizeof(scrapePart), "%s", outPath);
    argv[0] = "curl";
    argv[1] = "-fsSL";
    argv[2] = "--max-time";
    argv[3] = "120";   /* NES index alone is ~16 MB */
    argv[4] = "--retry";   /* transient 5xx / timeouts: two spaced retries, no hammering */
    argv[5] = "2";
    argv[6] = "--retry-delay";
    argv[7] = "3";
    argv[8] = "--create-dirs";   /* fresh clone: assets/images/systems/<id>/ is absent */
    argv[9] = "-o";
    argv[10] = scrapePart;
    argv[11] = (char *)url;
    argv[12] = NULL;
    scrapeProc = NsoSpawnArgv(NULL, argv);
    return NsoProcOk(scrapeProc);
}

/* A download only replaces a cover once it fully decodes. */
static int ScrapeMenu_Decodes(const char *path)
{
    Image img = LoadBoxartImage(path);
    if (!img.data) return 0;
    UnloadImage(img);
    return 1;
}

static void ScrapeMenu_Finish(void)
{
    Boxart_MissReset();
    if (scrapeMisses > 0) {
        Msg_Set(&scrapeMsg, S_SCRAPE_DONE_MISSED, scrapeMisses, NULL);
    } else {
        Msg_Set(&scrapeMsg, S_SCRAPE_DONE, 0, NULL);
    }
    ScrapeMenu_ClearNames();
    scrapeProc = NsoProcNone();
    scrapePhase = SCRAPE_IDLE;
    scrapeBusy = false;
    scrapeAutoPick = false;
    scrapePart[0] = '\0';
}

/* Release year + month of the shown system's games that have no date yet,
 * from libretro-database's metadat (one curl, two files). False = nothing to do. */
static bool ScrapeMenu_StartDates(void)
{
    const char *playlist = Scrape_Playlist(scrapeId);
    char urlYear[1024], urlMonth[1024], cache[2048];
    char *argv[16];
    if (!playlist || SortPanel_DatesMissing() == 0) return false;
    if (Scrape_DatUrl(urlYear, sizeof(urlYear), "releaseyear", playlist) != 0
        || Scrape_DatUrl(urlMonth, sizeof(urlMonth), "releasemonth", playlist) != 0) return false;
    snprintf(cache, sizeof(cache), "%s", GetDataPath("assets/cache"));
    MakeDirectory(cache);
    snprintf(scrapePart, sizeof(scrapePart), "%s/dates-year.dat.part", cache);
    snprintf(scrapePart2, sizeof(scrapePart2), "%s/dates-month.dat.part", cache);
    argv[0] = "curl";
    argv[1] = "-fsSL";
    argv[2] = "--max-time";
    argv[3] = "60";
    argv[4] = "--retry";
    argv[5] = "2";
    argv[6] = "--retry-delay";
    argv[7] = "3";
    argv[8] = "-o";
    argv[9] = scrapePart;
    argv[10] = urlYear;
    argv[11] = "-o";
    argv[12] = scrapePart2;
    argv[13] = urlMonth;
    argv[14] = NULL;
    scrapeProc = NsoSpawnArgv(NULL, argv);
    if (!NsoProcOk(scrapeProc)) {
        scrapePart[0] = '\0';
        scrapePart2[0] = '\0';
        return false;
    }
    scrapePhase = SCRAPE_DATES;
    Msg_Set(&scrapeMsg, S_SCRAPE_STEP_DATES, 0, NULL);
    return true;
}

static void ScrapeMenu_StartIndex(void)
{
    const char *playlist = Scrape_Playlist(scrapeId);
    char cache[2048];
    char indexPath[2200];
    char url[1024];
    snprintf(cache, sizeof(cache), "%s", GetDataPath("assets/cache"));
    MakeDirectory(cache);
    snprintf(indexPath, sizeof(indexPath), "%s/index.json.part", cache);
    if (!playlist || Scrape_IndexUrl(url, sizeof(url), playlist) != 0) {
        ScrapeMenu_Idle(S_SCRAPE_DOWNLOAD_FAILED);
        return;
    }
    scrapeBusy = true;
    scrapePhase = SCRAPE_INDEX;
    Msg_Set(&scrapeMsg, S_SCRAPE_STEP_INDEX, 0, NULL);
    if (!ScrapeMenu_SpawnCurl(url, indexPath)) {
        scrapeBusy = false;
        scrapePhase = SCRAPE_IDLE;
        Msg_Set(&scrapeMsg, S_SCRAPE_NO_CURL, 0, NULL);
    }
}

static void ScrapeMenu_Advance(void)
{
    int count = scrapeListCount > 0 ? scrapeListCount : ActiveGameTotal();   /* hidden titles get their covers too */
    GameEntry *games = ActiveGames();
    const char *playlist = Scrape_Playlist(scrapeId);

    int work = 0;
    scrapePhase = SCRAPE_SCAN;
    while (scrapeGame < count) {
        char dest[2048];
        char part[2064];
        char url[2048];
        const char *file;
        const char *title;
        const char *rel;
        const char *alias = NULL;
        int present;
        enum { SCRAPE_SCAN_BURST = 8 };

        if (scrapeListCount > 0) {
            title = scrapeListTitle[scrapeGame];
            rel = scrapeListDest[scrapeGame];
            alias = scrapeListAlias[scrapeGame][0] ? scrapeListAlias[scrapeGame] : NULL;
        } else {
            GameEntry *game = &games[scrapeGame];
            title = game->title ? game->title : "";
            rel = game->boxartPath;
            if (!rel || rel[0] == '\0'
                || (scrapeOnlyOn && (scrapeGame >= MAX_GAMES || !scrapeOnly[scrapeGame]))) {
                scrapeGame++;
                continue;
            }
        }

        {
            char prog[160];
            snprintf(prog, sizeof(prog), "%s  %d/%d", title, scrapeGame + 1, count);
            Msg_Set(&scrapeMsg, S_RAW, 0, prog);
        }
        if (!rel || rel[0] == '\0') {
            scrapeGame++;
            continue;
        }
        snprintf(dest, sizeof(dest), "%s", GetDataPath(rel));
        present = FileExists(dest) && Scrape_LooksLikeImage(dest);
        if (!scrapeReplace && present) {
            scrapeGame++;
            if (++work >= SCRAPE_SCAN_BURST) return;
            continue;
        }
        if (scrapeListCount > 0) {
            file = scrapeListFile[scrapeGame][0] ? scrapeListFile[scrapeGame] : NULL;
            if (file && scrapeNameCount > 0) {
                int found = 0, ni;
                for (ni = 0; ni < scrapeNameCount; ni++) {
                    if (scrapeNames[ni] && strcmp(scrapeNames[ni], file) == 0) { found = 1; break; }
                }
                if (!found) file = NULL;
            }
        } else {
            file = NULL;
        }
        if (!file) {
            file = Scrape_MatchLoose(title, (const char **)scrapeNames, scrapeNameCount);
            if (!file && alias)
                file = Scrape_MatchLoose(alias, (const char **)scrapeNames, scrapeNameCount);
        }
        /* Same-game region/revision box arts: pause for a choice when there
         * are two or more. An exact filename from Select games is already a
         * pick, so it skips the picker. */
        if (file && !(scrapeListCount > 0 && scrapeListFile[scrapeGame][0])) {
            const char *cands[SCRAPE_CHOICE_MAX];
            const char *q = title;
            int cn = Scrape_CollectMatches(q, (const char **)scrapeNames,
                                           scrapeNameCount, cands, SCRAPE_CHOICE_MAX);
            if (cn <= 0 && alias) {
                q = alias;
                cn = Scrape_CollectMatches(q, (const char **)scrapeNames,
                                           scrapeNameCount, cands, SCRAPE_CHOICE_MAX);
            }
            if (cn >= 2 && !scrapeAutoPick) {
                int ci;
                scrapeChoiceN = cn;
                scrapeChoiceCur = 0;
                for (ci = 0; ci < cn; ci++) {
                    scrapeChoices[ci] = cands[ci];
                    if (file && cands[ci] && strcmp(cands[ci], file) == 0)
                        scrapeChoiceCur = ci;
                }
                snprintf(scrapeChoiceTitle, sizeof(scrapeChoiceTitle), "%s", title);
                snprintf(scrapeDest, sizeof(scrapeDest), "%s", dest);
                scrapeActive = scrapeGame;
                scrapeAliasHops = 0;
                scrapePhase = SCRAPE_CHOOSE;
                return;
            }
            if (cn >= 1) {
                int ci, keep = 0;
                for (ci = 0; ci < cn; ci++)
                    if (file && cands[ci] && strcmp(cands[ci], file) == 0)
                        keep = 1;
                if (!keep) file = cands[0];
            }
        }
        if (!file || !playlist || Scrape_ImageUrl(url, sizeof(url), playlist, file) != 0) {
            scrapeMisses++;
            Msg_Set(&scrapeMsg, S_SCRAPE_NO_ART, 0, title);
            scrapeGame++;
            if (++work >= SCRAPE_SCAN_BURST) return;
            continue;
        }
        snprintf(part, sizeof(part), "%s.part", dest);
        snprintf(scrapeDest, sizeof(scrapeDest), "%s", dest);
        scrapeActive = scrapeGame;
        scrapeAliasHops = 0;
        scrapePhase = SCRAPE_FILE;
        if (!ScrapeMenu_SpawnCurl(url, part)) {
            scrapeMisses++;
            scrapeGame++;
            continue;
        }
        return;
    }

    ScrapeMenu_Finish();
}

/* On-disk folder for the current system (NES, SNES, ...), same names
 * the games menu uses. GameCube has no Gs_RomFolder, so "GameCube". */
static const char *ScrapeMenu_RomFolder(void)
{
    const char *id, *folder;
    if (!ActiveSystem() || !ActiveSystem()->platform) return NULL;
    id = ActiveSystem()->platform->id;
    folder = Gs_RomFolder(id);
    if (!folder && id && strcmp(id, "gamecube") == 0) folder = "GameCube";
    return folder;
}

static void ScrapeMenu_NoRomsText(char *dst, size_t n)
{
    const char *folder = ScrapeMenu_RomFolder();
    snprintf(dst, n, T(S_EMU_NO_ROMS), folder ? folder : "");
}

/* Catalog indices of titles that have a ROM in roms/<System>/.
 * Uses the same listing as the games menu (loose files and one level of
 * per-game folders). Read only: does not touch scrapeOnly or write files. */
static int ScrapeMenu_ListRoms(int *dst, int max)
{
    enum { SCRAPE_ROM_FILES = 1024 };
    static char baseBuf[SCRAPE_ROM_FILES][256];
    const char *bases[SCRAPE_ROM_FILES];
    const char *titles[MAX_GAMES];
    int matched[SCRAPE_ROM_FILES];
    unsigned char has[MAX_GAMES];
    int total = ActiveGameTotal();
    int n = 0, i, cap, nfiles = 0;
    const char *folder;
    char dir[2400];
    GameEntry *games = ActiveGames();
    VersionScanItem *items;
    int nscan, j;

    memset(has, 0, sizeof has);
    cap = total < MAX_GAMES ? total : MAX_GAMES;
    for (i = 0; i < cap; i++) {
        titles[i] = games[i].title ? games[i].title : "";
        if (games[i].rom && games[i].rom[0]) {
            char path[2400];
            snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), games[i].rom);
            if (FileExists(path) && !DirectoryExists(path)) has[i] = 1;
            else if (strncmp(games[i].rom, "repos/roms/", 11) == 0) {
                snprintf(path, sizeof(path), "%s/%s/%s", GetBaseDirectory(),
                         Gs_RomRoot(), games[i].rom + 11);
                if (FileExists(path) && !DirectoryExists(path)) has[i] = 1;
            }
        }
    }
    folder = ScrapeMenu_RomFolder();
    if (folder) {
        snprintf(dir, sizeof(dir), "%s/%s/%s", GetBaseDirectory(), Gs_RomRoot(), folder);
        items = (VersionScanItem *)calloc((size_t)MAX_GAMES, sizeof(*items));
        if (items) {
            nscan = Version_ScanDir(dir, folder, items, MAX_GAMES);
            for (j = 0; j < nscan; j++) {
                const char *rel, *name;
                if (items[j].isPatch) continue;
                rel = items[j].rel;
                name = items[j].name;
                if (nfiles < SCRAPE_ROM_FILES) {
                    Gs_RomBase(name, baseBuf[nfiles], sizeof(baseBuf[0]));
                    bases[nfiles] = baseBuf[nfiles];
                    nfiles++;
                }
                for (i = 0; i < cap; i++) {
                    const char *bn;
                    if (has[i]) continue;
                    if (games[i].rom && games[i].rom[0] && strcmp(games[i].rom, rel) == 0) {
                        has[i] = 1;
                        continue;
                    }
                    if (!games[i].rom || !games[i].rom[0] || !name || !name[0]) continue;
                    bn = GetFileName(games[i].rom);
                    if (bn && strcmp(bn, name) == 0) has[i] = 1;
                }
            }
            if (nfiles > 0) {
                Gs_MatchAll(bases, nfiles, titles, cap, matched);
                for (i = 0; i < nfiles; i++) {
                    int ti = matched[i];
                    if (ti >= 0 && ti < cap) has[ti] = 1;
                }
            }
            free(items);
        }
    }
    for (i = 0; i < cap; i++) {
        if (!has[i]) continue;
        if (dst && n < max) dst[n] = i;
        n++;
    }
    if (dst && n > max) n = max;
    return n;
}

static int ScrapeMenu_MarkRoms(void)
{
    int idx[MAX_GAMES], n, i;
    memset(scrapeOnly, 0, sizeof scrapeOnly);
    n = ScrapeMenu_ListRoms(idx, MAX_GAMES);
    for (i = 0; i < n; i++) scrapeOnly[idx[i]] = true;
    return n;
}

static void ScrapeMenu_Start(int replace);

static void ScrapeMenu_StartFromRoms(int replace)
{
    scrapeRomOnly = true;
    scrapeAutoPick = true;
    ScrapeMenu_Start(replace);
}

static int ScrapeMenu_Choosing(void)
{
    return scrapeBusy && scrapePhase == SCRAPE_CHOOSE;
}

static const char *ScrapeMenu_ChoiceTitle(void)
{
    return scrapeChoiceTitle;
}

static int ScrapeMenu_ChoiceCount(void)
{
    return scrapeChoiceN;
}

static int ScrapeMenu_ChoiceCur(void)
{
    return scrapeChoiceCur;
}


static int ScrapeMenu_ChoiceGame(void)
{
    return scrapeActive >= 0 ? scrapeActive : scrapeGame;
}

static const char *ScrapeMenu_ChoiceName(int i)
{
    if (i < 0 || i >= scrapeChoiceN || !scrapeChoices[i]) return "";
    return scrapeChoices[i];
}

static void ScrapeMenu_StartChosenFile(const char *file)
{
    const char *playlist = Scrape_Playlist(scrapeId);
    char url[2048], part[2304];
    ScrapeMenu_ChoicePrevStop();
    if (!file || !playlist || Scrape_ImageUrl(url, sizeof(url), playlist, file) != 0) {
        scrapeMisses++;
        Msg_Set(&scrapeMsg, S_SCRAPE_NO_ART, 0, scrapeChoiceTitle);
        scrapeGame++;
        scrapeChoiceN = 0;
        scrapePhase = SCRAPE_SCAN;
        return;
    }
    snprintf(part, sizeof(part), "%s.part", scrapeDest);
    scrapeActive = scrapeGame;
    scrapeAliasHops = 0;
    scrapeChoiceN = 0;
    scrapePhase = SCRAPE_FILE;
    if (!ScrapeMenu_SpawnCurl(url, part)) {
        scrapeMisses++;
        scrapeGame++;
        scrapePhase = SCRAPE_SCAN;
    }
}

/* 1 if the picker consumed the input. accept/back/prev/next are edge flags. */
static int ScrapeMenu_HandleChoice(int accept, int back, int prev, int next)
{
    if (!ScrapeMenu_Choosing()) return 0;
    if (prev && scrapeChoiceN > 0)
        scrapeChoiceCur = (scrapeChoiceCur + scrapeChoiceN - 1) % scrapeChoiceN;
    if (next && scrapeChoiceN > 0)
        scrapeChoiceCur = (scrapeChoiceCur + 1) % scrapeChoiceN;
    if (accept && scrapeChoiceN > 0 && scrapeChoices[scrapeChoiceCur]) {
        ScrapeMenu_StartChosenFile(scrapeChoices[scrapeChoiceCur]);
        return 1;
    }
    if (back) {
        scrapeMisses++;
        scrapeGame++;
        scrapeChoiceN = 0;
        scrapePhase = SCRAPE_SCAN;
        return 1;
    }
    return 1;
}

static void ScrapeMenu_Start(int replace)
{
    const char *playlist;
    int count;
    int need = 0;

    if (scrapeBusy) return;
    if (scrapeRomOnly) {
        scrapeRomOnly = false;
        scrapeListCount = 0;
        if (ScrapeMenu_MarkRoms() == 0) {
            scrapeOnlyOn = false;
            Msg_Set(&scrapeMsg, S_EMU_NO_ROMS, 0, ScrapeMenu_RomFolder() ? ScrapeMenu_RomFolder() : "");
            return;
        }
        scrapeOnlyOn = true;
    }
    playlist = Scrape_Playlist(ActiveSystem()->platform->id);
    if (!playlist) {
        Msg_Set(&scrapeMsg, S_SCRAPE_NO_SOURCE, 0, NULL);
        return;
    }
    count = scrapeListCount > 0 ? scrapeListCount : ActiveGameTotal();
    if (!replace) {
        for (int i = 0; i < count; i++) {
            const char *rel = scrapeListCount > 0 ? scrapeListDest[i] : ActiveGames()[i].boxartPath;
            char path[2304];
            if (!rel || rel[0] == '\0') continue;
            if (scrapeListCount == 0 && scrapeOnlyOn && (i >= MAX_GAMES || !scrapeOnly[i])) continue;
            if (strlen(rel) >= 1024) continue;
            snprintf(path, sizeof(path), "%s/%s", GetBaseDirectory(), rel);
            if (!FileExists(path) || !Scrape_LooksLikeImage(path)) need = 1;
        }
    }
    snprintf(scrapeId, sizeof(scrapeId), "%s", ActiveSystem()->platform->id);
    scrapeReplace = replace;
    scrapeMisses = 0;
    scrapeCancel = false;
    scrapeBusy = true;
    scrapeIndexNext = replace || need;   /* dates first (own files), then the covers */
    if (ScrapeMenu_StartDates()) return;
    if (scrapeIndexNext) {
        ScrapeMenu_StartIndex();
    } else {
        scrapeBusy = false;
        Msg_Set(&scrapeMsg, S_SCRAPE_DONE, 0, NULL);
    }
}

static void ScrapeMenu_Cancel(void)
{
    if (!scrapeBusy) return;
    scrapeCancel = true;
    if (NsoProcOk(scrapeProc)) NsoSpawnKill(scrapeProc);
    ScrapeMenu_ChoicePrevStop();
    Msg_Set(&scrapeMsg, S_SCRAPE_CANCELLED, 0, NULL);
}

static void ScrapeMenu_Poll(void)
{
    int code = 1;
    if (!scrapeBusy || scrapePhase == SCRAPE_IDLE) return;
    if (scrapeCancel) {
        if (NsoProcOk(scrapeProc) && !NsoSpawnExited(&scrapeProc, NULL)) return;
        ScrapeMenu_Idle(S_SCRAPE_CANCELLED);
        return;
    }
    if (scrapePhase == SCRAPE_SCAN) {
        ScrapeMenu_Advance();
        return;
    }
    if (scrapePhase == SCRAPE_CHOOSE) {
        ScrapeMenu_ChoicePrevPoll();
        return;
    }
    if (!NsoProcOk(scrapeProc) || !NsoSpawnExited(&scrapeProc, &code)) return;

    if (scrapePhase == SCRAPE_INDEX) {
        int size = 0;
        unsigned char *data;
        char *text;
        int parsed;
        if (code != 0) {
            ScrapeMenu_Idle(code == 127 ? S_SCRAPE_NO_CURL : S_SCRAPE_DOWNLOAD_FAILED);
            return;
        }
        data = LoadFileData(scrapePart, &size);
        FileRemove(scrapePart);
        scrapePart[0] = '\0';
        if (!data || size <= 0) {
            if (data) UnloadFileData(data);
            ScrapeMenu_Idle(S_SCRAPE_DOWNLOAD_FAILED);
            return;
        }
        text = malloc((size_t)size + 1);
        if (!text) {
            UnloadFileData(data);
            ScrapeMenu_Idle(S_SCRAPE_DOWNLOAD_FAILED);
            return;
        }
        memcpy(text, data, (size_t)size);
        text[size] = '\0';
        UnloadFileData(data);
        parsed = Scrape_ParseIndex(text, &scrapeNames, &scrapeNameCount);
        free(text);
        if (parsed != 0) {
            ScrapeMenu_Idle(S_SCRAPE_INDEX_TOO_LARGE);
            return;
        }
        if (scrapeNameCount == 0) {
            ScrapeMenu_Idle(S_SCRAPE_NO_INDEX);
            return;
        }
        scrapeGame = 0;
        ScrapeMenu_Advance();
        return;
    }

    if (scrapePhase == SCRAPE_FILE) {
        int ok = code == 0 && FileExists(scrapePart) && GetFileLength(scrapePart) > 0;
        if (ok && scrapeAliasHops < 2) {
            char alias[256];
            if (Scrape_AliasTarget(scrapePart, alias, sizeof(alias))) {
                char url[2048];
                char part[2200];
                const char *playlist = Scrape_Playlist(scrapeId);
                snprintf(part, sizeof(part), "%s", scrapePart);
                FileRemove(scrapePart);
                scrapePart[0] = '\0';
                scrapeAliasHops++;
                if (playlist && Scrape_ImageUrl(url, sizeof(url), playlist, alias) == 0
                    && ScrapeMenu_SpawnCurl(url, part))
                    return;
                scrapeMisses++;
                scrapeAliasHops = 0;
                scrapeGame++;
                ScrapeMenu_Advance();
                return;
            }
        }
        if (ok && Scrape_LooksLikeImage(scrapePart) && ScrapeMenu_Decodes(scrapePart)) {
            FileRemove(scrapeDest);
            if (FileMove(scrapePart, scrapeDest) != 0) {
                FileRemove(scrapePart);
                scrapeMisses++;
            } else {
                Boxart_MissReset();
                if (scrapeActive >= 0) {
                    UnloadBoxart(scrapeActive);
                    GetBoxart(scrapeActive);
                }
            }
        } else {
            FileRemove(scrapePart);
            scrapeMisses++;
            Msg_Set(&scrapeMsg, S_SCRAPE_DOWNLOAD_FAILED, 0, NULL);
        }
        scrapePart[0] = '\0';
        scrapeAliasHops = 0;
        scrapeGame++;
        ScrapeMenu_Advance();
        return;
    }

    if (scrapePhase == SCRAPE_DATES) {
        char *year = code == 0 ? LoadFileText(scrapePart) : NULL;
        char *month = code == 0 ? LoadFileText(scrapePart2) : NULL;
        if (year && month) SortPanel_DatesScrape(year, month);
        if (year) UnloadFileText(year);
        if (month) UnloadFileText(month);
        FileRemove(scrapePart);
        FileRemove(scrapePart2);
        scrapePart[0] = '\0';
        scrapePart2[0] = '\0';
        if (scrapeIndexNext) ScrapeMenu_StartIndex();
        else if (year && month) ScrapeMenu_Finish();
        else ScrapeMenu_Idle(S_SCRAPE_DOWNLOAD_FAILED);
    }
}

/* ---- Repository download (gear screen). One background job at a time:
 * tools/repo_sync.sh checks deps, clones or fast-forwards (only when the
 * remote moved), checks the ROM and builds; it writes logs/repos/<name>.log
 * and a one-line logs/repos/<name>.status. ---- */
#define REPO_MAX NSO_REPO_MAX
static NsoMsg repoMsg[REPO_MAX];
static bool repoHasResult[REPO_MAX];
static int repoQueue[REPO_MAX];
static int repoQueueLen = 0;
static int repoRunning = -1;
static NsoProc repoProc;
static int repoPollTick = 0;
static bool repoBuildOnly[REPO_MAX];   /* queued as Install: build mode, no fetch */
static int repoExitPlus1[REPO_MAX];    /* last repo_sync exit + 1; 0 = none */

static void Repo_Paths(int i, char *log, size_t ln, char *st, size_t sn) {
    snprintf(log, ln, "%s/logs/repos/%s.log", GetBaseDirectory(), nso_repos[i].name);
    snprintf(st, sn, "%s/logs/repos/%s.status", GetBaseDirectory(), nso_repos[i].name);
}

static bool Repo_ReadStatus(int i, char *dst, size_t n) {
    char log[2048], st[2048];
    Repo_Paths(i, log, sizeof(log), st, sizeof(st));
    FILE *f = fopen(st, "r");
    if (!f) return false;
    bool ok = fgets(dst, (int)n, f) != NULL;
    fclose(f);
    if (ok) dst[strcspn(dst, "\r\n")] = '\0';
    return ok && dst[0] != '\0';
}

/* "<key> [detail]" from repo_sync.sh -> translated message. */
static void Repo_SetFromStatus(int i, const char *line) {
    char key[64] = "";
    const char *detail = line + strcspn(line, " ");
    snprintf(key, sizeof(key), "%.*s", (int)strcspn(line, " "), line);
    while (*detail == ' ') detail++;
    int id = Repo_StatusKey(key);
    if (id < 0) Msg_Set(&repoMsg[i], S_RAW, 0, line);
    else Msg_Set(&repoMsg[i], id, 0, detail);
}

/* Disk-only state (no network): a few stat() calls. */
static RepoLocalState Repo_DiskState(int i, bool *binOut) {
    const RepoEntry *r = &nso_repos[i];
    char dir[2048], path[2304];
    snprintf(dir, sizeof(dir), "%s/%s", GetBaseDirectory(), r->dir);
    bool exists = DirectoryExists(dir);
    snprintf(path, sizeof(path), "%s/.git", dir);
    bool git = DirectoryExists(path);
    snprintf(path, sizeof(path), "%s/%s", dir, r->binary);
    bool bin = FileExists(path);
    bool rom = false;
    if (r->rom) {
        snprintf(path, sizeof(path), "%s/%s", dir, r->rom);
        rom = FileExists(path);
    }
    if (binOut) *binOut = bin;
    return Repo_LocalState(exists, git, bin, r->rom != NULL, rom);
}

static bool Repo_Busy(int i) {
    if (i == repoRunning) return true;
    for (int q = 0; q < repoQueueLen; q++) if (repoQueue[q] == i) return true;
    return false;
}

/* Install watch state (Repo_Watch below). */
static RepoLocalState repoDisk[REPO_MAX];
static bool repoDiskKnown[REPO_MAX];

/* One press can build it now (badge, notice, Install button). */
static bool Repo_Offer(int i) {
    if (i < 0 || i >= nso_repo_count || i >= REPO_MAX || !repoDiskKnown[i] || Repo_Busy(i)) return false;
    return Repo_CanInstall(repoDisk[i], repoExitPlus1[i] - 1);
}

/* Disk-only state for repos without a job result yet. */
static void Repo_RefreshLocal(void) {
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) {
        if (repoHasResult[i] || Repo_Busy(i)) continue;
        bool bin = false;
        RepoLocalState ls = Repo_DiskState(i, &bin);
        int id = (ls == REPO_LOCAL_COPY && bin) ? S_REPO_LOCAL_READY : Repo_LocalStr(ls);
        if (repoDiskKnown[i] && repoDisk[i] == ls && Repo_Offer(i)) id = S_REPO_INSTALLABLE;
        Msg_Set(&repoMsg[i], id, 0, NULL);
    }
}

static void Repo_StartNext(void) {
    static char a[10][2304];
    char *argv[12];
    if (repoRunning >= 0 || repoQueueLen == 0) return;
    int i = repoQueue[0];
    memmove(repoQueue, repoQueue + 1, sizeof(int) * (size_t)(--repoQueueLen));
    const RepoEntry *r = &nso_repos[i];
    snprintf(a[0], sizeof(a[0]), "%s/tools/repo_sync.sh", GetBaseDirectory());
    Repo_Paths(i, a[1], sizeof(a[1]), a[2], sizeof(a[2]));
    snprintf(a[3], sizeof(a[3]), "%s/%s", GetBaseDirectory(), r->dir);
    snprintf(a[4], sizeof(a[4]), "%s", r->url);
    snprintf(a[5], sizeof(a[5]), "%s", r->binary);
    snprintf(a[6], sizeof(a[6]), "%s", r->rom ? r->rom : "-");
    snprintf(a[7], sizeof(a[7]), "%s", r->deps ? r->deps : "-");
    snprintf(a[8], sizeof(a[8]), "%s", r->build);
    snprintf(a[9], sizeof(a[9]), "%s", repoBuildOnly[i] ? "build" : "sync");
    argv[0] = "sh";
    for (int k = 0; k < 10; k++) argv[k + 1] = a[k];
    argv[11] = NULL;
    FileRemove(a[2]);
    repoProc = NsoSpawnArgv(NULL, argv);
    repoHasResult[i] = true;
    if (!NsoProcOk(repoProc)) {
        Msg_Set(&repoMsg[i], S_REPO_START_FAILED, 0, NULL);
        return;
    }
    repoRunning = i;
    Msg_Set(&repoMsg[i], S_REPO_CHECKING, 0, NULL);
}

/* buildOnly = Install: build the checkout on disk, never clone or fetch. */
static void Repo_Enqueue(int i, bool buildOnly) {
    if (i < 0 || i >= nso_repo_count || i >= REPO_MAX || Repo_Busy(i)) return;
    repoBuildOnly[i] = buildOnly;
    repoQueue[repoQueueLen++] = i;
    repoHasResult[i] = true;
    Msg_Set(&repoMsg[i], S_REPO_QUEUED, 0, NULL);
    Repo_StartNext();
}

static bool repoWatchNow = true;   /* re-check disk state on the next frame */

static void Repo_Poll(void) {
    int code = 1;
    if (repoRunning < 0) return;
    if (!NsoSpawnExited(&repoProc, &code)) {
        char line[160];
        if (++repoPollTick % 15 == 0 && Repo_ReadStatus(repoRunning, line, sizeof(line))) {
            Repo_SetFromStatus(repoRunning, line);
        }
        return;
    }
    char line[160];
    if (Repo_ReadStatus(repoRunning, line, sizeof(line))) {
        Repo_SetFromStatus(repoRunning, line);
    } else {
        Msg_Set(&repoMsg[repoRunning], Repo_ExitStr(code), 0, NULL);
    }
    repoExitPlus1[repoRunning] = code + 1;
    repoRunning = -1;
    repoWatchNow = true;
    Repo_StartNext();
}

/* ---- Install watch. Disk-only (no network), every REPO_WATCH_SEC, when the
 * window regains focus and right after a repo_sync job exits. A transition
 * into "can build now" (downloaded, ROM present if needed, not built) shows
 * a notice; A on it, the repo row or Details > Install runs repo_sync.sh in
 * build mode. ROMs are never fetched: ROM needed only says where to put it. */
#define REPO_WATCH_SEC 3.0
static struct { int repo; RepoNote note; double until; } repoNotice = { -1, REPO_NOTE_NONE, 0.0 };

/* Grid title of the game this repo builds (same system + dir), else its name. */
static const char *Repo_Title(int i) {
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        if (strcmp(systems[s].platform->id, nso_repos[i].system) != 0) continue;
        for (int g = 0; g < *systems[s].gameCount; g++) {
            const GameEntry *e = &systems[s].games[g];
            if (e->dir && e->title && strcmp(e->dir, nso_repos[i].dir) == 0) return e->title;
        }
    }
    return nso_repos[i].name;
}

static void Repo_Watch(void) {
    static double next = 0.0;
    static bool wasFocused = true;
    bool focused = IsWindowFocused();
    double now = GetTime();
    if (focused && !wasFocused) repoWatchNow = true;
    wasFocused = focused;
    if (!repoWatchNow && now < next) return;
    repoWatchNow = false;
    next = now + REPO_WATCH_SEC;
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) {
        if (Repo_Busy(i)) continue;   /* mid-job: judge it when it exits */
        if (!repoDiskKnown[i] && repoExitPlus1[i] == 0) {
            char line[160];   /* earlier run's result (logs/repos/<name>.status) */
            if (Repo_ReadStatus(i, line, sizeof(line))) repoExitPlus1[i] = Repo_StatusExit(line) + 1;
        }
        RepoLocalState st = Repo_DiskState(i, NULL);
        RepoNote note = Repo_Transition(repoDiskKnown[i], repoDisk[i], st);
        bool changed = repoDiskKnown[i] && repoDisk[i] != st;
        repoDisk[i] = st;
        repoDiskKnown[i] = true;
        /* Disk moved on from the last job's message (ROM dropped in, binary
         * deleted...): show the disk state. ROM needed / installed keep the
         * job's more detailed line. */
        if (changed && (note == REPO_NOTE_NONE || note == REPO_NOTE_INSTALLABLE)) repoHasResult[i] = false;
        if (note == REPO_NOTE_INSTALLABLE) repoExitPlus1[i] = 0;   /* new state: offer again */
        if (note == REPO_NOTE_NONE) continue;
        repoNotice.repo = i;
        repoNotice.note = note;
        repoNotice.until = now + (note == REPO_NOTE_INSTALLABLE ? 10.0 : 7.0);
    }
    Repo_RefreshLocal();
}

/* The notice takes A only while it offers an install that is still valid. */
static bool Repo_NoticeActionable(void) {
    return repoNotice.repo >= 0 && repoNotice.note == REPO_NOTE_INSTALLABLE
        && GetTime() < repoNotice.until && Repo_Offer(repoNotice.repo);
}

/* Top-centre toast, same look as the info bar. */
static void Repo_NoticeDraw(Font font) {
    int i = repoNotice.repo;
    double left = repoNotice.until - GetTime();
    if (i < 0 || left <= 0.0) return;
    if (repoNotice.note == REPO_NOTE_INSTALLABLE && !Repo_Offer(i)) return;
    char text[512];
    const RepoEntry *r = &nso_repos[i];
    if (repoNotice.note == REPO_NOTE_ROM_NEEDED) {
        snprintf(text, sizeof(text), T(S_NOTE_ROM), Repo_Title(i), r->rom ? r->rom : "-", r->dir);
    } else {
        snprintf(text, sizeof(text), T(repoNotice.note == REPO_NOTE_INSTALLABLE ? S_NOTE_INSTALL
                                                                              : S_NOTE_INSTALLED),
                 Repo_Title(i));
    }
    float op = (float)fmin(1.0, fmin(left / 0.4, (GetTime() - (repoNotice.until
                 - (repoNotice.note == REPO_NOTE_INSTALLABLE ? 10.0 : 7.0))) / 0.25));
    if (op <= 0.0f) return;
    float fs = (float)font.baseSize;
    char hintBuf[512];
    const char *draw = Nso_HintTextForProfile(Pad_ActiveProfile(), text, hintBuf, sizeof hintBuf);
    Vector2 m = MeasureTextEx(font, draw, fs, 2.0f);
    float w = m.x + 48.0f, h = 56.0f;   /* clears the gear screen's tabs */
    float x = ((float)GetScreenWidth() - w) * 0.5f, y = 14.0f - (1.0f - op) * 20.0f;
    DrawRectangleRounded((Rectangle){ x, y, w, h }, 0.35f, 8, Fade(BLACK, 0.85f * op));
    DrawCircleV((Vector2){ x + 14.0f, y + h * 0.5f }, 5.0f,
                Fade(repoNotice.note == REPO_NOTE_ROM_NEEDED ? ORANGE : (Color){ 0, 190, 255, 255 }, op));
    DrawTextEx(font, draw, (Vector2){ x + 24.0f, y + (h - m.y) * 0.5f }, fs, 2.0f, Fade(WHITE, op));
}

/* N64-connect pop-up: #e5ebed panel, 1/3 x 1/4 of the live window, slides
 * down from the top (centered) and slides back up after PAD_NOTICE_SEC.
 * The controller image is optional; a marked placeholder is used if missing. */
static void PadNotice_Draw(Font font) {
    if (GetTime() >= padNoticeUntil) return;

    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float boxW = sw / 3.0f, boxH = sh / 4.0f;
    float slide = INFO_BAR_FADE_SEC > 0.0f ? INFO_BAR_FADE_SEC : 0.4f;
    float down = (float)(GetTime() - padNoticeStart);
    float up = (float)(padNoticeUntil - GetTime());
    float t = 1.0f;
    if (down < slide) t = down / slide;
    else if (up < slide) t = up / slide;
    if (t <= 0.0f) return;
    if (t > 1.0f) t = 1.0f;

    float x = (sw - boxW) * 0.5f;
    float y = -boxH * (1.0f - t);
    Color bg = { 0xe5, 0xeb, 0xed, 255 };
    DrawRectangleRec((Rectangle){ x, y, boxW, boxH }, bg);

    float fs = (float)font.baseSize;
    const char *label = T(S_PAD_N64_CONNECTED);
    Vector2 tm = MeasureTextEx(font, label, fs, 2.0f);
    if (tm.x > boxW * 0.9f && tm.x > 1.0f) {
        fs *= boxW * 0.9f / tm.x;
        tm = MeasureTextEx(font, label, fs, 2.0f);
    }
    Color ink = { 40, 44, 46, 255 };

    NsoPadLink link = Pad_LinkNow();
    bool usb = (link == NSO_PAD_LINK_USB);
    int player = Nso_PadPlayerFromIndex(Pad_N64Index());
    int battery = usb ? -1 : Nso_PadBatteryRead(NULL, PAD_NOTICE_MAC);

    float pad = boxH * 0.06f, gap = 6.0f;
    float topH = usb ? 0.0f : boxH * 0.11f;
    float botH = boxH * 0.13f;
    float srcW = padNoticeTex.id ? (float)padNoticeTex.width : (float)PAD_NOTICE_CROP_W;
    float srcH = padNoticeTex.id ? (float)padNoticeTex.height : (float)PAD_NOTICE_CROP_H;
    float areaY = y + pad + topH + (topH > 0.0f ? gap : 0.0f);
    float areaW = boxW - pad * 2.0f;
    float areaH = (y + boxH - pad - tm.y - gap) - areaY - botH - gap;
    if (areaH < 8.0f) areaH = 8.0f;
    float scale = fminf(areaW / srcW, areaH / srcH);
    float imgW = srcW * scale, imgH = srcH * scale;
    Rectangle dest = {
        x + (boxW - imgW) * 0.5f,
        areaY + (areaH - imgH) * 0.5f,
        imgW, imgH
    };
    if (!usb && topH > 0.0f) {
        float ledsW = fminf(areaW * 0.55f, boxW * 0.42f);
        PadNotice_DrawLeds((Rectangle){ x + (boxW - ledsW) * 0.5f, y + pad, ledsW, topH }, player);
    }
    if (padNoticeTex.id) {
        DrawTexturePro(padNoticeTex,
                       (Rectangle){ 0, 0, srcW, srcH },
                       dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        DrawRectangleRec(dest, (Color){ 210, 216, 218, 255 });
        DrawRectangleLinesEx(dest, 2.0f, (Color){ 0, 140, 190, 255 });
        const char *ph = "8BitDo N64";
        Vector2 pm = MeasureTextEx(font, ph, fs, 2.0f);
        DrawTextEx(font, ph,
                   (Vector2){ dest.x + (imgW - pm.x) * 0.5f, dest.y + (imgH - pm.y) * 0.5f },
                   fs, 2.0f, (Color){ 0, 140, 190, 255 });
    }
    {
        float by = dest.y + imgH + gap;
        float bh = (y + boxH - pad - tm.y - gap) - by;
        if (bh < botH * 0.5f) { by = y + boxH - pad - tm.y - gap - botH; bh = botH; }
        Rectangle slot = { x + boxW * 0.35f, by, boxW * 0.30f, bh };
        if (usb) {
            PadNotice_DrawUsb(slot, ink);
        } else {
            PadNotice_DrawBattery(slot, battery); /* -1 = empty outline */
        }
    }
    DrawTextEx(font, label,
               (Vector2){ x + (boxW - tm.x) * 0.5f, y + boxH - pad - tm.y },
               fs, 2.0f, ink);
}

/* Repository entry that builds this game of the active system (same dir), or -1. */
static int Game_Repo(const GameEntry *g) {
    if (!g || !g->dir) return -1;
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) {
        if (strcmp(nso_repos[i].system, ActiveSystem()->platform->id) == 0
            && strcmp(nso_repos[i].dir, g->dir) == 0) return i;
    }
    return -1;
}

/* Small "Install" pill used on game tiles. */
static void Repo_BadgeDraw(Font font, Rectangle tile) {
    float fs = fmaxf(14.0f, fminf(24.0f, tile.height * 0.09f));
    const char *t = T(S_DET_INSTALL);
    Vector2 m = MeasureTextEx(font, t, fs, 1.0f);
    Rectangle b = { tile.x + tile.width - m.x - 22.0f, tile.y + 8.0f, m.x + 14.0f, m.y + 6.0f };
    DrawRectangleRounded(b, 0.5f, 8, (Color){ 0, 150, 220, 235 });
    DrawTextEx(font, t, (Vector2){ b.x + 7.0f, b.y + 3.0f }, fs, 1.0f, WHITE);
}

static void Lang_ConfigPath(char *dst, size_t n) {
    snprintf(dst, n, "%s/config/nso.cfg", GetBaseDirectory());
}

/* ---- In-system sidebar (grid rail + B/O menu), as in the reference video:
 * four line icons (Single player, Play together, Play online, Settings),
 * icon + label rows when open, and a Settings panel over the blurred grid.
 * Icons are raylib primitives (no art). The first three have no function;
 * Two Players and Play online show a short Coming soon toast. ---- */
typedef enum { SB_ITEM_SINGLE = 0, SB_ITEM_TOGETHER, SB_ITEM_ONLINE, SB_ITEM_SETTINGS,
               SB_ITEM_COUNT } SidebarItem;
enum { SB_SET_VER_NA = 0, SB_SET_VER_NA_EU, SB_SET_RUMBLE, SB_SET_CONTROLS, SB_SET_CRTGEOM,
       SB_SET_PAD,
       SB_SET_COUNT };   /* PAD: the controller screen (NES/SNES/N64/GameCube);
                          * CRTGEOM: N64/GameCube only (NES/SNES have it as SB_RETRO_CRTGEOM) */

/* Stored per system in config/nso.cfg ("<id>.versions", "<id>.rumble",
 * "<id>.show_controls"; NES/SNES also independent "<id>.pixel" and
 * "<id>.scanlines", each "0" or "1", default both on). A one-time
 * "<id>.screen_ver" = "2" stamp forces both on and drops leftover
 * "<id>.screen"; after that, the user's choices are kept. N64/GameCube
 * embeds still use .screen as EMBED_SCREEN_* (default Pixel Perfect; not
 * shown in Settings). */
typedef struct {
    int versions;
    bool rumble;
    bool showControls;
    int screen;       /* N64/GC embeds: EMBED_SCREEN_*; unused for NES/SNES */
    bool pixel;       /* NES/SNES: largest integer scale, square pixels */
    bool scanlines;   /* NES/SNES: scanlines at 90% opacity */
    bool crtgeom;     /* "CRT Geometry" (shaders/crtgeom.glsl), off by default */
} SysSettings;
static const char *const SCREEN_MODE_KEYS[3] = { "4:3", "pixel", "crt" };
static SysSettings sysSettings[sizeof(systems) / sizeof(systems[0])];

static bool SysIdIsRetro(const char *id);

static void SysSettings_LoadAll(void) {
    char cfg[2112], text[4096], v[32];
    size_t n = 0;
    bool retroReset = false;
    Lang_ConfigPath(cfg, sizeof(cfg));
    FILE *f = fopen(cfg, "r");
    if (f) { n = fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[n] = '\0';
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        const char *id = systems[i].platform->id;
        char key[64];
        /* defaults: NA, rumble on, controls hidden, Pixel Perfect + Scanlines on */
        SysSettings s = { 0, true, false, 1, true, true, false };
        snprintf(key, sizeof(key), "%s.versions", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.versions = strcmp(v, "na_eu") == 0 ? 1 : 0;
        snprintf(key, sizeof(key), "%s.rumble", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.rumble = strcmp(v, "0") != 0;
        snprintf(key, sizeof(key), "%s.show_controls", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.showControls = strcmp(v, "0") != 0;
        if (SysIdIsRetro(id)) {
            if (Cfg_RetroScreenLoad(text, id, &s.pixel, &s.scanlines)
                && strcmp(id, "megadrive") != 0)
                retroReset = true;
        } else {
            snprintf(key, sizeof(key), "%s.screen", id);
            if (Cfg_Get(text, key, v, sizeof(v)))
                for (int m = 0; m < 3; m++) if (strcmp(v, SCREEN_MODE_KEYS[m]) == 0) s.screen = m;
        }
        snprintf(key, sizeof(key), "%s.crtgeom", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.crtgeom = strcmp(v, "1") == 0;
        sysSettings[i] = s;
    }
    if (retroReset) {
        char a[4200], b[4200];
        bool ok = Cfg_RetroScreenStamp(text, "nes", a, sizeof(a))
               && Cfg_RetroScreenStamp(a, "snes", b, sizeof(b));
        if (ok) {
            FILE *w = fopen(cfg, "w");
            if (w) { ok = fputs(b, w) >= 0; ok = (fclose(w) == 0) && ok; }
            else ok = false;
        }
        if (!ok) fprintf(stderr, "NSO: cannot reset NES/SNES Game Screen in %s\n", cfg);
    }
}

static void SysSettings_Save(int sys) {
    char dir[2048], cfg[2112], key[64];
    const char *id = systems[sys].platform->id;
    const SysSettings *s = &sysSettings[sys];
    bool ok = true;
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    MakeDirectory(dir);
    Lang_ConfigPath(cfg, sizeof(cfg));
    snprintf(key, sizeof(key), "%s.versions", id);
    ok &= Cfg_SaveKey(cfg, key, s->versions ? "na_eu" : "na");
    snprintf(key, sizeof(key), "%s.rumble", id);
    ok &= Cfg_SaveKey(cfg, key, s->rumble ? "1" : "0");
    snprintf(key, sizeof(key), "%s.show_controls", id);
    ok &= Cfg_SaveKey(cfg, key, s->showControls ? "1" : "0");
    if (SysIdIsRetro(id)) {
        snprintf(key, sizeof(key), "%s.pixel", id);
        ok &= Cfg_SaveKey(cfg, key, s->pixel ? "1" : "0");
        snprintf(key, sizeof(key), "%s.scanlines", id);
        ok &= Cfg_SaveKey(cfg, key, s->scanlines ? "1" : "0");
        snprintf(key, sizeof(key), "%s.screen_ver", id);
        ok &= Cfg_SaveKey(cfg, key, NSO_RETRO_SCREEN_VER);
    }
    snprintf(key, sizeof(key), "%s.crtgeom", id);
    ok &= Cfg_SaveKey(cfg, key, s->crtgeom ? "1" : "0");
    if (!ok) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

static bool SysIsNes(int sys) {
    return strcmp(systems[sys].platform->id, "nes") == 0;
}
static bool SysIsSnes(int sys) {
    return strcmp(systems[sys].platform->id, "snes") == 0;
}
static bool SysIsN64(int sys) {
    return strcmp(systems[sys].platform->id, "n64") == 0;
}
static bool SysIsGamecube(int sys) {
    return strcmp(systems[sys].platform->id, "gamecube") == 0;
}
static bool SysIdIsRetro(const char *id) {
    return id && (strcmp(id, "nes") == 0 || strcmp(id, "snes") == 0
                  || strcmp(id, "megadrive") == 0);
}
/* The systems with a controller screen (padui.h): NES, SNES, N64 and GameCube. */
static bool SysHasPad(int sys) {
    return SysIsNes(sys) || SysIsSnes(sys) || SysIsN64(sys) || SysIsGamecube(sys);
}
static StrId PadEntryLabel(int sys) {
    return SysIsGamecube(sys) ? S_PC_ENTRY_GC : SysIsN64(sys) ? S_PC_ENTRY_N64
         : SysIsSnes(sys) ? S_PC_ENTRY_SNES : S_PC_ENTRY;
}
/* 0 NES, 1 SNES, 2 N64, 3 GameCube — the PadUi_Open / PadUi_Select kind. */
static int PadUi_KindOf(int sys) {
    if (SysIsGamecube(sys)) return 3;
    if (SysIsN64(sys)) return 2;
    if (SysIsSnes(sys)) return 1;
    return 0;
}

/* The last Settings row of the current system: the controller entry exists for NES, SNES, N64 and GameCube. */
static int SetRowLast(void) {
    if (SysHasPad(currentSystem)) return SB_SET_PAD;
    return SysIdIsRetro(systems[currentSystem].platform->id) ? SB_SET_CONTROLS : SB_SET_CRTGEOM;
}

static int sidebarItem = SB_ITEM_SINGLE;
static bool sidebarPanelFocus = false;   /* Settings: focus in the right panel */
static int sidebarSetRow = 0;
/* One-shot icon animation per item; selection changes animate only the new
 * item, while opening or confirming can replay the current item. */
static double sbAnimStart[SB_ITEM_COUNT] = { -100.0, -100.0, -100.0, -100.0 };
static int sbAnimDir[SB_ITEM_COUNT];
static const float SB_ANIM_ENTER_SEC = 0.55f;
static float sbGearRestAngle = 0.0f;
static float sbGearFromAngle = 0.0f;

static void SidebarAnim_Play(int item, int dir) {
    sbAnimStart[item] = GetTime();
    sbAnimDir[item] = dir;
    if (item == SB_ITEM_SETTINGS) {
        sbGearFromAngle = sbGearRestAngle;
        sbGearRestAngle += (float)dir * 80.0f;
    }
}

static void Sidebar_Select(int item) {
    if (item == sidebarItem) return;
    sidebarItem = item;
    SidebarAnim_Play(item, +1);
    Sfx_Play(SFX_PLUCK);
}

/* Progress 0..1 of the item's one-shot, or -1 when it is at rest. */
static float SidebarAnim_Progress(int item) {
    float p = (float)(GetTime() - sbAnimStart[item]) / SB_ANIM_ENTER_SEC;
    return (p >= 0.0f && p < 1.0f) ? p : -1.0f;
}

/* Icon fills erase (alpha 0) inside the icon texture so overlapping people
 * hide what is behind them without knowing the background colour. */
static void IconErase(bool on) {
    if (on) {
        rlSetBlendFactors(RL_ONE, RL_ZERO, RL_FUNC_ADD);
        BeginBlendMode(BLEND_CUSTOM);
    } else {
        EndBlendMode();
    }
}

/* Polyline arc / ellipse of thickness th; angles in degrees (0 = right, 90 = down). */
static void IconArc(Vector2 c, float rx, float ry, float a0, float a1, float th, Color col) {
    int seg = 48;
    Vector2 prev = { c.x + rx * cosf(a0 * DEG2RAD), c.y + ry * sinf(a0 * DEG2RAD) };
    DrawCircleV(prev, th * 0.5f, col);
    for (int k = 1; k <= seg; k++) {
        float a = (a0 + (a1 - a0) * (float)k / (float)seg) * DEG2RAD;
        Vector2 p = { c.x + rx * cosf(a), c.y + ry * sinf(a) };
        DrawLineEx(prev, p, th, col);
        DrawCircleV(p, th * 0.5f, col);
        prev = p;
    }
}

/* Head + shoulders outline in a box of height s centred at c; fill hides
 * what is behind (used where people overlap). */
static void IconPerson(Vector2 c, float s, float th, Color col, Color fill, bool solidFill) {
    Vector2 head = { c.x, c.y - s * 0.22f };
    float hr = s * 0.2f;
    Rectangle body = { c.x - s * 0.34f, c.y + s * 0.06f, s * 0.68f, s * 0.42f };
    if (solidFill) {
        IconErase(true);
        DrawRectangleRounded(body, 0.9f, 24, fill);
        DrawCircleV(head, hr + th * 0.5f, fill);
        IconErase(false);
    }
    DrawRectangleRoundedLinesEx(body, 0.9f, 24, th, col);
    IconArc(head, hr, hr, 0.0f, 360.0f, th, col);
}

static float IconBump(float x) {   /* 0..1..0 over x in [0,1], 0 elsewhere */
    return (x <= 0.0f || x >= 1.0f) ? 0.0f : sinf(x * PI);
}

/* Video's settings gear as line art: six flat-topped teeth (one pointing
 * up) with rounded corners, traced as one closed outline, plus a round hole. */
static void IconGear(Vector2 c, float s, float th, float rotDeg, Color col) {
    const int n = 240;
    const float R = s * 0.40f, root = s * 0.315f;     /* tooth tip / valley radius */
    const float aTop = 14.0f, aBase = 26.5f;          /* half-angles: tooth top / flank foot */
    Vector2 prev = { 0 }, first = { 0 };
    for (int k = 0; k <= n; k++) {
        float a = 360.0f * (float)k / (float)n;
        float d = fmodf(a, 60.0f);                    /* angle from the nearest tooth centre */
        if (d > 30.0f) d = 60.0f - d;
        float u = (d <= aTop) ? 1.0f : (d >= aBase) ? 0.0f : 1.0f - (d - aTop) / (aBase - aTop);
        u = u * u * (3.0f - 2.0f * u);                /* smoothstep rounds tip and valley */
        float r = root + (R - root) * u;
        float ang = (a - 90.0f + rotDeg) * DEG2RAD;
        Vector2 pt = { c.x + r * cosf(ang), c.y + r * sinf(ang) };
        if (k == 0) first = pt;
        else DrawLineEx(prev, k == n ? first : pt, th, col);
        DrawCircleV(pt, th * 0.5f, col);
        prev = pt;
    }
    IconArc(c, s * 0.135f, s * 0.135f, 0.0f, 360.0f, th, col);
}

/* p: one-shot progress 0..1 (or < 0 at rest); dir: +1 enter, -1 leave (softer). */
static void DrawSidebarIcon(int item, Vector2 c, float s, Color col, Color fill, float p, int dir) {
    float th = fmaxf(2.0f, s * 0.065f);
    float amp = dir < 0 ? 0.5f : 1.0f;
    bool on = p >= 0.0f;
    switch (item) {
        case SB_ITEM_SINGLE: {   /* one small hop */
            float hop = on ? IconBump(p) * s * 0.10f * amp : 0.0f;
            IconPerson((Vector2){ c.x, c.y - hop }, s * 0.95f, th, col, fill, false);
        } break;
        case SB_ITEM_TOGETHER: {   /* members pop once in turn: left, right, front */
            Vector2 pos[3] = { { c.x - s * 0.2f, c.y - s * 0.12f }, { c.x + s * 0.2f, c.y - s * 0.12f },
                               { c.x, c.y + s * 0.16f } };
            for (int k = 0; k < 3; k++) {
                float sc = on ? 1.0f + 0.2f * amp * IconBump(p * 2.0f - (float)k * 0.5f) : 1.0f;
                IconPerson(pos[k], s * (k == 2 ? 0.62f : 0.55f) * sc, th * 0.85f, col, fill, true);
            }
        } break;
        case SB_ITEM_ONLINE: {   /* globe stays still; person hops like Single */
            Vector2 g = { c.x + s * 0.12f, c.y - s * 0.1f };
            float r = s * 0.36f;
            float ith = th * 0.8f;
            float hop = on ? IconBump(p) * s * 0.10f * amp : 0.0f;
            IconArc(g, r, r, 0.0f, 360.0f, th, col);
            /* 3 latitudes (north, equator, south) as chords */
            DrawLineEx((Vector2){ g.x - r, g.y }, (Vector2){ g.x + r, g.y }, ith, col);
            float lat = 0.38f * r;
            float hx = sqrtf(r * r - lat * lat);
            DrawLineEx((Vector2){ g.x - hx, g.y - lat }, (Vector2){ g.x + hx, g.y - lat }, ith, col);
            DrawLineEx((Vector2){ g.x - hx, g.y + lat }, (Vector2){ g.x + hx, g.y + lat }, ith, col);
            /* 3 meridians, 30° apart; one is the vertical diameter */
            for (int i = -1; i <= 1; i++) {
                float lon = (float)i * (PI / 6.0f);
                float sl = sinf(lon);
                float rx = r * fabsf(sl);
                if (rx < 0.5f)
                    DrawLineEx((Vector2){ g.x, g.y - r }, (Vector2){ g.x, g.y + r }, ith, col);
                else if (rx < r * 0.97f) {
                    if (sl >= 0.0f) IconArc(g, rx, r, -90.0f,  90.0f, ith, col);
                    else            IconArc(g, rx, r,  90.0f, 270.0f, ith, col);
                }
            }
            IconPerson((Vector2){ c.x - s * 0.22f, c.y + s * 0.22f - hop }, s * 0.55f, th * 0.85f, col, fill, true);
        } break;
        default: {   /* gear turns 80 degrees and holds its new position */
            float e = on ? 1.0f - (1.0f - p) * (1.0f - p) : 0.0f;  /* ease out */
            float angle = on ? sbGearFromAngle + (sbGearRestAngle - sbGearFromAngle) * e
                             : sbGearRestAngle;
            IconGear(c, s, th, angle, col);
        } break;
    }
}

/* Icons are drawn SB_ICON_SS x larger into a transparent texture, mipmapped,
 * then drawn down with trilinear filtering: smooth edges on any background
 * without touching the rest of the UI (no global MSAA). */
#define SB_ICON_SS 4
static RenderTexture2D sbIconRT[SB_ITEM_COUNT];
static int sbIconRTSize = 0;
static bool sbIconAtRest[SB_ITEM_COUNT];   /* texture holds the resting icon in sbIconCol */
static Color sbIconCol[SB_ITEM_COUNT];

static void SidebarIcons_Unload(void) {
    for (int i = 0; i < SB_ITEM_COUNT; i++) {
        if (sbIconRT[i].id) UnloadRenderTexture(sbIconRT[i]);
        sbIconRT[i] = (RenderTexture2D){ 0 };
        sbIconAtRest[i] = false;
    }
    sbIconRTSize = 0;
}

/* Renders item's icon (box = logical size in px) and returns its texture. */
static Texture2D SidebarIcon_Render(int item, float box, float iconS, Color col, float p, int dir) {
    int size = (int)ceilf(box) * SB_ICON_SS;
    if (size != sbIconRTSize) {
        SidebarIcons_Unload();
        for (int i = 0; i < SB_ITEM_COUNT; i++) sbIconRT[i] = LoadRenderTexture(size, size);
        sbIconRTSize = size;
    }
    RenderTexture2D *rt = &sbIconRT[item];
    bool rest = p < 0.0f;
    if (rest && sbIconAtRest[item] && ColorIsEqual(sbIconCol[item], col)) return rt->texture;
    sbIconAtRest[item] = rest;   /* re-render only while animating or on a colour change */
    sbIconCol[item] = col;
    Color clear = { col.r, col.g, col.b, 0 };   /* same RGB: no dark fringe when filtered */
    BeginTextureMode(*rt);
    ClearBackground(clear);
    DrawSidebarIcon(item, (Vector2){ size * 0.5f, size * 0.5f }, iconS * SB_ICON_SS, col, clear, p, dir);
    EndTextureMode();
    GenTextureMipmaps(&rt->texture);
    SetTextureFilter(rt->texture, TEXTURE_FILTER_TRILINEAR);
    return rt->texture;
}

static float SidebarRowY(int item) {
    return (float)GetScreenHeight() * (0.590f + 0.085f * (float)item);
}

static void DrawFitText(Font font, const char *text, Vector2 at, float size, float maxW, Color col) {
    Vector2 m = MeasureTextEx(font, text, size, 1.0f);
    if (m.x > maxW && m.x > 0.0f) { size *= maxW / m.x; m = MeasureTextEx(font, text, size, 1.0f); }
    DrawTextEx(font, text, (Vector2){ at.x, at.y - m.y * 0.5f }, size, 1.0f, col);
}

/* ---- NES / SNES Settings panel: "Game Screen" with two independent toggles
 * (Pixel Perfect and Scanlines), a preview of the current combination, then
 * "Control Display". N64 keeps SidebarPanel_Draw as is. ---- */
enum { SB_RETRO_PIXEL = 0, SB_RETRO_SCANLINES = 1, SB_RETRO_CRTGEOM = 2 };
static bool SysIsRetro(int sys) {
    return SysIdIsRetro(systems[sys].platform->id);
}

static EmbedScreen ActiveEmbedScreen(void) {
    const SysSettings *s = &sysSettings[currentSystem];
    if (SysIsRetro(currentSystem))
        return (EmbedScreen){ s->pixel, s->scanlines, false, s->crtgeom };
    return (EmbedScreen){
        s->screen == EMBED_SCREEN_PIXEL,
        s->screen == EMBED_SCREEN_CRT || s->crtgeom,   /* no Scanlines toggle: the CRT look has its beam lines */
        s->screen != EMBED_SCREEN_PIXEL,
        s->crtgeom
    };
}

/* Optional local screenshot (never shipped; assets/images/ is gitignored):
 * assets/images/settings_<id>.png. Loaded once; code-drawn scene otherwise. */
static Texture2D retroShot[2];
static bool retroShotTried[2];

static Texture2D RetroShot(bool snes) {
    int k = snes ? 1 : 0;
    if (!retroShotTried[k]) {
        retroShotTried[k] = true;
        const char *p = GetDataPath(snes ? "assets/images/settings_snes.png" : "assets/images/settings_nes.png");
        if (FileExists(p)) {
            retroShot[k] = LoadTexture(p);
            if (retroShot[k].id) SetTextureFilter(retroShot[k], TEXTURE_FILTER_BILINEAR);
        }
    }
    return retroShot[k];
}

static void RetroShots_Unload(void) {
    for (int k = 0; k < 2; k++) {
        if (retroShot[k].id) UnloadTexture(retroShot[k]);
        retroShot[k] = (Texture2D){ 0 };
        retroShotTried[k] = false;
    }
}

/* Generic platformer scene on a 64 x 48 cell canvas: sky, clouds, blocks,
 * a pipe, bushes, ground and a status bar. NES palette or a richer SNES one. */
static void DrawRetroScene(Rectangle r, bool snes) {
    float u = r.width / 64.0f, v = r.height / 48.0f;
#define RS_CELL(cx, cy, cw, chh, col) DrawRectangleRec((Rectangle){ r.x + (cx) * u, r.y + (cy) * v, (cw) * u, (chh) * v }, col)
    if (snes) {
        DrawRectangleGradientV((int)r.x, (int)r.y, (int)r.width, (int)(r.height * 0.8f),
                               (Color){ 96, 160, 248, 255 }, (Color){ 200, 232, 255, 255 });
        DrawCircleV((Vector2){ r.x + 14 * u, r.y + 37 * v }, 11 * u, (Color){ 72, 176, 96, 255 });
        DrawCircleV((Vector2){ r.x + 44 * u, r.y + 38 * v }, 14 * u, (Color){ 48, 152, 80, 255 });
        DrawCircleV((Vector2){ r.x + 12 * u, r.y + 34 * v }, 3 * u, (Color){ 120, 208, 128, 255 });
    } else {
        RS_CELL(0, 0, 64, 39, ((Color){ 164, 228, 252, 255 }));
    }
    Color cloud = WHITE;
    RS_CELL(6, 5, 7, 3, cloud); RS_CELL(8, 4, 3, 1, cloud);
    RS_CELL(44, 3, 8, 3, cloud); RS_CELL(46, 2, 4, 1, cloud);
    Color blk = snes ? (Color){ 248, 200, 40, 255 } : (Color){ 228, 160, 72, 255 };
    Color blkD = snes ? (Color){ 176, 112, 16, 255 } : (Color){ 136, 80, 24, 255 };
    RS_CELL(18, 12, 8, 4, blkD); RS_CELL(18.5f, 12.5f, 7, 3, blk);
    RS_CELL(7, 22, 8, 4, blkD); RS_CELL(7.5f, 22.5f, 7, 3, blk);
    /* hero */
    Color hat = snes ? (Color){ 232, 40, 40, 255 } : (Color){ 200, 56, 32, 255 };
    RS_CELL(28, 9, 4, 2, hat); RS_CELL(28, 11, 4, 2, ((Color){ 252, 188, 148, 255 }));
    RS_CELL(28, 13, 4, 3, (snes ? (Color){ 40, 72, 216, 255 } : hat));
    /* big blocks */
    RS_CELL(30, 18, 10, 20, ((Color){ 152, 184, 248, 255 }));
    RS_CELL(22, 25, 10, 13, ((Color){ 248, 200, 192, 255 }));
    RS_CELL(40, 19, 1.5f, 19, ((Color){ 24, 24, 24, 255 }));
    /* pipe */
    Color pipe = snes ? (Color){ 24, 200, 72, 255 } : (Color){ 56, 168, 40, 255 };
    Color pipeD = snes ? (Color){ 8, 120, 40, 255 } : (Color){ 24, 104, 16, 255 };
    RS_CELL(48, 24, 10, 3, pipeD); RS_CELL(49, 27, 8, 11, pipe); RS_CELL(50, 27, 1.5f, 11, ((Color){ 160, 240, 160, 255 }));
    /* bushes */
    for (int k = 0; k < 3; k++) {
        DrawCircleV((Vector2){ r.x + (4.5f + k * 4.0f) * u, r.y + 36.5f * v }, 2.0f * u, pipe);
    }
    /* ground + status bar */
    RS_CELL(0, 38, 64, 3, (snes ? (Color){ 216, 160, 88, 255 } : (Color){ 228, 200, 120, 255 }));
    for (int k = 0; k < 16; k++) RS_CELL(k * 4.0f, 38.5f, 2, 1, ((Color){ 96, 64, 24, 255 }));
    RS_CELL(0, 41, 64, 7, ((Color){ 16, 16, 16, 255 }));
    RS_CELL(2, 42, 36, 5, ((Color){ 232, 232, 232, 255 })); RS_CELL(3, 43, 34, 3, ((Color){ 24, 24, 24, 255 }));
    for (int k = 0; k < 3; k++) { RS_CELL(42 + k * 7.0f, 42, 6, 5, ((Color){ 232, 232, 232, 255 }));
                                  RS_CELL(43 + k * 7.0f, 43, 4, 3, ((Color){ 176, 216, 216, 255 })); }
#undef RS_CELL
}

/* Preview of the current Pixel Perfect + Scanlines combination. The picture
 * keeps the game's aspect (dark side bars in a 4:3 frame); scanlines are
 * overlaid when that toggle is on. */
static void DrawRetroThumb(Rectangle r, bool snes, bool scanlines, bool allowShot) {
    float w = r.width * 0.82f;
    DrawRectangleRec(r, (Color){ 66, 68, 68, 255 });
    Rectangle img = { r.x + (r.width - w) * 0.5f, r.y, w, r.height };
    Texture2D shot = allowShot ? RetroShot(snes) : (Texture2D){ 0 };
    if (shot.id) {
        DrawTexturePro(shot, (Rectangle){ 0, 0, (float)shot.width, (float)shot.height }, img,
                       (Vector2){ 0 }, 0.0f, WHITE);
    } else {
        BeginScissorMode((int)img.x, (int)img.y, (int)ceilf(img.width), (int)ceilf(img.height));
        DrawRetroScene(img, snes);
        EndScissorMode();
    }
    if (scanlines) {
        float step = fmaxf(2.0f, img.height / 48.0f);
        for (float y = img.y; y < img.y + img.height; y += step)
            DrawRectangleRec((Rectangle){ img.x, y, img.width, step * 0.45f }, Fade((Color){ 40, 60, 70, 255 }, 0.28f));
    }
}

static void SidebarPanelRetro_Draw(Font font, Color red) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    const SysSettings *st = &sysSettings[currentSystem];
    bool snes = strcmp(systems[currentSystem].platform->id, "snes") == 0;
    float hx = sw * 0.446f, cx = sw * 0.455f, bx = sw * 0.481f, bw = sw * 0.390f;
    float th = fmaxf(2.0f, sh * 0.0028f);
    float hsize = (float)font.baseSize * 0.85f, osize = (float)font.baseSize * 1.2f;
    Color pink = { 238, 139, 144, 255 }, curFill = { 223, 233, 231, 255 };

    DrawFitText(font, T(S_SET_SCREEN), (Vector2){ hx, sh * 0.092f }, hsize, sw - hx, WHITE);
    DrawFitText(font, T(S_SET_CTRL_DISPLAY), (Vector2){ hx, sh * 0.774f }, hsize, sw - hx, WHITE);

    float r = sh * 0.016f;
    /* Same thumbnail size as the pre-ba98df1 Game Screen cards. */
    float ch = sh * 0.155f;
    Rectangle frame = { bx, sh * 0.132f, sw * 0.112f + 2.0f * th, ch };
    DrawRectangleLinesEx(frame, th * 0.6f, WHITE);
    Rectangle thumb = { frame.x + th, frame.y + th, sw * 0.112f, ch - 2.0f * th };
    DrawRetroThumb(thumb, snes, st->scanlines, SysIsNes(currentSystem) || SysIsSnes(currentSystem));

    static const StrId labels[3] = { S_SET_PIXEL, S_SET_SCANLINES, S_SET_CRTGEOM };
    /* Raised to sit under the small thumb without a large gap. */
    static const float rowY[3] = { 0.36f, 0.48f, 0.60f };
    bool on[3] = { st->pixel, st->scanlines, st->crtgeom };
    float bh = sh * 0.069f;
    for (int k = 0; k < 3; k++) {
        float y = sh * rowY[k];
        bool cur = (k == sidebarSetRow);
        Rectangle box = { bx, y - bh * 0.5f, bw * 0.99f, bh };
        if (cur) DrawRectangleRec(box, curFill);
        else DrawRectangleLinesEx(box, th * 0.6f, WHITE);
        const char *lab = T(labels[k]);
        DrawFitText(font, lab, (Vector2){ sw * 0.504f, y }, osize, bw - sw * 0.03f, cur ? pink : WHITE);
        Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
        if (on[k]) {
            DrawRectangleRec(cb, red);
            DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f }, th * 1.3f, WHITE);
            DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f }, th * 1.3f, WHITE);
        } else {
            DrawRectangleLinesEx(cb, th * 0.7f, WHITE);
        }
    }

    /* Control Display: one checkbox row (same box as the N64 panel). */
    float y = sh * 0.844f;
    bool cur = (sidebarSetRow == SB_SET_CONTROLS);
    Rectangle box = { bx, y - bh * 0.5f, bw * 0.99f, bh };
    if (cur) DrawRectangleRec(box, curFill);
    else DrawRectangleLinesEx(box, th * 0.6f, WHITE);
    DrawFitText(font, T(S_SET_CONTROLS), (Vector2){ sw * 0.504f, y }, osize, bw - sw * 0.03f, cur ? pink : WHITE);
    Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
    if (st->showControls) {
        DrawRectangleRec(cb, red);
        DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f }, th * 1.3f, WHITE);
        DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f }, th * 1.3f, WHITE);
    } else {
        DrawRectangleLinesEx(cb, th * 0.7f, WHITE);
    }

    if (SysHasPad(currentSystem)) {
        float phy = sh * 0.900f, py = sh * 0.958f;
        bool pcur = (sidebarSetRow == SB_SET_PAD);
        Rectangle pbox = { bx, py - bh * 0.5f, bw * 0.99f, bh };
        DrawFitText(font, T(S_PC_HEAD), (Vector2){ hx, phy }, hsize, sw - hx, WHITE);
        if (pcur) DrawRectangleRec(pbox, curFill);
        else DrawRectangleLinesEx(pbox, th * 0.6f, WHITE);
        DrawFitText(font, T(PadEntryLabel(currentSystem)), (Vector2){ sw * 0.504f, py }, osize,
                    bw - sw * 0.03f, pcur ? pink : WHITE);
    }
}

/* Settings panel (right of the open menu, over the blurred grid). */
static void SidebarPanel_Draw(Font font, Color red) {
    if (SysIsRetro(currentSystem)) { SidebarPanelRetro_Draw(font, red); return; }
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    const SysSettings *st = &sysSettings[currentSystem];
    float hx = sw * 0.449f, cx = sw * 0.46f, bx = sw * 0.485f, bw = sw * 0.387f, bh = sh * 0.066f;
    float th = fmaxf(2.0f, sh * 0.0028f);
    float hsize = (float)font.baseSize * 0.85f, osize = (float)font.baseSize * 1.2f;   /* video sizes */
    struct { StrId head; float hy; } heads[] = {
        { S_SET_VERSIONS, 0.212f }, { S_SET_RUMBLE, 0.500f }, { S_SET_CONTROLS, 0.696f } };
    struct { StrId label; float y; bool radio; bool on; } rows[SB_SET_CONTROLS + 1] = {
        { S_SET_VER_NA, 0.280f, true, st->versions == 0 },
        { S_SET_VER_NA_EU, 0.373f, true, st->versions == 1 },
        { S_SET_ENABLED, 0.569f, false, st->rumble },
        { S_SET_SHOW, 0.766f, false, st->showControls } };

    for (int k = 0; k < 3; k++) {
        DrawFitText(font, T(heads[k].head), (Vector2){ hx, sh * heads[k].hy }, hsize, sw - hx, WHITE);
    }
    for (int k = 0; k <= SB_SET_CONTROLS; k++) {
        float y = sh * rows[k].y;
        bool cur = (k == sidebarSetRow);
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        DrawFitText(font, T(rows[k].label), (Vector2){ bx + sw * 0.0225f, y }, osize,
                    bw - sw * 0.03f, cur ? red : WHITE);

        float r = sh * 0.016f;
        if (rows[k].radio) {
            if (rows[k].on) {
                DrawCircleV((Vector2){ cx, y }, r, WHITE);
                DrawCircleV((Vector2){ cx, y }, r * 0.62f, red);
            } else {
                IconArc((Vector2){ cx, y }, r, r, 0.0f, 360.0f, th, WHITE);
            }
        } else {
            Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
            if (rows[k].on) {
                DrawRectangleRec(cb, red);
                DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f },
                           th * 1.3f, WHITE);
                DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f },
                           th * 1.3f, WHITE);
            } else {
                DrawRectangleLinesEx(cb, th, WHITE);
            }
        }
    }
    {   /* Game Screen: CRT Geometry (N64 / GameCube games drawn by NSO; the others keep their window) */
        float hy = 0.828f, y = sh * 0.880f, r = sh * 0.016f;
        bool cur = (sidebarSetRow == SB_SET_CRTGEOM);
        DrawFitText(font, T(S_SET_SCREEN), (Vector2){ hx, sh * hy }, hsize, sw - hx, WHITE);
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        Rectangle cb = { cx - r, y - r, r * 2.0f, r * 2.0f };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        DrawFitText(font, T(S_SET_CRTGEOM), (Vector2){ bx + sw * 0.0225f, y }, osize, bw - sw * 0.03f, cur ? red : WHITE);
        if (st->crtgeom) {
            DrawRectangleRec(cb, red);
            DrawLineEx((Vector2){ cb.x + r * 0.45f, y + r * 0.05f }, (Vector2){ cx - r * 0.15f, y + r * 0.5f }, th * 1.3f, WHITE);
            DrawLineEx((Vector2){ cx - r * 0.15f, y + r * 0.5f }, (Vector2){ cb.x + r * 1.6f, y - r * 0.5f }, th * 1.3f, WHITE);
        } else {
            DrawRectangleLinesEx(cb, th, WHITE);
        }
    }
    if (SysHasPad(currentSystem)) {
        float hy = 0.928f, y = sh * 0.978f, r = sh * 0.016f;
        bool cur = (sidebarSetRow == SB_SET_PAD);
        DrawFitText(font, T(S_PC_HEAD), (Vector2){ hx, sh * hy }, hsize, sw - hx, WHITE);
        Rectangle box = { bx, y - bh * 0.5f, bw, bh };
        if (cur) DrawRectangleRec(box, WHITE);
        else DrawRectangleLinesEx(box, th, WHITE);
        DrawFitText(font, T(PadEntryLabel(currentSystem)), (Vector2){ bx + sw * 0.0225f, y }, osize,
                    bw - sw * 0.03f, cur ? red : WHITE);
        (void)r;
    }
}

/* Settings panel vertical shift: the controller mapping page (padui). */
static float sbPanelShift = 0.0f;

#define SNESLIST_IMPLEMENTATION
#include "sneslist.h"
#include "gamesel_ui.h"
#include "padui.h"

/* Settings panel; scrolls down only to the controller mapping page. */
static void SidebarPanels_Draw(Font font, Color red) {
    float sh = (float)GetScreenHeight();
    float target = padOpen ? sh * PADUI_PAGE_Y : 0.0f;
    sbPanelShift += (target - sbPanelShift) * fminf(1.0f, GetFrameTime() * 16.0f);
    if (fabsf(sbPanelShift - target) < 0.5f) sbPanelShift = target;
    if (SysHasPad(currentSystem) && sbPanelShift > sh * 1.0f) PadUi_Prepare();   /* its texture, outside the translation */
    rlPushMatrix();
    rlTranslatef(0.0f, -sbPanelShift, 0.0f);
    SidebarPanel_Draw(font, red);
    if (SysHasPad(currentSystem) && sbPanelShift > sh * 1.0f) PadUi_Draw(font, red);
    rlPopMatrix();
}

static void DrawSidebar(Color systemBg, Font font) {
    /* Draw down to 1px width; hide only when thinner than 1px. */
    if (sidebarWidth < 1.0f) return;

    float sh = (float)GetScreenHeight();
    float sw = (float)GetScreenWidth();
    float railW = SidebarRailWidth();
    float t = SidebarExpandProgress();
    Color pink = ColorLerp(systemBg, WHITE, 0.5f);

    float iconS = sh * 0.052f;
    float boxS = sh * 0.066f;                        /* rail highlight square / row height */
    float iconX = railW * 0.5f + (sw * 0.0645f - railW * 0.5f) * t;
    float labelX = sw * 0.092f;
    bool settled = SidebarMenuSettled();
    float iconBox = iconS * 1.5f;

    /* Icon textures first: render-texture passes cannot run inside the scissor. */
    Texture2D iconTex[SB_ITEM_COUNT];
    for (int i = 0; i < SB_ITEM_COUNT; i++) {
        Color fg = (i == sidebarItem) ? systemBg : WHITE;
        iconTex[i] = SidebarIcon_Render(i, iconBox, iconS, fg, SidebarAnim_Progress(i), sbAnimDir[i]);
    }

    /* Blurred mosaic is drawn behind when expanded; dim it like the video. */
    if (t > 0.0f) DrawRectangle((int)sidebarWidth, 0, (int)(sw - sidebarWidth) + 1, (int)sh, Fade(BLACK, 0.35f * t));

    /* Move mode: the whole rail slides off to the left (after the icon
     * render-texture passes, which reset the matrix). */
    float slide = 0.5f - 0.5f * cosf(PI * railSlideT);
    bool slid = slide > 0.0f;
    if (slid) { rlPushMatrix(); rlTranslatef(-(sidebarWidth + 1.0f) * slide, 0.0f, 0.0f); }
    DrawRectangle(0, 0, (int)sidebarWidth, (int)sh, systemBg);

    BeginScissorMode(0, 0, (int)sidebarWidth, (int)sh);
    for (int i = 0; i < SB_ITEM_COUNT; i++) {
        float y = SidebarRowY(i);
        bool sel = (i == sidebarItem);
        Color fg = WHITE;
        if (sel) {
            /* Morph like the old gear strip: rail = padded rounded square,
             * open = full-width sharp bar (pink while the panel has focus). */
            float pad = (railW - boxS) * 0.5f * (1.0f - t);
            float roundness = SIDEBAR_HIGHLIGHT_ROUNDNESS * (1.0f - t);
            Rectangle bar = { pad, y - boxS * 0.5f, boxS + (sidebarWidth - boxS) * t, boxS };
            if (t <= 0.0f) bar.width = boxS;
            Color bc = (sidebarPanelFocus && settled) ? pink : WHITE;
            if (roundness < 0.001f) DrawRectangleRec(bar, bc);
            else DrawRectangleRounded(bar, roundness, 8, bc);
            fg = systemBg;
        }
        {
            float ts = (float)iconTex[i].width;
            DrawTexturePro(iconTex[i], (Rectangle){ 0, 0, ts, -ts },   /* RT is upside down */
                           (Rectangle){ iconX - iconBox * 0.5f, y - iconBox * 0.5f, iconBox, iconBox },
                           (Vector2){ 0 }, 0.0f, WHITE);
        }
        if (t > 0.0f) {
            static const StrId labels[SB_ITEM_COUNT] = { S_SB_SINGLE, S_SB_TOGETHER, S_SB_ONLINE, S_SB_SETTINGS };
            DrawFitText(font, T(labels[i]), (Vector2){ labelX, y }, (float)font.baseSize * 1.2f,
                        SidebarOpenWidth() - labelX - 12.0f, Fade(fg, t));
        }
    }
    EndScissorMode();
    if (slid) rlPopMatrix();

    if (settled && sidebarPanelFocus && sidebarItem == SB_ITEM_SETTINGS) {
        SidebarPanels_Draw(font, systemBg);
    }
}

static void ClampTileToContent(float *x, float *y, float w, float h,
                               int originX, int contentW, int screenH,
                               float pad) {
    float left = (float)originX + pad;
    float top = pad;
    float right = (float)(originX + contentW) - pad;
    float bottom = (float)screenH - pad;
    if (*x < left) *x = left;
    if (*y < top) *y = top;
    if (*x + w > right) *x = right - w;
    if (*y + h > bottom) *y = bottom - h;
}

static Font slotFont = { 0 };   /* menu font, set in main() */

/* No cover texture: a centred "?" in the slot (same tint as the cover). */
static void DrawMissingCover(Texture2D tex, Rectangle r, Color tint) {
    if (tex.id != 0 || slotFont.texture.id == 0) return;
    float fs = r.height * 0.4f;
    Vector2 m = MeasureTextEx(slotFont, "?", fs, 0.0f);
    DrawTextEx(slotFont, "?", (Vector2){ r.x + (r.width - m.x) * 0.5f, r.y + (r.height - m.y) * 0.5f },
               fs, 0.0f, ColorTint(LIGHTGRAY, tint));
}


/* Fit a cover to its slot (scale up or down, keep aspect). The scale
 * argument is then applied from that fitted size (1.18 for the selected
 * cover's grow; other covers stay at 1 or shrink in move/lift). */
static void CoverDestInSlot(Rectangle slot, Texture2D tex, float scale,
                            float v_offset,
                            float *ox, float *oy, float *dw, float *dh)
{
    float aspect = (tex.id > 0 && tex.width > 0 && tex.height > 0)
        ? (float)tex.width / (float)tex.height
        : (slot.height > 0.0f ? slot.width / slot.height : 4.0f / 3.0f);
    float fitW, fitH;
    FitAspectInCell(slot.width, slot.height, aspect, &fitW, &fitH);
    if (scale < 0.0f) scale = 0.0f;
    *dw = fitW * scale;
    *dh = fitH * scale;
    *ox = slot.x + (slot.width - *dw) * 0.5f;
    *oy = slot.y - v_offset + (slot.height - *dh) * 0.5f;
}

static void DrawSlotImages(const GridLayout *layout, Texture2D *images, int numImgs) {
    if (numImgs <= 0 || !layout) return;

    int origin_x = layout->originX;
    int content_w = layout->contentW;
    int screen_h = layout->screenH;
    float v_offset = viewportY;

    if (layout->rowCount < 1) return;

    float mv = liftT;
    if (mv > 0.0f) DrawRectangle(0, 0, GetScreenWidth(), screen_h, Fade(MOVE_BG, mv));
    float othersScale = 1.0f - (1.0f - MOVE_OTHERS_SCALE) * mv;
    Color othersTint = ColorLerp(WHITE, (Color){ 222, 222, 222, 255 }, mv);

    for (int i = 0; i < numImgs; i++) {
        if (i == GameIdx) continue;

        Rectangle item = layout->items[i];
        Rectangle slot = layout->slots[i];
        float x = item.x;
        float y = item.y;

        float tile_scale = animScales[i] * othersScale;
        float ox, oy, draw_w, draw_h;
        CoverDestInSlot((Rectangle){ x, y, item.width, item.height },
                        images[i], tile_scale, v_offset,
                        &ox, &oy, &draw_w, &draw_h);

        if (slot.y + slot.height - v_offset > 0.0f
            && slot.y - v_offset < (float)screen_h) {
            DrawTexturePro(images[i],
                           (Rectangle){0, 0, (float)images[i].width, (float)images[i].height},
                           (Rectangle){ox, oy, draw_w, draw_h},
                           (Vector2){0, 0},
                           0.0f,
                           ColorTint(GameTint(i), othersTint));
            DrawMissingCover(images[i], (Rectangle){ox, oy, draw_w, draw_h}, ColorTint(GameTint(i), othersTint));
            if (Repo_Offer(Game_Repo(&ActiveGames()[i])))
                Repo_BadgeDraw(slotFont, (Rectangle){ox, oy, draw_w, draw_h});
        }
    }

    if (GameIdx >= 0 && GameIdx < numImgs) {
        Rectangle item = layout->items[GameIdx];
        float x = item.x;
        float y = item.y;

        /* Lifted cover glides between slots (also back on B) instead of
         * jumping; plain cursor moves outside move mode still jump. */
        static int glideIdx = -1;
        static float gFromX, gFromY, gCurX, gCurY;
        static double gStart = 0.0;
        double now = GetTime();
        bool gliding = glideIdx >= 0 && now - gStart < (double)MOVE_GLIDE_SEC;
        if (gridLifted || liftT > 0.0f || gliding) {
            if (glideIdx < 0) {
                glideIdx = GameIdx; gCurX = x; gCurY = y; gStart = now - MOVE_GLIDE_SEC;
            } else if (glideIdx != GameIdx) {
                glideIdx = GameIdx; gFromX = gCurX; gFromY = gCurY; gStart = now;
            }
            float u = fminf(1.0f, (float)(now - gStart) / MOVE_GLIDE_SEC);
            float e = u < 0.5f ? 4.0f * u * u * u : 1.0f - powf(-2.0f * u + 2.0f, 3.0f) * 0.5f;
            if (u < 1.0f) { x = gFromX + (x - gFromX) * e; y = gFromY + (y - gFromY) * e; }
            gCurX = x; gCurY = y;
        } else {
            glideIdx = -1;
        }

        float scale = animScales[GameIdx];
        float draw_x, draw_y_scaled, scaled_w, scaled_h;
        CoverDestInSlot((Rectangle){ x, y, item.width, item.height },
                        images[GameIdx], scale, v_offset,
                        &draw_x, &draw_y_scaled, &scaled_w, &scaled_h);
        ClampTileToContent(&draw_x, &draw_y_scaled, scaled_w, scaled_h,
                           origin_x, content_w, screen_h, MOSAIC_CURSOR_PAD);

        if (gridLifted || mv > 0.0f) {
            /* Picked up (video): soft shadow all round, fading in with the mode. */
            for (int k = 1; k <= 8; k++) {
                float g = (float)k * 2.5f;
                DrawRectangleRounded((Rectangle){ draw_x - g, draw_y_scaled - g + 4.0f,
                                                  scaled_w + 2.0f * g, scaled_h + 2.0f * g },
                                     0.04f, 6, Fade(BLACK, 0.07f * mv));
            }
        }

        DrawTexturePro(images[GameIdx],
                       (Rectangle){0, 0, (float)images[GameIdx].width,
                                   (float)images[GameIdx].height},
                       (Rectangle){draw_x, draw_y_scaled, scaled_w, scaled_h},
                       (Vector2){0, 0},
                       0.0f,
                       GameTint(GameIdx));
        DrawMissingCover(images[GameIdx], (Rectangle){draw_x, draw_y_scaled, scaled_w, scaled_h},
                         GameTint(GameIdx));
        if (Repo_Offer(Game_Repo(&ActiveGames()[GameIdx])))
            Repo_BadgeDraw(slotFont, (Rectangle){draw_x, draw_y_scaled, scaled_w, scaled_h});

        float pulse = sinf((float)GetTime() * 5.6f) * 0.5f + 0.5f;
        unsigned char r = (unsigned char)(255 * (1.0f - pulse));
        unsigned char g = (unsigned char)(255 * (1.0f - pulse));
        unsigned char b = (unsigned char)(255 * (1.0f - pulse));
        unsigned char a = (unsigned char)(255 * (1.0f - pulse * 0.95f));
        Color line_color = {r, g, b, a};

        if (gridLifted) {
            /* Video: thin light edge on the lifted cover, no cursor pulse. */
            DrawRectangleLinesEx(
                (Rectangle){draw_x - 1.0f, draw_y_scaled - 1.0f, scaled_w + 2.0f, scaled_h + 2.0f},
                2.0f,
                (Color){ 225, 228, 228, 255 });
        } else {
            DrawRectangleLinesEx(
                (Rectangle){draw_x - 1.0f, draw_y_scaled - 1.0f, scaled_w + 2.0f, scaled_h + 2.0f},
                2.0f,
                line_color);
        }
    }
}

static void CollapseSidebarToRail(void) {
    if (sidebarClosing || sbWidthPhase != SB_WIDTH_IDLE) return;
    if (!sidebarOpen) return;

    sidebarExpandPending = false;
    sidebarClosing = true;
    sidebarOpen = true; /* stay open until rail settle */
    sidebarPanelFocus = false;
    Sidebar_Select(SB_ITEM_SINGLE);   /* rail highlight rests on Single player (video) */

    float rail = SidebarRailWidth();
    float open = SidebarOpenWidth();
    float travel = open - rail;
    if (travel < 0.0f) travel = 0.0f;
    /* Same full-width bounce as open (never past rail). */
    sbWidthRest = open;
    sbWidthPeak = open + travel * SIDEBAR_OVERSHOOT;
    sbWidthPhase = SB_WIDTH_TO_PEAK;
    targetSidebarWidth = sbWidthPeak;

    /* Close sidebar bar fully, then open mosaic (no overlap). */
    if (currentState == MOSAIC || currentState == LAUNCHING) {
        InfoBar_Request(INFO_BAR_MOSAIC);
    } else {
        InfoBar_Request(INFO_BAR_NONE);
    }
}

/* Actually expand width + info bars (call after selected tile has shrunk). */
static void ExpandSidebarMenu(void) {
    sidebarExpandPending = false;
    sidebarPanelFocus = false;
    SidebarAnim_Play(sidebarItem, +1);   /* play the selected row as the menu opens */
    sidebarClosing = false;
    sidebarOpen = true;

    float rail = SidebarRailWidth();
    float open = SidebarOpenWidth();
    float travel = open - rail;
    if (travel < 0.0f) travel = 0.0f;
    sbWidthRest = open;
    sbWidthPeak = open + travel * SIDEBAR_OVERSHOOT;
    sbWidthPhase = SB_WIDTH_TO_PEAK;
    targetSidebarWidth = sbWidthPeak;

    /* Close mosaic bar fully, then open sidebar bar (no overlap). */
    InfoBar_Request(INFO_BAR_SIDEBAR);
}

/* Start open sequence: shrink selection first, then expand. */
static void RequestSidebarExpand(void) {
    if (sidebarOpen || sidebarExpandPending || sidebarClosing || sbWidthPhase != SB_WIDTH_IDLE) {
        return;
    }
    sidebarExpandPending = true;
}

static void UpdateSidebarWidth(void) {
    float rail = SidebarRailWidth();
    float open = SidebarOpenWidth();

    if (sbWidthPhase == SB_WIDTH_TO_PEAK) {
        targetSidebarWidth = sbWidthPeak;
        if (fabsf(sidebarWidth - sbWidthPeak) < 1.0f) {
            sidebarWidth = sbWidthPeak;
            if (sidebarClosing) {
                /* Close: overshoot full width, then straight to rail. */
                sbWidthPhase = SB_WIDTH_TO_RAIL;
                targetSidebarWidth = rail;
            } else {
                /* Open: settle back to full open width. */
                sbWidthPhase = SB_WIDTH_TO_REST;
                targetSidebarWidth = sbWidthRest;
            }
        }
    } else if (sbWidthPhase == SB_WIDTH_TO_REST) {
        /* Open only: settle to full width after overshoot. */
        targetSidebarWidth = sbWidthRest;
        if (fabsf(sidebarWidth - sbWidthRest) < 1.0f) {
            sidebarWidth = sbWidthRest;
            targetSidebarWidth = sbWidthRest;
            sbWidthPhase = SB_WIDTH_IDLE;
        }
    } else if (sbWidthPhase == SB_WIDTH_TO_RAIL) {
        targetSidebarWidth = rail;
        if (fabsf(sidebarWidth - rail) < 1.0f) {
            sidebarWidth = rail;
            targetSidebarWidth = rail;
            sbWidthPhase = SB_WIDTH_IDLE;
            sidebarClosing = false;
            sidebarOpen = false;
        }
    } else if ((currentState == MOSAIC || currentState == LAUNCHING ||
                (screenSliding && slideDirection < 0)) && !sidebarOpen
               && !sidebarClosing) {
        if (screenSliding && slideDirection > 0) {
            /* Leave mosaic: collapse strip to 0 (shrink through 1px). */
            targetSidebarWidth = 0.0f;
        } else {
            /* Idle rail while browsing mosaic. */
            targetSidebarWidth = rail;
        }
    } else if (sidebarOpen && sbWidthPhase == SB_WIDTH_IDLE && !sidebarClosing) {
        /* Keep settled open width if display resizes. */
        targetSidebarWidth = open;
        sbWidthRest = open;
    }

    sidebarWidth += (targetSidebarWidth - sidebarWidth) * SIDEBAR_WIDTH_LERP;
    if (fabsf(sidebarWidth - targetSidebarWidth) < 1.0f) {
        sidebarWidth = targetSidebarWidth;
    }

    UpdateGearSpin();
}

/* Open menu input. Menu focus: Up/Down pick an item, A/Right on Settings
 * enters its panel (the other three items have no function), B closes to
 * the rail. Panel focus: Up/Down pick an option, A sets it (saved at once),
 * B/Left return to the menu. */
static void Sidebar_HandleInput(bool gpUp, bool gpDown, bool gpLeft, bool gpRight,
                                bool gpConfirm, bool gpBack) {
    bool up = IsKeyPressed(KEY_UP) || gpUp, down = IsKeyPressed(KEY_DOWN) || gpDown;
    bool ok = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm;
    bool back = IsKeyPressed(KEY_B) || gpBack;
    if (PadUi_SwallowBack()) back = false;
    if (gsOpen && sidebarPanelFocus) {   /* Select games owns the panel (and its runs) */
        GameSel_Input(up, down, ok, back, IsKeyPressed(KEY_LEFT) || gpLeft);
        return;
    }
    if (padOpen && sidebarPanelFocus) {   /* the controller screen owns the panel */
        PadUi_Input(up, down, ok, back, IsKeyPressed(KEY_LEFT) || gpLeft);
        return;
    }
    if (scrapeBusy) {   /* a scrape owns the menu: B cancels it */
        if (back) ScrapeMenu_Cancel();
        return;
    }
    if (sidebarPanelFocus) {
        if (back || IsKeyPressed(KEY_LEFT) || gpLeft) { sidebarPanelFocus = false; Hide_Resync(true); return; }
        /* the CRT Geometry row of N64/GameCube does not exist for NES/SNES (theirs sits under Scanlines) */
        int skipRow = SysIsRetro(currentSystem) ? SB_SET_CRTGEOM : -1;
        if (up && sidebarSetRow > 0) {
            sidebarSetRow--;
            if (sidebarSetRow == skipRow) sidebarSetRow--;
            Sfx_Play(SFX_PLUCK);
        }
        if (down && sidebarSetRow < SetRowLast()) {
            sidebarSetRow++;
            if (sidebarSetRow == skipRow) sidebarSetRow++;
            Sfx_Play(SFX_PLUCK);
        }
        if (ok) {
            Sfx_Play(SFX_CLICK);
            SysSettings *st = &sysSettings[currentSystem];
            if (sidebarSetRow == SB_SET_PAD) {
                PadUi_Open(PadUi_KindOf(currentSystem));
                return;
            }
            if (SysIsRetro(currentSystem) && sidebarSetRow == SB_RETRO_PIXEL) st->pixel = !st->pixel;
            else if (SysIsRetro(currentSystem) && sidebarSetRow == SB_RETRO_SCANLINES) st->scanlines = !st->scanlines;
            else if (SysIsRetro(currentSystem) && sidebarSetRow == SB_RETRO_CRTGEOM) {
                st->crtgeom = !st->crtgeom;
            }
            else if (SysIsRetro(currentSystem)) st->showControls = !st->showControls;
            else if (sidebarSetRow == SB_SET_VER_NA) st->versions = 0;
            else if (sidebarSetRow == SB_SET_VER_NA_EU) st->versions = 1;
            else if (sidebarSetRow == SB_SET_RUMBLE) st->rumble = !st->rumble;
            else if (sidebarSetRow == SB_SET_CRTGEOM) {
                st->crtgeom = !st->crtgeom;
            }
            else st->showControls = !st->showControls;
            SysSettings_Save(currentSystem);
        }
        return;
    }
    if (back) { CollapseSidebarToRail(); return; }
    if (up && sidebarItem > 0) Sidebar_Select(sidebarItem - 1);
    if (down && sidebarItem < SB_ITEM_COUNT - 1) Sidebar_Select(sidebarItem + 1);
    if (sidebarItem == SB_ITEM_SETTINGS && (ok || IsKeyPressed(KEY_RIGHT) || gpRight)) {
        sidebarPanelFocus = true;
        sidebarSetRow = 0;
        sbPanelShift = 0.0f;
        if (padOpen) PadUi_Close();
        if (!GameSel_Busy()) gsOpen = false;
        Msg_Clear(&scrapeMsg);
        Sfx_Play(SFX_CLICK);
    } else if (ok) {
        SidebarAnim_Play(sidebarItem, +1);   /* no function: replay the one-shot */
        Sfx_Play(SFX_CLICK);
        if (sidebarItem == SB_ITEM_TOGETHER || sidebarItem == SB_ITEM_ONLINE)
            LaunchMsg_Show(S_COMING_SOON);
    }
}

static bool SortPanel_Active(void);

static bool OverlayActive(void) {
    return SortPanel_Active() || sidebarOpen || sidebarExpandPending || sidebarClosing
        || sbWidthPhase != SB_WIDTH_IDLE
        || mosaicExitPhase != MOSAIC_EXIT_NONE
        || liveConfirm != 0
        || powerDialogOpen;
}

/* Select-games pipeline (gamesel.h) of a catalog game, -1 = none. */
static int GamePipelineOf(const GameEntry *game) {
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        const GameEntry *g = systems[s].games;
        if (game >= g && game < g + *systems[s].gameCount)
            return Gs_FindPipeline(systems[s].platform->id, game->title);
    }
    return -1;
}

/* Resolve a catalog/scan ROM path after the roms/ move (repos/roms → roms).
 * dst is NSO-relative. */
static bool Rom_ResolveRel(const char *rel, char *dst, size_t n)
{
    if (!dst || n == 0) return false;
    dst[0] = '\0';
    if (!rel || !rel[0]) return false;
    if (FileExists(GetDataPath(rel))) { snprintf(dst, n, "%s", rel); return true; }
    if (strncmp(rel, "repos/roms/", 11) == 0) {
        char alt[1200];
        snprintf(alt, sizeof(alt), "%s/%s", Gs_RomRoot(), rel + 11);
        if (FileExists(GetDataPath(alt))) { snprintf(dst, n, "%s", alt); return true; }
    }
    return false;
}

/* Playable: the binary plus whatever its pipeline says the game also needs
 * (assets, the provisioned ROM). */
static bool GameIsLaunchable(const GameEntry *game) {
    int p;
    char romRel[1200];
    if (!GameBinaryPresent(game)) return false;
    if (game->rom && game->rom[0] && !Rom_ResolveRel(game->rom, romRel, sizeof(romRel)))
        return false;
    p = GamePipelineOf(game);
    return p < 0 || Gs_NeedsMet(&gs_pipelines[p], GsRelExists, NULL);
}

/* Path consumed by the next LaunchGame / StartEmbeddedGame (NSO_STATE_LOAD). */
static char stateLoadPath[1200];

/* ---- Y Move: user game order, config/order-<system id>.txt ---- */
static void Order_Path(int sys, char *dst, size_t n) {
    snprintf(dst, n, "%s/config/order-%s.txt", GetBaseDirectory(), systems[sys].platform->id);
}

/* Move loaded covers and slot sizes onto the rebuilt list by game key, not
 * slot index. shown < count unloads textures for the hidden tail. */
static void Covers_SyncToGames(const GameEntry *games, int count, int shown)
{
    static char newK[MAX_GAMES][COVER_KEY_MAX];
    static int from[MAX_GAMES];
    static Texture2D art[MAX_GAMES];
    static float scale[MAX_GAMES], metaW[MAX_GAMES], metaH[MAX_GAMES];
    static char keys[MAX_GAMES][COVER_KEY_MAX];
    static unsigned char used[MAX_GAMES];
    int oldN = coverSlotN, i;
    if (count < 0) count = 0;
    if (count > MAX_GAMES) count = MAX_GAMES;
    if (shown < 0 || shown > count) shown = count;
    if (oldN < 0) oldN = 0;
    if (oldN > MAX_GAMES) oldN = MAX_GAMES;
    for (i = 0; i < count; i++) {
        const GameEntry *g = games ? &games[i] : NULL;
        Covers_Key(g ? g->title : NULL, g ? g->boxartPath : NULL, g ? g->rom : NULL,
                   newK[i], sizeof(newK[i]));
    }
    Covers_PlanMoves(coverSlotKey, oldN, newK, count, from);
    memset(used, 0, sizeof used);
    for (i = 0; i < count; i++) {
        int s = from[i];
        if (s >= 0 && s < oldN) {
            art[i] = gameBoxarts[s];
            scale[i] = animScales[s];
            metaW[i] = boxartMetaW[s];
            metaH[i] = boxartMetaH[s];
            memcpy(keys[i], coverSlotKey[s], sizeof keys[i]);
            used[s] = 1;
        } else {
            art[i] = (Texture2D){ 0 };
            scale[i] = 1.0f;
            metaW[i] = 0.0f;
            metaH[i] = 0.0f;
            memcpy(keys[i], newK[i], sizeof keys[i]);
        }
    }
    for (i = 0; i < oldN; i++) {
        if (!used[i] && gameBoxarts[i].id != 0) UnloadTexture(gameBoxarts[i]);
    }
    for (i = 0; i < count; i++) {
        gameBoxarts[i] = art[i];
        animScales[i] = scale[i];
        boxartMetaW[i] = metaW[i];
        boxartMetaH[i] = metaH[i];
        memcpy(coverSlotKey[i], keys[i], sizeof coverSlotKey[i]);
    }
    for (i = shown; i < count; i++) {   /* hidden covers are not kept on the GPU */
        if (gameBoxarts[i].id != 0) UnloadTexture(gameBoxarts[i]);
        gameBoxarts[i] = (Texture2D){ 0 };
    }
    for (i = count; i < oldN && i < MAX_GAMES; i++) {
        gameBoxarts[i] = (Texture2D){ 0 };
        coverSlotKey[i][0] = '\0';
        boxartMetaW[i] = 0.0f;
        boxartMetaH[i] = 0.0f;
    }
    coverSlotN = count;
}

/* At startup, before any cover is loaded: apply each system's saved order. */
#include "version_ui.h"

static void Cover_Replace(int gameIdx)
{
    if (gameIdx < 0 || gameIdx >= MAX_GAMES) return;
    CoverLoader_Stop();
    UnloadBoxart(gameIdx);
    {
        const char *rel = ActiveGames()[gameIdx].boxartPath;
        Image img = LoadBoxartImage(GetDataPath(rel));
        if (img.data) {
            gameBoxarts[gameIdx] = LoadTextureFromImage(img);
            BoxartMetaRemember(gameIdx, img.width, img.height);
            UnloadImage(img);
        }
        Cover_RememberGame(gameIdx, &ActiveGames()[gameIdx]);
    }
}

static void Order_LoadAll(void) {
    for (int s = 0; s < SYSTEM_COUNT; s++) {
        GameEntry *games = systems[s].games;
        int count = *systems[s].gameCount;
        char path[2112];
        if (count <= 1 || count > MAX_GAMES) continue;
        Order_Path(s, path, sizeof(path));
        if (!FileExists(path)) continue;
        char *text = LoadFileText(path);
        if (!text) continue;
        const char *titles[MAX_GAMES];
        int perm[MAX_GAMES];
        GameEntry tmp[MAX_GAMES];
        static char rewritten[MAX_GAMES * 256];
        const char *orderText = text;
        Version_RewriteOrder(s, text, rewritten, sizeof(rewritten));
        if (rewritten[0]) orderText = rewritten;
        for (int i = 0; i < count; i++) titles[i] = games[i].title;
        Order_Perm(orderText, titles, count, perm);
        UnloadFileText(text);
        for (int k = 0; k < count; k++) tmp[k] = games[perm[k]];
        memcpy(games, tmp, sizeof(GameEntry) * (size_t)count);
    }
}

static void Order_SaveActive(void) {
    static char buf[MAX_GAMES * 256];
    const char *titles[MAX_GAMES], *full[MAX_GAMES];
    char dir[2048], path[2112];
    int count = ActiveGameTotal(), shown = ActiveGameCount();
    if (count <= 0 || count > MAX_GAMES) return;
    for (int i = 0; i < count; i++) titles[i] = ActiveGames()[i].title;
    Order_Path(currentSystem, path, sizeof(path));
    if (shown < count) {   /* Hide unplayable: the hidden titles keep their old slots */
        char *old = FileExists(path) ? LoadFileText(path) : NULL;
        Hide_MergeOrder(old, titles, count, shown, full);
        if (old) UnloadFileText(old);
        memcpy(titles, full, sizeof(titles[0]) * (size_t)count);
    }
    if (Order_Format(buf, sizeof(buf), titles, count) != 0) {
        fprintf(stderr, "NSO: game order too long, not saved\n");
        return;
    }
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    MakeDirectory(dir);
    if (!SaveFileText(path, buf)) fprintf(stderr, "NSO: cannot save %s\n", path);
}

/* Lifted game to slot `target`; covers and tile scales travel with it. */
static void Grid_MoveLifted(int target) {
    int count = ActiveGameCount();
    if (target == GameIdx || target < 0 || target >= count || count > MAX_GAMES) return;
    Order_Move(ActiveGames(), sizeof(GameEntry), count, GameIdx, target);
    Order_Move(gameBoxarts, sizeof(Texture2D), count, GameIdx, target);
    Order_Move(animScales, sizeof(float), count, GameIdx, target);
    GameIdx = target;
    gridLiftMoved = true;
}

/* First pressed direction (Right, Left, Down, Up) or -1. */
static int GridNavPressed(bool gpLeft, bool gpRight, bool gpUp, bool gpDown) {
    if (IsKeyPressed(KEY_RIGHT) || gpRight) return ORDER_NAV_RIGHT;
    if (IsKeyPressed(KEY_LEFT) || gpLeft) return ORDER_NAV_LEFT;
    if (IsKeyPressed(KEY_DOWN) || gpDown) return ORDER_NAV_DOWN;
    if (IsKeyPressed(KEY_UP) || gpUp) return ORDER_NAV_UP;
    return -1;
}

#include "sortpanel.h"

/* ---- X Details panel ---- */
static int detailsRow = 0;   /* 0 = version (when shown), 1 = install */

static void Details_Open(void) {
    detailsOpen = true;
    detailsRow = 0;
    Repo_RefreshLocal();
    InfoBar_Request(INFO_BAR_NONE);
}

static void Details_Close(void) {
    detailsOpen = false;
    InfoBar_Request(INFO_BAR_MOSAIC);
}

static bool SortY_IsDown(void) {
    return IsKeyDown(KEY_Y)
        || padCDownHeld
        || (IsGamepadAvailable(0) && IsGamepadButtonDown(0, NSO_BTN_Y));
}

static bool SortY_IsPressed(void) {
    return IsKeyPressed(KEY_Y)
        || padCDown
        || (IsGamepadAvailable(0) && IsGamepadButtonPressed(0, NSO_BTN_Y));
}

static void Grid_BeginMove(void) {
    gridLifted = true;
    gridLiftMoved = false;
    gridLiftFrom = GameIdx;
}

/* Keyboard Y, gamepad Y, and C-Down on the games grid: hold SORT_HOLD_SEC
 * to open the sort panel; a shorter tap starts Move.
 * The info menu's Y (favorite) is handled separately while Details is open. */
static void SortY_Update(bool canHold, bool canMove) {
    if (sortYConsumed) {
        if (!SortY_IsDown()) sortYConsumed = false;
        sortYHeld = false;
        return;
    }
    if (SortY_IsDown()) {
        if (!sortYHeld) {
            if (canHold && SortY_IsPressed()) {
                sortYHeld = true;
                sortYT0 = GetTime();
            }
            return;
        }
        if (!canHold) {
            sortYHeld = false;
            sortYConsumed = true;
            return;
        }
        if (GetTime() - sortYT0 >= (double)SORT_HOLD_SEC) {
            sortYHeld = false;
            sortYConsumed = true;
            SortPanel_Open();
        }
        return;
    }
    if (sortYHeld) {
        sortYHeld = false;
        if (canMove) Grid_BeginMove();
    }
}

static void Details_Update(float dt) {
    float target = detailsOpen ? 1.0f : 0.0f;
    float step = dt * 1000.0f / DETAILS_FADE_DELTA_MS
               * (detailsOpen ? DETAILS_FADE_IN_STEP : DETAILS_FADE_OUT_STEP);
    if (detailsT < target) detailsT = fminf(detailsT + step, target);
    else if (detailsT > target) detailsT = fmaxf(detailsT - step, target);
}

static int Details_Repo(const GameEntry *g) { return Game_Repo(g); }

static void Details_HandleInput(bool gpConfirm, bool gpBack,
                                bool gpLeft, bool gpRight, bool gpUp, bool gpDown) {
    bool gp = IsGamepadAvailable(0);
    if (!detailsOpen) return;   /* sliding out */
    if (IsKeyPressed(KEY_B) || gpBack) {
        Details_Close();
        return;
    }
    if (GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm;
    int repo = Details_Repo(g);
    bool hasVer = Version_CountAt(currentSystem, g->title) > 1;
    bool hasInst = Repo_Offer(repo);
    if (hasVer && hasInst) {
        if (IsKeyPressed(KEY_UP) || gpUp) detailsRow = 0;
        if (IsKeyPressed(KEY_DOWN) || gpDown) detailsRow = 1;
    } else if (hasVer) detailsRow = 0;
    else detailsRow = 1;
    if (hasVer && detailsRow == 0) {
        int dir = 0;
        if (IsKeyPressed(KEY_RIGHT) || gpRight) dir = 1;
        if (IsKeyPressed(KEY_LEFT) || gpLeft) dir = -1;
        if (dir && Version_Cycle(currentSystem, GameIdx, dir)) Sfx_Play(SFX_CLICK);
    }
    if (hasInst && detailsRow == 1) {   /* Install is the highlighted row while offered */
        if (confirm) Repo_Enqueue(repo, true);
    }
    if (IsKeyPressed(KEY_Y) || padCDown || (gp && IsGamepadButtonPressed(0, NSO_BTN_Y))) {
        Sfx_Play(SFX_CLICK);
        Fav_ToggleCurrent();
        if (favOn[currentSystem]) {
            SortPanel_Apply(currentSystem, sortMode[currentSystem], true);
            CoverLoader_Start();
            if (GameIdx >= ActiveGameCount()) Details_Close();
        }
    }
    /* Info menu only: keyboard X and gamepad X hide/unhide. Grid tap/hold X is unchanged.
     * Rebuild off-screen and present the finished grid in one frame (same
     * serialize/hold as Hide Unplayable). The current title leaves this view,
     * so Details closes now; the hide is not deferred to that close. */
    if (IsKeyPressed(KEY_X) || padCUp || (gp && IsGamepadButtonPressed(0, NSO_BTN_X))) {
        int keep = GameIdx;
        Sfx_Play(SFX_CLICK);
        Hidden_ToggleCurrent();
        SortPanel_RelistReload();
        /* Apply follows the hidden title to the packed tail, then clamps to
         * the last slot. Keep the old index so the cursor lands on the game
         * that slid into this slot (or the new last item if this was last). */
        GameIdx = Hide_Clamp(keep, ActiveGameCount());
        Details_Close();
    }
}

/* Draw `text` at most maxW wide (shrinks down to 60% of size). */
static float Details_Text(Font font, const char *text, Vector2 pos, float size, float maxW, Color c) {
    float s = size;
    while (s > size * 0.6f && MeasureTextEx(font, text, s, 2.0f).x > maxW) s -= 1.0f;
    DrawTextEx(font, text, pos, s, 2.0f, c);
    return s;
}

/* Video-style gradient glow around the highlighted row: pink top, cyan right,
 * blue corners; traced along the rounded outline so it follows the corners. */
static void Details_GlowRect(Rectangle r, float rad, float th, float alpha) {
    const Color cTL = { 110, 150, 245, 255 }, cTR = { 245, 185, 232, 255 };
    const Color cBR = { 90, 125, 245, 255 },  cBL = { 190, 235, 250, 255 };
    const int steps = 10;
    Vector2 ctr[4] = { { r.x + r.width - rad, r.y + rad }, { r.x + r.width - rad, r.y + r.height - rad },
                       { r.x + rad, r.y + r.height - rad }, { r.x + rad, r.y + rad } };
    Vector2 prev = { 0 };
    for (int k = 0; k <= 4 * (steps + 1); k++) {
        int q = (k / (steps + 1)) % 4, j = k % (steps + 1);
        float a = (-90.0f + 90.0f * (float)q + 90.0f * (float)j / (float)steps) * DEG2RAD;
        Vector2 p = { ctr[q].x + rad * cosf(a), ctr[q].y + rad * sinf(a) };
        float u = (p.x - r.x) / r.width, v = (p.y - r.y) / r.height;
        Color col = ColorLerp(ColorLerp(cTL, cTR, u), ColorLerp(cBL, cBR, u), v);
        col = Fade(col, alpha);
        if (k > 0) DrawLineEx(prev, p, th, col);
        DrawCircleV(p, th * 0.5f, col);
        prev = p;
    }
}

/* The card frame shared by Details and Status: dimmed grid, one rounded panel
 * (lighter left column). Returns the card, its left column width and radius. */
static void Details_DrawFrameH(float a, Rectangle *cardOut, float *lwOut, float *radOut, float height) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    const Color colL = { 240, 245, 244, 255 }, colR = { 229, 235, 237, 255 };

    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.40f * a));

    /* Integer bounds; the same rounded panel is drawn twice, each clipped to
     * its own column, so every pixel is blended exactly once (no seam, no
     * shade change mid-fade, corners intact). */
    Rectangle card = { roundf(sw * 0.042f), roundf(sh * 0.047f), roundf(sw * 0.916f), height };
    float rad = sh * 0.012f, lw = roundf(sw * 0.259f);
    float rnd = 2.0f * rad / fminf(card.width, card.height);
    int cx0 = (int)card.x, cy0 = (int)card.y, split = cx0 + (int)lw;
    int cx1 = cx0 + (int)card.width, ch = (int)card.height;
    BeginScissorMode(cx0, cy0, split - cx0, ch);
    DrawRectangleRounded(card, rnd, 12, Fade(colL, a));
    EndScissorMode();
    BeginScissorMode(split, cy0, cx1 - split, ch);
    DrawRectangleRounded(card, rnd, 12, Fade(colR, a));
    EndScissorMode();
    *cardOut = card;
    *lwOut = lw;
    *radOut = rad;
}

static void Details_DrawFrame(float a, Rectangle *cardOut, float *lwOut, float *radOut) {
    Details_DrawFrameH(a, cardOut, lwOut, radOut, roundf((float)GetScreenHeight() * 0.905f));
}

/* Details card as in the video: frosted, dimmed grid behind; one light card
 * (one rounded panel; lighter cover column left, text column right). */
static void Details_Draw(Font font) {
    if (detailsT <= 0.0f || GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    float a = detailsT;   /* linear opacity; the card never moves or resizes */
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    const Color ink = { 44, 49, 50, 255 }, meta = { 92, 97, 98, 255 };
    Rectangle card;
    float lw, rad;
    Details_DrawFrame(a, &card, &lw, &rad);

    Texture2D tex = gameBoxarts[GameIdx];
    float cvW, cvH;
    FitAspectInCell(sw * 0.208f, sh * 0.30f, GridItemAspect(GameIdx), &cvW, &cvH);
    Rectangle cr = { card.x + (lw - cvW) * 0.5f, sh * 0.139f, cvW, cvH };
    if (tex.id) {
        DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height }, cr,
                       (Vector2){ 0 }, 0.0f, Fade(WHITE, a));
    } else {
        DrawRectangleRec(cr, Fade(DARKGRAY, a));
        DrawMissingCover(tex, cr, Fade(WHITE, a));
    }

    float tx = sw * 0.334f, tw = card.x + card.width - sw * 0.05f - tx;
    float titleSize = sh * 0.040f, heartSize = sh * 0.048f, iconGap = sw * 0.012f;
    const Color heartCol = { 230, 36, 40, 255 };
    int heartCp = Fav_IsTitle(currentSystem, g->title) ? FAV_HEART : FAV_HEART_O;
    bool blocked = Hidden_IsTitle(currentSystem, g->title);
    float heartW = favNerdLoaded ? Hint_GlyphAdvance(favNerdFont, heartCp, heartSize) : 0.0f;
    float blockW = hideNerdLoaded ? Hint_GlyphAdvance(favNerdFont, HIDE_BLOCK, heartSize) : 0.0f;
    float iconsW = 0.0f;
    if (favNerdLoaded) iconsW += heartW;
    if (hideNerdLoaded) iconsW += (iconsW > 0.0f ? iconGap : 0.0f) + blockW;
    float titleMax = tw - (iconsW > 0.0f ? iconsW + iconGap : 0.0f);
    titleSize = Details_Text(font, g->title ? g->title : "", (Vector2){ tx, sh * 0.140f },
                             titleSize, titleMax, Fade(ink, a));
    float hy = sh * 0.140f;
    if (favNerdLoaded) {
        float hh = Hint_GlyphHeight(favNerdFont, heartCp, heartSize);
        hy = sh * 0.140f + (titleSize - hh) * 0.5f;
    } else if (hideNerdLoaded) {
        float hh = Hint_GlyphHeight(favNerdFont, HIDE_BLOCK, heartSize);
        hy = sh * 0.140f + (titleSize - hh) * 0.5f;
    }
    if (favNerdLoaded) {
        float hx = tx + tw - iconsW;
        DrawTextCodepoint(favNerdFont, heartCp, (Vector2){ hx, hy },
                          heartSize, Fade(heartCol, a));
    }
    if (hideNerdLoaded) {
        float bx = tx + tw - blockW;
        Color blockCol = blocked ? heartCol : (Color){ 150, 150, 150, 255 };
        DrawTextCodepoint(favNerdFont, HIDE_BLOCK, (Vector2){ bx, hy },
                          heartSize, Fade(blockCol, blocked ? a : a * 0.55f));
    }

    int repo = Details_Repo(g);
    char source[2304], status[192] = "";
    if (repo >= 0) {
        const char *u = nso_repos[repo].url;
        if (strncmp(u, "https://", 8) == 0) u += 8;
        snprintf(source, sizeof(source), "%s", u);
        Msg_Format(&repoMsg[repo], status, sizeof(status));
        if (repo == repoRunning) strncat(status, "...", sizeof(status) - strlen(status) - 1);
    } else {
        snprintf(source, sizeof(source), "%s", g->dir ? g->dir : "-");
    }
    if (!g->dir || !g->binary) snprintf(status, sizeof(status), "%s", T(S_DET_UNAVAILABLE));
    else if (!status[0]) snprintf(status, sizeof(status), "%s",
                                  T(GameBinaryPresent(g) ? S_DET_INSTALLED : S_DET_NOT_INSTALLED));

    /* Video's two grey meta lines: "Label: value   Label: value". */
    float ms = sh * 0.026f;
    Details_Text(font, TextFormat("%s: %s      %s: %s", T(S_DET_SYSTEM), ActiveSystem()->platform->name,
                                  T(S_DET_STATUS), status),
                 (Vector2){ tx, sh * 0.220f - ms * 0.5f }, ms, tw, Fade(meta, a));
    Details_Text(font, TextFormat("%s: %s", T(S_DET_SOURCE), source),
                 (Vector2){ tx, sh * 0.253f - ms * 0.5f }, ms, tw, Fade(meta, a));

    /* Version selector (left/right) when a group has more than one release. */
    int verN = 1, verI = 0;
    char verLab[VERSION_NAME];
    verLab[0] = '\0';
    Version_LabelAt(currentSystem, g->title, verLab, sizeof(verLab), &verI, &verN);
    float rowY = sh * 0.32f, rowH = sh * 0.085f, rowGap = sh * 0.018f;
    if (verN > 1) {
        float rs = sh * 0.033f, rh = rowH, ry = rowY;
        float rx = tx - sw * 0.012f, rw = sw * 0.56f;
        Rectangle rr = { rx, ry, rw, rh };
        bool focus = detailsRow == 0 || !Repo_Offer(repo);
        DrawRectangleRounded(rr, 2.0f * rad / rh, 12, Fade((Color){ 250, 252, 252, 255 }, a));
        if (focus) Details_GlowRect(rr, rad, fmaxf(2.0f, sh * 0.0045f), a);
        char line[256];
        snprintf(line, sizeof(line), "%s:  ‹  %s  ›", T(S_DET_VERSION), verLab[0] ? verLab : "");
        Details_Text(font, line, (Vector2){ tx, ry + (rh - rs) * 0.5f }, rs, rw - sw * 0.03f,
                     Fade(ink, a));
        rowY = ry + rh + rowGap;
    }

    /* Install is highlighted while offered (and focused when both rows exist). */
    if (Repo_Offer(repo)) {
        float rs = sh * 0.033f, rh = rowH, ry = rowY;
        float rx = tx - sw * 0.012f, rw = sw * 0.56f;
        Rectangle rr = { rx, ry, rw, rh };
        bool focus = detailsRow == 1 || verN <= 1;
        DrawRectangleRounded(rr, 2.0f * rad / rh, 12, Fade((Color){ 250, 252, 252, 255 }, a));
        if (focus) Details_GlowRect(rr, rad, fmaxf(2.0f, sh * 0.0045f), a);
        Details_Text(font, T(S_DET_INSTALL), (Vector2){ tx, ry + (rh - rs) * 0.5f }, rs, rw - sw * 0.03f,
                     Fade(ink, a));
    }

    DrawInfoBarEx(font, T(S_DET_INFO), a, 0.0f);
}


/* Preview the focused box-art candidate (a short curl into assets/cache). */
static Texture2D scrapeChoicePrev;
static int scrapeChoicePrevAt = -1;
static char scrapeChoicePrevPart[4200];
static NsoProc scrapeChoicePrevProc;
static int scrapeChoicePrevBusy;

static void ScrapeMenu_ChoicePrevUnload(void)
{
    if (scrapeChoicePrev.id != 0) UnloadTexture(scrapeChoicePrev);
    scrapeChoicePrev = (Texture2D){ 0 };
}

static void ScrapeMenu_ChoicePrevStop(void)
{
    if (scrapeChoicePrevBusy && NsoProcOk(scrapeChoicePrevProc))
        NsoSpawnKill(scrapeChoicePrevProc);
    scrapeChoicePrevProc = NsoProcNone();
    scrapeChoicePrevBusy = 0;
    if (scrapeChoicePrevPart[0]) {
        FileRemove(scrapeChoicePrevPart);
        scrapeChoicePrevPart[0] = '\0';
    }
    ScrapeMenu_ChoicePrevUnload();
    scrapeChoicePrevAt = -1;
}

static void ScrapeMenu_ChoicePrevPoll(void)
{
    const char *file, *playlist;
    char url[2048], cache[2400];
    int code = 0;
    if (!(scrapeBusy && scrapePhase == SCRAPE_CHOOSE)) {
        if (scrapeChoicePrevBusy || scrapeChoicePrev.id || scrapeChoicePrevAt >= 0)
            ScrapeMenu_ChoicePrevStop();
        return;
    }
    if (scrapeChoicePrevBusy && NsoSpawnExited(&scrapeChoicePrevProc, &code)) {
        scrapeChoicePrevBusy = 0;
        scrapeChoicePrevProc = NsoProcNone();
        if (code == 0 && scrapeChoicePrevPart[0] && Scrape_LooksLikeImage(scrapeChoicePrevPart)) {
            Image img = LoadBoxartImage(scrapeChoicePrevPart);
            ScrapeMenu_ChoicePrevUnload();
            if (img.data) {
                scrapeChoicePrev = LoadTextureFromImage(img);
                UnloadImage(img);
            }
        }
        if (scrapeChoicePrevPart[0]) FileRemove(scrapeChoicePrevPart);
        scrapeChoicePrevPart[0] = '\0';
    }
    /* One attempt per focused candidate: a failed download or decode leaves
     * the pane empty instead of respawning curl every frame. */
    if (scrapeChoicePrevAt == scrapeChoiceCur)
        return;
    if (scrapeChoicePrevBusy) {
        NsoSpawnKill(scrapeChoicePrevProc);
        scrapeChoicePrevProc = NsoProcNone();
        scrapeChoicePrevBusy = 0;
        if (scrapeChoicePrevPart[0]) FileRemove(scrapeChoicePrevPart);
        scrapeChoicePrevPart[0] = '\0';
    }
    ScrapeMenu_ChoicePrevUnload();
    scrapeChoicePrevAt = scrapeChoiceCur;
    file = (scrapeChoiceCur >= 0 && scrapeChoiceCur < scrapeChoiceN)
        ? scrapeChoices[scrapeChoiceCur] : NULL;
    playlist = Scrape_Playlist(scrapeId[0] ? scrapeId : ActiveSystem()->platform->id);
    if (!file || !file[0] || !playlist
        || Scrape_ImageUrl(url, sizeof(url), playlist, file) != 0)
        return;
    snprintf(cache, sizeof(cache), "%s", GetDataPath("assets/cache"));
    MakeDirectory(GetDataPath("assets"));
    MakeDirectory(cache);
    {
        /* Unique per download, so a killed or slow curl can never write into
         * the file the next one (or the decoder) is using. */
        static unsigned prevSeq = 0;
        snprintf(scrapeChoicePrevPart, sizeof(scrapeChoicePrevPart),
                 "%s/scrape-choice-preview-%ld-%u.part", cache, (long)getpid(), ++prevSeq);
    }
    {
        char *argv[13];
        argv[0] = "curl";
        argv[1] = "-fsSL";
        argv[2] = "--max-time";
        argv[3] = "30";
        argv[4] = "--retry";
        argv[5] = "1";
        argv[6] = "--retry-delay";
        argv[7] = "1";
        argv[8] = "--create-dirs";
        argv[9] = "-o";
        argv[10] = scrapeChoicePrevPart;
        argv[11] = url;
        argv[12] = NULL;
        scrapeChoicePrevProc = NsoSpawnArgv(NULL, argv);
        if (!NsoProcOk(scrapeChoicePrevProc)) {
            scrapeChoicePrevPart[0] = '\0';
            return;
        }
        scrapeChoicePrevBusy = 1;
    }
}

static Texture2D ScrapeMenu_ChoicePreview(void)
{
    ScrapeMenu_ChoicePrevPoll();
    return scrapeChoicePrev;
}

#include "statuspanel.h"

static bool GameHasExternalMenu(const GameEntry *game)
{
#ifndef _WIN32
    /* Module games already use embed.h directly; every child game can use the
     * generic X11 bridge unless it has its own external-menu implementation. */
    return game && (game->externalMenu || !game->embedRomKey);
#else
    (void)game;
    return false;
#endif
}

static bool GameUsesMenuLauncher(const GameEntry *game)
{
#ifndef _WIN32
    return game && !game->externalMenu && !game->embedRomKey;
#else
    (void)game;
    return false;
#endif
}

static bool GameMenuDirectory(char *out, size_t cap, const GameEntry *game)
{
    char relative[256];
    int n;
    if (!game || !out || cap == 0) return false;
    if (game->stateDir && game->stateDir[0]) {
        snprintf(relative, sizeof(relative), "%s", game->stateDir);
    } else {
        const char *system = ActiveSystem() && ActiveSystem()->platform
            ? ActiveSystem()->platform->id : "games";
        snprintf(relative, sizeof(relative), "saves/menu/%s/%d", system, launchingGameIdx);
    }
    n = snprintf(out, cap, "%s", GetDataPath(relative));
    return n > 0 && (size_t)n < cap;
}

static void EnsureDirectoryTree(const char *path)
{
    char current[1200];
    size_t len = strlen(path);
    if (!len || len >= sizeof(current)) return;
    memcpy(current, path, len + 1);
    for (char *p = current + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (!DirectoryExists(current)) MakeDirectory(current);
            *p = '/';
        }
    }
    if (!DirectoryExists(current)) MakeDirectory(current);
}

/* CRT Geometry fine settings (crtparams.h): a game that runs with the CRT Geometry toggle on gets a session
 * with its own steps (config/crtgeom/<system>/<title slug>.cfg) over the system defaults
 * (config/crtgeom/<system>/_default.cfg). runPath: Mednafen's params file, NULL for in-process games. */
static void CrtSessionEnd(void) {
    float stock[CRTP_COUNT];
    for (int i = 0; i < CRTP_COUNT; i++) stock[i] = crtParamInfo[i].def;
    CrtSession_End();
    CrtGeom_SetParams(stock, false);
    CrtGeom_SetVirtualHeight(0.0f);
}

/* rawFrames: the hosted port draws the frame NSO captures unfiltered, so NSO applies the look to its frozen frame */
static void CrtSessionBegin(const GameEntry *game, const char *runPath, bool rawFrames) {
    char slug[128], gamePath[2048], defPath[2048];
    const char *sys = systems[currentSystem].platform->id;
    size_t n = 0;
    for (const char *t = game->title; *t && n + 1 < sizeof(slug); t++) {
        unsigned char c = (unsigned char)*t;
        if (isalnum(c)) slug[n++] = (char)tolower(c);
        else if (n && slug[n - 1] != '-') slug[n++] = '-';
    }
    while (n && slug[n - 1] == '-') n--;
    slug[n] = '\0';
    snprintf(gamePath, sizeof(gamePath), "%s", GetDataPath(TextFormat("config/crtgeom/%s/%s.cfg", sys, n ? slug : "game")));
    snprintf(defPath, sizeof(defPath), "%s", GetDataPath(TextFormat("config/crtgeom/%s/_default.cfg", sys)));
    CrtSession_Begin(gamePath, defPath, runPath);
    crtSession.rawFrames = rawFrames;
    CrtGeom_SetParams(crtSession.val, crtSession.angleSet);
    CrtGeom_SetVirtualHeight(rawFrames ? 480.0f : 0.0f);
}

/* Is the port's binary built with tools/soh-nsocrt.patch? It carries a marker string. */
static bool CrtHookBuilt(const char *binary) {
    static const char marker[] = "nso-crtgeom-hook-v1";
    const size_t mlen = sizeof(marker) - 1;
    FILE *f = fopen(binary, "rb");
    if (!f) return false;
    static unsigned char buf[1 << 16];
    size_t keep = 0;
    bool found = false;
    for (;;) {
        size_t n = fread(buf + keep, 1, sizeof(buf) - keep, f);
        if (n == 0) break;
        size_t total = keep + n;
        for (size_t i = 0; i + mlen <= total && !found; i++) {
            const unsigned char *at = memchr(buf + i, marker[0], total - mlen - i + 1);
            if (!at) break;
            i = (size_t)(at - buf);
            found = memcmp(at, marker, mlen) == 0;
        }
        if (found) break;
        keep = total >= mlen ? mlen - 1 : total;
        memmove(buf, buf + total - keep, keep);
    }
    fclose(f);
    return found;
}

/* GameEntry.env: set each NAME=VALUE for the next spawn ('@' = NSO-relative
 * path), then put NSO's own values back once the child has its copy. */
#define GAME_ENV_MAX 8
static char gameEnvSpec[512];
static char *gameEnvWords[GAME_ENV_MAX];
static char *gameEnvSaved[GAME_ENV_MAX];
static int gameEnvCount = 0;

/* The env word N64PAD_SDL (a game's env): an SDL port that would read a Switch-mode N64 pad as a Pro
 * Controller (n64pad.h) gets HIDAPI off (all of it, not only the Switch driver) and NSO's N64 mapping
 * instead. The mapping is always applied (known GUIDs), so a missing Poll result cannot skip this. */
static const char *const n64SdlVars[5] = { "SDL_JOYSTICK_HIDAPI", "SDL_JOYSTICK_HIDAPI_SWITCH", "SDL_JOYSTICK_HIDAPI_JOY_CONS", "SDL_GAMECONTROLLERCONFIG", "SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS" };
static char *n64SdlSaved[5];
static bool n64SdlSet = false;

static void N64SdlEnvSet(void)
{
    static char config[4096];
    char merged[5120];
    const char *old;
    n64SdlSet = false;
    if (N64Pad_SdlConfig(config, sizeof(config)) <= 0) return;
    for (int i = 0; i < 5; i++) {
        old = getenv(n64SdlVars[i]);
        n64SdlSaved[i] = old ? strdup(old) : NULL;
    }
    old = n64SdlSaved[3];
    snprintf(merged, sizeof(merged), "%s%s%s", old ? old : "", old && old[0] ? "\n" : "", config);
    NsoSetEnv(n64SdlVars[0], "0");
    NsoSetEnv(n64SdlVars[1], "0");
    NsoSetEnv(n64SdlVars[2], "0");
    NsoSetEnv(n64SdlVars[3], merged);
    NsoSetEnv(n64SdlVars[4], "1");
    n64SdlSet = true;
}

static void N64SdlEnvRestore(void)
{
    if (!n64SdlSet) return;
    for (int i = 0; i < 5; i++) {
        NsoSetEnv(n64SdlVars[i], n64SdlSaved[i]);
        free(n64SdlSaved[i]);
        n64SdlSaved[i] = NULL;
    }
    n64SdlSet = false;
}

static void GameEnvSet(const GameEntry *game)
{
    gameEnvCount = 0;
    if (!game->env || !game->env[0]) return;
    if (strstr(game->env, "N64PAD_SDL")) N64SdlEnvSet();
    snprintf(gameEnvSpec, sizeof(gameEnvSpec), "%s", game->env);
    gameEnvCount = NsoSplitWords(gameEnvSpec, gameEnvWords, GAME_ENV_MAX);
    for (int i = 0; i < gameEnvCount; i++) {
        char *eq = strchr(gameEnvWords[i], '=');
        const char *old;
        if (!eq) { gameEnvSaved[i] = NULL; continue; }
        *eq = '\0';
        old = getenv(gameEnvWords[i]);
        gameEnvSaved[i] = old ? strdup(old) : NULL;
        if (eq[1] == '@') {
            char path[1200];
            snprintf(path, sizeof(path), "%s", GetDataPath(eq + 2));
            if (!DirectoryExists(path)) MakeDirectory(path);
            NsoSetEnv(gameEnvWords[i], path);
        } else {
            NsoSetEnv(gameEnvWords[i], eq + 1);
        }
    }
}

static void GameEnvRestore(void)
{
    N64SdlEnvRestore();
    for (int i = 0; i < gameEnvCount; i++) {
        NsoSetEnv(gameEnvWords[i], gameEnvSaved[i]);
        free(gameEnvSaved[i]);
        gameEnvSaved[i] = NULL;
    }
    gameEnvCount = 0;
}

/* Ports that read their controls from a file beside their executable get the custom controller
 * mapping (padmap.h) written into it before they start; the rest of the file is the port's own.
 *  - the NESRecomp ports (the -recomp folders of repos/nes): config.ini, player 1's
 *    [Keyboard.Player1] and [Gamepad.Player1] sections (SDL names);
 *  - the snesrecomp ports (the -recomp folders of repos/snes): keybinds.ini [player1], keyboard
 *    only (these ports have no pad mapping of their own);
 *  - zelda3 and sm: zelda3.ini / sm.ini, the Controls lines of [KeyMap] and [GamepadMap].
 * OpenMadoola and OpenDana keep their bindings in their own formats and are not touched. */
static void PadPort_Apply(const GameEntry *game, const char *gameDir) {
    static char oldText[65536], newText[70000];
    char path[2400], binDir[512], base[64], iniName[96];
    const char *slash, *name = NULL;
    int mode = 0;   /* 1 NES config.ini, 2 snesrecomp keybinds.ini, 3 zelda3 / sm ini */
    size_t n = 0;
    if (game->mednafen || game->embedRomKey || !game->dir || !game->binary) return;
    slash = strrchr(game->binary, '/');
    snprintf(base, sizeof(base), "%s", slash ? slash + 1 : game->binary);
    if (nesPad.custom && strstr(game->dir, "repos/nes/") && strstr(game->dir, "-recomp")) { mode = 1; name = "config.ini"; }
    else if (snesPad.custom && strstr(game->dir, "repos/snes/") && strstr(game->dir, "-recomp")) { mode = 2; name = "keybinds.ini"; }
    else if (snesPad.custom && strstr(game->dir, "repos/snes/") && (!strcmp(base, "zelda3") || !strcmp(base, "sm"))) {
        snprintf(iniName, sizeof(iniName), "%s.ini", base);
        mode = 3;
        name = iniName;
    }
    if (!mode) return;
    snprintf(binDir, sizeof(binDir), "%.*s", slash ? (int)(slash - game->binary) : 0, game->binary);
    snprintf(path, sizeof(path), "%s/%s%s%s", gameDir, binDir, binDir[0] ? "/" : "", name);
    {
        char dir[2400];
        snprintf(dir, sizeof(dir), "%s/%s", gameDir, binDir);
        if (!DirectoryExists(dir)) return;
    }
    FILE *f = fopen(path, "r");
    if (f) { n = fread(oldText, 1, sizeof(oldText) - 1, f); fclose(f); }
    oldText[n] = '\0';
    if (mode == 3 && !f) return;   /* the port's own ini ships with it: never make a bare one */
    if (!(mode == 1 ? PadMap_IniMerge(oldText, &nesPad, newText, sizeof(newText))
          : mode == 2 ? PadMap_SnesKeybinds(oldText, &snesPad, newText, sizeof(newText))
                      : PadMap_ZeldaMerge(oldText, &snesPad, newText, sizeof(newText)))) return;
    if (f && strcmp(oldText, newText) == 0) return;
    f = fopen(path, "w");
    if (!f) { fprintf(stderr, "NSO: cannot write %s\n", path); return; }
    if (fputs(newText, f) < 0 || fclose(f) != 0) fprintf(stderr, "NSO: cannot write %s\n", path);
}

NsoProc LaunchGame(const GameEntry *game) {
    if (game && game->mednafen && game->mednafen[0])
        NsoMednafenRecheck();
    if (!GameIsLaunchable(game)) {
        fprintf(stderr, "LaunchGame: game is display-only or invalid\n");
        return NsoProcNone();
    }
    if (game->mednafen && game->mednafen[0] && !NsoBundledMednafen()) {
        fprintf(stderr, "LaunchGame: bundled Mednafen is missing\n");
        return NsoProcNone();
    }

    /* Games with save states get NSO_STATE_DIR (where to write states) and,
     * when resuming, NSO_STATE_LOAD (state to load at start). */
    bool withStates = game->stateDir && game->stateDir[0] != '\0';
    bool withMenu = GameHasExternalMenu(game);
    char menuDir[1200] = { 0 };
    if (withMenu && GameMenuDirectory(menuDir, sizeof(menuDir), game)) {
        EnsureDirectoryTree(menuDir);
#if !defined(_WIN32)
        char windowPath[1240], actionPath[1240];
        snprintf(windowPath, sizeof(windowPath), "%s/nso-menu-window", menuDir);
        snprintf(actionPath, sizeof(actionPath), "%s/nso-menu-action", menuDir);
        remove(windowPath);
        remove(actionPath);
#endif
        NsoSetEnv("NSO_MENU_DIR", menuDir);
    }
    if (withStates) {
        char stateDir[1100];
        snprintf(stateDir, sizeof(stateDir), "%s", GetDataPath(game->stateDir));
        if (!DirectoryExists(stateDir) && MakeDirectory(stateDir) != 0) {
            fprintf(stderr, "LaunchGame: cannot create %s\n", stateDir);
        }
        NsoSetEnv("NSO_STATE_DIR", stateDir);
        NsoSetEnv("NSO_STATE_LOAD", stateLoadPath[0] ? stateLoadPath : NULL);
    }
#if !defined(_WIN32)
    if (withMenu) {
        char parentPid[32];
        snprintf(parentPid, sizeof(parentPid), "%ld", (long)getpid());
        NsoSetEnv("NSO_MENU_PARENT_PID", parentPid);
        void *nativeWindow = GetWindowHandle();
        if (nativeWindow) {
            char parentWindow[32];
            snprintf(parentWindow, sizeof(parentWindow), "%lu", *(unsigned long *)nativeWindow);
            NsoSetEnv("NSO_PARENT_WINDOW_ID", parentWindow);
        }
    }
#endif
    stateLoadPath[0] = '\0';

    /* Copy out of GetDataPath's static buffer before any further use. */
    char gameDir[2048];
    snprintf(gameDir, sizeof(gameDir), "%s", GetDataPath(game->dir));
    if (game->romImport) {
        NsoPrepareRomImport(gameDir, game->binary, game->romImport);
    }
    PadPort_Apply(game, gameDir);
    if (game->appDir) {
        char appDir[1200];
        snprintf(appDir, sizeof(appDir), "%s", GetDataPath(game->appDir));
        if (!DirectoryExists(appDir)) MakeDirectory(appDir);
        NsoSetEnv("APP_FOLDER_PATH", appDir);
    }
    CrtSessionEnd();   /* only a Mednafen game or a hooked port below can have one */
    NsoSetEnv("NSO_CRTGEOM_SHADER", NULL);
    NsoSetEnv("NSO_CRTGEOM_PARAMS", NULL);
    NsoSetEnv("NSO_CRTGEOM_SCAN", NULL);
    NsoSetEnv("NSO_CRTGEOM_PLAIN", NULL);
    char mednafenHome[1200] = { 0 }, mednafenStates[1240];
    char mednafenArgs[4608], mednafenRom[1200];
    char *mednafenArgv[72];
    char padArgs[3072], *padWords[2 * PAD_MAX_BTN];
    int padCount = 0;
    int mednafenArgc = 0;
    if (game->mednafen && game->rom) {
        /* Mednafen keeps its config and saves under saves/<module>/mednafen. */
        snprintf(mednafenHome, sizeof(mednafenHome), "%s", GetDataPath(TextFormat("saves/%s/mednafen", game->mednafen)));
        snprintf(mednafenStates, sizeof(mednafenStates), "%s/mcs", mednafenHome);
        EnsureDirectoryTree(mednafenStates);
        {
            char romRel[1200];
            const char *use = game->rom;
            if (Rom_ResolveRel(game->rom, romRel, sizeof(romRel))) use = romRel;
            use = Version_LaunchRom(currentSystem, game, use, romRel, sizeof(romRel));
            snprintf(mednafenRom, sizeof(mednafenRom), "%s", GetDataPath(use));
        }
        /* "Show controls in game": the caption bar is drawn over the game (rewind.h): at its start and
         * while rewinding. The window is always full size. */
        bool showBar = sysSettings[currentSystem].showControls;
        /* NES / SNES: the controller mapping of Settings > Controller (padmap.h), when the player made one */
        if (strcmp(game->mednafen, "nes") == 0)
            padCount = PadMap_MednafenWordsSys(&PADSYS_NES, &nesPad, padArgs, sizeof(padArgs), padWords, 2 * PAD_MAX_BTN);
        else if (strcmp(game->mednafen, "snes") == 0)
            padCount = PadMap_MednafenWordsSys(&PADSYS_SNES, &snesPad, padArgs, sizeof(padArgs), padWords, 2 * PAD_MAX_BTN);
        /* CRT Geometry: Mednafen's own OpenGL path runs shaders/crtgeom.glsl ("nsocrt" shader of the patched
         * Mednafen). Its beam scanlines follow the Scanlines toggle (NSO_CRTGEOM_SCAN). */
        char crtShader[1200];
        bool crt = sysSettings[currentSystem].crtgeom;
        snprintf(crtShader, sizeof(crtShader), "%s", GetDataPath("shaders/crtgeom.glsl"));
        if (crt && !FileExists(crtShader)) crt = false;
        NsoSetEnv("NSO_CRTGEOM_SHADER", crt ? crtShader : NULL);
        /* the fine settings: a file the patched Mednafen re-reads whenever it changes (the End menu's
         * CRT Geometry screen rewrites it while the game is paused) */
        char crtRun[1260];
        snprintf(crtRun, sizeof(crtRun), "%s/nso-crtgeom.params", mednafenHome);
        if (crt) CrtSessionBegin(game, crtRun, false);
        else CrtSessionEnd();
        NsoSetEnv("NSO_CRTGEOM_PARAMS", crt ? crtRun : NULL);
        NsoSetEnv("NSO_CRTGEOM_SCAN", sysSettings[currentSystem].scanlines ? "1" : "0");
        mednafenArgc = NsoMednafenArgvEx(game->mednafen, sysSettings[currentSystem].pixel,
                                         sysSettings[currentSystem].scanlines, crt, mednafenRom,
                                         GetScreenWidth(), GetScreenHeight(), mednafenArgs, sizeof(mednafenArgs), mednafenArgv,
                                         (int)(sizeof(mednafenArgv) / sizeof(mednafenArgv[0])), padWords, padCount);
        if (!mednafenArgc) {
            fprintf(stderr, "LaunchGame: Mednafen command line too long\n");
            return NsoProcNone();
        }
        NsoSetEnv("MEDNAFEN_HOME", mednafenHome);
        NsoSetEnv("NSO_EMU_STATE_DIR", mednafenStates);
        char rewindShm[1260];
        snprintf(rewindShm, sizeof(rewindShm), "%s/nso-rewind.shm", mednafenHome);
#if !defined(_WIN32)
        remove(rewindShm);   /* a new file, so nothing maps the last run's */
        Rewind_Begin(rewindShm, menuDir, sysSettings[currentSystem].pixel,
                     sysSettings[currentSystem].scanlines, crt, showBar);
#endif
        NsoSetEnv("NSO_REWIND_SHM", rewindShm);
    }
    if (game->crtHook && !game->mednafen && sysSettings[currentSystem].crtgeom) {
        /* A port built with tools/soh-nsocrt.patch runs the shader in its own renderer; its frame for the
         * End menu is plain and NSO filters that one (CrtSession.rawFrames). */
        char hookShader[1200], hookBin[2400], hookRun[1260];
        snprintf(hookShader, sizeof(hookShader), "%s", GetDataPath("shaders/crtgeom.glsl"));
        snprintf(hookBin, sizeof(hookBin), "%s/%s", gameDir, game->binary);
        snprintf(hookRun, sizeof(hookRun), "%s/nso-crtgeom.params", menuDir);
        if (menuDir[0] && FileExists(hookShader) && CrtHookBuilt(hookBin)) {
            if (GameUsesMenuLauncher(game)) {
                /* no bridge of its own: NSO's menu launcher asks the game for plain frames before stopping it */
                char plainFlag[1260], plainAck[1270];
                snprintf(plainFlag, sizeof(plainFlag), "%s/nso-crt-plain", menuDir);
                snprintf(plainAck, sizeof(plainAck), "%s.ack", plainFlag);
                remove(plainFlag);
                remove(plainAck);
                NsoSetEnv("NSO_CRTGEOM_PLAIN", plainFlag);
            }
            NsoSetEnv("NSO_CRTGEOM_SHADER", hookShader);
            NsoSetEnv("NSO_CRTGEOM_SCAN", "1");   /* N64: always with its beam lines (no Scanlines toggle) */
            NsoSetEnv("NSO_CRTGEOM_PARAMS", hookRun);
            CrtSessionBegin(game, hookRun, true);
        }
    }
    GameEnvSet(game);
#if !defined(_WIN32)
    /* Rumble off: the game's SDL rumble calls do nothing (src/norumble.c). */
    char *preloadSaved = getenv("LD_PRELOAD") ? strdup(getenv("LD_PRELOAD")) : NULL;
    if (!sysSettings[currentSystem].rumble) {
        char shim[2400], preload[3800];
        snprintf(shim, sizeof(shim), "%s", GetDataPath("nso-norumble.so"));
        if (!FileExists(shim))
            snprintf(shim, sizeof(shim), "%s/nso-norumble.so", NsoExeDir());
        if (FileExists(shim)) {
            snprintf(preload, sizeof(preload), "%s%s%s", shim, preloadSaved ? ":" : "", preloadSaved ? preloadSaved : "");
            NsoSetEnv("LD_PRELOAD", preload);
        }
    }
#endif
    /* argv: [nso --menu-launcher] binary [GameEntry.args...] */
    char argsBuf[512];
    char *argv[64];
    int argc = 0;
    snprintf(argsBuf, sizeof(argsBuf), "%s", game->args ? game->args : "");
    NsoProc proc;
#if !defined(_WIN32)
    char self[4096];
    ssize_t selfLen = 0;
    if (GameUsesMenuLauncher(game)) {
        selfLen = readlink("/proc/self/exe", self, sizeof(self) - 1);
        if (selfLen > 0) {
            self[selfLen] = '\0';
            /* A rebuilt/updated nso replaces the file under a running one: the kernel then
             * reports "<path> (deleted)", which cannot be exec'd, so no external game
             * (every Mednafen NES/SNES game, every port) would open until a restart. The
             * path itself is the new binary. */
            static const char deleted[] = " (deleted)";
            if ((size_t)selfLen > sizeof(deleted) - 1
                && strcmp(self + selfLen - (sizeof(deleted) - 1), deleted) == 0)
                self[selfLen - (sizeof(deleted) - 1)] = '\0';
            argv[argc++] = self;
            argv[argc++] = (char *)"--menu-launcher";
            NsoSetEnv("NSO_START_KEYS", game->startKeys);
        } else {
            perror("readlink /proc/self/exe failed");
        }
    }
#endif
    if (game->mednafen && game->mednafen[0] && NsoBundledMednafen())
        argv[argc++] = (char *)NsoBundledMednafen();
    else
        argv[argc++] = (char *)game->binary;
    argc += NsoSplitWords(argsBuf, argv + argc, (int)(sizeof(argv) / sizeof(argv[0])) - argc - 1);
    for (int i = 0; i < mednafenArgc && argc < (int)(sizeof(argv) / sizeof(argv[0])) - 1; i++)
        argv[argc++] = mednafenArgv[i];
    argv[argc] = NULL;
    {
        char spawnDir[2048];
        snprintf(spawnDir, sizeof(spawnDir), "%s", gameDir);
        if (game->mednafen && game->mednafen[0])
            NsoMednafenDir(spawnDir, sizeof(spawnDir));
        proc = NsoSpawnArgv(spawnDir, argv);
    }
#if !defined(_WIN32)
    if (selfLen > 0) NsoSetEnv("NSO_START_KEYS", NULL);
#endif
    GameEnvRestore();
#if !defined(_WIN32)
    NsoSetEnv("LD_PRELOAD", preloadSaved);
    free(preloadSaved);
#endif

    if (game->appDir) NsoSetEnv("APP_FOLDER_PATH", NULL);
    if (mednafenHome[0]) {
        NsoSetEnv("MEDNAFEN_HOME", NULL);
        NsoSetEnv("NSO_EMU_STATE_DIR", NULL);
        NsoSetEnv("NSO_REWIND_SHM", NULL);
    }
    if (withStates) {
        NsoSetEnv("NSO_STATE_DIR", NULL);
        NsoSetEnv("NSO_STATE_LOAD", NULL);
    }
#if !defined(_WIN32)
    if (withMenu) {
        NsoSetEnv("NSO_MENU_PARENT_PID", NULL);
        NsoSetEnv("NSO_PARENT_WINDOW_ID", NULL);
        NsoSetEnv("NSO_MENU_DIR", NULL);
    }
#endif
    return proc;
}

/* Embedded card: module = dir/binary, game data (SMB: the ROM, SMW: the port's
 * directory) = config/nso.cfg <embedRomKey> (NSO-relative or absolute; never
 * copied). */
static bool StartEmbeddedGame(const GameEntry *game) {
    char cfg[2112], text[4096], rom[1024], module[2304], romPath[2048];
    size_t n = 0;
    Lang_ConfigPath(cfg, sizeof(cfg));
    FILE *f = fopen(cfg, "r");
    if (f) { n = fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[n] = '\0';
    if (!Cfg_Get(text, game->embedRomKey, rom, sizeof(rom)) || !rom[0]) { stateLoadPath[0] = '\0'; return false; }
    snprintf(romPath, sizeof(romPath), "%s", rom[0] == '/' ? rom : GetDataPath(rom));
    snprintf(module, sizeof(module), "%s/%s", GetDataPath(game->dir), game->binary);
    Embed_Stop();   /* relaunched before the back transition ended */
    if (GameExitActive()) GameExit_Reset();
    if (sysSettings[currentSystem].crtgeom && CrtGeom_Ready()) CrtSessionBegin(game, NULL, false);
    else CrtSessionEnd();
    Embed_SetPadMap(SysIsNes(currentSystem) && nesPad.custom ? &nesPad : SysIsSnes(currentSystem) && snesPad.custom ? &snesPad
                    : SysIsN64(currentSystem) && n64Pad.custom ? &n64Pad : NULL,
                    SysIsN64(currentSystem) ? 2 : SysIsSnes(currentSystem) ? 1 : 0);
    /* Rumble: NSO plays the module's motor (n64pad.h); AeroGauge drives its own pad. */
    N64Pad_SetEnabled(sysSettings[currentSystem].rumble);
    NsoSetEnv("AERO_RUMBLE", sysSettings[currentSystem].rumble ? "1" : "0");
    bool ok = Embed_Start(module, romPath, game->stateDir ? GetDataPath(game->stateDir) : NULL, stateLoadPath);
    stateLoadPath[0] = '\0';
    return ok;
}

static void SelectSystem(int direction) {
    int next = currentSystem + direction;
    if (next >= 0 && next < SYSTEM_COUNT) currentSystem = next;
}

/* Unload large frontend images once a game is running, keeping the current
 * cover so the return-card animation still has art. Reload starts the moment
 * the user presses the button to leave, during the exit animation: the
 * visible screen first, then the first and last screens, then the middle. */
static bool frontendArtsParked = false;
static int frontendKeepCover = -1;
static bool menuArtsParked = false;

/* System-card textures only. InitSystems() also resets WikiLive and re-walks
 * every scraped cover, then Version_Rebuild, for all systems; after the first
 * boot Version_Refresh is live, so that blocks the main thread (Mega Drive
 * is the largest). Platform unload did not drop that catalog state. */
static void FrontendArts_ReloadPlatforms(void)
{
    for (int i = 0; i < SYSTEM_COUNT; i++)
        Nso_PlatformInit(systems[i].platform);
}

static void FrontendArts_Unload(int keepIdx)
{
    CoverLoader_Stop();
    for (int i = 0; i < MAX_GAMES; i++) {
        if (i != keepIdx) UnloadBoxart(i);
    }
    UnloadSystems();
    menuArtsParked = true;
    PadNotice_Unload();
    RetroShots_Unload();
    SortPanel_Unload();
    SidebarIcons_Unload();
    UnloadBlurResources();
    frontendArtsParked = true;
    frontendKeepCover = keepIdx;
    CoverReload_Stamp("unload");
}

static void FrontendArts_Reload(void)
{
    if (!frontendArtsParked) return;
    /* System-card art stays parked while the games grid is showing; it is
     * reloaded only when returning to the main menu (MenuArts_Reload). */
    coverReloadVisibleLogged = false;
    MosaicBlur_HoldEnd();
    mosaicRevealWait = true;
    CoverReload_Stamp("reveal-wait");
    CoverLoader_Start();
    frontendArtsParked = false;
    frontendKeepCover = -1;
    CoverReload_Stamp("reload-start");
}

/* Main-menu system-card textures (PlatformEntry.texture). Unloaded once the
 * enter-a-system animation has finished and the games grid is showing. */
static void MenuArts_Unload(void)
{
    if (menuArtsParked) return;
    UnloadSystems();
    menuArtsParked = true;
}

static void MenuArts_Reload(void)
{
    if (!menuArtsParked) return;
    FrontendArts_ReloadPlatforms();
    menuArtsParked = false;
}

static bool MenuArts_Ready(void)
{
    return !menuArtsParked;
}

static void GameExit_Reset(void)
{
    gameExitPhase = GAME_EXIT_NONE;
    gameExitT = 0.0f;
    gameExitCloseStarted = false;
    gameExitHandoff = false;
    Embed_ExitOverlayDestroy();
}

static void GameExit_Begin(void)
{
    gameExitPhase = GAME_EXIT_FADE;
    gameExitT = 0.0f;
    gameExitCloseStarted = false;
    gameExitHandoff = false;
    /* Start the visible-screen reload during the fade so the grid is ready
     * before the black rectangle shrinks away. */
    FrontendArts_Reload();
    Embed_ExitOverlaySet(0.0f);
}

static bool GameExit_GameGone(void)
{
    if (liveParking && !live.parked && !gameExitCloseStarted) return false;
    if (Embed_IsExternal()) return Embed_ExternalChildGone();
    if (Embed_IsActive()) return Embed_IsParked();
    return true;
}

static void GameExit_Step(float dt)
{
    if (gameExitPhase == GAME_EXIT_FADE) {
        gameExitT += dt / GAME_FADE_SEC;
        if (gameExitT >= 1.0f) {
            gameExitT = 1.0f;
            gameExitPhase = GAME_EXIT_CLOSE;
        }
        Embed_ExitOverlaySet(gameExitT);
    }
    if (gameExitPhase == GAME_EXIT_CLOSE) {
        Embed_ExitOverlaySet(1.0f);
        if (!gameExitCloseStarted) {
            gameExitCloseStarted = true;
            /* Only now, with the screen fully covered, hide the child and park. */
            if (Embed_IsExternal()) {
                Embed_ExternalSetChildVisible(false);
            } else if (live.external && NsoProcOk(live.pid)) {
#ifndef _WIN32
                /* Pause via the launcher (SIGUSR1). Skip if the End menu already did. */
                kill(live.pid, SIGUSR1);
#endif
            }
            if (live.window) Embed_UnmapWindow(live.window);
            if (liveParking) Live_FinishPark();
            else Embed_Stop();
            currentState = MOSAIC;
        }
        if (GameExit_GameGone() && CoverLoader_VisibleReady())
            gameExitHandoff = true;
    } else if (gameExitPhase == GAME_EXIT_REVEAL) {
        gameExitT += dt / GAME_COVER_SEC;
        if (gameExitT >= 1.0f) GameExit_Reset();
    }
}

static void GameEnter_Reset(void)
{
    gameEnterPhase = GAME_ENTER_NONE;
    gameEnterT = 0.0f;
    gameEnterStarted = false;
    gameEnterHadFrame = false;
    gameEnterHandoff = false;
    gameEnterFailed = false;
    Embed_ExitOverlayDestroy();
}

static void GameEnter_Begin(void)
{
    gameEnterPhase = GAME_ENTER_GROW;
    gameEnterT = 0.0f;
    gameEnterStarted = false;
    gameEnterHadFrame = false;
    gameEnterHandoff = false;
    gameEnterFailed = false;
}

static unsigned long GameEnter_PollChildWindow(void)
{
    char menuDir[1200], windowPath[1240];
    unsigned long w = 0;
    const GameEntry *g;
    if (live.window) return live.window;
    if (launchingGameIdx < 0 || launchingGameIdx >= ActiveGameCount()) return 0;
    g = &ActiveGames()[launchingGameIdx];
    if (!GameMenuDirectory(menuDir, sizeof(menuDir), g)) return 0;
    snprintf(windowPath, sizeof(windowPath), "%s/nso-menu-window", menuDir);
    FILE *f = fopen(windowPath, "rb");
    if (!f) return 0;
    if (fscanf(f, "%lu", &w) != 1) w = 0;
    fclose(f);
    return w;
}

static bool GameEnter_FirstFrameReady(void)
{
    unsigned long w;
    if (currentState == EMBED && Embed_IsActive() && !Embed_IsParked())
        return gameEnterHadFrame;
    w = GameEnter_PollChildWindow();
    return w != 0 && Embed_WindowIsViewable(w);
}

static void GameEnter_StartGame(void)
{
    const GameEntry *launched;
    if (launchingGameIdx < 0 || launchingGameIdx >= ActiveGameCount()) {
        gameEnterFailed = true;
        currentState = MOSAIC;
        return;
    }
    launched = &ActiveGames()[launchingGameIdx];
    if (Live_IsSame(currentSystem, launchingGameIdx)) {
        Live_Resume();
        liveShowAfterFrame = false;
        FrontendArts_Unload(launchingGameIdx);
        if (live.window) {
            Embed_ExitOverlaySet(1.0f);
            Embed_MapExternalWindow(live.window);
            Embed_ExitOverlaySet(1.0f);
        }
        return;
    }
    /* Cover first, then kill the parked game while it stays unmapped/stopped. */
    Embed_ExitOverlaySet(1.0f);
    if (live.window) Embed_UnmapWindow(live.window);
    if (live.parked) Live_CloseHidden();
    if (launched->embedRomKey) {
        if (StartEmbeddedGame(launched)) {
            currentState = EMBED;
            FrontendArts_Unload(launchingGameIdx);
        } else {
            gameEnterFailed = true;
            currentState = MOSAIC;
        }
        return;
    }
    /* Child windows map on top of NSO: raise the overlay first. */
    Embed_ExitOverlaySet(1.0f);
    if (NsoProcOk(gamePid = LaunchGame(launched))) {
        waitingForGame = true;
        externalQuitting = false;
        FrontendArts_Unload(launchingGameIdx);
    } else {
        gameEnterFailed = true;
        currentState = MOSAIC;
        Embed_ExitOverlayDestroy();
        if (launched->mednafen && launched->mednafen[0] && !NsoBundledMednafen())
            LaunchMsg_Show(S_NO_MEDNAFEN);
    }
}

static void GameEnter_Step(float dt)
{
    if (gameEnterPhase == GAME_ENTER_GROW) {
        gameEnterT += dt / GAME_COVER_SEC;
        if (gameEnterT >= 1.0f) {
            gameEnterT = 1.0f;
            gameEnterPhase = GAME_ENTER_START;
        }
    }
    if (gameEnterPhase == GAME_ENTER_START) {
        if (!gameEnterStarted) {
            gameEnterStarted = true;
            GameEnter_StartGame();
            if (gameEnterFailed) {
                gameEnterPhase = GAME_ENTER_FADE;
                gameEnterT = 0.0f;
            }
        }
        if (currentState == EMBED && Embed_IsActive()) {
            if (Embed_Update(dt)) gameEnterHadFrame = true;
        }
        if (!gameEnterFailed) {
            if (GameEnter_FirstFrameReady()) gameEnterHandoff = true;
            Embed_ExitOverlaySet(1.0f);
        }
    } else if (gameEnterPhase == GAME_ENTER_FADE) {
        gameEnterT += dt / GAME_FADE_SEC;
        if (gameEnterT > 1.0f) gameEnterT = 1.0f;
        Embed_ExitOverlaySet(1.0f - gameEnterT);
        if (gameEnterT >= 1.0f) {
            unsigned long w = gameEnterFailed ? 0 : GameEnter_PollChildWindow();
            bool failed = gameEnterFailed;
            GameEnter_Reset();
            isLaunching = false;
            if (failed) {
                currentState = MOSAIC;
                BeginMosaicEnterIntro();
            } else {
                if (currentState != EMBED) currentState = MOSAIC;
                if (w) Embed_FocusWindow(w);
            }
        }
    }
}

static void BeginMosaicEnterIntro(void) {
    /* Leave-game / process-exit / park all come through here: start the
     * visible-screen reload during the exit animation, so the menu is
     * usable at once. No-op if arts were never parked (splash path). */
    FrontendArts_Reload();
    MenuArts_Unload();
    for (int i = 0; i < MAX_GAMES; i++) {
        animScales[i] = 1.0f;
    }
    mosaicIntroPhase = MOSAIC_INTRO_DELAY;
    mosaicIntroTimer = MOSAIC_GROW_DELAY_SEC;
    InfoBar_Reset(); /* mosaic bar opens after grow */
}

static void StartEnterMosaic(void) {
    if (screenSliding || scrapeBusy) { homeEnterGame = -1; return; }

    Hide_Resync(false);   /* games installed since the last visit */
    if (ActiveGameTotal() <= 0) { homeEnterGame = -1; return; }   /* nothing playable still opens the (empty) grid */

    if (homeEnterGame >= 0 && homeEnterGame < ActiveGameCount()) {
        GameIdx = homeEnterGame;
        GridLayout enterGrid = GetGridLayout();
        viewportY = GridClampViewportY(&enterGrid, homeEnterViewY, ActiveGameCount());
        homeEnterGame = -1;
    } else {
        GameIdx = 0;
        viewportY = 0.0f;
        homeEnterGame = -1;
    }
    targetViewportY = viewportY;
    viewportSlideFromY = viewportY;
    viewportSlideT = 1.0f;

    /* Covers load during the splash (UpdateSplash); start from a clean set. */
    UnloadAllBoxarts();
    MosaicBlur_HoldEnd();
    mosaicRevealWait = true;
    CoverReload_Stamp("reveal-wait");
    CoverLoader_Start();

    /* Rail visible under system art as the mosaic is revealed. */
    sidebarOpen = false;
    sidebarWidth = SidebarRailWidth();
    targetSidebarWidth = sidebarWidth;
    gearSpinAngle = 0.0f;

    /* Hide info bars under the slide; intro starts when slide completes. */
    InfoBar_Reset();
    for (int i = 0; i < MAX_GAMES; i++) {
        animScales[i] = 1.0f;
    }
    mosaicIntroPhase = MOSAIC_INTRO_NONE;

    menuBarFocus = -1;
    powerDialogOpen = false;
    /* Splash first: the enter transition over the menu, then the system's own art. */
    currentState = SPLASH;
    splashTimer = 0.0f;
    enterT = 0.0f;   /* the A press: enteranim.h runs from here */
    enterJinglePending = true;
    splashJingle = -1;
}

/* Full-screen system art (letterboxed to fit, like the original menu). */
static void DrawSystemArtTint(float offsetX, Color tint) {
    Texture2D tex = ActiveSystem()->platform->texture;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    if (tex.id == 0) {
        /* No system art (user-supplied, not in the repo): console name only. */
        const char *name = ActiveSystem()->platform->name;
        if (!name || slotFont.texture.id == 0) return;
        float fs = sh * 0.08f;
        Vector2 m = MeasureTextEx(slotFont, name, fs, 2.0f);
        DrawTextEx(slotFont, name, (Vector2){ offsetX + (sw - m.x) * 0.5f, (sh - m.y) * 0.5f },
                   fs, 2.0f, LIGHTGRAY);
        return;
    }
    float scale = fminf(sw / (float)tex.width, sh / (float)tex.height);
    float w = (float)tex.width * scale;
    float h = (float)tex.height * scale;
    DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                   (Rectangle){ offsetX + (sw - w) * 0.5f, (sh - h) * 0.5f, w, h },
                   (Vector2){ 0 }, 0.0f, tint);
}

static void DrawSystemArt(float offsetX) { DrawSystemArtTint(offsetX, WHITE); }

/* White logo of the enter transition: a Nerd Font glyph's bitmap (the Nintendo wordmark) drawn
 * with its centre at (cx, cy), width w, rotated by deg. */
static void DrawEnterLogoGlyph(float cx, float cy, float w, float deg, float alpha) {
    if (!enterLogoOk) return;
    int gi = GetGlyphIndex(enterLogoFont, ENTER_LOGO_CP);
    Rectangle src = enterLogoFont.recs[gi];
    float h = w * src.height / src.width;
    DrawTexturePro(enterLogoFont.texture, src, (Rectangle){ cx, cy, w, h }, (Vector2){ w * 0.5f, h * 0.5f },
                   deg, Fade(WHITE, alpha));
}

/* System name, right-aligned (right edge x, vertical centre cy). Every letter flips like a card
 * about its own horizontal axis, each a little out of phase (EnterAnim_LetterAngle): height
 * scales by cos(angle), with a little perspective (the half turning towards the viewer grows
 * slightly wider). All letters are face-on while the word holds. */
static void DrawEnterPortsText(float x, float cy, float size, float t, float alpha) {
    if (!enterTextOk) return;
    const char *text = ActiveSystem()->platform && ActiveSystem()->platform->name
        ? ActiveSystem()->platform->name : "NSO";
    const int n = (int)strlen(text);
    const float spacing = size * 0.02f;
    Vector2 whole = MeasureTextEx(enterTextFont, text, size, spacing);
    Color c = Fade(WHITE, alpha);
    for (int i = 0; i < n; i++) {
        if (text[i] == ' ') continue;
        char prefix[48] = { 0 }, one[2] = { text[i], 0 };
        memcpy(prefix, text, (size_t)i);
        float ox = i ? MeasureTextEx(enterTextFont, prefix, size, spacing).x : 0.0f;
        float lw = MeasureTextEx(enterTextFont, one, size, 0.0f).x;
        float rad = EnterAnim_LetterAngle(t, i, n) * (float)PI / 180.0f;
        float sy = cosf(rad), sx = 1.0f + 0.10f * fabsf(sinf(rad));
        if (fabsf(sy) < 0.01f) continue;   /* edge-on */
        rlPushMatrix();
        rlTranslatef(x - whole.x + ox + lw * 0.5f, cy, 0.0f);
        rlScalef(sx, sy, 1.0f);
        for (int pass = 0; pass < 2; pass++)   /* a second pass a hair to the right: bolder strokes */
            DrawTextEx(enterTextFont, one, (Vector2){ -lw * 0.5f + (float)pass * size * 0.02f, -whole.y * 0.5f },
                       size, 0.0f, c);
        rlPopMatrix();
    }
}

/* Pressing A on a system tile (enteranim.h): the tile's art lifts off as a fading ghost while a
 * (17,17,17) layer rises over the menu; the layer holds with the white logos (Nintendo wordmark
 * top-left, the system name bottom-right, flipping in); then the system's own screen crossfades in. */
static void DrawEnterTransition(void) {
    float sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
    float cover = EnterAnim_Cover(enterT), cross = EnterAnim_Cross(enterT);
    Color layer = { ENTER_COVER_R, ENTER_COVER_G, ENTER_COVER_B, 255 };
    if (cover < 1.0f) {
        DrawSystemMenu(0.0f);
        float r = 0.0f;
        Rectangle tile = SystemTileRect(currentSystem, &r);
        const PlatformEntry *p = ActiveSystem()->platform;
        float ga = EnterAnim_GhostAlpha(enterT), k = 1.0f + 0.5f * EnterAnim_GhostProgress(enterT);
        if (ga > 0.0f) {
            Rectangle g = { tile.x - tile.width * (k - 1.0f) * 0.5f, tile.y - tile.height * (k - 1.0f) * 0.5f,
                            tile.width * k, tile.height * k };
            Color bg = p->texture.id != 0 ? p->bg : GetColor(0x3A3A3AFF);
            for (int i = 3; i >= 1; i--) {   /* soft halo in the tile's colour */
                float grow = tile.width * 0.03f * (float)i;
                DrawRectangleRounded((Rectangle){ g.x - grow, g.y - grow, g.width + 2.0f * grow, g.height + 2.0f * grow },
                                     MENU_CARD_ROUNDNESS, 16, Fade(bg, ga * 0.12f));
            }
            DrawRectangleRounded(g, MENU_CARD_ROUNDNESS, 16, Fade(bg, ga));
            if (p->texture.id != 0)
                DrawSystemCardArt(p, g, MENU_CARD_ROUNDNESS * g.width, Fade(WHITE, ga));
        }
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(layer, cover));
    } else {
        ClearBackground(BLACK);
        DrawSystemArt(0.0f);                                   /* the destination, as it always is */
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(layer, 1.0f - cross));
    }
    float la = EnterAnim_LogoAlpha(enterT);
    if (la > 0.0f) {   /* positions and sizes as in the reference, relative to the screen */
        float lw = sw * 0.120f;
        DrawEnterLogoGlyph(sw * 0.0229f + lw * 0.5f, sh * 0.0495f + lw * 0.25f * 0.5f, lw, 0.0f, la);   /* never rotates */
        DrawEnterPortsText(sw * 0.967f, sh * 0.920f, sh * 0.072f, enterT, la);
    }
}

/* After the fade + hold, slide the art away to reveal the game grid. */
static void UpdateSplash(float dt) {
    if (currentState != SPLASH) return;

    CoverLoader_Poll();
    bool loaded = CoverLoader_VisibleReady();

    float step = dt > 0.1f ? 0.1f : dt;   /* a hitch must not skip the sequence */
    const char *fixed = getenv("NSO_ENTER_STEP");   /* debug: fixed step in seconds per frame (frame capture) */
    if (fixed && atof(fixed) > 0.0) step = (float)atof(fixed);
    /* Hold the gray logo layer (the loading screen) until visible covers are
     * ready; do not sit on the full-screen system art waiting for artboxes. */
    {
        float crossAt = EnterAnim_TimeOfFrame(176.0f);
        if (!loaded && enterT < crossAt && enterT + step >= crossAt)
            step = crossAt - enterT;
        if (!loaded && enterT >= crossAt - 0.0001f && enterT < EnterAnim_Total())
            return;
    }
    if (enterT < EnterAnim_Total()) {   /* transition still running */
        enterT += step;
        if (enterT >= EnterAnim_Total() && enterJinglePending) {   /* destination at 100%, no overlay */
            enterJinglePending = false;
            int jingle = SystemSelectJingle(ActiveSystem()->platform);
            if (jingle >= 0 && Sfx_Has((SfxId)jingle)) { Sfx_Play((SfxId)jingle); splashJingle = jingle; }
        }
        return;
    }
    splashTimer += dt;
    if (EnterAnim_SlideReady(loaded, splashTimer, splashJingle >= 0,
                             splashJingle >= 0 && Sfx_Playing((SfxId)splashJingle))) {
        screenSliding = true;
        slideDirection = -1;
        slideOffset = 0.0f;
    }
}

/* Selected cover at scale, as DrawSlotImages places it. */
static Rectangle SelectedCardRect(float scale) {
    GridLayout g = GetGridLayout();
    if (g.rowCount < 1 || GameIdx < 0 || GameIdx >= ActiveGameCount()) return (Rectangle){ 0 };
    Rectangle item = g.items[GameIdx];
    Texture2D tex = (GameIdx < MAX_GAMES) ? gameBoxarts[GameIdx] : (Texture2D){ 0 };
    float x, y, w, h;
    CoverDestInSlot(item, tex, scale, viewportY, &x, &y, &w, &h);
    ClampTileToContent(&x, &y, w, h, g.originX, g.contentW, g.screenH, MOSAIC_CURSOR_PAD);
    return (Rectangle){ x, y, w, h };
}

static void DrawMosaicSlots(Color background, Font font) {
    ClearBackground(background);

    int count = ActiveGameCount();
    if (count > 0) {
        GridLayout drawGrid = GetGridLayout();
        Texture2D boxarts[MAX_GAMES];

        for (int i = 0; i < count; i++) {
            boxarts[i] = gameBoxarts[i];
        }

        DrawSlotImages(&drawGrid, boxarts, count);
    } else if (ActiveGameTotal() > 0) {   /* a filter is on and nothing is visible */
        const char *msg = T(showHidden[currentSystem] ? S_NO_HIDDEN
                            : favOn[currentSystem] ? S_NO_FAVORITES : S_NO_PLAYABLE);
        float x0 = SidebarRailWidth(), w = (float)GetScreenWidth() - x0, size = (float)font.baseSize * 1.2f;
        float tw = fminf(MeasureTextEx(font, msg, size, 1.0f).x, w * 0.8f);
        DrawFitText(font, msg, (Vector2){ x0 + (w - tw) * 0.5f, (float)GetScreenHeight() * 0.5f }, size, w * 0.8f, WHITE);
    }
}

/* Snapshot the current (still-old) grid and hold a blur of it until the
 * rebuilt list's visible covers are ready. A second toggle keeps this frame
 * rather than recapturing a half-built list. */
static void MosaicBlur_HoldBegin(void)
{
    int sw, sh;
    if (mosaicBlurHeld && mosaicRT.id) return;
    sw = GetScreenWidth();
    sh = GetScreenHeight();
    if (sw < 1 || sh < 1) { mosaicBlurHeld = true; return; }
    EnsureBlurResources(sw, sh);
    if (mosaicRT.id) {
        BeginTextureMode(mosaicRT);
        /* Same grey as the mosaic (Mosaicbg in main). Font is only used when
         * a filter has emptied the grid; the pre-toggle list usually has tiles. */
        DrawMosaicSlots(GetColor(0x303030FF), slotFont);
        EndTextureMode();
    }
    mosaicBlurHeld = true;
}

static void DrawMosaic(Color background, Font font) {
    Color SystemBG = GetColor(0xEE0010FF);

    Live_ConfirmStep();
    float overlayT = fmaxf(fmaxf(detailsT, statusT), liveConfirmT);
    if (SidebarWantsBlur() || overlayT > 0.0f || mosaicBlurHeld) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        EnsureBlurResources(sw, sh);

        if (mosaicRT.id) {
            if (!mosaicBlurHeld) {
                BeginTextureMode(mosaicRT);
                DrawMosaicSlots(background, font);
                EndTextureMode();
            }
            /* Side menu (every item): twice the blur radius, same passes and taps.
             * Details / status keep the standard blur. A hide/filter re-list
             * keeps that same heavy blur on the frozen pre-toggle snapshot. */
            DrawBlurredMosaicBackdrop(background, (SidebarWantsBlur() || mosaicBlurHeld) ? 2.0f : 1.0f);
            /* Details: the sharp grid dissolves into the blur in step with the card. */
            if (!SidebarWantsBlur() && !mosaicBlurHeld && overlayT < 1.0f)
                DrawRT(mosaicRT.texture, (Rectangle){ 0, 0, (float)sw, (float)sh },
                       Fade(WHITE, 1.0f - overlayT));
        } else {
            DrawMosaicSlots(background, font);
        }
        DrawSidebar(SystemBG, font);
    } else {
        DrawMosaicSlots(background, font);
        DrawSidebar(SystemBG, font);
    }

    SortPanel_Draw(font);

    if (liveConfirmT > 0.0f) {
        /* Close-the-running-game question: over the blur, with its own A / B hint. */
        Live_ConfirmDraw(font);
        DrawInfoBarEx(font, T(S_SIDEBAR_INFO), liveConfirmT, (1.0f - liveConfirmT) * INFO_BAR_SLIDE_PX);
        return;
    }
    /* Above blur / panel so accept/back help stays readable.
     * While the status card is up, Status_Draw draws the bars after its dim. */
    if (statusT <= 0.0f) InfoBar_Draw(font);
    LaunchMsg_Draw(font);
}

static void SlideEffect(Color mosaicBg, Font font)
{
    if (slideDirection > 0) {
        /* Leaving: a (17,17,17) layer fades over the games grid (faster than
         * the enter dim), holds fully opaque until every main-menu system card
         * is loaded, then fades out over ENTER_FADE_SEC along the same curve. */
        float step = GetFrameTime() > 0.1f ? 0.1f : GetFrameTime();
        const char *fixed = getenv("NSO_ENTER_STEP");   /* debug: fixed step per frame (frame capture) */
        if (fixed && atof(fixed) > 0.0) step = (float)atof(fixed);
        Color layer = { ENTER_COVER_R, ENTER_COVER_G, ENTER_COVER_B, 255 };
        float cover;
        leaveT += step;
        if (leaveFadeInT < 0.0f) {
            float u = leaveT / LEAVE_FADE_OUT_SEC;
            if (u < 1.0f) {
                if (u < 0.0f) u = 0.0f;
                /* Same dim curve as the original exit, compressed into the shorter fade-out. */
                cover = EnterAnim_ExitCover(u * ENTER_FADE_SEC);
            } else {
                cover = 1.0f;
                if (!leaveSwitched) {
                    leaveSwitched = true;
                    currentState = SYSTEMS;
                    menuBarFocus = -1;
                    powerDialogOpen = false;
                    sidebarExpandPending = false;
                    sidebarOpen = false;
                    sidebarClosing = false;
                    sbWidthPhase = SB_WIDTH_IDLE;
                    sidebarWidth = 0.0f;
                    targetSidebarWidth = 0.0f;
                    gearSpinAngle = 0.0f;
                    mosaicIntroPhase = MOSAIC_INTRO_NONE;
                    mosaicExitPhase = MOSAIC_EXIT_NONE;
                    InfoBar_Reset();
                    UnloadAllBoxarts();
                } else {
                    /* One full-black frame first, then load so a hitch stays hidden. */
                    MenuArts_Reload();
                    if (MenuArts_Ready()) leaveFadeInT = 0.0f;
                }
            }
        }
        if (leaveFadeInT >= 0.0f) {
            cover = EnterAnim_ExitCover(ENTER_FADE_SEC + leaveFadeInT);
            leaveFadeInT += step;
        }
        if (leaveSwitched && MenuArts_Ready()) DrawSystemMenu(0.0f);
        else if (!leaveSwitched) DrawMosaic(mosaicBg, font);
        else ClearBackground(layer);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(layer, cover));
        if (leaveFadeInT >= ENTER_FADE_SEC) {
            screenSliding = false;
            slideOffset = 0.0f;
        }
        return;
    }

    /* Mosaic underneath: entering slides the splash art away. */
    DrawMosaic(mosaicBg, font);
    DrawSystemArt(slideOffset);

    slideOffset += SLIDE_SPEED * (float)slideDirection;

    if (slideOffset <= -(float)GetScreenWidth()) {
        slideOffset = -(float)GetScreenWidth();
        screenSliding = false;
        currentState = MOSAIC;
        BeginMosaicEnterIntro();
    }
}

/* Start the leave fade-through only (after info out + shrink). */
static void BeginLeaveMosaicSlide(void) {
    sidebarExpandPending = false;
    sidebarOpen = false;
    sidebarClosing = false;
    sbWidthPhase = SB_WIDTH_IDLE;
    sbWidthRest = 0.0f;
    sbWidthPeak = 0.0f;
    targetSidebarWidth = 0.0f;
    gearSpinAngle = 0.0f;
    mosaicIntroPhase = MOSAIC_INTRO_NONE;
    mosaicExitPhase = MOSAIC_EXIT_NONE;

    for (int i = 0; i < MAX_GAMES; i++) {
        animScales[i] = 1.0f;
    }

    screenSliding = true;
    slideDirection = 1;
    slideOffset = -(float)GetScreenWidth();
    leaveT = 0.0f;
    leaveSwitched = false;
    leaveFadeInT = -1.0f;
}

/* Reverse of enter post-slide: info fade out → shrink → slide (no delay). */
static void RequestLeaveMosaic(void) {
    if (screenSliding || mosaicExitPhase != MOSAIC_EXIT_NONE) {
        return;
    }
    if (sidebarOpen || sidebarExpandPending || sidebarClosing
        || sbWidthPhase != SB_WIDTH_IDLE) {
        return;
    }

    mosaicIntroPhase = MOSAIC_INTRO_NONE;
    mosaicExitPhase = MOSAIC_EXIT_INFO;
    InfoBar_Request(INFO_BAR_NONE);
}

static bool Home_InGame(void)
{
    if (live.parked) return false;
    if (currentState == EMBED && Embed_IsActive() && !Embed_IsParked()) return true;
    return Live_Busy();
}

static bool Home_KeyPressed(void)
{
    /* KEY_END is the pause/End menu only (pre-0f2efa5). Never treat it as Home,
     * even if this frame also reports KEY_HOME or a leftover USR2. */
    if (IsKeyPressed(KEY_END)) {
#ifndef _WIN32
        externalHomeSignal = 0;
#endif
        return false;
    }
#ifndef _WIN32
    if (externalMenuSignal)
        return false;
    if (externalHomeSignal) {
        externalHomeSignal = 0;
        return true;
    }
#endif
    return IsKeyPressed(KEY_HOME);
}

/* Top-level system select (NSO's main menu), not a system's game grid. */
static bool Home_AtMainMenu(void)
{
    if (currentState != SYSTEMS) return false;
    if (Home_InGame() || GameExitActive() || GameEnterActive() || isLaunching) return false;
    if (screenSliding || homeResumePending) return false;
    return true;
}

static void Home_DismissOverlays(void)
{
    if (liveConfirm) {
        liveConfirm = 0;
        liveConfirmT = 0.0f;
    }
    if (gridLifted) {
        if (gridLiftMoved && gridLiftFrom >= 0 && gridLiftFrom < ActiveGameCount())
            Grid_MoveLifted(gridLiftFrom);
        gridLifted = false;
        gridLiftMoved = false;
    }
    if (padOpen) PadUi_Close();
    if (GameSel_Busy()) GameSel_Cancel();
    gsOpen = false;
    if (scrapeBusy) ScrapeMenu_Cancel();
    if (detailsOpen) Details_Close();
    if (statusOpen) Status_Close();
    if (SortPanel_Active()) SortPanel_Close();
    sidebarExpandPending = false;
    sidebarClosing = false;
    sidebarOpen = false;
    sidebarPanelFocus = false;
    sbWidthPhase = SB_WIDTH_IDLE;
    {
        float rail = SidebarRailWidth();
        sidebarWidth = rail;
        targetSidebarWidth = rail;
    }
    mosaicExitPhase = MOSAIC_EXIT_NONE;
    powerDialogOpen = false;
    if (currentState == MOSAIC) InfoBar_Request(INFO_BAR_MOSAIC);
}

static void Home_RememberGrid(void)
{
    if (currentState == MOSAIC || currentState == LAUNCHING || currentState == EMBED || Home_InGame()) {
        homeRetSys = currentSystem;
        homeRetGame = (Home_InGame() && launchingGameIdx >= 0) ? launchingGameIdx : GameIdx;
        homeRetViewY = viewportY;
    }
}

static void Home_SaveReturn(void)
{
    if (homeReturn != HOME_RET_NONE) return;
    Home_RememberGrid();
    if (Home_InGame()) {
        homeReturn = HOME_RET_GAME;
        return;
    }
    if (currentState == SYSTEMS) { homeReturn = HOME_RET_SYSTEMS; return; }
    if (detailsOpen) { homeReturn = HOME_RET_DETAILS; return; }
    if (statusOpen) { homeReturn = HOME_RET_STATUS; return; }
    if (SortPanel_Active()) { homeReturn = HOME_RET_SORT; return; }
    if (gridLifted) { homeReturn = HOME_RET_LIFT; return; }
    if (sidebarOpen || sidebarExpandPending || sidebarClosing || sidebarPanelFocus
        || padOpen || gsOpen || GameSel_Busy() || scrapeBusy) {
        homeReturn = HOME_RET_SIDEBAR;
        homeRetSidebarItem = sidebarItem;
        homeRetSetRow = sidebarSetRow;
        homeRetPanelFocus = sidebarPanelFocus;
        homeRetPad = padOpen;
        homeRetGameSel = gsOpen || GameSel_Busy();
        return;
    }
    if (currentState == MOSAIC)
        homeReturn = HOME_RET_MOSAIC;
}

static void Home_PrepareGrid(int sys, int game, float viewY)
{
    if (sys >= 0 && sys < SYSTEM_COUNT) currentSystem = sys;
    Hide_Resync(false);
    int n = ActiveGameCount();
    GameIdx = (game >= 0 && game < n) ? game : 0;
    launchingGameIdx = GameIdx;
    GridLayout g = GetGridLayout();
    viewportY = GridClampViewportY(&g, viewY, n);
    targetViewportY = viewportY;
    viewportSlideFromY = viewportY;
    viewportSlideT = 1.0f;
}

static void Home_UnloadGfx(void)
{
    if (homeUnderlay.id) { UnloadTexture(homeUnderlay); homeUnderlay = (Texture2D){ 0 }; }
    if (homeMenuRt.id) { UnloadRenderTexture(homeMenuRt); homeMenuRt = (RenderTexture2D){ 0 }; }
}

static void Home_CaptureUnderlay(void)
{
    Image img = { 0 };
#ifndef _WIN32
    unsigned long child = 0;
    if (Home_InGame()) {
        if (live.window) child = live.window;
        else if (Embed_IsExternal()) child = Embed_ExternalWindow();
    }
    if (child) {
        Display *dpy = XOpenDisplay(NULL);
        void *hostHandle = GetWindowHandle();
        Window host = hostHandle ? *(Window *)hostHandle : None;
        if (dpy && host != None) {
            XWindowAttributes ha;
            Window root = DefaultRootWindow(dpy), unused;
            int rx = 0, ry = 0;
            XErrorHandler prev = XSetErrorHandler(Embed_XErrorTrap);
            embedXErrorCode = 0;
            if (XGetWindowAttributes(dpy, host, &ha)
                && ha.width > 0 && ha.height > 0
                && XTranslateCoordinates(dpy, host, root, 0, 0, &rx, &ry, &unused)) {
                XImage *shot = XGetImage(dpy, root, rx, ry, (unsigned)ha.width, (unsigned)ha.height,
                                         AllPlanes, ZPixmap);
                XSync(dpy, False);
                if (shot && embedXErrorCode == 0) {
                    img = GenImageColor(ha.width, ha.height, BLACK);
                    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
                    if (img.data) {
                        unsigned char *px = (unsigned char *)img.data;
                        for (int y = 0; y < ha.height; y++) {
                            for (int x = 0; x < ha.width; x++) {
                                unsigned long p = XGetPixel(shot, x, y);
                                size_t i = ((size_t)y * (size_t)ha.width + (size_t)x) * 4;
                                px[i + 0] = Embed_XChannel(p, shot->red_mask);
                                px[i + 1] = Embed_XChannel(p, shot->green_mask);
                                px[i + 2] = Embed_XChannel(p, shot->blue_mask);
                                px[i + 3] = 255;
                            }
                        }
                    }
                    XDestroyImage(shot);
                } else if (shot) {
                    XDestroyImage(shot);
                }
            }
            XSetErrorHandler(prev);
            XCloseDisplay(dpy);
        }
    }
#endif
    if (!img.data) img = LoadImageFromScreen();
    Home_UnloadGfx();
    if (img.data) {
        homeUnderlay = LoadTextureFromImage(img);
        if (homeUnderlay.id)
            SetTextureFilter(homeUnderlay, TEXTURE_FILTER_BILINEAR);
        UnloadImage(img);
    }
}

static void Home_EnsureMenuRt(void)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (homeMenuRt.id && (homeMenuRt.texture.width != sw || homeMenuRt.texture.height != sh)) {
        UnloadRenderTexture(homeMenuRt);
        homeMenuRt = (RenderTexture2D){ 0 };
    }
    if (!homeMenuRt.id) homeMenuRt = LoadRenderTexture(sw, sh);
}

static void Home_DrawXFade(float menuAlpha)
{
    int sw = GetScreenWidth(), sh = GetScreenHeight();
    if (homeUnderlay.id) {
        DrawTexturePro(homeUnderlay,
                       (Rectangle){ 0, 0, (float)homeUnderlay.width, (float)homeUnderlay.height },
                       (Rectangle){ 0, 0, (float)sw, (float)sh },
                       (Vector2){ 0, 0 }, 0.0f, WHITE);
    } else {
        ClearBackground(MENU_BG);
    }
    if (menuAlpha <= 0.0f) return;
    Home_EnsureMenuRt();
    if (!homeMenuRt.id) {
        DrawSystemMenu(0.0f);
        return;
    }
    BeginTextureMode(homeMenuRt);
    DrawSystemMenu(0.0f);
    EndTextureMode();
    DrawTexturePro(homeMenuRt.texture,
                   (Rectangle){ 0, 0, (float)homeMenuRt.texture.width, -(float)homeMenuRt.texture.height },
                   (Rectangle){ 0, 0, (float)sw, (float)sh },
                   (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, menuAlpha));
}

static bool Home_XFadeActive(void) { return homeXFade != HOME_XFADE_NONE; }

static float Home_MenuAlpha(void)
{
    if (homeXFade == HOME_XFADE_IN) return homeXFadeT;
    if (homeXFade == HOME_XFADE_OUT) return 1.0f - homeXFadeT;
    return 1.0f;
}

static void Home_ParkAfterFrame(void)
{
    if (!homeUnmapAfterFrame) return;
    homeUnmapAfterFrame = false;
#ifndef _WIN32
    /* SIGUSR2 tells the launcher to stop the game only. SIGUSR1 is the
     * End key: pause and signal NSO to open the End menu. */
    if (live.external && NsoProcOk(live.pid)) kill(live.pid, SIGUSR2);
#endif
    if (live.window) Embed_UnmapWindow(live.window);
    if (Embed_IsExternal()) Embed_ExternalSetChildVisible(false);
    if (liveParking) Live_FinishPark();
}

static void Home_GoToMainMenu(void)
{
    Home_SaveReturn();
    if (currentState == SYSTEMS && !Home_InGame() && !Home_XFadeActive())
        return;
    Home_DismissOverlays();
    /* Snapshot the current screen (the live game, or mosaic/manage/End)
     * before anything is unmapped or redrawn. */
    Home_CaptureUnderlay();
    /* System tiles need their art; the games-grid covers do not — they are
     * loaded again when a system is entered (StartEnterMosaic). */
    MenuArts_Reload();
    if (frontendArtsParked) {
        frontendArtsParked = false;
        frontendKeepCover = -1;
    }
    CoverLoader_Stop();
    UnloadAllBoxarts();
    mosaicRevealWait = false;
    MosaicBlur_HoldEnd();
    if (Home_InGame()) {
        Live_BeginPark();
        homeUnmapAfterFrame = true; /* unmap only after the snapshot is on screen */
    }
    currentState = SYSTEMS;
    menuBarFocus = -1;
    powerDialogOpen = false;
    introOpacity = 0.0f;
    introFrames = FPS;
    mosaicIntroPhase = MOSAIC_INTRO_NONE;
    mosaicExitPhase = MOSAIC_EXIT_NONE;
    homeXFade = HOME_XFADE_IN;
    homeXFadeT = 0.0f;
}

static void Home_ApplyReturn(void)
{
    HomeReturn dest = homeReturn;
    homeReturn = HOME_RET_NONE;
    switch (dest) {
    case HOME_RET_SYSTEMS:
        break;
    case HOME_RET_MOSAIC:
    case HOME_RET_DETAILS:
    case HOME_RET_STATUS:
    case HOME_RET_SORT:
    case HOME_RET_LIFT:
    case HOME_RET_SIDEBAR:
        Home_PrepareGrid(homeRetSys, homeRetGame, homeRetViewY);
        if (frontendArtsParked) FrontendArts_Reload();
        MenuArts_Unload();
        mosaicIntroPhase = MOSAIC_INTRO_NONE;
        mosaicRevealWait = false;
        MosaicBlur_HoldEnd();
        currentState = MOSAIC;
        if (dest != HOME_RET_MOSAIC) homeReturn = dest;
        break;
    case HOME_RET_GAME:
        if (!live.parked) break;
        Home_PrepareGrid(live.sys, live.game, homeRetViewY);
        /* Resume behind the still-visible snapshot; no enter veil. */
        Live_Resume();
        homeResumeAfterFrame = true;
        break;
    default:
        break;
    }
}

static void Home_XFadeBeginOut(void)
{
    homeXFade = HOME_XFADE_OUT;
    homeXFadeT = 0.0f;
}

static void Home_XFadeStep(float dt)
{
    if (homeXFade == HOME_XFADE_IN) {
        homeXFadeT += dt / HOME_XFADE_SEC;
        if (homeXFadeT >= 1.0f) {
            homeXFadeT = 1.0f;
            homeXFade = HOME_XFADE_NONE;
            /* Keep the underlay: a later Home fades back to it. */
            if (homeMenuRt.id) {
                UnloadRenderTexture(homeMenuRt);
                homeMenuRt = (RenderTexture2D){ 0 };
            }
        }
    } else if (homeXFade == HOME_XFADE_OUT) {
        homeXFadeT += dt / HOME_XFADE_SEC;
        if (homeXFadeT >= 1.0f) {
            homeXFadeT = 1.0f;
            homeXFade = HOME_XFADE_NONE;
            /* Apply after this frame's snapshot has been swapped, so the
             * grid or the enter veil cannot flash over it. */
            homeApplyAfterFrame = true;
        }
    }
}

static void Home_ApplyDeferredOverlay(void)
{
    HomeReturn dest = homeReturn;
    if (dest != HOME_RET_DETAILS && dest != HOME_RET_STATUS && dest != HOME_RET_SORT
        && dest != HOME_RET_LIFT && dest != HOME_RET_SIDEBAR) return;
    if (currentState != MOSAIC || screenSliding || mosaicRevealWait) return;
    if (mosaicIntroPhase != MOSAIC_INTRO_NONE) return;
    if (Home_XFadeActive()) return;
    homeReturn = HOME_RET_NONE;
    switch (dest) {
    case HOME_RET_DETAILS: Details_Open(); break;
    case HOME_RET_STATUS: Status_Open(); break;
    case HOME_RET_SORT: SortPanel_Open(); break;
    case HOME_RET_LIFT:
        gridLifted = true;
        gridLiftFrom = GameIdx;
        gridLiftMoved = false;
        break;
    case HOME_RET_SIDEBAR:
        ExpandSidebarMenu();
        sidebarItem = homeRetSidebarItem;
        sidebarSetRow = homeRetSetRow;
        sidebarPanelFocus = homeRetPanelFocus;
        if (homeRetPad)
            PadUi_Open(PadUi_KindOf(currentSystem));
        if (homeRetGameSel) GameSel_Open();
        break;
    default: break;
    }
}

static void Home_Poll(void)
{
    if (Home_XFadeActive() || homeUnmapAfterFrame || homeResumeAfterFrame
        || homeApplyAfterFrame) return;
    if (!Home_KeyPressed()) return;
    if (GameExitActive() || GameEnterActive() || isLaunching || screenSliding) return;
    if (currentState == SPLASH) return;
    if (currentState != SYSTEMS && mosaicExitPhase != MOSAIC_EXIT_NONE) return;
    /* Home only with a game: in-game → systems menu; on the systems menu
     * with a game parked by Home → toggle back. Otherwise do nothing. */
    if (homeReturn == HOME_RET_GAME && Home_AtMainMenu() && live.parked) {
        Sfx_Play(SFX_CLICK);
        Home_XFadeBeginOut();
    } else if (Home_InGame()) {
        Sfx_Play(SFX_CLICK);
        Home_GoToMainMenu();
    }
}

// ====================== MAIN PROGRAM ======================

/* The window must land on a real monitor. On some X11 setups raylib/GLFW
 * centres it on a monitor rectangle that is not on the visible display (seen at
 * -3840+0 on a single 1920x1080 screen). If the window is not on a monitor,
 * centre it on the primary (or first usable) one. Returns 1 if it moved it. */
static int Window_EnsureOnScreen(void)
{
    int n = GetMonitorCount();
    Vector2 wp = GetWindowPosition();
    int ww = GetScreenWidth(), wh = GetScreenHeight();
    int cx = (int)wp.x + ww / 2, cy = (int)wp.y + wh / 2;
    int best = -1;
    if (n <= 0) {
        if (wp.x < -ww / 2 || wp.y < -wh / 2) { SetWindowPosition(0, 0); return 1; }
        return 0;
    }
    for (int m = 0; m < n; m++) {
        Vector2 mp = GetMonitorPosition(m);
        int mw = GetMonitorWidth(m), mh = GetMonitorHeight(m);
        if (mw <= 0 || mh <= 0) continue;
        if (cx >= (int)mp.x && cx < (int)mp.x + mw && cy >= (int)mp.y && cy < (int)mp.y + mh)
            return 0;   /* already on a monitor */
        if (best < 0) best = m;   /* monitor 0 is GLFW's primary */
    }
    if (best < 0) { SetWindowPosition(0, 0); return 1; }
    {
        Vector2 mp = GetMonitorPosition(best);
        int mw = GetMonitorWidth(best), mh = GetMonitorHeight(best);
        int x = (int)mp.x + (mw - ww) / 2, y = (int)mp.y + (mh - wh) / 2;
        if (x < (int)mp.x) x = (int)mp.x;
        if (y < (int)mp.y) y = (int)mp.y;
        SetWindowPosition(x, y);
    }
    return 1;
}

int main(int argc, char **argv) {
    nsoArgv = argv;
#ifndef _WIN32
    if (argc >= 3 && strcmp(argv[1], "--menu-launcher") == 0) {
        return NsoMenuLauncherMain(argc - 2, argv + 2);
    }
    struct sigaction externalMenuAction = { 0 };
    externalMenuAction.sa_handler = NsoExternalMenuSignal;
    sigemptyset(&externalMenuAction.sa_mask);
    sigaction(SIGUSR1, &externalMenuAction, NULL);
    struct sigaction externalHomeAction = { 0 };
    externalHomeAction.sa_handler = NsoExternalHomeSignal;
    sigemptyset(&externalHomeAction.sa_mask);
    sigaction(SIGUSR2, &externalHomeAction, NULL);
#endif
#ifdef NSO_RELEASE
    AssetPack_Install();
#endif
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NSO");
    Window_EnsureOnScreen();
    SetGamepadMappings(NSO_PAD_MAPPINGS);
    Nso_PadIsN64Hook = N64Pad_IsSwitchName;
    SetWindowTitle("NSO");
    SetTargetFPS(FPS);

    const char *base = GetBaseDirectory();
    if (NsoChdir(base) != 0) {
        perror("Failed to chdir to base directory");
    }
    NsoEnsureConfigDir();
    NsoEnsureShaderFile();
    /* CRT Geometry for the games NSO draws itself (embedded, rewind): built once here, so no hitch later */
    if (!CrtGeom_Load(GetDataPath("shaders/crtgeom.glsl"))) fprintf(stderr, "NSO: CRT Geometry shader unavailable\n");

    InitSystems();
    {
        char cfg[2112];
        Lang_ConfigPath(cfg, sizeof(cfg));
        Lang_Set(Lang_Load(cfg));
    }
    SysSettings_LoadAll();
    Pad_LoadMaps();
    Order_LoadAll();
    CoverSize_Start();
    SortPanel_LoadAll();
    Sfx_Init();
    Update_Start();

    PadNotice_Load();

    int Count = 0;
    char FontPath[2048];   /* base-dir relative, so ./nso works from any cwd */
    snprintf(FontPath, sizeof(FontPath), "%s", GetDataPath("assets/fonts/NotoSansCJKkr-Regular.otf"));
    int *Codepoints = LoadCodepoints(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        " .,!?()[]{}@'°-:/_+%&\"~*$"
        "áàâãéêíóôõúçÁÂÃÉÊÍÓÔÕÚÇëË"
        "āēīōūÿäöüĀĒĪŌŪÄÖÜ"
        "×½²³☆‼—–′‹›"
        "🅐🅑🅧🅨",
        &Count
    );

    Font font = LoadFontEx(FontPath, 36, Codepoints, Count);
    if (Codepoints) UnloadCodepoints(Codepoints);

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    slotFont = font;

    {
        char nerdPath[2048];
        snprintf(nerdPath, sizeof(nerdPath), "%s",
                 GetDataPath("assets/fonts/SymbolsNerdFont-Regular.ttf"));
        hintNerdLoaded = false;
        hintYNerdLoaded = false;
        hintNerdFont = (Font){ 0 };
        if (FileExists(nerdPath)) {
            int nerdCps[3] = { HINT_NERD_CUP, HINT_NERD_CDOWN, HINT_NERD_Y_CIRCLE };
            hintNerdFont = LoadFontEx(nerdPath, 36, nerdCps, 3);
            hintNerdLoaded = (hintNerdFont.texture.id != 0 && hintNerdFont.glyphCount >= 2);
            hintYNerdLoaded = false;
            if (hintNerdLoaded) {
                int gi = GetGlyphIndex(hintNerdFont, HINT_NERD_Y_CIRCLE);
                hintYNerdLoaded = (gi >= 0 && hintNerdFont.glyphs[gi].value == HINT_NERD_Y_CIRCLE
                                   && hintNerdFont.recs[gi].width > 1.0f);
                SetTextureFilter(hintNerdFont.texture, TEXTURE_FILTER_BILINEAR);
            } else if (hintNerdFont.texture.id) {
                UnloadFont(hintNerdFont);
                hintNerdFont = (Font){ 0 };
            }
        }
        favNerdLoaded = false;
        favNerdFont = (Font){ 0 };
        if (FileExists(nerdPath)) {
            int favCps[3] = { FAV_HEART, FAV_HEART_O, HIDE_BLOCK };
            favNerdFont = LoadFontEx(nerdPath, 64, favCps, 3);
            favNerdLoaded = (favNerdFont.texture.id != 0 && favNerdFont.glyphCount >= 2
                             && favNerdFont.recs[0].width > 1.0f && favNerdFont.recs[1].width > 1.0f);
            hideNerdLoaded = false;
            if (favNerdLoaded) {
                int gi = GetGlyphIndex(favNerdFont, HIDE_BLOCK);
                hideNerdLoaded = (gi >= 0 && favNerdFont.glyphs[gi].value == HIDE_BLOCK
                                  && favNerdFont.recs[gi].width > 1.0f);
                SetTextureFilter(favNerdFont.texture, TEXTURE_FILTER_BILINEAR);
            } else if (favNerdFont.texture.id) {
                UnloadFont(favNerdFont);
                favNerdFont = (Font){ 0 };
            }
        }
        statusNerdLoaded = false;
        statusNerdFont = (Font){ 0 };
        if (FileExists(nerdPath)) {
            int statusCps[] = {
                STATUS_NERD_WIFI, STATUS_NERD_WIFI_OFF, STATUS_NERD_WIFI_DISC,
                STATUS_NERD_BAT_FULL, STATUS_NERD_BAT_75, STATUS_NERD_BAT_50,
                STATUS_NERD_BAT_25, STATUS_NERD_BAT_0, STATUS_NERD_BOLT,
                STATUS_NERD_CIRCLE_PLUS
            };
            statusNerdFont = LoadFontEx(nerdPath, 48, statusCps,
                                        (int)(sizeof(statusCps) / sizeof(statusCps[0])));
            statusNerdLoaded = (statusNerdFont.texture.id != 0
                                && statusNerdFont.glyphCount >= 1);
            if (statusNerdLoaded) {
                SetTextureFilter(statusNerdFont.texture, TEXTURE_FILTER_BILINEAR);
            } else if (statusNerdFont.texture.id) {
                UnloadFont(statusNerdFont);
                statusNerdFont = (Font){ 0 };
            }
        }
        menuBarFontLoaded = false;
        menuBarFont = (Font){ 0 };
        if (FileExists(nerdPath)) {
            int barCps[2] = { MENU_BAR_DOWNLOAD, MENU_BAR_POWER };
            menuBarFont = LoadFontEx(nerdPath, 128, barCps, 2);
            menuBarFontLoaded = (menuBarFont.texture.id != 0 && menuBarFont.glyphCount >= 2);
            if (menuBarFontLoaded) {
                SetTextureFilter(menuBarFont.texture, TEXTURE_FILTER_BILINEAR);
            } else if (menuBarFont.texture.id) {
                UnloadFont(menuBarFont);
                menuBarFont = (Font){ 0 };
            }
        }
        statusPadN64Loaded = false;
        statusPadN64 = (Texture2D){ 0 };
        {
            const char *padPath = GetDataPath("assets/images/ui/pad-n64.png");
            if (FileExists(padPath)) {
                Image padImg = LoadImage(padPath);
                if (padImg.data) {
                    statusPadN64 = LoadTextureFromImage(padImg);
                    UnloadImage(padImg);
                    statusPadN64Loaded = (statusPadN64.id != 0);
                    if (statusPadN64Loaded)
                        SetTextureFilter(statusPadN64, TEXTURE_FILTER_BILINEAR);
                }
            }
        }
    }

    {   /* enter transition: just the wordmark glyph and the letters of "PC Ports", large for a clean downscale */
        char nerdPath[2048];
        snprintf(nerdPath, sizeof(nerdPath), "%s", GetDataPath("assets/fonts/SymbolsNerdFont-Regular.ttf"));
        enterLogoOk = false;
        if (FileExists(nerdPath)) {
            int cps[1] = { ENTER_LOGO_CP };
            enterLogoFont = LoadFontEx(nerdPath, 256, cps, 1);
            enterLogoOk = enterLogoFont.texture.id != 0 && enterLogoFont.glyphCount >= 1
                          && enterLogoFont.recs[0].width > 1.0f;
            if (enterLogoOk) SetTextureFilter(enterLogoFont.texture, TEXTURE_FILTER_BILINEAR);
        }
        int tcps[64], ntc = 0, cp;
        for (cp = 'A'; cp <= 'Z'; cp++) tcps[ntc++] = cp;
        for (cp = 'a'; cp <= 'z'; cp++) tcps[ntc++] = cp;
        for (cp = '0'; cp <= '9'; cp++) tcps[ntc++] = cp;
        tcps[ntc++] = ' ';
        enterTextFont = LoadFontEx(FontPath, 160, tcps, ntc);
        enterTextOk = enterTextFont.texture.id != 0;
        if (enterTextOk) SetTextureFilter(enterTextFont.texture, TEXTURE_FILTER_BILINEAR);
    }

    for (int i = 0; i < MAX_GAMES; i++) {
        gameBoxarts[i] = (Texture2D){0};
        animScales[i] = 1.0f;
    }

    Color Mosaicbg = GetColor(0x303030FF);

    bool externalResumeAfterFrame = false;
    int winFixFrames = 120;   /* a window manager may move the window right after it maps */
    while (!WindowShouldClose() && !nsoQuit) {
        if (winFixFrames > 0 && (--winFixFrames % 10) == 0) Window_EnsureOnScreen();

        N64Pad_Poll();   /* Switch-mode N64 pad mapping, rumble device */
        bool gp = IsGamepadAvailable(0);
        static bool padWasAvail = false;
        static double padSettleUntil = 0.0;
        static NsoPadAxisArm armLX, armLY, armRX, armRY;
        if (gp != padWasAvail) {
            Nso_PadAxisArmReset(&armLX);
            Nso_PadAxisArmReset(&armLY);
            Nso_PadAxisArmReset(&armRX);
            Nso_PadAxisArmReset(&armRY);
            if (gp) padSettleUntil = GetTime() + (double)NSO_PAD_SETTLE_SEC;
            padWasAvail = gp;
        }
        bool gpReady = gp && GetTime() >= padSettleUntil;

        float lx = 0.0f, ly = 0.0f, rx = 0.0f, ry = 0.0f;
        if (gp) {
            lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
            N64Pad_RightAxes(0, &rx, &ry);
        }
        int eLX = Nso_PadAxisEdgeArmed(&armLX, lx);
        int eLY = Nso_PadAxisEdgeArmed(&armLY, ly);
        int eRX = Nso_PadAxisEdgeArmed(&armRX, rx);
        int eRY = Nso_PadAxisEdgeArmed(&armRY, ry);

        bool gpLeft = gpReady && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) || eLX < 0);
        bool gpRight = gpReady && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) || eLX > 0);
        bool gpUp = gpReady && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) || eLY < 0);
        bool gpDown = gpReady && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) || eLY > 0);
        bool gpConfirm = gpReady && Nso_PadConfirmPressed(0);
        /* Select opens the full sidebar; keep Back as B only (not Select). */
//        bool gpSelect = gpReady && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT);
        bool gpBack = gpReady && Nso_PadBackPressed(0);

        Home_Poll();

        padCUp = gpReady && eRY < 0;
        padCDown = gpReady && eRY > 0;
        padCDownHeld = gpReady && ry > NSO_PAD_DEADZONE;
        padCLeft = gpReady && eRX < 0;
        padCRight = gpReady && eRX > 0;

        bool overlayConsumedConfirm = false;
        /* Returning to the grid: hold the previous (game / splash) frame until
         * the visible screen's covers are uploaded (no empty slots, no loading
         * screen). Only while mosaicRevealWait: browsing must not blank the grid. */
        if (mosaicRevealWait && CoverLoader_VisibleReady()) {
            mosaicRevealWait = false;
            MosaicBlur_HoldEnd();
            CoverReload_Stamp("reveal-ready");
        }
        bool gridHold = mosaicRevealWait && !CoverLoader_VisibleReady();
        bool canNavigate = !OverlayActive() && !isLaunching && !Live_Busy()
            && !screenSliding && !gridHold && !GameExitActive() && !GameEnterActive();

        if (introFrames < FPS) {
            introOpacity = 1.0f - ((float)introFrames / (float)FPS);
            introFrames++;
        } else {
            introOpacity = 0.0f;
        }

        /* Debug: NSO_SCREENSHOT=<path> focuses the dock (download icon) after
         * the intro fade; the capture itself happens just before EndDrawing. */
        if (getenv("NSO_SCREENSHOT") && getenv("NSO_SCREENSHOT")[0]
            && currentState == SYSTEMS && introFrames >= FPS && menuBarFocus < 0) {
            menuBarFocus = 0;
        }

        /* Debug: NSO_AUTOSELECT=<system index> enters that system after 1.5s
         * (lets the splash be captured without a keyboard). */
        static bool autoSelected = false;
        const char *autoSel = getenv("NSO_AUTOSELECT");
        if (autoSel && !autoSelected && currentState == SYSTEMS && GetTime() > 1.5) {
            autoSelected = true;
            int idx = atoi(autoSel);
            if (idx >= 0 && idx < SYSTEM_COUNT) currentSystem = idx;
            Sfx_Play(SFX_CLICK);
            StartEnterMosaic();
        }

        if (waitingForGame && NsoProcOk(gamePid)) {
            if (NsoSpawnExited(&gamePid, NULL)) {
                if (currentState == EMBED) Embed_Stop();
                bool wasParked = live.parked && live.pid == gamePid;
                if (wasParked) Live_Clear();
                if (externalRestartOnExit && launchingGameIdx < ActiveGameCount()) {
                    externalRestartOnExit = false;
                    const GameEntry *restartGame = &ActiveGames()[launchingGameIdx];
                    gamePid = LaunchGame(restartGame);
                    if (NsoProcOk(gamePid)) {
                        waitingForGame = true;
                        externalQuitting = false;
                        currentState = MOSAIC;
                        if (!GameHasExternalMenu(restartGame)) MinimizeWindow();
                    } else {
                        waitingForGame = false;
                        RestoreWindow();
                        currentState = MOSAIC;
                        BeginMosaicEnterIntro();
                    }
                } else {
                    externalRestartOnExit = false;
                    waitingForGame = false;
                    RestoreWindow();
                    currentState = MOSAIC;
                    /* Same enter intro as systems → mosaic (delay, grow, then info fade);
                     * after a Quit the card-return animation already started it. */
                    if (!GameExitActive() && !GameEnterActive() && !externalQuitting && !wasParked) BeginMosaicEnterIntro();
                    if (gameExitPhase == GAME_EXIT_FADE) {
                        gameExitT = 1.0f;
                        gameExitPhase = GAME_EXIT_CLOSE;
                        Embed_ExitOverlaySet(1.0f);
                    }
                    if (GameEnterActive() && !wasParked) {
                        gameEnterFailed = true;
                        if (gameEnterPhase != GAME_ENTER_FADE) gameEnterT = 0.0f;
                        gameEnterPhase = GAME_ENTER_FADE;
                    }
                    externalQuitting = false;
                }
            }
        }
#ifndef _WIN32
        if (externalMenuSignal) {
            externalMenuSignal = 0;
            if (Live_Busy() && currentState != EMBED && launchingGameIdx < ActiveGameCount()) {
                const GameEntry *runningGame = &ActiveGames()[launchingGameIdx];
                if (GameHasExternalMenu(runningGame)) {
                    char menuDir[1200];
                    char windowPath[1240];
                    unsigned long childWindow = 0;
                    if (GameMenuDirectory(menuDir, sizeof(menuDir), runningGame)) {
                        snprintf(windowPath, sizeof(windowPath), "%s/nso-menu-window", menuDir);
                        FILE *windowFile = fopen(windowPath, "rb");
                        if (windowFile) {
                            if (fscanf(windowFile, "%lu", &childWindow) != 1) childWindow = 0;
                            fclose(windowFile);
                        }
                        Rewind_Reset();   /* the window must be showing when its frame is captured */
                        if (Embed_StartExternal(menuDir, childWindow, runningGame->externalSlots)) {
                            currentState = EMBED;
                            RestoreWindow();
                        }
                    }
                }
            }
        }
#endif

#ifndef _WIN32
        /* Generic child launchers own the game process and translate the
         * Guide request into a pause plus a signal back to the host. */
        if (Live_Busy() && currentState != EMBED && launchingGameIdx < ActiveGameCount()) {
            const GameEntry *runningGame = &ActiveGames()[launchingGameIdx];
            bool menuPressed = IsKeyPressed(KEY_END)
                || (IsGamepadAvailable(0)
                    && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE));
            if (GameUsesMenuLauncher(runningGame) && menuPressed && NsoProcOk(gamePid))
                kill(gamePid, SIGUSR1);
        }
#endif

        int gameCount = ActiveGameCount();
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;
        Pad_Update(dt);
#ifndef _WIN32
        if (Rewind_Session()) {
            if (!waitingForGame) {
                Rewind_End();
            } else {
                bool inGame = currentState != EMBED && !GameExitActive() && !externalResumeAfterFrame
                              && !externalQuitting && !isLaunching && !live.parked;
                bool chord = IsGamepadAvailable(0) && IsGamepadButtonDown(0, GAMEPAD_BUTTON_LEFT_TRIGGER_2)
                             && IsGamepadButtonDown(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_2);
                Rewind_Frame(dt, inGame, chord);
                if (Rewind_MenuTap() && inGame && NsoProcOk(gamePid)) kill(gamePid, SIGUSR1);
            }
        }
#endif
        {
            static bool padNoticeEnvDone = false;
            const char *force = getenv("NSO_PAD_NOTICE");
            if (!padNoticeEnvDone && force && force[0] && force[0] != '0'
                && currentState == MOSAIC && !InfoBar_IsHidden()) {
                padNoticeEnvDone = true;
                PadNotice_Show();
            }
        }

        bool layoutLocked = screenSliding;

        if (currentState == MOSAIC && !Live_Busy() && !layoutLocked) {
            if (mosaicExitPhase == MOSAIC_EXIT_INFO) {
                /* Hold grown selection while info bar closes (reverse of grow→info). */
                if (InfoBar_IsHidden()) {
                    mosaicExitPhase = MOSAIC_EXIT_SHRINK;
                }
            } else if (mosaicExitPhase == MOSAIC_EXIT_SHRINK) {
                if (UpdateMosaicItemScales(gameCount, dt, false)) {
                    BeginLeaveMosaicSlide();
                }
            } else if (mosaicIntroPhase == MOSAIC_INTRO_DELAY) {
                /* Hold flat for enter delay; only difference vs normal selection grow. */
                for (int i = 0; i < gameCount; i++) {
                    animScales[i] = 1.0f;
                }
                if (!gridHold) mosaicIntroTimer -= dt;
                if (mosaicIntroTimer <= 0.0f) {
                    mosaicIntroTimer = 0.0f;
                    mosaicIntroPhase = MOSAIC_INTRO_GROW;
                }
            } else if (mosaicIntroPhase == MOSAIC_INTRO_GROW) {
                /* Same constant-velocity grow as normal menu; then info bar fades in. */
                if (UpdateMosaicItemScales(gameCount, dt, !sidebarOpen && !sidebarExpandPending)) {
                    mosaicIntroPhase = MOSAIC_INTRO_NONE;
                    if (!sidebarOpen && !sidebarExpandPending) {
                        InfoBar_Request(INFO_BAR_MOSAIC);
                    }
                }
            } else if (sidebarExpandPending) {
                /* Shrink selected first; only then open the full sidebar. */
                if (UpdateMosaicItemScales(gameCount, dt, false)) {
                    ExpandSidebarMenu();
                }
            } else {
                /* Grow when rail-only; shrink while full sidebar is open / animating. */
                bool grow = !sidebarOpen && !sidebarExpandPending && !sidebarClosing
                    && sbWidthPhase == SB_WIDTH_IDLE;
                UpdateMosaicItemScales(gameCount, dt, grow);
            }
        } else if (!layoutLocked && currentState != MOSAIC) {
            for (int i = 0; i < MAX_GAMES; i++) {
                animScales[i] = 1.0f;
            }
        }

        /* Info bars: sequential close→open, no overlap. */
        InfoBar_Update(dt);

        GridLayout grid = GetGridLayout();

        if (currentState == MOSAIC && !layoutLocked) {
            if (gameCount > 0 && GameIdx >= gameCount) {
                GameIdx = gameCount - 1;
            }

            float nextTarget = GridTargetViewportY(&grid, GameIdx, gameCount, viewportY);
            nextTarget = GridClampViewportY(&grid, nextTarget, gameCount);
            if (nextTarget != targetViewportY) {
                viewportSlideFromY = viewportY;
                viewportSlideT = 0.0f;
                targetViewportY = nextTarget;
            }
            CoverLoader_Poll();
            ManageVisibleBoxarts(gameCount, viewportY);
        } else {
            CoverLoader_Poll();
        }

        if (viewportSlideT < 1.0f) {
            viewportSlideT += dt / GRID_PAGE_SLIDE_SEC;
            if (viewportSlideT >= 1.0f) {
                viewportSlideT = 1.0f;
                viewportY = targetViewportY;
            } else {
                viewportY = viewportSlideFromY
                    + (targetViewportY - viewportSlideFromY) * viewportSlideT;
            }
        }
        if (currentState == MOSAIC && !layoutLocked) {
            viewportY = GridClampViewportY(&grid, viewportY, gameCount);
        }
        UpdateSidebarWidth();

        if (Home_XFadeActive()) Home_XFadeStep(dt);
        Home_ApplyDeferredOverlay();
        if (GameEnterActive()) {
            GameEnter_Step(dt);
        } else if (GameExitActive()) {
            GameExit_Step(dt);
        } else if (currentState == EMBED && !Embed_Update(dt)) {
            const int externalResult = Embed_ExternalResult();
            if (externalResult != EMBED_EXTERNAL_NONE) {
                if (externalResult == EMBED_EXTERNAL_QUIT) {
                    /* To Game Selection: fade to black first; park only when covered. */
                    Live_BeginPark();
                    GameExit_Begin();
                } else {
                    if (externalResult == EMBED_EXTERNAL_RESUME) {
                        /* Present one clean host frame before raising the SDL
                         * child; swapping the GL window after XMapRaised can
                         * expose a stale parent buffer for a frame. */
                        externalResumeAfterFrame = true;
                    } else if (externalResult == EMBED_EXTERNAL_RESTART) {
                        Embed_ExternalSetChildVisible(false);
                        externalRestartOnExit = true;
                    }
                    if (!externalResumeAfterFrame) Embed_Stop();
                    currentState = MOSAIC;
                }
                overlayConsumedConfirm = true;
            } else {
                /* In-process module: fade to black, then park behind the grid. */
                Live_BeginPark();
                GameExit_Begin();
                overlayConsumedConfirm = true;   /* the Quit press must not relaunch */
            }
        } else if (currentState == EMBED && Embed_IsLeaving() && !GameExitActive()) {
            /* Start the black fade on the Quit press, over the still-open menu. */
            Live_BeginPark();
            GameExit_Begin();
            overlayConsumedConfirm = true;
        }

        if (currentState == MOSAIC && SidebarMenuSettled()) {
            Sidebar_HandleInput(gpUp, gpDown, gpLeft, gpRight, gpConfirm, gpBack);
        }

        ScrapeMenu_Poll();
        Update_Poll();
        GameSel_Poll();
        Repo_Poll();
        Repo_Watch();
        Details_Update(dt);
        Status_Update(dt);
        {   /* move-mode look: linear, picom default rates (see Details) */
            float st = dt * 1000.0f / DETAILS_FADE_DELTA_MS;
            if (gridLifted) liftT = fminf(1.0f, liftT + st * DETAILS_FADE_IN_STEP);
            else liftT = fmaxf(0.0f, liftT - st * DETAILS_FADE_OUT_STEP);
            float rs = dt / MOVE_RAIL_SLIDE_SEC;
            railSlideT = gridLifted ? fminf(1.0f, railSlideT + rs) : fmaxf(0.0f, railSlideT - rs);
        }

        /* A on an install notice builds that game (one press). Not while a
         * game, Details, a scrape or a slide owns input. */
        bool noticeTookInput = false;
        if (Repo_NoticeActionable() && !screenSliding && !isLaunching && !Live_Busy()
            && !detailsOpen && detailsT <= 0.0f && !scrapeBusy && !OverlayActive()
            && currentState != LAUNCHING && currentState != SPLASH && currentState != EMBED
            && (IsKeyPressed(KEY_A) || gpConfirm)) {
            Repo_Enqueue(repoNotice.repo, true);
            repoNotice.repo = -1;
            noticeTookInput = true;
        }

        if (!screenSliding && !noticeTookInput) {
            switch (currentState) {
                case SYSTEMS:
                    /* KEY_END leaves NSO only from this menu (not in-game). */
                    if (canNavigate && !Home_XFadeActive() && !Home_InGame()
                        && IsKeyPressed(KEY_END)) {
                        nsoQuit = true;
                        break;
                    }
                    if (powerDialogOpen) {
                        bool up = IsKeyPressed(KEY_UP) || gpUp;
                        bool down = IsKeyPressed(KEY_DOWN) || gpDown;
                        bool ok = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm;
                        bool back = IsKeyPressed(KEY_B) || gpBack;
                        if (up && powerDialogRow > 0) { powerDialogRow--; Sfx_Play(SFX_PLUCK); }
                        if (down && powerDialogRow < 2) { powerDialogRow++; Sfx_Play(SFX_PLUCK); }
                        if (back) { powerDialogOpen = false; Sfx_Play(SFX_CLICK); }
                        else if (ok) {
                            Sfx_Play(SFX_CLICK);
                            if (powerDialogRow == 0) Power_ShutDown();
                            else if (powerDialogRow == 1) nsoQuit = true;
                            powerDialogOpen = false;
                        }
                        break;
                    }
                    if (canNavigate && menuBarFocus < 0 && SYSTEM_COUNT > 1) {
                        if (IsKeyPressed(KEY_LEFT) || gpLeft) {
                            SelectSystem(-1);
                        }
                        if (IsKeyPressed(KEY_RIGHT) || gpRight) {
                            SelectSystem(1);
                        }
                    }
                    if (canNavigate && menuBarFocus >= 0) {
                        if ((IsKeyPressed(KEY_LEFT) || gpLeft) && menuBarFocus > 0) {
                            menuBarFocus--;
                            Sfx_Play(SFX_PLUCK);
                        }
                        if ((IsKeyPressed(KEY_RIGHT) || gpRight) && menuBarFocus < MENU_BAR_ICON_COUNT - 1) {
                            menuBarFocus++;
                            Sfx_Play(SFX_PLUCK);
                        }
                    }
                    if (canNavigate && (IsKeyPressed(KEY_DOWN) || gpDown) && menuBarFocus < 0) {
                        menuBarFocus = 0;
                        Sfx_Play(SFX_PLUCK);
                    }
                    if (canNavigate && (IsKeyPressed(KEY_UP) || gpUp) && menuBarFocus >= 0) {
                        menuBarFocus = -1;
                        Sfx_Play(SFX_PLUCK);
                    }

                    if (canNavigate && !overlayConsumedConfirm) {
                        bool ok = IsKeyPressed(KEY_ENTER) || gpConfirm;
                        bool okBar = ok || IsKeyPressed(KEY_A);
                        if (menuBarFocus == 0 && okBar) {
                            if (Update_Install()) Sfx_Play(SFX_CLICK);   /* no update: does nothing */
                        } else if (menuBarFocus == 1 && okBar) {
                            powerDialogOpen = true;
                            powerDialogRow = 2;   /* Cancel */
                            Sfx_Play(SFX_CLICK);
                        } else if (menuBarFocus < 0 && ok) {
                            Sfx_Play(SFX_CLICK);   /* the selection; the system's jingle follows the transition */
                            StartEnterMosaic();
                        }
                    }
                    break;

                case MOSAIC: {
                    /* Enter-games-menu animation (tile delay+grow after the
                     * slide): no games-menu input until it has finished. */
                    if (mosaicIntroPhase != MOSAIC_INTRO_NONE) break;
                    bool mosaicIdle = liveConfirm == 0 && !Live_Busy()
                        && !GameExitActive() && !GameEnterActive()
                        && !SortPanel_Active()
                        && !sidebarOpen && !sidebarExpandPending && !sidebarClosing
                        && sbWidthPhase == SB_WIDTH_IDLE && !gridLifted
                        && !statusOpen && statusT <= 0.0f
                        && !detailsOpen && detailsT <= 0.0f;
                    SortY_Update(canNavigate && mosaicIdle,
                                 canNavigate && mosaicIdle && gameCount > 0 && GameIdx < gameCount);
                    if (liveConfirm != 0) {
                        Live_ConfirmInput(gpUp, gpDown, gpConfirm, gpBack);
                        break;
                    }
                    if (Live_Busy()) break;
                    if (GameExitActive() || GameEnterActive()) break;

                    if (SortPanel_Active()) {
                        SortPanel_Input(gpUp, gpDown, gpConfirm, gpBack);
                        break;
                    }

                    /* KEY_O only: shrink selection, then expand full menu (no Select). */
                    if (!sidebarOpen && !sidebarExpandPending && !sidebarClosing
                        && sbWidthPhase == SB_WIDTH_IDLE && !gridLifted
                        && canNavigate && (IsKeyPressed(KEY_O) || padCLeft)) {
                        RequestSidebarExpand();
                        break;
                    }

                    if (sidebarOpen || sidebarExpandPending || sidebarClosing
                        || sbWidthPhase != SB_WIDTH_IDLE) {
                        break;
                    }

                    if (statusOpen || statusT > 0.0f) {
                        Status_HandleInput(gpUp, gpDown, gpBack, gpConfirm, gpLeft, gpRight);
                        break;
                    }

                    if (detailsOpen || detailsT > 0.0f) {
                        Details_HandleInput(gpConfirm, gpBack, gpLeft, gpRight, gpUp, gpDown);
                        break;
                    }

                    /* Move mode (Y tap): arrows move the lifted game; A confirms
                     * and saves, B puts it back where it was (no save); Y ignored. */
                    if (gridLifted) {
                        if (GameIdx >= gameCount) {
                            gridLifted = false;
                            gridLiftMoved = false;
                            break;
                        }
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm) {
                            gridLifted = false;
                            if (gridLiftMoved) { Order_SaveActive(); SortPanel_SetCustom(); }
                            gridLiftMoved = false;
                            break;
                        }
                        if (IsKeyPressed(KEY_B) || gpBack) {
                            /* Only the lifted game moved, so moving it back restores
                             * the order exactly. */
                            if (gridLiftFrom >= 0 && gridLiftFrom < gameCount) Grid_MoveLifted(gridLiftFrom);
                            gridLifted = false;
                            gridLiftMoved = false;
                            break;
                        }
                        int dir = GridNavPressed(gpLeft, gpRight, gpUp, gpDown);
                        if (dir >= 0) {
                            int next = GridNavigate(&grid, GameIdx, gameCount, (OrderNav)dir);
                            if (next != GameIdx) Sfx_Play(SFX_NAVIGATE);
                            Grid_MoveLifted(next);
                        }
                        break;
                    }

                    /* T / right stick right / C-Right: change order of titles.
                     * Keyboard Y / gamepad Y / C-Down: hold (SortY_Update). */
                    if (canNavigate && !gridLifted && (IsKeyPressed(KEY_T) || padCRight)) {
                        SortPanel_Open();
                        break;
                    }

                    /* S: port status overlay. */
                    if (canNavigate && !gridLifted && IsKeyPressed(KEY_S)) {
                        Status_Open();
                        break;
                    }

                    bool onGame = canNavigate && gameCount > 0 && GameIdx < gameCount;
                    /* Keyboard Y is SortY_Update (hold = sort, tap = Move). */

                    /* Keyboard X / gamepad X / C-Up: details panel. */
                    if (onGame && (IsKeyPressed(KEY_X) || padCUp || (gp && IsGamepadButtonPressed(0, NSO_BTN_X)))) {
                        Details_Open();
                        break;
                    }

                    if (gameCount > 0) {
                        int dir = GridNavPressed(gpLeft, gpRight, gpUp, gpDown);
                        if (dir >= 0) {
                            int next = GridNavigate(&grid, GameIdx, gameCount, (OrderNav)dir);
                            if (next != GameIdx) Sfx_Play(SFX_NAVIGATE);
                            GameIdx = next;
                        }
                    }

                    if (!overlayConsumedConfirm && liveConfirm == 0
                        && (IsKeyPressed(KEY_ENTER) || gpConfirm)) {
                        if (gameCount > 0 && GameIdx < gameCount) {
                            if (GameIsLaunchable(&ActiveGames()[GameIdx])) {
                                Sfx_Play(SFX_CLICK);
                                if (live.parked && !Live_IsSame(currentSystem, GameIdx)) {
                                    liveConfirm = 1;
                                    liveConfirmSel = 0;
                                    liveConfirmGame = GameIdx;
                                    liveConfirmT = 0.0f;
                                } else {
                                    isLaunching = true;
                                    launchingGameIdx = GameIdx;
                                    currentState = LAUNCHING;
                                    GameEnter_Begin();
                                }
                            } else if (ActiveGames()[GameIdx].mednafen
                                       && ActiveGames()[GameIdx].mednafen[0]) {
                                NsoMednafenRecheck();
                                if (!NsoBundledMednafen()) {
                                    Sfx_Play(SFX_CLICK);
                                    LaunchMsg_Show(S_NO_MEDNAFEN);
                                }
                            }
                        }
                    }

                    if (canNavigate && (IsKeyPressed(KEY_B) || gpBack)) {
                        RequestLeaveMosaic();
                    }
                    break;
                }

                case LAUNCHING:
                case SPLASH:
                case EMBED:   /* embed.h reads its own input */
                    break;
            }
        }

        SortPanel_Play(Live_Busy() || currentState == EMBED);

        if (!screenSliding) UpdateSplash(dt);

        /* While a child (Mednafen / reparented port) owns the screen, do not keep
         * redrawing the 4x-MSAA 1920x1080 grid under it. Poll End / exit only. */
        static bool frontendIdleBlanked = false;
        /* A Home trip sets SYSTEMS / xfade / unmap-after-frame while the
         * child is still Live_Busy(); those frames must keep drawing and
         * reach Home_ParkAfterFrame. Skipping them latched the Home action
         * until End (or another wakeup) left this idle path. */
        bool gameOwnsScreen = Live_Busy() && currentState != EMBED
            && !GameExitActive() && !GameEnterActive() && !externalResumeAfterFrame && !externalQuitting
            && !isLaunching
            && !Home_XFadeActive() && !homeUnmapAfterFrame && !homeResumeAfterFrame
            && !homeApplyAfterFrame;
        bool frontendIdle = gameOwnsScreen && !Rewind_NeedsDraw();
#ifndef _WIN32
        if (frontendIdle) {
            if (!frontendIdleBlanked) {
                BeginDrawing();
                ClearBackground(BLACK);
                EndDrawing();
                frontendIdleBlanked = true;
            } else {
                PollInputEvents();
                WaitTime(1.0 / (double)FPS);
            }
            continue;
        }
        frontendIdleBlanked = false;
#endif

        BeginDrawing();

        if (screenSliding) {
            SlideEffect(Mosaicbg, font);
        } else if (currentState == SYSTEMS) {
            if (Home_XFadeActive() || homeApplyAfterFrame || homeResumeAfterFrame) {
                Home_DrawXFade(Home_XFadeActive() ? Home_MenuAlpha() : 0.0f);
            } else {
                DrawSystemMenu(0.0f);
                if (introOpacity > 0.0f) {
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                  Fade(BLACK, introOpacity));
                }
            }
        } else if (currentState == SPLASH) {
            DrawEnterTransition();
        } else if (currentState == EMBED) {
            const char *const text[EMBED_TXT_COUNT] = { T(S_EMB_CONTINUE), T(S_EMB_LOAD), T(S_EMB_SAVE),
                T(S_EMB_RESTART), T(S_EMB_CRT), T(S_EMB_QUIT), T(S_EMB_SLOT), T(S_EMB_NODATA), T(S_EMB_DATE),
                T(S_EMB_LOADING),
                T(S_CRT_GAMMA), T(S_CRT_MGAMMA), T(S_CRT_OVERSCAN), T(S_CRT_SAT), T(S_CRT_CURV), T(S_CRT_DIST),
                T(S_CRT_ANGLE), T(S_CRT_ASPECT), T(S_CRT_CORNER), T(S_CRT_CSMOOTH), T(S_CRT_HALATION),
                T(S_CRT_FILMIC), T(S_CRT_RESET) };
            if (Embed_MenuOpacity() > 0.0f) EnsureBlurResources(GetScreenWidth(), GetScreenHeight());
            Embed_Draw(font, ActiveEmbedScreen(), text, blurShaderOk ? blurShader : (Shader){ 0 }, blurDirLoc);
            DrawInfoBarEx(font, Embed_CrtOpen() ? T(S_EMB_CRT_INFO) : T(S_SIDEBAR_INFO), Embed_MenuOpacity(), 0.0f);
            if (gameExitPhase == GAME_EXIT_FADE || gameExitPhase == GAME_EXIT_CLOSE)
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, gameExitT));
            if (gameEnterPhase == GAME_ENTER_START || gameEnterPhase == GAME_ENTER_FADE) {
                float a = (gameEnterPhase == GAME_ENTER_START) ? 1.0f : (1.0f - gameEnterT);
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, a));
            }
        } else if (currentState == MOSAIC || currentState == LAUNCHING) {
            if (homeResumeAfterFrame && homeUnderlay.id) {
                Home_DrawXFade(0.0f);
            } else
            /* A child already owns the screen: skip the grid (the black cover
             * plus Rewind_Draw are enough). Enter/leave still draw it when
             * gameOwnsScreen is false. */
            if (gameExitPhase == GAME_EXIT_CLOSE && !gameExitHandoff) {
                ClearBackground(BLACK);
            } else if (gameExitPhase == GAME_EXIT_CLOSE || gameExitPhase == GAME_EXIT_REVEAL) {
                DrawMosaic(Mosaicbg, font);
                Details_Draw(font);
                Status_Draw(font);
                float shrink = (gameExitPhase == GAME_EXIT_REVEAL) ? gameExitT : 0.0f;
                Rectangle full = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
                DrawRectangleRec(Embed_ZoomRect(SelectedCardRect(1.0f), full, 1.0f - shrink), BLACK);
            } else if (gameEnterPhase == GAME_ENTER_GROW) {
                DrawMosaic(Mosaicbg, font);
                Details_Draw(font);
                Status_Draw(font);
                Rectangle full = { 0, 0, (float)GetScreenWidth(), (float)GetScreenHeight() };
                DrawRectangleRec(Embed_ZoomRect(SelectedCardRect(animScales[GameIdx]), full, gameEnterT), BLACK);
            } else if (gameEnterPhase == GAME_ENTER_START || gameEnterPhase == GAME_ENTER_FADE) {
                ClearBackground(BLACK);
                if (gameEnterPhase == GAME_ENTER_FADE && gameEnterFailed) {
                    DrawMosaic(Mosaicbg, font);
                    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                                  Fade(BLACK, 1.0f - gameEnterT));
                }
            } else if (gridHold) {
                if (mosaicBlurHeld) {
                    /* Hide/filter re-list: keep the pre-toggle blur, not black. */
                    DrawMosaic(Mosaicbg, font);
                    Details_Draw(font);
                    Status_Draw(font);
                } else {
                    ClearBackground(BLACK);   /* the held frame: game / black, never empty slots */
                }
            } else if (!gameOwnsScreen) {
                DrawMosaic(Mosaicbg, font);
                Details_Draw(font);
                Status_Draw(font);
            }
            if (externalResumeAfterFrame)
                Embed_DrawExternalResumeFrame(ActiveEmbedScreen());
        }

        if (Live_Busy() && currentState != EMBED && !GameExitActive() && !externalResumeAfterFrame && !externalQuitting) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
            if (Rewind_Session())
                Rewind_Draw(font, T(S_RW_HOLD), T(S_RW_SUSPEND), T(S_RW_BACK), T(S_RW_OK));
        } else if (!isLaunching && currentState != EMBED) {
            Repo_NoticeDraw(font);
            PadNotice_Draw(font);
        }

        {
            const char *shot = getenv("NSO_SCREENSHOT");
            if (shot && shot[0] && currentState == SYSTEMS && introFrames >= FPS
                && menuBarFocus == 0) {
                static int shotFrames = 0;
                shotFrames++;
                if (shotFrames >= 2) {   /* a full focused bar frame is in the back buffer */
                    TakeScreenshot(shot);
                    nsoQuit = true;
                }
            }
        }

        EndDrawing();

        /* After a fully black host frame has been swapped, drop the X overlay
         * and start shrinking the GL black rectangle into the cover. */
        if (gameExitHandoff && gameExitPhase == GAME_EXIT_CLOSE) {
            Embed_ExitOverlayDestroy();
            gameExitHandoff = false;
            gameExitPhase = GAME_EXIT_REVEAL;
            gameExitT = 0.0f;
            BeginMosaicEnterIntro();
        }
        if (gameEnterHandoff && gameEnterPhase == GAME_ENTER_START) {
            gameEnterHandoff = false;
            gameEnterPhase = GAME_ENTER_FADE;
            gameEnterT = 0.0f;
        }

        Home_ParkAfterFrame();
        if (homeApplyAfterFrame) {
            homeApplyAfterFrame = false;
            Home_ApplyReturn();
            if (liveShowAfterFrame) {
                Embed_ShowExternalWindow(live.window);
                liveShowAfterFrame = false;
            }
            if (!homeResumeAfterFrame) Home_UnloadGfx();
        }
        if (homeResumeAfterFrame) {
            homeResumeAfterFrame = false;
            if (liveShowAfterFrame) {
                Embed_ShowExternalWindow(live.window);
                liveShowAfterFrame = false;
            }
            Home_UnloadGfx();
        }
        if (liveShowAfterFrame) {
            Embed_ShowExternalWindow(live.window);
            liveShowAfterFrame = false;
        }
        if (externalResumeAfterFrame) {
            Embed_ExternalSetChildVisible(true);
            Embed_Stop();
            externalResumeAfterFrame = false;
        }
    }

    GameEnter_Reset();
    GameExit_Reset();
    if (live.parked || Embed_IsActive()) Live_Close();
    else Embed_Stop();
    CoverLoader_Stop();
    CoverSize_Stop();
    if (NsoProcOk(repoProc)) NsoSpawnKill(repoProc);
    if (GameSel_Busy()) GameSel_Cancel();
    UnloadAllBoxarts();
    if (enterLogoOk) UnloadFont(enterLogoFont);
    if (enterTextOk) UnloadFont(enterTextFont);
    UnloadFont(font);
    if (hintNerdLoaded) {
        UnloadFont(hintNerdFont);
        hintNerdFont = (Font){ 0 };
        hintNerdLoaded = false;
        hintYNerdLoaded = false;
    }
    if (favNerdLoaded) {
        UnloadFont(favNerdFont);
        favNerdFont = (Font){ 0 };
        favNerdLoaded = false;
        hideNerdLoaded = false;
    }
    if (statusNerdLoaded) {
        UnloadFont(statusNerdFont);
        statusNerdFont = (Font){ 0 };
        statusNerdLoaded = false;
    }
    if (menuBarFontLoaded) {
        UnloadFont(menuBarFont);
        menuBarFont = (Font){ 0 };
        menuBarFontLoaded = false;
    }
    MenuBarCursorTex_Unload();
    if (statusPadN64Loaded) {
        UnloadTexture(statusPadN64);
        statusPadN64 = (Texture2D){ 0 };
        statusPadN64Loaded = false;
    }
    PadUi_Unload();
    PadNotice_Unload();
    RetroShots_Unload();
    UnloadBlurResources();
    CrtGeom_Unload();
    SortPanel_Unload();
    SidebarIcons_Unload();
    UnloadSystems();
    Sfx_Shutdown();
    CloseWindow();

#ifndef _WIN32
    if (nsoRestart && nsoSelfPath[0] && nsoArgv) {   /* updated: start the new binary */
        fflush(NULL);
        execv(nsoSelfPath, nsoArgv);
        perror("NSO: restart after update failed");
    }
#endif

    /* Skip static destructors of a persistent port engine (they wait on its threads). */
    if (Embed_NeedsFastExit()) { fflush(NULL); _exit(EXIT_SUCCESS); }
    return EXIT_SUCCESS;
}
