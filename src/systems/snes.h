/* snes.h - STB-style SNES system module */
#ifndef SNES_H
#define SNES_H

#include "nso.h"

extern PlatformEntry snes_platform;
extern GameEntry     snes_games[];
extern const int     snes_game_count;

#endif /* SNES_H */

#ifdef SNES_IMPLEMENTATION
#ifndef SNES_IMPLEMENTATION_ONCE
#define SNES_IMPLEMENTATION_ONCE

PlatformEntry snes_platform = {
    .id = "snes",
    .name = "SNES",
    .imagePath = "assets/images/systems/snes/snes.png",
    .orient = SLOT_ORIENT_USA,
};

GameEntry snes_games[] = {
    {
        .title = "Super Mario World",
        .boxartPath = "assets/images/systems/snes/smw.png",
        .dir = "repos/snes/smw",
        .binary = "./smw",
        .stateDir = "saves/snes/smw",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Zelda: A Link to the Past",
        .boxartPath = "assets/images/systems/snes/zelda3.png",
        .dir = "repos/snes/zelda3",
        .binary = "./zelda3",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Metroid",
        .boxartPath = "assets/images/systems/snes/sm.png",
        .dir = "repos/snes/sm",
        .binary = "./sm",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-Zero",
        .boxartPath = "assets/images/systems/snes/fzero.png",
        .dir = "repos/snes/fzero-recomp",
        .binary = "build/fzero_recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Go Go Ackman",
        .boxartPath = "assets/images/systems/snes/ackman.png",
        .dir = "repos/snes/ackman-recomp",
        .binary = "build/AckmanRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kirby Super Star",
        .boxartPath = "assets/images/systems/snes/kss.png",
        .dir = "repos/snes/kss-recomp",
        .binary = "build/KirbySuperStarSNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario All-Stars",
        .boxartPath = "assets/images/systems/snes/smas.png",
        .dir = "repos/snes/smas-recomp",
        .binary = "build/SuperMarioAllStarsRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario RPG: Legend of the Seven Stars",
        .boxartPath = "assets/images/systems/snes/smrpg.png",
        .dir = "repos/snes/smrpg-recomp",
        .binary = "build-linux-prod/SuperMarioRPGSNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man X2",
        .boxartPath = "assets/images/systems/snes/mmx2.png",
        .dir = "repos/snes/mmx2-recomp",
        .binary = "build-linux-prod/MegaManX2SNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man X3",
        .boxartPath = "assets/images/systems/snes/mmx3.png",
        .dir = "repos/snes/mmx3-recomp",
        .binary = "build-linux-prod/MegaManX3SNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Fox",
        .boxartPath = "assets/images/systems/snes/starfox.png",
        .dir = "repos/snes/starfox-recomp",
        .binary = "build-linux-prod/StarFoxSNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int snes_game_count = (int)(sizeof(snes_games) / sizeof(snes_games[0]));

#endif /* SNES_IMPLEMENTATION_ONCE */
#endif /* SNES_IMPLEMENTATION */
