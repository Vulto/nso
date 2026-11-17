#define SCRAPE_IMPLEMENTATION
#include "scrape.h"

#include <stdio.h>
#include <string.h>

static int fails = 0;

static void expect_match(const char *title, const char **names, int n, const char *want)
{
    const char *got = Scrape_Match(title, names, n);
    if ((want == NULL && got != NULL) || (want && (got == NULL || strcmp(got, want) != 0))) {
        fprintf(stderr, "match %s -> %s, want %s\n", title,
                got ? got : "(null)", want ? want : "(null)");
        fails++;
    }
}

static void expect_loose(const char *title, const char **names, int n, const char *want)
{
    const char *got = Scrape_MatchLoose(title, names, n);
    if ((want == NULL && got != NULL) || (want && (got == NULL || strcmp(got, want) != 0))) {
        fprintf(stderr, "loose %s -> %s, want %s\n", title, got ? got : "(null)", want ? want : "(null)");
        fails++;
    }
}

int main(void)
{
    const char *names[] = {
        "Super Mario Bros. 3 (USA).png",
        "Super Mario Bros. (Japan).png",
        "Super Mario Bros. (USA).png",
        "Legend of Zelda, The - Twilight Princess (Japan).png",
        "Legend of Zelda, The - Twilight Princess (USA).png",
        "Dr. Mario 64 (Japan).png",
        "Dr. Mario 64 (USA).png",
    };
    char repo[64];
    char **parsed = NULL;
    int count = 0;
    const char *json =
        "{\"path\": \"Named_Boxarts\"}"
        "{\"path\": \"Named_Boxarts/readme.txt\"}"
        "{\"path\": \"Named_Boxarts/Super Mario Bros. (USA).png\"}"
        "{\"path\":\"Named_Boxarts/Dr. Mario 64 (USA).png\"}";

    expect_match("Super Mario Bros", names, 7, "Super Mario Bros. (USA).png");
    expect_match("Super Mario Bros. 3", names, 7, "Super Mario Bros. 3 (USA).png");
    expect_match("Twilight Princess", names, 7,
                 "Legend of Zelda, The - Twilight Princess (USA).png");
    expect_match("Dr. Mario 64", names, 7, "Dr. Mario 64 (USA).png");

    {
        const char *snes[] = {
            "Super Mario World (Europe).png",
            "Super Mario World (USA).png",
            "Super Metroid (Japan, USA) (En,Ja).png",
            "Super Metroid (USA).png",
            "Legend of Zelda, The - A Link to the Past (Japan).png",
            "Legend of Zelda, The - A Link to the Past (USA).png",
            "Legend of Zelda, The - Twilight Princess (USA).png",
            "Kirby Super Star (USA).png",
        };
        expect_match("The Legend of Zelda: A Link to the Past", snes, 8,
                     "Legend of Zelda, The - A Link to the Past (USA).png");
        expect_match("Super Mario World", snes, 8, "Super Mario World (USA).png");
        expect_match("Super Metroid", snes, 8, "Super Metroid (USA).png");
        expect_match("The Legend of Zelda: Oracle of Ages", snes, 8, NULL);
    }

    {
        const char *nes[] = {
            "Legend of Zelda, The (Japan).png",
            "Legend of Zelda, The (Europe).png",
            "Legend of Zelda, The (USA).png",
        };
        expect_match("The Legend of Zelda", nes, 3,
                     "Legend of Zelda, The (USA).png");
    }

    {
        const char *n64[] = {
            "SpaceStation Silicon Valley (Europe).png",
            "SpaceStation Silicon Valley (USA).png",
        };
        expect_match("Space Station Silicon Valley", n64, 2,
                     "SpaceStation Silicon Valley (USA).png");
    }

    {   /* "(World)" before "(Europe)"; a "(tag)" in the title picks the release */
        const char *nes[] = {
            "Super Mario Bros. (Europe).png",
            "Super Mario Bros. (World).png",
            "Ms. Pac-Man (USA) (Namco).png",
            "Ms. Pac-Man (USA) (Tengen) (Unl).png",
            "Road Fighter (Europe).png",
            "Road Fighter (Japan).png",
            "Excitebike (Japan, USA) (En).png",
            "Excitebike (Japan, USA).png",
            "Excitebike (USA) (GameCube Edition).png",
        };
        expect_match("Super Mario Bros", nes, 9, "Super Mario Bros. (World).png");
        expect_match("Ms. Pac-Man (Namco)", nes, 9, "Ms. Pac-Man (USA) (Namco).png");
        expect_match("Ms. Pac-Man (Tengen)", nes, 9, "Ms. Pac-Man (USA) (Tengen) (Unl).png");
        expect_match("Road Fighter", nes, 9, "Road Fighter (Europe).png");
        expect_match("Road Fighter (Japan)", nes, 9, "Road Fighter (Japan).png");
        expect_match("Excitebike (Japan, USA)", nes, 9, "Excitebike (Japan, USA).png");
        expect_match("Road Fighter (USA)", nes, 9, NULL);
    }

    {   /* the exact database name always finds its own entry (and dates: ".dat") */
        const char *nes[] = {
            "Pac-Mania (USA).png",
            "Pac-Mania (USA) (Unl).png",
            "Gimmick! (Japan).png",
            "Mr. Gimmick (Europe).png",
        };
        const char *dats[] = { "Pac-Mania (USA) (Unl).dat", "Gimmick! (Japan).dat" };
        expect_match("Pac-Mania (USA) (Unl)", nes, 4, "Pac-Mania (USA) (Unl).png");
        expect_match("Pac-Mania (USA) (Unl)", dats, 2, "Pac-Mania (USA) (Unl).dat");
        expect_match("Gimmick! (Japan)", dats, 2, "Gimmick! (Japan).dat");
        expect_match("Gimmick! (Japan)", nes, 4, "Gimmick! (Japan).png");
    }

    {   /* loose: a tag no release carries falls back to the same game's other releases */
        const char *dats[] = {
            "1080 Snowboarding (Europe) (En,Ja,Fr,De).dat",
            "Final Fight 3 (USA).dat",
            "Road Fighter (Europe).dat",
        };
        expect_match("1080 Snowboarding (Japan, USA) (En,Ja)", dats, 3, NULL);
        expect_loose("1080 Snowboarding (Japan, USA) (En,Ja)", dats, 3, "1080 Snowboarding (Europe) (En,Ja,Fr,De).dat");
        expect_loose("Final Fight 3 (patched)", dats, 3, "Final Fight 3 (USA).dat");
        expect_loose("Final Fight 3 (USA)", dats, 3, "Final Fight 3 (USA).dat");
        expect_loose("Road Fighter (Japan)", dats, 3, "Road Fighter (Europe).dat");
        expect_loose("Zelda (patched)", dats, 3, NULL);
        expect_loose("(x)", dats, 3, NULL);
    }

    {   /* ", A" like ", The" */
        const char *n64[] = {
            "Bug's Life, A (Europe).png",
            "Bug's Life, A (USA).png",
            "World Is Not Enough, The (USA).png",
        };
        expect_match("A Bug's Life", n64, 3, "Bug's Life, A (USA).png");
        expect_match("Bug's Life", n64, 3, "Bug's Life, A (USA).png");
        expect_match("The World Is Not Enough", n64, 3, "World Is Not Enough, The (USA).png");
    }

    Scrape_Repo(repo, sizeof(repo), "Nintendo - Nintendo 64");
    if (strcmp(repo, "Nintendo_-_Nintendo_64") != 0) {
        fprintf(stderr, "repo %s\n", repo);
        fails++;
    }
    {
        char url[1024];
        const char *want =
            "https://thumbnails.libretro.com/Nintendo%20-%20Nintendo%2064/"
            "Named_Boxarts/Super%20Mario%2064%20%28USA%29.png";
        if (Scrape_ImageUrl(url, sizeof(url), "Nintendo - Nintendo 64",
                            "Super Mario 64 (USA).png") != 0
            || strcmp(url, want) != 0) {
            fprintf(stderr, "url %s\n", url);
            fails++;
        }
    }

    {
        const char *n64[] = {
            "1080 Snowboarding (Europe) (En,Ja,Fr,De).png",
            "1080 Snowboarding (Japan, USA) (En,Ja).png",
            "1080 Snowboarding (USA) (En,Ja) (LodgeNet).png",
            "Lylat Wars (Australia) (En,Fr,De).png",
            "Star Fox 64 (Japan).png",
            "Star Fox 64 (USA) (Rev 1).png",
            "Star Fox 64 (USA).png",
            "Wave Race 64 - Kawasaki Jet Ski (Europe) (En,De).png",
            "Wave Race 64 - Kawasaki Jet Ski (USA) (Rev 1).png",
            "Wave Race 64 - Kawasaki Jet Ski (USA).png",
        };
        int nn = (int)(sizeof(n64) / sizeof(n64[0]));
        expect_match("1080° Snowboarding (Japan, USA)", n64, nn, "1080 Snowboarding (Japan, USA) (En,Ja).png");
        expect_match("Star Fox 64", n64, nn, "Star Fox 64 (USA).png");
        expect_match("Wave Race 64 - Kawasaki Jet Ski", n64, nn, "Wave Race 64 - Kawasaki Jet Ski (USA).png");
    }

    if (Scrape_ParseIndex(json, &parsed, &count) != 0 || count != 2
        || strcmp(parsed[0], "Super Mario Bros. (USA).png") != 0
        || strcmp(parsed[1], "Dr. Mario 64 (USA).png") != 0) {
        fprintf(stderr, "index count %d\n", count);
        fails++;
    }
    Scrape_FreeNames(parsed, count);

    parsed = NULL;
    count = 0;
    if (Scrape_ParseIndex("{\"truncated\": true}", &parsed, &count) == 0) {
        fprintf(stderr, "truncated index was accepted\n");
        fails++;
    }
    Scrape_FreeNames(parsed, count);

    {
        char dat[1024];
        const char *want =
            "https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/"
            "releaseyear/Nintendo%20-%20Nintendo%2064.dat";
        if (Scrape_DatUrl(dat, sizeof(dat), "releaseyear", "Nintendo - Nintendo 64") != 0
            || strcmp(dat, want) != 0) {
            fprintf(stderr, "dat url %s\n", dat);
            fails++;
        }
        char small[16];
        if (Scrape_DatUrl(small, sizeof(small), "releaseyear", "Nintendo - Nintendo 64") != -1) {
            fprintf(stderr, "small dat url accepted\n");
            fails++;
        }
    }

    {
        const char *text =
            "game (\n"
            "\tcomment \"Legend of Zelda, The - Majora's Mask (Europe)\"\n"
            "\treleaseyear \"2000\"\n"
            "\trom ( crc 11111111 )\n"
            ")\n"
            "game (\n"
            "\tcomment \"Legend of Zelda, The - Majora's Mask (USA)\"\n"
            "\treleaseyear \"2000\"\n"
            "\trom ( crc 3EE3F2D1 )\n"
            ")\n"
            "game (\n"
            "\tcomment \"Dr. Mario\"\n"
            "\treleaseyear \"1990\"\n"
            "\trom ( crc 22222222 )\n"
            ")\n"
            "game (\n"
            "\tcomment \"No Year\"\n"
            "\trom ( crc 33333333 )\n"
            ")\n";
        char **pnames = NULL;
        int *pvals = NULL;
        int pcount = 0;
        if (Scrape_ParseDat(text, "releaseyear", &pnames, &pvals, &pcount) != 0
            || pcount != 3
            || strcmp(pnames[0], "Legend of Zelda, The - Majora's Mask (Europe).dat") != 0
            || strcmp(pnames[1], "Legend of Zelda, The - Majora's Mask (USA).dat") != 0
            || strcmp(pnames[2], "Dr. Mario.dat") != 0
            || pvals[0] != 2000 || pvals[1] != 2000 || pvals[2] != 1990) {
            fprintf(stderr, "dat parse count %d\n", pcount);
            fails++;
        }
        expect_match("The Legend of Zelda: Majora's Mask", (const char **)pnames, pcount,
                     "Legend of Zelda, The - Majora's Mask (USA).dat");
        expect_match("Dr. Mario", (const char **)pnames, pcount, "Dr. Mario.dat");
        Scrape_FreeNames(pnames, pcount);
        free(pvals);
    }

    return fails ? 1 : 0;
}
