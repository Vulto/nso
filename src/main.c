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
#include "rlgl.h"   /* blend factors: erase inside the sidebar icon texture */

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
#define SFX_IMPLEMENTATION
#include "sfx.h"
#define EMBED_IMPLEMENTATION
#include "embed.h"

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
static const float SIDEBAR_OPEN_FRAC = 0.385f;   /* video: ~38.6% */
static const float SIDEBAR_OVERSHOOT = 0.08f; /* past target, then settle */
static const float SIDEBAR_WIDTH_LERP = 0.22f;
/* 3 full rows + ~10% of a fourth so the user sees more games below */
static const float MOSAIC_VISIBLE_ROWS = 3.1f;
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

    if (!FileExists(path)) {   /* quiet: missing covers are retried every frame */
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

bool IsScreenFadeAnimationFinished(const ScreenFadeAnimation *anim) {
    return anim->finished;
}

typedef enum State {
    LAUNCHING,
    SPLASH,     /* full-screen system art fading in before the game grid */
    SYSTEMS,
    MOSAIC,
    MANAGE,     /* gear: box-art scraping + repository download */
    EMBED,      /* in-process game drawn in this window (embed.h) */
} State;

static State currentState = SYSTEMS;

static int GameIdx = 0;

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

/* X Details card: fades in over the frosted grid (detailsT 0..1). */
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

/* Settings-icon spin / highlight morph from sidebar width progress (rail→open maps to 0→1). */
static float gearSpinAngle = 0.0f;
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
static float embedBackT = -1.0f;   /* >= 0: embedded game shrinking into its card */

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

/* C-up / C-down / C-left this frame (right stick edges on any pad), set in the main loop. */
static bool padCUp, padCDown, padCLeft;

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
        case INFO_BAR_MOSAIC:  return T(gridLifted ? S_MOVE_INFO : S_MOSAIC_INFO);
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
/* No gear.png (art is not shipped in the repo): a solid gear made of three
 * crossed bars (six teeth), a body disc and a hole in the background colour. */
static void DrawFallbackGear(Vector2 c, float size, float angle, Color col, Color hole) {
    float w = size * 0.2f, h = size * 0.96f;
    for (int k = 0; k < 3; k++) {
        DrawRectanglePro((Rectangle){ c.x, c.y, w, h }, (Vector2){ w * 0.5f, h * 0.5f },
                         angle + 60.0f * (float)k, col);
    }
    DrawCircleV(c, size * 0.37f, col);
    DrawCircleV(c, size * 0.16f, hole);
}

#define GEAR_FALLBACK_COLOR ((Color){ 172, 49, 49, 255 })   /* gear.png stroke */

static void Repo_GearDot(Vector2 gearCentre, float radius);   /* install badge */

static void DrawMenuGear(float offsetX) {
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    float d = sh * 0.075f;                       /* icon disc diameter */
    float cx = offsetX + sw * 0.5f;
    float cy = sh - sh * 0.12f;
    float g = d * 0.72f;
    DrawCircleV((Vector2){ cx, cy }, d * 0.5f, WHITE);
    if (gearTexture.id != 0) {
        DrawTexturePro(gearTexture, (Rectangle){ 0, 0, (float)gearTexture.width, (float)gearTexture.height },
                       (Rectangle){ cx, cy, g, g }, (Vector2){ g * 0.5f, g * 0.5f }, 0.0f, WHITE);
    } else {
        DrawFallbackGear((Vector2){ cx, cy }, g, 0.0f, GEAR_FALLBACK_COLOR, WHITE);
    }
    if (menuFocus == 1) {
        DrawRing((Vector2){ cx, cy }, d * 0.5f + d * 0.05f, d * 0.5f + d * 0.11f, 0.0f, 360.0f, 48, MenuGlow());
    }
    Repo_GearDot((Vector2){ cx, cy }, d * 0.5f);
}

/* Draw the systems menu shifted by offsetX (used by the mosaic slide).
 * Every system card, plus the selection ring, sits inside a rectangle a
 * quarter of the screen (half width x half height) centred on screen. */
static Font slotFont;   /* menu font, defined (and set in main) below */

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
    /* USB (or no N64 pad) keeps the default bar; Bluetooth gets N64 labels. */
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
    char buf[16384];
    size_t n = 0;
    FILE *f = fopen("/proc/bus/input/devices", "r");
    if (!f) return NSO_PAD_LINK_NONE;
    n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    NsoPadLink link = Nso_PadLinkFromProc(buf, PAD_NOTICE_MAC);
    if (link == NSO_PAD_LINK_NONE) link = Nso_PadLinkFromProc(buf, NULL);
    return link;
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
    Vector2 textSize = MeasureTextEx(font, draw, fontSize, spacing);
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
               draw,
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
    char *argv[9];
    snprintf(scrapePart, sizeof(scrapePart), "%s", outPath);
    argv[0] = "curl";
    argv[1] = "-fsSL";
    argv[2] = "--max-time";
    argv[3] = "30";
    argv[4] = "--create-dirs";   /* fresh clone: assets/images/systems/<id>/ is absent */
    argv[5] = "-o";
    argv[6] = scrapePart;
    argv[7] = (char *)url;
    argv[8] = NULL;
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

static bool Repo_AnyOffer(void) {
    for (int i = 0; i < nso_repo_count && i < REPO_MAX; i++) if (Repo_Offer(i)) return true;
    return false;
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
 * a notice; A on it, the gear row or Details > Install runs repo_sync.sh in
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

/* Small "Install" pill / dot used on game tiles and the gear. */
static void Repo_BadgeDraw(Font font, Rectangle tile) {
    float fs = fmaxf(14.0f, fminf(24.0f, tile.height * 0.09f));
    const char *t = T(S_DET_INSTALL);
    Vector2 m = MeasureTextEx(font, t, fs, 1.0f);
    Rectangle b = { tile.x + tile.width - m.x - 22.0f, tile.y + 8.0f, m.x + 14.0f, m.y + 6.0f };
    DrawRectangleRounded(b, 0.5f, 8, (Color){ 0, 150, 220, 235 });
    DrawTextEx(font, t, (Vector2){ b.x + 7.0f, b.y + 3.0f }, fs, 1.0f, WHITE);
}

static void Repo_GearDot(Vector2 gearCentre, float radius) {
    if (!Repo_AnyOffer()) return;
    Vector2 c = { gearCentre.x + radius * 0.72f, gearCentre.y - radius * 0.72f };
    DrawCircleV(c, fmaxf(6.0f, radius * 0.24f) + 2.0f, WHITE);
    DrawCircleV(c, fmaxf(6.0f, radius * 0.24f), (Color){ 0, 150, 220, 255 });
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
        int i = idx[manageRow - 2];
        Repo_Enqueue(i, Repo_Offer(i));   /* "Ready to install": build only */
    } else if (n > 0 && manageRow == 2 + n) {
        for (int k = 0; k < n; k++) Repo_Enqueue(idx[k], false);
    } else if (manageRow == Manage_RowCount() - 2) {
        for (int k = 0; k < nso_repo_count; k++) Repo_Enqueue(k, false);   /* all systems */
    } else {
        char dir[2048], cfg[2112];
        Lang_Set(Lang_Get() == LANG_EN ? LANG_PT : LANG_EN);
        snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
        MakeDirectory(dir);
        Lang_ConfigPath(cfg, sizeof(cfg));
        if (!Lang_Save(cfg, Lang_Get())) fprintf(stderr, "NSO: cannot save %s\n", cfg);
    }
}

/* Gear screen scroll: long repo lists (static recompilations) do not fit,
 * so the page follows the selected row. Content-space y of that row. */
static float manageScroll = 0.0f;
static float manageSelY = 0.0f;

static void Manage_DrawRow(Font font, Rectangle sec, float *y, int row, const char *left,
                           const char *right) {
    float size = (float)font.baseSize;
    float rowH = 54.0f;
    float x = sec.x + 28.0f;
    Color fg = WHITE;
    if (row == manageRow) {
        manageSelY = *y;
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
    rlPushMatrix();
    rlTranslatef(0.0f, -manageScroll, 0.0f);

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
    y += h3;
    rlPopMatrix();

    /* Keep the selected row between the top margin and the info bar. */
    float top = sh * 0.08f, bottom = sh - 100.0f, rowH = 62.0f;
    if (manageRow == 0) manageScroll = 0.0f;
    if (manageSelY - manageScroll < top) manageScroll = manageSelY - top;
    if (manageSelY + rowH - manageScroll > bottom) manageScroll = manageSelY + rowH - bottom;
    float maxScroll = y + 24.0f - bottom;
    if (manageScroll > maxScroll) manageScroll = maxScroll;
    if (manageScroll < 0.0f) manageScroll = 0.0f;

    DrawInfoBarEx(font, scrapeBusy ? T(S_INFO_CANCEL) : T(S_MANAGE_INFO), 1.0f, 0.0f);
}

/* ---- In-system sidebar (grid rail + B/O menu), as in the reference video:
 * four line icons (Single player, Play together, Play online, Settings),
 * icon + label rows when open, and a Settings panel over the blurred grid.
 * Icons are raylib primitives (no art). The first three have no function;
 * the selected one loops a small animation. ---- */
typedef enum { SB_ITEM_SINGLE = 0, SB_ITEM_TOGETHER, SB_ITEM_ONLINE, SB_ITEM_SETTINGS,
               SB_ITEM_COUNT } SidebarItem;
enum { SB_SET_VER_NA = 0, SB_SET_VER_NA_EU, SB_SET_RUMBLE, SB_SET_CONTROLS, SB_SET_COUNT };

/* Stored per system in config/nso.cfg ("<id>.versions", "<id>.rumble",
 * "<id>.show_controls"; NES/SNES also "<id>.screen" = 4:3|pixel|crt).
 * Settings only: not passed to the emulators yet. */
typedef struct { int versions; bool rumble; bool showControls; int screen; } SysSettings;
static const char *const SCREEN_MODE_KEYS[3] = { "4:3", "pixel", "crt" };
static SysSettings sysSettings[sizeof(systems) / sizeof(systems[0])];

static void SysSettings_LoadAll(void) {
    char cfg[2112], text[4096], v[32];
    size_t n = 0;
    Lang_ConfigPath(cfg, sizeof(cfg));
    FILE *f = fopen(cfg, "r");
    if (f) { n = fread(text, 1, sizeof(text) - 1, f); fclose(f); }
    text[n] = '\0';
    for (int i = 0; i < SYSTEM_COUNT; i++) {
        const char *id = systems[i].platform->id;
        char key[64];
        SysSettings s = { 0, true, false, 1 };   /* defaults: NA, rumble on, controls hidden, pixel */
        snprintf(key, sizeof(key), "%s.versions", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.versions = strcmp(v, "na_eu") == 0 ? 1 : 0;
        snprintf(key, sizeof(key), "%s.rumble", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.rumble = strcmp(v, "0") != 0;
        snprintf(key, sizeof(key), "%s.show_controls", id);
        if (Cfg_Get(text, key, v, sizeof(v))) s.showControls = strcmp(v, "0") != 0;
        snprintf(key, sizeof(key), "%s.screen", id);
        if (Cfg_Get(text, key, v, sizeof(v)))
            for (int m = 0; m < 3; m++) if (strcmp(v, SCREEN_MODE_KEYS[m]) == 0) s.screen = m;
        sysSettings[i] = s;
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
    if (strcmp(id, "nes") == 0 || strcmp(id, "snes") == 0) {
        snprintf(key, sizeof(key), "%s.screen", id);
        ok &= Cfg_SaveKey(cfg, key, SCREEN_MODE_KEYS[s->screen >= 0 && s->screen < 3 ? s->screen : 1]);
    }
    if (!ok) fprintf(stderr, "NSO: cannot save %s\n", cfg);
}

static int sidebarItem = SB_ITEM_SINGLE;
static bool sidebarPanelFocus = false;   /* Settings: focus in the right panel */
static int sidebarSetRow = 0;
/* One-shot icon animation per item: plays once when the item becomes
 * selected (dir +1) and once, shorter and softer, when it loses selection
 * (dir -1); then the icon rests. */
static double sbAnimStart[SB_ITEM_COUNT] = { -100.0, -100.0, -100.0, -100.0 };
static int sbAnimDir[SB_ITEM_COUNT];
static const float SB_ANIM_ENTER_SEC = 0.55f;
static const float SB_ANIM_LEAVE_SEC = 0.30f;

static void SidebarAnim_Play(int item, int dir) {
    sbAnimStart[item] = GetTime();
    sbAnimDir[item] = dir;
}

static void Sidebar_Select(int item) {
    if (item == sidebarItem) return;
    SidebarAnim_Play(sidebarItem, -1);
    sidebarItem = item;
    SidebarAnim_Play(item, +1);
    Sfx_Play(SFX_PLUCK);
}

/* Progress 0..1 of the item's one-shot, or -1 when it is at rest. */
static float SidebarAnim_Progress(int item) {
    float dur = sbAnimDir[item] > 0 ? SB_ANIM_ENTER_SEC : SB_ANIM_LEAVE_SEC;
    float p = (float)(GetTime() - sbAnimStart[item]) / dur;
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
        case SB_ITEM_ONLINE: {   /* meridian turns half a revolution once, person in front */
            Vector2 g = { c.x + s * 0.12f, c.y - s * 0.1f };
            float r = s * 0.36f;
            float e = on ? sinf(p * PI * 0.5f) : 1.0f;             /* ease out */
            float turn = 0.6f + (float)dir * e * PI * amp;
            if (!on) turn = 0.6f;
            IconArc(g, r, r, 0.0f, 360.0f, th, col);
            DrawLineEx((Vector2){ g.x - r, g.y }, (Vector2){ g.x + r, g.y }, th * 0.8f, col);
            IconArc(g, r, r * 0.45f, 180.0f, 360.0f, th * 0.7f, col);
            IconArc(g, r * fabsf(cosf(turn)), r, -90.0f, 90.0f, th * 0.8f, col);
            IconArc(g, r * fabsf(sinf(turn)) * 0.999f, r, 90.0f, 270.0f, th * 0.8f, col);
            IconPerson((Vector2){ c.x - s * 0.22f, c.y + s * 0.22f }, s * 0.55f, th * 0.85f, col, fill, true);
        } break;
        default: {   /* gear turns one tooth (60 deg) once */
            float e = on ? 1.0f - (1.0f - p) * (1.0f - p) : 0.0f;  /* ease out */
            IconGear(c, s, th, (float)dir * 60.0f * amp * e, col);
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

/* ---- NES / SNES Settings panel (video frame 000484): "Game Screen" with
 * 4:3 / Pixel Perfect / CRT Filter cards, each with a small game-screen
 * thumbnail, then "Control Display". N64 keeps SidebarPanel_Draw as is. ---- */
static bool SysIsRetro(int sys) {
    const char *id = systems[sys].platform->id;
    return strcmp(id, "nes") == 0 || strcmp(id, "snes") == 0;
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

/* One thumbnail as the option would show it: 4:3 fills the frame, Pixel
 * Perfect is narrower with dark side bars, CRT adds scanlines. */
static void DrawRetroThumb(Rectangle r, bool snes, int mode) {
    Rectangle img = r;
    if (mode == 1) {   /* square pixels: 8:7 inside a 4:3 frame -> ~0.82 wide */
        float w = r.width * 0.82f;
        DrawRectangleRec(r, (Color){ 66, 68, 68, 255 });
        img = (Rectangle){ r.x + (r.width - w) * 0.5f, r.y, w, r.height };
    }
    Texture2D shot = RetroShot(snes);
    if (shot.id) {
        DrawTexturePro(shot, (Rectangle){ 0, 0, (float)shot.width, (float)shot.height }, img,
                       (Vector2){ 0 }, 0.0f, WHITE);
    } else {
        BeginScissorMode((int)img.x, (int)img.y, (int)ceilf(img.width), (int)ceilf(img.height));
        DrawRetroScene(img, snes);
        EndScissorMode();
    }
    if (mode == 2) {
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

    static const StrId labels[3] = { S_SET_43, S_SET_PIXEL, S_SET_CRT };
    static const float tops[3] = { 0.132f, 0.326f, 0.521f };
    float ch = sh * 0.155f, r = sh * 0.016f;
    for (int k = 0; k < 3; k++) {
        Rectangle card = { bx, sh * tops[k], bw, ch };
        float y = card.y + ch * 0.5f;
        bool cur = (k == sidebarSetRow);
        if (cur) DrawRectangleRec(card, curFill);
        Rectangle thumb = { card.x + th, card.y + th, sw * 0.112f, ch - 2.0f * th };
        DrawRetroThumb(thumb, snes, k);
        DrawRectangleLinesEx(card, th * 0.6f, WHITE);
        DrawFitText(font, T(labels[k]), (Vector2){ sw * 0.617f, y }, osize, bx + bw - sw * 0.625f,
                    cur ? pink : WHITE);
        if (st->screen == k) {
            DrawCircleV((Vector2){ cx, y }, r, WHITE);
            DrawCircleV((Vector2){ cx, y }, r * 0.78f, (Color){ 235, 75, 55, 255 });
        } else {
            IconArc((Vector2){ cx, y }, r, r, 0.0f, 360.0f, th * 0.7f, WHITE);
        }
    }

    /* Control Display: one checkbox row (same box as the N64 panel). */
    float y = sh * 0.844f, bh = sh * 0.069f;
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
    struct { StrId label; float y; bool radio; bool on; } rows[SB_SET_COUNT] = {
        { S_SET_VER_NA, 0.280f, true, st->versions == 0 },
        { S_SET_VER_NA_EU, 0.373f, true, st->versions == 1 },
        { S_SET_ENABLED, 0.569f, false, st->rumble },
        { S_SET_SHOW, 0.766f, false, st->showControls } };

    for (int k = 0; k < 3; k++) {
        DrawFitText(font, T(heads[k].head), (Vector2){ hx, sh * heads[k].hy }, hsize, sw - hx, WHITE);
    }
    for (int k = 0; k < SB_SET_COUNT; k++) {
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
            float roundness = GEAR_STRIP_ROUNDNESS * (1.0f - t);
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
        SidebarPanel_Draw(font, systemBg);
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

    float mv = liftT;
    if (mv > 0.0f) DrawRectangle(0, 0, GetScreenWidth(), screen_h, Fade(MOVE_BG, mv));
    float othersScale = 1.0f - (1.0f - MOVE_OTHERS_SCALE) * mv;
    Color othersTint = ColorLerp(WHITE, (Color){ 222, 222, 222, 255 }, mv);

    for (int i = 0; i < numImgs; i++) {
        if (i == GameIdx) continue;

        int col = i % cols;
        int row = i / cols;
        float x = (float)origin_x + (float)col * cellW;
        float y = (float)row * cellH;

        float fitW, fitH;
        FitAspectInCell(cellW, cellH, Nso_OrientAspect(GameOrientAt(i)), &fitW, &fitH);

        float tile_scale = animScales[i] * othersScale;
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
                           ColorTint(GameTint(i), othersTint));
            DrawMissingCover(images[i], (Rectangle){ox, oy, draw_w, draw_h}, ColorTint(GameTint(i), othersTint));
            if (Repo_Offer(Game_Repo(&ActiveGames()[i])))
                Repo_BadgeDraw(slotFont, (Rectangle){ox, oy, draw_w, draw_h});
        }
    }

    if (GameIdx >= 0 && GameIdx < numImgs) {
        int col = GameIdx % cols;
        int row = GameIdx / cols;
        float x = (float)origin_x + (float)col * cellW;
        float y = (float)row * cellH;

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
    SidebarAnim_Play(sidebarItem, +1);   /* the selected row plays once as it opens */
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
    if (sidebarPanelFocus) {
        if (back || IsKeyPressed(KEY_LEFT) || gpLeft) { sidebarPanelFocus = false; return; }
        if (up && sidebarSetRow > 0) { sidebarSetRow--; Sfx_Play(SFX_PLUCK); }
        if (down && sidebarSetRow < SB_SET_COUNT - 1) { sidebarSetRow++; Sfx_Play(SFX_PLUCK); }
        if (ok) {
            Sfx_Play(SFX_CLICK);
            SysSettings *st = &sysSettings[currentSystem];
            if (SysIsRetro(currentSystem) && sidebarSetRow < SB_SET_CONTROLS) st->screen = sidebarSetRow;
            else if (SysIsRetro(currentSystem)) st->showControls = !st->showControls;
            else if (sidebarSetRow == SB_SET_VER_NA) st->versions = 0;
            else if (sidebarSetRow == SB_SET_VER_NA_EU) st->versions = 1;
            else if (sidebarSetRow == SB_SET_RUMBLE) st->rumble = !st->rumble;
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
        Sfx_Play(SFX_CLICK);
    } else if (ok) {
        SidebarAnim_Play(sidebarItem, +1);   /* no function: replay the one-shot */
        Sfx_Play(SFX_CLICK);
    }
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
    bool gpX = padCUp || (gp && IsGamepadButtonPressed(0, NSO_BTN_X));
    bool gpY = padCDown || (gp && IsGamepadButtonPressed(0, NSO_BTN_Y));
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
    float step = dt * 1000.0f / DETAILS_FADE_DELTA_MS
               * (detailsOpen ? DETAILS_FADE_IN_STEP : DETAILS_FADE_OUT_STEP);
    if (detailsT < target) detailsT = fminf(detailsT + step, target);
    else if (detailsT > target) detailsT = fmaxf(detailsT - step, target);
}

static int Details_Repo(const GameEntry *g) { return Game_Repo(g); }

static void Details_HandleInput(bool gpConfirm, bool gpBack) {
    bool gp = IsGamepadAvailable(0);
    if (!detailsOpen) return;   /* sliding out */
    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_X) || gpBack
        || padCUp || (gp && IsGamepadButtonPressed(0, NSO_BTN_X))) {
        Details_Close();
        return;
    }
    if (GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    bool confirm = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_A) || gpConfirm;
    int repo = Details_Repo(g);
    if (Repo_Offer(repo)) {   /* Install is the highlighted row while offered */
        if (confirm) Repo_Enqueue(repo, true);
        return;
    }
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

/* Details card as in the video: frosted, dimmed grid behind; one light card
 * (one rounded panel; lighter cover column left, text column right). */
static void Details_Draw(Font font) {
    if (detailsT <= 0.0f || GameIdx >= ActiveGameCount()) return;
    const GameEntry *g = &ActiveGames()[GameIdx];
    float a = detailsT;   /* linear opacity; the card never moves or resizes */
    float sw = (float)GetScreenWidth();
    float sh = (float)GetScreenHeight();
    const Color ink = { 44, 49, 50, 255 }, meta = { 92, 97, 98, 255 }, body = { 85, 90, 91, 255 };
    const Color colL = { 240, 245, 244, 255 }, colR = { 229, 235, 237, 255 }, rule = { 205, 211, 213, 255 };

    DrawRectangle(0, 0, (int)sw, (int)sh, Fade(BLACK, 0.40f * a));

    /* Integer bounds; the same rounded panel is drawn twice, each clipped to
     * its own column, so every pixel is blended exactly once (no seam, no
     * shade change mid-fade, corners intact). */
    Rectangle card = { roundf(sw * 0.042f), roundf(sh * 0.047f), roundf(sw * 0.916f), roundf(sh * 0.905f) };
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

    Texture2D tex = gameBoxarts[GameIdx];
    float cvW, cvH;
    FitAspectInCell(sw * 0.208f, sh * 0.30f, Nso_OrientAspect(GameOrientAt(GameIdx)), &cvW, &cvH);
    Rectangle cr = { card.x + (lw - cvW) * 0.5f, sh * 0.139f, cvW, cvH };
    if (tex.id) {
        DrawTexturePro(tex, (Rectangle){ 0, 0, (float)tex.width, (float)tex.height }, cr,
                       (Vector2){ 0 }, 0.0f, Fade(WHITE, a));
    } else {
        DrawRectangleRec(cr, Fade(DARKGRAY, a));
        DrawMissingCover(tex, cr, Fade(WHITE, a));
    }

    float tx = sw * 0.334f, tw = card.x + card.width - sw * 0.05f - tx;
    Details_Text(font, g->title ? g->title : "", (Vector2){ tx, sh * 0.140f }, sh * 0.040f, tw, Fade(ink, a));

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

    /* Action rows where the video has its description; highlighted row
     * gets the gradient glow, the others sit on a thin rule. */
    bool install = Repo_Offer(repo);
    struct { const char *label; bool sel; bool box; } rows[2];
    int n = 0;
    if (install) rows[n++] = (typeof(rows[0])){ T(S_DET_INSTALL), true, true };
    if (g->stateDir) rows[n++] = (typeof(rows[0])){ T(S_DET_SAVES), !install, true };
    else rows[n++] = (typeof(rows[0])){ T(S_DET_NO_SAVES), false, false };
    float rs = sh * 0.033f, rh = sh * 0.085f, ry = sh * 0.32f;
    float rx = tx - sw * 0.012f, rw = sw * 0.56f;
    for (int i = 0; i < n; i++) {
        Rectangle rr = { rx, ry, rw, rh };
        if (rows[i].sel) {
            DrawRectangleRounded(rr, 2.0f * rad / rh, 12, Fade((Color){ 250, 252, 252, 255 }, a));
            Details_GlowRect(rr, rad, fmaxf(2.0f, sh * 0.0045f), a);
        } else if (rows[i].box) {
            DrawLineEx((Vector2){ rx, ry + rh }, (Vector2){ rx + rw, ry + rh }, fmaxf(1.0f, sh * 0.0014f),
                       Fade(rule, a));
        }
        Details_Text(font, rows[i].label, (Vector2){ tx, ry + (rh - rs) * 0.5f }, rs, rw - sw * 0.03f,
                     Fade(rows[i].sel ? ink : body, a));
        ry += rh + sh * 0.012f;
    }

    DrawInfoBarEx(font, T(S_DET_INFO), a, 0.0f);
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

/* Embedded card: module = dir/binary, game data (SMB: the ROM, SMW: the port's
 * directory) = config/nso.cfg <embedRomKey> (NSO-relative or absolute; never
 * copied). A state picked in the grid's Save States resumes in the module. */
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
    embedBackT = -1.0f;
    bool ok = Embed_Start(module, romPath, game->stateDir ? GetDataPath(game->stateDir) : NULL, stateLoadPath);
    stateLoadPath[0] = '\0';
    return ok;
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

/* Selected cover at scale, as DrawSlotImages places it. */
static Rectangle SelectedCardRect(float scale) {
    GridLayout g = GetGridLayout();
    if (g.slotsPerRow < 1) return (Rectangle){ 0 };
    float fitW, fitH;
    FitAspectInCell(g.cellW, g.cellH, Nso_OrientAspect(GameOrientAt(GameIdx)), &fitW, &fitH);
    float x = (float)g.originX + (float)(GameIdx % g.slotsPerRow) * g.cellW;
    float y = (float)(GameIdx / g.slotsPerRow) * g.cellH - viewportY;
    float w = fitW * scale, h = fitH * scale;
    x += (g.cellW - w) * 0.5f;
    y += (g.cellH - h) * 0.5f;
    ClampTileToContent(&x, &y, w, h, g.originX, g.contentW, g.screenH, MOSAIC_CURSOR_PAD);
    return (Rectangle){ x, y, w, h };
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

    if (SidebarWantsBlur() || detailsT > 0.0f) {
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        EnsureBlurResources(sw, sh);

        if (mosaicRT.id) {
            BeginTextureMode(mosaicRT);
            DrawMosaicSlots(background);
            EndTextureMode();
            DrawBlurredMosaicBackdrop(background);
            /* Details: the sharp grid dissolves into the blur in step with the card. */
            if (!SidebarWantsBlur() && detailsT < 1.0f)
                DrawRT(mosaicRT.texture, (Rectangle){ 0, 0, (float)sw, (float)sh },
                       Fade(WHITE, 1.0f - detailsT));
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
    SetGamepadMappings(NSO_PAD_MAPPINGS);
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
    SysSettings_LoadAll();
    Order_LoadAll();
    Sfx_Init();

    gearTexture = LoadTexture(GetDataPath("assets/images/ui/gear.png"));
    PadNotice_Load();

    int Count = 0;
    char FontPath[2048];   /* base-dir relative, so ./nso works from any cwd */
    snprintf(FontPath, sizeof(FontPath), "%s", GetDataPath("assets/fonts/NotoSansCJKkr-Regular.otf"));
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

    InitScreenFadeAnimation(&launchFadeAnim, EMBED_BACK_SEC, 0.65f, 0.0f, 1.0f, BLACK);

    while (!WindowShouldClose()) {

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
            rx = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_X);
            ry = GetGamepadAxisMovement(0, GAMEPAD_AXIS_RIGHT_Y);
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
        bool gpSelect = gpReady && IsGamepadButtonPressed(0, GAMEPAD_BUTTON_MIDDLE_LEFT);
        bool gpBack = gpReady && Nso_PadBackPressed(0);

        padCUp = gpReady && eRY < 0;
        padCDown = gpReady && eRY > 0;
        padCLeft = gpReady && eRX < 0;

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
            if (ActiveSystem()->platform == &n64_platform) Sfx_Play(SFX_COIN);
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
        Pad_Update(dt);
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
                const GameEntry *launched = &ActiveGames()[launchingGameIdx];
                if (launched->embedRomKey) {
                    isLaunching = false;
                    if (StartEmbeddedGame(launched)) {
                        currentState = EMBED;
                    } else {
                        currentState = MOSAIC;
                        BeginMosaicEnterIntro();
                    }
                } else if (NsoProcOk(gamePid = LaunchGame(launched))) {
                    waitingForGame = true;
                    isLaunching = false;
                    MinimizeWindow();
                } else {
                    isLaunching = false;
                    currentState = MOSAIC;
                    BeginMosaicEnterIntro();
                }
                InitScreenFadeAnimation(&launchFadeAnim, EMBED_BACK_SEC, 0.65f, 0.0f, 1.0f, BLACK);
            }
        }

        if (embedBackT >= 0.0f) {
            embedBackT += dt / EMBED_BACK_SEC;
            if (embedBackT >= 1.0f) { Embed_Stop(); embedBackT = -1.0f; }
        }
        if (currentState == EMBED && !Embed_Update(dt)) {
            embedBackT = 0.0f;   /* Embed_Stop once the frame reached its card */
            currentState = MOSAIC;
            BeginMosaicEnterIntro();
            overlayConsumedConfirm = true;   /* the Quit press must not relaunch */
        }

        if (currentState == MOSAIC && SidebarMenuSettled()) {
            Sidebar_HandleInput(gpUp, gpDown, gpLeft, gpRight, gpConfirm, gpBack);
        }

        ScrapeMenu_Poll();
        Repo_Poll();
        Repo_Watch();
        Details_Update(dt);
        {   /* move-mode look: linear, picom default rates (see Details) */
            float st = dt * 1000.0f / DETAILS_FADE_DELTA_MS;
            if (gridLifted) liftT = fminf(1.0f, liftT + st * DETAILS_FADE_IN_STEP);
            else liftT = fmaxf(0.0f, liftT - st * DETAILS_FADE_OUT_STEP);
            float rs = dt / MOVE_RAIL_SLIDE_SEC;
            railSlideT = gridLifted ? fminf(1.0f, railSlideT + rs) : fmaxf(0.0f, railSlideT - rs);
        }

        /* A on an install notice builds that game (one press). Not while a
         * game, the save states, Details, a scrape or a slide owns input. */
        bool noticeTookInput = false;
        if (Repo_NoticeActionable() && !screenSliding && !isLaunching && !waitingForGame
            && !statesOpen && !detailsOpen && detailsT <= 0.0f && !scrapeBusy && !OverlayActive()
            && currentState != LAUNCHING && currentState != SPLASH && currentState != EMBED
            && (IsKeyPressed(KEY_A) || gpConfirm)) {
            Repo_Enqueue(repoNotice.repo, true);
            repoNotice.repo = -1;
            noticeTookInput = true;
        }

        if (!screenSliding && !noticeTookInput) {
            switch (currentState) {
                case SYSTEMS:
                    /* O: shortcut to the gear screen. Down / Up: cards <-> gear. */
                    if (canNavigate && (IsKeyPressed(KEY_O) || padCLeft)) {
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
                        if (ActiveSystem()->platform == &n64_platform) Sfx_Play(SFX_COIN);
                        else Sfx_Play(SFX_CLICK);
                        StartEnterMosaic();
                    }
                    break;

                case MOSAIC:
                    if (waitingForGame) break;

                    if (statesOpen) {
                        if (States_HandleInput(gpLeft, gpRight, gpConfirm, gpBack)) {
                            if (GameIdx < gameCount && GameIsLaunchable(&ActiveGames()[GameIdx])) {
                                Sfx_Play(SFX_CLICK);
                                isLaunching = true;
                                launchingGameIdx = GameIdx;
                                currentState = LAUNCHING;
                                InitScreenFadeAnimation(&launchFadeAnim, EMBED_BACK_SEC, 0.65f, 0.0f, 1.0f, BLACK);
                            } else {
                                stateLoadPath[0] = '\0'; /* never leak into another launch */
                            }
                        }
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

                    if (detailsOpen || detailsT > 0.0f) {
                        Details_HandleInput(gpConfirm, gpBack);
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
                            if (gridLiftMoved) Order_SaveActive();
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
                            int next = Order_GridNav(GameIdx, gameCount, grid.slotsPerRow, (OrderNav)dir);
                            if (next != GameIdx) Sfx_Play(SFX_NAVIGATE);
                            Grid_MoveLifted(next);
                        }
                        break;
                    }

                    bool onGame = canNavigate && gameCount > 0 && GameIdx < gameCount;
                    if (onGame && (IsKeyPressed(KEY_Y) || padCDown || (gp && IsGamepadButtonPressed(0, NSO_BTN_Y)))) {
                        gridLifted = true;
                        gridLiftMoved = false;
                        gridLiftFrom = GameIdx;
                        break;
                    }

                    /* X / gamepad X: details panel (its Save States row opens the overlay). */
                    if (onGame && (IsKeyPressed(KEY_X) || padCUp || (gp && IsGamepadButtonPressed(0, NSO_BTN_X)))) {
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
                        if (dir >= 0) {
                            int next = Order_GridNav(GameIdx, gameCount, grid.slotsPerRow, (OrderNav)dir);
                            if (next != GameIdx) Sfx_Play(SFX_NAVIGATE);
                            GameIdx = next;
                        }
                    }

                    if (!overlayConsumedConfirm && (IsKeyPressed(KEY_ENTER) || gpConfirm)) {
                        if (gameCount > 0 && GameIdx < gameCount) {
                            if (GameIsLaunchable(&ActiveGames()[GameIdx])) {
                                Sfx_Play(SFX_CLICK);
                                isLaunching = true;
                                launchingGameIdx = GameIdx;
                                currentState = LAUNCHING;
                                InitScreenFadeAnimation(&launchFadeAnim, EMBED_BACK_SEC, 0.65f, 0.0f, 1.0f, BLACK);
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
                    if (IsKeyPressed(KEY_B) || IsKeyPressed(KEY_O) || padCLeft || gpBack) Manage_Close();
                    break;
                }

                case LAUNCHING:
                case SPLASH:
                case EMBED:   /* embed.h reads its own input */
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
        } else if (currentState == EMBED) {
            const char *const text[EMBED_TXT_COUNT] = { T(S_EMB_CONTINUE), T(S_EMB_LOAD), T(S_EMB_SAVE),
                T(S_EMB_RESTART), T(S_EMB_QUIT), T(S_EMB_SLOT), T(S_EMB_NODATA), T(S_EMB_DATE) };
            if (Embed_MenuOpacity() > 0.0f) EnsureBlurResources(GetScreenWidth(), GetScreenHeight());
            Embed_Draw(font, sysSettings[currentSystem].screen, text, blurShaderOk ? blurShader : (Shader){ 0 }, blurDirLoc);
            DrawInfoBarEx(font, T(S_SIDEBAR_INFO), Embed_MenuOpacity(), 0.0f);
        } else if (currentState == MOSAIC || currentState == LAUNCHING) {
            DrawMosaic(Mosaicbg, font);
            Details_Draw(font);
            if (embedBackT >= 0.0f)
                Embed_DrawBack(sysSettings[currentSystem].screen, SelectedCardRect(1.0f), embedBackT);
            if (statesOpen && GameIdx < ActiveGameCount()) {
                States_Draw(font, ActiveGames()[GameIdx].title);
            }
        }

        if (isLaunching) {   /* every game: the card grows into the 4:3 game frame */
            float t = launchFadeAnim.timer / launchFadeAnim.duration;
            float u = fminf(1.0f, fmaxf(0.0f, (t - 0.75f) / 0.25f));   /* then the rest goes black */
            float p = Embed_BackEase(t), sw = (float)GetScreenWidth(), sh = (float)GetScreenHeight();
            /* Black meets the game's black first frame, so the end rect needs no exact size. */
            Embed_DrawZoom((Texture2D){ 0 }, Embed_DestRect(EMBED_SCREEN_43, sw, sh, 4, 3), SelectedCardRect(animScales[GameIdx]),
                           p, 1.0f - (1.0f - t) * (1.0f - t), 0.35f * p + 0.65f * u * u * (3.0f - 2.0f * u));
        }

        if (waitingForGame) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        } else if (!isLaunching && currentState != EMBED) {
            Repo_NoticeDraw(font);
            PadNotice_Draw(font);
        }

        EndDrawing();
    }

    Embed_Stop();
    CoverLoader_Stop();
    if (NsoProcOk(repoProc)) NsoSpawnKill(repoProc);
    States_Unload();
    UnloadAllBoxarts();
    UnloadFont(font);
    if (gearTexture.id) UnloadTexture(gearTexture);
    PadNotice_Unload();
    RetroShots_Unload();
    UnloadBlurResources();
    SidebarIcons_Unload();
    UnloadSystems();
    Sfx_Shutdown();
    CloseWindow();

    return EXIT_SUCCESS;
}
