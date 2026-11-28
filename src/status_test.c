/* status_test.c - the port-type classification behind the S overlay.
 * The overlay tallies only the active system; each catalog is checked the
 * same way, plus a few known titles. */
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
#define MEGADRIVE_IMPLEMENTATION
#include "systems/megadrive.h"
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

static void check_sys(const char *name, const GameEntry *g, int n,
                      int want[ST_COUNT], int out[ST_COUNT])
{
    int counts[ST_COUNT] = { 0 };
    char label[64];
    for (int i = 0; i < n; i++)
        if (!Status_Known(&g[i])) {
            fprintf(stderr, "%s: unclassified dir: %s\n", name, g[i].dir);
            fails++;
        }
    Status_Tally(g, n, counts);
    expect(name, counts[0] + counts[1] + counts[2] + counts[3], n);
    snprintf(label, sizeof(label), "%s recomp", name);   expect(label, counts[ST_RECOMP], want[ST_RECOMP]);
    snprintf(label, sizeof(label), "%s native", name);   expect(label, counts[ST_NATIVE], want[ST_NATIVE]);
    snprintf(label, sizeof(label), "%s emulated", name); expect(label, counts[ST_EMULATED], want[ST_EMULATED]);
    snprintf(label, sizeof(label), "%s none", name);     expect(label, counts[ST_NONE], want[ST_NONE]);
    printf("%s: %d games — recomp %d (%d%%), native %d (%d%%), emulated %d (%d%%), none %d (%d%%)\n",
           name, n,
           counts[ST_RECOMP], Status_Percent(counts[ST_RECOMP], n),
           counts[ST_NATIVE], Status_Percent(counts[ST_NATIVE], n),
           counts[ST_EMULATED], Status_Percent(counts[ST_EMULATED], n),
           counts[ST_NONE], Status_Percent(counts[ST_NONE], n));
    for (int t = 0; t < ST_COUNT; t++) out[t] += counts[t];
}

int main(void)
{
    int counts[ST_COUNT] = { 0 }, total;
    check_sys("NES", nes_games, nes_game_count, (int[ST_COUNT]){ 5, 3, 50, 1728 }, counts);
    check_sys("SNES", snes_games, snes_game_count, (int[ST_COUNT]){ 8, 3, 50, 0 }, counts);
    check_sys("Nintendo 64", n64_games, n64_game_count, (int[ST_COUNT]){ 41, 10, 0, 190 }, counts);
    check_sys("GameCube", gamecube_games, gamecube_game_count, (int[ST_COUNT]){ 0, 4, 0, 0 }, counts);
    check_sys("Mega Drive", megadrive_games, megadrive_game_count, (int[ST_COUNT]){ 0, 0, 447, 0 }, counts);
    total = nes_game_count + snes_game_count + n64_game_count + gamecube_game_count + megadrive_game_count;
    expect("sum of categories", counts[0] + counts[1] + counts[2] + counts[3], total);
    if (counts[ST_RECOMP] <= 0 || counts[ST_NATIVE] <= 0 || counts[ST_EMULATED] <= 0 || counts[ST_NONE] <= 0) {
        fprintf(stderr, "a category is empty\n");
        fails++;
    }

    type_of(nes_games, nes_game_count, "Super Mario Bros", ST_NATIVE);
    type_of(nes_games, nes_game_count, "Dr. Mario", ST_RECOMP);
    type_of(nes_games, nes_game_count, "Mega Man 3", ST_RECOMP);
    type_of(snes_games, snes_game_count, "Super Mario World", ST_NATIVE);
    type_of(snes_games, snes_game_count, "Star Fox", ST_RECOMP);
    type_of(snes_games, snes_game_count, "Aladdin", ST_EMULATED);
    type_of(snes_games, snes_game_count, "Super Mario Kart", ST_EMULATED);
    type_of(nes_games, nes_game_count, "Contra", ST_EMULATED);
    type_of(nes_games, nes_game_count, "Top Gun", ST_EMULATED);
    type_of(n64_games, n64_game_count, "Super Mario 64", ST_NATIVE);
    type_of(n64_games, n64_game_count, "Zelda Ocarina of Time", ST_NATIVE);
    type_of(n64_games, n64_game_count, "Blast Corps", ST_RECOMP);
    type_of(n64_games, n64_game_count, "AeroGauge", ST_RECOMP);
    type_of(n64_games, n64_game_count, "Banjo Kazooie", ST_NATIVE);
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
