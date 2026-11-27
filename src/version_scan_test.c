/* version_scan_test.c - ROM root rewrite, folder layout, IPS patching, and
 * a read-only check that catalog ROMs under roms/ stay locatable. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>

#define VERSION_IMPLEMENTATION
#include "version.h"
#include "nso.h"
#define NES_IMPLEMENTATION
#include "systems/nes.h"
#define SNES_IMPLEMENTATION
#include "systems/snes.h"
#define MEGADRIVE_IMPLEMENTATION
#include "systems/megadrive.h"

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int exists_abs(const char *path)
{
    return path && path[0] && access(path, F_OK) == 0;
}

static int write_bytes(const char *path, const uint8_t *data, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (n && fwrite(data, 1, n, f) != n) { fclose(f); return -1; }
    return fclose(f) == 0 ? 0 : -1;
}

static const char *catalog_sys_folder(const char *id)
{
    if (id && strcmp(id, "snes") == 0) return "SNES";
    if (id && strcmp(id, "nes") == 0) return "NES";
    if (id && strcmp(id, "megadrive") == 0) return "MegaDrive";
    if (id && strcmp(id, "n64") == 0) return "N64";
    if (id && strcmp(id, "gamecube") == 0) return "GameCube";
    return NULL;
}

static int catalog_playable(const char *root, const char *id, const GameEntry *g, int n,
                            int *listed, int *present)
{
    const char *folder = catalog_sys_folder(id);
    VersionScanItem *items = NULL;
    int nItems = 0, miss = 0;
    *listed = *present = 0;
    if (folder) {
        char abs[2112];
        snprintf(abs, sizeof(abs), "%s/%s/%s", root, VERSION_ROM_ROOT, folder);
        items = (VersionScanItem *)calloc(8192, sizeof(*items));
        if (items && exists_abs(abs))
            nItems = Version_ScanDir(abs, folder, items, 8192);
    }
    for (int i = 0; i < n; i++) {
        char rel[1200], path[2400], base[256];
        int found = 0;
        if (!g[i].rom || !g[i].rom[0]) continue;
        (*listed)++;
        if (strncmp(g[i].rom, "repos/roms/", 11) == 0) {
            fprintf(stderr, "%s: stale ROM path %s\n", id, g[i].rom);
            fails++;
            miss++;
            continue;
        }
        Version_RewriteRomPrefix(g[i].rom, rel, sizeof(rel));
        snprintf(path, sizeof(path), "%s/%s", root, rel);
        if (exists_abs(path)) {
            (*present)++;
            continue;
        }
        /* Moved into roms/<System>/<Game>/: same basename still counts. */
        Version_FileName(rel, base, sizeof(base));
        for (int k = 0; k < nItems; k++) {
            if (items[k].isPatch) continue;
            if (strcasecmp(items[k].name, base) == 0) { found = 1; break; }
        }
        if (found) (*present)++;
    }
    free(items);
    return miss;
}

static int real_tree_matches(const char *root, const char *sysFolder,
                             const GameEntry *g, int n)
{
    char abs[2112];
    VersionScanItem *items;
    int nItems, missing = 0, scannedPresent = 0;
    snprintf(abs, sizeof(abs), "%s/%s/%s", root, VERSION_ROM_ROOT, sysFolder);
    if (!exists_abs(abs)) return 0;
    items = (VersionScanItem *)calloc(8192, sizeof(*items));
    if (!items) { fails++; return 1; }
    nItems = Version_ScanDir(abs, sysFolder, items, 8192);
    for (int i = 0; i < n; i++) {
        char rel[1200], path[2400], base[256];
        int found = 0;
        if (!g[i].rom || !g[i].rom[0]) continue;
        Version_RewriteRomPrefix(g[i].rom, rel, sizeof(rel));
        snprintf(path, sizeof(path), "%s/%s", root, rel);
        if (!exists_abs(path)) {
            /* May have been moved into a per-game folder: match basename. */
            Version_FileName(rel, base, sizeof(base));
            for (int k = 0; k < nItems; k++) {
                if (items[k].isPatch) continue;
                if (strcmp(items[k].name, base) == 0) { found = 1; break; }
            }
            if (found) continue;   /* moved into a per-game folder, still scanned */
            /* Not on disk at all (e.g. Super Bomberman 5) — not a failure. */
            continue;
        }
        scannedPresent++;
        for (int k = 0; k < nItems; k++) {
            if (items[k].isPatch) continue;
            if (strcmp(items[k].rel, rel) == 0) { found = 1; break; }
        }
        if (!found) {
            fprintf(stderr, "scan missed existing ROM %s\n", rel);
            missing++;
            fails++;
        }
    }
    free(items);
    (void)scannedPresent;
    return missing;
}

int main(void)
{
    {   /* path rewrite and helpers */
        char d[256];
        Version_RewriteRomPrefix("repos/roms/SNES/Foo (USA).sfc", d, sizeof(d));
        CHECK(strcmp(d, "roms/SNES/Foo (USA).sfc") == 0);
        Version_RewriteRomPrefix("roms/NES/Bar.nes", d, sizeof(d));
        CHECK(strcmp(d, "roms/NES/Bar.nes") == 0);
        CHECK(Version_IsRomExt("Foo.sfc") && Version_IsRomExt("x.NES"));
        CHECK(Version_IsPatchExt("h.ips") && Version_IsPatchExt("h.BPS"));
        CHECK(!Version_IsRomExt("notes.txt") && !Version_IsPatchExt("Foo.sfc"));
        Version_PrettifyFolder("FinalFight3", d, sizeof(d));
        CHECK(strcmp(d, "Final Fight 3") == 0);
        Version_PrettifyFolder("final-fight-3", d, sizeof(d));
        CHECK(strcmp(d, "final fight 3") == 0);
        Version_FileStem("roms/SNES/Foo (USA).sfc", d, sizeof(d));
        CHECK(strcmp(d, "Foo (USA)") == 0);
        CHECK(strcmp(VERSION_ROM_ROOT, "roms") == 0);
    }

    {   /* folder layout fixture — never touches the user's ROM tree */
        char dir[256], game[300], loose[300], a[340], b[340], pth[340];
        uint8_t romA[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
        uint8_t romB[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,99 };
        VersionScanItem items[16];
        int n, folders = 0, looseN = 0, patches = 0, roms = 0;
        snprintf(dir, sizeof(dir), "/tmp/nso-verscan-%d", (int)getpid());
        snprintf(game, sizeof(game), "%s/FinalFight3", dir);
        snprintf(loose, sizeof(loose), "%s/Loose (USA).sfc", dir);
        if (mkdir(dir, 0755) != 0 || mkdir(game, 0755) != 0) {
            fprintf(stderr, "cannot create fixture %s: %s\n", dir, strerror(errno));
            fails++;
        } else {
            snprintf(a, sizeof(a), "%s/Final Fight 3 (USA).sfc", game);
            snprintf(b, sizeof(b), "%s/Final Fight 3 (USA) (FastRom).sfc", game);
            snprintf(pth, sizeof(pth), "%s/Final Fight 3 (USA) (patched).ips", game);
            CHECK(write_bytes(a, romA, sizeof(romA)) == 0);
            CHECK(write_bytes(b, romB, sizeof(romB)) == 0);
            CHECK(write_bytes(pth, (const uint8_t *)"PATCH", 5) == 0);
            CHECK(write_bytes(loose, romA, sizeof(romA)) == 0);
            n = Version_ScanDir(dir, "SNES", items, 16);
            CHECK(n == 4);
            for (int i = 0; i < n; i++) {
                if (items[i].folder[0]) folders++;
                else looseN++;
                if (items[i].isPatch) patches++;
                else roms++;
                CHECK(strncmp(items[i].rel, "roms/SNES/", 10) == 0);
            }
            CHECK(folders == 3 && looseN == 1 && patches == 1 && roms == 3);
            {   /* labels come from filename tags */
                VersionInfo v;
                Version_Parse(NULL, "roms/SNES/FinalFight3/Final Fight 3 (USA) (FastRom).sfc",
                              "snes", NULL, NULL, &v);
                CHECK(strstr(v.label, "USA") && strstr(v.label, "FastRom"));
            }
            {   /* IPS is applied to a copy, never the source */
                uint8_t ips[5 + 3 + 2 + 1 + 3];
                char src[340], dst[340], ipsPath[340];
                uint8_t out[16];
                FILE *f;
                /* PATCH + offset 15, size 1, value 0xAA + EOF */
                memcpy(ips, "PATCH", 5);
                ips[5] = 0; ips[6] = 0; ips[7] = 15;
                ips[8] = 0; ips[9] = 1;
                ips[10] = 0xAA;
                ips[11] = 'E'; ips[12] = 'O'; ips[13] = 'F';
                snprintf(src, sizeof(src), "%s/src.sfc", dir);
                snprintf(ipsPath, sizeof(ipsPath), "%s/p.ips", dir);
                snprintf(dst, sizeof(dst), "%s/out.sfc", dir);
                CHECK(write_bytes(src, romA, sizeof(romA)) == 0);
                CHECK(write_bytes(ipsPath, ips, 14) == 0);
                CHECK(Version_ApplyIps(src, ipsPath, dst) == 0);
                f = fopen(src, "rb");
                CHECK(f && fread(out, 1, 16, f) == 16 && out[15] == 15);
                if (f) fclose(f);
                f = fopen(dst, "rb");
                CHECK(f && fread(out, 1, 16, f) == 16 && out[15] == 0xAA);
                if (f) fclose(f);
                {   /* PreparePatchedRom: sidecar next to a launch copy */
                    char destDir[340], launch[340];
                    snprintf(destDir, sizeof(destDir), "%s/patch-out", dir);
                    CHECK(Version_PreparePatchedRom(src, ipsPath, destDir, launch, sizeof(launch)) == 0);
                    CHECK(exists_abs(launch));
                    {   char side[512], stem[64];
                        Version_FileStem(launch, stem, sizeof(stem));
                        snprintf(side, sizeof(side), "%s/%s.ips", destDir, stem);
                        CHECK(exists_abs(side));
                    }
                    /* original untouched */
                    f = fopen(src, "rb");
                    CHECK(f && fread(out, 1, 16, f) == 16 && out[15] == 15);
                    if (f) fclose(f);
                }
            }
            /* Remove only files we created. */
            unlink(a); unlink(b); unlink(pth); unlink(loose);
            { char p[360]; snprintf(p,sizeof p,"%s/src.sfc",dir); unlink(p);
              snprintf(p,sizeof p,"%s/p.ips",dir); unlink(p);
              snprintf(p,sizeof p,"%s/out.sfc",dir); unlink(p);
              snprintf(p,sizeof p,"%s/patch-out/launch.sfc",dir); unlink(p);
              snprintf(p,sizeof p,"%s/patch-out/launch.ips",dir); unlink(p);
              snprintf(p,sizeof p,"%s/patch-out",dir); rmdir(p); }
            rmdir(game); rmdir(dir);
        }
    }

    {   /* Real tree, read-only: every catalog ROM that exists is under roms/. */
        const char *root = getenv("NSO_BASE_DIR");
        char snesDir[2112];
        int listed = 0, present = 0, snesPresent = 0;
        if (!root || !root[0]) root = "/home/vulto/mnt/nso";
        snprintf(snesDir, sizeof(snesDir), "%s/%s/SNES", root, VERSION_ROM_ROOT);
        if (!exists_abs(snesDir)) {
            printf("version_scan_test: no %s (skipping live catalog check)\n", snesDir);
        } else {
            CHECK(catalog_playable(root, "snes", snes_games, snes_game_count, &listed, &present) == 0);
            CHECK(present >= 1 && listed >= present);
            snesPresent = present;
            CHECK(real_tree_matches(root, "SNES", snes_games, snes_game_count) == 0);
            {
                char ff[2112];
                VersionScanItem *it = (VersionScanItem *)calloc(8192, sizeof(*it));
                int nff, fcount = 0, roms = 0;
                CHECK(it != NULL);
                snprintf(ff, sizeof(ff), "%s/%s/SNES", root, VERSION_ROM_ROOT);
                nff = it ? Version_ScanDir(ff, "SNES", it, 8192) : 0;
                for (int i = 0; it && i < nff; i++) {
                    if (strcmp(it[i].folder, "FinalFight3") != 0) continue;
                    fcount++;
                    if (!it[i].isPatch) roms++;
                    CHECK(strstr(it[i].rel, "roms/SNES/FinalFight3/") == it[i].rel);
                }
                snprintf(ff, sizeof(ff), "%s/%s/SNES/FinalFight3", root, VERSION_ROM_ROOT);
                if (exists_abs(ff))
                    CHECK(fcount >= 2 && roms >= 2);
                free(it);
            }
            CHECK(catalog_playable(root, "nes", nes_games, nes_game_count, &listed, &present) == 0);
            CHECK(real_tree_matches(root, "NES", nes_games, nes_game_count) == 0);
            CHECK(catalog_playable(root, "megadrive", megadrive_games, megadrive_game_count, &listed, &present) == 0);
            CHECK(real_tree_matches(root, "MegaDrive", megadrive_games, megadrive_game_count) == 0);
            printf("version_scan_test: live roms at %s (SNES present=%d)\n", snesDir, snesPresent);
        }
    }

    if (fails) { fprintf(stderr, "version_scan_test: %d failed\n", fails); return 1; }
    printf("version_scan_test: ok\n");
    return 0;
}
