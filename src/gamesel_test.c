/* gamesel_test.c - "Select games": ROM name -> catalog title, the check list
 * and the install queue (downloads in parallel, builds one at a time). */
#include <stdio.h>
#include <string.h>
#define GAMESEL_IMPLEMENTATION
#include "gamesel.h"

static bool NeedsAll(const char *rel, void *u) { (void)rel; (void)u; return true; }
static bool NeedsNoRom(const char *rel, void *u) { (void)u; return strstr(rel, ".z64") == NULL; }
static bool NeedsCount(const char *rel, void *u) { (void)rel; (*(int *)u)++; return true; }

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int Match(const char *rom, const char *const *titles, int n)
{
    int out = -2;
    Gs_MatchAll(&rom, 1, titles, n, &out);
    return out;
}

int main(void)
{
    char b[256];

    Gs_RomBase("Super Mario Bros (U).nes", b, sizeof(b));
    CHECK(strcmp(b, "Super Mario Bros (U)") == 0);
    Gs_RomBase("Mega Man 3 (U) [!].nes", b, sizeof(b));
    CHECK(strcmp(b, "Mega Man 3 (U) [!]") == 0);
    Gs_RomBase("Gun.Smoke", b, sizeof(b));
    CHECK(strcmp(b, "Gun") == 0 || strcmp(b, "Gun.Smoke") == 0);
    Gs_Norm("Castlevania II: Simon's Quest", b, sizeof(b));
    CHECK(strcmp(b, "castlevania 2 simon s quest") == 0);
    Gs_Norm("Legend of Kage, The (U)", b, sizeof(b));
    CHECK(strcmp(b, "legend of kage") == 0);
    Gs_Norm("Mega Man X", b, sizeof(b));
    CHECK(strcmp(b, "mega man x") == 0);

    {
        static const char *const t[] = { "Mega Man", "Mega Man 2", "Galaga: Demons of Death",
            "Garry Kitchen's Battletank", "Battletoads & Double Dragon", "Battletoads",
            "Ms. Pac-Man (Namco)", "Ms. Pac-Man (USA) (Tengen) (Unl)", "DuckTales 2",
            "The Legend of Kage", "Super Mario Bros", "A Bug's Life" };
        int n = (int)(sizeof(t) / sizeof(t[0]));
        CHECK(Match("Mega Man 2 (U)", t, n) == 1);
        CHECK(Match("Mega Man (U)", t, n) == 0);
        CHECK(Match("Galaga (U)", t, n) == 2);
        CHECK(Match("Battle Tank (U)", t, n) == 3);
        CHECK(Match("Battletoads Double Dragon (U)", t, n) == 4);
        CHECK(Match("Battletoads (U)", t, n) == 5);
        CHECK(Match("Duck Tales 2 (U)", t, n) == 8);
        CHECK(Match("Legend of Kage, The (U)", t, n) == 9);
        CHECK(Match("Super Mario Bros (U)", t, n) == 10);
        CHECK(Match("A Bug_s Life (U) [!]", t, n) == 11);
        CHECK(Match("Contra (U)", t, n) == -1);
        {   /* two ROMs, two titles: the second takes the one left */
            const char *roms[2] = { "Ms Pac-Man (Namco) (U)", "Ms Pac-Man (U)" };
            int out[2];
            Gs_MatchAll(roms, 2, t, n, out);
            CHECK(out[0] == 6 && out[1] == 7);
        }
    }

    CHECK(strcmp(Gs_RomFolder("nes"), "NES") == 0);
    CHECK(strcmp(Gs_RomFolder("n64"), "N64") == 0);
    CHECK(Gs_RomFolder("gamecube") == NULL);
    {   /* every system has a pipeline, each findable, launcher paths set */
        const char *sys[] = { "nes", "snes", "n64" };
        for (int s = 0; s < 3; s++) {
            int found = 0;
            for (int i = 0; i < gs_pipeline_count; i++) {
                const GamePipeline *p = &gs_pipelines[i];
                CHECK(p->url && p->dir && p->build && p->binary && p->name);
                if (strcmp(p->system, sys[s]) == 0) {
                    found++;
                    CHECK(Gs_FindPipeline(sys[s], p->title) == i);
                }
            }
            CHECK(found >= 1);
        }
        CHECK(Gs_FindPipeline("nes", "Contra") == -1);
        /* a looser match (another game of the series) never gets the installer */
        CHECK(Gs_RowPipeline("snes", "Super Mario World", "Super Mario World") >= 0);
        CHECK(Gs_RowPipeline("snes", "Super Mario World 2 - Yoshi's Island", "Super Mario World") == -1);
        CHECK(Gs_RowPipeline("n64", "Super Mario 64 (USA)", "Super Mario 64") >= 0);
        int smb = Gs_FindPipeline("nes", "Super Mario Bros");
        CHECK(smb >= 0 && strcmp(Gs_CfgValue(&gs_pipelines[smb], "repos/roms/NES/x.nes"), "repos/roms/NES/x.nes") == 0);
        int smw = Gs_FindPipeline("snes", "Super Mario World");
        CHECK(smw >= 0 && strcmp(Gs_CfgValue(&gs_pipelines[smw], "x"), "repos/snes/smw") == 0);
        /* Blast Corps: the dumped ROM name installs it; playable needs its assets + stored ROM */
        int bc = Gs_RowPipeline("n64", "Blast Corps (U) (V1.1) [!]", "Blast Corps");
        CHECK(bc >= 0 && gs_pipelines[bc].needs != NULL);
        CHECK(Gs_NeedsMet(&gs_pipelines[bc], NeedsAll, NULL));
        CHECK(!Gs_NeedsMet(&gs_pipelines[bc], NeedsNoRom, NULL));
        CHECK(Gs_NeedsMet(&gs_pipelines[smb], NULL, NULL));   /* no needs: binary alone */
        GamePipeline two = { .needs = "a;;b" };
        int seen = 0;
        CHECK(Gs_NeedsMet(&two, NeedsCount, &seen) && seen == 2);
    }

    {   /* queue: 3 pipeline games + 1 cover-only; 2 download slots, 1 build slot */
        GsRow r[5] = { { 0, 0, false, GS_IDLE }, { 1, 1, false, GS_IDLE }, { 2, -1, false, GS_IDLE },
                       { 3, 2, false, GS_IDLE }, { 4, -1, false, GS_IDLE } };
        CHECK(Gs_Toggle(r, 5, 0) && Gs_Toggle(r, 5, 1) && Gs_Toggle(r, 5, 2) && Gs_Toggle(r, 5, 3));
        CHECK(Gs_Toggle(r, 5, 4) && Gs_Toggle(r, 5, 4));   /* on, then off again */
        CHECK(Gs_Checked(r, 5) == 4);
        CHECK(Gs_Start(r, 5) == 4);
        CHECK(Gs_Running(r, 5));
        CHECK(!Gs_Toggle(r, 5, 4));                         /* locked while running */
        CHECK(r[2].st == GS_COVER && r[4].st == GS_IDLE);
        CHECK(Gs_NextBuild(r, 5) == -1);
        int f = Gs_NextFetch(r, 5, 2); CHECK(f == 0); r[f].st = GS_DOWNLOADING;
        f = Gs_NextFetch(r, 5, 2); CHECK(f == 1); r[f].st = GS_DOWNLOADING;
        CHECK(Gs_NextFetch(r, 5, 2) == -1);                 /* both slots busy */
        Gs_FetchDone(&r[1], true);
        CHECK(r[1].st == GS_WAIT_BUILD);
        f = Gs_NextFetch(r, 5, 2); CHECK(f == 3); r[f].st = GS_DOWNLOADING;
        int bld = Gs_NextBuild(r, 5); CHECK(bld == 1); r[bld].st = GS_BUILDING;
        Gs_FetchDone(&r[0], true);
        CHECK(Gs_NextBuild(r, 5) == -1);                    /* one build at a time */
        Gs_BuildDone(&r[1], true);
        CHECK(r[1].st == GS_READY);
        bld = Gs_NextBuild(r, 5); CHECK(bld == 0); r[bld].st = GS_BUILDING;
        Gs_FetchDone(&r[3], false);
        CHECK(r[3].st == GS_FAILED);
        Gs_BuildDone(&r[0], false);
        CHECK(r[0].st == GS_FAILED);
        CHECK(Gs_Running(r, 5));                            /* the cover pass still goes */
        Gs_CoversDone(r, 5);
        CHECK(r[2].st == GS_NO_INSTALLER);
        CHECK(!Gs_Running(r, 5));
        CHECK(Gs_Toggle(r, 5, 4));
    }

    {   /* cancel: unfinished rows stop, finished ones keep their state */
        GsRow r[3] = { { 0, 0, true, GS_IDLE }, { 1, 1, true, GS_IDLE }, { 2, -1, true, GS_IDLE } };
        Gs_Start(r, 3);
        r[0].st = GS_DOWNLOADING;
        Gs_FetchDone(&r[0], true);
        r[0].st = GS_BUILDING;
        Gs_BuildDone(&r[0], true);
        r[1].st = GS_DOWNLOADING;
        Gs_Cancel(r, 3);
        CHECK(r[0].st == GS_READY && r[1].st == GS_CANCELLED && r[2].st == GS_CANCELLED);
        Gs_FetchDone(&r[1], true);                          /* the killed download exits later */
        CHECK(r[1].st == GS_CANCELLED);
        CHECK(!Gs_Running(r, 3));
    }

    if (fails) return 1;
    printf("gamesel_test: ok\n");
    return 0;
}
