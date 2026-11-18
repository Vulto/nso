#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
#include "launch.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int fails = 0;
#define EXPECT(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void write_file(const char *path, const char *data)
{
    FILE *f = fopen(path, "wb");
    EXPECT(f != NULL);
    if (!f) return;
    fputs(data ? data : "", f);
    fclose(f);
}

int main(void)
{
    char tmp[] = "/tmp/nso-launch-test-XXXXXX";
    char game[2048], sub[2048], xdg[2048], marker_dir[2048], marker[2048];
    char out[2048];
    char path_a[2048], path_b[2048], path_sub[2048];
    char d1[2048], d2[2048];
    NsoRomImport spec;
    int st;
    char fake_bin[2048], log_path[2048], script[4096];

    if (!mkdtemp(tmp)) { perror("mkdtemp"); return 1; }

    snprintf(game, sizeof(game), "%s/game", tmp);
    snprintf(sub, sizeof(sub), "%s/nested", game);
    snprintf(xdg, sizeof(xdg), "%s/xdg", tmp);
    EXPECT(mkdir(game, 0755) == 0);
    EXPECT(mkdir(sub, 0755) == 0);
    EXPECT(mkdir(xdg, 0755) == 0);
    setenv("XDG_DATA_HOME", xdg, 1);

    spec = (NsoRomImport){
        .ext = ".nes",
        .importArg = "--import-rom",
        .importedRel = "opendana/content/base/pack.json",
    };

    /* No ROM, no marker: not needed. */
    EXPECT(!NsoRomImportDone(&spec));
    EXPECT(!NsoRomImportNeeded(game, &spec, out, sizeof(out)));
    EXPECT(NsoPrepareRomImport(game, "./missing", &spec) == NSO_ROM_IMPORT_SKIP);

    /* A ROM only in a subdirectory is ignored. */
    snprintf(path_sub, sizeof(path_sub), "%s/hidden.nes", sub);
    write_file(path_sub, "nope");
    EXPECT(!NsoFindTopLevelExt(game, ".nes", out, sizeof(out)));
    EXPECT(!NsoRomImportNeeded(game, &spec, out, sizeof(out)));

    /* Two top-level ROMs: pick the lexicographically first; .NES matches. */
    snprintf(path_b, sizeof(path_b), "%s/zeta.nes", game);
    snprintf(path_a, sizeof(path_a), "%s/alpha.NES", game);
    write_file(path_b, "b");
    write_file(path_a, "a");
    EXPECT(NsoFindTopLevelExt(game, ".nes", out, sizeof(out)));
    EXPECT(strcmp(out, path_a) == 0);
    EXPECT(NsoRomImportNeeded(game, &spec, out, sizeof(out)) && strcmp(out, path_a) == 0);

    /* Marker present: skip even if a ROM is there. */
    snprintf(d1, sizeof(d1), "%s/opendana", xdg);
    snprintf(d2, sizeof(d2), "%s/opendana/content", xdg);
    snprintf(marker_dir, sizeof(marker_dir), "%s/opendana/content/base", xdg);
    EXPECT(mkdir(d1, 0755) == 0);
    EXPECT(mkdir(d2, 0755) == 0);
    EXPECT(mkdir(marker_dir, 0755) == 0);
    snprintf(marker, sizeof(marker), "%s/pack.json", marker_dir);
    write_file(marker, "{}");
    EXPECT(NsoRomImportDone(&spec));
    EXPECT(!NsoRomImportNeeded(game, &spec, out, sizeof(out)));
    EXPECT(NsoPrepareRomImport(game, "./missing", &spec) == NSO_ROM_IMPORT_SKIP);

    /* Remove the marker and run a fake importer that records argv and exits 1. */
    EXPECT(unlink(marker) == 0);
    snprintf(fake_bin, sizeof(fake_bin), "%s/fakeimport", game);
    snprintf(log_path, sizeof(log_path), "%s/argv.log", game);
    snprintf(script, sizeof(script),
             "#!/bin/sh\nprintf '%%s\\n' \"$0\" \"$@\" > \"%s\"\nexit 1\n", log_path);
    write_file(fake_bin, script);
    EXPECT(chmod(fake_bin, 0755) == 0);

    st = NsoPrepareRomImport(game, "./fakeimport", &spec);
    EXPECT(st == NSO_ROM_IMPORT_RAN);
    {
        FILE *f = fopen(log_path, "rb");
        char line0[256] = {0}, line1[256] = {0}, line2[2048] = {0};
        EXPECT(f != NULL);
        if (f) {
            if (fgets(line0, sizeof(line0), f)) line0[strcspn(line0, "\n")] = 0;
            if (fgets(line1, sizeof(line1), f)) line1[strcspn(line1, "\n")] = 0;
            if (fgets(line2, sizeof(line2), f)) line2[strcspn(line2, "\n")] = 0;
            fclose(f);
        }
        EXPECT(strcmp(line0, "./fakeimport") == 0);
        EXPECT(strcmp(line1, "--import-rom") == 0);
        EXPECT(strcmp(line2, path_a) == 0);
    }

    EXPECT(NsoPrepareRomImport(game, "./fakeimport", NULL) == NSO_ROM_IMPORT_SKIP);

    {   /* GameEntry.args / .env word lists */
        char buf[64] = "  --boot  --window 640x480 ";
        char *w[4];
        EXPECT(NsoSplitWords(buf, w, 4) == 3);
        EXPECT(strcmp(w[0], "--boot") == 0);
        EXPECT(strcmp(w[1], "--window") == 0);
        EXPECT(strcmp(w[2], "640x480") == 0);
        char one[16] = "A=1 B=2 C=3";
        EXPECT(NsoSplitWords(one, w, 2) == 2);
        EXPECT(strcmp(w[1], "B=2") == 0);
        char none[4] = "   ";
        EXPECT(NsoSplitWords(none, w, 4) == 0);
    }

    {   /* Mednafen: Pixel Perfect and Scanlines are independent Game Screen toggles */
        char buf[1024];
        char *w[24];
        /* Both on (recommended): integer square scale + scanlines 90, shader none. */
        int n = NsoMednafenArgv("snes", true, true, "/r/Super Mario Kart.sfc", 1920, 1080, buf, sizeof(buf), w, 24);
        EXPECT(n == 17);
        EXPECT(strcmp(w[0], "-video.fs") == 0 && strcmp(w[1], "0") == 0);
        EXPECT(strcmp(w[3], "state.%X") == 0);
        EXPECT(strcmp(w[4], "-snes.xscale") == 0 && strcmp(w[6], "-snes.yscale") == 0);
        EXPECT(atof(w[5]) == 4.0 && atof(w[7]) == 4.0);             /* 1080/239 floors to 4 */
        EXPECT(strcmp(w[10], "-snes.shader") == 0 && strcmp(w[11], "none") == 0);
        EXPECT(strcmp(w[12], "-snes.videoip") == 0 && strcmp(w[13], "0") == 0);
        EXPECT(strcmp(w[14], "-snes.scanlines") == 0 && strcmp(w[15], "90") == 0);
        EXPECT(strcmp(w[n - 1], "/r/Super Mario Kart.sfc") == 0);
        /* Pixel Perfect only. */
        n = NsoMednafenArgv("nes", true, false, "a.nes", 1920, 1080, buf, sizeof(buf), w, 24);
        EXPECT(n == 17 && strcmp(w[4], "-nes.xscale") == 0);
        EXPECT(atof(w[5]) == 4.0 && atof(w[7]) == 4.0);             /* integer, square */
        EXPECT(strcmp(w[11], "none") == 0 && strcmp(w[13], "0") == 0);
        EXPECT(strcmp(w[14], "-nes.scanlines") == 0 && strcmp(w[15], "0") == 0);
        /* Scanlines only: non-integer scale, same aspect, videoip 1, scanlines 90. */
        n = NsoMednafenArgv("nes", false, true, "a.nes", 1280, 720, buf, sizeof(buf), w, 24);
        EXPECT(n == 17 && strcmp(w[11], "none") == 0);
        EXPECT(fabs(atof(w[5]) - atof(w[7])) < 0.001);
        EXPECT(fabs(atof(w[7]) - 720.0 / 224.0) < 0.001);
        EXPECT(strcmp(w[13], "1") == 0);
        EXPECT(strcmp(w[14], "-nes.scanlines") == 0 && strcmp(w[15], "90") == 0);
        /* Both off: as large as fits, width-limited, no scanlines. */
        n = NsoMednafenArgv("nes", false, false, "a.nes", 800, 1080, buf, sizeof(buf), w, 24);
        EXPECT(n == 17 && fabs(atof(w[5]) * 256.0 - 800.0) < 0.5);
        EXPECT(fabs(atof(w[5]) - atof(w[7])) < 0.001 && strcmp(w[15], "0") == 0);
        EXPECT(NsoMednafenArgv("nes", false, false, "a.nes", 1920, 1080, buf, 20, w, 24) == 0);
        EXPECT(NsoMednafenArgv("nes", false, false, "a.nes", 1920, 1080, buf, sizeof(buf), w, 5) == 0);
    }

    if (fails) {
        fprintf(stderr, "launch_test: %d failure(s)\n", fails);
        return 1;
    }
    printf("launch_test: ok\n");
    return 0;
}
