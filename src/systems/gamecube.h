/* gamecube.h - STB-style GameCube system module */
#ifndef GAMECUBE_H
#define GAMECUBE_H

#include "nso.h"

extern PlatformEntry gamecube_platform;
extern GameEntry     gamecube_games[];
extern const int     gamecube_game_count;

#endif /* GAMECUBE_H */

#ifdef GAMECUBE_IMPLEMENTATION
#ifndef GAMECUBE_IMPLEMENTATION_ONCE
#define GAMECUBE_IMPLEMENTATION_ONCE

PlatformEntry gamecube_platform = {
    .id = "gamecube",
    .name = "GameCube",
    .imagePath = "assets/images/systems/gamecube/gamecube.png",
    .orient = SLOT_ORIENT_JAPAN,
};

GameEntry gamecube_games[] = {
    {
        .title = "Twilight Princess",
        .boxartPath = "assets/images/systems/gamecube/ztp.png",
        .dir = "repos/gamecube/dusk",
        .binary = "./ztp.appimage",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Smash Bros. Melee",
        .boxartPath = "assets/images/systems/gamecube/melee.png",
        .dir = "repos/gamecube/melee-pc",
        .binary = "build/melee",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metroid Prime",
        .boxartPath = "assets/images/systems/gamecube/metroidprime.png",
        .dir = "repos/gamecube/metroidprime",
        .binary = "build/native/metroid_prime_port",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paper Mario: The Thousand-Year Door",
        .boxartPath = "assets/images/systems/gamecube/papercut.png",
        .dir = "repos/gamecube/papercut",
        .binary = "build-pc-us-aurora/ttyd-us",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int gamecube_game_count =
    (int)(sizeof(gamecube_games) / sizeof(gamecube_games[0]));

#endif /* GAMECUBE_IMPLEMENTATION_ONCE */
#endif /* GAMECUBE_IMPLEMENTATION */
