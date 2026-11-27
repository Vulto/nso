/* sneslist_test.c - Wiki title list (syslist.h): parse, USA/Japan filter, slugs.
 * Also loads assets/data/snes-titles.txt and gamecube-titles.txt. */
#include <stdio.h>
#include <string.h>
#define SNESLIST_IMPLEMENTATION
#include "sneslist.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    SnesListGame games[SNESLIST_MAX];
    int idx[SNESLIST_MAX];
    char buf[1 << 20];
    FILE *f;
    int n, usa, jp, both;
    char slug[64];

    {
        const char *tiny = "# comment\n"
                           "UJ\tSuper Mario World\tSuper Mario World: Super Mario Bros. 4\n"
                           "U\tFrogger\n"
                           "J\tFire Emblem: Thracia 776\n";
        n = SnesList_Parse(tiny, games, SNESLIST_MAX);
        CHECK(n == 3);
        CHECK(games[0].usa && games[0].jp);
        CHECK(strcmp(games[0].title, "Super Mario World") == 0);
        CHECK(strcmp(games[0].alias, "Super Mario World: Super Mario Bros. 4") == 0);
        CHECK(games[0].boxartUsa[0] == '\0' && games[0].boxartJp[0] == '\0');
        CHECK(games[1].usa && !games[1].jp);
        {
            SnesListGame one[1];
            const char *mapped =
                "UJ\tSuper Mario World\t\tSuper Mario World (USA).png\t"
                "Super Mario World - Super Mario Bros. 4 (Japan).png\n";
            int n1 = SnesList_Parse(mapped, one, 1);
            CHECK(n1 == 1);
            CHECK(strcmp(SnesList_Boxart(&one[0], SNESLIST_USA), "Super Mario World (USA).png") == 0);
            CHECK(strcmp(SnesList_Boxart(&one[0], SNESLIST_JP),
                         "Super Mario World - Super Mario Bros. 4 (Japan).png") == 0);
            CHECK(strcmp(SnesList_Boxart(&one[0], SNESLIST_ALL), "Super Mario World (USA).png") == 0);
        }
        CHECK(games[2].jp && !games[2].usa);
        CHECK(SnesList_Filter(games, n, SNESLIST_ALL, idx, SNESLIST_MAX) == 3);
        CHECK(SnesList_Filter(games, n, SNESLIST_USA, idx, SNESLIST_MAX) == 2);
        CHECK(idx[0] == 0 && idx[1] == 1);
        CHECK(SnesList_Filter(games, n, SNESLIST_JP, idx, SNESLIST_MAX) == 2);
        CHECK(idx[0] == 0 && idx[1] == 2);
        CHECK(SnesList_Parse("X\tBad\n", games, SNESLIST_MAX) == -1);
        SnesList_Slug("Super Mario World", slug, sizeof(slug));
        CHECK(strcmp(slug, "super-mario-world") == 0);
        SnesList_Slug("F-ZERO", slug, sizeof(slug));
        CHECK(strcmp(slug, "f-zero") == 0);
    }

    f = fopen("assets/data/snes-titles.txt", "rb");
    CHECK(f != NULL);
    if (!f) return 1;
    n = (int)fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    CHECK(n > 0);
    buf[n] = '\0';
    n = SnesList_Parse(buf, games, SNESLIST_MAX);
    CHECK(n > 700);
    usa = jp = both = 0;
    int mapped = 0;
    for (int i = 0; i < n; i++) {
        if (games[i].usa) usa++;
        if (games[i].jp) jp++;
        if (games[i].usa && games[i].jp) both++;
        CHECK(games[i].title[0] != '\0');
        if (SnesList_Boxart(&games[i], SNESLIST_ALL)[0]) mapped++;
    }
    CHECK(mapped > 1400);
    CHECK(usa == SnesList_Filter(games, n, SNESLIST_USA, idx, SNESLIST_MAX));
    CHECK(jp == SnesList_Filter(games, n, SNESLIST_JP, idx, SNESLIST_MAX));
    CHECK(n == SnesList_Filter(games, n, SNESLIST_ALL, idx, SNESLIST_MAX));
    CHECK(usa + jp - both == n);
    fprintf(stderr, "sneslist_test: %d titles (USA %d, Japan %d, both %d, mapped %d)\n",
            n, usa, jp, both, mapped);

    f = fopen("assets/data/gamecube-titles.txt", "rb");
    CHECK(f != NULL);
    if (!f) return 1;
    n = (int)fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    CHECK(n > 0);
    buf[n] = '\0';
    n = SnesList_Parse(buf, games, SNESLIST_MAX);
    CHECK(n > 500);
    usa = jp = both = 0;
    mapped = 0;
    int sunshine = 0, melee = 0;
    for (int i = 0; i < n; i++) {
        if (games[i].usa) usa++;
        if (games[i].jp) jp++;
        if (games[i].usa && games[i].jp) both++;
        CHECK(games[i].title[0] != '\0');
        if (SnesList_Boxart(&games[i], SNESLIST_ALL)[0]) mapped++;
        if (strcmp(games[i].title, "Super Mario Sunshine") == 0) {
            CHECK(games[i].usa && games[i].jp);
            CHECK(strcmp(SnesList_Boxart(&games[i], SNESLIST_USA),
                         "Super Mario Sunshine (USA).png") == 0);
            CHECK(strcmp(SnesList_Boxart(&games[i], SNESLIST_JP),
                         "Super Mario Sunshine (Japan).png") == 0);
            sunshine = 1;
        }
        if (strcmp(games[i].title, "Super Smash Bros. Melee") == 0) {
            CHECK(games[i].usa && games[i].jp);
            CHECK(strcmp(SnesList_Boxart(&games[i], SNESLIST_USA),
                         "Super Smash Bros. Melee (USA).png") == 0);
            CHECK(strcmp(SnesList_Boxart(&games[i], SNESLIST_JP),
                         "Dairantou Smash Brothers DX (Japan) (En,Ja).png") == 0);
            melee = 1;
        }
    }
    CHECK(sunshine && melee);
    CHECK(usa == 564);
    CHECK(jp == 280);
    CHECK(mapped > 500);
    CHECK(usa == SnesList_Filter(games, n, SNESLIST_USA, idx, SNESLIST_MAX));
    CHECK(jp == SnesList_Filter(games, n, SNESLIST_JP, idx, SNESLIST_MAX));
    CHECK(n == SnesList_Filter(games, n, SNESLIST_ALL, idx, SNESLIST_MAX));
    CHECK(usa + jp - both == n);
    fprintf(stderr, "sneslist_test: gamecube %d titles (USA %d, Japan %d, both %d, mapped %d)\n",
            n, usa, jp, both, mapped);

    f = fopen("assets/data/megadrive-titles.txt", "rb");
    CHECK(f != NULL);
    if (!f) return 1;
    n = (int)fread(buf, 1, sizeof(buf) - 1, f);
    fclose(f);
    CHECK(n > 0);
    buf[n] = '\0';
    n = SnesList_Parse(buf, games, SNESLIST_MAX);
    CHECK(n > 600);
    usa = jp = both = 0;
    mapped = 0;
    int sonic = 0;
    for (int i = 0; i < n; i++) {
        if (games[i].usa) usa++;
        if (games[i].jp) jp++;
        if (games[i].usa && games[i].jp) both++;
        CHECK(games[i].title[0] != '\0');
        if (SnesList_Boxart(&games[i], SNESLIST_ALL)[0]) mapped++;
        if (strcmp(games[i].title, "Sonic the Hedgehog") == 0) {
            CHECK(games[i].usa && games[i].jp);
            CHECK(strcmp(SnesList_Boxart(&games[i], SNESLIST_USA),
                         "Sonic The Hedgehog (USA).png") == 0);
            sonic = 1;
        }
    }
    CHECK(sonic);
    CHECK(usa == 552);
    CHECK(jp == 372);
    CHECK(mapped > 600);
    CHECK(usa == SnesList_Filter(games, n, SNESLIST_USA, idx, SNESLIST_MAX));
    CHECK(jp == SnesList_Filter(games, n, SNESLIST_JP, idx, SNESLIST_MAX));
    CHECK(n == SnesList_Filter(games, n, SNESLIST_ALL, idx, SNESLIST_MAX));
    CHECK(usa + jp - both == n);
    fprintf(stderr, "sneslist_test: megadrive %d titles (USA %d, Japan %d, both %d, mapped %d)\n",
            n, usa, jp, both, mapped);
    if (fails) {
        fprintf(stderr, "%d failed\n", fails);
        return 1;
    }
    printf("sneslist_test: ok\n");
    return 0;
}
