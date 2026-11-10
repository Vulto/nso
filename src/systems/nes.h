/* nes.h - STB-style NES system module */
#ifndef NES_H
#define NES_H

#include "nso.h"

extern PlatformEntry nes_platform;
extern GameEntry     nes_games[];
extern const int     nes_game_count;

#endif /* NES_H */

#ifdef NES_IMPLEMENTATION
#ifndef NES_IMPLEMENTATION_ONCE
#define NES_IMPLEMENTATION_ONCE

#include "launch.h"

static const NsoRomImport opendana_rom_import = {
    .ext = ".nes",
    .importArg = "--import-rom",
    .importedRel = "opendana/content/base/pack.json",
};

PlatformEntry nes_platform = {
    .id = "nes",
    .name = "NES",
    .imagePath = "assets/images/systems/nes/nes.png",
    .orient = SLOT_ORIENT_JAPAN,
};

GameEntry nes_games[] = {
    {
        .title = "Super Mario Bros",
        .boxartPath = "assets/images/systems/nes/smb.png",
        .dir = "repos/nes/smb",
        .binary = "./smb",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Wing of Madoola",
        .boxartPath = "assets/images/systems/nes/openmadoola.png",
        .dir = "repos/nes/openmadoola",
        .binary = "build/openmadoola",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fire 'n Ice",
        .boxartPath = "assets/images/systems/nes/opendana.png",
        .dir = "repos/nes/opendana",
        .binary = "build/opendana",
        .orient = SLOT_ORIENT_INHERIT,
        .romImport = &opendana_rom_import,
    },
};

const int nes_game_count = (int)(sizeof(nes_games) / sizeof(nes_games[0]));

#endif /* NES_IMPLEMENTATION_ONCE */
#endif /* NES_IMPLEMENTATION */
