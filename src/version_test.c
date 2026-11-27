/* version_test.c - title normalizer, per-system tags, prefer, sel file. */
#define VERSION_IMPLEMENTATION
#include "version.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void expect(const char *id, const char *title, const char *rom, const char *sys,
                   const char *group, const char *ver,
                   const char *base, const char *label)
{
    VersionInfo v;
    Version_Parse(title, rom, sys, group, ver, &v);
    if (strcmp(v.base, base) != 0 || strcmp(v.label, label) != 0) {
        fprintf(stderr, "FAIL %s\n  title=%s rom=%s\n  got base=\"%s\" label=\"%s\"\n  want base=\"%s\" label=\"%s\"\n",
                id, title ? title : "", rom ? rom : "", v.base, v.label, base, label);
        fails++;
    }
}

int main(void)
{
    int n = -1;
    CHECK(Version_Tags("snes", &n) && n > 0);
    CHECK(Version_Tags("nes", &n) && n == 0);
    CHECK(Version_Tags("megadrive", &n) && n == 0);
    CHECK(Version_Tags("n64", &n) && n == 0);
    CHECK(Version_Tags("gamecube", &n) && n == 0);
    CHECK(Version_Tags(NULL, &n) && n == 0);

    /* SNES: Jefferson's FastRom / SA-1 examples */
    expect("snes-usa", "Super Mario World (USA)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA");
    expect("snes-japan", "Super Mario World (Japan)", NULL, "snes", NULL, NULL,
           "Super Mario World", "Japan");
    expect("snes-fastrom", "Super Mario World (USA) (FastRom)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom");
    expect("snes-sa1", "Super Mario World (USA) (SA-1)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · SA-1");
    expect("snes-combo-paren", "Super Mario World (USA) (FastRom + SA-1)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom + SA-1");
    expect("snes-combo-split", "Super Mario World (USA) (FastRom) (SA-1)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom + SA-1");
    expect("snes-combo-fastrom-case", "Super Mario World (USA) (FastROM + SA-1)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom + SA-1");
    expect("snes-unbracketed", "Super Mario World (USA) FastRom + SA-1", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom + SA-1");
    expect("snes-rom", "Super Mario World",
           "roms/SNES/Super Mario World (USA) (FastRom).sfc", "snes", NULL, NULL,
           "Super Mario World", "USA · FastRom");
    expect("snes-hack-named", "Super Mario World (USA) (Hack) (Brutal Mario)", NULL, "snes", NULL, NULL,
           "Super Mario World", "Brutal Mario");
    expect("snes-hack-plain", "Super Mario World (USA) (Hack)", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · Hack");
    expect("snes-patched-title", "Final Fight 3 (patched)",
           "Final Fight 3 (USA) (patched).sfc", "snes", NULL, NULL,
           "Final Fight 3", "USA · patched");
    expect("snes-rev-dump", "Super Mario World (USA) (Rev 1) [!]", NULL, "snes", NULL, NULL,
           "Super Mario World", "USA · Rev 1");
    expect("snes-lang", "Donkey Kong Country 2 - Diddy's Kong Quest (USA) (En,Fr)", NULL, "snes", NULL, NULL,
           "Donkey Kong Country 2 - Diddy's Kong Quest", "USA");
    expect("snes-plus-in-name", "Super Mario All-Stars + Super Mario World", NULL, "snes", NULL, NULL,
           "Super Mario All-Stars + Super Mario World", "Original");
    expect("snes-override", "Brutal Mario World", NULL, "snes",
           "Super Mario World", "Brutal Mario",
           "Super Mario World", "Brutal Mario");
    expect("snes-short-u", "Axelay (U)", NULL, "snes", NULL, NULL, "Axelay", "USA");
    expect("snes-bang-file", "Biker Mice From Mars (E) [!].smc", NULL, "snes", NULL, NULL,
           "Biker Mice From Mars", "Europe");

    /* Same key for USA / FastRom / hack of one game. */
    {
        VersionInfo a, b, c;
        Version_Parse("Super Mario World (USA)", NULL, "snes", NULL, NULL, &a);
        Version_Parse("Super Mario World (USA) (FastRom + SA-1)", NULL, "snes", NULL, NULL, &b);
        Version_Parse("Super Mario World (USA) (Hack) (Brutal Mario)", NULL, "snes", NULL, NULL, &c);
        CHECK(strcmp(a.key, b.key) == 0 && strcmp(a.key, c.key) == 0);
        CHECK(strcmp(a.key, "super mario world") == 0);
    }

    /* Disc numbers stay in the base so Disc 1 / Disc 2 do not group. */
    {
        VersionInfo a, b;
        Version_Parse("Resident Evil (USA) (Disc 1)", NULL, "gamecube", NULL, NULL, &a);
        Version_Parse("Resident Evil (USA) (Disc 2)", NULL, "gamecube", NULL, NULL, &b);
        CHECK(strcmp(a.base, "Resident Evil (Disc 1)") == 0 || strstr(a.base, "Disc 1"));
        CHECK(strstr(b.base, "Disc 2"));
        CHECK(strcmp(a.key, b.key) != 0);
    }

    /* FastRom is SNES-table only: unbracketed on Mega Drive stays in the base. */
    expect("md-unbracketed-fastrom", "Sonic the Hedgehog (USA) FastRom", NULL, "megadrive", NULL, NULL,
           "Sonic the Hedgehog (USA) FastRom", "Original");
    /* Bracketed unknown tag still leaves the base and becomes a label extra. */
    expect("md-paren-fastrom", "Sonic the Hedgehog (USA) (FastRom)", NULL, "megadrive", NULL, NULL,
           "Sonic the Hedgehog", "USA · FastRom");
    expect("md-region", "Sonic the Hedgehog (Japan)", NULL, "megadrive", NULL, NULL,
           "Sonic the Hedgehog", "Japan");

    /* Prefer: playable, then USA, then unpatched; saved label wins. */
    {
        VersionInfo v[4];
        bool play[4] = { false, true, true, true };
        int i;
        Version_Parse("Game (Japan)", NULL, "snes", NULL, NULL, &v[0]);
        Version_Parse("Game (USA) (FastRom)", NULL, "snes", NULL, NULL, &v[1]);
        Version_Parse("Game (USA)", NULL, "snes", NULL, NULL, &v[2]);
        Version_Parse("Game (Europe)", NULL, "snes", NULL, NULL, &v[3]);
        i = Version_Prefer(v, play, 4, NULL);
        CHECK(i == 2); /* playable + USA + unpatched */
        play[2] = false;
        i = Version_Prefer(v, play, 4, NULL);
        CHECK(i == 1); /* playable + USA (patched) beats Europe */
        i = Version_Prefer(v, play, 4, "Europe");
        CHECK(i == 3);
    }

    /* Sel file */
    {
        char out[256], lab[64];
        const char *p;
        char out2[256];
        CHECK(Version_SelSet(NULL, "Super Mario World", "USA · FastRom", out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Super Mario World\tUSA · FastRom\n") == 0);
        CHECK(Version_SelSet(out, "Final Fight 3", "USA · patched", out2, sizeof(out2)) == 0);
        memcpy(out, out2, sizeof(out));
        p = Version_SelGet(out, "Super Mario World", NULL);
        Version_SelLabelCopy(p, lab, sizeof(lab));
        CHECK(strcmp(lab, "USA · FastRom") == 0);
        CHECK(Version_SelSet(out, "Super Mario World", "USA", out, sizeof(out)) == 0);
        p = Version_SelGet(out, "Super Mario World", "super mario world");
        Version_SelLabelCopy(p, lab, sizeof(lab));
        CHECK(strcmp(lab, "USA") == 0);
        CHECK(strstr(out, "Final Fight 3\tUSA · patched\n"));
    }

    if (fails) { fprintf(stderr, "%d failed\n", fails); return 1; }
    printf("version_test: ok\n");
    return 0;
}
