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
        .dir = "repos/n64/perfect_dark/build",
        .binary = "pd.x86_64",
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
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dr. Mario 64",
        .boxartPath = "assets/images/systems/n64/Dr._Mario_64_NSO.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duke Nukem: Zero Hour",
        .boxartPath = "assets/images/systems/n64/Duke_Nukem_Zero_Hour.png",
        .dir = NULL,
        .binary = NULL,
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
        .dir = NULL,
        .binary = NULL,
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
        .dir = NULL,
        .binary = NULL,
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
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int n64_game_count = (int)(sizeof(n64_games) / sizeof(n64_games[0]));

#endif /* N64_IMPLEMENTATION_ONCE */
#endif /* N64_IMPLEMENTATION */
