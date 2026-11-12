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

/* Install watch: what a disk-state change tells the user. */
typedef enum {
    REPO_NOTE_NONE,
    REPO_NOTE_ROM_NEEDED,    /* downloaded, waits for the user's own ROM */
    REPO_NOTE_INSTALLABLE,   /* everything on disk: one press builds it */
    REPO_NOTE_INSTALLED,     /* binary appeared: launchable */
} RepoNote;

/* Upper bound of nso_repos (main.c sizes its per-repo state with it). */
#define NSO_REPO_MAX 96

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
/* Can the build run now from what is on disk (cloned, ROM present if
 * required, not built)? lastExit = last repo_sync exit for the repo, -1 if
 * none: missing deps (21) or a failed build (2) is not offered again until
 * the disk state changes. */
bool Repo_CanInstall(RepoLocalState s, int lastExit);
/* Exit code a saved status line stands for, so a failed build or missing
 * deps from an earlier run is not offered again after a restart; -1 if the
 * line says nothing against installing. */
int Repo_StatusExit(const char *line);
/* Notice for a disk-state change. The first sighting (known = false, e.g. at
 * startup) never notifies; only transitions do. */
RepoNote Repo_Transition(bool known, RepoLocalState prev, RepoLocalState now);

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

/* Static recompilations (N64Recomp / snesrecomp / nesrecomp): built from the
 * upstream sources against the user's own ROM; nothing prebuilt is fetched. */
#define N64RECOMP_DEPS "git,cmake,ninja,clang,clang++,ld.lld,python3,pkg:sdl2,pkg:gtk+-3.0"
#define N64RECOMP_VK_DEPS N64RECOMP_DEPS ",pkg:vulkan"
/* tools/build_linux.sh repos: their own clang pick, no lld (patches prebuilt). */
#define N64RECOMP_DG_DEPS "git,cmake,ninja,clang,clang++,python3,bash,pkg:sdl2,pkg:gtk+-3.0"
#define N64RECOMP_GE_DEPS N64RECOMP_DEPS ",xdelta3"
#define N64RECOMP_BT_DEPS N64RECOMP_DEPS ",pkg:freetype2,pkg:dbus-1"
#define SNESRECOMP_DEPS "git,cmake,ninja,cc,c++,python3,bash"
#define MSTAN_DEPS "git,cmake,cc,c++,python3,bash,file"
/* N64Recomp + RSPRecomp from the repo's own submodule (or upstream when the
 * pinned runtime has none); the .toml files then read the ROM in the repo. */
#define N64RECOMP_TOOLS \
    "T=lib/N64ModernRuntime/N64Recomp; [ -f $T/CMakeLists.txt ] || T=N64Recomp; " \
    "[ -f $T/CMakeLists.txt ] || { [ -d build-tools-src ] || git clone --recursive https://github.com/N64Recomp/N64Recomp.git build-tools-src; T=build-tools-src; }; " \
    "cmake -S $T -B build-tools -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++" \
    " && cmake --build build-tools --target N64RecompCLI RSPRecomp -j\"$(nproc)\""
#define N64RECOMP_CMAKE(target) \
    "cmake -S . -B build-cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++" \
    " && cmake --build build-cmake --target " target " -j\"$(nproc)\""
/* wootbeer repos: N64Recomp is a target of the game's own CMake project. */
#define WOOT_BUILD(toml, target) \
    "cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++" \
    " && cmake --build build --target N64RecompCLI -j\"$(nproc)\"" \
    " && R=$(find build -type f -name N64Recomp -perm -u+x | head -n 1) && [ -n \"$R\" ] && \"$R\" " toml \
    " && cmake -S . -B build && cmake --build build --target " target " -j\"$(nproc)\"" \
    " && { [ -x build/" target " ] || command /bin/cp -f \"$(find build -type f -name " target " -perm -u+x | head -n 1)\" build/; }"
/* mstan snesrecomp / nesrecomp repos: their own Linux script, no AppImage. */
#define MSTAN_BUILD(flags, target) \
    "bash tools/build-linux.sh " flags \
    " && { [ -x build-linux-prod/" target " ] || command /bin/cp -f \"$(find build-linux-prod -type f -name " target " -perm -u+x | head -n 1)\" build-linux-prod/; }"

/* Source ports (decomps / source ports) and static recompilations, always
 * built from source, never binary releases. New NES/N64/GameCube checkouts use *-src / unique dirs so
 * existing AppImage and hand-made installs are never touched. */
const RepoEntry nso_repos[] = {
    { "snes", "smw",    "https://github.com/snesrev/smw.git",    "repos/snes/smw",
      "./smw",    "smw.sfc",    SNESREV_DEPS, "make -j\"$(nproc)\"" },
    { "snes", "zelda3", "https://github.com/snesrev/zelda3.git", "repos/snes/zelda3",
      "./zelda3", "zelda3.sfc", SNESREV_DEPS, "make -j\"$(nproc)\"" },
    { "snes", "sm",     "https://github.com/snesrev/sm.git",     "repos/snes/sm",
      "./sm",     "sm.smc",     SNESREV_DEPS, "make -j\"$(nproc)\"" },
    /* snes: static recompilations (retroreversing.com list). */
    { "snes", "F-Zero", "https://github.com/craigshaw/FZeroRecomp.git",
      "repos/snes/fzero-recomp", "build/fzero_recomp", "fzero_usa_reference.sfc",
      SNESRECOMP_DEPS,
      "sh tools/apply_snesrecomp_patches.sh && python3 snesrecomp/tools/v2_sync_funcs_h.py --cfg-dir config --out config/funcs.h && python3 snesrecomp/tools/v2_emit.py --rom fzero_usa_reference.sfc --cfg-dir config --out-dir generated --cfg-roots && sh build.sh" },
    { "snes", "Go Go Ackman", "https://github.com/catmidori/gogoackman1recomp.git",
      "repos/snes/ackman-recomp", "build/AckmanRecomp", "gogoackman.jp.sfc",
      SNESRECOMP_DEPS,
      "python3 scripts/regen.py --rom gogoackman.jp.sfc && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel" },
    { "snes", "Kirby Super Star", "https://github.com/NVDEMU/KirbySuperStarPC.git",
      "repos/snes/kss-recomp", "build/KirbySuperStarSNESRecomp", "kss.us.sfc",
      SNESRECOMP_DEPS,
      "bash tools/regen.sh --rom kss.us.sfc && cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j\"$(nproc)\"" },
    { "snes", "Super Mario All-Stars", "https://github.com/TanookiStudios/SuperMarioAllStarsRecomp.git",
      "repos/snes/smas-recomp", "build/SuperMarioAllStarsRecomp", "rom/smas.sfc",
      SNESRECOMP_DEPS,
      "sh ./setup.sh && bash tools/regen.sh && cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build" },
    { "snes", "Super Mario RPG: Legend of the Seven Stars", "https://github.com/mstan/SuperMarioRPGRecomp.git",
      "repos/snes/smrpg-recomp", "build-linux-prod/SuperMarioRPGSNESRecomp", "smrpg.sfc",
      MSTAN_DEPS,
      MSTAN_BUILD("--regen --no-package", "SuperMarioRPGSNESRecomp") },
    { "snes", "Mega Man X2", "https://github.com/mstan/MegaManX2Recomp.git",
      "repos/snes/mmx2-recomp", "build-linux-prod/MegaManX2SNESRecomp", "mmx2.sfc",
      MSTAN_DEPS,
      MSTAN_BUILD("--regen --no-package", "MegaManX2SNESRecomp") },
    { "snes", "Mega Man X3", "https://github.com/mstan/MegamanX3SNESRecomp.git",
      "repos/snes/mmx3-recomp", "build-linux-prod/MegaManX3SNESRecomp", "mmx3.sfc",
      MSTAN_DEPS,
      MSTAN_BUILD("--regen --no-package", "MegaManX3SNESRecomp") },
    { "snes", "Star Fox", "https://github.com/mstan/StarFoxSNESRecomp.git",
      "repos/snes/starfox-recomp", "build-linux-prod/StarFoxSNESRecomp", "starfox.sfc",
      MSTAN_DEPS,
      MSTAN_BUILD("--regen --no-package", "StarFoxSNESRecomp") },

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
    /* nes: static recompilations (retroreversing.com list). */
    { "nes", "Dr. Mario", "https://github.com/mstan/DrMarioNesRecomp.git",
      "repos/nes/drmario-recomp", "build-linux-prod/DrMarioRecomp", NULL,
      MSTAN_DEPS,
      MSTAN_BUILD("--no-package", "DrMarioRecomp") },
    { "nes", "Faxanadu", "https://github.com/mstan/FaxanaduRecomp.git",
      "repos/nes/faxanadu-recomp", "build-linux-prod/FaxanaduRecomp", NULL,
      MSTAN_DEPS,
      MSTAN_BUILD("--no-package", "FaxanaduRecomp") },
    { "nes", "The Legend of Zelda", "https://github.com/mstan/LegendOfZeldaNESRecomp.git",
      "repos/nes/zelda1-recomp", "build-linux-prod/LegendOfZeldaNESRecomp", NULL,
      MSTAN_DEPS,
      MSTAN_BUILD("--no-package", "LegendOfZeldaNESRecomp") },
    { "nes", "Mega Man 3", "https://github.com/mstan/Megaman3NESRecomp.git",
      "repos/nes/mm3-recomp", "build-linux-prod/MegaMan3Recomp", NULL,
      MSTAN_DEPS,
      MSTAN_BUILD("--no-package", "MegaMan3Recomp") },
    { "nes", "Metroid", "https://github.com/mstan/MetroidNESRecomp.git",
      "repos/nes/metroid-recomp", "build-linux-prod/MetroidNESRecomp", NULL,
      MSTAN_DEPS,
      MSTAN_BUILD("--no-package", "MetroidNESRecomp") },

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
    /* n64: static recompilations (retroreversing.com list). */
    { "n64", "Bomberman 64", "https://github.com/RevoSucks/BM64Recomp.git",
      "repos/n64/bm64-recomp", "build-cmake/BM64Recompiled", "bm64.decomp.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp bm64.us.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("BM64Recompiled") },
    { "n64", "Bomberman Hero", "https://github.com/RevoSucks/BMHeroRecomp.git",
      "repos/n64/bmhero-recomp", "build-cmake/BMHeroRecompiled", "bmhero.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp bmhero.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("BMHeroRecompiled") },
    { "n64", "Chameleon Twist", "https://github.com/Rainchus/ChameleonTwist1-JP-Recomp.git",
      "repos/n64/ct1-recomp", "build-cmake/ChameleonTwistJPRecompiled", "baserom.jp.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp jp.rev0.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("ChameleonTwistJPRecompiled") },
    { "n64", "Donkey Kong 64", "https://github.com/Rainchus/Donkey-Kong-64-Recompiled.git",
      "repos/n64/dk64-recomp", "build-cmake/DK64Recompiled", "donkeykong64.decompressed.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.toml && build-tools/RSPRecomp n_aspMain.toml && " N64RECOMP_CMAKE("DK64Recompiled") },
    { "n64", "Dr. Mario 64", "https://github.com/theboy181/drmario64_recomp_plus.git",
      "repos/n64/drmario64-recomp", "build-cmake/drmario64_recomp", "drmario64_uncompressed.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp drmario64.us.toml && build-tools/RSPRecomp aspMain.us.toml && " N64RECOMP_CMAKE("drmario64_recomp") },
    { "n64", "Duke Nukem: Zero Hour", "https://github.com/sonicdcer/DNZHRecomp.git",
      "repos/n64/dnzh-recomp", "build-cmake/DNZHRecompiled", "dnzh.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.toml && build-tools/RSPRecomp aspMain.us.toml && " N64RECOMP_CMAKE("DNZHRecompiled") },
    { "n64", "Extreme-G", "https://gitlab.com/sonicdcer/ExtremeGRecomp.git",
      "repos/n64/extremeg-recomp", "build-cmake/ExtremeGRecompiled", "extremeg.us.uncompressed.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.toml && build-tools/RSPRecomp aspMain.us.toml && " N64RECOMP_CMAKE("ExtremeGRecompiled") },
    { "n64", "Harvest Moon 64", "https://github.com/HarvestMoon64Recomp/HarvestMoon64Recomp.git",
      "repos/n64/hm64-recomp", "build-cmake/HarvestMoon64Recompiled", "hm64.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp hm64.us.toml && build-tools/RSPRecomp n_aspMain.toml && " N64RECOMP_CMAKE("HarvestMoon64Recompiled") },
    { "n64", "Mystical Ninja Starring Goemon", "https://github.com/klorfmorf/Goemon64Recomp.git",
      "repos/n64/goemon64-recomp", "build-cmake/Goemon64Recompiled", "mnsg.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp mnsg.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("Goemon64Recompiled") },
    { "n64", "Quest 64", "https://github.com/Rainchus/Quest64-Recomp.git",
      "repos/n64/quest64-recomp", "build-cmake/Quest64Recompiled", "quest64.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.rev0.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("Quest64Recompiled") },
    { "n64", "Snowboard Kids", "https://github.com/cdlewis/snowboardkids-recomp.git",
      "repos/n64/sk1-recomp", "build-cmake/SnowboardKidsRecompiled", "snowboardkids.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.toml && build-tools/RSPRecomp aspMain.us.toml && " N64RECOMP_CMAKE("SnowboardKidsRecompiled") },
    { "n64", "Snowboard Kids 2", "https://github.com/cdlewis/snowboardkids2-recomp.git",
      "repos/n64/sk2-recomp", "build-cmake/SnowboardKids2Recompiled", "snowboardkids2.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.toml && build-tools/RSPRecomp aspMain.us.toml && " N64RECOMP_CMAKE("SnowboardKids2Recompiled") },
    { "n64", "Space Station Silicon Valley", "https://github.com/Cellenseres/SSSV_Recomp.git",
      "repos/n64/sssv-recomp", "build-cmake/SSSVRecompiled", "sssv.us.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp sssv.us.toml && build-tools/RSPRecomp aspMain.toml && " N64RECOMP_CMAKE("SSSVRecompiled") },
    { "n64", "Mega Man 64", "https://github.com/MegaMan64Recomp/MegaMan64Recompiled.git",
      "repos/n64/mm64-recomp", "build-cmake/MegaMan64Recompiled", "MegaMan64.us.rev1.z64",
      N64RECOMP_DEPS,
      N64RECOMP_TOOLS " && build-tools/N64Recomp us.rev1.toml && build-tools/RSPRecomp aspMain.us.rev1.toml && " N64RECOMP_CMAKE("MegaMan64Recompiled") },
    { "n64", "Sin and Punishment", "https://github.com/wootbeer/snprecomp.git",
      "repos/n64/snp-recomp", "build/SnPRecompiled", "Tsumi to Batsu - Hoshi no Keishousha (Japan).z64",
      N64RECOMP_VK_DEPS,
      WOOT_BUILD("snp.jp.toml", "SnPRecompiled") },
    { "n64", "BattleTanx: Global Assault", "https://github.com/wootbeer/btgarecomp.git",
      "repos/n64/btga-recomp", "build/BattleTanxGARecompiled", "BattleTanx Global Assault (USA).z64",
      N64RECOMP_VK_DEPS,
      WOOT_BUILD("battletanxga.us.rev0.toml", "BattleTanxGARecompiled") },
    { "n64", "Battlezone: Rise of the Black Dogs", "https://github.com/wootbeer/bzrecomp.git",
      "repos/n64/bz-recomp", "build/BattlezoneRecompiled", "Battlezone - Rise of the Black Dogs (USA).z64",
      N64RECOMP_VK_DEPS,
      WOOT_BUILD("battlezone.us.rev0.toml", "BattlezoneRecompiled") },
    { "n64", "Turok: Rage Wars", "https://github.com/wootbeer/PaxDino.git",
      "repos/n64/turokrw-recomp", "build/PaxDino", "turokrw.us.z64",
      N64RECOMP_VK_DEPS,
      WOOT_BUILD("turokrw.us.toml", "PaxDino") },
    { "n64", "Yoshi's Story", "https://github.com/wootbeer/dinotale.git",
      "repos/n64/yoshisstory-recomp", "build/DinoTale", "yoshisstory.us.z64",
      N64RECOMP_VK_DEPS,
      WOOT_BUILD("yoshisstory.us.toml", "DinoTale") },
    { "n64", "Pilotwings 64", "https://github.com/danielgomesvieira2000/pilotwings-64-recomp.git",
      "repos/n64/pw64-recomp", "build-linux/Pilotwings64Recomp", "pilotwings64.us.z64",
      N64RECOMP_DG_DEPS,
      "bash tools/build_linux.sh pilotwings64.us.z64" },
    { "n64", "Body Harvest", "https://github.com/danielgomesvieira2000/body-harvest-recomp.git",
      "repos/n64/bodyharvest-recomp", "build-linux/body-harvest-recomp", "bodyharvest.us.z64",
      N64RECOMP_DG_DEPS,
      "bash tools/build_linux.sh bodyharvest.us.z64" },
    { "n64", "Hybrid Heaven", "https://github.com/danielgomesvieira2000/hybrid-heaven-recomp.git",
      "repos/n64/hybridheaven-recomp", "build-linux/hybrid-heaven-recomp", "hybridheaven.us.z64",
      N64RECOMP_DG_DEPS,
      "bash tools/build_linux.sh hybridheaven.us.z64" },
    { "n64", "Wave Race 64", "https://github.com/danielgomesvieira2000/wave-race-64-recomp.git",
      "repos/n64/waverace64-recomp", "build-linux/WaveRace64Recomp", "waverace64.us.revA.z64",
      N64RECOMP_DG_DEPS,
      "bash tools/build_linux.sh waverace64.us.revA.z64" },
    { "n64", "GoldenEye 007", "https://github.com/cblock85/GoldenEye64Recomp.git",
      "repos/n64/ge007-recomp", "build/GoldenRecomp", "baserom.u.z64",
      N64RECOMP_GE_DEPS,
      "bash ./build_linux.sh" },
    { "n64", "Jet Force Gemini", "https://github.com/VibinToast/JetForceGemini-Recomp.git",
      "repos/n64/jfg-recomp", "bin/JetForceGeminiRecompiled", "baserom.us.z64",
      N64RECOMP_VK_DEPS,
      "bash ./build_linux.sh" },
    { "n64", "Conker's Bad Fur Day", "https://github.com/sciaschi/CBFD-Recompiled.git",
      "repos/n64/conker-recomp", "host/build/ConkerRecomp", "conker/baserom.us.z64",
      N64RECOMP_DEPS,
      "sh ./build.sh" },
    { "n64", "Banjo-Tooie", "https://github.com/Vidanox/BanjoTooieRecompiled.git",
      "repos/n64/bt-recomp", "build-cmake/BanjoTooieRecompiled", NULL,
      N64RECOMP_BT_DEPS,
      "python3 tools/setup_deps.py && " N64RECOMP_CMAKE("BanjoTooieRecompiled") },

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
        { "building", S_REPO_BUILDING }, { "not_cloned", S_REPO_NOT_CLONED }, { "build_failed", S_REPO_BUILD_FAILED },
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

bool Repo_CanInstall(RepoLocalState s, int lastExit)
{
    return s == REPO_NOT_BUILT && lastExit != 21 && lastExit != 2;
}

int Repo_StatusExit(const char *line)
{
    if (!line) return -1;
    if (strncmp(line, "build_failed", 12) == 0 || strncmp(line, "build_no_binary", 15) == 0) return 2;
    if (strncmp(line, "missing_deps", 12) == 0) return 21;
    return -1;
}

RepoNote Repo_Transition(bool known, RepoLocalState prev, RepoLocalState now)
{
    if (!known || prev == now) return REPO_NOTE_NONE;
    if (now == REPO_ROM_NEEDED && prev == REPO_NOT_CLONED) return REPO_NOTE_ROM_NEEDED;
    if (now == REPO_NOT_BUILT && (prev == REPO_ROM_NEEDED || prev == REPO_NOT_CLONED))
        return REPO_NOTE_INSTALLABLE;
    if (now == REPO_READY && prev != REPO_LOCAL_COPY) return REPO_NOTE_INSTALLED;
    return REPO_NOTE_NONE;
}

#endif /* REPOS_IMPLEMENTATION_ONCE */
#endif /* REPOS_IMPLEMENTATION */
