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
            "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_Nintendo_64"
            "/master/Named_Boxarts/Super%20Mario%2064%20%28USA%29.png";
        if (Scrape_ImageUrl(url, sizeof(url), "Nintendo - Nintendo 64",
                            "Super Mario 64 (USA).png") != 0
            || strcmp(url, want) != 0) {
            fprintf(stderr, "url %s\n", url);
            fails++;
        }
    }
    {
        char url[1024];
        const char *want =
            "https://raw.githubusercontent.com/libretro-thumbnails/Nintendo_-_GameCube"
            "/master/Named_Boxarts/Super%20Mario%20Sunshine%20%28USA%29.png";
        if (Scrape_ImageUrl(url, sizeof(url), "Nintendo - GameCube",
                            "Super Mario Sunshine (USA).png") != 0
            || strcmp(url, want) != 0) {
            fprintf(stderr, "gc url %s\n", url);
            fails++;
        }
    }
    {
        char url[1024];
        const char *want =
            "https://raw.githubusercontent.com/libretro-thumbnails/Sega_-_Mega_Drive_-_Genesis"
            "/master/Named_Boxarts/Sonic%20The%20Hedgehog%20%28USA%29.png";
        if (Scrape_ImageUrl(url, sizeof(url), "Sega - Mega Drive - Genesis",
                            "Sonic The Hedgehog (USA).png") != 0
            || strcmp(url, want) != 0) {
            fprintf(stderr, "md url %s\n", url);
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

    {   /* Mega Drive: subtitles, ~ aliases, versus/vs, articles, no sequel steal */
        const char *md[] = {
            "James Pond 3 - Operation Starfish (USA).png",
            "James Pond 2 (USA).png",
            "Super Mario World (USA).png",
            "Super Mario World 2 - Yoshi's Island (USA).png",
            "Batman (USA).png",
            "Batman Returns (USA).png",
            "Batman Forever (USA).png",
            "Streets of Rage (USA).png",
            "Streets of Rage 2 (USA).png",
            "Streets of Rage II (Japan, Europe) (En,Ja) (Mega Drive Mini, Genesis Mini).png",
            "Lions vs Tigers and the Playoffs (USA).png",
            "Dinosaur's Tale, A (USA).png",
            "Arch Rivals (USA).png",
            "Arch Rivals - The Arcade Game (USA, Europe).png",
            "Exo Squad (USA) (Beta).png",
            "Exo Squad (USA).png",
            "Itchy _ Scratchy Game, The (USA).png",
        };
        expect_match("James Pond 3", md, 17, "James Pond 3 - Operation Starfish (USA).png");
        expect_match("Super Mario World", md, 17, "Super Mario World (USA).png");
        expect_match("Batman", md, 17, "Batman (USA).png");
        expect_match("Bare Knuckle - Ikari no Tetsuken ~ Streets of Rage", md, 17,
                     "Streets of Rage (USA).png");
        expect_match("Bare Knuckle II ~ Streets of Rage II", md, 17,
                     "Streets of Rage 2 (USA).png");
        expect_match("Lions versus Tigers and the Playoffs", md, 17,
                     "Lions vs Tigers and the Playoffs (USA).png");
        expect_match("A Dinosaur's Tale", md, 17, "Dinosaur's Tale, A (USA).png");
        expect_match("Dinosaur's Tale, A", md, 17, "Dinosaur's Tale, A (USA).png");
        expect_match("Arch Rivals - The Arcade Game", md, 17,
                     "Arch Rivals - The Arcade Game (USA, Europe).png");
        expect_match("Exo Squad", md, 17, "Exo Squad (USA).png");
        expect_match("Itchy & Scratchy Game, The", md, 17,
                     "Itchy _ Scratchy Game, The (USA).png");
    }
    {   /* No plain "(USA)" retail: "(Japan, USA)" must beat "(USA) (Beta)". */
        const char *drm[] = {
            "Dr. Mario (USA) (Beta).png",
            "Dr. Mario (Japan, USA).png",
            "Dr. Mario (Europe).png",
            "Fire 'n Ice (USA).png",
            "Madoola no Tsubasa (Japan) (Sample).png",
            "Madoola no Tsubasa (Japan).png",
            "The Wing of Madoola (USA).png",
            "Metroid (USA) (Virtual Console).png",
            "Metroid (USA).png",
            "Legend of Zelda, The (USA) (Beta).png",
            "Legend of Zelda, The (USA).png",
            "Faxanadu (USA) (Rev 1).png",
            "Faxanadu (USA).png",
        };
        expect_match("Dr. Mario", drm, 13, "Dr. Mario (Japan, USA).png");
        expect_match("Faxanadu", drm, 13, "Faxanadu (USA).png");
        expect_match("Fire 'n Ice", drm, 13, "Fire 'n Ice (USA).png");
        expect_match("Madoola no Tsubasa (Japan)", drm, 13, "Madoola no Tsubasa (Japan).png");
        expect_match("Metroid", drm, 13, "Metroid (USA).png");
        expect_match("The Legend of Zelda", drm, 13, "Legend of Zelda, The (USA).png");
    }

    {   /* GameCube: subtitle, publisher prefix, romanization, acronym, combo disc */
        const char *gc[] = {
            "All-Star Baseball 2003 (USA).png",
            "Baseball 2003, The - Battle Ball Park Sengen Perfect Play Pro Yakyuu (Japan).png",
            "DreamWorks _ Aardman Flushed Away (USA).png",
            "Nick Jr. Dora the Explorer - Journey to the Purple Planet (USA).png",
            "SeaWorld Adventure Parks - Shamu's Deep Sea Adventures (USA).png",
            "TY the Tasmanian Tiger - Night of the Quinkan (USA).png",
            "TY the Tasmanian Tiger 2 - Bush Rescue (USA).png",
            "Generation of Chaos Exceed - Yami no Koujo Roze (Japan).png",
            "Momotarou Dentetsu 12 - Nishi Nihon-hen mo Arimasse! (Japan).png",
            "Gekitou Pro Yakyuu - Mizushima Shinji All Stars vs. Pro Yakyuu (Japan).png",
            "Super Robot Taisen GC (Japan).png",
            "UFC - Throwdown (USA).png",
            "Legend of Zelda, The - The Wind Waker (USA).png",
            "Metroid Prime (USA).png",
            "Hobbit, The - The Prelude to the Lord of the Rings (USA).png",
        };
        int n = 15;
        expect_match("The Baseball 2003", gc, n,
                     "Baseball 2003, The - Battle Ball Park Sengen Perfect Play Pro Yakyuu (Japan).png");
        expect_match("Flushed Away", gc, n, "DreamWorks _ Aardman Flushed Away (USA).png");
        expect_match("Dora the Explorer: Journey to the Purple Planet", gc, n,
                     "Nick Jr. Dora the Explorer - Journey to the Purple Planet (USA).png");
        expect_match("Sea World: Shamu's Deep Sea Adventures", gc, n,
                     "SeaWorld Adventure Parks - Shamu's Deep Sea Adventures (USA).png");
        expect_match("Ty the Tasmanian Tiger 3: Night of the Quinkan", gc, n,
                     "TY the Tasmanian Tiger - Night of the Quinkan (USA).png");
        expect_match("Generation of Chaos Exceed: Yami no Miko Rose", gc, n,
                     "Generation of Chaos Exceed - Yami no Koujo Roze (Japan).png");
        expect_match("Momotaro Dentetsu 12", gc, n,
                     "Momotarou Dentetsu 12 - Nishi Nihon-hen mo Arimasse! (Japan).png");
        expect_match("Gekit\u014d Pro Yaky\u016b", gc, n,
                     "Gekitou Pro Yakyuu - Mizushima Shinji All Stars vs. Pro Yakyuu (Japan).png");
        expect_match("Super Robot Wars GC", gc, n, "Super Robot Taisen GC (Japan).png");
        expect_match("Ultimate Fighting Championship: Throwdown", gc, n, "UFC - Throwdown (USA).png");
        expect_match("The Legend of Zelda: The Wind Waker / Metroid Prime", gc, n,
                     "Legend of Zelda, The - The Wind Waker (USA).png");
        expect_match("The Hobbit", gc, n, "Hobbit, The - The Prelude to the Lord of the Rings (USA).png");
    }

    /* Alias stubs: short text pointing at the real Named_Boxarts file. */
    {
        const char *tmp = "/tmp/nso-scrape-alias-test";
        char dst[128];
        FILE *f = fopen(tmp, "wb");
        if (f) { fputs("Bible Adventures (USA) (Unl).png", f); fclose(f); }
        if (!Scrape_AliasTarget(tmp, dst, sizeof(dst)) ||
            strcmp(dst, "Bible Adventures (USA) (Unl).png") != 0) {
            fprintf(stderr, "alias target failed: %s\n", dst);
            fails++;
        }
        if (Scrape_LooksLikeImage(tmp)) {
            fprintf(stderr, "stub looked like image\n");
            fails++;
        }
        f = fopen(tmp, "wb");
        if (f) { fputs("../evil.png", f); fclose(f); }
        if (Scrape_AliasTarget(tmp, dst, sizeof(dst))) {
            fprintf(stderr, "alias accepted path\n");
            fails++;
        }
        f = fopen(tmp, "wb");
        if (f) { fwrite("\x89PNG\r\n\x1a\n", 1, 8, f); fclose(f); }
        if (!Scrape_LooksLikeImage(tmp)) {
            fprintf(stderr, "png magic missed\n");
            fails++;
        }
        if (Scrape_AliasTarget(tmp, dst, sizeof(dst))) {
            fprintf(stderr, "png accepted as alias\n");
            fails++;
        }
        remove(tmp);
    }

    {
        const char *names[] = {
            "Super Mario Bros. 3 (USA).png",
            "Super Mario Bros. (Japan).png",
            "Super Mario Bros. (USA).png",
            "Dr. Mario 64 (Japan).png",
            "Dr. Mario 64 (USA).png",
        };
        const char *out[8];
        int n = Scrape_CollectMatches("Super Mario Bros", names, 5, out, 8);
        if (n != 2) {
            fprintf(stderr, "collect Super Mario Bros count %d, want 2\n", n);
            fails++;
        } else {
            int sawJp = 0, sawUs = 0, i;
            for (i = 0; i < n; i++) {
                if (strcmp(out[i], "Super Mario Bros. (Japan).png") == 0) sawJp = 1;
                if (strcmp(out[i], "Super Mario Bros. (USA).png") == 0) sawUs = 1;
            }
            if (!sawJp || !sawUs) {
                fprintf(stderr, "collect Super Mario Bros missed a region\n");
                fails++;
            }
        }
        n = Scrape_CollectMatches("Super Mario Bros. 3", names, 5, out, 8);
        if (n != 1 || !out[0] || strcmp(out[0], "Super Mario Bros. 3 (USA).png") != 0) {
            fprintf(stderr, "collect Super Mario Bros. 3 -> %s (n=%d)\n",
                    (n > 0 && out[0]) ? out[0] : "(null)", n);
            fails++;
        }
        n = Scrape_CollectMatches("No Such Game", names, 5, out, 8);
        if (n != 0) {
            fprintf(stderr, "collect missing title n=%d\n", n);
            fails++;
        }
    }

    {
        const char *names[] = {
            "Excitebike (1984-11-30)(Nintendo)(JP-US).png",
            "Excitebike (Japan, USA).png",
            "Excitebike (USA) [h1].png",
            "Excitebike (Japan) (Hack).png",
            "Balloon Fight (USA).png",
        };
        const char *out[8];
        const char *hit;
        int n, i, sawJunk = 0;
        n = Scrape_CollectMatches("Excitebike", names, 5, out, 8);
        if (n != 1 || !out[0] || strcmp(out[0], "Excitebike (Japan, USA).png") != 0) {
            fprintf(stderr, "collect Excitebike n=%d -> %s\n", n,
                    (n > 0 && out[0]) ? out[0] : "(null)");
            fails++;
        }
        for (i = 0; i < n; i++) {
            if (strstr(out[i], "1984") || strstr(out[i], "[h") || strstr(out[i], "(Hack"))
                sawJunk = 1;
        }
        if (sawJunk) {
            fprintf(stderr, "collect Excitebike offered a dated/hack name\n");
            fails++;
        }
        hit = Scrape_MatchLoose("Excitebike", names, 5);
        if (!hit || strcmp(hit, "Excitebike (Japan, USA).png") != 0) {
            fprintf(stderr, "match Excitebike -> %s\n", hit ? hit : "(null)");
            fails++;
        }
    }

    {   /* Short "Battletoads" must not share a cover with
         * "Battletoads & Double Dragon"; only a tag tail may extend a name. */
        const char *both[] = {
            "Battletoads (USA).png",
            "Battletoads & Double Dragon (USA).png",
        };
        const char *libretro[] = {
            "Battletoads (USA).png",
            "Battletoads-Double Dragon (USA).png",
        };
        expect_match("Battletoads", both, 2, "Battletoads (USA).png");
        expect_match("Battletoads & Double Dragon", both, 2,
                     "Battletoads & Double Dragon (USA).png");
        expect_loose("Battletoads", both, 2, "Battletoads (USA).png");
        expect_loose("Battletoads & Double Dragon", both, 2,
                     "Battletoads & Double Dragon (USA).png");
        expect_match("Battletoads", libretro, 2, "Battletoads (USA).png");
        expect_match("Battletoads & Double Dragon", libretro, 2,
                     "Battletoads-Double Dragon (USA).png");
        {
            const char *hyphen[] = { "Battletoads-Double Dragon (USA).png" };
            expect_match("Battletoads", hyphen, 1, NULL);
            expect_match("Battletoads & Double Dragon", hyphen, 1,
                         "Battletoads-Double Dragon (USA).png");
        }
        {
            const char *onlyShort[] = { "Battletoads (USA).png", "battletoads.png" };
            expect_match("Battletoads & Double Dragon", onlyShort, 2, NULL);
            expect_match("Battletoads", onlyShort, 2, "Battletoads (USA).png");
        }
        {
            const char *onlyLong[] = { "Battletoads & Double Dragon (USA).png" };
            expect_match("Battletoads", onlyLong, 1, NULL);
            expect_match("Battletoads & Double Dragon", onlyLong, 1,
                         "Battletoads & Double Dragon (USA).png");
        }
        {
            const char *rev[] = { "Battletoads [Rev 1].png" };
            const char *ver[] = { "Battletoads v1.1.png" };
            const char *disc[] = { "Battletoads Disc 2.png" };
            expect_match("Battletoads", rev, 1, "Battletoads [Rev 1].png");
            expect_match("Battletoads", ver, 1, "Battletoads v1.1.png");
            expect_match("Battletoads", disc, 1, "Battletoads Disc 2.png");
        }
    }

    {
        int w, h;
        unsigned char png[24] = {
            0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A,
            0,0,0,13,'I','H','D','R',
            0,0,2,0, 0,0,2,219
        }; /* 512x731 */
        if (Scrape_NesArtScore(512, 731) != 0) { fprintf(stderr, "exact nes art score\n"); fails++; }
        if (Scrape_NesArtScore(512, 730) <= Scrape_NesArtScore(512, 731)
            || Scrape_NesArtScore(731, 512) <= Scrape_NesArtScore(359, 512)
            || Scrape_NesArtScore(0, 0) <= Scrape_NesArtScore(512, 731)) {
            fprintf(stderr, "nes art score order\n"); fails++;
        }
        if (!Scrape_PngSize(png, sizeof png, &w, &h) || w != 512 || h != 731) {
            fprintf(stderr, "png size %dx%d\n", w, h); fails++;
        }
    }

    return fails ? 1 : 0;
}

