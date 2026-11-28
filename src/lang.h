/* lang.h - UI strings (English default, Portuguese pt-BR) + language config.
 * No raylib: unit-tested in lang_test.c. T(id) returns the active language's
 * string; NsoMsg keeps a status as (id, args) so it re-renders when the
 * language changes at runtime. */
#ifndef NSO_LANG_H
#define NSO_LANG_H

#include <stdbool.h>
#include <stddef.h>

typedef enum { LANG_EN = 0, LANG_PT, LANG_COUNT } NsoLang;

typedef enum {
    S_RAW = 0,                 /* NsoMsg only: show arg verbatim */
    S_MOSAIC_INFO, S_SIDEBAR_INFO, S_INFO_CANCEL, S_MANAGE_INFO, S_MOVE_INFO,
    /* scraping */
    S_SCRAPE_DONE, S_SCRAPE_DONE_MISSED, S_SCRAPE_NO_SOURCE, S_SCRAPE_DOWNLOAD_FAILED,
    S_SCRAPE_INDEX, S_SCRAPE_NO_CURL, S_SCRAPE_CANCELLED, S_SCRAPE_INDEX_TOO_LARGE,
    S_SCRAPE_NO_INDEX,
    /* gear screen */
    S_MANAGE_COVERS, S_MANAGE_MISSING, S_MANAGE_REPLACE, S_MANAGE_REPOS, S_MANAGE_NO_REPOS,
    S_MANAGE_ALL_SYSTEM, S_MANAGE_ALL, S_MANAGE_QUEUED_N, S_MANAGE_SETTINGS, S_MANAGE_LANGUAGE,
    S_LANG_NAME,
    /* repositories */
    S_REPO_NOT_CLONED, S_REPO_LOCAL_COPY, S_REPO_LOCAL_READY, S_REPO_LOCAL_UNBUILT,
    S_REPO_NOT_BUILT, S_REPO_ROM_NEEDED, S_REPO_READY, S_REPO_UP_TO_DATE, S_REPO_MISSING_DEPS,
    S_REPO_CLONING, S_REPO_CLONE_FAILED, S_REPO_CHECKING, S_REPO_FETCH_FAILED,
    S_REPO_BAD_CHECKOUT, S_REPO_NO_UPSTREAM, S_REPO_UPDATING, S_REPO_UPDATE_FAILED,
    S_REPO_SUBMODULE_FAILED, S_REPO_BUILDING, S_REPO_BUILD_FAILED, S_REPO_BUILD_NO_BINARY,
    S_REPO_DOWNLOAD_FAILED, S_REPO_NO_SH, S_REPO_FAILED, S_REPO_QUEUED, S_REPO_START_FAILED,
    /* install watch (notice, gear row, Details button) */
    S_REPO_INSTALLABLE, S_NOTE_INSTALL, S_NOTE_ROM, S_NOTE_INSTALLED, S_DET_INSTALL,
    /* details panel (grid X) */
    S_DET_SYSTEM, S_DET_SOURCE, S_DET_STATUS,
    S_DET_INSTALLED, S_DET_NOT_INSTALLED, S_DET_UNAVAILABLE, S_DET_INFO,
    S_DET_VERSION,
    /* in-system sidebar (grid B/O menu) and its Settings panel */
    S_SB_SINGLE, S_SB_TOGETHER, S_SB_ONLINE, S_SB_SETTINGS, S_COMING_SOON,
    S_SET_VERSIONS, S_SET_VER_NA, S_SET_VER_NA_EU, S_SET_RUMBLE, S_SET_ENABLED,
    S_SET_CONTROLS, S_SET_SHOW, S_SET_SCREEN, S_SET_43, S_SET_PIXEL, S_SET_CRT, S_SET_SCANLINES, S_SET_CRTGEOM, S_SET_CTRL_DISPLAY,
    /* sort panel (grid T) */ S_SORT_HEAD, S_SORT_CUSTOM, S_SORT_TITLE, S_SORT_RELEASE, S_SORT_PLAYED, S_SORT_TIME,
    S_SORT_FILTER, S_SORT_FAVORITES, S_SORT_HIDDEN,
    /* "Select games" (Settings > Image scraping, gamesel_ui.h) */
    S_GS_SELECT, S_GS_START, S_GS_NO_ROMS, S_GS_HINT, S_GS_HINT_RUN, S_GS_INSTALLER, S_GS_INSTALLED,
    S_GS_QUEUED, S_GS_DOWNLOADING, S_GS_WAIT_BUILD, S_GS_BUILDING, S_GS_COVER, S_GS_READY,
    S_GS_NO_INSTALLER, S_GS_FAILED, S_GS_CANCELLED, S_GS_DONE,
    S_GS_FILTER_ALL, S_GS_FILTER_USA, S_GS_FILTER_JP,
    /* Sort menu > Hide unplayable, and the grid when nothing is playable */
    S_SET_HIDE, S_NO_PLAYABLE, S_NO_FAVORITES, S_NO_HIDDEN,
    /* Settings > Controller (padmap.h, padui.h); S_PC_UP.. are the button names; padui.h orders them per system */
    S_PC_HEAD, S_PC_ENTRY, S_PC_UP, S_PC_DOWN, S_PC_LEFT, S_PC_RIGHT, S_PC_SELECT, S_PC_START, S_PC_B, S_PC_A,
    S_PC_ENTRY_SNES, S_PC_X, S_PC_Y, S_PC_L, S_PC_R, S_PC_ZL, S_PC_ZR,
    S_PC_ENTRY_N64, S_PC_DP_UP, S_PC_DP_DOWN, S_PC_DP_LEFT, S_PC_DP_RIGHT, S_PC_ST_UP, S_PC_ST_DOWN, S_PC_ST_LEFT, S_PC_ST_RIGHT,
    S_PC_Z, S_PC_C_UP, S_PC_C_DOWN, S_PC_C_LEFT, S_PC_C_RIGHT,
    S_PC_ENTRY_GC, S_PC_L_ANALOG, S_PC_R_ANALOG, S_PC_CS_UP, S_PC_CS_DOWN, S_PC_CS_LEFT, S_PC_CS_RIGHT,
    S_PC_PRESS, S_PC_SKIP, S_PC_BAR, S_PC_BAR_MAP,
    /* embedded game pause menu (embed.h) */
    S_EMB_CONTINUE, S_EMB_LOAD, S_EMB_SAVE, S_EMB_RESTART, S_EMB_QUIT,
    S_EMB_SLOT, S_EMB_NODATA, S_EMB_DATE, S_EMB_LOADING,
    /* CRT Geometry fine settings: the pause menu's item and its sub-screen (embed.h); the twelve labels
     * follow crtparams.h's CRTP_* order */
    S_EMB_CRT,
    S_CRT_GAMMA, S_CRT_MGAMMA, S_CRT_OVERSCAN, S_CRT_SAT, S_CRT_CURV, S_CRT_DIST, S_CRT_ANGLE, S_CRT_ASPECT,
    S_CRT_CORNER, S_CRT_CSMOOTH, S_CRT_HALATION, S_CRT_FILMIC,
    S_CRT_RESET, S_EMB_CRT_INFO,
    /* game rewind caption bar (rewind.h) */
    S_RW_HOLD, S_RW_SUSPEND, S_RW_BACK, S_RW_OK,
    /* N64 controller connect pop-up */
    S_PAD_N64_CONNECTED,
    /* keep-alive: a game left running; confirm before closing it for another */
    S_LIVE_CLOSE,   /* "Close %s?" / "Fechar %s?" */
    S_LIVE_YES,
    S_LIVE_NO,
    /* main-menu power dialog */
    S_PWR_SHUTDOWN, S_PWR_CLOSE, S_PWR_CANCEL,
    /* main-menu dock labels (focused bar icon) */
    S_BAR_DOWNLOADS, S_BAR_SHUTDOWN,
    /* status + scraper menu (grid S): tabs, legend, emulated-tab extras */
    S_TAB_PORTS, S_TAB_DECOMPS, S_TAB_EMU, S_TAB_STATS,
    S_TAB_PLACEHOLDER, S_STATUS_LEGEND, S_STATUS_LEGEND_SCRAPER, S_EMU_NO_ROMS, S_EMU_CHOOSE,
    S_EMU_DOWNLOAD_ALL, S_EMU_START_COVERS, S_NO_MEDNAFEN, S_SCRAPE_NO_ART,
    S_UPD_AVAILABLE, S_UPD_INSTALLING, S_UPD_FAILED,
    S_SCRAPE_STEP_DATES, S_SCRAPE_STEP_INDEX,
    S_COUNT
} StrId;

typedef struct { int id; int n; char arg[160]; } NsoMsg;   /* id < 0 = empty */

extern const char *const nso_strings[LANG_COUNT][S_COUNT];

void Lang_Set(NsoLang lang);
NsoLang Lang_Get(void);
const char *T(StrId id);
/* Config text -> language; anything missing / unknown = English. */
NsoLang Lang_Parse(const char *text);
NsoLang Lang_Load(const char *path);
bool Lang_Save(const char *path, NsoLang lang);   /* keeps the file's other keys */

/* config/nso.cfg is "key=value" lines. Cfg_Get copies key's value (false if
 * absent); Cfg_Put writes text with key set (replaced in place or appended),
 * keeping every other line (false if out is too small). Cfg_SaveKey does a
 * read-modify-write of the file. */
bool Cfg_Get(const char *text, const char *key, char *dst, size_t n);
bool Cfg_Put(const char *text, const char *key, const char *value, char *out, size_t n);
bool Cfg_Remove(const char *text, const char *key, char *out, size_t n);
bool Cfg_SaveKey(const char *path, const char *key, const char *value);

/* NES/SNES Game Screen: "<id>.pixel" and "<id>.scanlines" (0/1), default both
 * on. "<id>.screen_ver" = "2" means the one-time reset has been applied
 * (both toggles forced on, leftover "<id>.screen" dropped) and later user
 * choices are kept. Cfg_RetroScreenLoad returns true when that stamp is
 * still needed (*pixel and *scanlines are then both true, ignoring old
 * values). Cfg_RetroScreenStamp writes ON+ON, the marker, and drops
 * "<id>.screen". */
#define NSO_RETRO_SCREEN_VER "2"
bool Cfg_RetroScreenLoad(const char *text, const char *id, bool *pixel, bool *scanlines);
bool Cfg_RetroScreenStamp(const char *text, const char *id, char *out, size_t n);

void Msg_Clear(NsoMsg *m);
void Msg_Set(NsoMsg *m, int id, int n, const char *arg);
/* "%d" uses n, "%s" uses arg; otherwise a non-empty arg is appended as ": arg". */
void Msg_Format(const NsoMsg *m, char *dst, size_t size);

#endif /* NSO_LANG_H */

#ifdef LANG_IMPLEMENTATION
#ifndef LANG_IMPLEMENTATION_ONCE
#define LANG_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <string.h>

const char *const nso_strings[LANG_COUNT][S_COUNT] = {
    [LANG_EN] = {
        [S_RAW] = "%s",
        [S_MOSAIC_INFO] = "    🅨  Move (Press and hold to sort)      🅧 Details      🅐 Start      🅑 Back   ",
        [S_SIDEBAR_INFO] = "    🅑 Back      🅐 OK   ",
        [S_INFO_CANCEL] = "    🅑 Cancel   ",
        [S_MANAGE_INFO] = "    🅐 Select      🅑 Back   ",
        [S_MOVE_INFO] = "    🅐 Confirm      🅑 Cancel Move   ",
        [S_SCRAPE_DONE] = "Done",
        [S_SCRAPE_DONE_MISSED] = "Done, %d missed",
        [S_SCRAPE_NO_SOURCE] = "No art source",
        [S_SCRAPE_DOWNLOAD_FAILED] = "Download failed",
        [S_SCRAPE_INDEX] = "Index",
        [S_SCRAPE_NO_CURL] = "curl not found",
        [S_SCRAPE_CANCELLED] = "Cancelled",
        [S_SCRAPE_INDEX_TOO_LARGE] = "Index too large",
        [S_SCRAPE_NO_INDEX] = "No index",
        [S_MANAGE_COVERS] = "Image scraping",
        [S_MANAGE_MISSING] = "Missing box art",
        [S_MANAGE_REPLACE] = "Replace box art",
        [S_MANAGE_REPOS] = "Repository download",
        [S_MANAGE_NO_REPOS] = "No repositories for this system yet",
        [S_MANAGE_ALL_SYSTEM] = "Download all from %s",
        [S_MANAGE_ALL] = "Download all (all systems)",
        [S_MANAGE_QUEUED_N] = "%d queued",
        [S_MANAGE_SETTINGS] = "Settings",
        [S_MANAGE_LANGUAGE] = "Language",
        [S_LANG_NAME] = "English",
        [S_REPO_NOT_CLONED] = "Not downloaded",
        [S_REPO_LOCAL_COPY] = "Local copy",
        [S_REPO_LOCAL_READY] = "Local copy, ready",
        [S_REPO_LOCAL_UNBUILT] = "Local copy, not built",
        [S_REPO_NOT_BUILT] = "Not built",
        [S_REPO_ROM_NEEDED] = "ROM needed",
        [S_REPO_READY] = "Ready",
        [S_REPO_UP_TO_DATE] = "Up to date",
        [S_REPO_MISSING_DEPS] = "Missing deps",
        [S_REPO_CLONING] = "Cloning",
        [S_REPO_CLONE_FAILED] = "Clone failed",
        [S_REPO_CHECKING] = "Checking for updates",
        [S_REPO_FETCH_FAILED] = "Fetch failed",
        [S_REPO_BAD_CHECKOUT] = "Bad checkout",
        [S_REPO_NO_UPSTREAM] = "No upstream branch",
        [S_REPO_UPDATING] = "Updating",
        [S_REPO_UPDATE_FAILED] = "Update failed (local changes?)",
        [S_REPO_SUBMODULE_FAILED] = "Submodule update failed",
        [S_REPO_BUILDING] = "Building",
        [S_REPO_BUILD_FAILED] = "Build failed (see log)",
        [S_REPO_BUILD_NO_BINARY] = "Build made no binary",
        [S_REPO_DOWNLOAD_FAILED] = "Download failed",
        [S_REPO_NO_SH] = "sh not found",
        [S_REPO_FAILED] = "Failed",
        [S_REPO_QUEUED] = "Queued",
        [S_REPO_START_FAILED] = "Could not start sh",
        [S_REPO_INSTALLABLE] = "Ready to install",
        [S_NOTE_INSTALL] = "%s is ready - press 🅐 to install",
        [S_NOTE_ROM] = "%s downloaded - put %s in %s",
        [S_NOTE_INSTALLED] = "%s is installed - ready to play",
        [S_DET_INSTALL] = "Install",
        [S_DET_SYSTEM] = "System",
        [S_DET_SOURCE] = "Source",
        [S_DET_STATUS] = "Status",
        [S_DET_INSTALLED] = "Installed",
        [S_DET_NOT_INSTALLED] = "Not installed",
        [S_DET_UNAVAILABLE] = "Not available yet",
        [S_DET_INFO] = "    🅐 Open      🅑 Back      🅨 Favorite   ",
        [S_DET_VERSION] = "Version",
        [S_SB_SINGLE] = "One player",
        [S_SB_TOGETHER] = "Two Players",
        [S_SB_ONLINE] = "Play online",
        [S_SB_SETTINGS] = "Settings",
        [S_COMING_SOON] = "Coming soon",
        [S_SET_VERSIONS] = "Playable versions",
        [S_SET_VER_NA] = "North American versions",
        [S_SET_VER_NA_EU] = "North American and European versions",
        [S_SET_RUMBLE] = "Rumble",
        [S_SET_ENABLED] = "Enabled",
        [S_SET_CONTROLS] = "Show controls in game",
        [S_SET_SHOW] = "Show",
        [S_SET_SCREEN] = "Game Screen",
        [S_SET_43] = "4:3",
        [S_SET_PIXEL] = "Pixel Perfect",
        [S_SET_CRT] = "CRT Filter",
        [S_SET_SCANLINES] = "Scanlines",
        [S_SET_CRTGEOM] = "CRT Geometry",
        [S_SET_CTRL_DISPLAY] = "Control Display",
        [S_SORT_HEAD] = "Change order of titles",
        [S_SORT_CUSTOM] = "By custom order",
        [S_SORT_TITLE] = "By title",
        [S_SORT_RELEASE] = "By release date",
        [S_SORT_PLAYED] = "By time last played",
        [S_SORT_TIME] = "By total play time",
        [S_SORT_FILTER] = "Filter",
        [S_SORT_FAVORITES] = "Favorites",
        [S_SORT_HIDDEN] = "Show hidden",
        [S_GS_SELECT] = "Select games",
        [S_SET_HIDE] = "Hide unplayable",
        [S_NO_PLAYABLE] = "No playable games here. Turn off Hide unplayable in the sort menu.",
        [S_NO_FAVORITES] = "No favorite games here. Favorite a game from its info menu, or turn off the Favorites filter.",
        [S_NO_HIDDEN] = "No hidden games here. Hide a game from its info menu, or turn off Show hidden.",
        [S_PC_HEAD] = "Controller",
        [S_PC_ENTRY] = "NES controller",
        [S_PC_UP] = "Up",
        [S_PC_DOWN] = "Down",
        [S_PC_LEFT] = "Left",
        [S_PC_RIGHT] = "Right",
        [S_PC_SELECT] = "Select",
        [S_PC_START] = "Start",
        [S_PC_B] = "B",
        [S_PC_A] = "A",
        [S_PC_ENTRY_SNES] = "SNES controller",
        [S_PC_X] = "X",
        [S_PC_Y] = "Y",
        [S_PC_L] = "L",
        [S_PC_R] = "R",
        [S_PC_ZL] = "ZL",
        [S_PC_ZR] = "ZR",
        [S_PC_ENTRY_N64] = "N64 controller",
        [S_PC_DP_UP] = "D-Pad Up",
        [S_PC_DP_DOWN] = "D-Pad Down",
        [S_PC_DP_LEFT] = "D-Pad Left",
        [S_PC_DP_RIGHT] = "D-Pad Right",
        [S_PC_ST_UP] = "Stick Up",
        [S_PC_ST_DOWN] = "Stick Down",
        [S_PC_ST_LEFT] = "Stick Left",
        [S_PC_ST_RIGHT] = "Stick Right",
        [S_PC_Z] = "Z",
        [S_PC_C_UP] = "C-Up",
        [S_PC_C_DOWN] = "C-Down",
        [S_PC_C_LEFT] = "C-Left",
        [S_PC_C_RIGHT] = "C-Right",
        [S_PC_ENTRY_GC] = "GameCube controller",
        [S_PC_L_ANALOG] = "L Analog",
        [S_PC_R_ANALOG] = "R Analog",
        [S_PC_CS_UP] = "C-Stick Up",
        [S_PC_CS_DOWN] = "C-Stick Down",
        [S_PC_CS_LEFT] = "C-Stick Left",
        [S_PC_CS_RIGHT] = "C-Stick Right",
        [S_PC_PRESS] = "Press",
        [S_PC_SKIP] = "Hold to skip",
        [S_PC_BAR] = "    Hold 🅐: Remap      Hold ZR: Reset to default      Hold 🅑: Back   ",
        [S_PC_BAR_MAP] = "    Esc: Cancel   ",
        [S_GS_START] = "Download and install (%d)",
        [S_GS_NO_ROMS] = "No ROMs in roms/ for this system",
        [S_GS_HINT] = "A: check   B: back",
        [S_GS_HINT_RUN] = "B: cancel",
        [S_GS_INSTALLER] = "Installer",
        [S_GS_INSTALLED] = "Installed",
        [S_GS_QUEUED] = "Queued",
        [S_GS_DOWNLOADING] = "Downloading",
        [S_GS_WAIT_BUILD] = "Waiting to build",
        [S_GS_BUILDING] = "Building",
        [S_GS_COVER] = "Cover",
        [S_GS_READY] = "Ready",
        [S_GS_NO_INSTALLER] = "No installer yet",
        [S_GS_FAILED] = "Failed",
        [S_GS_CANCELLED] = "Cancelled",
        [S_GS_DONE] = "Done: %d ready",
        [S_GS_FILTER_ALL] = "All regions",
        [S_GS_FILTER_USA] = "Released in USA",
        [S_GS_FILTER_JP] = "Released in Japan",
        [S_EMB_CONTINUE] = "Continue",
        [S_EMB_LOAD] = "Load Suspend Point",
        [S_EMB_SAVE] = "Create Suspend Point",
        [S_EMB_RESTART] = "Reset Game",
        [S_EMB_QUIT] = "To Game Selection",
        [S_EMB_SLOT] = "Save slot %d",
        [S_EMB_NODATA] = "No data.",
        [S_EMB_DATE] = "%m/%d/%Y - %H:%M",
        [S_EMB_LOADING] = "Restoring suspend point...",
        [S_EMB_CRT] = "CRT Geometry",
        [S_CRT_GAMMA] = "CRT Gamma",
        [S_CRT_MGAMMA] = "Monitor Gamma",
        [S_CRT_OVERSCAN] = "Overscan",
        [S_CRT_SAT] = "Saturation",
        [S_CRT_CURV] = "Curvature",
        [S_CRT_DIST] = "Distance",
        [S_CRT_ANGLE] = "Angle",
        [S_CRT_ASPECT] = "Aspect",
        [S_CRT_CORNER] = "Corner Size",
        [S_CRT_CSMOOTH] = "Corner Smooth",
        [S_CRT_HALATION] = "Halation",
        [S_CRT_FILMIC] = "Filmic",
        [S_CRT_RESET] = "Reset to defaults",
        [S_EMB_CRT_INFO] = "    🅑 Back      Hold 🅐: Reset   ",
        [S_RW_HOLD] = "Suspend Menu (Press and hold to rewind)",
        [S_RW_SUSPEND] = "Suspend Menu",
        [S_RW_BACK] = "Back",
        [S_RW_OK] = "OK",
        [S_PAD_N64_CONNECTED] = "N64 controller connected",
        [S_LIVE_CLOSE] = "Close %s?",
        [S_LIVE_YES] = "Yes",
        [S_LIVE_NO] = "No",
        [S_PWR_SHUTDOWN] = "Shut down",
        [S_PWR_CLOSE] = "Close NSO",
        [S_PWR_CANCEL] = "Cancel",
        [S_BAR_DOWNLOADS] = "Downloads",
        [S_BAR_SHUTDOWN] = "Shutdown",
        [S_TAB_PORTS] = "PC Ports",
        [S_TAB_DECOMPS] = "Decomps",
        [S_TAB_EMU] = "Scraper",
        [S_TAB_STATS] = "Statistics",
        [S_TAB_PLACEHOLDER] = "Nothing here yet.",
        [S_STATUS_LEGEND] = "    🅑 Back      🅐 Accept      󰰴 Change tabs   ",
        [S_STATUS_LEGEND_SCRAPER] = "    🅑 Back      🅐 Accept      󰰱 Download all      󰰴 Change tabs   ",
        [S_EMU_NO_ROMS] = "No ROMs in roms/%s",
        [S_EMU_CHOOSE] = "Choose box art",
        [S_EMU_DOWNLOAD_ALL] = "Download all box arts",
        [S_EMU_START_COVERS] = "Download box art (%d)",
        [S_NO_MEDNAFEN] = "Bundled Mednafen is missing",
        [S_SCRAPE_NO_ART] = "No box art found",
        [S_UPD_AVAILABLE] = "New version %s available",
        [S_UPD_INSTALLING] = "Updating to %s",
        [S_UPD_FAILED] = "Update failed",
        [S_SCRAPE_STEP_DATES] = "Release dates…",
        [S_SCRAPE_STEP_INDEX] = "Image index…",
    },
    [LANG_PT] = {
        [S_RAW] = "%s",
        [S_MOSAIC_INFO] = "    🅨  Mover (segure para ordenar)      🅧 Detalhes      🅐 Iniciar      🅑 Voltar   ",
        [S_SIDEBAR_INFO] = "    🅑 Voltar      🅐 OK   ",
        [S_INFO_CANCEL] = "    🅑 Cancelar   ",
        [S_MANAGE_INFO] = "    🅐 Selecionar      🅑 Voltar   ",
        [S_MOVE_INFO] = "    🅐 Confirmar      🅑 Cancelar movimento   ",
        [S_SCRAPE_DONE] = "Concluído",
        [S_SCRAPE_DONE_MISSED] = "Concluído, %d sem capa",
        [S_SCRAPE_NO_SOURCE] = "Sem fonte de capas",
        [S_SCRAPE_DOWNLOAD_FAILED] = "Falha no download",
        [S_SCRAPE_INDEX] = "Índice",
        [S_SCRAPE_NO_CURL] = "curl não encontrado",
        [S_SCRAPE_CANCELLED] = "Cancelado",
        [S_SCRAPE_INDEX_TOO_LARGE] = "Índice grande demais",
        [S_SCRAPE_NO_INDEX] = "Sem índice",
        [S_MANAGE_COVERS] = "Capas dos jogos",
        [S_MANAGE_MISSING] = "Baixar capas que faltam",
        [S_MANAGE_REPLACE] = "Substituir todas as capas",
        [S_MANAGE_REPOS] = "Repositórios",
        [S_MANAGE_NO_REPOS] = "Nenhum repositório para este sistema ainda",
        [S_MANAGE_ALL_SYSTEM] = "Baixar todos do %s",
        [S_MANAGE_ALL] = "Baixar todos (todos os sistemas)",
        [S_MANAGE_QUEUED_N] = "%d na fila",
        [S_MANAGE_SETTINGS] = "Configurações",
        [S_MANAGE_LANGUAGE] = "Idioma",
        [S_LANG_NAME] = "Português (Brasil)",
        [S_REPO_NOT_CLONED] = "Não baixado",
        [S_REPO_LOCAL_COPY] = "Cópia local",
        [S_REPO_LOCAL_READY] = "Cópia local, pronta",
        [S_REPO_LOCAL_UNBUILT] = "Cópia local, sem build",
        [S_REPO_NOT_BUILT] = "Sem build",
        [S_REPO_ROM_NEEDED] = "Precisa da ROM",
        [S_REPO_READY] = "Pronto",
        [S_REPO_UP_TO_DATE] = "Atualizado",
        [S_REPO_MISSING_DEPS] = "Faltam dependências",
        [S_REPO_CLONING] = "Clonando",
        [S_REPO_CLONE_FAILED] = "Falha ao clonar",
        [S_REPO_CHECKING] = "Verificando atualizações",
        [S_REPO_FETCH_FAILED] = "Falha ao verificar",
        [S_REPO_BAD_CHECKOUT] = "Checkout inválido",
        [S_REPO_NO_UPSTREAM] = "Sem branch remoto",
        [S_REPO_UPDATING] = "Atualizando",
        [S_REPO_UPDATE_FAILED] = "Falha ao atualizar (mudanças locais?)",
        [S_REPO_SUBMODULE_FAILED] = "Falha nos submódulos",
        [S_REPO_BUILDING] = "Compilando",
        [S_REPO_BUILD_FAILED] = "Falha na compilação (ver log)",
        [S_REPO_BUILD_NO_BINARY] = "Compilação não gerou o binário",
        [S_REPO_DOWNLOAD_FAILED] = "Falha no download",
        [S_REPO_NO_SH] = "sh não encontrado",
        [S_REPO_FAILED] = "Falhou",
        [S_REPO_QUEUED] = "Na fila",
        [S_REPO_START_FAILED] = "Falha ao iniciar sh",
        [S_REPO_INSTALLABLE] = "Pronto para instalar",
        [S_NOTE_INSTALL] = "%s está pronto - aperte 🅐 para instalar",
        [S_NOTE_ROM] = "%s baixado - coloque %s em %s",
        [S_NOTE_INSTALLED] = "%s está instalado - pronto para jogar",
        [S_DET_INSTALL] = "Instalar",
        [S_DET_SYSTEM] = "Sistema",
        [S_DET_SOURCE] = "Origem",
        [S_DET_STATUS] = "Situação",
        [S_DET_INSTALLED] = "Instalado",
        [S_DET_NOT_INSTALLED] = "Não instalado",
        [S_DET_UNAVAILABLE] = "Ainda não disponível",
        [S_DET_INFO] = "    🅐 Abrir      🅑 Voltar      🅨 Favorito   ",
        [S_DET_VERSION] = "Versão",
        [S_SB_SINGLE] = "Um jogador",
        [S_SB_TOGETHER] = "Jogar junto",
        [S_SB_ONLINE] = "Jogar online",
        [S_SB_SETTINGS] = "Configurações",
        [S_COMING_SOON] = "Em breve",
        [S_SET_VERSIONS] = "Versões jogáveis",
        [S_SET_VER_NA] = "Versões norte-americanas",
        [S_SET_VER_NA_EU] = "Versões norte-americanas e europeias",
        [S_SET_RUMBLE] = "Função de vibração",
        [S_SET_ENABLED] = "Ativado",
        [S_SET_CONTROLS] = "Mostrar controles no jogo",
        [S_SET_SHOW] = "Mostrar",
        [S_SET_SCREEN] = "Tela do jogo",
        [S_SET_43] = "4:3",
        [S_SET_PIXEL] = "Pixel Perfect",
        [S_SET_CRT] = "Filtro CRT",
        [S_SET_SCANLINES] = "Scanlines",
        [S_SET_CRTGEOM] = "Geometria CRT",
        [S_SET_CTRL_DISPLAY] = "Exibição de controles",
        [S_SORT_HEAD] = "Alterar ordem dos títulos",
        [S_SORT_CUSTOM] = "Ordem personalizada",
        [S_SORT_TITLE] = "Por título",
        [S_SORT_RELEASE] = "Por data de lançamento",
        [S_SORT_PLAYED] = "Por último jogado",
        [S_SORT_TIME] = "Por tempo total de jogo",
        [S_SORT_FILTER] = "Filtro",
        [S_SORT_FAVORITES] = "Favoritos",
        [S_SORT_HIDDEN] = "Mostrar ocultos",
        [S_GS_SELECT] = "Selecionar jogos",
        [S_SET_HIDE] = "Ocultar não jogáveis",
        [S_NO_PLAYABLE] = "Nenhum jogo jogável aqui. Desative Ocultar não jogáveis no menu de ordem.",
        [S_NO_FAVORITES] = "Nenhum jogo favorito aqui. Marque um favorito no menu de informações do jogo, ou desative o filtro Favoritos.",
        [S_NO_HIDDEN] = "Nenhum jogo oculto aqui. Oculte um jogo no menu de informações, ou desative Mostrar ocultos.",
        [S_PC_HEAD] = "Controle",
        [S_PC_ENTRY] = "Controle do NES",
        [S_PC_UP] = "Cima",
        [S_PC_DOWN] = "Baixo",
        [S_PC_LEFT] = "Esquerda",
        [S_PC_RIGHT] = "Direita",
        [S_PC_SELECT] = "Select",
        [S_PC_START] = "Start",
        [S_PC_B] = "B",
        [S_PC_A] = "A",
        [S_PC_ENTRY_SNES] = "Controle do SNES",
        [S_PC_X] = "X",
        [S_PC_Y] = "Y",
        [S_PC_L] = "L",
        [S_PC_R] = "R",
        [S_PC_ZL] = "ZL",
        [S_PC_ZR] = "ZR",
        [S_PC_ENTRY_N64] = "Controle do N64",
        [S_PC_DP_UP] = "Direcional cima",
        [S_PC_DP_DOWN] = "Direcional baixo",
        [S_PC_DP_LEFT] = "Direcional esquerda",
        [S_PC_DP_RIGHT] = "Direcional direita",
        [S_PC_ST_UP] = "Analógico cima",
        [S_PC_ST_DOWN] = "Analógico baixo",
        [S_PC_ST_LEFT] = "Analógico esquerda",
        [S_PC_ST_RIGHT] = "Analógico direita",
        [S_PC_Z] = "Z",
        [S_PC_C_UP] = "C cima",
        [S_PC_C_DOWN] = "C baixo",
        [S_PC_C_LEFT] = "C esquerda",
        [S_PC_C_RIGHT] = "C direita",
        [S_PC_ENTRY_GC] = "Controle do GameCube",
        [S_PC_L_ANALOG] = "L analógico",
        [S_PC_R_ANALOG] = "R analógico",
        [S_PC_CS_UP] = "C-Stick cima",
        [S_PC_CS_DOWN] = "C-Stick baixo",
        [S_PC_CS_LEFT] = "C-Stick esquerda",
        [S_PC_CS_RIGHT] = "C-Stick direita",
        [S_PC_PRESS] = "Pressione",
        [S_PC_SKIP] = "Segure para pular",
        [S_PC_BAR] = "    Segure 🅐: Remapear      Segure ZR: Restaurar padrão      Segure 🅑: Voltar   ",
        [S_PC_BAR_MAP] = "    Esc: Cancelar   ",
        [S_GS_START] = "Baixar e instalar (%d)",
        [S_GS_NO_ROMS] = "Nenhuma ROM em roms/ para este sistema",
        [S_GS_HINT] = "A: marcar   B: voltar",
        [S_GS_HINT_RUN] = "B: cancelar",
        [S_GS_INSTALLER] = "Instalador",
        [S_GS_INSTALLED] = "Instalado",
        [S_GS_QUEUED] = "Na fila",
        [S_GS_DOWNLOADING] = "Baixando",
        [S_GS_WAIT_BUILD] = "Aguardando compilação",
        [S_GS_BUILDING] = "Compilando",
        [S_GS_COVER] = "Capa",
        [S_GS_READY] = "Pronto",
        [S_GS_NO_INSTALLER] = "Sem instalador ainda",
        [S_GS_FAILED] = "Falhou",
        [S_GS_CANCELLED] = "Cancelado",
        [S_GS_DONE] = "Concluído: %d prontos",
        [S_GS_FILTER_ALL] = "Todas as regiões",
        [S_GS_FILTER_USA] = "Lançado nos EUA",
        [S_GS_FILTER_JP] = "Lançado no Japão",
        [S_EMB_CONTINUE] = "Continuar",
        [S_EMB_LOAD] = "Carregar ponto de suspensão",
        [S_EMB_SAVE] = "Criar ponto de suspensão",
        [S_EMB_RESTART] = "Reiniciar jogo",
        [S_EMB_QUIT] = "Voltar à seleção de jogos",
        [S_EMB_SLOT] = "Espaço %d",
        [S_EMB_NODATA] = "Sem dados.",
        [S_EMB_DATE] = "%d/%m/%Y - %H:%M",
        [S_EMB_LOADING] = "Restaurando ponto de suspensão...",
        [S_EMB_CRT] = "Geometria CRT",
        [S_CRT_GAMMA] = "Gama do CRT",
        [S_CRT_MGAMMA] = "Gama do Monitor",
        [S_CRT_OVERSCAN] = "Overscan",
        [S_CRT_SAT] = "Saturação",
        [S_CRT_CURV] = "Curvatura",
        [S_CRT_DIST] = "Distância",
        [S_CRT_ANGLE] = "Ângulo",
        [S_CRT_ASPECT] = "Aspecto",
        [S_CRT_CORNER] = "Tamanho do Canto",
        [S_CRT_CSMOOTH] = "Suavidade do Canto",
        [S_CRT_HALATION] = "Halation",
        [S_CRT_FILMIC] = "Fílmico",
        [S_CRT_RESET] = "Restaurar padrões",
        [S_EMB_CRT_INFO] = "    🅑 Voltar      Segure 🅐: Restaurar   ",
        [S_RW_HOLD] = "Menu de suspensão (Mantenha pressionado para voltar)",
        [S_RW_SUSPEND] = "Menu de suspensão",
        [S_RW_BACK] = "Voltar",
        [S_RW_OK] = "OK",
        [S_PAD_N64_CONNECTED] = "Controle N64 conectado",
        [S_LIVE_CLOSE] = "Fechar %s?",
        [S_LIVE_YES] = "Sim",
        [S_LIVE_NO] = "Não",
        [S_PWR_SHUTDOWN] = "Desligar",
        [S_PWR_CLOSE] = "Fechar o NSO",
        [S_PWR_CANCEL] = "Cancelar",
        [S_BAR_DOWNLOADS] = "Downloads",
        [S_BAR_SHUTDOWN] = "Desligar",
        [S_TAB_PORTS] = "Ports para PC",
        [S_TAB_DECOMPS] = "Decomps",
        [S_TAB_EMU] = "Scraper",
        [S_TAB_STATS] = "Estatísticas",
        [S_TAB_PLACEHOLDER] = "Nada aqui ainda.",
        [S_STATUS_LEGEND] = "    🅑 Voltar      🅐 Confirmar      󰰴 Trocar abas   ",
        [S_STATUS_LEGEND_SCRAPER] = "    🅑 Voltar      🅐 Confirmar      󰰱 Baixar todas      󰰴 Trocar abas   ",
        [S_EMU_NO_ROMS] = "Nenhuma ROM em roms/%s",
        [S_EMU_CHOOSE] = "Escolher capa",
        [S_EMU_DOWNLOAD_ALL] = "Baixar todas as capas",
        [S_EMU_START_COVERS] = "Baixar capas (%d)",
        [S_NO_MEDNAFEN] = "O Mednafen empacotado não foi encontrado",
        [S_SCRAPE_NO_ART] = "Nenhuma capa encontrada",
        [S_UPD_AVAILABLE] = "Nova versão %s disponível",
        [S_UPD_INSTALLING] = "Atualizando para %s",
        [S_UPD_FAILED] = "Falha na atualização",
        [S_SCRAPE_STEP_DATES] = "Datas de lançamento…",
        [S_SCRAPE_STEP_INDEX] = "Índice de imagens…",
    },
};

static NsoLang lang_current = LANG_EN;

void Lang_Set(NsoLang lang) { lang_current = (lang >= 0 && lang < LANG_COUNT) ? lang : LANG_EN; }
NsoLang Lang_Get(void) { return lang_current; }

const char *T(StrId id)
{
    const char *s;
    if (id < 0 || id >= S_COUNT) return "";
    s = nso_strings[lang_current][id];
    if (!s || !s[0]) s = nso_strings[LANG_EN][id];
    return s ? s : "";
}

NsoLang Lang_Parse(const char *text)
{
    const char *p = text;
    while (p && *p) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        while (len && (p[len - 1] == '\r' || p[len - 1] == ' ')) len--;
        if (len >= 9 && strncmp(p, "language=", 9) == 0) {
            const char *v = p + 9;
            size_t vl = len - 9;
            if ((vl == 2 && strncmp(v, "pt", 2) == 0) || (vl == 5 && strncmp(v, "pt-BR", 5) == 0)) return LANG_PT;
            return LANG_EN;
        }
        p = e ? e + 1 : NULL;
    }
    return LANG_EN;
}

NsoLang Lang_Load(const char *path)
{
    char buf[512];
    size_t n;
    FILE *f = path ? fopen(path, "r") : NULL;
    if (!f) return LANG_EN;
    n = fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    buf[n] = '\0';
    return Lang_Parse(buf);
}

bool Lang_Save(const char *path, NsoLang lang)
{
    return Cfg_SaveKey(path, "language", lang == LANG_PT ? "pt-BR" : "en");
}

/* Length of line p (without '\n') with trailing CR / spaces trimmed. */
static size_t cfg_line_len(const char *p, const char **next)
{
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    *next = e ? e + 1 : NULL;
    while (len && (p[len - 1] == '\r' || p[len - 1] == ' ')) len--;
    return len;
}

static bool cfg_line_is(const char *p, size_t len, const char *key, size_t kl)
{
    return len > kl && strncmp(p, key, kl) == 0 && p[kl] == '=';
}

bool Cfg_Get(const char *text, const char *key, char *dst, size_t n)
{
    size_t kl = key ? strlen(key) : 0;
    const char *p = text;
    if (!kl || !dst || !n) return false;
    while (p && *p) {
        const char *next;
        size_t len = cfg_line_len(p, &next);
        if (cfg_line_is(p, len, key, kl)) {
            size_t vl = len - kl - 1;
            if (vl >= n) vl = n - 1;
            memcpy(dst, p + kl + 1, vl);
            dst[vl] = '\0';
            return true;
        }
        p = next;
    }
    return false;
}

bool Cfg_Put(const char *text, const char *key, const char *value, char *out, size_t n)
{
    size_t kl = key ? strlen(key) : 0, used = 0;
    bool done = false;
    const char *p = text ? text : "";
    if (!kl || !out || !n) return false;
    out[0] = '\0';
    while (*p) {
        const char *next;
        size_t len = cfg_line_len(p, &next);
        int w;
        if (cfg_line_is(p, len, key, kl)) {
            if (done) { p = next ? next : ""; continue; }   /* drop duplicates */
            w = snprintf(out + used, n - used, "%s=%s\n", key, value ? value : "");
            done = true;
        } else {
            w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
        }
        if (w < 0 || (size_t)w >= n - used) return false;
        used += (size_t)w;
        p = next ? next : "";
    }
    if (!done) {
        int w = snprintf(out + used, n - used, "%s=%s\n", key, value ? value : "");
        if (w < 0 || (size_t)w >= n - used) return false;
    }
    return true;
}

bool Cfg_Remove(const char *text, const char *key, char *out, size_t n)
{
    size_t kl = key ? strlen(key) : 0, used = 0;
    const char *p = text ? text : "";
    if (!kl || !out || !n) return false;
    out[0] = '\0';
    while (*p) {
        const char *next;
        size_t len = cfg_line_len(p, &next);
        if (!cfg_line_is(p, len, key, kl)) {
            int w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
            if (w < 0 || (size_t)w >= n - used) return false;
            used += (size_t)w;
        }
        p = next ? next : "";
    }
    return true;
}

bool Cfg_RetroScreenLoad(const char *text, const char *id, bool *pixel, bool *scanlines)
{
    char key[64], v[32];
    if (pixel) *pixel = true;
    if (scanlines) *scanlines = true;
    if (!id || !id[0]) return true;
    snprintf(key, sizeof(key), "%s.screen_ver", id);
    if (!Cfg_Get(text, key, v, sizeof(v)) || strcmp(v, NSO_RETRO_SCREEN_VER) != 0)
        return true;
    snprintf(key, sizeof(key), "%s.pixel", id);
    if (pixel && Cfg_Get(text, key, v, sizeof(v))) *pixel = strcmp(v, "0") != 0;
    snprintf(key, sizeof(key), "%s.scanlines", id);
    if (scanlines && Cfg_Get(text, key, v, sizeof(v))) *scanlines = strcmp(v, "0") != 0;
    return false;
}

bool Cfg_RetroScreenStamp(const char *text, const char *id, char *out, size_t n)
{
    char a[4200], b[4200], c[4200], key[64];
    if (!id || !id[0] || !out || !n) return false;
    snprintf(key, sizeof(key), "%s.pixel", id);
    if (!Cfg_Put(text ? text : "", key, "1", a, sizeof(a))) return false;
    snprintf(key, sizeof(key), "%s.scanlines", id);
    if (!Cfg_Put(a, key, "1", b, sizeof(b))) return false;
    snprintf(key, sizeof(key), "%s.screen_ver", id);
    if (!Cfg_Put(b, key, NSO_RETRO_SCREEN_VER, c, sizeof(c))) return false;
    snprintf(key, sizeof(key), "%s.screen", id);
    return Cfg_Remove(c, key, out, n);
}

bool Cfg_SaveKey(const char *path, const char *key, const char *value)
{
    char in[4096], out[4200];
    size_t len = 0;
    FILE *f = path ? fopen(path, "r") : NULL;
    if (f) { len = fread(in, 1, sizeof(in) - 1, f); fclose(f); }
    in[len] = '\0';
    if (!Cfg_Put(in, key, value, out, sizeof(out))) return false;
    f = path ? fopen(path, "w") : NULL;
    if (!f) return false;
    bool ok = fputs(out, f) >= 0;
    return (fclose(f) == 0) && ok;
}

void Msg_Clear(NsoMsg *m) { m->id = -1; m->n = 0; m->arg[0] = '\0'; }

void Msg_Set(NsoMsg *m, int id, int n, const char *arg)
{
    m->id = id;
    m->n = n;
    snprintf(m->arg, sizeof(m->arg), "%s", arg ? arg : "");
}

void Msg_Format(const NsoMsg *m, char *dst, size_t size)
{
    const char *fmt;
    if (!dst || size == 0) return;
    dst[0] = '\0';
    if (!m || m->id < 0 || m->id >= S_COUNT) return;
    fmt = T((StrId)m->id);
    if (strstr(fmt, "%d")) {
        snprintf(dst, size, fmt, m->n);
    } else if (strstr(fmt, "%s")) {
        snprintf(dst, size, fmt, m->arg);
    } else if (m->arg[0]) {
        snprintf(dst, size, "%s: %s", fmt, m->arg);
    } else {
        snprintf(dst, size, "%s", fmt);
    }
}

#endif /* LANG_IMPLEMENTATION_ONCE */
#endif /* LANG_IMPLEMENTATION */
