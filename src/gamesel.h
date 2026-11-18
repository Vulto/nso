/* gamesel.h - "Select games" (system sidebar > Settings > Image scraping):
 * the user's ROMs in repos/roms/<folder>, matched to catalog titles, one check
 * per game, and the queue that downloads the cover + date of each checked game
 * and, when the game has a pipeline below, fetches its port, builds it and
 * installs it where the launcher looks. Pure logic (no raylib), unit-tested in
 * gamesel_test.c; the UI and the processes are in gamesel_ui.h.
 * ROMs are only read: a build gets the ROM's path in $ROM and copies or links
 * what the port needs. */
#ifndef NSO_GAMESEL_H
#define NSO_GAMESEL_H

#include <stdbool.h>
#include <stddef.h>

/* One installable game. Commands run with sh from the NSO directory. */
typedef struct {
    const char *system;   /* PlatformEntry id */
    const char *title;    /* GameEntry.title it installs */
    const char *name;     /* log name: logs/install/<name>.log */
    const char *url;      /* port source */
    const char *dir;      /* checkout, NSO-relative (a non-git copy is used as is) */
    const char *pin;      /* commit our adapter was written against; NULL = upstream HEAD */
    const char *build;    /* prepare + build; $ROM = absolute path of the user's ROM */
    const char *binary;   /* NSO-relative file that means "installed" */
    const char *cfgKey;   /* config/nso.cfg key the launcher reads; NULL = none */
    const char *cfgValue; /* its value; "$ROM" = the ROM's NSO-relative path */
    const char *needs;    /* NSO-relative files the game also needs to launch
                             (assets, the provisioned ROM), ';'-separated; NULL = none */
} GamePipeline;

extern const GamePipeline gs_pipelines[];
extern const int gs_pipeline_count;

typedef enum {
    GS_IDLE = 0,
    GS_QUEUED,         /* waits for a download slot */
    GS_DOWNLOADING,    /* port source: clone / checkout */
    GS_WAIT_BUILD,     /* downloaded, waits for the single build slot */
    GS_BUILDING,
    GS_COVER,          /* no pipeline: waits for its cover + date */
    GS_READY,          /* built and installed: playable */
    GS_NO_INSTALLER,   /* cover + date only */
    GS_FAILED,
    GS_CANCELLED,
} GsState;

typedef struct {
    int game;   /* catalog index in the system, -1 = no catalog title */
    int pipe;   /* gs_pipelines index, -1 = none */
    bool on;    /* checked */
    GsState st;
} GsRow;

/* Folder under repos/roms for a system id ("NES", ...); NULL = none. */
const char *Gs_RomFolder(const char *system);
/* Pipeline for a catalog title of a system, -1 if none. */
int Gs_FindPipeline(const char *system, const char *title);
/* "Name (U) [!].nes" -> "Name (U) [!]" (extension dropped). */
void Gs_RomBase(const char *file, char *dst, size_t n);
/* Lowercase words: tags in () [] dropped, punctuation -> space, "_" = "'",
 * "the" / "and" / "&" dropped, roman ii..ix -> digits. */
void Gs_Norm(const char *s, char *dst, size_t n);
/* How well a ROM base name fits a catalog title: 0 = no match, higher is
 * better (same name > one name starts the other > same letters inside > same
 * first three words). */
int Gs_MatchScore(const char *romBase, const char *title);
/* Catalog index for each ROM (out[r], -1 = none). Best score wins; on a tie a
 * title no earlier ROM took is preferred, then the one sharing more tag words
 * ("Namco", "Tengen"). */
void Gs_MatchAll(const char *const *romBases, int romCount,
                 const char *const *titles, int titleCount, int *out);

/* Pipeline for a ROM matched to a catalog title, -1 if none: only when the
 * names are the same (a looser match never builds a port from that ROM). */
int Gs_RowPipeline(const char *system, const char *romBase, const char *title);
/* Toggle a row's check; false while a run is going on. */
bool Gs_Toggle(GsRow *rows, int n, int r);
int Gs_Checked(const GsRow *rows, int n);
/* Queue every checked row; the rest go back to idle. Returns how many. */
int Gs_Start(GsRow *rows, int n);
/* Row whose download may start now (fewer than maxParallel downloading), -1 = none. */
int Gs_NextFetch(const GsRow *rows, int n, int maxParallel);
/* Row whose build may start now (builds run one at a time), -1 = none. */
int Gs_NextBuild(const GsRow *rows, int n);
void Gs_FetchDone(GsRow *row, bool ok);
void Gs_BuildDone(GsRow *row, bool ok);
/* The cover + date pass ended: rows without a pipeline are done. */
void Gs_CoversDone(GsRow *rows, int n);
/* Every row not finished yet becomes cancelled (the caller stops the processes). */
void Gs_Cancel(GsRow *rows, int n);
bool Gs_Running(const GsRow *rows, int n);
/* Value written to cfgKey: "$ROM" -> romRel, else the literal value. */
const char *Gs_CfgValue(const GamePipeline *p, const char *romRel);
/* Every file in p->needs exists (exists(rel, u) on each NSO-relative path). */
bool Gs_NeedsMet(const GamePipeline *p, bool (*exists)(const char *rel, void *u), void *u);

#endif /* NSO_GAMESEL_H */

#ifdef GAMESEL_IMPLEMENTATION
#ifndef GAMESEL_IMPLEMENTATION_ONCE
#define GAMESEL_IMPLEMENTATION_ONCE

#include <ctype.h>
#include <string.h>

/* One in-process module per system (the ones already in use), plus spawned ports.
 * Adapters stay in embed/<game>; the port trees are never edited. */
const GamePipeline gs_pipelines[] = {
    { "nes", "Super Mario Bros", "smb",
      "https://codeberg.org/dannysp/SMB-Vanilla.git", "repos/nes/smbvanilla", "43777eb",
      "make -C embed/smb",
      "embed/smb/build/libsmbmod.so", "nes.smb_rom", "$ROM", NULL },
    { "snes", "Super Mario World", "smw",
      "https://github.com/snesrev/smw.git", "repos/snes/smw", NULL,
      "{ [ -f repos/snes/smw/smw.sfc ] || cp \"$ROM\" repos/snes/smw/smw.sfc; }"
      " && { [ -f repos/snes/smw/smw_assets.dat ] || make -C repos/snes/smw smw_assets.dat; }"
      " && make -C embed/smw",
      "embed/smw/build/libsmwmod.so", "snes.smw_dir", "repos/snes/smw", NULL },
    { "n64", "Super Mario 64", "sm64",
      "https://github.com/sm64pc/sm64ex.git", "repos/n64/sm64ex", "d7ca2c0",
      "{ [ -e repos/n64/sm64ex/baserom.us.z64 ] || ln -sf \"$ROM\" repos/n64/sm64ex/baserom.us.z64; }"
      " && make -C embed/sm64",
      "embed/sm64/build/libsm64mod.so", "n64.sm64_dir", "saves/n64/sm64ex", NULL },
    /* N64Recomp port, spawned from its tree (assets/ and recompcontrollerdb.txt
     * are read from the working directory). The US 1.1 ROM goes, in .z64 order,
     * where the launcher's APP_FOLDER_PATH (GameEntry.appDir) points: the port
     * looks for <game_id>.z64 there and starts without its ROM picker. */
    { "n64", "Blast Corps", "blastcorps",
      "https://github.com/SlaveOfIDO/blastcorps-recomp.git", "repos/n64/blastcorps-recomp", "b551045",
      "D=repos/n64/blastcorps-recomp S=saves/n64/blastcorps-recomp && mkdir -p $S"
      " && { [ -f $S/bc.n64.us.z64 ] || python3 tools/n64rom.py \"$ROM\" $S/bc.n64.us.z64"
      " 42e4d8cde3c106637a25bbfa62d74cc2e5c1eed1d64de5bbb0b1c4896b185927; }"
      " && { [ -f $D/baserom.us.v11.decompressed.z64 ] || python3 $D/tools/decompress_rom.py"
      " $S/bc.n64.us.z64 $D/baserom.us.v11.decompressed.z64; }"
      /* the recompiled C must exist before the port's CMake can configure */
      " && { { ls $D/RecompiledFuncs/*.c && [ -f $D/rsp/aspMain.cpp ]; } >/dev/null 2>&1"
      " || { { { [ -x $D/build-tools/N64Recomp ] && [ -x $D/build-tools/RSPRecomp ]; }"
      " || { cmake -S $D/lib/N64ModernRuntime/N64Recomp -B $D/build-tools -G Ninja"
      " -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
      " && cmake --build $D/build-tools --target N64Recomp RSPRecomp; }; }"
      " && (cd $D && build-tools/N64Recomp us.v11.toml && build-tools/RSPRecomp aspMain.us.v11.toml); }; }"
      /* RmlUi's robin_hood.h misses <cstdint> on current libstdc++ */
      " && { [ -f $D/build-cmake/build.ninja ] || cmake -S $D -B $D/build-cmake -G Ninja"
      " -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++"
      " '-DCMAKE_CXX_FLAGS=-include cstdint'; }"
      " && { [ -f $D/patches/patches.elf ] || command -v ld.lld >/dev/null"
      " || { echo 'ld.lld (lld) is needed to link the port patches'; false; }; }"
      " && cmake --build $D/build-cmake --target BlastCorpsRecompiled",
      "repos/n64/blastcorps-recomp/build-cmake/BlastCorpsRecompiled", NULL, NULL,
      "repos/n64/blastcorps-recomp/assets/LatoLatin-Regular.ttf;"
      "repos/n64/blastcorps-recomp/recompcontrollerdb.txt;"
      "saves/n64/blastcorps-recomp/bc.n64.us.z64" },
};
const int gs_pipeline_count = (int)(sizeof(gs_pipelines) / sizeof(gs_pipelines[0]));

const char *Gs_RomFolder(const char *system)
{
    if (!system) return NULL;
    if (strcmp(system, "nes") == 0) return "NES";
    if (strcmp(system, "snes") == 0) return "SNES";
    if (strcmp(system, "n64") == 0) return "N64";
    return NULL;
}

int Gs_FindPipeline(const char *system, const char *title)
{
    if (!system || !title) return -1;
    for (int i = 0; i < gs_pipeline_count; i++)
        if (strcmp(gs_pipelines[i].system, system) == 0 && strcmp(gs_pipelines[i].title, title) == 0) return i;
    return -1;
}

int Gs_RowPipeline(const char *system, const char *romBase, const char *title)
{
    if (Gs_MatchScore(romBase, title) < 1000) return -1;
    return Gs_FindPipeline(system, title);
}

void Gs_RomBase(const char *file, char *dst, size_t n)
{
    const char *dot;
    size_t len;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!file) return;
    dot = strrchr(file, '.');
    len = (dot && dot != file && strlen(dot) <= 5 && !strchr(dot, ' ')) ? (size_t)(dot - file) : strlen(file);
    if (len >= n) len = n - 1;
    memcpy(dst, file, len);
    dst[len] = '\0';
}

/* Words of s into dst, keeping (keepTags) or dropping text in () []. */
static void Gs_Words(const char *s, char *dst, size_t n, bool keepTags)
{
    /* No single letters: "Mega Man X" stays X. */
    static const char *const roman[] = { "ii", "iii", "iv", "vi", "vii", "viii", "ix" };
    static const char *const digit[] = { "2", "3", "4", "6", "7", "8", "9" };
    char word[128];
    size_t u = 0, w = 0;
    int depth = 0;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!s) return;
    for (const char *p = s;; p++) {
        unsigned char c = (unsigned char)*p;
        bool alnum = c && isalnum(c) && (depth == 0 || keepTags);
        if (c == '(' || c == '[') depth++;
        else if ((c == ')' || c == ']') && depth > 0) depth--;
        if (alnum) {
            if (w + 1 < sizeof(word)) word[w++] = (char)tolower(c);
            continue;
        }
        if (w > 0) {
            const char *out = word;
            word[w] = '\0';
            for (size_t k = 0; k < sizeof(roman) / sizeof(roman[0]); k++)
                if (strcmp(word, roman[k]) == 0) out = digit[k];
            if (strcmp(out, "the") != 0 && strcmp(out, "and") != 0) {
                size_t len = strlen(out);
                if (u + len + 2 < n) {
                    if (u > 0) dst[u++] = ' ';
                    memcpy(dst + u, out, len);
                    u += len;
                    dst[u] = '\0';
                }
            }
            w = 0;
        }
        if (!c) break;
    }
}

void Gs_Norm(const char *s, char *dst, size_t n)
{
    Gs_Words(s, dst, n, false);
}

static void Gs_Squash(const char *norm, char *dst, size_t n)
{
    size_t u = 0;
    for (const char *p = norm; *p && u + 1 < n; p++) if (*p != ' ') dst[u++] = *p;
    dst[u] = '\0';
}

/* Leading words a and b share. */
static int Gs_CommonWords(const char *a, const char *b, bool *aAll, bool *bAll)
{
    int words = 0;
    while (*a && *b) {
        size_t la = strcspn(a, " "), lb = strcspn(b, " ");
        if (la != lb || strncmp(a, b, la) != 0) break;
        words++;
        a += la;
        b += lb;
        while (*a == ' ') a++;
        while (*b == ' ') b++;
    }
    *aAll = (*a == '\0');
    *bAll = (*b == '\0');
    return words;
}

int Gs_MatchScore(const char *romBase, const char *title)
{
    char a[256], b[256], sa[256], sb[256];
    bool aAll, bAll;
    int common;
    Gs_Norm(romBase, a, sizeof(a));
    Gs_Norm(title, b, sizeof(b));
    if (!a[0] || !b[0]) return 0;
    Gs_Squash(a, sa, sizeof(sa));
    Gs_Squash(b, sb, sizeof(sb));
    if (strcmp(sa, sb) == 0) return 1000;
    common = Gs_CommonWords(a, b, &aAll, &bAll);
    if (common > 0 && (aAll || bAll)) return 500 + common;
    if ((strlen(sa) >= 5 && strstr(sb, sa)) || (strlen(sb) >= 5 && strstr(sa, sb))) return 300;
    if (common >= 3) return 100 + common;
    return 0;
}

/* Tag words of the ROM name also in the title ("namco", "tengen", ...). */
static int Gs_TagScore(const char *romBase, const char *title)
{
    char a[256], b[256];
    int score = 0;
    Gs_Words(romBase, a, sizeof(a), true);
    Gs_Words(title, b, sizeof(b), true);
    for (char *w = b; *w;) {
        size_t len = strcspn(w, " ");
        for (const char *p = a; *p;) {
            size_t lp = strcspn(p, " ");
            if (lp == len && strncmp(p, w, len) == 0) { score++; break; }
            p += lp;
            while (*p == ' ') p++;
        }
        w += len;
        while (*w == ' ') w++;
    }
    return score;
}

void Gs_MatchAll(const char *const *romBases, int romCount,
                 const char *const *titles, int titleCount, int *out)
{
    enum { GS_TAKEN_MAX = 1024 };
    bool taken[GS_TAKEN_MAX] = { false };
    for (int r = 0; r < romCount; r++) {
        int best = -1, bestScore = 0, bestTag = -1;
        bool bestFree = false;
        for (int t = 0; t < titleCount; t++) {
            int s = Gs_MatchScore(romBases[r], titles[t]);
            bool free;
            int tag;
            if (s <= 0 || s < bestScore) continue;
            free = !(t < GS_TAKEN_MAX && taken[t]);
            tag = Gs_TagScore(romBases[r], titles[t]);
            if (s == bestScore && (bestFree && !free)) continue;
            if (s == bestScore && bestFree == free && tag <= bestTag) continue;
            best = t;
            bestScore = s;
            bestFree = free;
            bestTag = tag;
        }
        out[r] = best;
        if (best >= 0 && best < GS_TAKEN_MAX) taken[best] = true;
    }
}

bool Gs_Running(const GsRow *rows, int n)
{
    for (int i = 0; i < n; i++) {
        GsState s = rows[i].st;
        if (s == GS_QUEUED || s == GS_DOWNLOADING || s == GS_WAIT_BUILD || s == GS_BUILDING || s == GS_COVER)
            return true;
    }
    return false;
}

bool Gs_Toggle(GsRow *rows, int n, int r)
{
    if (r < 0 || r >= n || Gs_Running(rows, n)) return false;
    rows[r].on = !rows[r].on;
    return true;
}

int Gs_Checked(const GsRow *rows, int n)
{
    int c = 0;
    for (int i = 0; i < n; i++) if (rows[i].on) c++;
    return c;
}

int Gs_Start(GsRow *rows, int n)
{
    int c = 0;
    if (Gs_Running(rows, n)) return 0;
    for (int i = 0; i < n; i++) {
        if (!rows[i].on) { rows[i].st = GS_IDLE; continue; }
        rows[i].st = rows[i].pipe >= 0 ? GS_QUEUED : GS_COVER;
        c++;
    }
    return c;
}

int Gs_NextFetch(const GsRow *rows, int n, int maxParallel)
{
    int busy = 0;
    for (int i = 0; i < n; i++) if (rows[i].st == GS_DOWNLOADING) busy++;
    if (busy >= maxParallel) return -1;
    for (int i = 0; i < n; i++) if (rows[i].st == GS_QUEUED) return i;
    return -1;
}

int Gs_NextBuild(const GsRow *rows, int n)
{
    for (int i = 0; i < n; i++) if (rows[i].st == GS_BUILDING) return -1;
    for (int i = 0; i < n; i++) if (rows[i].st == GS_WAIT_BUILD) return i;
    return -1;
}

void Gs_FetchDone(GsRow *row, bool ok)
{
    if (row && row->st == GS_DOWNLOADING) row->st = ok ? GS_WAIT_BUILD : GS_FAILED;
}

void Gs_BuildDone(GsRow *row, bool ok)
{
    if (row && row->st == GS_BUILDING) row->st = ok ? GS_READY : GS_FAILED;
}

void Gs_CoversDone(GsRow *rows, int n)
{
    for (int i = 0; i < n; i++) if (rows[i].st == GS_COVER) rows[i].st = GS_NO_INSTALLER;
}

void Gs_Cancel(GsRow *rows, int n)
{
    for (int i = 0; i < n; i++) {
        GsState s = rows[i].st;
        if (s == GS_QUEUED || s == GS_DOWNLOADING || s == GS_WAIT_BUILD || s == GS_BUILDING || s == GS_COVER)
            rows[i].st = GS_CANCELLED;
    }
}

const char *Gs_CfgValue(const GamePipeline *p, const char *romRel)
{
    if (!p || !p->cfgValue) return NULL;
    return strcmp(p->cfgValue, "$ROM") == 0 ? romRel : p->cfgValue;
}

bool Gs_NeedsMet(const GamePipeline *p, bool (*exists)(const char *rel, void *u), void *u)
{
    char rel[512];
    if (!p || !p->needs) return true;
    if (!exists) return false;
    for (const char *s = p->needs; *s;) {
        const char *e = strchr(s, ';');
        size_t len = e ? (size_t)(e - s) : strlen(s);
        if (len > 0) {
            if (len >= sizeof(rel)) return false;
            memcpy(rel, s, len);
            rel[len] = '\0';
            if (!exists(rel, u)) return false;
        }
        s += len + (e ? 1 : 0);
    }
    return true;
}

#endif /* GAMESEL_IMPLEMENTATION_ONCE */
#endif /* GAMESEL_IMPLEMENTATION */
