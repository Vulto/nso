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
    {   /* SMB Vanilla (repos/nes/smbvanilla) inside NSO's window: `make embed` */
        .title = "Super Mario Bros",
        .boxartPath = "assets/images/systems/nes/smb.png",
        .dir = "embed/smb",
        .binary = "build/libsmbmod.so",
        .orient = SLOT_ORIENT_INHERIT,
        .stateDir = "saves/nes/smb",
        .embedRomKey = "nes.smb_rom",
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
    {
        .title = "Dr. Mario",
        .boxartPath = "assets/images/systems/nes/drmario.png",
        .dir = "repos/nes/drmario-recomp",
        .binary = "build-linux-prod/DrMarioRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Faxanadu",
        .boxartPath = "assets/images/systems/nes/faxanadu.png",
        .dir = "repos/nes/faxanadu-recomp",
        .binary = "build-linux-prod/FaxanaduRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Zelda",
        .boxartPath = "assets/images/systems/nes/zelda1.png",
        .dir = "repos/nes/zelda1-recomp",
        .binary = "build-linux-prod/LegendOfZeldaNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 3",
        .boxartPath = "assets/images/systems/nes/mm3.png",
        .dir = "repos/nes/mm3-recomp",
        .binary = "build-linux-prod/MegaMan3Recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metroid",
        .boxartPath = "assets/images/systems/nes/metroid.png",
        .dir = "repos/nes/metroid-recomp",
        .binary = "build-linux-prod/MetroidNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int nes_game_count = (int)(sizeof(nes_games) / sizeof(nes_games[0]));

#endif /* NES_IMPLEMENTATION_ONCE */
#endif /* NES_IMPLEMENTATION */
