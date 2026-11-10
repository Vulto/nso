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

#include "launch.h"
#include "raylib.h"

#define SCRAPE_IMPLEMENTATION
#include "scrape.h"

#define SAVESTATE_IMPLEMENTATION
#include "savestate.h"
#define LANG_IMPLEMENTATION
#include "lang.h"
#define REPOS_IMPLEMENTATION
#include "repos.h"
#define ORDER_IMPLEMENTATION
#include "order.h"
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

#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080
#define MAX_GAMES 100
#define FPS 60

typedef struct {
    PlatformEntry *platform;
    GameEntry     *games;
    const int     *gameCount;
} System;

static System systems[] = {
    { &nes_platform,      nes_games,      &nes_game_count },
    { &snes_platform,     snes_games,     &snes_game_count },
    { &n64_platform,      n64_games,      &n64_game_count },
    { &gamecube_platform, gamecube_games, &gamecube_game_count },
};

static const int SYSTEM_COUNT = (int)(sizeof(systems) / sizeof(systems[0]));

static Texture2D gameBoxarts[MAX_GAMES];

static const int ASSET_MAX_BOXART_W = 512;
static const int ASSET_MAX_BOXART_H = 384;
static const float SLIDE_SPEED = 100.0f;
static const float SIDEBAR_RAIL_FRAC = 0.06f; // side bar size 
static const float SIDEBAR_OPEN_FRAC = 0.40f;
static const float SIDEBAR_OVERSHOOT = 0.08f; /* past target, then settle */
static const float SIDEBAR_WIDTH_LERP = 0.22f;
/* 3 full rows + ~10% of a fourth so the user sees more games below */
static const float MOSAIC_VISIBLE_ROWS = 3.1f;
static const float MOSAIC_GROW_DELAY_SEC = 0.3f;
static const float INFO_BAR_FADE_SEC = 0.4f;
static const float INFO_BAR_SLIDE_PX = 13.0f;
static const float MOSAIC_SELECT_SCALE = 1.18f;
static const float MOSAIC_LIFT_SCALE = 1.30f;   /* Y Move: lifted tile */
static const float DETAILS_SLIDE_SEC = 0.28f;
static const float DETAILS_WIDTH_FRAC = 0.34f;


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
    int slotW;     /* rounded cell size (nav / legacy int math) */
    int slotH;
    float cellW;   /* exact cell size: cols * cellW == contentW */
    float cellH;
    int slotsPerRow;
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

static int ActiveGameCount(void) {
    return *ActiveSystem()->gameCount;
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

GridLayout GetGridLayout(void) {
    GridLayout g;
    g.screenW = GetScreenWidth();
    g.screenH = GetScreenHeight();
    g.originX = (int)SidebarRailWidth();
    if (g.originX < 0) g.originX = 0;
    g.contentW = g.screenW - g.originX;
    if (g.contentW < 1) g.contentW = 1;

    /* Grid cell aspect from platform global orient (games orient fits inside). */
    float aspect = Nso_OrientAspect(ActivePlatformOrient());

    float idealH = (float)g.screenH / MOSAIC_VISIBLE_ROWS;
    float idealW = idealH * aspect;
    if (idealW < 1.0f) idealW = 1.0f;

    int cols = (int)((float)g.contentW / idealW);
    if (cols < 1) cols = 1;

    /* Fill content width exactly; preserve aspect. Bump column count if tiles
     * grow so tall that fewer than ~3.1 rows would be visible. */
    float cellW = 1.0f;
    float cellH = 1.0f;
    for (;;) {
        cellW = (float)g.contentW / (float)cols;
        cellH = cellW / aspect;
        if (cellH < 1.0f) cellH = 1.0f;

        float visibleRows = (float)g.screenH / cellH;
        if (visibleRows + 0.001f >= MOSAIC_VISIBLE_ROWS || cols >= g.contentW) {
            break;
        }
        cols++;
    }

    g.slotsPerRow = cols;
    g.cellW = cellW;
    g.cellH = cellH;
    g.slotW = (int)(cellW + 0.5f);
    g.slotH = (int)(cellH + 0.5f);
    if (g.slotW < 1) g.slotW = 1;
    if (g.slotH < 1) g.slotH = 1;
    return g;
}

float GridMaxScroll(const GridLayout *g, int itemCount) {
    if (!g || itemCount <= 0) {
        return 0.0f;
    }

    int totalRows = (itemCount + g->slotsPerRow - 1) / g->slotsPerRow;
    /* Full rows that fit; partial 4th row peeks without counting as "full". */
    int visibleRows = (int)((float)g->screenH / g->cellH);
    if (visibleRows < 1) {
        visibleRows = 1;
    }

    if (totalRows <= visibleRows) {
        return 0.0f;
    }

    return (float)(totalRows - visibleRows) * g->cellH;
}

float GridTargetViewportY(const GridLayout *g, int selectedIndex, int itemCount) {
    if (!g || itemCount <= 0) {
        return 0.0f;
    }

    int totalRows = (itemCount + g->slotsPerRow - 1) / g->slotsPerRow;
    int visibleRows = (int)((float)g->screenH / g->cellH);

    if (visibleRows < 1) {
        visibleRows = 1;
    }

    if (totalRows <= visibleRows) {
        return 0.0f;
    }

    int selectedRow = selectedIndex / g->slotsPerRow;
    int topRow = 0;

    if (selectedRow >= visibleRows) {
        topRow = selectedRow - visibleRows + 1;
    }

    int maxTopRow = totalRows - visibleRows;

    if (topRow > maxTopRow) {
        topRow = maxTopRow;
    }

    return (float)topRow * g->cellH;
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

/* Installed = the game's binary exists (built / downloaded). */
static bool GameBinaryPresent(const GameEntry *game) {
    char path[2304];
    if (!game || !game->dir || !game->dir[0] || !game->binary || !game->binary[0]) return false;
    snprintf(path, sizeof(path), "%s/%s/%s", GetBaseDirectory(), game->dir, game->binary);
    return FileExists(path);
}

/* Grid tint: games that are not installed yet are dimmed. */
static Color GameTint(int gameIdx) {
    return GameBinaryPresent(&ActiveGames()[gameIdx]) ? WHITE : (Color){ 110, 110, 110, 255 };
}

/* gameIdx is local to the active system */
static Image LoadBoxartImage(const char *path);

Texture2D GetBoxart(int gameIdx) {
    int count = ActiveGameCount();
    if (gameIdx < 0 || gameIdx >= count || gameIdx >= MAX_GAMES) {
        return (Texture2D){0};
    }

    if (gameBoxarts[gameIdx].id != 0) {
        return gameBoxarts[gameIdx];
    }

    Image img = LoadBoxartImage(GetDataPath(ActiveGames()[gameIdx].boxartPath));
    if (!img.data) {
        return (Texture2D){0};
    }

    gameBoxarts[gameIdx] = LoadTextureFromImage(img);
    UnloadImage(img);

    return gameBoxarts[gameIdx];
}

/* CPU-only cover decode + downscale (no GL, safe on the loader thread). */
static Image LoadBoxartImage(const char *path) {
    if (!path || path[0] == '\0') {
        return (Image){0};
    }

    Image img = LoadImage(path);
    if (!img.data) {
        return img;
    }

    if (img.width > ASSET_MAX_BOXART_W || img.height > ASSET_MAX_BOXART_H) {
        float sx = (float)ASSET_MAX_BOXART_W / (float)img.width;
        float sy = (float)ASSET_MAX_BOXART_H / (float)img.height;
        float s = fminf(sx, sy);
        if (s < 1.0f) {
            int nw = (int)(img.width * s + 0.5f);
            int nh = (int)(img.height * s + 0.5f);
            if (nw < 1) nw = 1;
            if (nh < 1) nh = 1;
            ImageResize(&img, nw, nh);
        }
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
    for (int i = 0; i < MAX_GAMES; i++) {
        UnloadBoxart(i);
    }
}

/* Ensure boxarts exist; do not unload while browsing (full preload on enter). */
void ManageVisibleBoxarts(int gameCount, float viewportY) {
    (void)viewportY;
    if (gameCount <= 0) return;

    for (int i = 0; i < gameCount && i < MAX_GAMES; i++) {
        if (gameBoxarts[i].id == 0) {
            GetBoxart(i);
        }
    }
}

/* ---- Background cover loader (used by the loading splash) ----
 * A worker thread decodes/resizes cover images (LoadImage, CPU only); the
 * main thread uploads finished ones with LoadTextureFromImage, since GL
 * textures must be created on the main thread. */
static struct {
    pthread_t thread;
    bool running;              /* thread started and not yet joined */
    int count;
    char *paths[MAX_GAMES];
    Image images[MAX_GAMES];
    atomic_int ready[MAX_GAMES];  /* 1 = images[i] decoded (maybe empty) */
    bool uploaded[MAX_GAMES];
    atomic_bool cancel;
} coverLoader;

static void *CoverLoaderThread(void *arg) {
    (void)arg;
    for (int i = 0; i < coverLoader.count; i++) {
        if (atomic_load(&coverLoader.cancel)) break;
        coverLoader.images[i] = LoadBoxartImage(coverLoader.paths[i]);
        atomic_store(&coverLoader.ready[i], 1);
    }
    return NULL;
}

/* Join the worker and free anything not handed to the GPU. */
static void CoverLoader_Stop(void) {
    if (coverLoader.running) {
        atomic_store(&coverLoader.cancel, true);
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
}

static void CoverLoader_Start(void) {
    CoverLoader_Stop();
    int count = ActiveGameCount();
    if (count > MAX_GAMES) count = MAX_GAMES;
    for (int i = 0; i < count; i++) {
        coverLoader.paths[i] = strdup(GetDataPath(ActiveGames()[i].boxartPath));
        coverLoader.images[i] = (Image){0};
        atomic_store(&coverLoader.ready[i], 0);
        coverLoader.uploaded[i] = false;
    }
    coverLoader.count = count;
    atomic_store(&coverLoader.cancel, false);
    coverLoader.running = pthread_create(&coverLoader.thread, NULL,
                                         CoverLoaderThread, NULL) == 0;
    if (!coverLoader.running) {
        coverLoader.count = 0; /* fall back: grid loads covers itself */
        for (int i = 0; i < count; i++) { free(coverLoader.paths[i]); coverLoader.paths[i] = NULL; }
    }
}

/* Upload decoded covers; returns true when every cover is on the GPU. */
static bool CoverLoader_Poll(void) {
    bool done = true;
    for (int i = 0; i < coverLoader.count; i++) {
        if (coverLoader.uploaded[i]) continue;
        if (!atomic_load(&coverLoader.ready[i])) { done = false; continue; }
        if (coverLoader.images[i].data) {
            gameBoxarts[i] = LoadTextureFromImage(coverLoader.images[i]);
            UnloadImage(coverLoader.images[i]);
        }
        coverLoader.images[i] = (Image){0};
        coverLoader.uploaded[i] = true;
    }
    if (done) CoverLoader_Stop();
    return done;
}

static void InitSystems(void) {
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        Nso_PlatformInit(systems[i].platform);
    }
}

static void UnloadSystems(void) {
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        Nso_PlatformUnload(systems[i].platform);
    }
}

typedef struct {
    float duration;
    float startScale;
    float startAlpha;
    float endAlpha;
    Color color;

    float timer;
    float screenW;
    float screenH;
    bool  finished;
} ScreenFadeAnimation;

void InitScreenFadeAnimation(ScreenFadeAnimation *anim,
                             float duration,
                             float startScale,
                             float startAlpha,
                             float endAlpha,
                             Color color) {
    anim->duration   = duration;
    anim->startScale = startScale;
    anim->startAlpha = startAlpha;
    anim->endAlpha   = endAlpha;
    anim->color      = color;
    anim->timer      = 0.0f;
    anim->screenW    = (float)GetScreenWidth();
    anim->screenH    = (float)GetScreenHeight();
    anim->finished   = false;
}

void UpdateScreenFadeAnimation(ScreenFadeAnimation *anim, float deltaTime) {
    if (anim->finished) return;
    anim->timer += deltaTime;
    if (anim->timer >= anim->duration) {
        anim->timer = anim->duration;
        anim->finished = true;
    }
}

void DrawScreenFadeAnimation(const ScreenFadeAnimation *anim) {
    if (anim->duration <= 0.0f) return;

    float progress = anim->timer / anim->duration;
    if (progress > 1.0f) progress = 1.0f;

    float startW = anim->screenW * anim->startScale;
    float startH = anim->screenH * anim->startScale;
    float startX = (anim->screenW - startW) * 0.5f;
    float startY = (anim->screenH - startH) * 0.5f;

    float x = startX + (0.0f - startX) * progress;
    float y = startY + (0.0f - startY) * progress;
    float w = startW + (anim->screenW - startW) * progress;
    float h = startH + (anim->screenH - startH) * progress;

    float alpha = anim->startAlpha + (anim->endAlpha - anim->startAlpha) * progress;
    unsigned char alphaByte = (unsigned char)(alpha * 255.0f);

    Color drawColor = {anim->color.r, anim->color.g, anim->color.b, alphaByte};
    DrawRectangleRec((Rectangle){x, y, w, h}, drawColor);
}

bool IsScreenFadeAnimationFinished(const ScreenFadeAnimation *anim) {
    return anim->finished;
}

typedef enum State {
    LAUNCHING,
    SPLASH,     /* full-screen system art fading in before the game grid */
    SYSTEMS,
    MOSAIC,
    MANAGE,     /* gear: box-art scraping + repository download */
} State;

static State currentState = SYSTEMS;

static int GameIdx = 0;

/* Y Move (hold): selected game follows the arrows; saved on release. */
static bool gridLifted = false;
static bool gridLiftMoved = false;

/* X Details panel: slides in from the right (detailsT 0..1). */
static bool detailsOpen = false;
static float detailsT = 0.0f;

static float viewportY = 0.0f;
static float targetViewportY = 0.0f;

static bool isLaunching = false;
static int launchingGameIdx = 0;

static NsoProc gamePid;
static bool waitingForGame = false;

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
} ScrapePhase;

static ScrapePhase scrapePhase = SCRAPE_IDLE;
static bool scrapeBusy = false;
static bool scrapeCancel = false;
static bool scrapeReplace = false;
static int scrapeRow = 0;
static int scrapeGame = 0;
static int scrapeMisses = 0;
static int scrapeActive = -1;
static NsoProc scrapeProc;
static char **scrapeNames = NULL;
static int scrapeNameCount = 0;
static char scrapeId[32];
static char scrapePart[2200];
static char scrapeDest[2200];
static NsoMsg scrapeMsg = { -1, 0, "" };

/* Gear spin / strip morph from sidebar width progress (rail→open maps to 0→1). */
static float gearSpinAngle = 0.0f;
static const float GEAR_STRIP_PAD = 8.0f;
static const float GEAR_STRIP_ROUNDNESS = 0.35f;

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

static ScreenFadeAnimation launchFadeAnim;

/* Mosaic enter intro: delay → selected grow → info bar available for fade-in */
typedef enum {
    MOSAIC_INTRO_NONE = 0,
    MOSAIC_INTRO_DELAY,
    MOSAIC_INTRO_GROW,
} MosaicIntroPhase;

static MosaicIntroPhase mosaicIntroPhase = MOSAIC_INTRO_NONE;
static float mosaicIntroTimer = 0.0f;

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
    /* future: INFO_BAR_ORG, INFO_BAR_DETAILS, ... */
    INFO_BAR_COUNT
} InfoBarId;

static float infoBarOpacity[INFO_BAR_COUNT];
static InfoBarId infoBarActive = INFO_BAR_NONE;
static InfoBarId infoBarWanted = INFO_BAR_NONE;

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

static const char *InfoBar_Text(InfoBarId id) {
    switch (id) {
        case INFO_BAR_MOSAIC:  return T(S_MOSAIC_INFO);
        case INFO_BAR_SIDEBAR:
            return scrapeBusy ? T(S_INFO_CANCEL) : T(S_SIDEBAR_INFO);
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
    for (int i = 0; i < gameCount; i++) {
        float target = (i == GameIdx && growSelected)
            ? (gridLifted ? MOSAIC_LIFT_SCALE : MOSAIC_SELECT_SCALE) : 1.0f;
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
static const float SPLASH_MIN_HOLD_SEC = 0.2f;
static float splashTimer = 0.0f;   /* time since the fade completed */


static bool screenSliding = false;
static float slideOffset = 0.0f;
static int slideDirection = 0; /* -1 enter mosaic, +1 back to systems */


static Texture2D gearTexture = {0};

/* Systems menu focus: 0 = system cards, 1 = gear (Down / Up). */
static int menuFocus = 0;

static Color MenuGlow(void) {
    float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
    return ColorLerp((Color){ 80, 200, 255, 255 }, (Color){ 160, 110, 255, 255 }, pulse);
}

/* Settings gear, bottom centre like the Switch home's bottom icon row. */
static void DrawMenuGear(float offsetX) {
    if (gearTexture.id == 0) return;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float d = sh * 0.075f;                       /* icon disc diameter */
    float cx = offsetX + sw * 0.5f;
    float cy = sh - sh * 0.12f;
    float g = d * 0.72f;
    DrawCircleV((Vector2){ cx, cy }, d * 0.5f, WHITE);
    DrawTexturePro(gearTexture, (Rectangle){ 0, 0, (float)gearTexture.width, (float)gearTexture.height },
                   (Rectangle){ cx, cy, g, g }, (Vector2){ g * 0.5f, g * 0.5f }, 0.0f, WHITE);
    if (menuFocus == 1) {
        DrawRing((Vector2){ cx, cy }, d * 0.5f + d * 0.05f, d * 0.5f + d * 0.11f, 0.0f, 360.0f, 48, MenuGlow());
    }
}

/* Draw the systems menu shifted by offsetX (used by the mosaic slide).
 * Every system card, plus the selection ring, sits inside a rectangle a
 * quarter of the screen (half width x half height) centred on screen. */
static void DrawSystemMenu(float offsetX) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    Rectangle box = { offsetX + sw * 0.25f, sh * 0.25f, sw * 0.5f, sh * 0.5f };

    DrawRectangleRec((Rectangle){ offsetX, 0.0f, sw, sh }, MENU_BG);

    float gap = box.width * 0.025f;
    float pad = box.width * 0.02f; /* room for the selection ring */
    float card = fminf((box.width - 2.0f * pad - gap * (SYSTEM_COUNT - 1)) / SYSTEM_COUNT,
                       box.height - 2.0f * pad);
    float rowW = card * SYSTEM_COUNT + gap * (SYSTEM_COUNT - 1);
    float x0 = box.x + (box.width - rowW) * 0.5f;
    float y = box.y + (box.height - card) * 0.5f;
    float r = card * MENU_CARD_ROUNDNESS * 0.5f; /* corner radius in px */

    for (int i = 0; i < SYSTEM_COUNT; i++) {
        const PlatformEntry *p = systems[i].platform;
        Rectangle dst = { x0 + (float)i * (card + gap), y, card, card };

        DrawRectangleRounded(dst, MENU_CARD_ROUNDNESS, 16, p->bg);

        if (p->texture.id != 0) {
            /* Square centre crop of the 16:9 art. Draw it as a "+" of two
             * bands so the rounded corners stay the solid card colour. */
            float side = (float)p->texture.height;
            Rectangle src = { ((float)p->texture.width - side) * 0.5f, 0.0f, side, side };
            float k = side / card;
            DrawTexturePro(p->texture,
                           (Rectangle){ src.x, src.y + r * k, side, side - 2.0f * r * k },
                           (Rectangle){ dst.x, dst.y + r, card, card - 2.0f * r },
                           (Vector2){ 0 }, 0.0f, WHITE);
            DrawTexturePro(p->texture,
                           (Rectangle){ src.x + r * k, src.y, side - 2.0f * r * k, side },
                           (Rectangle){ dst.x + r, dst.y, card - 2.0f * r, card },
                           (Vector2){ 0 }, 0.0f, WHITE);
        }

        if (i == currentSystem && menuFocus == 0) {
            /* Black gap, then a glowing ring that pulses cyan <-> violet. */
            float gapT = card * 0.015f;
            float ringT = card * 0.02f;
            Color glow = MenuGlow();
            Rectangle outer = { dst.x - gapT, dst.y - gapT, card + 2.0f * gapT, card + 2.0f * gapT };
            DrawRectangleRoundedLinesEx(dst, MENU_CARD_ROUNDNESS, 16, gapT, BLACK);
            DrawRectangleRoundedLinesEx(outer, 2.0f * (r + gapT) / outer.width, 16, ringT, glow);
        }
    }
    DrawMenuGear(offsetX);
}

/* opacity 0..1; ySlide = (1-opacity)*INFO_BAR_SLIDE_PX so close is reverse of open.
 * Far-right aligned; 10px margin between text and rounded border. */
static void DrawInfoBarEx(Font font, const char *text, float opacity, float ySlide) {
    if (opacity <= 0.001f || !text) return;

    const float textPad = 10.0f;
    float fontSize = (float)font.baseSize;
    float spacing = 2.0f;
    Vector2 textSize = MeasureTextEx(font, text, fontSize, spacing);
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

    DrawTextEx(font,
               text,
               textPosition,
               fontSize,
               spacing,
               Fade(WHITE, opacity));
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

static void DrawBlurredMosaicBackdrop(Color clearColor) {
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
            float dir[2] = { 1.0f / hw, 0.0f };
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
            float dir[2] = { 0.0f, 1.0f / hh };
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
            float dir[2] = { 1.5f / hw, 0.0f };
            SetShaderValue(blurShader, blurDirLoc, dir, SHADER_UNIFORM_VEC2);
            DrawRT(blurRT_A.texture, (Rectangle){0, 0, hw, hh}, WHITE);
        }
        EndShaderMode();
        EndTextureMode();

        BeginTextureMode(blurRT_A);
        ClearBackground(clearColor);
        BeginShaderMode(blurShader);
        {
            float dir[2] = { 0.0f, 1.5f / hh };
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

static void ScrapeMenu_ClearNames(void)
{
    Scrape_FreeNames(scrapeNames, scrapeNameCount);
    scrapeNames = NULL;
    scrapeNameCount = 0;
}

static void ScrapeMenu_Idle(int status)
{
    if (scrapePart[0] != '\0') FileRemove(scrapePart);
    ScrapeMenu_ClearNames();
    scrapeProc = NsoProcNone();
    scrapePhase = SCRAPE_IDLE;
    scrapeBusy = false;
    scrapeCancel = false;
    scrapePart[0] = '\0';
    if (status >= 0) Msg_Set(&scrapeMsg, status, 0, NULL);
}

static int ScrapeMenu_SpawnCurl(const char *url, const char *outPath)
{
    char *argv[8];
    snprintf(scrapePart, sizeof(scrapePart), "%s", outPath);
    argv[0] = "curl";
    argv[1] = "-fsSL";
    argv[2] = "--max-time";
    argv[3] = "30";
    argv[4] = "-o";
    argv[5] = scrapePart;
    argv[6] = (char *)url;
    argv[7] = NULL;
    scrapeProc = NsoSpawnArgv(NULL, argv);
    return NsoProcOk(scrapeProc);
}

static void ScrapeMenu_Advance(void)
{
    int count = ActiveGameCount();
    GameEntry *games = ActiveGames();
    const char *playlist = Scrape_Playlist(scrapeId);

    while (scrapeGame < count) {
        GameEntry *game = &games[scrapeGame];
        char dest[2048];
        char part[2064];
        char url[2048];
        const char *file;
        int present;

        {
            char prog[160];
            snprintf(prog, sizeof(prog), "%s  %d/%d", game->title ? game->title : "", scrapeGame + 1, count);
            Msg_Set(&scrapeMsg, S_RAW, 0, prog);
        }
        if (!game->boxartPath || game->boxartPath[0] == '\0') {
            scrapeGame++;
            continue;
        }
        snprintf(dest, sizeof(dest), "%s", GetDataPath(game->boxartPath));
        present = FileExists(dest) && GetFileLength(dest) > 0;
        if (!scrapeReplace && present) {
            scrapeGame++;
            continue;
        }
        file = Scrape_Match(game->title, (const char **)scrapeNames, scrapeNameCount);
        if (!file || !playlist || Scrape_ImageUrl(url, sizeof(url), playlist, file) != 0) {
            scrapeMisses++;
            scrapeGame++;
            continue;
        }
        snprintf(part, sizeof(part), "%s.part", dest);
        snprintf(scrapeDest, sizeof(scrapeDest), "%s", dest);
        scrapeActive = scrapeGame;
        scrapePhase = SCRAPE_FILE;
        if (!ScrapeMenu_SpawnCurl(url, part)) {
            scrapeMisses++;
            scrapeGame++;
            continue;
        }
        return;
    }

    if (scrapeMisses > 0) {
        Msg_Set(&scrapeMsg, S_SCRAPE_DONE_MISSED, scrapeMisses, NULL);
    } else {
        Msg_Set(&scrapeMsg, S_SCRAPE_DONE, 0, NULL);
    }
    ScrapeMenu_ClearNames();
    scrapeProc = NsoProcNone();
    scrapePhase = SCRAPE_IDLE;
    scrapeBusy = false;
    scrapePart[0] = '\0';
}

static void ScrapeMenu_Start(int replace)
{
    const char *playlist;
    char cache[2048];
    char indexPath[2200];
    char url[1024];
    int count;
    int need = 0;

    if (scrapeBusy) return;
    playlist = Scrape_Playlist(ActiveSystem()->platform->id);
    if (!playlist) {
        Msg_Set(&scrapeMsg, S_SCRAPE_NO_SOURCE, 0, NULL);
        return;
    }
    count = ActiveGameCount();
    if (!replace) {
        for (int i = 0; i < count; i++) {
            const char *rel = ActiveGames()[i].boxartPath;
            char path[2048];
            if (!rel || rel[0] == '\0') continue;
            snprintf(path, sizeof(path), "%s", GetDataPath(rel));
            if (!FileExists(path) || GetFileLength(path) <= 0) need = 1;
        }
        if (!need) {
            Msg_Set(&scrapeMsg, S_SCRAPE_DONE, 0, NULL);
            return;
        }
    }
    snprintf(scrapeId, sizeof(scrapeId), "%s", ActiveSystem()->platform->id);
    scrapeReplace = replace;
    scrapeMisses = 0;
    scrapeCancel = false;
    snprintf(cache, sizeof(cache), "%s", GetDataPath("assets/cache"));
    MakeDirectory(cache);
    snprintf(indexPath, sizeof(indexPath), "%s/index.json.part", cache);
    if (Scrape_IndexUrl(url, sizeof(url), playlist) != 0) {
        Msg_Set(&scrapeMsg, S_SCRAPE_DOWNLOAD_FAILED, 0, NULL);
        return;
    }
    scrapeBusy = true;
    scrapePhase = SCRAPE_INDEX;
    Msg_Set(&scrapeMsg, S_SCRAPE_INDEX, 0, NULL);
    if (!ScrapeMenu_SpawnCurl(url, indexPath)) {
        scrapeBusy = false;
        scrapePhase = SCRAPE_IDLE;
        Msg_Set(&scrapeMsg, S_SCRAPE_NO_CURL, 0, NULL);
    }
}

static void ScrapeMenu_Cancel(void)
{
    if (!scrapeBusy) return;
    scrapeCancel = true;
    if (NsoProcOk(scrapeProc)) NsoSpawnKill(scrapeProc);
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
        if (ok) {
            FileRemove(scrapeDest);
            if (FileMove(scrapePart, scrapeDest) != 0) {
                FileRemove(scrapePart);
                scrapeMisses++;
            } else if (scrapeActive >= 0) {
                UnloadBoxart(scrapeActive);
                GetBoxart(scrapeActive);
            }
        } else {
            FileRemove(scrapePart);
            scrapeMisses++;
        }
        scrapePart[0] = '\0';
        scrapeGame++;
        ScrapeMenu_Advance();
    }
}

/* Sidebar (organization mode) header only; the scrape rows moved to the
 * systems-menu panel below. */
static void SidebarHeader_Draw(Font font)
{
    float x = SidebarRailWidth() + 28.0f;
    if (!SidebarMenuSettled()) return;
    BeginScissorMode(0, 0, (int)sidebarWidth, GetScreenHeight());
    DrawTextEx(font, ActiveSystem()->platform->name, (Vector2){x, 56.0f},
               (float)font.baseSize, 2.0f, WHITE);
    EndScissorMode();
}

/* ---- Repository download (gear screen). One background job at a time:
 * tools/repo_sync.sh checks deps, clones or fast-forwards (only when the
 * remote moved), checks the ROM and builds; it writes logs/repos/<name>.log
 * and a one-line logs/repos/<name>.status. ---- */
#define REPO_MAX 32
static NsoMsg repoMsg[REPO_MAX];
static bool repoHasResult[REPO_MAX];
static int repoQueue[REPO_MAX];
static int repoQueueLen = 0;
static int repoRunning = -1;
static NsoProc repoProc;
static int repoPollTick = 0;

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

/* Disk-only state (no network) for repos without a job result yet. */
static void Repo_RefreshLocal(void) {
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) {
        const RepoEntry *r = &nso_repos[i];
        char dir[2048], path[2304];
        if (i == repoRunning || repoHasResult[i]) continue;
        bool queued = false;
        for (int q = 0; q < repoQueueLen; q++) queued |= repoQueue[q] == i;
        if (queued) continue;
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
        RepoLocalState ls = Repo_LocalState(exists, git, bin, r->rom != NULL, rom);
        Msg_Set(&repoMsg[i], (ls == REPO_LOCAL_COPY && bin) ? S_REPO_LOCAL_READY : Repo_LocalStr(ls), 0, NULL);
    }
}

static void Repo_StartNext(void) {
    static char a[9][2304];
    char *argv[11];
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
    argv[0] = "sh";
    for (int k = 0; k < 9; k++) argv[k + 1] = a[k];
    argv[10] = NULL;
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

static void Repo_Enqueue(int i) {
    if (i < 0 || i >= nso_repo_count || i >= REPO_MAX || i == repoRunning) return;
    for (int q = 0; q < repoQueueLen; q++) if (repoQueue[q] == i) return;
    repoQueue[repoQueueLen++] = i;
    repoHasResult[i] = true;
    Msg_Set(&repoMsg[i], S_REPO_QUEUED, 0, NULL);
    Repo_StartNext();
}

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
    repoRunning = -1;
    Repo_StartNext();
}

/* Repos of the active system, in list order. */
static int Manage_Repos(int *out, int max) {
    int n = 0;
    for (int i = 0; i < nso_repo_count && n < max; i++) {
        if (strcmp(nso_repos[i].system, ActiveSystem()->platform->id) == 0) out[n++] = i;
    }
    return n;
}

static int manageRow = 0;
static int manageReturnSystem = 0;

/* Rows: 0-1 scraping, one per repo of the system, "all from this system"
 * (if it has any), the global "all repositories" row, then Language. */
static int Manage_RowCount(void) {
    int idx[REPO_MAX];
    int n = Manage_Repos(idx, REPO_MAX);
    return 2 + n + (n > 0 ? 1 : 0) + 2;
}

static void Lang_ConfigPath(char *dst, size_t n) {
    snprintf(dst, n, "%s/config/nso.cfg", GetBaseDirectory());
}

static void Manage_Open(void) {
    manageReturnSystem = currentSystem;
    manageRow = 0;
    Repo_RefreshLocal();
    currentState = MANAGE;
    introFrames = 0;   /* same short fade-in as app open */
    introOpacity = 1.0f;
}

static void Manage_Close(void) {
    currentSystem = manageReturnSystem;
    menuFocus = 1;
    currentState = SYSTEMS;
    introFrames = 0;
    introOpacity = 1.0f;
}

static void Manage_Activate(void) {
    int idx[REPO_MAX];
    int n = Manage_Repos(idx, REPO_MAX);
    if (manageRow < 2) {
        scrapeRow = manageRow;
        ScrapeMenu_Start(manageRow == 1);
    } else if (manageRow < 2 + n) {
        Repo_Enqueue(idx[manageRow - 2]);
    } else if (n > 0 && manageRow == 2 + n) {
        for (int k = 0; k < n; k++) Repo_Enqueue(idx[k]);
    } else if (manageRow == Manage_RowCount() - 2) {
        for (int k = 0; k < nso_repo_count; k++) Repo_Enqueue(k);   /* all systems */
    } else {
        char dir[2048], cfg[2112];
        Lang_Set(Lang_Get() == LANG_EN ? LANG_PT : LANG_EN);
        snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
        MakeDirectory(dir);
        Lang_ConfigPath(cfg, sizeof(cfg));
        if (!Lang_Save(cfg, Lang_Get())) fprintf(stderr, "NSO: cannot save %s\n", cfg);
    }
}

static void Manage_DrawRow(Font font, Rectangle sec, float *y, int row, const char *left,
                           const char *right) {
    float size = (float)font.baseSize;
    float rowH = 54.0f;
    float x = sec.x + 28.0f;
    Color fg = WHITE;
    if (row == manageRow) {
        DrawRectangleRounded((Rectangle){ x - 14.0f, *y - 6.0f, sec.width - 28.0f, rowH },
                             0.35f, 8, WHITE);
        fg = BLACK;
    }
    DrawTextEx(font, left, (Vector2){ x, *y + 4.0f }, size, 2.0f, fg);
    if (right && right[0]) {
        float rs = size * 0.72f;
        Vector2 m = MeasureTextEx(font, right, rs, 2.0f);
        DrawTextEx(font, right, (Vector2){ sec.x + sec.width - 28.0f - m.x, *y + 4.0f + (size - rs) * 0.5f },
                   rs, 2.0f, row == manageRow ? DARKGRAY : LIGHTGRAY);
    }
    *y += rowH + 8.0f;
}

/* Full-screen gear screen: system tabs, then the two rounded sections. */
static void Manage_Draw(Font font) {
    const Color panel = { 52, 52, 52, 255 };
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float size = (float)font.baseSize;
    float small = size * 0.72f;
    float w = sw * 0.5f;
    float x0 = (sw - w) * 0.5f;
    float y = sh * 0.08f;
    int idx[REPO_MAX];
    int n = Manage_Repos(idx, REPO_MAX);

    ClearBackground(MENU_BG);

    /* System tabs (Left / Right). */
    float tabGap = 14.0f, tabW = (w - tabGap * (SYSTEM_COUNT - 1)) / SYSTEM_COUNT, tabH = 54.0f;
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        Rectangle t = { x0 + (float)i * (tabW + tabGap), y, tabW, tabH };
        bool on = i == currentSystem;
        DrawRectangleRounded(t, 0.5f, 12, on ? WHITE : panel);
        const char *nm = systems[i].platform->name;
        Vector2 m = MeasureTextEx(font, nm, size, 2.0f);
        DrawTextEx(font, nm, (Vector2){ t.x + (t.width - m.x) * 0.5f, t.y + (tabH - m.y) * 0.5f },
                   size, 2.0f, on ? BLACK : WHITE);
    }
    y += tabH + 32.0f;

    /* Image scraping. */
    char scrapeText[192];
    Msg_Format(&scrapeMsg, scrapeText, sizeof(scrapeText));
    float h1 = 28.0f + size + 18.0f + 2.0f * 62.0f + (scrapeText[0] ? small + 10.0f : 0.0f) + 20.0f;
    Rectangle s1 = { x0, y, w, h1 };
    DrawRectangleRounded(s1, 40.0f / h1, 16, panel);
    float ry = y + 28.0f;
    DrawTextEx(font, T(S_MANAGE_COVERS), (Vector2){ x0 + 28.0f, ry }, size, 2.0f, GRAY);
    ry += size + 18.0f;
    Manage_DrawRow(font, s1, &ry, 0, T(S_MANAGE_MISSING), NULL);
    Manage_DrawRow(font, s1, &ry, 1, T(S_MANAGE_REPLACE), NULL);
    if (scrapeText[0]) DrawTextEx(font, scrapeText, (Vector2){ x0 + 28.0f, ry }, small, 2.0f, WHITE);
    y += h1 + 24.0f;

    /* Repository download. */
    int rows = (n > 0 ? n + 1 : 0) + 1;
    float h2 = 28.0f + size + 18.0f + (float)rows * 62.0f + (n == 0 ? small + 16.0f : 0.0f) + 20.0f;
    Rectangle s2 = { x0, y, w, h2 };
    DrawRectangleRounded(s2, 40.0f / h2, 16, panel);
    ry = y + 28.0f;
    DrawTextEx(font, T(S_MANAGE_REPOS), (Vector2){ x0 + 28.0f, ry }, size, 2.0f, GRAY);
    ry += size + 18.0f;
    if (n == 0) {
        DrawTextEx(font, T(S_MANAGE_NO_REPOS), (Vector2){ x0 + 28.0f, ry }, small, 2.0f, WHITE);
        ry += small + 16.0f;
    }
    for (int k = 0; k < n; k++) {
        char st[192];
        Msg_Format(&repoMsg[idx[k]], st, sizeof(st));
        if (idx[k] == repoRunning) strncat(st, "...", sizeof(st) - strlen(st) - 1);
        Manage_DrawRow(font, s2, &ry, 2 + k, nso_repos[idx[k]].name, st);
    }
    if (n > 0) {
        char lbl[96];
        snprintf(lbl, sizeof(lbl), T(S_MANAGE_ALL_SYSTEM), ActiveSystem()->platform->name);
        Manage_DrawRow(font, s2, &ry, 2 + n, lbl, NULL);
    }
    {
        char st[64] = "";
        int pending = repoQueueLen + (repoRunning >= 0 ? 1 : 0);
        if (pending > 0) snprintf(st, sizeof(st), T(S_MANAGE_QUEUED_N), pending);
        Manage_DrawRow(font, s2, &ry, 2 + n + (n > 0 ? 1 : 0), T(S_MANAGE_ALL), st);
    }
    y += h2 + 24.0f;

    /* Settings: language (A toggles, saved to config/nso.cfg). */
    float h3 = 28.0f + size + 18.0f + 62.0f + 20.0f;
    Rectangle s3 = { x0, y, w, h3 };
    DrawRectangleRounded(s3, 40.0f / h3, 16, panel);
    ry = y + 28.0f;
    DrawTextEx(font, T(S_MANAGE_SETTINGS), (Vector2){ x0 + 28.0f, ry }, size, 2.0f, GRAY);
    ry += size + 18.0f;
    Manage_DrawRow(font, s3, &ry, Manage_RowCount() - 1, T(S_MANAGE_LANGUAGE), T(S_LANG_NAME));

    DrawInfoBarEx(font, scrapeBusy ? T(S_INFO_CANCEL) : T(S_MANAGE_INFO), 1.0f, 0.0f);
}

static void DrawSidebar(Color systemBg, Font font) {
    /* Draw down to 1px width; hide only when thinner than 1px. */
    if (sidebarWidth < 1.0f) return;

    int screenH = GetScreenHeight();
    int sw = (int)sidebarWidth;
    float railW = SidebarRailWidth();

    /* Blurred mosaic is drawn behind when expanded. */
    DrawRectangle(0, 0, sw, screenH, systemBg);
    SidebarHeader_Draw(font);

    if (gearTexture.id != 0) {
        float gearScale = 0.08f;
        float gearW = gearTexture.width * gearScale;
        float gearH = gearTexture.height * gearScale;
        float pad = GEAR_STRIP_PAD;

        /* Gear stays centered on the rail column. */
        float gearCX = railW * 0.5f;
        float gearCY = (float)screenH - gearH * 0.5f - 72.0f;

        float stripY = gearCY - gearH * 0.5f - pad;
        float stripH = gearH + pad * 2.0f;

        /* Morph: rail = padded rounded; open = full-width sharp (no pad). */
        float t = SidebarExpandProgress();
        float curPad = pad * (1.0f - t);
        float roundness = GEAR_STRIP_ROUNDNESS * (1.0f - t);
        float stripX = curPad;
        float stripW = sidebarWidth - curPad * 2.0f;
        if (stripW >= 1.0f) {
            Rectangle strip = { stripX, stripY, stripW, stripH };
            if (roundness < 0.001f) {
                DrawRectangleRec(strip, WHITE);
            } else {
                DrawRectangleRounded(strip, roundness, 8, WHITE);
            }
        }

        /* Gear with spin (DrawTexturePro rotates around origin). */
        Rectangle src = { 0, 0, (float)gearTexture.width, (float)gearTexture.height };
        Rectangle dst = { gearCX, gearCY, gearW, gearH };
        Vector2 origin = { gearW * 0.5f, gearH * 0.5f };
        DrawTexturePro(gearTexture, src, dst, origin, gearSpinAngle, WHITE);
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

static void DrawSlotImages(const GridLayout *layout, Texture2D *images, int numImgs) {
    if (numImgs <= 0 || !layout) return;

    int origin_x = layout->originX;
    int content_w = layout->contentW;
    int cols = layout->slotsPerRow;
    int screen_h = layout->screenH;
    float cellW = layout->cellW;
    float cellH = layout->cellH;
    float v_offset = viewportY;

    if (cols < 1 || cellW <= 0.0f || cellH <= 0.0f) return;

    for (int i = 0; i < numImgs; i++) {
        if (i == GameIdx) continue;

        int col = i % cols;
        int row = i / cols;
        float x = (float)origin_x + (float)col * cellW;
        float y = (float)row * cellH;

        float fitW, fitH;
        FitAspectInCell(cellW, cellH, Nso_OrientAspect(GameOrientAt(i)), &fitW, &fitH);

        float tile_scale = animScales[i];
        float draw_y = y - v_offset;
        float draw_w = fitW * tile_scale;
        float draw_h = fitH * tile_scale;
        float ox = x + (cellW - draw_w) * 0.5f;
        float oy = draw_y + (cellH - draw_h) * 0.5f;

        if (draw_y + cellH > 0.0f && draw_y < (float)screen_h) {
            DrawRectangleLinesEx(
                (Rectangle){ox - 0.5f, oy - 0.5f, draw_w + 1.0f, draw_h + 1.0f},
                1.0f,
                BLACK);

            DrawTexturePro(images[i],
                           (Rectangle){0, 0, (float)images[i].width, (float)images[i].height},
                           (Rectangle){ox, oy, draw_w, draw_h},
                           (Vector2){0, 0},
                           0.0f,
                           GameTint(i));
            DrawMissingCover(images[i], (Rectangle){ox, oy, draw_w, draw_h}, GameTint(i));
        }
    }

    if (GameIdx >= 0 && GameIdx < numImgs) {
        int col = GameIdx % cols;
        int row = GameIdx / cols;
        float x = (float)origin_x + (float)col * cellW;
        float y = (float)row * cellH;

        float fitW, fitH;
        FitAspectInCell(cellW, cellH, Nso_OrientAspect(GameOrientAt(GameIdx)),
                        &fitW, &fitH);

        float scale = animScales[GameIdx];
        float scaled_w = fitW * scale;
        float scaled_h = fitH * scale;
        float draw_y = y - v_offset;
        float draw_x = x + (cellW - scaled_w) * 0.5f;
        float draw_y_scaled = draw_y + (cellH - scaled_h) * 0.5f;
        ClampTileToContent(&draw_x, &draw_y_scaled, scaled_w, scaled_h,
                           origin_x, content_w, screen_h, MOSAIC_CURSOR_PAD);

        if (gridLifted) {
            /* Picked up: raised a little over a drop shadow. */
            draw_y_scaled -= 8.0f;
            DrawRectangleRec((Rectangle){draw_x + 10.0f, draw_y_scaled + 14.0f, scaled_w, scaled_h},
                             Fade(BLACK, 0.55f));
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

        float pulse = sinf((float)GetTime() * 5.6f) * 0.5f + 0.5f;
        unsigned char r = (unsigned char)(255 * (1.0f - pulse));
        unsigned char g = (unsigned char)(255 * (1.0f - pulse));
        unsigned char b = (unsigned char)(255 * (1.0f - pulse));
        unsigned char a = (unsigned char)(255 * (1.0f - pulse * 0.95f));
        Color line_color = {r, g, b, a};

        if (gridLifted) {
            DrawRectangleLinesEx(
                (Rectangle){draw_x - 3.0f, draw_y_scaled - 3.0f, scaled_w + 6.0f, scaled_h + 6.0f},
                4.0f,
                (Color){ 255, 196, 0, 255 });
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

static bool OverlayActive(void) {
    return sidebarOpen || sidebarExpandPending || sidebarClosing
        || sbWidthPhase != SB_WIDTH_IDLE
        || mosaicExitPhase != MOSAIC_EXIT_NONE;
}

static bool GameIsLaunchable(const GameEntry *game) {
    return GameBinaryPresent(game);
}

/* ---- Save states overlay (grid: S, or X Details -> Save States) ----
 * Lists the .sav files in stateDir newest first with their .bmp thumbnails; file
 * operations go through savestate.h. */
#define MAX_STATES 64

typedef struct {
    char name[SS_NAME_MAX + 1];
    long mtime;
    Texture2D thumb;
} StateItem;

static StateItem states[MAX_STATES];
static int stateCount = 0;
static int stateSel = 0;
static bool statesOpen = false;
static char statesDir[1100];
static char stateLoadPath[1200];   /* consumed by the next LaunchGame */
static bool stateDeleteArmed = false;
static bool stateRenaming = false;
static char renameBuf[SS_NAME_MAX + 1];
static NsoMsg stateMsg = { -1, 0, "" };
/* Opened by a confirm press (Details): ignore confirm until it is released. */
static bool stateConfirmLatch = false;

static void States_Unload(void) {
    for (int i = 0; i < stateCount; i++) {
        if (states[i].thumb.id) UnloadTexture(states[i].thumb);
    }
    stateCount = 0;
}

static int StateNewerFirst(const void *a, const void *b) {
    const StateItem *x = a, *y = b;
    if (x->mtime != y->mtime) return (y->mtime > x->mtime) ? 1 : -1;
    return strcmp(y->name, x->name);
}

/* Rescan the directory; keep the selection on `keep` if it still exists. */
static void States_Refresh(const char *keep) {
    States_Unload();
    if (DirectoryExists(statesDir)) {
        FilePathList list = LoadDirectoryFilesEx(statesDir, ".sav", false);
        for (unsigned int i = 0; i < list.count && stateCount < MAX_STATES; i++) {
            const char *name = GetFileNameWithoutExt(list.paths[i]);
            if (!Ss_ValidName(name)) continue;
            StateItem *it = &states[stateCount];
            snprintf(it->name, sizeof(it->name), "%s", name);
            it->mtime = GetFileModTime(list.paths[i]);
            it->thumb = (Texture2D){0};
            char bmp[1200];
            if (Ss_Path(bmp, sizeof(bmp), statesDir, it->name, ".bmp") == 0 && FileExists(bmp)) {
                it->thumb = LoadTexture(bmp);
            }
            stateCount++;
        }
        UnloadDirectoryFiles(list);
    }
    qsort(states, (size_t)stateCount, sizeof(states[0]), StateNewerFirst);
    for (int i = 0; keep && i < stateCount; i++) {
        if (strcmp(states[i].name, keep) == 0) stateSel = i;
    }
    if (stateSel >= stateCount) stateSel = stateCount - 1;
    if (stateSel < 0) stateSel = 0;
}

static void States_Open(const GameEntry *game) {
    if (!game || !game->stateDir) return;
    snprintf(statesDir, sizeof(statesDir), "%s", GetDataPath(game->stateDir));
    stateSel = 0;
    stateDeleteArmed = false;
    stateRenaming = false;
    stateConfirmLatch = false;
    Msg_Clear(&stateMsg);
    States_Refresh(NULL);
    statesOpen = true;
    InfoBar_Request(INFO_BAR_NONE);
}

static void States_Close(void) {
    States_Unload();
    statesOpen = false;
    stateRenaming = false;
    InfoBar_Request(INFO_BAR_MOSAIC);
}

/* Returns true when the user picked a state to load (stateLoadPath set). */
static bool States_HandleInput(bool gpLeft, bool gpRight, bool gpConfirm, bool gpBack) {
    bool gp = IsGamepadAvailable(0);
    bool gpX = gp && IsGamepadButtonPressed(0, NSO_BTN_X);
    bool gpY = gp && IsGamepadButtonPressed(0, NSO_BTN_Y);
    StateItem *cur = (stateCount > 0) ? &states[stateSel] : NULL;
    bool confirmHeld = false;

    if (stateConfirmLatch) {
        confirmHeld = IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_A)
            || (gp && (IsGamepadButtonDown(0, NSO_BTN_A)
                       || IsGamepadButtonDown(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)));
        if (!confirmHeld) stateConfirmLatch = false;
    }

    if (stateRenaming) {
        int c;
        size_t len = strlen(renameBuf);
        while ((c = GetCharPressed()) != 0) {
            if (c >= 32 && c < 127 && len < SS_NAME_MAX) {
                renameBuf[len++] = (char)c;
                renameBuf[len] = '\0';
            }
        }
        if (IsKeyPressed(KEY_BACKSPACE) && len > 0) renameBuf[--len] = '\0';
        if (IsKeyPressed(KEY_TAB)) {
            stateRenaming = false;
            Msg_Set(&stateMsg, S_SS_RENAME_CANCELLED, 0, NULL);
        } else if (IsKeyPressed(KEY_ENTER) && cur) {
            char newName[SS_NAME_MAX + 1];
            snprintf(newName, sizeof(newName), "%s", renameBuf);
            if (!Ss_ValidName(newName)) {
                Msg_Set(&stateMsg, S_SS_INVALID_NAME, 0, NULL);
            } else if (Ss_Rename(statesDir, cur->name, newName) != 0) {
                Msg_Set(&stateMsg, S_SS_RENAME_FAILED, 0, NULL);
            } else {
                stateRenaming = false;
                Msg_Set(&stateMsg, S_SS_RENAMED, 0, NULL);
                States_Refresh(newName);
            }
        }
        return false;
    }

    bool del = IsKeyPressed(KEY_DELETE) || gpX;
    if (!del && GetKeyPressed() != 0) stateDeleteArmed = false;

    if (IsKeyPressed(KEY_B) || gpBack) {
        States_Close();
        return false;
    }
    if ((IsKeyPressed(KEY_LEFT) || gpLeft) && stateSel > 0) stateSel--;
    if ((IsKeyPressed(KEY_RIGHT) || gpRight) && stateSel + 1 < stateCount) stateSel++;
    if (!cur) return false;

    if (!confirmHeld && (IsKeyPressed(KEY_ENTER) || gpConfirm)) {
        if (Ss_Path(stateLoadPath, sizeof(stateLoadPath), statesDir, cur->name, ".sav") != 0) {
            stateLoadPath[0] = '\0';
            Msg_Set(&stateMsg, S_SS_PATH_TOO_LONG, 0, NULL);
            return false;
        }
        States_Close();
        return true;
    }
    if (del) {
        if (!stateDeleteArmed) {
            stateDeleteArmed = true;
            Msg_Set(&stateMsg, S_SS_CONFIRM_DELETE, 0, cur->name);
        } else {
            stateDeleteArmed = false;
            if (Ss_Delete(statesDir, cur->name) != 0) {
                Msg_Set(&stateMsg, S_SS_DELETE_FAILED, 0, NULL);
            } else {
                Msg_Set(&stateMsg, S_SS_DELETED, 0, NULL);
                States_Refresh(NULL);
            }
        }
    } else if (IsKeyPressed(KEY_C) || gpY) {
        char copyName[SS_NAME_MAX + 1];
        if (Ss_Copy(statesDir, cur->name, copyName, sizeof(copyName)) != 0) {
            Msg_Set(&stateMsg, S_SS_COPY_FAILED, 0, NULL);
        } else {
            Msg_Set(&stateMsg, S_SS_COPIED, 0, copyName);
            States_Refresh(copyName);
        }
    } else if (IsKeyPressed(KEY_R)) {
        stateRenaming = true;
        snprintf(renameBuf, sizeof(renameBuf), "%s", cur->name);
        while (GetCharPressed() != 0) {} /* drop the 'r' itself */
        Msg_Clear(&stateMsg);
    }
    return false;
}

static void States_Draw(Font font, const char *title) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float fs = (float)font.baseSize;
    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.82f));

    char head[160];
    snprintf(head, sizeof(head), T(S_SS_TITLE), title ? title : "");
    DrawTextEx(font, head, (Vector2){ sw * 0.08f, sh * 0.10f }, fs * 1.3f, 2.0f, WHITE);

    float cardW = 384.0f, cardH = 336.0f, gap = 48.0f;  /* 256x224 x1.5 */
    float cx = sw * 0.5f - cardW * 0.5f;
    float y = sh * 0.5f - cardH * 0.5f - 20.0f;

    if (stateCount == 0) {
        const char *t = T(S_SS_EMPTY);
        Vector2 m = MeasureTextEx(font, t, fs, 2.0f);
        DrawTextEx(font, t, (Vector2){ (sw - m.x) * 0.5f, sh * 0.5f - m.y }, fs, 2.0f, LIGHTGRAY);
    }
    for (int i = 0; i < stateCount; i++) {
        Rectangle r = { cx + (float)(i - stateSel) * (cardW + gap), y, cardW, cardH };
        if (r.x + r.width < 0.0f || r.x > sw) continue;
        if (states[i].thumb.id) {
            DrawTexturePro(states[i].thumb,
                           (Rectangle){ 0, 0, (float)states[i].thumb.width, (float)states[i].thumb.height },
                           r, (Vector2){ 0 }, 0.0f, WHITE);
        } else {
            DrawRectangleRec(r, DARKGRAY);
            DrawTextEx(font, T(S_SS_NO_IMAGE), (Vector2){ r.x + 20, r.y + 20 }, fs, 2.0f, LIGHTGRAY);
        }
        if (i == stateSel) {
            float pulse = 0.5f + 0.5f * sinf((float)GetTime() * 3.0f);
            Color glow = ColorLerp((Color){ 80, 200, 255, 255 }, (Color){ 160, 110, 255, 255 }, pulse);
            DrawRectangleRoundedLinesEx((Rectangle){ r.x - 6, r.y - 6, r.width + 12, r.height + 12 },
                                        0.04f, 8, 5.0f, glow);
        }
        const char *label = (i == stateSel && stateRenaming) ? TextFormat("%s|", renameBuf) : states[i].name;
        DrawTextEx(font, label, (Vector2){ r.x, r.y + r.height + 16 }, fs * 0.8f, 2.0f,
                   (i == stateSel && stateRenaming) ? YELLOW : WHITE);
        char when[64];
        time_t t = (time_t)states[i].mtime;
        struct tm *tm = localtime(&t);
        if (tm && strftime(when, sizeof(when), "%Y-%m-%d %H:%M", tm) > 0) {
            DrawTextEx(font, when, (Vector2){ r.x, r.y + r.height + 16 + fs }, fs * 0.65f, 2.0f, LIGHTGRAY);
        }
    }
    char msgText[256];
    Msg_Format(&stateMsg, msgText, sizeof(msgText));
    if (msgText[0]) {
        Vector2 m = MeasureTextEx(font, msgText, fs * 0.8f, 2.0f);
        DrawTextEx(font, msgText, (Vector2){ (sw - m.x) * 0.5f, y + cardH + fs * 2.6f }, fs * 0.8f, 2.0f, YELLOW);
    }
    DrawInfoBarEx(font, stateRenaming ? T(S_SS_INFO_RENAME) : T(S_SS_INFO),
                  1.0f, 0.0f);
}

/* ---- Y Move: user game order, config/order-<system id>.txt ---- */
static void Order_Path(int sys, char *dst, size_t n) {
    snprintf(dst, n, "%s/config/order-%s.txt", GetBaseDirectory(), systems[sys].platform->id);
}

/* At startup, before any cover is loaded: apply each system's saved order. */
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
        for (int i = 0; i < count; i++) titles[i] = games[i].title;
        Order_Perm(text, titles, count, perm);
        UnloadFileText(text);
        for (int k = 0; k < count; k++) tmp[k] = games[perm[k]];
        memcpy(games, tmp, sizeof(GameEntry) * (size_t)count);
    }
}

static void Order_SaveActive(void) {
    static char buf[MAX_GAMES * 256];
    const char *titles[MAX_GAMES];
    char dir[2048], path[2112];
    int count = ActiveGameCount();
    if (count <= 0 || count > MAX_GAMES) return;
    for (int i = 0; i < count; i++) titles[i] = ActiveGames()[i].title;
    if (Order_Format(buf, sizeof(buf), titles, count) != 0) {
        fprintf(stderr, "NSO: game order too long, not saved\n");
        return;
    }
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    MakeDirectory(dir);
    Order_Path(currentSystem, path, sizeof(path));
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

/* ---- X Details panel ---- */
static void Details_Open(void) {
    detailsOpen = true;
    Repo_RefreshLocal();
    InfoBar_Request(INFO_BAR_NONE);
}

static void Details_Close(void) {
    detailsOpen = false;
    InfoBar_Request(INFO_BAR_MOSAIC);
}

static void Details_Update(float dt) {
    float target = detailsOpen ? 1.0f : 0.0f;
    float step = dt / DETAILS_SLIDE_SEC;
    if (detailsT < target) detailsT = fminf(detailsT + step, target);
    else if (detailsT > target) detailsT = fmaxf(detailsT - step, target);
}

/* Repository entry that builds this game (same dir), or -1. */
static int Details_Repo(const GameEntry *g) {
    if (!g || !g->dir) return -1;
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) {
        if (strcmp(nso_repos[i].system, ActiveSystem()->platform->id) == 0
            && strcmp(nso_repos[i].dir, g->dir) == 0) return i;
    }
    return -1;
}

static void Details_HandleInput(bool gpConfirm, bool gpBack) {
    bool gp = IsGamepadAvailable(0);
    if (!detailsOpen) return;   /* sliding out */
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_X) || gpBack
        || (gp && IsGamepadButtonPressed(0, NSO_BTN_X))) {
        Details_Close();
        return;
    }
    if (GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    if (g->stateDir && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm)) {
        detailsOpen = false;
        detailsT = 0.0f;
        States_Open(g);   /* B there returns to the grid */
        stateConfirmLatch = true;   /* this press must not also Load */
    }
}

/* Draw `text` at most maxW wide (shrinks down to 60% of size). */
static float Details_Text(Font font, const char *text, Vector2 pos, float size, float maxW, Color c) {
    float s = size;
    while (s > size * 0.6f && MeasureTextEx(font, text, s, 2.0f).x > maxW) s -= 1.0f;
    DrawTextEx(font, text, pos, s, 2.0f, c);
    return s;
}

static void Details_Draw(Font font) {
    if (detailsT <= 0.0f || GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    float e = 1.0f - powf(1.0f - detailsT, 3.0f);   /* ease-out cubic */
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float fs = (float)font.baseSize;
    float w = sw * DETAILS_WIDTH_FRAC;
    float x = sw - w * e;
    float pad = 40.0f;
    float cx = x + pad, cw = w - 2.0f * pad, y = 60.0f;

    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.45f * e));
    DrawRectangleRec((Rectangle){ x, 0.0f, w, sh }, (Color){ 40, 40, 40, 245 });
    BeginScissorMode((int)x, 0, (int)w + 1, (int)sh);

    Texture2D tex = gameBoxarts[GameIdx];
    float cvW, cvH;
    FitAspectInCell(cw, sh * 0.36f, Nso_OrientAspect(GameOrientAt(GameIdx)), &cvW, &cvH);
    Rectangle cr = { cx + (cw - cvW) * 0.5f, y, cvW, cvH };
    if (tex.id) {
        DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height }, cr,
                       (Vector2){ 0 }, 0.0f, WHITE);
    } else {
        DrawRectangleRec(cr, DARKGRAY);
        DrawMissingCover(tex, cr, WHITE);
    }
    y += cvH + 30.0f;

    y += Details_Text(font, g->title ? g->title : "", (Vector2){ cx, y }, fs * 1.2f, cw, WHITE) + 30.0f;

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

    const char *labels[3] = { T(S_DET_SYSTEM), T(S_DET_SOURCE), T(S_DET_STATUS) };
    const char *values[3] = { ActiveSystem()->platform->name, source, status };
    for (int i = 0; i < 3; i++) {
        DrawTextEx(font, labels[i], (Vector2){ cx, y }, fs * 0.7f, 2.0f, GRAY);
        y += fs * 0.7f + 6.0f;
        Details_Text(font, values[i], (Vector2){ cx, y }, fs * 0.85f, cw, WHITE);
        y += fs * 0.85f + 22.0f;
    }
    y += 16.0f;

    if (g->stateDir) {
        DrawRectangleRounded((Rectangle){ cx - 14.0f, y - 6.0f, cw + 28.0f, 54.0f }, 0.35f, 8, WHITE);
        DrawTextEx(font, T(S_DET_SAVES), (Vector2){ cx, y + 4.0f }, fs, 2.0f, BLACK);
    } else {
        Details_Text(font, T(S_DET_NO_SAVES), (Vector2){ cx, y + 4.0f }, fs * 0.8f, cw, LIGHTGRAY);
    }
    EndScissorMode();

    DrawInfoBarEx(font, T(S_DET_INFO), e, (1.0f - e) * INFO_BAR_SLIDE_PX);
}

NsoProc LaunchGame(const GameEntry *game) {
    if (!GameIsLaunchable(game)) {
        fprintf(stderr, "LaunchGame: game is display-only or invalid\n");
        return NsoProcNone();
    }

    /* Games with save states get NSO_STATE_DIR (where to write states) and,
     * when resuming, NSO_STATE_LOAD (state to load at start). */
    bool withStates = game->stateDir && game->stateDir[0] != '\0';
    if (withStates) {
        char stateDir[1100];
        snprintf(stateDir, sizeof(stateDir), "%s", GetDataPath(game->stateDir));
        if (!DirectoryExists(stateDir) && MakeDirectory(stateDir) != 0) {
            fprintf(stderr, "LaunchGame: cannot create %s\n", stateDir);
        }
        NsoSetEnv("NSO_STATE_DIR", stateDir);
        NsoSetEnv("NSO_STATE_LOAD", stateLoadPath[0] ? stateLoadPath : NULL);
    }
    stateLoadPath[0] = '\0';

    /* Copy out of GetDataPath's static buffer before any further use. */
    char gameDir[2048];
    snprintf(gameDir, sizeof(gameDir), "%s", GetDataPath(game->dir));
    if (game->romImport) {
        NsoPrepareRomImport(gameDir, game->binary, game->romImport);
    }
    NsoProc proc = NsoSpawn(gameDir, game->binary);

    if (withStates) {
        NsoSetEnv("NSO_STATE_DIR", NULL);
        NsoSetEnv("NSO_STATE_LOAD", NULL);
    }
    return proc;
}

static void SelectSystem(int direction) {
    int next = currentSystem + direction;
    if (next >= 0 && next < SYSTEM_COUNT) currentSystem = next;
}

static void BeginMosaicEnterIntro(void) {
    for (int i = 0; i < MAX_GAMES; i++) {
        animScales[i] = 1.0f;
    }
    mosaicIntroPhase = MOSAIC_INTRO_DELAY;
    mosaicIntroTimer = MOSAIC_GROW_DELAY_SEC;
    InfoBar_Reset(); /* mosaic bar opens after grow */
}

static void StartEnterMosaic(void) {
    if (screenSliding || scrapeBusy) return;
    menuFocus = 0;

    int count = ActiveGameCount();
    if (count <= 0) return;

    GameIdx = 0;
    viewportY = 0.0f;
    targetViewportY = 0.0f;

    /* Covers load during the splash (UpdateSplash); start from a clean set. */
    UnloadAllBoxarts();
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

    /* Splash first: restart the app-open fade over the system's own art. */
    currentState = SPLASH;
    splashTimer = 0.0f;
    introFrames = 0;
    introOpacity = 1.0f;
}

/* Full-screen system art (letterboxed to fit, like the original menu). */
static void DrawSystemArt(float offsetX) {
    Texture2D tex = ActiveSystem()->platform->texture;
    if (tex.id == 0) return;
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float scale = fminf(sw / (float)tex.width, sh / (float)tex.height);
    float w = (float)tex.width * scale;
    float h = (float)tex.height * scale;
    DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height },
                   (Rectangle){ offsetX + (sw - w) * 0.5f, (sh - h) * 0.5f, w, h },
                   (Vector2){ 0 }, 0.0f, WHITE);
}

/* After the fade + hold, slide the art away to reveal the game grid. */
static void UpdateSplash(float dt) {
    if (currentState != SPLASH) return;

    bool loaded = CoverLoader_Poll();

    if (introOpacity > 0.0f) return;   /* fade still running */
    splashTimer += dt;
    if (loaded && splashTimer >= SPLASH_MIN_HOLD_SEC) {
        screenSliding = true;
        slideDirection = -1;
        slideOffset = 0.0f;
    }
}

static void DrawMosaicSlots(Color background) {
    ClearBackground(background);

    int count = ActiveGameCount();
    if (count > 0) {
        GridLayout drawGrid = GetGridLayout();
        Texture2D boxarts[MAX_GAMES];

        for (int i = 0; i < count; i++) {
            boxarts[i] = gameBoxarts[i];
        }

        DrawSlotImages(&drawGrid, boxarts, count);
    }
}

static void DrawMosaic(Color background, Font font) {
    Color SystemBG = GetColor(0xEE0010FF);

    if (SidebarWantsBlur()) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        EnsureBlurResources(sw, sh);

        if (mosaicRT.id) {
            BeginTextureMode(mosaicRT);
            DrawMosaicSlots(background);
            EndTextureMode();
            DrawBlurredMosaicBackdrop(background);
        } else {
            DrawMosaicSlots(background);
        }
        DrawSidebar(SystemBG, font);
    } else {
        DrawMosaicSlots(background);
        DrawSidebar(SystemBG, font);
    }

    /* Above blur / panel so accept/back help stays readable. */
    InfoBar_Draw(font);
}

static void SlideEffect(Color mosaicBg, Font font)
{
    /* Mosaic underneath: entering slides the splash art away, leaving
     * slides the card menu back in. */
    DrawMosaic(mosaicBg, font);

    if (slideDirection < 0) {
        DrawSystemArt(slideOffset);
    } else {
        DrawSystemMenu(slideOffset);
    }

    slideOffset += SLIDE_SPEED * (float)slideDirection;

    if (slideDirection < 0) {
        if (slideOffset <= -(float)GetScreenWidth()) {
            slideOffset = -(float)GetScreenWidth();
            screenSliding = false;
            currentState = MOSAIC;
            BeginMosaicEnterIntro();
        }
    } else {
        if (slideOffset >= 0.0f) {
            slideOffset = 0.0f;
            screenSliding = false;
            currentState = SYSTEMS;
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
        }
    }
}

/* Start the system-cover slide only (after info out + shrink). */
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

// ====================== MAIN PROGRAM ======================
int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "NSO");
    SetWindowTitle("NSO");
    SetTargetFPS(FPS);

    const char *base = GetBaseDirectory();
    if (NsoChdir(base) != 0) {
        perror("Failed to chdir to base directory");
    }

    InitSystems();
    {
        char cfg[2112];
        Lang_ConfigPath(cfg, sizeof(cfg));
        Lang_Set(Lang_Load(cfg));
    }
    Order_LoadAll();

    gearTexture = LoadTexture(GetDataPath("assets/images/ui/gear.png"));

    int Count = 0;
    const char *FontPath = "assets/fonts/NotoSansCJKkr-Regular.otf";
    int *Codepoints = LoadCodepoints(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        " .,!?()[]{}'°-:/_+"
        "áàâãéêíóôõúçÁÂÃÉÊÍÓÔÕÚÇ"
        "🅐🅑🅧🅨",
        &Count
    );

    Font font = LoadFontEx(FontPath, 36, Codepoints, Count);
    if (Codepoints) UnloadCodepoints(Codepoints);

    SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    slotFont = font;

    for (int i = 0; i < MAX_GAMES; i++) {
        gameBoxarts[i] = (Texture2D){0};
        animScales[i] = 1.0f;
    }

    Color Mosaicbg = GetColor(0x303030FF);

    InitScreenFadeAnimation(&launchFadeAnim, 0.32f, 0.65f, 0.0f, 1.0f, BLACK);

    while (!WindowShouldClose()) {

        bool gp = IsGamepadAvailable(0);
        static float prevLX = 0.0f;
        static float prevLY = 0.0f;
        float lx = 0.0f;
        float ly = 0.0f;
        if (gp) {
            lx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_X);
            ly = GetGamepadAxisMovement(0, GAMEPAD_AXIS_LEFT_Y);
        }

        bool gpLeft = gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT) ||
                             (lx < -0.5f && prevLX >= -0.5f));
        bool gpRight = gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT) ||
                              (lx > 0.5f && prevLX <= 0.5f));
        bool gpUp = gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_UP) ||
                           (ly < -0.5f && prevLY >= -0.5f));
        bool gpDown = gp && (IsGamepadButtonPressed(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN) ||
                             (ly > 0.5f && prevLY <= 0.5f));
        bool gpConfirm = gp && Nso_PadConfirmPressed(0);
        /* Select opens the full sidebar; keep Back as B only (not Select). */
        bool gpSelect = gp && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT);
        bool gpBack = gp && Nso_PadBackPressed(0);

        prevLX = lx;
        prevLY = ly;

        bool overlayConsumedConfirm = false;
        bool canNavigate = !OverlayActive() && !isLaunching && !waitingForGame
            && !screenSliding;

        if (introFrames < FPS) {
            introOpacity = 1.0f - ((float)introFrames / (float)FPS);
            introFrames++;
        } else {
            introOpacity = 0.0f;
        }

        /* Debug: NSO_AUTOSELECT=<system index> enters that system after 1.5s
         * (lets the splash be captured without a keyboard). */
        static bool autoSelected = false;
        const char *autoSel = getenv("NSO_AUTOSELECT");
        if (autoSel && !autoSelected && currentState == SYSTEMS && GetTime() > 1.5) {
            autoSelected = true;
            int idx = atoi(autoSel);
            if (idx >= 0 && idx < SYSTEM_COUNT) currentSystem = idx;
            StartEnterMosaic();
        }

        if (waitingForGame && NsoProcOk(gamePid)) {
            if (NsoSpawnExited(&gamePid, NULL)) {
                waitingForGame = false;
                RestoreWindow();
                currentState = MOSAIC;
                /* Same enter intro as systems → mosaic (delay, grow, then info fade). */
                BeginMosaicEnterIntro();
            }
        }

        int gameCount = ActiveGameCount();
        float dt = GetFrameTime();

        bool layoutLocked = screenSliding;

        if (currentState == MOSAIC && !waitingForGame && !layoutLocked) {
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
                mosaicIntroTimer -= dt;
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

            targetViewportY = GridTargetViewportY(&grid, GameIdx, gameCount);
            targetViewportY = GridClampViewportY(&grid, targetViewportY, gameCount);
            /* Keep preloaded art; only ensure visible set is present */
            ManageVisibleBoxarts(gameCount, viewportY);
        } else if (currentState != MOSAIC && !layoutLocked) {
            targetViewportY = 0;
            viewportY = 0;
        }

        viewportY += (targetViewportY - viewportY) * 0.18f;
        if (currentState == MOSAIC && !layoutLocked) {
            viewportY = GridClampViewportY(&grid, viewportY, gameCount);
        }
        UpdateSidebarWidth();

        if (isLaunching) {
            UpdateScreenFadeAnimation(&launchFadeAnim, GetFrameTime());
            if (IsScreenFadeAnimationFinished(&launchFadeAnim)) {
                gamePid = LaunchGame(&ActiveGames()[launchingGameIdx]);
                if (NsoProcOk(gamePid)) {
                    waitingForGame = true;
                    isLaunching = false;
                    MinimizeWindow();
                } else {
                    isLaunching = false;
                    currentState = MOSAIC;
                    BeginMosaicEnterIntro();
                }
                InitScreenFadeAnimation(&launchFadeAnim, 0.32f, 0.65f, 0.0f, 1.0f, BLACK);
            }
        }

        if (sidebarOpen && !sidebarClosing && sbWidthPhase == SB_WIDTH_IDLE) {
            if (IsKeyPressed(KEY_B) || gpBack) CollapseSidebarToRail();
        }

        ScrapeMenu_Poll();
        Repo_Poll();
        Details_Update(dt);

        if (!screenSliding) {
            switch (currentState) {
                case SYSTEMS:
                    /* O: shortcut to the gear screen. Down / Up: cards <-> gear. */
                    if (canNavigate && IsKeyPressed(KEY_O)) {
                        Manage_Open();
                        break;
                    }
                    if (canNavigate && menuFocus == 1) {
                        if (IsKeyPressed(KEY_UP) || gpUp) menuFocus = 0;
                        else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm) Manage_Open();
                        break;
                    }
                    if (canNavigate && (IsKeyPressed(KEY_DOWN) || gpDown)) {
                        menuFocus = 1;
                        break;
                    }
                    if (canNavigate && SYSTEM_COUNT > 1) {
                        if (IsKeyPressed(KEY_LEFT) || gpLeft) {
                            SelectSystem(-1);
                        }
                        if (IsKeyPressed(KEY_RIGHT) || gpRight) {
                            SelectSystem(1);
                        }
                    }

                    if (canNavigate && !overlayConsumedConfirm &&
                        (IsKeyPressed(KEY_ENTER) || gpConfirm)) {
                        StartEnterMosaic();
                    }
                    break;

                case MOSAIC:
                    if (waitingForGame) break;

                    if (statesOpen) {
                        if (States_HandleInput(gpLeft, gpRight, gpConfirm, gpBack)) {
                            if (GameIdx < gameCount && GameIsLaunchable(&ActiveGames()[GameIdx])) {
                                isLaunching = true;
                                launchingGameIdx = GameIdx;
                                currentState = LAUNCHING;
                                InitScreenFadeAnimation(&launchFadeAnim, 0.32f, 0.65f, 0.0f, 1.0f, BLACK);
                            } else {
                                stateLoadPath[0] = '\0'; /* never leak into another launch */
                            }
                        }
                        break;
                    }

                    /* KEY_O only: shrink selection, then expand full menu (no Select). */
                    if (!sidebarOpen && !sidebarExpandPending && !sidebarClosing
                        && sbWidthPhase == SB_WIDTH_IDLE
                        && canNavigate && IsKeyPressed(KEY_O)) {
                        RequestSidebarExpand();
                        break;
                    }

                    if (sidebarOpen || sidebarExpandPending || sidebarClosing
                        || sbWidthPhase != SB_WIDTH_IDLE) {
                        break;
                    }

                    if (detailsOpen || detailsT > 0.0f) {
                        Details_HandleInput(gpConfirm, gpBack);
                        break;
                    }

                    /* Y held: the lifted game follows the arrows; release drops + saves. */
                    if (gridLifted) {
                        bool held = IsKeyDown(KEY_Y) || (gp && IsGamepadButtonDown(0, NSO_BTN_Y));
                        if (!held || GameIdx >= gameCount) {
                            gridLifted = false;
                            if (gridLiftMoved) Order_SaveActive();
                            gridLiftMoved = false;
                            break;
                        }
                        int dir = GridNavPressed(gpLeft, gpRight, gpUp, gpDown);
                        if (dir >= 0) {
                            Grid_MoveLifted(Order_GridNav(GameIdx, gameCount, grid.slotsPerRow, (OrderNav)dir));
                        }
                        break;
                    }

                    bool onGame = canNavigate && gameCount > 0 && GameIdx < gameCount;
                    if (onGame && (IsKeyPressed(KEY_Y) || (gp && IsGamepadButtonPressed(0, NSO_BTN_Y)))) {
                        gridLifted = true;
                        gridLiftMoved = false;
                        break;
                    }

                    /* X / gamepad X: details panel (its Save States row opens the overlay). */
                    if (onGame && (IsKeyPressed(KEY_X) || (gp && IsGamepadButtonPressed(0, NSO_BTN_X)))) {
                        Details_Open();
                        break;
                    }

                    /* S: save states of the selected game (keyboard shortcut). */
                    if (onGame && ActiveGames()[GameIdx].stateDir && IsKeyPressed(KEY_S)) {
                        States_Open(&ActiveGames()[GameIdx]);
                        break;
                    }

                    if (gameCount > 0) {
                        int dir = GridNavPressed(gpLeft, gpRight, gpUp, gpDown);
                        if (dir >= 0) GameIdx = Order_GridNav(GameIdx, gameCount, grid.slotsPerRow, (OrderNav)dir);
                    }

                    if (!overlayConsumedConfirm && (IsKeyPressed(KEY_ENTER) || gpConfirm)) {
                        if (gameCount > 0 && GameIdx < gameCount) {
                            if (GameIsLaunchable(&ActiveGames()[GameIdx])) {
                                isLaunching = true;
                                launchingGameIdx = GameIdx;
                                currentState = LAUNCHING;
                                InitScreenFadeAnimation(&launchFadeAnim, 0.32f, 0.65f, 0.0f, 1.0f, BLACK);
                            }
                        }
                    }

                    if (canNavigate && (IsKeyPressed(KEY_B) || gpBack)) {
                        RequestLeaveMosaic();
                    }
                    break;

                case MANAGE: {
                    int rowsN = Manage_RowCount();
                    if (scrapeBusy) {
                        if (IsKeyPressed(KEY_B) || gpBack) ScrapeMenu_Cancel();
                        break;
                    }
                    if (IsKeyPressed(KEY_LEFT) || gpLeft) { SelectSystem(-1); manageRow = 0; }
                    if (IsKeyPressed(KEY_RIGHT) || gpRight) { SelectSystem(1); manageRow = 0; }
                    if (IsKeyPressed(KEY_UP) || gpUp) manageRow = (manageRow + rowsN - 1) % rowsN;
                    if (IsKeyPressed(KEY_DOWN) || gpDown) manageRow = (manageRow + 1) % rowsN;
                    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm) Manage_Activate();
                    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_O) || gpBack) Manage_Close();
                    break;
                }

                case LAUNCHING:
                case SPLASH:
                    break;
            }
        }

        if (!screenSliding) UpdateSplash(dt);

        BeginDrawing();

        if (screenSliding) {
            SlideEffect(Mosaicbg, font);
        } else if (currentState == SYSTEMS) {
            DrawSystemMenu(0.0f);
            if (introOpacity > 0.0f) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                              Fade(BLACK, introOpacity));
            }
        } else if (currentState == SPLASH) {
            ClearBackground(BLACK);
            DrawSystemArt(0.0f);
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                          Fade(BLACK, introOpacity));
        } else if (currentState == MANAGE) {
            Manage_Draw(font);
            if (introOpacity > 0.0f) {
                DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, introOpacity));
            }
        } else if (currentState == MOSAIC || currentState == LAUNCHING) {
            DrawMosaic(Mosaicbg, font);
            Details_Draw(font);
            if (statesOpen && GameIdx < ActiveGameCount()) {
                States_Draw(font, ActiveGames()[GameIdx].title);
            }
        }

        if (isLaunching) {
            DrawScreenFadeAnimation(&launchFadeAnim);
        }

        if (waitingForGame) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        }

        EndDrawing();
    }

    CoverLoader_Stop();
    if (NsoProcOk(repoProc)) NsoSpawnKill(repoProc);
    States_Unload();
    UnloadAllBoxarts();
    UnloadFont(font);
    if (gearTexture.id) UnloadTexture(gearTexture);
    UnloadBlurResources();
    UnloadSystems();
    CloseWindow();

    return EXIT_SUCCESS;
}
