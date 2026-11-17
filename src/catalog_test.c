/* catalog_test.c - the catalogs: a title is the key of order / played / dates
 * files, so no system may list one twice, nor two games on the same box art. */
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

static int fails = 0;

static void check(const char *id, const GameEntry *g, int n)
{
    for (int i = 0; i < n; i++) {
        if (!g[i].title || !g[i].title[0]) {
            fprintf(stderr, "%s #%d: no title\n", id, i);
            fails++;
            continue;
        }
        for (int j = 0; j < i; j++) {
            if (g[j].title && strcmp(g[i].title, g[j].title) == 0) {
                fprintf(stderr, "%s: duplicate title \"%s\"\n", id, g[i].title);
                fails++;
            }
            if (g[i].boxartPath && g[j].boxartPath && strcmp(g[i].boxartPath, g[j].boxartPath) == 0) {
                fprintf(stderr, "%s: \"%s\" and \"%s\" share %s\n", id, g[j].title, g[i].title, g[i].boxartPath);
                fails++;
            }
        }
    }
}

int main(void)
{
    check("nes", nes_games, nes_game_count);
    check("snes", snes_games, snes_game_count);
    check("n64", n64_games, n64_game_count);
    check("gamecube", gamecube_games, gamecube_game_count);
    if (fails) return 1;
    printf("catalog_test: ok\n");
    return 0;
}
