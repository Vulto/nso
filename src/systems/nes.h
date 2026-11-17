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
        .title = "Madoola no Tsubasa (Japan)",
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
    {   /* Megaman3NESRecomp (repos/nes/mm3-recomp) inside NSO's window: `make embed` */
        .title = "Mega Man 3",
        .boxartPath = "assets/images/systems/nes/mm3.png",
        .dir = "embed/mm3",
        .binary = "build/libmm3mod.so",
        .orient = SLOT_ORIENT_INHERIT,
        .stateDir = "saves/nes/mm3",
        .embedRomKey = "nes.mm3_rom",
    },
    {
        .title = "Metroid",
        .boxartPath = "assets/images/systems/nes/metroid.png",
        .dir = "repos/nes/metroid-recomp",
        .binary = "build-linux-prod/MetroidNESRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    /* Display-only: cover and catalog, nothing to launch yet. */
    {
        .title = "Abadox: The Deadly Inner War",
        .boxartPath = "assets/images/systems/nes/abadox.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alpha Mission",
        .boxartPath = "assets/images/systems/nes/alphamission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Batman: Return of the Joker",
        .boxartPath = "assets/images/systems/nes/batmanrotj.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Garry Kitchen's Battletank",
        .boxartPath = "assets/images/systems/nes/battletank.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battletoads & Double Dragon",
        .boxartPath = "assets/images/systems/nes/bttdd.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battletoads",
        .boxartPath = "assets/images/systems/nes/battletoads.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania II: Simon's Quest",
        .boxartPath = "assets/images/systems/nes/castlevania2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania III: Dracula's Curse",
        .boxartPath = "assets/images/systems/nes/castlevania3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania",
        .boxartPath = "assets/images/systems/nes/castlevania.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Contra",
        .boxartPath = "assets/images/systems/nes/contra.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Classics",
        .boxartPath = "assets/images/systems/nes/dkclassics.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Jr. no Sansuu Asobi (Japan)",
        .boxartPath = "assets/images/systems/nes/dkjrmath.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon II: The Revenge",
        .boxartPath = "assets/images/systems/nes/doubledragon2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon III: The Sacred Stones",
        .boxartPath = "assets/images/systems/nes/doubledragon3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon",
        .boxartPath = "assets/images/systems/nes/doubledragon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "DuckTales 2",
        .boxartPath = "assets/images/systems/nes/ducktales2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "DuckTales",
        .boxartPath = "assets/images/systems/nes/ducktales.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Excitebike (Japan, USA)",
        .boxartPath = "assets/images/systems/nes/excitebike.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galaga: Demons of Death",
        .boxartPath = "assets/images/systems/nes/galaga.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghosts'n Goblins",
        .boxartPath = "assets/images/systems/nes/ghostsngoblins.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gradius",
        .boxartPath = "assets/images/systems/nes/gradius.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gun.Smoke",
        .boxartPath = "assets/images/systems/nes/gunsmoke.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Kage",
        .boxartPath = "assets/images/systems/nes/kage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 2",
        .boxartPath = "assets/images/systems/nes/megaman2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 4",
        .boxartPath = "assets/images/systems/nes/megaman4.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 5",
        .boxartPath = "assets/images/systems/nes/megaman5.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 6",
        .boxartPath = "assets/images/systems/nes/megaman6.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man",
        .boxartPath = "assets/images/systems/nes/megaman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gimmick! (Japan)",
        .boxartPath = "assets/images/systems/nes/mrgimmick.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ms. Pac-Man (Namco)",
        .boxartPath = "assets/images/systems/nes/mspacman_namco.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ms. Pac-Man (USA) (Tengen) (Unl)",
        .boxartPath = "assets/images/systems/nes/mspacman_tengen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden II: The Dark Sword of Chaos",
        .boxartPath = "assets/images/systems/nes/ninjagaiden2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden III: The Ancient Ship of Doom",
        .boxartPath = "assets/images/systems/nes/ninjagaiden3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden",
        .boxartPath = "assets/images/systems/nes/ninjagaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pac-Mania (USA) (Unl)",
        .boxartPath = "assets/images/systems/nes/pacmania.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pac-Man (Tengen)",
        .boxartPath = "assets/images/systems/nes/pacman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Road Fighter (Japan)",
        .boxartPath = "assets/images/systems/nes/roadfighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bartman Meets Radioactive Man",
        .boxartPath = "assets/images/systems/nes/simpsons_bartman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bart vs. the Space Mutants",
        .boxartPath = "assets/images/systems/nes/simpsons_bvsm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bart vs. the World",
        .boxartPath = "assets/images/systems/nes/simpsons_bvtw.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super C",
        .boxartPath = "assets/images/systems/nes/superc.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles II: The Arcade Game",
        .boxartPath = "assets/images/systems/nes/tmnt2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles III: The Manhattan Project",
        .boxartPath = "assets/images/systems/nes/tmnt3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles: Tournament Fighters",
        .boxartPath = "assets/images/systems/nes/tmnt_tf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles",
        .boxartPath = "assets/images/systems/nes/tmnt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiger-Heli",
        .boxartPath = "assets/images/systems/nes/tigerheli.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures 2: Trouble in Wackyland",
        .boxartPath = "assets/images/systems/nes/tinytoon2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures Cartoon Workshop",
        .boxartPath = "assets/images/systems/nes/tinytoon_cw.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures",
        .boxartPath = "assets/images/systems/nes/tinytoon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gun",
        .boxartPath = "assets/images/systems/nes/topgun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int nes_game_count = (int)(sizeof(nes_games) / sizeof(nes_games[0]));

#endif /* NES_IMPLEMENTATION_ONCE */
#endif /* NES_IMPLEMENTATION */
