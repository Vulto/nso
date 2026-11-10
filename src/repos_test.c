/* repos_test.c - state logic + tools/repo_sync.sh against a local bare repo. */
#define _DEFAULT_SOURCE
#define LANG_IMPLEMENTATION
#define REPOS_IMPLEMENTATION
#include "repos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int run(const char *cmd)
{
    int st = system(cmd);
    return (st >= 0 && (st & 0x7f) == 0) ? (st >> 8) & 0xff : -1;
}

int main(void)
{
    /* Local state decisions. */
    CHECK(Repo_LocalState(false, false, false, true, false) == REPO_NOT_CLONED);
    CHECK(Repo_LocalState(true, false, true, true, true) == REPO_LOCAL_COPY);
    CHECK(Repo_LocalState(true, true, false, true, false) == REPO_ROM_NEEDED);
    CHECK(Repo_LocalState(true, true, true, true, false) == REPO_ROM_NEEDED);
    CHECK(Repo_LocalState(true, true, false, true, true) == REPO_NOT_BUILT);
    CHECK(Repo_LocalState(true, true, false, false, false) == REPO_NOT_BUILT);
    CHECK(Repo_LocalState(true, true, true, false, false) == REPO_READY);
    CHECK(Repo_ExitReady(0) && Repo_ExitReady(10) && !Repo_ExitReady(20) && !Repo_ExitReady(2));
    CHECK(Repo_ExitStr(21) == S_REPO_MISSING_DEPS && Repo_ExitStr(10) == S_REPO_UP_TO_DATE);
    CHECK(Repo_ExitStr(99) == S_REPO_FAILED && Repo_StatusKey("nope") == -1);

    /* Every status key the script writes has a translation id. */
    {
        FILE *f = fopen("tools/repo_sync.sh", "r");
        char line[512];
        int keys = 0;
        CHECK(f != NULL);
        while (f && fgets(line, sizeof(line), f)) {
            for (char *p = strstr(line, "say "); p; p = strstr(p + 4, "say ")) {
                char key[64];
                if (p > line && p[-1] != ' ' && p[-1] != '{' && p[-1] != '\t') continue;
                if (sscanf(p + 4, "%63[a-z_]", key) != 1) continue;
                keys++;
                if (Repo_StatusKey(key) < 0) { fprintf(stderr, "unmapped key %s\n", key); fails++; }
            }
        }
        if (f) fclose(f);
        CHECK(keys >= 15);
    }

    /* List sanity: every entry complete, source dirs unique. */
    for (int i = 0; i < nso_repo_count; i++) {
        const RepoEntry *r = &nso_repos[i];
        CHECK(r->system && r->name && r->url && r->dir && r->binary && r->deps && r->build);
        CHECK(strcmp(r->system, "nes") == 0 || strcmp(r->system, "snes") == 0
              || strcmp(r->system, "n64") == 0 || strcmp(r->system, "gamecube") == 0);
        CHECK(strncmp(r->url, "https://", 8) == 0);
        for (int j = 0; j < i; j++) CHECK(strcmp(r->dir, nso_repos[j].dir) != 0);
    }
    CHECK(nso_repo_count >= 15);

    /* Never point a download at an existing AppImage or hand-made install. */
    {
        static const char *keep[] = {
            "repos/nes/smb",
            "repos/n64/sm64", "repos/n64/soh", "repos/n64/2s2h",
            "repos/n64/perfect_dark", "repos/n64/mk64", "repos/n64/starship",
            "repos/n64/bk", "repos/n64/ssb64", "repos/n64/ddr64",
            "repos/gamecube/dusk",
        };
        for (int i = 0; i < nso_repo_count; i++) {
            for (size_t k = 0; k < sizeof(keep) / sizeof(keep[0]); k++) {
                CHECK(strcmp(nso_repos[i].dir, keep[k]) != 0);
            }
        }
    }

    /* repo_sync.sh end to end with a local upstream (no network). */
    char tmp[] = "/tmp/nso-repos-test-XXXXXX";
    if (!mkdtemp(tmp)) { perror("mkdtemp"); return 1; }
    char cmd[4096];
#define SH(...) (snprintf(cmd, sizeof(cmd), __VA_ARGS__), run(cmd))
#define SYNC(rom, deps) SH("sh tools/repo_sync.sh %s/log %s/status %s/work file://%s/up.git ./game %s %s " \
        "'printf \"#!/bin/sh\\n\" > game && chmod +x game'", tmp, tmp, tmp, tmp, rom, deps)
    CHECK(SH("cd %s && git init -q --bare up.git && git clone -q up.git seed 2>/dev/null && cd seed"
             " && git -c user.name=t -c user.email=t@t commit -q --allow-empty -m one && git push -q origin HEAD", tmp) == 0);

    CHECK(SYNC("-", "nonexistent-tool-xyz") == 21);         /* deps checked first */
    CHECK(SH("test ! -e %s/work", tmp) == 0);               /* ...nothing cloned */
    CHECK(SYNC("rom.sfc", "git") == 20);                    /* clone, then ROM needed */
    CHECK(SH("grep -qx 'rom_needed rom.sfc' %s/status", tmp) == 0);
    CHECK(SH("touch %s/work/rom.sfc", tmp) == 0);
    CHECK(SYNC("rom.sfc", "git") == 0);                     /* builds */
    CHECK(SH("test -x %s/work/game", tmp) == 0);
    CHECK(SYNC("rom.sfc", "git") == 10);                    /* up to date: no rebuild */
    CHECK(SH("cd %s/seed && git -c user.name=t -c user.email=t@t commit -q --allow-empty -m two"
             " && git push -q origin HEAD", tmp) == 0);
    CHECK(SYNC("rom.sfc", "git") == 0);                     /* fast-forward + rebuild */
    CHECK(SH("test \"$(git -C %s/work rev-parse HEAD)\" = \"$(git -C %s/seed rev-parse HEAD)\"", tmp, tmp) == 0);
    CHECK(SH("rm -rf %s/work/.git", tmp) == 0);
    CHECK(SYNC("rom.sfc", "git") == 11);                    /* non-git copy left alone */
    CHECK(SH("test -f %s/work/rom.sfc", tmp) == 0);

    /* ROM-optional ("-") builds with no ROM file, and a ROM in a subdir
     * binary folder is copied next to the executable. */
    CHECK(SH("sh tools/repo_sync.sh %s/log %s/status %s/work3 file://%s/up.git ./game - git "
             "'printf \"#!/bin/sh\\n\" > game && chmod +x game'", tmp, tmp, tmp, tmp) == 0);
    CHECK(SH("test -x %s/work3/game", tmp) == 0);
    CHECK(SH("sh tools/repo_sync.sh %s/log %s/status %s/work4 file://%s/up.git out/game rom.sfc git "
             "'mkdir -p out && printf \"#!/bin/sh\\n\" > out/game && chmod +x out/game'",
             tmp, tmp, tmp, tmp) == 20);
    CHECK(SH("touch %s/work4/rom.sfc", tmp) == 0);
    CHECK(SH("sh tools/repo_sync.sh %s/log %s/status %s/work4 file://%s/up.git out/game rom.sfc git "
             "'mkdir -p out && printf \"#!/bin/sh\\n\" > out/game && chmod +x out/game'",
             tmp, tmp, tmp, tmp) == 0);
    CHECK(SH("test -x %s/work4/out/game && test -f %s/work4/out/rom.sfc", tmp, tmp) == 0);

    SH("rm -rf %s", tmp);
    if (fails) return 1;
    puts("repos_test: ok");
    return 0;
}
