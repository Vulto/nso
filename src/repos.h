/* repos.h - fixed list of source ports NSO can clone/update/build, plus the
 * pure state logic (no raylib, unit-tested in repos_test.c).
 * The work itself runs in tools/repo_sync.sh as a background child. */
#ifndef NSO_REPOS_H
#define NSO_REPOS_H

#include <stdbool.h>
#include "lang.h"

typedef struct {
    const char *system;  /* PlatformEntry id: "snes", "n64", ... */
    const char *name;    /* label + log name */
    const char *url;
    const char *dir;     /* NSO-relative checkout dir (= GameEntry.dir) */
    const char *binary;  /* relative to dir (= GameEntry.binary) */
    const char *rom;     /* file the user must put in dir, or NULL */
    const char *deps;    /* comma list for repo_sync.sh ("pkg:x" = pkg-config) */
    const char *build;   /* shell command run inside dir */
} RepoEntry;

typedef enum {
    REPO_NOT_CLONED,
    REPO_LOCAL_COPY,     /* dir exists without .git: left alone */
    REPO_NOT_BUILT,
    REPO_ROM_NEEDED,
    REPO_READY,
} RepoLocalState;

extern const RepoEntry nso_repos[];
extern const int nso_repo_count;

/* State from what is on disk (no network). */
RepoLocalState Repo_LocalState(bool dirExists, bool isGit, bool binaryExists,
                               bool romNeeded, bool romExists);
StrId Repo_LocalStr(RepoLocalState s);
/* UI string for a finished repo_sync.sh exit code. */
StrId Repo_ExitStr(int code);
/* repo_sync.sh status key ("up_to_date", ...) -> UI string; -1 if unknown. */
int Repo_StatusKey(const char *key);
/* True when the exit code means the game can be launched now. */
bool Repo_ExitReady(int code);

#endif /* NSO_REPOS_H */

#ifdef REPOS_IMPLEMENTATION
#ifndef REPOS_IMPLEMENTATION_ONCE
#define REPOS_IMPLEMENTATION_ONCE

#include <string.h>

#define SNESREV_DEPS "git,make,cc,python3,sdl2-config,pkg:sdl2"
#define NES_MESON_DEPS "git,meson,cc,c++,pkg:sdl3"
#define NES_CMAKE_SDL3 "git,cmake,cc,c++,pkg:sdl3"
#define NES_CMAKE_SDL2 "git,cmake,cc,bash,pkg:sdl2"
#define N64_MAKE_DEPS "git,make,cc,python3,hexdump,pkg:sdl2,pkg:glew"
#define N64_HM_DEPS "git,cmake,ninja,cc,c++,python3,pkg:sdl2"
#define N64_PD_DEPS "git,cmake,cc,c++,python3,pkg:sdl2"
#define GCN_CMAKE_DEPS "git,cmake,ninja,cc,c++,python3"
#define GCN_MELEE_DEPS "git,cmake,ninja,gcc,g++"
#define GCN_PAPER_DEPS "git,cmake,ninja,clang,lld,python3,pkg-config"

/* Source ports only (decomps / source ports), never recompilations or
 * binary releases. New NES/N64/GameCube checkouts use *-src / unique dirs so
 * existing AppImage and hand-made installs are never touched. */
const RepoEntry nso_repos[] = {
    { "snes", "smw",    "https://github.com/snesrev/smw.git",    "repos/snes/smw",
      "./smw",    "smw.sfc",    SNESREV_DEPS, "make -j\"$(nproc)\"" },
    { "snes", "zelda3", "https://github.com/snesrev/zelda3.git", "repos/snes/zelda3",
      "./zelda3", "zelda3.sfc", SNESREV_DEPS, "make -j\"$(nproc)\"" },
    { "snes", "sm",     "https://github.com/snesrev/sm.git",     "repos/snes/sm",
      "./sm",     "sm.smc",     SNESREV_DEPS, "make -j\"$(nproc)\"" },

    { "nes", "SMB Vanilla", "https://codeberg.org/dannysp/SMB-Vanilla.git",
      "repos/nes/smbvanilla", "build/src/platform/smbvanilla", "smb.nes",
      NES_MESON_DEPS,
      "if [ -f build/meson-info/meson-info.json ]; then meson setup --reconfigure build; else meson setup build; fi && meson compile -C build" },
    { "nes", "OpenMadoola", "https://github.com/nfroggy/openmadoola.git",
      "repos/nes/openmadoola", "build/openmadoola", "madoola.nes",
      NES_CMAKE_SDL3,
      "cmake -B build -DACTIVE_PLATFORM=SDL3 && cmake --build build -j\"$(nproc)\"" },
    { "nes", "OpenDana", "https://github.com/dvilelaf/opendana.git",
      "repos/nes/opendana", "build/opendana", NULL,
      NES_CMAKE_SDL2, "bash build.sh" },

    { "n64", "sm64ex", "https://github.com/sm64pc/sm64ex.git",
      "repos/n64/sm64ex", "build/us_pc/sm64.us.f3dex2e", "baserom.us.z64",
      N64_MAKE_DEPS, "make -j\"$(nproc)\"" },
    { "n64", "Ship of Harkinian", "https://github.com/HarbourMasters/Shipwright.git",
      "repos/n64/soh-src", "build-cmake/soh/soh.elf", NULL,
      N64_HM_DEPS,
      "cmake -H. -Bbuild-cmake -GNinja -DCMAKE_BUILD_TYPE:STRING=Release && cmake --build build-cmake --target GenerateSohOtr && cmake --build build-cmake" },
    { "n64", "2 Ship 2 Harkinian", "https://github.com/2ship2harkinian/2ship2harkinian.git",
      "repos/n64/2s2h-src", "build-cmake/mm/2s2h.elf", NULL,
      N64_HM_DEPS,
      "cmake -H. -Bbuild-cmake -GNinja -DCMAKE_BUILD_TYPE:STRING=Release && cmake --build build-cmake --target Generate2ShipOtr && cmake --build build-cmake" },
    { "n64", "Perfect Dark", "https://github.com/perfect-dark-pc-port/perfect_dark.git",
      "repos/n64/perfect_dark-src", "build/pd.x86_64", "data/pd.ntsc-final.z64",
      N64_PD_DEPS,
      "cmake -G\"Unix Makefiles\" -Bbuild . && cmake --build build -j\"$(nproc)\" && mkdir -p build/data && command /bin/cp -f data/pd.ntsc-final.z64 build/data/" },
    { "n64", "SpaghettiKart", "https://github.com/HarbourMasters/SpaghettiKart.git",
      "repos/n64/spaghettikart", "build-cmake/Spaghettify", "mk64.z64",
      N64_HM_DEPS,
      "cmake -H. -Bbuild-cmake -GNinja -DCMAKE_BUILD_TYPE:STRING=Release && cmake --build build-cmake --target ExtractAssets && cmake --build build-cmake" },

    { "gamecube", "Dusklight", "https://github.com/TwilitRealm/dusklight.git",
      "repos/gamecube/dusklight", "build/linux-default-relwithdebinfo/dusklight", NULL,
      GCN_CMAKE_DEPS,
      "cmake --preset linux-default-relwithdebinfo && cmake --build --preset linux-default-relwithdebinfo -- -j\"$(nproc)\"" },
    { "gamecube", "melee-pc", "https://github.com/999sian/melee-pc.git",
      "repos/gamecube/melee-pc", "build/melee", NULL,
      GCN_MELEE_DEPS, "cmake -B build -G Ninja && ninja -C build" },
    { "gamecube", "Metroid Prime", "https://github.com/Odrannnn/MetroidPrimePort.git",
      "repos/gamecube/metroidprime", "build/native/metroid_prime_port", NULL,
      GCN_CMAKE_DEPS,
      "cmake -S . -B build/native -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DAURORA_ENABLE_TESTS=OFF -DMP_BUILD_TESTS=OFF && cmake --build build/native -j\"$(nproc)\"" },
    { "gamecube", "Paper Cut", "https://github.com/LLCOOLT/paper-cut.git",
      "repos/gamecube/papercut", "build-pc-us-aurora/ttyd-us", NULL,
      GCN_PAPER_DEPS,
      "cmake -S . -B build-pc-us-aurora -G Ninja --toolchain cmake/toolchains/linux-clang.cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DTTYD_VERSION=G8ME01 -DTTYD_BACKEND=aurora && ninja -C build-pc-us-aurora" },
};
const int nso_repo_count = (int)(sizeof(nso_repos) / sizeof(nso_repos[0]));

RepoLocalState Repo_LocalState(bool dirExists, bool isGit, bool binaryExists,
                               bool romNeeded, bool romExists)
{
    if (!dirExists) return REPO_NOT_CLONED;
    if (!isGit) return REPO_LOCAL_COPY;
    if (romNeeded && !romExists) return REPO_ROM_NEEDED;
    if (!binaryExists) return REPO_NOT_BUILT;
    return REPO_READY;
}

StrId Repo_LocalStr(RepoLocalState s)
{
    switch (s) {
        case REPO_NOT_CLONED: return S_REPO_NOT_CLONED;
        case REPO_LOCAL_COPY: return S_REPO_LOCAL_COPY;
        case REPO_NOT_BUILT:  return S_REPO_NOT_BUILT;
        case REPO_ROM_NEEDED: return S_REPO_ROM_NEEDED;
        case REPO_READY:      return S_REPO_READY;
    }
    return S_REPO_FAILED;
}

StrId Repo_ExitStr(int code)
{
    switch (code) {
        case 0:   return S_REPO_READY;
        case 10:  return S_REPO_UP_TO_DATE;
        case 11:  return S_REPO_LOCAL_COPY;
        case 20:  return S_REPO_ROM_NEEDED;
        case 21:  return S_REPO_MISSING_DEPS;
        case 1:   return S_REPO_DOWNLOAD_FAILED;
        case 2:   return S_REPO_BUILD_FAILED;
        case 127: return S_REPO_NO_SH;
    }
    return S_REPO_FAILED;
}

int Repo_StatusKey(const char *key)
{
    static const struct { const char *key; StrId id; } map[] = {
        { "missing_deps", S_REPO_MISSING_DEPS }, { "cloning", S_REPO_CLONING },
        { "clone_failed", S_REPO_CLONE_FAILED }, { "local_ready", S_REPO_LOCAL_READY },
        { "local_unbuilt", S_REPO_LOCAL_UNBUILT }, { "checking", S_REPO_CHECKING },
        { "fetch_failed", S_REPO_FETCH_FAILED }, { "bad_checkout", S_REPO_BAD_CHECKOUT },
        { "no_upstream", S_REPO_NO_UPSTREAM }, { "updating", S_REPO_UPDATING },
        { "update_failed", S_REPO_UPDATE_FAILED }, { "submodule_failed", S_REPO_SUBMODULE_FAILED },
        { "rom_needed", S_REPO_ROM_NEEDED }, { "up_to_date", S_REPO_UP_TO_DATE },
        { "building", S_REPO_BUILDING }, { "build_failed", S_REPO_BUILD_FAILED },
        { "build_no_binary", S_REPO_BUILD_NO_BINARY }, { "ready", S_REPO_READY },
    };
    if (!key) return -1;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
        if (strcmp(map[i].key, key) == 0) return (int)map[i].id;
    }
    return -1;
}

bool Repo_ExitReady(int code)
{
    return code == 0 || code == 10;
}

#endif /* REPOS_IMPLEMENTATION_ONCE */
#endif /* REPOS_IMPLEMENTATION */
