/* n64.h - STB-style Nintendo 64 system module */
#ifndef N64_H
#define N64_H

#include "nso.h"

extern PlatformEntry n64_platform;
extern GameEntry     n64_games[];
extern const int     n64_game_count;

#endif /* N64_H */

#ifdef N64_IMPLEMENTATION
#ifndef N64_IMPLEMENTATION_ONCE
#define N64_IMPLEMENTATION_ONCE

PlatformEntry n64_platform = {
    .id = "n64",
    .name = "Nintendo 64",
    .imagePath = "assets/images/systems/n64/n64.png",
    .orient = SLOT_ORIENT_USA,
};

GameEntry n64_games[] = {
    {
        .title = "Super Mario 64",
        .boxartPath = "assets/images/systems/n64/sm64.png",
        .dir = "repos/n64/sm64",
        .binary = "./sm64",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zelda Ocarina of Time",
        .boxartPath = "assets/images/systems/n64/oot.png",
        .dir = "repos/n64/soh",
        .binary = "./soh.appimage",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Majora's Mask",
        .boxartPath = "assets/images/systems/n64/mm.png",
        .dir = "repos/n64/2s2h",
        .binary = "./2ship.appimage",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Perfect Dark",
        .boxartPath = "assets/images/systems/n64/pd.png",
        .dir = "repos/n64/perfect_dark-src",
        .binary = "build/pd.x86_64",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Smash Bros",
        .boxartPath = "assets/images/systems/n64/ssb64.png",
        .dir = "repos/n64/ssb64",
        .binary = "ssb64",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Fox",
        .boxartPath = "assets/images/systems/n64/starfox.png",
        .dir = "repos/n64/starship",
        .binary = "starship.appimage",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "1080° Snowboarding",
        .boxartPath = "assets/images/systems/n64/1080.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Banjo Kazooie",
        .boxartPath = "assets/images/systems/n64/bk.png",
        .dir = "repos/n64/bk",
        .binary = "BanjoRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Banjo-Tooie",
        .boxartPath = "assets/images/systems/n64/Banjo-Tooie.png",
        .dir = "repos/n64/bt-recomp",
        .binary = "build-cmake/BanjoTooieRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dr. Mario 64",
        .boxartPath = "assets/images/systems/n64/Dr._Mario_64_NSO.png",
        .dir = "repos/n64/drmario64-recomp",
        .binary = "build-cmake/drmario64_recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duke Nukem: Zero Hour",
        .boxartPath = "assets/images/systems/n64/Duke_Nukem_Zero_Hour.png",
        .dir = "repos/n64/dnzh-recomp",
        .binary = "build-cmake/DNZHRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-Zero X",
        .boxartPath = "assets/images/systems/n64/F_Zero_X_NSO.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "GoldenEye 007",
        .boxartPath = "assets/images/systems/n64/GoldenEye_007_NSO.png",
        .dir = "repos/n64/ge007-recomp",
        .binary = "build/GoldenRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Killer Instinct Gold",
        .boxartPath = "assets/images/systems/n64/KillerInstinctGold.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Kart 64",
        .boxartPath = "assets/images/systems/n64/Mario_Kart_64_NSO.png",
        .dir = "repos/n64/mk64",
        .binary = "spaghetti.appimage",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Party 3",
        .boxartPath = "assets/images/systems/n64/Mario_Party_3_NSO.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Tennis",
        .boxartPath = "assets/images/systems/n64/Mariotennis64.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mortal Kombat Mythologies: Sub-Zero",
        .boxartPath = "assets/images/systems/n64/MortalKombatMythologiesSubZero.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paper Mario",
        .boxartPath = "assets/images/systems/n64/Paper_Mario_NSO.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pilotwings 64",
        .boxartPath = "assets/images/systems/n64/Pilotwings_N64_NSO.png",
        .dir = "repos/n64/pw64-recomp",
        .binary = "build-linux/Pilotwings64Recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pokémon Stadium",
        .boxartPath = "assets/images/systems/n64/Pokemon_Stadium.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars Episode I: Racer",
        .boxartPath = "assets/images/systems/n64/Star_Wars_Episode_I_Racer.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tony Hawk's Pro Skater",
        .boxartPath = "assets/images/systems/n64/Tony_Hawk_Pro_Skater.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wave Race 64",
        .boxartPath = "assets/images/systems/n64/Wave_Race_64_NSO.png",
        .dir = "repos/n64/waverace64-recomp",
        .binary = "build-linux/WaveRace64Recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman 64",
        .boxartPath = "assets/images/systems/n64/bm64.png",
        .dir = "repos/n64/bm64-recomp",
        .binary = "build-cmake/BM64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman Hero",
        .boxartPath = "assets/images/systems/n64/bmhero.png",
        .dir = "repos/n64/bmhero-recomp",
        .binary = "build-cmake/BMHeroRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chameleon Twist",
        .boxartPath = "assets/images/systems/n64/ct1.png",
        .dir = "repos/n64/ct1-recomp",
        .binary = "build-cmake/ChameleonTwistJPRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong 64",
        .boxartPath = "assets/images/systems/n64/dk64.png",
        .dir = "repos/n64/dk64-recomp",
        .binary = "build-cmake/DK64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Extreme-G",
        .boxartPath = "assets/images/systems/n64/extremeg.png",
        .dir = "repos/n64/extremeg-recomp",
        .binary = "build-cmake/ExtremeGRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Harvest Moon 64",
        .boxartPath = "assets/images/systems/n64/hm64.png",
        .dir = "repos/n64/hm64-recomp",
        .binary = "build-cmake/HarvestMoon64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mystical Ninja Starring Goemon",
        .boxartPath = "assets/images/systems/n64/goemon64.png",
        .dir = "repos/n64/goemon64-recomp",
        .binary = "build-cmake/Goemon64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quest 64",
        .boxartPath = "assets/images/systems/n64/quest64.png",
        .dir = "repos/n64/quest64-recomp",
        .binary = "build-cmake/Quest64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snowboard Kids",
        .boxartPath = "assets/images/systems/n64/sk1.png",
        .dir = "repos/n64/sk1-recomp",
        .binary = "build-cmake/SnowboardKidsRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snowboard Kids 2",
        .boxartPath = "assets/images/systems/n64/sk2.png",
        .dir = "repos/n64/sk2-recomp",
        .binary = "build-cmake/SnowboardKids2Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Station Silicon Valley",
        .boxartPath = "assets/images/systems/n64/sssv.png",
        .dir = "repos/n64/sssv-recomp",
        .binary = "build-cmake/SSSVRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 64",
        .boxartPath = "assets/images/systems/n64/mm64.png",
        .dir = "repos/n64/mm64-recomp",
        .binary = "build-cmake/MegaMan64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sin and Punishment",
        .boxartPath = "assets/images/systems/n64/snp.png",
        .dir = "repos/n64/snp-recomp",
        .binary = "build/SnPRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "BattleTanx: Global Assault",
        .boxartPath = "assets/images/systems/n64/btga.png",
        .dir = "repos/n64/btga-recomp",
        .binary = "build/BattleTanxGARecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battlezone: Rise of the Black Dogs",
        .boxartPath = "assets/images/systems/n64/bz.png",
        .dir = "repos/n64/bz-recomp",
        .binary = "build/BattlezoneRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turok: Rage Wars",
        .boxartPath = "assets/images/systems/n64/turokrw.png",
        .dir = "repos/n64/turokrw-recomp",
        .binary = "build/PaxDino",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi's Story",
        .boxartPath = "assets/images/systems/n64/yoshisstory.png",
        .dir = "repos/n64/yoshisstory-recomp",
        .binary = "build/DinoTale",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Body Harvest",
        .boxartPath = "assets/images/systems/n64/bodyharvest.png",
        .dir = "repos/n64/bodyharvest-recomp",
        .binary = "build-linux/body-harvest-recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hybrid Heaven",
        .boxartPath = "assets/images/systems/n64/hybridheaven.png",
        .dir = "repos/n64/hybridheaven-recomp",
        .binary = "build-linux/hybrid-heaven-recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jet Force Gemini",
        .boxartPath = "assets/images/systems/n64/jfg.png",
        .dir = "repos/n64/jfg-recomp",
        .binary = "bin/JetForceGeminiRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Conker's Bad Fur Day",
        .boxartPath = "assets/images/systems/n64/conker.png",
        .dir = "repos/n64/conker-recomp",
        .binary = "host/build/ConkerRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int n64_game_count = (int)(sizeof(n64_games) / sizeof(n64_games[0]));

#endif /* N64_IMPLEMENTATION_ONCE */
#endif /* N64_IMPLEMENTATION */
