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

/* Mednafen (mednafen/src/mednafen, built from the source tree in the NSO root)
 * runs the ROM in repos/roms/NES. Config and saves: saves/nes/mednafen; NSO's
 * suspend points: saves/nes/mednafen/states/<ROM file>. */
#define MEDNAFEN_NES(file) \
    .dir = "mednafen/src", .binary = "./mednafen", .mednafen = "nes", \
    .rom = "repos/roms/NES/" file, \
    .stateDir = "saves/nes/mednafen/states/" file, .externalSlots = true

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
    /* Everything else runs in Mednafen (mednafen/src/mednafen), ROMs from repos/roms/NES. */
    {
        .title = "Abadox: The Deadly Inner War",
        .boxartPath = "assets/images/systems/nes/abadox.png",
        MEDNAFEN_NES("Abadox (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alpha Mission",
        .boxartPath = "assets/images/systems/nes/alphamission.png",
        MEDNAFEN_NES("Alpha Mission (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Batman: Return of the Joker",
        .boxartPath = "assets/images/systems/nes/batmanrotj.png",
        MEDNAFEN_NES("Batman - Return of the Joker (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Garry Kitchen's Battletank",
        .boxartPath = "assets/images/systems/nes/battletank.png",
        MEDNAFEN_NES("Battle Tank (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battletoads & Double Dragon",
        .boxartPath = "assets/images/systems/nes/bttdd.png",
        MEDNAFEN_NES("Battletoads Double Dragon (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battletoads",
        .boxartPath = "assets/images/systems/nes/battletoads.png",
        MEDNAFEN_NES("Battletoads (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania II: Simon's Quest",
        .boxartPath = "assets/images/systems/nes/castlevania2.png",
        MEDNAFEN_NES("Castlevania 2 - Simon's Quest (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania III: Dracula's Curse",
        .boxartPath = "assets/images/systems/nes/castlevania3.png",
        MEDNAFEN_NES("Castlevania 3 - Dracula's Curse (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania",
        .boxartPath = "assets/images/systems/nes/castlevania.png",
        MEDNAFEN_NES("Castlevania (U) (PRG 0).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Contra",
        .boxartPath = "assets/images/systems/nes/contra.png",
        MEDNAFEN_NES("Contra (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Classics",
        .boxartPath = "assets/images/systems/nes/dkclassics.png",
        MEDNAFEN_NES("Donkey Kong Classics (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Jr. no Sansuu Asobi (Japan)",
        .boxartPath = "assets/images/systems/nes/dkjrmath.png",
        MEDNAFEN_NES("Donkey Kong Jr. Math (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon II: The Revenge",
        .boxartPath = "assets/images/systems/nes/doubledragon2.png",
        MEDNAFEN_NES("Double Dragon 2 - The Revenge (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon III: The Sacred Stones",
        .boxartPath = "assets/images/systems/nes/doubledragon3.png",
        MEDNAFEN_NES("Double Dragon 3 - The Sacred Stones (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon",
        .boxartPath = "assets/images/systems/nes/doubledragon.png",
        MEDNAFEN_NES("Double Dragon (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "DuckTales 2",
        .boxartPath = "assets/images/systems/nes/ducktales2.png",
        MEDNAFEN_NES("Duck Tales 2 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "DuckTales",
        .boxartPath = "assets/images/systems/nes/ducktales.png",
        MEDNAFEN_NES("Duck Tales (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Excitebike (Japan, USA)",
        .boxartPath = "assets/images/systems/nes/excitebike.png",
        MEDNAFEN_NES("Excitebike (JU).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galaga: Demons of Death",
        .boxartPath = "assets/images/systems/nes/galaga.png",
        MEDNAFEN_NES("Galaga (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghosts'n Goblins",
        .boxartPath = "assets/images/systems/nes/ghostsngoblins.png",
        MEDNAFEN_NES("Ghosts'n Goblins (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gradius",
        .boxartPath = "assets/images/systems/nes/gradius.png",
        MEDNAFEN_NES("Gradius (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gun.Smoke",
        .boxartPath = "assets/images/systems/nes/gunsmoke.png",
        MEDNAFEN_NES("Gun Smoke (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Kage",
        .boxartPath = "assets/images/systems/nes/kage.png",
        MEDNAFEN_NES("Legend of Kage, The (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 2",
        .boxartPath = "assets/images/systems/nes/megaman2.png",
        MEDNAFEN_NES("Mega Man 2 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 4",
        .boxartPath = "assets/images/systems/nes/megaman4.png",
        MEDNAFEN_NES("Mega Man 4 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 5",
        .boxartPath = "assets/images/systems/nes/megaman5.png",
        MEDNAFEN_NES("Mega Man 5 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 6",
        .boxartPath = "assets/images/systems/nes/megaman6.png",
        MEDNAFEN_NES("Mega Man 6 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man",
        .boxartPath = "assets/images/systems/nes/megaman.png",
        MEDNAFEN_NES("Mega Man (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gimmick! (Japan)",
        .boxartPath = "assets/images/systems/nes/mrgimmick.png",
        MEDNAFEN_NES("Mr. Gimmick (E) [!].nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ms. Pac-Man (Namco)",
        .boxartPath = "assets/images/systems/nes/mspacman_namco.png",
        MEDNAFEN_NES("Ms Pac-Man (Namco) (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ms. Pac-Man (USA) (Tengen) (Unl)",
        .boxartPath = "assets/images/systems/nes/mspacman_tengen.png",
        MEDNAFEN_NES("Ms Pac-Man (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden II: The Dark Sword of Chaos",
        .boxartPath = "assets/images/systems/nes/ninjagaiden2.png",
        MEDNAFEN_NES("Ninja Gaiden 2 - The Dark Sword of Chaos (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden III: The Ancient Ship of Doom",
        .boxartPath = "assets/images/systems/nes/ninjagaiden3.png",
        MEDNAFEN_NES("Ninja Gaiden 3 - The Ancient Ship of Doom (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden",
        .boxartPath = "assets/images/systems/nes/ninjagaiden.png",
        MEDNAFEN_NES("Ninja Gaiden (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pac-Mania (USA) (Unl)",
        .boxartPath = "assets/images/systems/nes/pacmania.png",
        MEDNAFEN_NES("Pacmania (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pac-Man (Tengen)",
        .boxartPath = "assets/images/systems/nes/pacman.png",
        MEDNAFEN_NES("Pac-Man (U) [!].nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Road Fighter (Japan)",
        .boxartPath = "assets/images/systems/nes/roadfighter.png",
        MEDNAFEN_NES("Road Fighter (J) [T-Port].nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bartman Meets Radioactive Man",
        .boxartPath = "assets/images/systems/nes/simpsons_bartman.png",
        MEDNAFEN_NES("Simpsons - Bartman Meets Radioactive Man, The (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bart vs. the Space Mutants",
        .boxartPath = "assets/images/systems/nes/simpsons_bvsm.png",
        MEDNAFEN_NES("Simpsons - Bart Vs the Space Mutants, The (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Simpsons: Bart vs. the World",
        .boxartPath = "assets/images/systems/nes/simpsons_bvtw.png",
        MEDNAFEN_NES("Simpsons - Bart Vs the World, The (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super C",
        .boxartPath = "assets/images/systems/nes/superc.png",
        MEDNAFEN_NES("Super C (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles II: The Arcade Game",
        .boxartPath = "assets/images/systems/nes/tmnt2.png",
        MEDNAFEN_NES("Teenage Mutant Ninja Turtles 2 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles III: The Manhattan Project",
        .boxartPath = "assets/images/systems/nes/tmnt3.png",
        MEDNAFEN_NES("Teenage Mutant Ninja Turtles 3 (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles: Tournament Fighters",
        .boxartPath = "assets/images/systems/nes/tmnt_tf.png",
        MEDNAFEN_NES("Teenage Mutant Ninja Turtles Tournament Fighters (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles",
        .boxartPath = "assets/images/systems/nes/tmnt.png",
        MEDNAFEN_NES("Teenage Mutant Ninja Turtles (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiger-Heli",
        .boxartPath = "assets/images/systems/nes/tigerheli.png",
        MEDNAFEN_NES("Tiger-Heli (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures 2: Trouble in Wackyland",
        .boxartPath = "assets/images/systems/nes/tinytoon2.png",
        MEDNAFEN_NES("Tiny Toon Adventures 2 - Trouble in Wackyland (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures Cartoon Workshop",
        .boxartPath = "assets/images/systems/nes/tinytoon_cw.png",
        MEDNAFEN_NES("Tiny Toon Adventures Cartoon Workshop (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures",
        .boxartPath = "assets/images/systems/nes/tinytoon.png",
        MEDNAFEN_NES("Tiny Toon Adventures (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gun",
        .boxartPath = "assets/images/systems/nes/topgun.png",
        MEDNAFEN_NES("Top Gun (U).nes"),
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int nes_game_count = (int)(sizeof(nes_games) / sizeof(nes_games[0]));

#endif /* NES_IMPLEMENTATION_ONCE */
#endif /* NES_IMPLEMENTATION */
