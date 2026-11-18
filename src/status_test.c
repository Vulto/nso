/* status_test.c - the port-type classification behind the S overlay. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <string.h>
#include "nso.h"
#define NES_IMPLEMENTATION
#include "systems/nes.h"
#define SNES_IMPLEMENTATION
#include "systems/snes.h"
#define N64_IMPLEMENTATION
#include "systems/n64.h"
#define GAMECUBE_IMPLEMENTATION
#include "systems/gamecube.h"
#define STATUS_IMPLEMENTATION
#include "status.h"

static int fails = 0;

static void expect(const char *what, int got, int want)
{
    if (got != want) {
        fprintf(stderr, "%s: got %d, want %d\n", what, got, want);
        fails++;
    }
}

static const GameEntry *find(const GameEntry *g, int n, const char *title)
{
    for (int i = 0; i < n; i++) if (strcmp(g[i].title, title) == 0) return &g[i];
    return NULL;
}

static void type_of(const GameEntry *g, int n, const char *title, StatusType want)
{
    const GameEntry *e = find(g, n, title);
    if (!e) { fprintf(stderr, "no game \"%s\"\n", title); fails++; return; }
    expect(title, Status_Classify(e), want);
}

static void tally_all(int counts[ST_COUNT], int *total)
{
    const struct { const GameEntry *g; int n; } sys[] = {
        { nes_games, nes_game_count }, { snes_games, snes_game_count },
        { n64_games, n64_game_count }, { gamecube_games, gamecube_game_count },
    };
    *total = 0;
    for (int s = 0; s < 4; s++) {
        for (int i = 0; i < sys[s].n; i++)
            if (!Status_Known(&sys[s].g[i])) {
                fprintf(stderr, "unclassified dir: %s\n", sys[s].g[i].dir);
                fails++;
            }
        Status_Tally(sys[s].g, sys[s].n, counts);
        *total += sys[s].n;
    }
}

int main(void)
{
    int counts[ST_COUNT] = { 0 }, total;
    tally_all(counts, &total);
    expect("sum of categories", counts[0] + counts[1] + counts[2] + counts[3], total);
    expect("emulated (no emulator in the catalog)", counts[ST_EMULATED], 0);
    if (counts[ST_RECOMP] <= 0 || counts[ST_NATIVE] <= 0 || counts[ST_NONE] <= 0) {
        fprintf(stderr, "a category is empty\n");
        fails++;
    }

    type_of(nes_games, nes_game_count, "Super Mario Bros", ST_NATIVE);
    type_of(nes_games, nes_game_count, "Dr. Mario", ST_RECOMP);
    type_of(nes_games, nes_game_count, "Mega Man 3", ST_RECOMP);
    type_of(snes_games, snes_game_count, "Super Mario World", ST_NATIVE);
    type_of(snes_games, snes_game_count, "Star Fox", ST_RECOMP);
    type_of(snes_games, snes_game_count, "Aladdin", ST_NONE);
    type_of(n64_games, n64_game_count, "Super Mario 64", ST_NATIVE);
    type_of(n64_games, n64_game_count, "Zelda Ocarina of Time", ST_NATIVE);
    type_of(n64_games, n64_game_count, "Blast Corps", ST_RECOMP);
    type_of(n64_games, n64_game_count, "AeroGauge", ST_RECOMP);
    type_of(n64_games, n64_game_count, "Banjo Kazooie", ST_RECOMP);
    type_of(n64_games, n64_game_count, "Killer Instinct Gold", ST_NONE);
    type_of(gamecube_games, gamecube_game_count, "Metroid Prime", ST_NATIVE);

    GameEntry unknown = { .title = "x", .dir = "repos/n64/mystery", .binary = "b" };
    expect("unknown dir is flagged", Status_Known(&unknown), 0);
    expect("unknown dir defaults to native", Status_Classify(&unknown), ST_NATIVE);
    GameEntry nobin = { .title = "x", .dir = "repos/n64/x-recomp" };
    expect("no binary = not available", Status_Classify(&nobin), ST_NONE);

    expect("percent 1/3", Status_Percent(1, 3), 33);
    expect("percent 2/3", Status_Percent(2, 3), 67);
    expect("percent of nothing", Status_Percent(5, 0), 0);
    expect("percent all", Status_Percent(7, 7), 100);

    if (fails) return 1;
    printf("status_test: ok\n");
    return 0;
}
