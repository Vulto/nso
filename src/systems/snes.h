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
};

const int snes_game_count = (int)(sizeof(snes_games) / sizeof(snes_games[0]));

#endif /* SNES_IMPLEMENTATION_ONCE */
#endif /* SNES_IMPLEMENTATION */
