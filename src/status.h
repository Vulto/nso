/* status.h - how each catalog game runs: recompilation, native port built from
 * full source / decompilation, emulated, or not available yet. Pure C (nso.h
 * types only): unit-tested in status_test.c, drawn by statuspanel.h (S key).
 * The type comes from the catalog entry: no dir/binary = not available; a
 * "recomp" directory = recompilation; any other directory must be listed in
 * status_dirs below (an unknown one is reported by Status_Known so the test
 * fails instead of guessing). */
#ifndef NSO_STATUS_H
#define NSO_STATUS_H

#include <stdbool.h>
#include "nso.h"

typedef enum { ST_RECOMP = 0, ST_NATIVE, ST_EMULATED, ST_NONE, ST_COUNT } StatusType;

/* Type of a catalog entry (ST_NONE when it has no dir or binary). */
StatusType Status_Classify(const GameEntry *g);
/* False when the entry has a dir that is neither "recomp" nor in the table. */
bool Status_Known(const GameEntry *g);
/* counts[type] += 1 for every game of games[0..n). */
void Status_Tally(const GameEntry *games, int n, int counts[ST_COUNT]);
/* Rounded share in percent (0 when total <= 0). */
int Status_Percent(int count, int total);

#endif /* NSO_STATUS_H */

#ifdef STATUS_IMPLEMENTATION
#ifndef STATUS_IMPLEMENTATION_ONCE
#define STATUS_IMPLEMENTATION_ONCE

#include <string.h>

typedef struct { const char *dir; StatusType type; } StatusDir;

/* Directories that do not say what they are. Native = port built from full
 * source or a decompilation; recomp = static recompilation (N64Recomp and
 * friends). */
static const StatusDir status_dirs[] = {
    { "embed/smb", ST_NATIVE },
    { "repos/nes/openmadoola", ST_NATIVE },
    { "repos/nes/opendana", ST_NATIVE },
    { "embed/mm3", ST_RECOMP },
    { "embed/smw", ST_NATIVE },
    { "repos/snes/zelda3", ST_NATIVE },
    { "repos/snes/sm", ST_NATIVE },
    { "embed/starfox", ST_RECOMP },
    { "embed/sm64", ST_NATIVE },
    { "repos/n64/soh-src", ST_NATIVE },
    { "repos/n64/2s2h-src/build-cmake/mm", ST_NATIVE },
    { "repos/n64/perfect_dark-src", ST_NATIVE },
    { "repos/n64/BattleShip/build-us", ST_NATIVE },
    { "embed/starship", ST_NATIVE },
    { "repos/n64/starship", ST_NATIVE },
    { "repos/n64/starship-src/build-cmake", ST_NATIVE },
    { "repos/n64/mk64", ST_NATIVE },
    { "repos/n64/spaghettikart/build-cmake", ST_NATIVE },
    { "repos/n64/paperboat/build/ninja-debug", ST_NATIVE },
    { "repos/n64/ProjectMidas-v1.0.2b-linux", ST_NATIVE },
    { "embed/bk", ST_RECOMP },
    { "repos/n64/bk", ST_RECOMP },
    { "repos/n64/Lighthouse/build-cmake", ST_NATIVE },
    { "repos/gamecube/dusklight/build/linux-default-relwithdebinfo", ST_NATIVE },
    { "repos/gamecube/melee-pc", ST_NATIVE },
    { "repos/gamecube/metroidprime", ST_NATIVE },
    { "repos/gamecube/papercut", ST_NATIVE },
    { "mednafen/src", ST_EMULATED },
};

static const StatusDir *Status_Lookup(const char *dir)
{
    for (size_t i = 0; i < sizeof(status_dirs) / sizeof(status_dirs[0]); i++)
        if (strcmp(status_dirs[i].dir, dir) == 0) return &status_dirs[i];
    return NULL;
}

StatusType Status_Classify(const GameEntry *g)
{
    if (!g || !g->dir || !g->binary) return ST_NONE;
    const StatusDir *d = Status_Lookup(g->dir);
    if (d) return d->type;
    if (strstr(g->dir, "recomp")) return ST_RECOMP;
    return ST_NATIVE;
}

bool Status_Known(const GameEntry *g)
{
    if (!g || !g->dir || !g->binary) return true;
    return Status_Lookup(g->dir) || strstr(g->dir, "recomp");
}

void Status_Tally(const GameEntry *games, int n, int counts[ST_COUNT])
{
    for (int i = 0; i < n; i++) counts[Status_Classify(&games[i])]++;
}

int Status_Percent(int count, int total)
{
    if (total <= 0 || count <= 0) return 0;
    return (int)(((long)count * 100 + total / 2) / total);
}

#endif /* STATUS_IMPLEMENTATION_ONCE */
#endif /* STATUS_IMPLEMENTATION */
