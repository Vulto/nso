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
 * runs the ROM in roms/NES. Config and saves: saves/nes/mednafen; NSO's
 * suspend points: saves/nes/mednafen/states/<ROM file>. */
#define MEDNAFEN_NES(file) \
    .dir = "mednafen/src", .binary = "./mednafen", .mednafen = "nes", \
    .rom = "roms/NES/" file, \
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
    /* Everything else runs in Mednafen (mednafen/src/mednafen), ROMs from roms/NES. */
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
    /* Names only (display-only): complete NES list from
     * https://nintendo.fandom.com/wiki/List_of_Nintendo_Entertainment_System_games
     * merged with the entries above; existing titles and spelling kept.
     * Japan, North America, Europe, and Canceled/Unreleased. No ROM or
     * launch paths; boxartPath is the cover slot used when a ROM exists. */
    {
        .title = "$1,000,000 Kid: Maboroshi no Teiou Hen",
        .boxartPath = "assets/images/systems/nes/1-000-000-kid-maboroshi-no-teiou-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "10-Yard Fight",
        .boxartPath = "assets/images/systems/nes/10-yard-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "1942",
        .boxartPath = "assets/images/systems/nes/1942.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "1943: The Battle of Midway",
        .boxartPath = "assets/images/systems/nes/1943-the-battle-of-midway.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "1943: The Battle of Valhalla",
        .boxartPath = "assets/images/systems/nes/1943-the-battle-of-valhalla.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "1999: Hore, Mita Koto Ka! Seikimatsu",
        .boxartPath = "assets/images/systems/nes/1999-hore-mita-koto-ka-seikimatsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "2010: Street Fighter",
        .boxartPath = "assets/images/systems/nes/2010-street-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "3-D WorldRunner",
        .boxartPath = "assets/images/systems/nes/3-d-worldrunner.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "4 Nin Uchi Mahjong",
        .boxartPath = "assets/images/systems/nes/4-nin-uchi-mahjong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "6 in 1 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/6-in-1-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "720°",
        .boxartPath = "assets/images/systems/nes/720.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "8 Eyes",
        .boxartPath = "assets/images/systems/nes/8-eyes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "'89 Dennō Kyūsei Uranai",
        .boxartPath = "assets/images/systems/nes/89-denn-ky-sei-uranai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "A Boy and His Blob: Trouble on Blobolonia",
        .boxartPath = "assets/images/systems/nes/a-boy-and-his-blob-trouble-on-blobolonia.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "A Nightmare on Elm Street",
        .boxartPath = "assets/images/systems/nes/a-nightmare-on-elm-street.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "A Ressha de Ikō",
        .boxartPath = "assets/images/systems/nes/a-ressha-de-ik.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "A Week of Garfield",
        .boxartPath = "assets/images/systems/nes/a-week-of-garfield.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aa Yakyū Jinsei Icchokusen",
        .boxartPath = "assets/images/systems/nes/aa-yaky-jinsei-icchokusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Abadox",
        .boxartPath = "assets/images/systems/nes/abadox-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Abarenbou Tengu",
        .boxartPath = "assets/images/systems/nes/abarenbou-tengu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ace Harding: Lost in Las Vegas",
        .boxartPath = "assets/images/systems/nes/ace-harding-lost-in-las-vegas.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aces: Iron Eagle III",
        .boxartPath = "assets/images/systems/nes/aces-iron-eagle-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Action 52 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/action-52-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Action in New York",
        .boxartPath = "assets/images/systems/nes/action-in-new-york.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adan y Eva",
        .boxartPath = "assets/images/systems/nes/adan-y-eva.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Addams Family",
        .boxartPath = "assets/images/systems/nes/the-addams-family.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Addams Family: Pugsley's Scavenger Hunt",
        .boxartPath = "assets/images/systems/nes/the-addams-family-pugsley-s-scavenger-hunt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Advanced Dungeons & Dragons: Dragons of Flame",
        .boxartPath = "assets/images/systems/nes/advanced-dungeons-dragons-dragons-of-flame.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Advanced Dungeons & Dragons: DragonStrike",
        .boxartPath = "assets/images/systems/nes/advanced-dungeons-dragons-dragonstrike.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Advanced Dungeons & Dragons: Heroes of the Lance",
        .boxartPath = "assets/images/systems/nes/advanced-dungeons-dragons-heroes-of-the-lance.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Advanced Dungeons & Dragons: Hillsfar",
        .boxartPath = "assets/images/systems/nes/advanced-dungeons-dragons-hillsfar.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Advanced Dungeons & Dragons: Pool of Radiance",
        .boxartPath = "assets/images/systems/nes/advanced-dungeons-dragons-pool-of-radiance.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventure Island",
        .boxartPath = "assets/images/systems/nes/adventure-island.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventure Island 3",
        .boxartPath = "assets/images/systems/nes/adventure-island-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventure Island Classic",
        .boxartPath = "assets/images/systems/nes/adventure-island-classic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventure Island II",
        .boxartPath = "assets/images/systems/nes/adventure-island-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventure Island Part II",
        .boxartPath = "assets/images/systems/nes/the-adventure-island-part-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures in the Magic Kingdom",
        .boxartPath = "assets/images/systems/nes/adventures-in-the-magic-kingdom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Bayou Billy",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-bayou-billy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Captain Comic (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-captain-comic-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Dino Riki",
        .boxartPath = "assets/images/systems/nes/adventures-of-dino-riki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Dr. Franken",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-dr-franken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Gilligan's Island",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-gilligan-s-island.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Lolo",
        .boxartPath = "assets/images/systems/nes/adventures-of-lolo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Lolo 2",
        .boxartPath = "assets/images/systems/nes/adventures-of-lolo-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Lolo 3",
        .boxartPath = "assets/images/systems/nes/adventures-of-lolo-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Lolo II",
        .boxartPath = "assets/images/systems/nes/adventures-of-lolo-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Rad Gravity",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-rad-gravity.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Adventures of Rocky and Bullwinkle and Friends",
        .boxartPath = "assets/images/systems/nes/the-adventures-of-rocky-and-bullwinkle-and-friends.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Adventures of Tom Sawyer",
        .boxartPath = "assets/images/systems/nes/adventures-of-tom-sawyer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "After Burner (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/after-burner-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "After Burner II",
        .boxartPath = "assets/images/systems/nes/after-burner-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ai Sensei no Oshiete: Watashi no Hoshi",
        .boxartPath = "assets/images/systems/nes/ai-sensei-no-oshiete-watashi-no-hoshi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aigīna no Yogen: Barubarūku no Densetsu Yori",
        .boxartPath = "assets/images/systems/nes/aig-na-no-yogen-barubar-ku-no-densetsu-yori.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Air Fortress",
        .boxartPath = "assets/images/systems/nes/air-fortress.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Airball",
        .boxartPath = "assets/images/systems/nes/airball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Airwolf",
        .boxartPath = "assets/images/systems/nes/airwolf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akagawa Jirō no Yūrei Ressha",
        .boxartPath = "assets/images/systems/nes/akagawa-jir-no-y-rei-ressha.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akira",
        .boxartPath = "assets/images/systems/nes/akira.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akuma-kun: Makai no Wana",
        .boxartPath = "assets/images/systems/nes/akuma-kun-makai-no-wana.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akuma no Shoutaijou",
        .boxartPath = "assets/images/systems/nes/akuma-no-shoutaijou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akumajō Densetsu",
        .boxartPath = "assets/images/systems/nes/akumaj-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akumajō Dracula",
        .boxartPath = "assets/images/systems/nes/akumaj-dracula.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Akumajō Special: Boku Dracula-kun",
        .boxartPath = "assets/images/systems/nes/akumaj-special-boku-dracula-kun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Al Unser Jr.'s Turbo Racing",
        .boxartPath = "assets/images/systems/nes/al-unser-jr-s-turbo-racing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aladdin",
        .boxartPath = "assets/images/systems/nes/aladdin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alex DeMeo's Race America",
        .boxartPath = "assets/images/systems/nes/alex-demeo-s-race-america.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alfred Chicken",
        .boxartPath = "assets/images/systems/nes/alfred-chicken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alien Syndrome (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/alien-syndrome-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Alien³",
        .boxartPath = "assets/images/systems/nes/alien.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "All-Pro Basketball",
        .boxartPath = "assets/images/systems/nes/all-pro-basketball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Amagon",
        .boxartPath = "assets/images/systems/nes/amagon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "America Daitōryō Senkyo",
        .boxartPath = "assets/images/systems/nes/america-dait-ry-senkyo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "America Ōdan Ultra Quiz: Shijō Saidai no Tatakai",
        .boxartPath = "assets/images/systems/nes/america-dan-ultra-quiz-shij-saidai-no-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "American Dream",
        .boxartPath = "assets/images/systems/nes/american-dream.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "American Football: Touch Down Fever",
        .boxartPath = "assets/images/systems/nes/american-football-touch-down-fever.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "American Gladiators",
        .boxartPath = "assets/images/systems/nes/american-gladiators.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ankoku Shinwa: Yamato Takeru Densetsu",
        .boxartPath = "assets/images/systems/nes/ankoku-shinwa-yamato-takeru-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Anticipation",
        .boxartPath = "assets/images/systems/nes/anticipation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aoki Ookami to Shiroki Meshika: Genghis Khan",
        .boxartPath = "assets/images/systems/nes/aoki-ookami-to-shiroki-meshika-genghis-khan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aoki Ōkami to Shiroki Mejika: Genchō Hishi",
        .boxartPath = "assets/images/systems/nes/aoki-kami-to-shiroki-mejika-gench-hishi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arabian Dream Scheherazade",
        .boxartPath = "assets/images/systems/nes/arabian-dream-scheherazade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arch Rivals: A Basket Brawl!",
        .boxartPath = "assets/images/systems/nes/arch-rivals-a-basket-brawl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Archon",
        .boxartPath = "assets/images/systems/nes/archon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arctic",
        .boxartPath = "assets/images/systems/nes/arctic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Argos no Senshi: Hachamecha Daishingeki",
        .boxartPath = "assets/images/systems/nes/argos-no-senshi-hachamecha-daishingeki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Argus",
        .boxartPath = "assets/images/systems/nes/argus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arkanoid",
        .boxartPath = "assets/images/systems/nes/arkanoid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arkanoid II",
        .boxartPath = "assets/images/systems/nes/arkanoid-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Arkista's Ring",
        .boxartPath = "assets/images/systems/nes/arkista-s-ring.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Armadillo",
        .boxartPath = "assets/images/systems/nes/armadillo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Artelius",
        .boxartPath = "assets/images/systems/nes/artelius.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Asmik-kun Land",
        .boxartPath = "assets/images/systems/nes/asmik-kun-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "ASO: Armored Scrum Object",
        .boxartPath = "assets/images/systems/nes/aso-armored-scrum-object.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Asterix",
        .boxartPath = "assets/images/systems/nes/asterix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Astro Fang: Super Machine",
        .boxartPath = "assets/images/systems/nes/astro-fang-super-machine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Astro Robo Sasa",
        .boxartPath = "assets/images/systems/nes/astro-robo-sasa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Astyanax",
        .boxartPath = "assets/images/systems/nes/astyanax.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Athena",
        .boxartPath = "assets/images/systems/nes/athena.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Athletic World",
        .boxartPath = "assets/images/systems/nes/athletic-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Atlantis no Nazo",
        .boxartPath = "assets/images/systems/nes/atlantis-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Attack Animal Gakuen",
        .boxartPath = "assets/images/systems/nes/attack-animal-gakuen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Attack of the Killer Tomatoes",
        .boxartPath = "assets/images/systems/nes/attack-of-the-killer-tomatoes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "B-Wings",
        .boxartPath = "assets/images/systems/nes/b-wings.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Babel no Tō",
        .boxartPath = "assets/images/systems/nes/babel-no-t.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baby Boomer (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/baby-boomer-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Back to the Future",
        .boxartPath = "assets/images/systems/nes/back-to-the-future.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Back to the Future II & III",
        .boxartPath = "assets/images/systems/nes/back-to-the-future-ii-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bad Dudes",
        .boxartPath = "assets/images/systems/nes/bad-dudes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bad Dudes vs. Dragon Ninja",
        .boxartPath = "assets/images/systems/nes/bad-dudes-vs-dragon-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bad News Baseball",
        .boxartPath = "assets/images/systems/nes/bad-news-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bad Street Brawler",
        .boxartPath = "assets/images/systems/nes/bad-street-brawler.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baken Hisshō Gaku: Gate In",
        .boxartPath = "assets/images/systems/nes/baken-hissh-gaku-gate-in.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bakushō‼ Ai no Gekijō",
        .boxartPath = "assets/images/systems/nes/bakush-ai-no-gekij.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bakushō‼ Jinsei Gekijō",
        .boxartPath = "assets/images/systems/nes/bakush-jinsei-gekij.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bakushō‼ Jinsei Gekijō 2",
        .boxartPath = "assets/images/systems/nes/bakush-jinsei-gekij-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bakushō‼ Jinsei Gekijō 3",
        .boxartPath = "assets/images/systems/nes/bakush-jinsei-gekij-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bakushō! Star Monomane Shitennō",
        .boxartPath = "assets/images/systems/nes/bakush-star-monomane-shitenn.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ballblazer",
        .boxartPath = "assets/images/systems/nes/ballblazer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Balloon Fight",
        .boxartPath = "assets/images/systems/nes/balloon-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baltron",
        .boxartPath = "assets/images/systems/nes/baltron.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Banana",
        .boxartPath = "assets/images/systems/nes/banana.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Banana Prince",
        .boxartPath = "assets/images/systems/nes/banana-prince.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bananan Ouji no Daibouken",
        .boxartPath = "assets/images/systems/nes/bananan-ouji-no-daibouken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bandai Golf: Challenge Pebble Beach",
        .boxartPath = "assets/images/systems/nes/bandai-golf-challenge-pebble-beach.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bandit Kings of Ancient China",
        .boxartPath = "assets/images/systems/nes/bandit-kings-of-ancient-china.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Barbie",
        .boxartPath = "assets/images/systems/nes/barbie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Barcode World",
        .boxartPath = "assets/images/systems/nes/barcode-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Bard's Tale",
        .boxartPath = "assets/images/systems/nes/the-bard-s-tale.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Bard's Tale II: The Destiny Knight",
        .boxartPath = "assets/images/systems/nes/the-bard-s-tale-ii-the-destiny-knight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Barker Bill's Trick Shooting",
        .boxartPath = "assets/images/systems/nes/barker-bill-s-trick-shooting.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball",
        .boxartPath = "assets/images/systems/nes/baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball Fighter",
        .boxartPath = "assets/images/systems/nes/baseball-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball Simulator 1.000",
        .boxartPath = "assets/images/systems/nes/baseball-simulator-1-000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball Star: Mezase Sankanou",
        .boxartPath = "assets/images/systems/nes/baseball-star-mezase-sankanou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball Stars",
        .boxartPath = "assets/images/systems/nes/baseball-stars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Baseball Stars II",
        .boxartPath = "assets/images/systems/nes/baseball-stars-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bases Loaded",
        .boxartPath = "assets/images/systems/nes/bases-loaded.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bases Loaded 3",
        .boxartPath = "assets/images/systems/nes/bases-loaded-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bases Loaded 4",
        .boxartPath = "assets/images/systems/nes/bases-loaded-4.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bases Loaded II: Second Season",
        .boxartPath = "assets/images/systems/nes/bases-loaded-ii-second-season.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Batman",
        .boxartPath = "assets/images/systems/nes/batman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Batman Returns",
        .boxartPath = "assets/images/systems/nes/batman-returns.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bats & Terry",
        .boxartPath = "assets/images/systems/nes/bats-terry.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Baseball",
        .boxartPath = "assets/images/systems/nes/battle-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Chess",
        .boxartPath = "assets/images/systems/nes/battle-chess.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle City",
        .boxartPath = "assets/images/systems/nes/battle-city.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Fleet",
        .boxartPath = "assets/images/systems/nes/battle-fleet.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Formula",
        .boxartPath = "assets/images/systems/nes/battle-formula.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Battle of Olympus",
        .boxartPath = "assets/images/systems/nes/the-battle-of-olympus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Rush: Build Up Robot Tournament",
        .boxartPath = "assets/images/systems/nes/battle-rush-build-up-robot-tournament.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Stadium: Senbatsu Pro Yakyū",
        .boxartPath = "assets/images/systems/nes/battle-stadium-senbatsu-pro-yaky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battle Storm",
        .boxartPath = "assets/images/systems/nes/battle-storm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battleship",
        .boxartPath = "assets/images/systems/nes/battleship.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Be-Bop High School: Kōkōsei Gokuraku Densetsu",
        .boxartPath = "assets/images/systems/nes/be-bop-high-school-k-k-sei-gokuraku-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bee 52 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/bee-52-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Beetlejuice",
        .boxartPath = "assets/images/systems/nes/beetlejuice.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best Keiba Derby Stallion",
        .boxartPath = "assets/images/systems/nes/best-keiba-derby-stallion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best of the Best: Championship Karate",
        .boxartPath = "assets/images/systems/nes/best-of-the-best-championship-karate.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best Play Pro Yakyū",
        .boxartPath = "assets/images/systems/nes/best-play-pro-yaky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best Play Pro Yakyū '90",
        .boxartPath = "assets/images/systems/nes/best-play-pro-yaky-90.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best Play Pro Yakyū II",
        .boxartPath = "assets/images/systems/nes/best-play-pro-yaky-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Best Play Pro Yakyū Special",
        .boxartPath = "assets/images/systems/nes/best-play-pro-yaky-special.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bible Adventures (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/bible-adventures-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bible Buffet (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/bible-buffet-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Big Nose Freaks Out (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/big-nose-freaks-out-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Big Nose the Caveman (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/big-nose-the-caveman-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bigfoot",
        .boxartPath = "assets/images/systems/nes/bigfoot.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bikkuri Nekketsu Shin Kiroku! Harukanaru Kin Medal",
        .boxartPath = "assets/images/systems/nes/bikkuri-nekketsu-shin-kiroku-harukanaru-kin-medal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bikkuriman World: Gekitō Sei Senshi",
        .boxartPath = "assets/images/systems/nes/bikkuriman-world-gekit-sei-senshi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bill & Ted's Excellent Video Game Adventure",
        .boxartPath = "assets/images/systems/nes/bill-ted-s-excellent-video-game-adventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bill Elliott's NASCAR Challenge",
        .boxartPath = "assets/images/systems/nes/bill-elliott-s-nascar-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Binary Land",
        .boxartPath = "assets/images/systems/nes/binary-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bio Force Ape",
        .boxartPath = "assets/images/systems/nes/bio-force-ape.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bio Miracle Bokutte Upa",
        .boxartPath = "assets/images/systems/nes/bio-miracle-bokutte-upa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bio Senshi Dan: Increaser to no Tatakai",
        .boxartPath = "assets/images/systems/nes/bio-senshi-dan-increaser-to-no-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bionic Commando",
        .boxartPath = "assets/images/systems/nes/bionic-commando.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bird Week",
        .boxartPath = "assets/images/systems/nes/bird-week.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Black Bass",
        .boxartPath = "assets/images/systems/nes/the-black-bass.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Black Bass II",
        .boxartPath = "assets/images/systems/nes/the-black-bass-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Black Bass USA",
        .boxartPath = "assets/images/systems/nes/the-black-bass-usa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blackjack (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/blackjack-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blades of Steel",
        .boxartPath = "assets/images/systems/nes/blades-of-steel.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blaster Master",
        .boxartPath = "assets/images/systems/nes/blaster-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Block Set",
        .boxartPath = "assets/images/systems/nes/block-set.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blockout",
        .boxartPath = "assets/images/systems/nes/blockout.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blodia Land: Puzzle Quest",
        .boxartPath = "assets/images/systems/nes/blodia-land-puzzle-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bloody Warriors: Shango no Gyakushū",
        .boxartPath = "assets/images/systems/nes/bloody-warriors-shango-no-gyakush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Blue Marlin",
        .boxartPath = "assets/images/systems/nes/the-blue-marlin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blue Shadow",
        .boxartPath = "assets/images/systems/nes/blue-shadow.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Blues Brothers",
        .boxartPath = "assets/images/systems/nes/the-blues-brothers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bo Jackson Baseball",
        .boxartPath = "assets/images/systems/nes/bo-jackson-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bokosuka Wars",
        .boxartPath = "assets/images/systems/nes/bokosuka-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomber King",
        .boxartPath = "assets/images/systems/nes/bomber-king.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman",
        .boxartPath = "assets/images/systems/nes/bomberman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman II",
        .boxartPath = "assets/images/systems/nes/bomberman-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bonk's Adventure",
        .boxartPath = "assets/images/systems/nes/bonk-s-adventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Booby Kids",
        .boxartPath = "assets/images/systems/nes/booby-kids.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Boulder Dash",
        .boxartPath = "assets/images/systems/nes/boulder-dash.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bram Stoker's Dracula",
        .boxartPath = "assets/images/systems/nes/bram-stoker-s-dracula.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Break Time: The National Pool Tour",
        .boxartPath = "assets/images/systems/nes/break-time-the-national-pool-tour.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "BreakThru",
        .boxartPath = "assets/images/systems/nes/breakthru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bubble Bath Babes (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/bubble-bath-babes-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bubble Bobble",
        .boxartPath = "assets/images/systems/nes/bubble-bobble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bubble Bobble 2",
        .boxartPath = "assets/images/systems/nes/bubble-bobble-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bubble Bobble Part 2",
        .boxartPath = "assets/images/systems/nes/bubble-bobble-part-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bucky O'Hare",
        .boxartPath = "assets/images/systems/nes/bucky-o-hare.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Buggy Popper",
        .boxartPath = "assets/images/systems/nes/buggy-popper.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Bugs Bunny Birthday Blowout",
        .boxartPath = "assets/images/systems/nes/the-bugs-bunny-birthday-blowout.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Bugs Bunny Blowout",
        .boxartPath = "assets/images/systems/nes/the-bugs-bunny-blowout.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Bugs Bunny Crazy Castle",
        .boxartPath = "assets/images/systems/nes/the-bugs-bunny-crazy-castle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bump 'n' Jump",
        .boxartPath = "assets/images/systems/nes/bump-n-jump.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bungeling Bay",
        .boxartPath = "assets/images/systems/nes/bungeling-bay.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Burai Fighter",
        .boxartPath = "assets/images/systems/nes/burai-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "BurgerTime",
        .boxartPath = "assets/images/systems/nes/burgertime.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Business Wars",
        .boxartPath = "assets/images/systems/nes/business-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Buster Bros.",
        .boxartPath = "assets/images/systems/nes/buster-bros.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cabal",
        .boxartPath = "assets/images/systems/nes/cabal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cadillac",
        .boxartPath = "assets/images/systems/nes/cadillac.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Caesars Palace",
        .boxartPath = "assets/images/systems/nes/caesars-palace.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "California Games",
        .boxartPath = "assets/images/systems/nes/california-games.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The California Raisins: The Grape Escape",
        .boxartPath = "assets/images/systems/nes/the-california-raisins-the-grape-escape.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Capcom Barcelona '92",
        .boxartPath = "assets/images/systems/nes/capcom-barcelona-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Capcom's Gold Medal Challenge '92",
        .boxartPath = "assets/images/systems/nes/capcom-s-gold-medal-challenge-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain America and the Avengers",
        .boxartPath = "assets/images/systems/nes/captain-america-and-the-avengers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Ed",
        .boxartPath = "assets/images/systems/nes/captain-ed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Planet and the Planeteers",
        .boxartPath = "assets/images/systems/nes/captain-planet-and-the-planeteers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Saver",
        .boxartPath = "assets/images/systems/nes/captain-saver.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Silver",
        .boxartPath = "assets/images/systems/nes/captain-silver.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Skyhawk",
        .boxartPath = "assets/images/systems/nes/captain-skyhawk.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Tsubasa",
        .boxartPath = "assets/images/systems/nes/captain-tsubasa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Captain Tsubasa II: Super Striker",
        .boxartPath = "assets/images/systems/nes/captain-tsubasa-ii-super-striker.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Casino Derby & Super Bingo",
        .boxartPath = "assets/images/systems/nes/casino-derby-super-bingo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Casino Kid",
        .boxartPath = "assets/images/systems/nes/casino-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Casino Kid II",
        .boxartPath = "assets/images/systems/nes/casino-kid-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castelian",
        .boxartPath = "assets/images/systems/nes/castelian.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castle Excellent",
        .boxartPath = "assets/images/systems/nes/castle-excellent.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castle of Deceit (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/castle-of-deceit-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castle of Dragon",
        .boxartPath = "assets/images/systems/nes/castle-of-dragon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castle Quest",
        .boxartPath = "assets/images/systems/nes/castle-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlequest",
        .boxartPath = "assets/images/systems/nes/castlequest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Caveman Games",
        .boxartPath = "assets/images/systems/nes/caveman-games.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chack'n Pop",
        .boxartPath = "assets/images/systems/nes/chack-n-pop.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Challenge of the Dragon (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/challenge-of-the-dragon-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Challenger",
        .boxartPath = "assets/images/systems/nes/challenger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Championship Bowling",
        .boxartPath = "assets/images/systems/nes/championship-bowling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Championship Lode Runner",
        .boxartPath = "assets/images/systems/nes/championship-lode-runner.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Championship Pool",
        .boxartPath = "assets/images/systems/nes/championship-pool.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Championship Rally",
        .boxartPath = "assets/images/systems/nes/championship-rally.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chaos World",
        .boxartPath = "assets/images/systems/nes/chaos-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chase H.Q.",
        .boxartPath = "assets/images/systems/nes/chase-h-q.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Chessmaster",
        .boxartPath = "assets/images/systems/nes/the-chessmaster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chester Field: Ankoku Shin e no Chōsen",
        .boxartPath = "assets/images/systems/nes/chester-field-ankoku-shin-e-no-ch-sen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chibi Maruko-chan: Uki Uki Shopping",
        .boxartPath = "assets/images/systems/nes/chibi-maruko-chan-uki-uki-shopping.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chiisana Obake Acchi, Kocchi, Socchi",
        .boxartPath = "assets/images/systems/nes/chiisana-obake-acchi-kocchi-socchi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chiki Chiki Machine Mou Race",
        .boxartPath = "assets/images/systems/nes/chiki-chiki-machine-mou-race.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chiller (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/chiller-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chip 'n Dale Rescue Rangers",
        .boxartPath = "assets/images/systems/nes/chip-n-dale-rescue-rangers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chip 'n Dale Rescue Rangers 2",
        .boxartPath = "assets/images/systems/nes/chip-n-dale-rescue-rangers-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chip to Dale no Daisakusen",
        .boxartPath = "assets/images/systems/nes/chip-to-dale-no-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chip to Dale no Daisakusen 2",
        .boxartPath = "assets/images/systems/nes/chip-to-dale-no-daisakusen-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chip's Challenge",
        .boxartPath = "assets/images/systems/nes/chip-s-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chitei Senkou Bazolder",
        .boxartPath = "assets/images/systems/nes/chitei-senkou-bazolder.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chiyonofuji no Ōichō",
        .boxartPath = "assets/images/systems/nes/chiyonofuji-no-ich.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Choplifter",
        .boxartPath = "assets/images/systems/nes/choplifter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chō Jikū Yōsai Macross",
        .boxartPath = "assets/images/systems/nes/ch-jik-y-sai-macross.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chou-Wakusei Senki MetaFight",
        .boxartPath = "assets/images/systems/nes/chou-wakusei-senki-metafight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chōjin Sentai Jetman",
        .boxartPath = "assets/images/systems/nes/ch-jin-sentai-jetman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Choujin Ultra Baseball",
        .boxartPath = "assets/images/systems/nes/choujin-ultra-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Choujinrou Senki Warwolf",
        .boxartPath = "assets/images/systems/nes/choujinrou-senki-warwolf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chubby Cherub",
        .boxartPath = "assets/images/systems/nes/chubby-cherub.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chuck Yeager's Fighter Combat",
        .boxartPath = "assets/images/systems/nes/chuck-yeager-s-fighter-combat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chuka Taisen",
        .boxartPath = "assets/images/systems/nes/chuka-taisen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chūgoku Janshi Story: Tonpū",
        .boxartPath = "assets/images/systems/nes/ch-goku-janshi-story-tonp.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chūgoku Senseijutsu",
        .boxartPath = "assets/images/systems/nes/ch-goku-senseijutsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Circus Caper",
        .boxartPath = "assets/images/systems/nes/circus-caper.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Circus Charlie",
        .boxartPath = "assets/images/systems/nes/circus-charlie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "City Adventure Touch: Mystery of Triangle",
        .boxartPath = "assets/images/systems/nes/city-adventure-touch-mystery-of-triangle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "City Connection",
        .boxartPath = "assets/images/systems/nes/city-connection.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Clash at Demonhead",
        .boxartPath = "assets/images/systems/nes/clash-at-demonhead.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Classic Concentration",
        .boxartPath = "assets/images/systems/nes/classic-concentration.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cliffhanger",
        .boxartPath = "assets/images/systems/nes/cliffhanger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Clu Clu Land",
        .boxartPath = "assets/images/systems/nes/clu-clu-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cobra Command",
        .boxartPath = "assets/images/systems/nes/cobra-command.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cobra Triangle",
        .boxartPath = "assets/images/systems/nes/cobra-triangle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cocoron",
        .boxartPath = "assets/images/systems/nes/cocoron.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Code Name: Viper",
        .boxartPath = "assets/images/systems/nes/code-name-viper.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Color a Dinosaur",
        .boxartPath = "assets/images/systems/nes/color-a-dinosaur.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Columbus: Ōgon no Yoake",
        .boxartPath = "assets/images/systems/nes/columbus-gon-no-yoake.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Commando",
        .boxartPath = "assets/images/systems/nes/commando.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Conan: The Mysteries of Time",
        .boxartPath = "assets/images/systems/nes/conan-the-mysteries-of-time.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Conflict",
        .boxartPath = "assets/images/systems/nes/conflict.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Conquest of the Crystal Palace",
        .boxartPath = "assets/images/systems/nes/conquest-of-the-crystal-palace.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Contra Force",
        .boxartPath = "assets/images/systems/nes/contra-force.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cool World",
        .boxartPath = "assets/images/systems/nes/cool-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Corvette ZR-1 Challenge",
        .boxartPath = "assets/images/systems/nes/corvette-zr-1-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cosmic Epsilon",
        .boxartPath = "assets/images/systems/nes/cosmic-epsilon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cosmic Spacehead",
        .boxartPath = "assets/images/systems/nes/cosmic-spacehead.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cosmic Wars",
        .boxartPath = "assets/images/systems/nes/cosmic-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cosmo Genesis",
        .boxartPath = "assets/images/systems/nes/cosmo-genesis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cosmo Police Galivan",
        .boxartPath = "assets/images/systems/nes/cosmo-police-galivan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cowboy Kid",
        .boxartPath = "assets/images/systems/nes/cowboy-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crackout",
        .boxartPath = "assets/images/systems/nes/crackout.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crash 'n' the Boys: Street Challenge",
        .boxartPath = "assets/images/systems/nes/crash-n-the-boys-street-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crayon Shin-chan: Ora to Poi Poi",
        .boxartPath = "assets/images/systems/nes/crayon-shin-chan-ora-to-poi-poi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crazy Climber",
        .boxartPath = "assets/images/systems/nes/crazy-climber.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crisis Force",
        .boxartPath = "assets/images/systems/nes/crisis-force.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "CrossFire",
        .boxartPath = "assets/images/systems/nes/crossfire.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crystal Mines (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/crystal-mines-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Crystalis",
        .boxartPath = "assets/images/systems/nes/crystalis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cyber Stadium Series: Base Wars",
        .boxartPath = "assets/images/systems/nes/cyber-stadium-series-base-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cyberball",
        .boxartPath = "assets/images/systems/nes/cyberball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cybernoid: The Fighting Machine",
        .boxartPath = "assets/images/systems/nes/cybernoid-the-fighting-machine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cycle Race: Road Man",
        .boxartPath = "assets/images/systems/nes/cycle-race-road-man.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dai-2-Ji Super Robot Taisen",
        .boxartPath = "assets/images/systems/nes/dai-2-ji-super-robot-taisen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dai Meiro: Meikyū no Tatsujin",
        .boxartPath = "assets/images/systems/nes/dai-meiro-meiky-no-tatsujin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daikaijyu Deburasu",
        .boxartPath = "assets/images/systems/nes/daikaijyu-deburasu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daikoukai Jidai",
        .boxartPath = "assets/images/systems/nes/daikoukai-jidai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daiku no Gen-San",
        .boxartPath = "assets/images/systems/nes/daiku-no-gen-san.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daiku no Gen-san 2: Akage no Dan no Gyakushō",
        .boxartPath = "assets/images/systems/nes/daiku-no-gen-san-2-akage-no-dan-no-gyakush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daisenryaku",
        .boxartPath = "assets/images/systems/nes/daisenryaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Daiva Story 6: Imperial of Nirsartia",
        .boxartPath = "assets/images/systems/nes/daiva-story-6-imperial-of-nirsartia.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dance Aerobics",
        .boxartPath = "assets/images/systems/nes/dance-aerobics.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Danny Sullivan's Indy Heat",
        .boxartPath = "assets/images/systems/nes/danny-sullivan-s-indy-heat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dark Lord",
        .boxartPath = "assets/images/systems/nes/dark-lord.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Darkman",
        .boxartPath = "assets/images/systems/nes/darkman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Darkwing Duck",
        .boxartPath = "assets/images/systems/nes/darkwing-duck.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dash Galaxy in the Alien Asylum",
        .boxartPath = "assets/images/systems/nes/dash-galaxy-in-the-alien-asylum.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dash Yarou",
        .boxartPath = "assets/images/systems/nes/dash-yarou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Datsugoku: Prisoners of War",
        .boxartPath = "assets/images/systems/nes/datsugoku-prisoners-of-war.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Day Dreamin' Davey",
        .boxartPath = "assets/images/systems/nes/day-dreamin-davey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Days of Thunder",
        .boxartPath = "assets/images/systems/nes/days-of-thunder.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "De-Block",
        .boxartPath = "assets/images/systems/nes/de-block.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deadly Towers",
        .boxartPath = "assets/images/systems/nes/deadly-towers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Death Race (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/death-race-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deathbots (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/deathbots-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deep Dungeon III: Yūshi he no Tabi",
        .boxartPath = "assets/images/systems/nes/deep-dungeon-iii-y-shi-he-no-tabi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deep Dungeon IV: Kuro no Yōjutsushi",
        .boxartPath = "assets/images/systems/nes/deep-dungeon-iv-kuro-no-y-jutsushi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Defender II",
        .boxartPath = "assets/images/systems/nes/defender-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Defender of the Crown",
        .boxartPath = "assets/images/systems/nes/defender-of-the-crown.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Defenders of Dynatron City",
        .boxartPath = "assets/images/systems/nes/defenders-of-dynatron-city.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Déjà Vu",
        .boxartPath = "assets/images/systems/nes/d-j-vu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deja Vu: Akumu wa Hotouni Yatte Raita",
        .boxartPath = "assets/images/systems/nes/deja-vu-akumu-wa-hotouni-yatte-raita.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Demon Sword",
        .boxartPath = "assets/images/systems/nes/demon-sword.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dengeki Big Bang!",
        .boxartPath = "assets/images/systems/nes/dengeki-big-bang.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Densetsu no Kishi Elrond",
        .boxartPath = "assets/images/systems/nes/densetsu-no-kishi-elrond.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Derby Stallion Zengokuban",
        .boxartPath = "assets/images/systems/nes/derby-stallion-zengokuban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Desert Commander",
        .boxartPath = "assets/images/systems/nes/desert-commander.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Destination Earthstar",
        .boxartPath = "assets/images/systems/nes/destination-earthstar.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Destiny of an Emperor",
        .boxartPath = "assets/images/systems/nes/destiny-of-an-emperor.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Devil Man",
        .boxartPath = "assets/images/systems/nes/devil-man.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Devil World",
        .boxartPath = "assets/images/systems/nes/devil-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dezaemon",
        .boxartPath = "assets/images/systems/nes/dezaemon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dick Tracy",
        .boxartPath = "assets/images/systems/nes/dick-tracy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Die Hard",
        .boxartPath = "assets/images/systems/nes/die-hard.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dig Dug",
        .boxartPath = "assets/images/systems/nes/dig-dug.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dig Dug II",
        .boxartPath = "assets/images/systems/nes/dig-dug-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dig Dug II: Trouble in Paradise",
        .boxartPath = "assets/images/systems/nes/dig-dug-ii-trouble-in-paradise.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Digger T. Rock: The Legend of the Lost City",
        .boxartPath = "assets/images/systems/nes/digger-t-rock-the-legend-of-the-lost-city.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Digital Devil Story: Megami Tensei",
        .boxartPath = "assets/images/systems/nes/digital-devil-story-megami-tensei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Digital Devil Story: Megami Tensei II",
        .boxartPath = "assets/images/systems/nes/digital-devil-story-megami-tensei-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dino Hockey",
        .boxartPath = "assets/images/systems/nes/dino-hockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dirty Harry",
        .boxartPath = "assets/images/systems/nes/dirty-harry.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dizzy the Adventurer (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/dizzy-the-adventurer-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Doki! Doki! Yuuenchi: Crazy Land Daisakusen",
        .boxartPath = "assets/images/systems/nes/doki-doki-yuuenchi-crazy-land-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dokuganryu Masamune",
        .boxartPath = "assets/images/systems/nes/dokuganryu-masamune.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Don Doko Don",
        .boxartPath = "assets/images/systems/nes/don-doko-don.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Don Doko Don 2",
        .boxartPath = "assets/images/systems/nes/don-doko-don-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donald Duck",
        .boxartPath = "assets/images/systems/nes/donald-duck.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donald Land",
        .boxartPath = "assets/images/systems/nes/donald-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong",
        .boxartPath = "assets/images/systems/nes/donkey-kong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong 3",
        .boxartPath = "assets/images/systems/nes/donkey-kong-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Jr.",
        .boxartPath = "assets/images/systems/nes/donkey-kong-jr.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Jr. Math",
        .boxartPath = "assets/images/systems/nes/donkey-kong-jr-math.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong no Ongaku Asobi",
        .boxartPath = "assets/images/systems/nes/donkey-kong-no-ongaku-asobi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Door Door",
        .boxartPath = "assets/images/systems/nes/door-door.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Doraemon",
        .boxartPath = "assets/images/systems/nes/doraemon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Doraemon: Giga Zombie no Gyakushū",
        .boxartPath = "assets/images/systems/nes/doraemon-giga-zombie-no-gyakush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dare",
        .boxartPath = "assets/images/systems/nes/double-dare.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dragon III: The Rosetta Stone",
        .boxartPath = "assets/images/systems/nes/double-dragon-iii-the-rosetta-stone.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Dribble",
        .boxartPath = "assets/images/systems/nes/double-dribble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Moon Densetsu",
        .boxartPath = "assets/images/systems/nes/double-moon-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Double Strike (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/double-strike-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dough Boy",
        .boxartPath = "assets/images/systems/nes/dough-boy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Downtown Nekketsu Kōshinkyoku: Soreyuke Daiundōkai",
        .boxartPath = "assets/images/systems/nes/downtown-nekketsu-k-shinkyoku-soreyuke-daiund-kai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Downtown Nekketsu Monogatari",
        .boxartPath = "assets/images/systems/nes/downtown-nekketsu-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Downtown Special: Kunio-kun no Jidaigeki da yo Zen'in Shūgō!",
        .boxartPath = "assets/images/systems/nes/downtown-special-kunio-kun-no-jidaigeki-da-yo-zen-in-sh-g.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dr. Chaos",
        .boxartPath = "assets/images/systems/nes/dr-chaos.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dr. Jekyll and Mr. Hyde",
        .boxartPath = "assets/images/systems/nes/dr-jekyll-and-mr-hyde.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Drac's Night Out",
        .boxartPath = "assets/images/systems/nes/drac-s-night-out.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball 3: Gokūden",
        .boxartPath = "assets/images/systems/nes/dragon-ball-3-gok-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball: Daimaō Fukkatsu",
        .boxartPath = "assets/images/systems/nes/dragon-ball-daima-fukkatsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball: Le Secret Du Dragon",
        .boxartPath = "assets/images/systems/nes/dragon-ball-le-secret-du-dragon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball: Shenlong no Nazo",
        .boxartPath = "assets/images/systems/nes/dragon-ball-shenlong-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball Z Gaiden: Saiyajin Zetsumetsu Keikaku",
        .boxartPath = "assets/images/systems/nes/dragon-ball-z-gaiden-saiyajin-zetsumetsu-keikaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball Z: Gekitō Tenkaichi Budokai",
        .boxartPath = "assets/images/systems/nes/dragon-ball-z-gekit-tenkaichi-budokai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball Z II: Gekishin Freeza‼",
        .boxartPath = "assets/images/systems/nes/dragon-ball-z-ii-gekishin-freeza.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball Z III: Ressen Jinzōningen",
        .boxartPath = "assets/images/systems/nes/dragon-ball-z-iii-ressen-jinz-ningen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ball Z: Kyōshū! Saiyajin",
        .boxartPath = "assets/images/systems/nes/dragon-ball-z-ky-sh-saiyajin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Buster",
        .boxartPath = "assets/images/systems/nes/dragon-buster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Buster II: Yami no Fūin",
        .boxartPath = "assets/images/systems/nes/dragon-buster-ii-yami-no-f-in.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Fighter",
        .boxartPath = "assets/images/systems/nes/dragon-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Ninja",
        .boxartPath = "assets/images/systems/nes/dragon-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Power",
        .boxartPath = "assets/images/systems/nes/dragon-power.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Quest",
        .boxartPath = "assets/images/systems/nes/dragon-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Quest II: Akuryou no Kamigami",
        .boxartPath = "assets/images/systems/nes/dragon-quest-ii-akuryou-no-kamigami.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Quest III: Soshite Densetsu e...",
        .boxartPath = "assets/images/systems/nes/dragon-quest-iii-soshite-densetsu-e.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Quest IV: Michibikareshi Monotachi",
        .boxartPath = "assets/images/systems/nes/dragon-quest-iv-michibikareshi-monotachi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Scroll: Yomigaerishi Maryū",
        .boxartPath = "assets/images/systems/nes/dragon-scroll-yomigaerishi-mary.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Slayer IV: Drasle Family",
        .boxartPath = "assets/images/systems/nes/dragon-slayer-iv-drasle-family.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Spirit: Aratanaru Densetsu",
        .boxartPath = "assets/images/systems/nes/dragon-spirit-aratanaru-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Spirit: The New Legend",
        .boxartPath = "assets/images/systems/nes/dragon-spirit-the-new-legend.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Unit",
        .boxartPath = "assets/images/systems/nes/dragon-unit.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Warrior",
        .boxartPath = "assets/images/systems/nes/dragon-warrior.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Warrior II",
        .boxartPath = "assets/images/systems/nes/dragon-warrior-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Warrior III",
        .boxartPath = "assets/images/systems/nes/dragon-warrior-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Warrior IV",
        .boxartPath = "assets/images/systems/nes/dragon-warrior-iv.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon Wars",
        .boxartPath = "assets/images/systems/nes/dragon-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dragon's Lair",
        .boxartPath = "assets/images/systems/nes/dragon-s-lair.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dream Master",
        .boxartPath = "assets/images/systems/nes/dream-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dropzone",
        .boxartPath = "assets/images/systems/nes/dropzone.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duck Hunt",
        .boxartPath = "assets/images/systems/nes/duck-hunt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dudes With Attitude (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/dudes-with-attitude-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dungeon & Magic: Swords of Element",
        .boxartPath = "assets/images/systems/nes/dungeon-magic-swords-of-element.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dungeon Kid",
        .boxartPath = "assets/images/systems/nes/dungeon-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dungeon Magic: Sword of the Elements",
        .boxartPath = "assets/images/systems/nes/dungeon-magic-sword-of-the-elements.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dusty Diamond's All-Star Softball",
        .boxartPath = "assets/images/systems/nes/dusty-diamond-s-all-star-softball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dynablaster",
        .boxartPath = "assets/images/systems/nes/dynablaster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dynamite Batman",
        .boxartPath = "assets/images/systems/nes/dynamite-batman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dynamite Bowl",
        .boxartPath = "assets/images/systems/nes/dynamite-bowl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dynowarz: The Destruction of Spondylus",
        .boxartPath = "assets/images/systems/nes/dynowarz-the-destruction-of-spondylus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Eggerland: Meikyū no Fukkatsu",
        .boxartPath = "assets/images/systems/nes/eggerland-meiky-no-fukkatsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Egypt",
        .boxartPath = "assets/images/systems/nes/egypt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Elevator Action",
        .boxartPath = "assets/images/systems/nes/elevator-action.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Eliminator Boat Duel",
        .boxartPath = "assets/images/systems/nes/eliminator-boat-duel.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Elite",
        .boxartPath = "assets/images/systems/nes/elite.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Elnark no Zaihō",
        .boxartPath = "assets/images/systems/nes/elnark-no-zaih.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Elysion",
        .boxartPath = "assets/images/systems/nes/elysion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Emoyan no 10-bai Pro Yakyū",
        .boxartPath = "assets/images/systems/nes/emoyan-no-10-bai-pro-yaky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Erika to Satoru no Yume Bōken",
        .boxartPath = "assets/images/systems/nes/erika-to-satoru-no-yume-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Esper Bōkentai",
        .boxartPath = "assets/images/systems/nes/esper-b-kentai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Esper Dream 2: Aratanaru Tatakai",
        .boxartPath = "assets/images/systems/nes/esper-dream-2-aratanaru-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Exciting Boxing",
        .boxartPath = "assets/images/systems/nes/exciting-boxing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Exciting Rally: World Rally Championship",
        .boxartPath = "assets/images/systems/nes/exciting-rally-world-rally-championship.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Exed Exes",
        .boxartPath = "assets/images/systems/nes/exed-exes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Exerion",
        .boxartPath = "assets/images/systems/nes/exerion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Exodus: Journey to the Promised Land (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/exodus-journey-to-the-promised-land-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-1 Sensation",
        .boxartPath = "assets/images/systems/nes/f-1-sensation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-117A Stealth Fighter",
        .boxartPath = "assets/images/systems/nes/f-117a-stealth-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-15 City War (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/f-15-city-war-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-15 Strike Eagle",
        .boxartPath = "assets/images/systems/nes/f-15-strike-eagle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F1 Circus",
        .boxartPath = "assets/images/systems/nes/f1-circus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F1 Race",
        .boxartPath = "assets/images/systems/nes/f1-race.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Fairyland Story",
        .boxartPath = "assets/images/systems/nes/the-fairyland-story.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Dōbutsu Seitai Zukan! Katte ni Shirokuma: Mori o Sukue no Maki!",
        .boxartPath = "assets/images/systems/nes/famicom-d-butsu-seitai-zukan-katte-ni-shirokuma-mori-o-sukue-no-maki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Igo Nyuumon",
        .boxartPath = "assets/images/systems/nes/famicom-igo-nyuumon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Jump: Hero Retsuden",
        .boxartPath = "assets/images/systems/nes/famicom-jump-hero-retsuden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Jump II: Saikyō no Shichinin",
        .boxartPath = "assets/images/systems/nes/famicom-jump-ii-saiky-no-shichinin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Meijinsen",
        .boxartPath = "assets/images/systems/nes/famicom-meijinsen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Shogi: Ryū-Ō-Sen",
        .boxartPath = "assets/images/systems/nes/famicom-shogi-ry-sen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Wars",
        .boxartPath = "assets/images/systems/nes/famicom-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famicom Yakyū Ban",
        .boxartPath = "assets/images/systems/nes/famicom-yaky-ban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family BASIC V3",
        .boxartPath = "assets/images/systems/nes/family-basic-v3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Block",
        .boxartPath = "assets/images/systems/nes/family-block.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Boxing",
        .boxartPath = "assets/images/systems/nes/family-boxing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Circuit",
        .boxartPath = "assets/images/systems/nes/family-circuit.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Circuit '91",
        .boxartPath = "assets/images/systems/nes/family-circuit-91.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Feud",
        .boxartPath = "assets/images/systems/nes/family-feud.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Jockey",
        .boxartPath = "assets/images/systems/nes/family-jockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Mahjong",
        .boxartPath = "assets/images/systems/nes/family-mahjong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Mahjong II: Shanghai e no Michi",
        .boxartPath = "assets/images/systems/nes/family-mahjong-ii-shanghai-e-no-michi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Pinball",
        .boxartPath = "assets/images/systems/nes/family-pinball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Quiz: 4-nin wa Rival",
        .boxartPath = "assets/images/systems/nes/family-quiz-4-nin-wa-rival.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Tennis",
        .boxartPath = "assets/images/systems/nes/family-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Aerobics Studio",
        .boxartPath = "assets/images/systems/nes/family-trainer-aerobics-studio.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Athletic World",
        .boxartPath = "assets/images/systems/nes/family-trainer-athletic-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Famitre Daiundoukai",
        .boxartPath = "assets/images/systems/nes/family-trainer-famitre-daiundoukai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Fūun! Takeshi Jō 2",
        .boxartPath = "assets/images/systems/nes/family-trainer-f-un-takeshi-j-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Jogging Race",
        .boxartPath = "assets/images/systems/nes/family-trainer-jogging-race.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Manhattan Police",
        .boxartPath = "assets/images/systems/nes/family-trainer-manhattan-police.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Meiro Daisakusen",
        .boxartPath = "assets/images/systems/nes/family-trainer-meiro-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Rai Rai! Kyonshis Baby Kyonshi no Amida Daibōken",
        .boxartPath = "assets/images/systems/nes/family-trainer-rai-rai-kyonshis-baby-kyonshi-no-amida-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Running Stadium",
        .boxartPath = "assets/images/systems/nes/family-trainer-running-stadium.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Family Trainer: Totsugeki! Fūun Takeshi Jō",
        .boxartPath = "assets/images/systems/nes/family-trainer-totsugeki-f-un-takeshi-j.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '89: Kaimaku Ban‼",
        .boxartPath = "assets/images/systems/nes/famista-89-kaimaku-ban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '90",
        .boxartPath = "assets/images/systems/nes/famista-90.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '91",
        .boxartPath = "assets/images/systems/nes/famista-91.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '92",
        .boxartPath = "assets/images/systems/nes/famista-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '93",
        .boxartPath = "assets/images/systems/nes/famista-93.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Famista '94",
        .boxartPath = "assets/images/systems/nes/famista-94.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Fantastic Adventures of Dizzy (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/the-fantastic-adventures-of-dizzy-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fantasy Zone (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/fantasy-zone-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fantasy Zone II: Opa-Opa no Namida",
        .boxartPath = "assets/images/systems/nes/fantasy-zone-ii-opa-opa-no-namida.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Faria: A World of Mystery and Danger!",
        .boxartPath = "assets/images/systems/nes/faria-a-world-of-mystery-and-danger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Faria: Fuuin no Tsurugi",
        .boxartPath = "assets/images/systems/nes/faria-fuuin-no-tsurugi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "FC Genjin: Frekthoropus Computerus",
        .boxartPath = "assets/images/systems/nes/fc-genjin-frekthoropus-computerus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Felix the Cat",
        .boxartPath = "assets/images/systems/nes/felix-the-cat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ferrari Grand Prix Challenge",
        .boxartPath = "assets/images/systems/nes/ferrari-grand-prix-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fester's Quest",
        .boxartPath = "assets/images/systems/nes/fester-s-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Field Combat",
        .boxartPath = "assets/images/systems/nes/field-combat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fighting Golf",
        .boxartPath = "assets/images/systems/nes/fighting-golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fighting Road",
        .boxartPath = "assets/images/systems/nes/fighting-road.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fantasy",
        .boxartPath = "assets/images/systems/nes/final-fantasy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fantasy I & II",
        .boxartPath = "assets/images/systems/nes/final-fantasy-i-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fantasy II",
        .boxartPath = "assets/images/systems/nes/final-fantasy-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fantasy III",
        .boxartPath = "assets/images/systems/nes/final-fantasy-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Lap",
        .boxartPath = "assets/images/systems/nes/final-lap.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Mission",
        .boxartPath = "assets/images/systems/nes/final-mission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fire Emblem: Ankoku Ryū to Hikari no Tsurugi",
        .boxartPath = "assets/images/systems/nes/fire-emblem-ankoku-ry-to-hikari-no-tsurugi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fire Emblem Gaiden",
        .boxartPath = "assets/images/systems/nes/fire-emblem-gaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Firehawk (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/firehawk-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fisher-Price: Firehouse Rescue",
        .boxartPath = "assets/images/systems/nes/fisher-price-firehouse-rescue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fisher-Price: I Can Remember",
        .boxartPath = "assets/images/systems/nes/fisher-price-i-can-remember.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fisher-Price: Perfect Fit",
        .boxartPath = "assets/images/systems/nes/fisher-price-perfect-fit.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fist of the North Star",
        .boxartPath = "assets/images/systems/nes/fist-of-the-north-star.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flappy",
        .boxartPath = "assets/images/systems/nes/flappy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Flash",
        .boxartPath = "assets/images/systems/nes/the-flash.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fleet Commander",
        .boxartPath = "assets/images/systems/nes/fleet-commander.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flight of the Intruder",
        .boxartPath = "assets/images/systems/nes/flight-of-the-intruder.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Flintstones: The Rescue of Dino & Hoppy",
        .boxartPath = "assets/images/systems/nes/the-flintstones-the-rescue-of-dino-hoppy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Flintstones: The Surprise at Dinosaur Peak!",
        .boxartPath = "assets/images/systems/nes/the-flintstones-the-surprise-at-dinosaur-peak.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flipull: An Exciting Cube Game",
        .boxartPath = "assets/images/systems/nes/flipull-an-exciting-cube-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flying Dragon: The Secret Scroll",
        .boxartPath = "assets/images/systems/nes/flying-dragon-the-secret-scroll.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flying Hero",
        .boxartPath = "assets/images/systems/nes/flying-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flying Warriors",
        .boxartPath = "assets/images/systems/nes/flying-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Formation Z",
        .boxartPath = "assets/images/systems/nes/formation-z.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Formula 1 Sensation",
        .boxartPath = "assets/images/systems/nes/formula-1-sensation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Formula One: Built to Win",
        .boxartPath = "assets/images/systems/nes/formula-one-built-to-win.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Four Players' Tennis",
        .boxartPath = "assets/images/systems/nes/four-players-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Frankenstein: The Monster Returns",
        .boxartPath = "assets/images/systems/nes/frankenstein-the-monster-returns.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Freedom Force",
        .boxartPath = "assets/images/systems/nes/freedom-force.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Friday the 13th",
        .boxartPath = "assets/images/systems/nes/friday-the-13th.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Front Line",
        .boxartPath = "assets/images/systems/nes/front-line.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fudo Myouoden",
        .boxartPath = "assets/images/systems/nes/fudo-myouoden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fun House",
        .boxartPath = "assets/images/systems/nes/fun-house.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fushigi na Blobby: Blobania no Kiki",
        .boxartPath = "assets/images/systems/nes/fushigi-na-blobby-blobania-no-kiki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fushigi no Umi Nadia: The Secret of Blue Water",
        .boxartPath = "assets/images/systems/nes/fushigi-no-umi-nadia-the-secret-of-blue-water.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fuzzical Fighter",
        .boxartPath = "assets/images/systems/nes/fuzzical-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "G.I. Joe: A Real American Hero",
        .boxartPath = "assets/images/systems/nes/g-i-joe-a-real-american-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "G.I. Joe: The Atlantis Factor",
        .boxartPath = "assets/images/systems/nes/g-i-joe-the-atlantis-factor.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galactic Crusader (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/galactic-crusader-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galaga",
        .boxartPath = "assets/images/systems/nes/galaga-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galaxian",
        .boxartPath = "assets/images/systems/nes/galaxian.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Galaxy 5000: Racing in the 51st Century",
        .boxartPath = "assets/images/systems/nes/galaxy-5000-racing-in-the-51st-century.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gambler Jiko Chūshinha",
        .boxartPath = "assets/images/systems/nes/gambler-jiko-ch-shinha.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gambler Jiko Chūshinha 2",
        .boxartPath = "assets/images/systems/nes/gambler-jiko-ch-shinha-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Game Party",
        .boxartPath = "assets/images/systems/nes/game-party.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganbare Goemon 2",
        .boxartPath = "assets/images/systems/nes/ganbare-goemon-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganbare Goemon Gaiden 2: Tenka no Zaihō",
        .boxartPath = "assets/images/systems/nes/ganbare-goemon-gaiden-2-tenka-no-zaih.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganbare Goemon Gaiden: Kieta Ōgon Kiseru",
        .boxartPath = "assets/images/systems/nes/ganbare-goemon-gaiden-kieta-gon-kiseru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganbare Goemon! Karakuri Dōchū",
        .boxartPath = "assets/images/systems/nes/ganbare-goemon-karakuri-d-ch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganbare Pennant Race!",
        .boxartPath = "assets/images/systems/nes/ganbare-pennant-race.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ganso Saiyūki: Super Monkey Daibōken",
        .boxartPath = "assets/images/systems/nes/ganso-saiy-ki-super-monkey-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gargoyle's Quest II",
        .boxartPath = "assets/images/systems/nes/gargoyle-s-quest-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gauntlet",
        .boxartPath = "assets/images/systems/nes/gauntlet.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gauntlet II",
        .boxartPath = "assets/images/systems/nes/gauntlet-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gegege no Kitaro: Youkai Daimakyou",
        .boxartPath = "assets/images/systems/nes/gegege-no-kitaro-youkai-daimakyou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "GeGeGe no Kitarō 2: Yōkai Gundan no Chōsen",
        .boxartPath = "assets/images/systems/nes/gegege-no-kitar-2-y-kai-gundan-no-ch-sen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Geimos",
        .boxartPath = "assets/images/systems/nes/geimos.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gekikame Ninja Den",
        .boxartPath = "assets/images/systems/nes/gekikame-ninja-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gekitotsu Yonku Battle",
        .boxartPath = "assets/images/systems/nes/gekitotsu-yonku-battle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gekitou Pro Wrestling‼ Toukon Densetsu",
        .boxartPath = "assets/images/systems/nes/gekitou-pro-wrestling-toukon-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gekitou‼ Stadium",
        .boxartPath = "assets/images/systems/nes/gekitou-stadium.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gemfire",
        .boxartPath = "assets/images/systems/nes/gemfire.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Genghis Khan",
        .boxartPath = "assets/images/systems/nes/genghis-khan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Genpei Tōma Den",
        .boxartPath = "assets/images/systems/nes/genpei-t-ma-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "George Foreman's KO Boxing",
        .boxartPath = "assets/images/systems/nes/george-foreman-s-ko-boxing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Getsu Fūma Den",
        .boxartPath = "assets/images/systems/nes/getsu-f-ma-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghost Lion",
        .boxartPath = "assets/images/systems/nes/ghost-lion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghostbusters",
        .boxartPath = "assets/images/systems/nes/ghostbusters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghostbusters II",
        .boxartPath = "assets/images/systems/nes/ghostbusters-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghosts 'n Goblins",
        .boxartPath = "assets/images/systems/nes/ghosts-n-goblins.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ghoul School",
        .boxartPath = "assets/images/systems/nes/ghoul-school.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gimme a Break: Shijō Saikyō no Quiz Ō Ketteisen",
        .boxartPath = "assets/images/systems/nes/gimme-a-break-shij-saiky-no-quiz-ketteisen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gimme a Break: Shijō Saikyō no Quiz Ō Ketteisen 2",
        .boxartPath = "assets/images/systems/nes/gimme-a-break-shij-saiky-no-quiz-ketteisen-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ginga Eiyū Densetsu",
        .boxartPath = "assets/images/systems/nes/ginga-eiy-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ginga no Sannin",
        .boxartPath = "assets/images/systems/nes/ginga-no-sannin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Glove Pilot",
        .boxartPath = "assets/images/systems/nes/glove-pilot.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gluk the Thunder Warrior",
        .boxartPath = "assets/images/systems/nes/gluk-the-thunder-warrior.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Goal!",
        .boxartPath = "assets/images/systems/nes/goal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Goal! Two",
        .boxartPath = "assets/images/systems/nes/goal-two.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "God Slayer: Haruka Tenkuu no Sonata",
        .boxartPath = "assets/images/systems/nes/god-slayer-haruka-tenkuu-no-sonata.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Godzilla 2: War of the Monsters",
        .boxartPath = "assets/images/systems/nes/godzilla-2-war-of-the-monsters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Godzilla: King of the Monsters",
        .boxartPath = "assets/images/systems/nes/godzilla-king-of-the-monsters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Godzilla: Monster of Monsters!",
        .boxartPath = "assets/images/systems/nes/godzilla-monster-of-monsters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golf",
        .boxartPath = "assets/images/systems/nes/golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Golf '92",
        .boxartPath = "assets/images/systems/nes/the-golf-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golf Club: Birdie Rush",
        .boxartPath = "assets/images/systems/nes/golf-club-birdie-rush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golf Grand Slam",
        .boxartPath = "assets/images/systems/nes/golf-grand-slam.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golf-kko Open",
        .boxartPath = "assets/images/systems/nes/golf-kko-open.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golgo 13: Dai 1 Shou: Kamigami no Koukon",
        .boxartPath = "assets/images/systems/nes/golgo-13-dai-1-shou-kamigami-no-koukon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golgo 13: Dai 2 Shou: The Riddle of Icarus",
        .boxartPath = "assets/images/systems/nes/golgo-13-dai-2-shou-the-riddle-of-icarus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golgo 13: Top Secret Episode",
        .boxartPath = "assets/images/systems/nes/golgo-13-top-secret-episode.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gomoku Narabe Renju",
        .boxartPath = "assets/images/systems/nes/gomoku-narabe-renju.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Goonies",
        .boxartPath = "assets/images/systems/nes/the-goonies.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Goonies II",
        .boxartPath = "assets/images/systems/nes/the-goonies-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Goonies II: Fratelli Saigo no Chousen",
        .boxartPath = "assets/images/systems/nes/the-goonies-ii-fratelli-saigo-no-chousen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gorby no Pipeline Daisakusen",
        .boxartPath = "assets/images/systems/nes/gorby-no-pipeline-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Gorilla Man",
        .boxartPath = "assets/images/systems/nes/the-gorilla-man.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gotcha! The Sport!",
        .boxartPath = "assets/images/systems/nes/gotcha-the-sport.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gozonji: Yaji Kita Chin Dōchū",
        .boxartPath = "assets/images/systems/nes/gozonji-yaji-kita-chin-d-ch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gradius II",
        .boxartPath = "assets/images/systems/nes/gradius-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Grand Master",
        .boxartPath = "assets/images/systems/nes/grand-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Great Battle Cyber",
        .boxartPath = "assets/images/systems/nes/great-battle-cyber.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Great Boxing: Rush Up",
        .boxartPath = "assets/images/systems/nes/great-boxing-rush-up.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Great Deal",
        .boxartPath = "assets/images/systems/nes/great-deal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Great Tank",
        .boxartPath = "assets/images/systems/nes/great-tank.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Great Waldo Search",
        .boxartPath = "assets/images/systems/nes/the-great-waldo-search.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Greg Norman's Golf Power",
        .boxartPath = "assets/images/systems/nes/greg-norman-s-golf-power.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gremlins 2: Shinshu Tanjou",
        .boxartPath = "assets/images/systems/nes/gremlins-2-shinshu-tanjou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gremlins 2: The New Batch",
        .boxartPath = "assets/images/systems/nes/gremlins-2-the-new-batch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Grid Grabber",
        .boxartPath = "assets/images/systems/nes/grid-grabber.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Guardian Legend",
        .boxartPath = "assets/images/systems/nes/the-guardian-legend.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Guardic Gaiden",
        .boxartPath = "assets/images/systems/nes/guardic-gaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Guerrilla War",
        .boxartPath = "assets/images/systems/nes/guerrilla-war.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Guevara",
        .boxartPath = "assets/images/systems/nes/guevara.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gumshoe",
        .boxartPath = "assets/images/systems/nes/gumshoe.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gun-Dec",
        .boxartPath = "assets/images/systems/nes/gun-dec.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gun-Nac",
        .boxartPath = "assets/images/systems/nes/gun-nac.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gun Sight",
        .boxartPath = "assets/images/systems/nes/gun-sight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "GunHed: Aratanaru Tatakai",
        .boxartPath = "assets/images/systems/nes/gunhed-aratanaru-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gyro Set",
        .boxartPath = "assets/images/systems/nes/gyro-set.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gyrodine",
        .boxartPath = "assets/images/systems/nes/gyrodine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gyromite",
        .boxartPath = "assets/images/systems/nes/gyromite.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gyruss",
        .boxartPath = "assets/images/systems/nes/gyruss.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Haja no Fūin",
        .boxartPath = "assets/images/systems/nes/haja-no-f-in.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hammerin' Harry",
        .boxartPath = "assets/images/systems/nes/hammerin-harry.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hana no Star Kaidō",
        .boxartPath = "assets/images/systems/nes/hana-no-star-kaid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hanjuku Hero",
        .boxartPath = "assets/images/systems/nes/hanjuku-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Happily Ever After",
        .boxartPath = "assets/images/systems/nes/happily-ever-after.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Happy Birthday Bugs",
        .boxartPath = "assets/images/systems/nes/happy-birthday-bugs.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Happy Camper",
        .boxartPath = "assets/images/systems/nes/happy-camper.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hard Drivin'",
        .boxartPath = "assets/images/systems/nes/hard-drivin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Harlem Globetrotters",
        .boxartPath = "assets/images/systems/nes/harlem-globetrotters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hatris",
        .boxartPath = "assets/images/systems/nes/hatris.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hayauchi Super Igo",
        .boxartPath = "assets/images/systems/nes/hayauchi-super-igo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Heavy Barrel",
        .boxartPath = "assets/images/systems/nes/heavy-barrel.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Heavy Shreddin'",
        .boxartPath = "assets/images/systems/nes/heavy-shreddin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hebereke",
        .boxartPath = "assets/images/systems/nes/hebereke.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hector '87",
        .boxartPath = "assets/images/systems/nes/hector-87.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Heisei Tensai Bakabon",
        .boxartPath = "assets/images/systems/nes/heisei-tensai-bakabon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hello Kitty no Hanabatake",
        .boxartPath = "assets/images/systems/nes/hello-kitty-no-hanabatake.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hello Kitty World",
        .boxartPath = "assets/images/systems/nes/hello-kitty-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Heracles no Eikō II: Titan no Metsubō",
        .boxartPath = "assets/images/systems/nes/heracles-no-eik-ii-titan-no-metsub.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "HeroQuest",
        .boxartPath = "assets/images/systems/nes/heroquest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hi no Tori: Hōō-hen: Gaō no Bōken",
        .boxartPath = "assets/images/systems/nes/hi-no-tori-h-hen-ga-no-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Higemaru Makaijima: Nanatsu no Shima Daibōken",
        .boxartPath = "assets/images/systems/nes/higemaru-makaijima-nanatsu-no-shima-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "High Speed",
        .boxartPath = "assets/images/systems/nes/high-speed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Highway Star",
        .boxartPath = "assets/images/systems/nes/highway-star.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hikari no Senshi Photon: The Ultimate Game on Planet Earth",
        .boxartPath = "assets/images/systems/nes/hikari-no-senshi-photon-the-ultimate-game-on-planet-earth.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hirake! Ponkikki",
        .boxartPath = "assets/images/systems/nes/hirake-ponkikki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hiryu no Ken II: Dragon no Tsubasa",
        .boxartPath = "assets/images/systems/nes/hiryu-no-ken-ii-dragon-no-tsubasa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hiryu no Ken III: 5 Nin no Ryu Senshi",
        .boxartPath = "assets/images/systems/nes/hiryu-no-ken-iii-5-nin-no-ryu-senshi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hiryu no Ken Special: Fighting Wars",
        .boxartPath = "assets/images/systems/nes/hiryu-no-ken-special-fighting-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hiryū no Ken: Ougi no Sho",
        .boxartPath = "assets/images/systems/nes/hiry-no-ken-ougi-no-sho.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hissatsu Dōjō Yaburi",
        .boxartPath = "assets/images/systems/nes/hissatsu-d-j-yaburi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hissatsu Shigoto Nin",
        .boxartPath = "assets/images/systems/nes/hissatsu-shigoto-nin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hit the Ice",
        .boxartPath = "assets/images/systems/nes/hit-the-ice.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hitler no Fukkatsu: Top Secret",
        .boxartPath = "assets/images/systems/nes/hitler-no-fukkatsu-top-secret.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hogan's Alley",
        .boxartPath = "assets/images/systems/nes/hogan-s-alley.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hokkaidō Rensa Satsujin: Ohotsuku ni Kiyu",
        .boxartPath = "assets/images/systems/nes/hokkaid-rensa-satsujin-ohotsuku-ni-kiyu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hokuto no Ken",
        .boxartPath = "assets/images/systems/nes/hokuto-no-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hokuto no Ken 2: Seikimatsu Kyūseishu Densetsu",
        .boxartPath = "assets/images/systems/nes/hokuto-no-ken-2-seikimatsu-ky-seishu-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hokuto no Ken 3: Shin Seiki Sōzō: Seiken Retsuden",
        .boxartPath = "assets/images/systems/nes/hokuto-no-ken-3-shin-seiki-s-z-seiken-retsuden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hokuto no Ken 4: Shichisei Hakenden: Hokuto Shinken no Kanata he",
        .boxartPath = "assets/images/systems/nes/hokuto-no-ken-4-shichisei-hakenden-hokuto-shinken-no-kanata-he.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hollywood Squares",
        .boxartPath = "assets/images/systems/nes/hollywood-squares.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Holy Diver",
        .boxartPath = "assets/images/systems/nes/holy-diver.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Home Alone",
        .boxartPath = "assets/images/systems/nes/home-alone.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Home Alone 2: Lost in New York",
        .boxartPath = "assets/images/systems/nes/home-alone-2-lost-in-new-york.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Home Run Night '90: The Pennant League",
        .boxartPath = "assets/images/systems/nes/home-run-night-90-the-pennant-league.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Home Run Nighter: Pennant League‼",
        .boxartPath = "assets/images/systems/nes/home-run-nighter-pennant-league.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hon Shogi: Naitō Kudan Shogi Hiden",
        .boxartPath = "assets/images/systems/nes/hon-shogi-nait-kudan-shogi-hiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Honō no Tōkyūji: Dodge Danpei",
        .boxartPath = "assets/images/systems/nes/hon-no-t-ky-ji-dodge-danpei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Honō no Tōkyūji: Dodge Danpei 2",
        .boxartPath = "assets/images/systems/nes/hon-no-t-ky-ji-dodge-danpei-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hook",
        .boxartPath = "assets/images/systems/nes/hook.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hoops",
        .boxartPath = "assets/images/systems/nes/hoops.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hoppin' Mad",
        .boxartPath = "assets/images/systems/nes/hoppin-mad.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hoshi no Kirby: Yume no Izumi no Monogatari",
        .boxartPath = "assets/images/systems/nes/hoshi-no-kirby-yume-no-izumi-no-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hoshi wo Miru Hito",
        .boxartPath = "assets/images/systems/nes/hoshi-wo-miru-hito.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hostages: The Embassy Mission",
        .boxartPath = "assets/images/systems/nes/hostages-the-embassy-mission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hototogisu",
        .boxartPath = "assets/images/systems/nes/hototogisu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hottāman no Chitei Tanken",
        .boxartPath = "assets/images/systems/nes/hott-man-no-chitei-tanken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hudson Hawk",
        .boxartPath = "assets/images/systems/nes/hudson-hawk.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Hunt for Red October",
        .boxartPath = "assets/images/systems/nes/the-hunt-for-red-october.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hyakki Yagyō",
        .boxartPath = "assets/images/systems/nes/hyakki-yagy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hyaku no Sekai no Monogatari: The Tales on a Watery Wilderness",
        .boxartPath = "assets/images/systems/nes/hyaku-no-sekai-no-monogatari-the-tales-on-a-watery-wilderness.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hydlide",
        .boxartPath = "assets/images/systems/nes/hydlide.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hydlide 3: Yami Kara no Houmonsha",
        .boxartPath = "assets/images/systems/nes/hydlide-3-yami-kara-no-houmonsha.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hydlide Special",
        .boxartPath = "assets/images/systems/nes/hydlide-special.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hyokkori Hyōtanjima: Nazo no Kaizokusen",
        .boxartPath = "assets/images/systems/nes/hyokkori-hy-tanjima-nazo-no-kaizokusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hyper Olympic",
        .boxartPath = "assets/images/systems/nes/hyper-olympic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hyper Sports",
        .boxartPath = "assets/images/systems/nes/hyper-sports.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "I Love Softball",
        .boxartPath = "assets/images/systems/nes/i-love-softball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ice Climber",
        .boxartPath = "assets/images/systems/nes/ice-climber.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ice Hockey",
        .boxartPath = "assets/images/systems/nes/ice-hockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Idol Hakkenden",
        .boxartPath = "assets/images/systems/nes/idol-hakkenden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo: Kyū Roban Taikyoku",
        .boxartPath = "assets/images/systems/nes/igo-ky-roban-taikyoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Meikan",
        .boxartPath = "assets/images/systems/nes/igo-meikan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Shinan",
        .boxartPath = "assets/images/systems/nes/igo-shinan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Shinan '91",
        .boxartPath = "assets/images/systems/nes/igo-shinan-91.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Shinan '92",
        .boxartPath = "assets/images/systems/nes/igo-shinan-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Shinan '93",
        .boxartPath = "assets/images/systems/nes/igo-shinan-93.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Igo Shinan '94",
        .boxartPath = "assets/images/systems/nes/igo-shinan-94.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari",
        .boxartPath = "assets/images/systems/nes/ikari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari II: Dogosoken",
        .boxartPath = "assets/images/systems/nes/ikari-ii-dogosoken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari III",
        .boxartPath = "assets/images/systems/nes/ikari-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari III: The Rescue",
        .boxartPath = "assets/images/systems/nes/ikari-iii-the-rescue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari Warriors",
        .boxartPath = "assets/images/systems/nes/ikari-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikari Warriors II: Victory Road",
        .boxartPath = "assets/images/systems/nes/ikari-warriors-ii-victory-road.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ike Ike! Nekketsu Hockey-bu: Subette Koronde Dairanto",
        .boxartPath = "assets/images/systems/nes/ike-ike-nekketsu-hockey-bu-subette-koronde-dairanto.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikinari Musician",
        .boxartPath = "assets/images/systems/nes/ikinari-musician.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ikki",
        .boxartPath = "assets/images/systems/nes/ikki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Image Fight",
        .boxartPath = "assets/images/systems/nes/image-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Immortal",
        .boxartPath = "assets/images/systems/nes/the-immortal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Impossible Mission II (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/impossible-mission-ii-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Incredible Crash Dummies",
        .boxartPath = "assets/images/systems/nes/the-incredible-crash-dummies.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Indiana Jones and the Last Crusade",
        .boxartPath = "assets/images/systems/nes/indiana-jones-and-the-last-crusade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Indiana Jones and the Temple of Doom",
        .boxartPath = "assets/images/systems/nes/indiana-jones-and-the-temple-of-doom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Indora no Hikari",
        .boxartPath = "assets/images/systems/nes/indora-no-hikari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Infiltrator",
        .boxartPath = "assets/images/systems/nes/infiltrator.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Insector X",
        .boxartPath = "assets/images/systems/nes/insector-x.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Iron Tank",
        .boxartPath = "assets/images/systems/nes/iron-tank.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Iron Tank: The Invasion of Normandy",
        .boxartPath = "assets/images/systems/nes/iron-tank-the-invasion-of-normandy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ironsword: Wizards & Warriors II",
        .boxartPath = "assets/images/systems/nes/ironsword-wizards-warriors-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Isaki Shūgorō no Keiba Hisshōgaku",
        .boxartPath = "assets/images/systems/nes/isaki-sh-gor-no-keiba-hissh-gaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ishin no Arashi",
        .boxartPath = "assets/images/systems/nes/ishin-no-arashi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Isolated Warrior",
        .boxartPath = "assets/images/systems/nes/isolated-warrior.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Itadaki Street: Watashi no Omise ni Yottette",
        .boxartPath = "assets/images/systems/nes/itadaki-street-watashi-no-omise-ni-yottette.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ivan \"Ironman\" Stewart's Super Off Road",
        .boxartPath = "assets/images/systems/nes/ivan-ironman-stewart-s-super-off-road.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "J.League Fighting Soccer: The King of Ace Strikers",
        .boxartPath = "assets/images/systems/nes/j-league-fighting-soccer-the-king-of-ace-strikers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "J.League Super Top Players",
        .boxartPath = "assets/images/systems/nes/j-league-super-top-players.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "J.League Winning Goal",
        .boxartPath = "assets/images/systems/nes/j-league-winning-goal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jack Nicklaus' Greatest 18 Holes of Major Championship Golf",
        .boxartPath = "assets/images/systems/nes/jack-nicklaus-greatest-18-holes-of-major-championship-golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jackal",
        .boxartPath = "assets/images/systems/nes/jackal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jackie Chan",
        .boxartPath = "assets/images/systems/nes/jackie-chan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jackie Chan's Action Kung Fu",
        .boxartPath = "assets/images/systems/nes/jackie-chan-s-action-kung-fu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "JaJaMaru Gekimaden: Maboroshi no Kinmajō",
        .boxartPath = "assets/images/systems/nes/jajamaru-gekimaden-maboroshi-no-kinmaj.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "JaJaMaru: Ninpō Chō",
        .boxartPath = "assets/images/systems/nes/jajamaru-ninp-ch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "JaJaMaru no Daibōken",
        .boxartPath = "assets/images/systems/nes/jajamaru-no-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "James Bond Jr.",
        .boxartPath = "assets/images/systems/nes/james-bond-jr.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jangō",
        .boxartPath = "assets/images/systems/nes/jang.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jarinko Chie: Bakudan Musume no Shiawase Sagashi",
        .boxartPath = "assets/images/systems/nes/jarinko-chie-bakudan-musume-no-shiawase-sagashi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jaws",
        .boxartPath = "assets/images/systems/nes/jaws.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jekyll Hakase no Hōma ga Toki",
        .boxartPath = "assets/images/systems/nes/jekyll-hakase-no-h-ma-ga-toki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jeopardy!",
        .boxartPath = "assets/images/systems/nes/jeopardy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jeopardy! 25th Anniversary Edition",
        .boxartPath = "assets/images/systems/nes/jeopardy-25th-anniversary-edition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jeopardy! Junior Edition",
        .boxartPath = "assets/images/systems/nes/jeopardy-junior-edition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jesus: Kyōfu no Bio Monster",
        .boxartPath = "assets/images/systems/nes/jesus-ky-fu-no-bio-monster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Jetsons: Cogswell's Caper",
        .boxartPath = "assets/images/systems/nes/the-jetsons-cogswell-s-caper.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jigoku Gokuraku Maru",
        .boxartPath = "assets/images/systems/nes/jigoku-gokuraku-maru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jikū Yūden: Debias",
        .boxartPath = "assets/images/systems/nes/jik-y-den-debias.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jimmy Connors Tennis",
        .boxartPath = "assets/images/systems/nes/jimmy-connors-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "JJ: Tobidase Daisakusen Part II",
        .boxartPath = "assets/images/systems/nes/jj-tobidase-daisakusen-part-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Joe & Mac",
        .boxartPath = "assets/images/systems/nes/joe-mac.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Joe & Mac: Caveman Ninja",
        .boxartPath = "assets/images/systems/nes/joe-mac-caveman-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "John Elway's Quarterback",
        .boxartPath = "assets/images/systems/nes/john-elway-s-quarterback.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jongbō",
        .boxartPath = "assets/images/systems/nes/jongb.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jordan vs. Bird: One on One",
        .boxartPath = "assets/images/systems/nes/jordan-vs-bird-one-on-one.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Joshua & the Battle of Jericho (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/joshua-the-battle-of-jericho-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Journey to Silius",
        .boxartPath = "assets/images/systems/nes/journey-to-silius.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Joust",
        .boxartPath = "assets/images/systems/nes/joust.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Joy Mech Fight",
        .boxartPath = "assets/images/systems/nes/joy-mech-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "JuJu Densetsu",
        .boxartPath = "assets/images/systems/nes/juju-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jumbo Ozaki no Hole in One Professional",
        .boxartPath = "assets/images/systems/nes/jumbo-ozaki-no-hole-in-one-professional.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jumpin' Kid: Jack to Mame no Ki Monogatari",
        .boxartPath = "assets/images/systems/nes/jumpin-kid-jack-to-mame-no-ki-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Jungle Book",
        .boxartPath = "assets/images/systems/nes/the-jungle-book.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jurassic Park",
        .boxartPath = "assets/images/systems/nes/jurassic-park.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Just Breed",
        .boxartPath = "assets/images/systems/nes/just-breed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jūōki",
        .boxartPath = "assets/images/systems/nes/j-ki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Juuryoku Soukou Metal Storm",
        .boxartPath = "assets/images/systems/nes/juuryoku-soukou-metal-storm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Juvei Quest",
        .boxartPath = "assets/images/systems/nes/juvei-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kabuki: Quantum Fighter",
        .boxartPath = "assets/images/systems/nes/kabuki-quantum-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kabushiki Dōjō: The Stock Speculation",
        .boxartPath = "assets/images/systems/nes/kabushiki-d-j-the-stock-speculation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaettekita! Gunjin Shogi: Nanya Sore!?",
        .boxartPath = "assets/images/systems/nes/kaettekita-gunjin-shogi-nanya-sore.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kage no Densetsu",
        .boxartPath = "assets/images/systems/nes/kage-no-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kagerō Densetsu",
        .boxartPath = "assets/images/systems/nes/kager-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaguya-hime Densetsu",
        .boxartPath = "assets/images/systems/nes/kaguya-hime-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kai no Bōken",
        .boxartPath = "assets/images/systems/nes/kai-no-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaijū Monogatari",
        .boxartPath = "assets/images/systems/nes/kaij-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaiketsu Yancha Maru",
        .boxartPath = "assets/images/systems/nes/kaiketsu-yancha-maru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaiketsu Yanchamaru 2: Karakuri Land",
        .boxartPath = "assets/images/systems/nes/kaiketsu-yanchamaru-2-karakuri-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kaiketsu Yanchamaru 3: Taiketsu! Zōringen",
        .boxartPath = "assets/images/systems/nes/kaiketsu-yanchamaru-3-taiketsu-z-ringen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kakefu Kimi no Jump Tengoku: Speed Jigoku",
        .boxartPath = "assets/images/systems/nes/kakefu-kimi-no-jump-tengoku-speed-jigoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kame no Ongaeshi: Urashima Densetsu",
        .boxartPath = "assets/images/systems/nes/kame-no-ongaeshi-urashima-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kamen no Ninja: Akakage",
        .boxartPath = "assets/images/systems/nes/kamen-no-ninja-akakage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kamen no Ninja Hanamaru",
        .boxartPath = "assets/images/systems/nes/kamen-no-ninja-hanamaru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kamen Rider Club: Gekitotsu Shocker Land",
        .boxartPath = "assets/images/systems/nes/kamen-rider-club-gekitotsu-shocker-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kamen Rider SD: GranShocker no Yabō",
        .boxartPath = "assets/images/systems/nes/kamen-rider-sd-granshocker-no-yab.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kanshaku Tamanage Kantarō no Tōkaidō Gojūsan-tsugi",
        .boxartPath = "assets/images/systems/nes/kanshaku-tamanage-kantar-no-t-kaid-goj-san-tsugi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karakuri Kengō Den Musashi Lord: Karakuri Jin Hashiru!",
        .boxartPath = "assets/images/systems/nes/karakuri-keng-den-musashi-lord-karakuri-jin-hashiru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karaoke Studio",
        .boxartPath = "assets/images/systems/nes/karaoke-studio.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karaoke Studio Senyou Cassette Vol. 1",
        .boxartPath = "assets/images/systems/nes/karaoke-studio-senyou-cassette-vol-1.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karaoke Studio Senyou Cassette Vol. 2",
        .boxartPath = "assets/images/systems/nes/karaoke-studio-senyou-cassette-vol-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karate Champ",
        .boxartPath = "assets/images/systems/nes/karate-champ.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Karate Kid",
        .boxartPath = "assets/images/systems/nes/the-karate-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karateka",
        .boxartPath = "assets/images/systems/nes/karateka.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Karnov",
        .boxartPath = "assets/images/systems/nes/karnov.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kawa no Nushi Tsuri",
        .boxartPath = "assets/images/systems/nes/kawa-no-nushi-tsuri.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keiba Simulation: Honmei",
        .boxartPath = "assets/images/systems/nes/keiba-simulation-honmei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keisan Game Sansū 1-nen",
        .boxartPath = "assets/images/systems/nes/keisan-game-sans-1-nen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keisan Game Sansū 2-nen",
        .boxartPath = "assets/images/systems/nes/keisan-game-sans-2-nen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keisan Game Sansū 3-nen",
        .boxartPath = "assets/images/systems/nes/keisan-game-sans-3-nen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keisan Game Sansū 4-nen",
        .boxartPath = "assets/images/systems/nes/keisan-game-sans-4-nen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keisan Game Sansū 5+6-nen",
        .boxartPath = "assets/images/systems/nes/keisan-game-sans-5-6-nen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kekkyoku Nankyoku Daibōken",
        .boxartPath = "assets/images/systems/nes/kekkyoku-nankyoku-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kero Kero Keroppi no Daibōken",
        .boxartPath = "assets/images/systems/nes/kero-kero-keroppi-no-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kero Kero Keroppi no Daibōken 2: Donuts Ike ha Ōsawagi!",
        .boxartPath = "assets/images/systems/nes/kero-kero-keroppi-no-daib-ken-2-donuts-ike-ha-sawagi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Keroppi to Keroriinu no Splash Bomb!",
        .boxartPath = "assets/images/systems/nes/keroppi-to-keroriinu-no-splash-bomb.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kick Off",
        .boxartPath = "assets/images/systems/nes/kick-off.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kickle Cubicle",
        .boxartPath = "assets/images/systems/nes/kickle-cubicle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "KickMaster",
        .boxartPath = "assets/images/systems/nes/kickmaster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kid Icarus",
        .boxartPath = "assets/images/systems/nes/kid-icarus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kid Klown in Night Mayor World",
        .boxartPath = "assets/images/systems/nes/kid-klown-in-night-mayor-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kid Kool and the Quest for the Seven Wonder Herbs",
        .boxartPath = "assets/images/systems/nes/kid-kool-and-the-quest-for-the-seven-wonder-herbs.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kid Niki: Radical Ninja",
        .boxartPath = "assets/images/systems/nes/kid-niki-radical-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King Kong 2: Ikari no Megaton Punch",
        .boxartPath = "assets/images/systems/nes/king-kong-2-ikari-no-megaton-punch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King Neptune's Adventure (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/king-neptune-s-adventure-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King of Kings",
        .boxartPath = "assets/images/systems/nes/king-of-kings.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The King of Kings: The Early Years (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/the-king-of-kings-the-early-years-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King's Knight",
        .boxartPath = "assets/images/systems/nes/king-s-knight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kings of the Beach",
        .boxartPath = "assets/images/systems/nes/kings-of-the-beach.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King's Quest V",
        .boxartPath = "assets/images/systems/nes/king-s-quest-v.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kinnikuman: Muscle Tag Match",
        .boxartPath = "assets/images/systems/nes/kinnikuman-muscle-tag-match.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kirby's Adventure",
        .boxartPath = "assets/images/systems/nes/kirby-s-adventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kiteretsu Daihyakka",
        .boxartPath = "assets/images/systems/nes/kiteretsu-daihyakka.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kitty's Catch",
        .boxartPath = "assets/images/systems/nes/kitty-s-catch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kiwi Kraze",
        .boxartPath = "assets/images/systems/nes/kiwi-kraze.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "KlashBall",
        .boxartPath = "assets/images/systems/nes/klashball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Klax (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/klax-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Knight Rider",
        .boxartPath = "assets/images/systems/nes/knight-rider.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Koji Tanigawa no Shogi Shinan III",
        .boxartPath = "assets/images/systems/nes/koji-tanigawa-no-shogi-shinan-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Konami Hyper Soccer",
        .boxartPath = "assets/images/systems/nes/konami-hyper-soccer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Konamik Sports in Seoul",
        .boxartPath = "assets/images/systems/nes/konamik-sports-in-seoul.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Koufuku wo Yobu Game: Dora Dora Dora",
        .boxartPath = "assets/images/systems/nes/koufuku-wo-yobu-game-dora-dora-dora.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kōryu Densetsu Villgust Gaiden",
        .boxartPath = "assets/images/systems/nes/k-ryu-densetsu-villgust-gaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Koushien",
        .boxartPath = "assets/images/systems/nes/koushien.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Krazy Kreatures (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/krazy-kreatures-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Krion Conquest",
        .boxartPath = "assets/images/systems/nes/the-krion-conquest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Krusty's Fun House",
        .boxartPath = "assets/images/systems/nes/krusty-s-fun-house.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kujaku Ō",
        .boxartPath = "assets/images/systems/nes/kujaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kujaku Ō II",
        .boxartPath = "assets/images/systems/nes/kujaku-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kung Fu",
        .boxartPath = "assets/images/systems/nes/kung-fu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kung-Fu Heroes",
        .boxartPath = "assets/images/systems/nes/kung-fu-heroes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kunio-kun no Nekketsu Soccer League",
        .boxartPath = "assets/images/systems/nes/kunio-kun-no-nekketsu-soccer-league.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kurogane Hiroshi no Yosō Daisuki! Kachiuma Densetsu",
        .boxartPath = "assets/images/systems/nes/kurogane-hiroshi-no-yos-daisuki-kachiuma-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyattō Ninden Teyandē",
        .boxartPath = "assets/images/systems/nes/kyatt-ninden-teyand.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyonshies 2",
        .boxartPath = "assets/images/systems/nes/kyonshies-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyoro-chan Land",
        .boxartPath = "assets/images/systems/nes/kyoro-chan-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyotaro Nishimura Mystery: Blue Train Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/kyotaro-nishimura-mystery-blue-train-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyotaro Nishimura Mystery: Super Express Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/kyotaro-nishimura-mystery-super-express-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyōryū Sentai Zyuranger",
        .boxartPath = "assets/images/systems/nes/ky-ry-sentai-zyuranger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Harikiri Kōshien",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-harikiri-k-shien.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Harikiri Stadium",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-harikiri-stadium.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Harikiri Stadium '88 Senshuu Shin Data Version",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-harikiri-stadium-88-senshuu-shin-data-version.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Harikiri Stadium: Heisei Gannen-ban",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-harikiri-stadium-heisei-gannen-ban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Harikiri Stadium III",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-harikiri-stadium-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kyūkyoku Tiger",
        .boxartPath = "assets/images/systems/nes/ky-kyoku-tiger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "La Alfombra Magica",
        .boxartPath = "assets/images/systems/nes/la-alfombra-magica.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "La Gran Aventura Submarina",
        .boxartPath = "assets/images/systems/nes/la-gran-aventura-submarina.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "La Guerra del Golfo",
        .boxartPath = "assets/images/systems/nes/la-guerra-del-golfo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "La Salle Ishii no Child's Quest",
        .boxartPath = "assets/images/systems/nes/la-salle-ishii-no-child-s-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Labyrinth: Maō no Meikyū",
        .boxartPath = "assets/images/systems/nes/labyrinth-ma-no-meiky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lagrange Point",
        .boxartPath = "assets/images/systems/nes/lagrange-point.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Laser Invasion",
        .boxartPath = "assets/images/systems/nes/laser-invasion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Last Action Hero",
        .boxartPath = "assets/images/systems/nes/last-action-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Last Armageddon",
        .boxartPath = "assets/images/systems/nes/last-armageddon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Last Ninja",
        .boxartPath = "assets/images/systems/nes/the-last-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Last Starfighter",
        .boxartPath = "assets/images/systems/nes/the-last-starfighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Law of the West",
        .boxartPath = "assets/images/systems/nes/law-of-the-west.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Layla",
        .boxartPath = "assets/images/systems/nes/layla.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lee Trevino's Fighting Golf",
        .boxartPath = "assets/images/systems/nes/lee-trevino-s-fighting-golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Legacy of the Wizard",
        .boxartPath = "assets/images/systems/nes/legacy-of-the-wizard.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Legend of Hero Tonma",
        .boxartPath = "assets/images/systems/nes/legend-of-hero-tonma.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Prince Valiant",
        .boxartPath = "assets/images/systems/nes/the-legend-of-prince-valiant.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Legendary Wings",
        .boxartPath = "assets/images/systems/nes/legendary-wings.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Legends of the Diamond",
        .boxartPath = "assets/images/systems/nes/legends-of-the-diamond.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lemmings",
        .boxartPath = "assets/images/systems/nes/lemmings.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "L'Empereur",
        .boxartPath = "assets/images/systems/nes/l-empereur.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Les Chevaliers du Zodiaque: La Legende d'Or",
        .boxartPath = "assets/images/systems/nes/les-chevaliers-du-zodiaque-la-legende-d-or.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lethal Weapon",
        .boxartPath = "assets/images/systems/nes/lethal-weapon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Licence to Kill",
        .boxartPath = "assets/images/systems/nes/licence-to-kill.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Life Force",
        .boxartPath = "assets/images/systems/nes/life-force.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Life Force Salamander",
        .boxartPath = "assets/images/systems/nes/life-force-salamander.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Linus Spacehead's Cosmic Crusade (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/linus-spacehead-s-cosmic-crusade-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Lion King",
        .boxartPath = "assets/images/systems/nes/the-lion-king.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little League Baseball: Championship Series",
        .boxartPath = "assets/images/systems/nes/little-league-baseball-championship-series.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little Magic",
        .boxartPath = "assets/images/systems/nes/little-magic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Little Mermaid",
        .boxartPath = "assets/images/systems/nes/the-little-mermaid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little Mermaid: Ningyo Hime",
        .boxartPath = "assets/images/systems/nes/little-mermaid-ningyo-hime.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little Nemo: The Dream Master",
        .boxartPath = "assets/images/systems/nes/little-nemo-the-dream-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little Ninja Brothers",
        .boxartPath = "assets/images/systems/nes/little-ninja-brothers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Little Samson",
        .boxartPath = "assets/images/systems/nes/little-samson.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lode Runner",
        .boxartPath = "assets/images/systems/nes/lode-runner.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Lone Ranger",
        .boxartPath = "assets/images/systems/nes/the-lone-ranger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Loopz",
        .boxartPath = "assets/images/systems/nes/loopz.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Lord of King",
        .boxartPath = "assets/images/systems/nes/the-lord-of-king.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lost Word of Jenny: Ushinawareta Message",
        .boxartPath = "assets/images/systems/nes/lost-word-of-jenny-ushinawareta-message.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lot Lot",
        .boxartPath = "assets/images/systems/nes/lot-lot.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Low G Man: The Low Gravity Man",
        .boxartPath = "assets/images/systems/nes/low-g-man-the-low-gravity-man.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lunar Ball",
        .boxartPath = "assets/images/systems/nes/lunar-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lunar Pool",
        .boxartPath = "assets/images/systems/nes/lunar-pool.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lupin Sansei: Pandora no Isan",
        .boxartPath = "assets/images/systems/nes/lupin-sansei-pandora-no-isan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "M.C. Kids",
        .boxartPath = "assets/images/systems/nes/m-c-kids.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "M.U.L.E.",
        .boxartPath = "assets/images/systems/nes/m-u-l-e.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mach Rider",
        .boxartPath = "assets/images/systems/nes/mach-rider.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mad City",
        .boxartPath = "assets/images/systems/nes/mad-city.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mad Max",
        .boxartPath = "assets/images/systems/nes/mad-max.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Madoola no Tsubasa: The Wing of Madoola",
        .boxartPath = "assets/images/systems/nes/madoola-no-tsubasa-the-wing-of-madoola.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Mafat Conspiracy",
        .boxartPath = "assets/images/systems/nes/the-mafat-conspiracy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Magic Candle",
        .boxartPath = "assets/images/systems/nes/the-magic-candle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magic Darts",
        .boxartPath = "assets/images/systems/nes/magic-darts.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magic John",
        .boxartPath = "assets/images/systems/nes/magic-john.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magic Johnson's Fast Break",
        .boxartPath = "assets/images/systems/nes/magic-johnson-s-fast-break.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Magic of Scheherazade",
        .boxartPath = "assets/images/systems/nes/the-magic-of-scheherazade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magical Kids Doropie",
        .boxartPath = "assets/images/systems/nes/magical-kids-doropie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magical ☆ Tarurūto-kun 2: Mahō Daibōken",
        .boxartPath = "assets/images/systems/nes/magical-tarur-to-kun-2-mah-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magical ☆ Tarurūto-kun: Fantastic World‼",
        .boxartPath = "assets/images/systems/nes/magical-tarur-to-kun-fantastic-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magician",
        .boxartPath = "assets/images/systems/nes/magician.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "MagMax",
        .boxartPath = "assets/images/systems/nes/magmax.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magnum Kiki Ippatsu: Empire City: 1931",
        .boxartPath = "assets/images/systems/nes/magnum-kiki-ippatsu-empire-city-1931.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "MahaRaja",
        .boxartPath = "assets/images/systems/nes/maharaja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahjong",
        .boxartPath = "assets/images/systems/nes/mahjong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahjong Club Nagatacho: Sōsaisen",
        .boxartPath = "assets/images/systems/nes/mahjong-club-nagatacho-s-saisen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahjong G-Men: Nichibutsu Mahjong III",
        .boxartPath = "assets/images/systems/nes/mahjong-g-men-nichibutsu-mahjong-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahjong Taikai",
        .boxartPath = "assets/images/systems/nes/mahjong-taikai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahjong Taisen",
        .boxartPath = "assets/images/systems/nes/mahjong-taisen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mahō no Princess Minky Momo: Remember Dream",
        .boxartPath = "assets/images/systems/nes/mah-no-princess-minky-momo-remember-dream.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Maison Ikkoku",
        .boxartPath = "assets/images/systems/nes/maison-ikkoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Majaventure: Mahjong Senki",
        .boxartPath = "assets/images/systems/nes/majaventure-mahjong-senki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Major League",
        .boxartPath = "assets/images/systems/nes/major-league.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Major League Baseball",
        .boxartPath = "assets/images/systems/nes/major-league-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Majō Densetsu II: Gariusu no Meikyū",
        .boxartPath = "assets/images/systems/nes/maj-densetsu-ii-gariusu-no-meiky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Makaimura",
        .boxartPath = "assets/images/systems/nes/makaimura.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Maniac Mansion",
        .boxartPath = "assets/images/systems/nes/maniac-mansion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mappy",
        .boxartPath = "assets/images/systems/nes/mappy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mappy Kids",
        .boxartPath = "assets/images/systems/nes/mappy-kids.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mappy-Land",
        .boxartPath = "assets/images/systems/nes/mappy-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Marble Madness",
        .boxartPath = "assets/images/systems/nes/marble-madness.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario & Yoshi",
        .boxartPath = "assets/images/systems/nes/mario-yoshi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Bros.",
        .boxartPath = "assets/images/systems/nes/mario-bros.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario is Missing!",
        .boxartPath = "assets/images/systems/nes/mario-is-missing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Open Golf",
        .boxartPath = "assets/images/systems/nes/mario-open-golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario's Time Machine",
        .boxartPath = "assets/images/systems/nes/mario-s-time-machine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mark's Magic Darts",
        .boxartPath = "assets/images/systems/nes/mark-s-magic-darts.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Marusa no Onna",
        .boxartPath = "assets/images/systems/nes/marusa-no-onna.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mashin Eiyūden Wataru Gaiden",
        .boxartPath = "assets/images/systems/nes/mashin-eiy-den-wataru-gaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mashou",
        .boxartPath = "assets/images/systems/nes/mashou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Master Chu and the Drunkard Hu (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/master-chu-and-the-drunkard-hu-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Matendo Douji",
        .boxartPath = "assets/images/systems/nes/matendo-douji.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Matsumoto Tōru no Kabushiki Hisshōgaku",
        .boxartPath = "assets/images/systems/nes/matsumoto-t-ru-no-kabushiki-hissh-gaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Matsumoto Tōru no Kabushiki Hisshōgaku Part II",
        .boxartPath = "assets/images/systems/nes/matsumoto-t-ru-no-kabushiki-hissh-gaku-part-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Max Warrior: Wakusei Kaigenrei",
        .boxartPath = "assets/images/systems/nes/max-warrior-wakusei-kaigenrei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Maxi 15 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/maxi-15-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "McDonaldland",
        .boxartPath = "assets/images/systems/nes/mcdonaldland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mechanized Attack",
        .boxartPath = "assets/images/systems/nes/mechanized-attack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meiji Ishin",
        .boxartPath = "assets/images/systems/nes/meiji-ishin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meikyuu Kumikyoku: Miron no Daibouken",
        .boxartPath = "assets/images/systems/nes/meikyuu-kumikyoku-miron-no-daibouken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meikyuu Shima",
        .boxartPath = "assets/images/systems/nes/meikyuu-shima.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meimon! Daisan Yakyū-bu",
        .boxartPath = "assets/images/systems/nes/meimon-daisan-yaky-bu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meimon! Tako Nishiōendan",
        .boxartPath = "assets/images/systems/nes/meimon-tako-nishi-endan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meitantei Holmes: Kiri no London Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/meitantei-holmes-kiri-no-london-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Meitantei Holmes: M Kara no Chōsenjō",
        .boxartPath = "assets/images/systems/nes/meitantei-holmes-m-kara-no-ch-senj.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Menace Beach (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/menace-beach-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mendel Palace",
        .boxartPath = "assets/images/systems/nes/mendel-palace.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Fighter (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/metal-fighter-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Flame PsyBuster",
        .boxartPath = "assets/images/systems/nes/metal-flame-psybuster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Gear",
        .boxartPath = "assets/images/systems/nes/metal-gear.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Max",
        .boxartPath = "assets/images/systems/nes/metal-max.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Mech: Man & Machine",
        .boxartPath = "assets/images/systems/nes/metal-mech-man-machine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Slader Glory",
        .boxartPath = "assets/images/systems/nes/metal-slader-glory.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metal Storm",
        .boxartPath = "assets/images/systems/nes/metal-storm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Metro-Cross",
        .boxartPath = "assets/images/systems/nes/metro-cross.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mezase Pachi Pro: Pachio-kun",
        .boxartPath = "assets/images/systems/nes/mezase-pachi-pro-pachio-kun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mezase! Top Pro Green ni Kakeru Yume",
        .boxartPath = "assets/images/systems/nes/mezase-top-pro-green-ni-kakeru-yume.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Michael Andretti's World Grand Prix",
        .boxartPath = "assets/images/systems/nes/michael-andretti-s-world-grand-prix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey Mouse: Fushigi no Kuni do Daibouken",
        .boxartPath = "assets/images/systems/nes/mickey-mouse-fushigi-no-kuni-do-daibouken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey Mouse III: Yume Fuusen",
        .boxartPath = "assets/images/systems/nes/mickey-mouse-iii-yume-fuusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey Mousecapade",
        .boxartPath = "assets/images/systems/nes/mickey-mousecapade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey's Adventure in Numberland",
        .boxartPath = "assets/images/systems/nes/mickey-s-adventure-in-numberland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey's Safari in Letterland",
        .boxartPath = "assets/images/systems/nes/mickey-s-safari-in-letterland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Micro Machines (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/micro-machines-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "MiG-29: Soviet Fighter (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/mig-29-soviet-fighter-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Might and Magic Book One: Secret of the Inner Sanctum",
        .boxartPath = "assets/images/systems/nes/might-and-magic-book-one-secret-of-the-inner-sanctum.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Might & Magic: Secret of the Inner Sanctum",
        .boxartPath = "assets/images/systems/nes/might-magic-secret-of-the-inner-sanctum.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mighty Bomb Jack",
        .boxartPath = "assets/images/systems/nes/mighty-bomb-jack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mighty Final Fight",
        .boxartPath = "assets/images/systems/nes/mighty-final-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mike Tyson's Punch-Out‼",
        .boxartPath = "assets/images/systems/nes/mike-tyson-s-punch-out.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Millipede",
        .boxartPath = "assets/images/systems/nes/millipede.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Millipede: Kyodai Konchuu no Gyakushuu",
        .boxartPath = "assets/images/systems/nes/millipede-kyodai-konchuu-no-gyakushuu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Milon's Secret Castle",
        .boxartPath = "assets/images/systems/nes/milon-s-secret-castle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mindseeker",
        .boxartPath = "assets/images/systems/nes/mindseeker.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Minelvaton Saga: Ragon no Fukkatsu",
        .boxartPath = "assets/images/systems/nes/minelvaton-saga-ragon-no-fukkatsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mini-Putt",
        .boxartPath = "assets/images/systems/nes/mini-putt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Minna no Tabo no Nakayoshi Daisakusen",
        .boxartPath = "assets/images/systems/nes/minna-no-tabo-no-nakayoshi-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Miracle Piano Teaching System",
        .boxartPath = "assets/images/systems/nes/the-miracle-piano-teaching-system.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Miracle Ropitt: 2100-Nen no Daibōken",
        .boxartPath = "assets/images/systems/nes/miracle-ropitt-2100-nen-no-daib-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mirai Senshi: Lios",
        .boxartPath = "assets/images/systems/nes/mirai-senshi-lios.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mirai Shinwa Jarvas",
        .boxartPath = "assets/images/systems/nes/mirai-shinwa-jarvas.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Misa Yamamura Suspense: Kyōto Hana no Misshitsu Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/misa-yamamura-suspense-ky-to-hana-no-misshitsu-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Misa Yamamura Suspense: Kyōto Ryū no Tera Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/misa-yamamura-suspense-ky-to-ry-no-tera-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Misa Yamamura Suspense: Kyōto Zaiteku Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/misa-yamamura-suspense-ky-to-zaiteku-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mission Cobra (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/mission-cobra-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mission: Impossible",
        .boxartPath = "assets/images/systems/nes/mission-impossible.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mississippi Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/mississippi-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mito Kōmon II: Sekai Manyūki",
        .boxartPath = "assets/images/systems/nes/mito-k-mon-ii-sekai-many-ki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mitsume ga Tōru",
        .boxartPath = "assets/images/systems/nes/mitsume-ga-t-ru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mizushima Shinji no Daikōshien",
        .boxartPath = "assets/images/systems/nes/mizushima-shinji-no-daik-shien.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moai-kun",
        .boxartPath = "assets/images/systems/nes/moai-kun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mobile Suit Z Gundam: Hot Scramble",
        .boxartPath = "assets/images/systems/nes/mobile-suit-z-gundam-hot-scramble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moe-Pro! Saikyou Hen",
        .boxartPath = "assets/images/systems/nes/moe-pro-saikyou-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moe Pro!! Yakyuu '90: Kandou Hen",
        .boxartPath = "assets/images/systems/nes/moe-pro-yakyuu-90-kandou-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Judo Warriors",
        .boxartPath = "assets/images/systems/nes/moero-judo-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Junior Basket: Two on Two",
        .boxartPath = "assets/images/systems/nes/moero-junior-basket-two-on-two.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Pro Soccer",
        .boxartPath = "assets/images/systems/nes/moero-pro-soccer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Pro Tennis",
        .boxartPath = "assets/images/systems/nes/moero-pro-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Pro Yakyū",
        .boxartPath = "assets/images/systems/nes/moero-pro-yaky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero‼ Pro Yakyuu '88: Kettei Ban",
        .boxartPath = "assets/images/systems/nes/moero-pro-yakyuu-88-kettei-ban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moero TwinBee: Cinnamon Hakase o Sukue!",
        .boxartPath = "assets/images/systems/nes/moero-twinbee-cinnamon-hakase-o-sukue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moeru! Onīsan",
        .boxartPath = "assets/images/systems/nes/moeru-on-san.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Momotaro Densetsu Gaiden",
        .boxartPath = "assets/images/systems/nes/momotaro-densetsu-gaiden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Momotaro Densetsu: Peach Boy Legend",
        .boxartPath = "assets/images/systems/nes/momotaro-densetsu-peach-boy-legend.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Momotaro Dentetsu",
        .boxartPath = "assets/images/systems/nes/momotaro-dentetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Money Game",
        .boxartPath = "assets/images/systems/nes/the-money-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Money Game II: Kabutochou no Kiseki",
        .boxartPath = "assets/images/systems/nes/the-money-game-ii-kabutochou-no-kiseki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monopoly",
        .boxartPath = "assets/images/systems/nes/monopoly.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monster in My Pocket",
        .boxartPath = "assets/images/systems/nes/monster-in-my-pocket.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monster Maker: 7-tsu no Hihō",
        .boxartPath = "assets/images/systems/nes/monster-maker-7-tsu-no-hih.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monster Party",
        .boxartPath = "assets/images/systems/nes/monster-party.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monster Truck Rally",
        .boxartPath = "assets/images/systems/nes/monster-truck-rally.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moon Crystal",
        .boxartPath = "assets/images/systems/nes/moon-crystal.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moon Ranger (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/moon-ranger-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Morita Shogi",
        .boxartPath = "assets/images/systems/nes/morita-shogi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mother",
        .boxartPath = "assets/images/systems/nes/mother.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Motocross Champion",
        .boxartPath = "assets/images/systems/nes/motocross-champion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Motor City Patrol",
        .boxartPath = "assets/images/systems/nes/motor-city-patrol.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mottomo Abunai Deka",
        .boxartPath = "assets/images/systems/nes/mottomo-abunai-deka.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Moulin Rouge Senki: Melville no Honoo",
        .boxartPath = "assets/images/systems/nes/moulin-rouge-senki-melville-no-honoo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mōryō Senki MADARA",
        .boxartPath = "assets/images/systems/nes/m-ry-senki-madara.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mr. Gimmick",
        .boxartPath = "assets/images/systems/nes/mr-gimmick.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mugen Senshi Valis",
        .boxartPath = "assets/images/systems/nes/mugen-senshi-valis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Muppet Adventure: Chaos at the Carnival",
        .boxartPath = "assets/images/systems/nes/muppet-adventure-chaos-at-the-carnival.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Murder Club",
        .boxartPath = "assets/images/systems/nes/murder-club.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Musashi no Bōken",
        .boxartPath = "assets/images/systems/nes/musashi-no-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Musashi no Ken: Tadaima Shugyō Chū",
        .boxartPath = "assets/images/systems/nes/musashi-no-ken-tadaima-shugy-ch.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Mutant Virus: Crisis in a Computer World",
        .boxartPath = "assets/images/systems/nes/the-mutant-virus-crisis-in-a-computer-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "My Life My Love: Boku no Yume Watashi no Negai",
        .boxartPath = "assets/images/systems/nes/my-life-my-love-boku-no-yume-watashi-no-negai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mystery Quest",
        .boxartPath = "assets/images/systems/nes/mystery-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nagagutsu o Haita Neko: Sekai Isshū 80 Nichi Dai Bōken",
        .boxartPath = "assets/images/systems/nes/nagagutsu-o-haita-neko-sekai-issh-80-nichi-dai-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nakayoshi to Issho",
        .boxartPath = "assets/images/systems/nes/nakayoshi-to-issho.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Namco Classic",
        .boxartPath = "assets/images/systems/nes/namco-classic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Namco Classic II",
        .boxartPath = "assets/images/systems/nes/namco-classic-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Namcot Mahjong III: Mahjong Tengoku",
        .boxartPath = "assets/images/systems/nes/namcot-mahjong-iii-mahjong-tengoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nankoku Shirei‼ Spy vs. Spy",
        .boxartPath = "assets/images/systems/nes/nankoku-shirei-spy-vs-spy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nantettatte‼ Baseball",
        .boxartPath = "assets/images/systems/nes/nantettatte-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nantettatte‼ Baseball '91 Kaimaku Hen",
        .boxartPath = "assets/images/systems/nes/nantettatte-baseball-91-kaimaku-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nantettatte‼ Baseball OB All Star Hen",
        .boxartPath = "assets/images/systems/nes/nantettatte-baseball-ob-all-star-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Napoleon Senki",
        .boxartPath = "assets/images/systems/nes/napoleon-senki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NARC",
        .boxartPath = "assets/images/systems/nes/narc.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Navy Blue",
        .boxartPath = "assets/images/systems/nes/navy-blue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nekketsu Kakutou Densetsu",
        .boxartPath = "assets/images/systems/nes/nekketsu-kakutou-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nekketsu Kouha Kunio-Kun",
        .boxartPath = "assets/images/systems/nes/nekketsu-kouha-kunio-kun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nekketsu Koukou Dodge Ball Bu",
        .boxartPath = "assets/images/systems/nes/nekketsu-koukou-dodge-ball-bu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nekketsu Koukou Dodgeball-bu Soccer-hen",
        .boxartPath = "assets/images/systems/nes/nekketsu-koukou-dodgeball-bu-soccer-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nekketsu! Street Basket: Ganbare Dunk Heroes",
        .boxartPath = "assets/images/systems/nes/nekketsu-street-basket-ganbare-dunk-heroes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nemo: Pajama Hero",
        .boxartPath = "assets/images/systems/nes/nemo-pajama-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NES Open Tournament Golf",
        .boxartPath = "assets/images/systems/nes/nes-open-tournament-golf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NES Play Action Football",
        .boxartPath = "assets/images/systems/nes/nes-play-action-football.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "New Ghostbusters II",
        .boxartPath = "assets/images/systems/nes/new-ghostbusters-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The NewZealand Story",
        .boxartPath = "assets/images/systems/nes/the-newzealand-story.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NFL",
        .boxartPath = "assets/images/systems/nes/nfl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nigel Mansell's World Championship Racing",
        .boxartPath = "assets/images/systems/nes/nigel-mansell-s-world-championship-racing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nightshade",
        .boxartPath = "assets/images/systems/nes/nightshade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Niji no Silkroad",
        .boxartPath = "assets/images/systems/nes/niji-no-silkroad.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ningen Heiki: Dead Fox",
        .boxartPath = "assets/images/systems/nes/ningen-heiki-dead-fox.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Cop Saizou",
        .boxartPath = "assets/images/systems/nes/ninja-cop-saizou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Crusaders",
        .boxartPath = "assets/images/systems/nes/ninja-crusaders.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Hattori-kun: Ninja wa Shūgyō de Gozaru no Maki",
        .boxartPath = "assets/images/systems/nes/ninja-hattori-kun-ninja-wa-sh-gy-de-gozaru-no-maki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja JaJaMaru: Ginga Daisakusen",
        .boxartPath = "assets/images/systems/nes/ninja-jajamaru-ginga-daisakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja JaJaMaru-kun",
        .boxartPath = "assets/images/systems/nes/ninja-jajamaru-kun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Kid",
        .boxartPath = "assets/images/systems/nes/ninja-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja-kun: Ashura no Shō",
        .boxartPath = "assets/images/systems/nes/ninja-kun-ashura-no-sh.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja-kun: Majō no Bōken",
        .boxartPath = "assets/images/systems/nes/ninja-kun-maj-no-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Ryuukenden",
        .boxartPath = "assets/images/systems/nes/ninja-ryuukenden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Ryuukenden II: Ankoku no Jashinken",
        .boxartPath = "assets/images/systems/nes/ninja-ryuukenden-ii-ankoku-no-jashinken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Ryuukenden III: Yomi no Hakobune",
        .boxartPath = "assets/images/systems/nes/ninja-ryuukenden-iii-yomi-no-hakobune.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninjara Hoi!",
        .boxartPath = "assets/images/systems/nes/ninjara-hoi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nintendo World Cup",
        .boxartPath = "assets/images/systems/nes/nintendo-world-cup.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nippon Ichi no Meikantoku",
        .boxartPath = "assets/images/systems/nes/nippon-ichi-no-meikantoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Noah's Ark",
        .boxartPath = "assets/images/systems/nes/noah-s-ark.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nobunaga no Yabō: Bushō Fūunroku",
        .boxartPath = "assets/images/systems/nes/nobunaga-no-yab-bush-f-unroku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nobunaga no Yabō: Sengoku Gun'yūden",
        .boxartPath = "assets/images/systems/nes/nobunaga-no-yab-sengoku-gun-y-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nobunaga no Yabō: Zenkokuban",
        .boxartPath = "assets/images/systems/nes/nobunaga-no-yab-zenkokuban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nobunaga's Ambition",
        .boxartPath = "assets/images/systems/nes/nobunaga-s-ambition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nobunaga's Ambition II",
        .boxartPath = "assets/images/systems/nes/nobunaga-s-ambition-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "North & South",
        .boxartPath = "assets/images/systems/nes/north-south.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "North & South: Wakuwaku Nanboku Sensou",
        .boxartPath = "assets/images/systems/nes/north-south-wakuwaku-nanboku-sensou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nuts & Milk",
        .boxartPath = "assets/images/systems/nes/nuts-milk.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NY Nyankies",
        .boxartPath = "assets/images/systems/nes/ny-nyankies.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Obake no Q-tarō WanWan Panikku",
        .boxartPath = "assets/images/systems/nes/obake-no-q-tar-wanwan-panikku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Obocchamakun",
        .boxartPath = "assets/images/systems/nes/obocchamakun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Oeka Kids: Anpanman no Hiragana Daisuki",
        .boxartPath = "assets/images/systems/nes/oeka-kids-anpanman-no-hiragana-daisuki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Oeka Kids: Anpanman to Oekaki Shiyō‼",
        .boxartPath = "assets/images/systems/nes/oeka-kids-anpanman-to-oekaki-shiy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Oishinbo: Kyukyoku no Menu 3bon Syoubu",
        .boxartPath = "assets/images/systems/nes/oishinbo-kyukyoku-no-menu-3bon-syoubu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Okkotoshi Puzzle Tonjan!?",
        .boxartPath = "assets/images/systems/nes/okkotoshi-puzzle-tonjan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Olympus no Tatakai: Ai no Densetsu",
        .boxartPath = "assets/images/systems/nes/olympus-no-tatakai-ai-no-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Onyanko Town",
        .boxartPath = "assets/images/systems/nes/onyanko-town.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Operation: Secret Storm (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/operation-secret-storm-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Operation Wolf",
        .boxartPath = "assets/images/systems/nes/operation-wolf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Orb-3D",
        .boxartPath = "assets/images/systems/nes/orb-3d.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Osomatsu-kun: Back to the Me no Deppa no Maki",
        .boxartPath = "assets/images/systems/nes/osomatsu-kun-back-to-the-me-no-deppa-no-maki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Otaku no Seiza: An Adventure in the Otaku Galaxy",
        .boxartPath = "assets/images/systems/nes/otaku-no-seiza-an-adventure-in-the-otaku-galaxy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Othello",
        .boxartPath = "assets/images/systems/nes/othello.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Outlanders",
        .boxartPath = "assets/images/systems/nes/outlanders.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Over Horizon",
        .boxartPath = "assets/images/systems/nes/over-horizon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Overlord",
        .boxartPath = "assets/images/systems/nes/overlord.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "P.O.W.: Prisoners of War",
        .boxartPath = "assets/images/systems/nes/p-o-w-prisoners-of-war.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pac-Land",
        .boxartPath = "assets/images/systems/nes/pac-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachi-Slot Adventure 2",
        .boxartPath = "assets/images/systems/nes/pachi-slot-adventure-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachi-Slot Adventure 3: Bitaoshii 7 Kenzan!",
        .boxartPath = "assets/images/systems/nes/pachi-slot-adventure-3-bitaoshii-7-kenzan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachicom",
        .boxartPath = "assets/images/systems/nes/pachicom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachinko Daisakusen 1",
        .boxartPath = "assets/images/systems/nes/pachinko-daisakusen-1.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachinko Daisakusen 2",
        .boxartPath = "assets/images/systems/nes/pachinko-daisakusen-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachio-kun 2",
        .boxartPath = "assets/images/systems/nes/pachio-kun-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachio-kun 3",
        .boxartPath = "assets/images/systems/nes/pachio-kun-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachio-kun 4",
        .boxartPath = "assets/images/systems/nes/pachio-kun-4.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pachio-kun 5",
        .boxartPath = "assets/images/systems/nes/pachio-kun-5.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Palamedes",
        .boxartPath = "assets/images/systems/nes/palamedes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Palamedes II: Star Twinkles",
        .boxartPath = "assets/images/systems/nes/palamedes-ii-star-twinkles.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Panic Restaurant",
        .boxartPath = "assets/images/systems/nes/panic-restaurant.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paperboy",
        .boxartPath = "assets/images/systems/nes/paperboy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paperboy 2",
        .boxartPath = "assets/images/systems/nes/paperboy-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Parallel World",
        .boxartPath = "assets/images/systems/nes/parallel-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Parasol Henbē",
        .boxartPath = "assets/images/systems/nes/parasol-henb.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Parasol Stars: Rainbow Islands II",
        .boxartPath = "assets/images/systems/nes/parasol-stars-rainbow-islands-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paris-Dakar Rally Special!",
        .boxartPath = "assets/images/systems/nes/paris-dakar-rally-special.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Parodius",
        .boxartPath = "assets/images/systems/nes/parodius.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Parodius Da!",
        .boxartPath = "assets/images/systems/nes/parodius-da.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Peek-A-Boo Poker (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/peek-a-boo-poker-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Peepar Time",
        .boxartPath = "assets/images/systems/nes/peepar-time.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Penguin-kun WARS",
        .boxartPath = "assets/images/systems/nes/penguin-kun-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Perfect Bowling",
        .boxartPath = "assets/images/systems/nes/perfect-bowling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Perman: Enban wo Torikaese‼",
        .boxartPath = "assets/images/systems/nes/perman-enban-wo-torikaese.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Perman Part 2: Himitsu Kessha Madoodan wo Taose!",
        .boxartPath = "assets/images/systems/nes/perman-part-2-himitsu-kessha-madoodan-wo-taose.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pescatore",
        .boxartPath = "assets/images/systems/nes/pescatore.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pesterminator: The Western Exterminator (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/pesterminator-the-western-exterminator-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Peter Pan and the Pirates: The Revenge of Captain Hook",
        .boxartPath = "assets/images/systems/nes/peter-pan-and-the-pirates-the-revenge-of-captain-hook.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Phantom Air Mission",
        .boxartPath = "assets/images/systems/nes/phantom-air-mission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Phantom Fighter",
        .boxartPath = "assets/images/systems/nes/phantom-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pictionary",
        .boxartPath = "assets/images/systems/nes/pictionary.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pin•Bot",
        .boxartPath = "assets/images/systems/nes/pin-bot.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pinball",
        .boxartPath = "assets/images/systems/nes/pinball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pinball Quest",
        .boxartPath = "assets/images/systems/nes/pinball-quest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pipe Dream",
        .boxartPath = "assets/images/systems/nes/pipe-dream.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pirates!",
        .boxartPath = "assets/images/systems/nes/pirates.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pizza Pop!",
        .boxartPath = "assets/images/systems/nes/pizza-pop.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Plasma Ball",
        .boxartPath = "assets/images/systems/nes/plasma-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Platoon",
        .boxartPath = "assets/images/systems/nes/platoon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pocket Zaurus: Jū Ōken no Nazo",
        .boxartPath = "assets/images/systems/nes/pocket-zaurus-j-ken-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Poke Block",
        .boxartPath = "assets/images/systems/nes/poke-block.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Police Academy: The Video Game",
        .boxartPath = "assets/images/systems/nes/police-academy-the-video-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pooyan",
        .boxartPath = "assets/images/systems/nes/pooyan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Popeye",
        .boxartPath = "assets/images/systems/nes/popeye.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Popeye no Eigo Asobi",
        .boxartPath = "assets/images/systems/nes/popeye-no-eigo-asobi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pōtopia Renzoku Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/p-topia-renzoku-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Blade",
        .boxartPath = "assets/images/systems/nes/power-blade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Blade 2",
        .boxartPath = "assets/images/systems/nes/power-blade-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Blazer",
        .boxartPath = "assets/images/systems/nes/power-blazer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Punch II",
        .boxartPath = "assets/images/systems/nes/power-punch-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Soccer",
        .boxartPath = "assets/images/systems/nes/power-soccer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "P'radikus Conflict (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/p-radikus-conflict-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Predator",
        .boxartPath = "assets/images/systems/nes/predator.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "President no Sentaku",
        .boxartPath = "assets/images/systems/nes/president-no-sentaku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Prince of Persia",
        .boxartPath = "assets/images/systems/nes/prince-of-persia.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Princess Tomato in the Salad Kingdom",
        .boxartPath = "assets/images/systems/nes/princess-tomato-in-the-salad-kingdom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Sport Hockey",
        .boxartPath = "assets/images/systems/nes/pro-sport-hockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Wrestling",
        .boxartPath = "assets/images/systems/nes/pro-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Yakyuu Family Stadium",
        .boxartPath = "assets/images/systems/nes/pro-yakyuu-family-stadium.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Yakyū: Family Stadium '87",
        .boxartPath = "assets/images/systems/nes/pro-yaky-family-stadium-87.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Yakyū: Family Stadium '88 Nendoban",
        .boxartPath = "assets/images/systems/nes/pro-yaky-family-stadium-88-nendoban.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pro Yakyū? Satsujin Jiken!",
        .boxartPath = "assets/images/systems/nes/pro-yaky-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Probotector",
        .boxartPath = "assets/images/systems/nes/probotector.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Probotector II: Return of the Evil Forces",
        .boxartPath = "assets/images/systems/nes/probotector-ii-return-of-the-evil-forces.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Punch-Out‼",
        .boxartPath = "assets/images/systems/nes/punch-out.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Punch-Out‼ Featuring Mr. Dream",
        .boxartPath = "assets/images/systems/nes/punch-out-featuring-mr-dream.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Punisher",
        .boxartPath = "assets/images/systems/nes/the-punisher.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Puss 'n Boots: Pero's Great Adventure",
        .boxartPath = "assets/images/systems/nes/puss-n-boots-pero-s-great-adventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Putt Master",
        .boxartPath = "assets/images/systems/nes/putt-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Puyo Puyo",
        .boxartPath = "assets/images/systems/nes/puyo-puyo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Puzslot",
        .boxartPath = "assets/images/systems/nes/puzslot.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Puzzle (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/puzzle-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Puzznic",
        .boxartPath = "assets/images/systems/nes/puzznic.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pyokotan no Dai Meiro",
        .boxartPath = "assets/images/systems/nes/pyokotan-no-dai-meiro.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pyramid (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/pyramid-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pyross",
        .boxartPath = "assets/images/systems/nes/pyross.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Q*bert",
        .boxartPath = "assets/images/systems/nes/q-bert.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Qix",
        .boxartPath = "assets/images/systems/nes/qix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quarter Back Scramble: American Football Game",
        .boxartPath = "assets/images/systems/nes/quarter-back-scramble-american-football-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quarth",
        .boxartPath = "assets/images/systems/nes/quarth.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quattro Adventure (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/quattro-adventure-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quattro Arcade (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/quattro-arcade-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quattro Sports (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/quattro-sports-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quinty",
        .boxartPath = "assets/images/systems/nes/quinty.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quiz Project Q: Cutie Project & Battle 10000",
        .boxartPath = "assets/images/systems/nes/quiz-project-q-cutie-project-battle-10000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R.B.I. Baseball",
        .boxartPath = "assets/images/systems/nes/r-b-i-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R.B.I. Baseball 2 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/r-b-i-baseball-2-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R.B.I. Baseball 3 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/r-b-i-baseball-3-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R.C. Pro-Am",
        .boxartPath = "assets/images/systems/nes/r-c-pro-am.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R.C. Pro-Am II",
        .boxartPath = "assets/images/systems/nes/r-c-pro-am-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Racer Mini Yonku: Japan Cup",
        .boxartPath = "assets/images/systems/nes/racer-mini-yonku-japan-cup.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Racket Attack",
        .boxartPath = "assets/images/systems/nes/racket-attack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rackets & Rivals",
        .boxartPath = "assets/images/systems/nes/rackets-rivals.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rad Racer",
        .boxartPath = "assets/images/systems/nes/rad-racer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rad Racer II",
        .boxartPath = "assets/images/systems/nes/rad-racer-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rad Racket: Deluxe Tennis II (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/rad-racket-deluxe-tennis-ii-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Radia Senki: Reimeihen",
        .boxartPath = "assets/images/systems/nes/radia-senki-reimeihen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Raf World",
        .boxartPath = "assets/images/systems/nes/raf-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Raid 2020 (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/raid-2020-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Raid on Bungeling Bay",
        .boxartPath = "assets/images/systems/nes/raid-on-bungeling-bay.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rainbow Islands",
        .boxartPath = "assets/images/systems/nes/rainbow-islands.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rainbow Islands: Bubble Bobble 2",
        .boxartPath = "assets/images/systems/nes/rainbow-islands-bubble-bobble-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rainbow Islands: The Story of Bubble Bobble 2",
        .boxartPath = "assets/images/systems/nes/rainbow-islands-the-story-of-bubble-bobble-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rally Bike",
        .boxartPath = "assets/images/systems/nes/rally-bike.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rambo",
        .boxartPath = "assets/images/systems/nes/rambo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rampage",
        .boxartPath = "assets/images/systems/nes/rampage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rampart",
        .boxartPath = "assets/images/systems/nes/rampart.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Red Arremer II",
        .boxartPath = "assets/images/systems/nes/red-arremer-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Reigen Doushi",
        .boxartPath = "assets/images/systems/nes/reigen-doushi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Remote Control",
        .boxartPath = "assets/images/systems/nes/remote-control.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Ren & Stimpy Show: Buckaroo$!",
        .boxartPath = "assets/images/systems/nes/the-ren-stimpy-show-buckaroo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Renegade",
        .boxartPath = "assets/images/systems/nes/renegade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rescue: The Embassy Mission",
        .boxartPath = "assets/images/systems/nes/rescue-the-embassy-mission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Return of Donkey Kong",
        .boxartPath = "assets/images/systems/nes/return-of-donkey-kong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ring King",
        .boxartPath = "assets/images/systems/nes/ring-king.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ripple Island",
        .boxartPath = "assets/images/systems/nes/ripple-island.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "River City Ransom",
        .boxartPath = "assets/images/systems/nes/river-city-ransom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Road Runner (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/road-runner-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RoadBlasters",
        .boxartPath = "assets/images/systems/nes/roadblasters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robert Byrne's Pool Challenge",
        .boxartPath = "assets/images/systems/nes/robert-byrne-s-pool-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robin Hood: Prince of Thieves",
        .boxartPath = "assets/images/systems/nes/robin-hood-prince-of-thieves.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robocco Wars",
        .boxartPath = "assets/images/systems/nes/robocco-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RoboCop",
        .boxartPath = "assets/images/systems/nes/robocop.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RoboCop 2",
        .boxartPath = "assets/images/systems/nes/robocop-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RoboCop 3",
        .boxartPath = "assets/images/systems/nes/robocop-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RoboCop Versus The Terminator",
        .boxartPath = "assets/images/systems/nes/robocop-versus-the-terminator.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robodemons (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/robodemons-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robowarrior",
        .boxartPath = "assets/images/systems/nes/robowarrior.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rock 'n Ball",
        .boxartPath = "assets/images/systems/nes/rock-n-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rocket Ranger",
        .boxartPath = "assets/images/systems/nes/rocket-ranger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Rocketeer",
        .boxartPath = "assets/images/systems/nes/the-rocketeer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rockin' Kats",
        .boxartPath = "assets/images/systems/nes/rockin-kats.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan",
        .boxartPath = "assets/images/systems/nes/rockman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan 2: Dr. Wily no Nazo",
        .boxartPath = "assets/images/systems/nes/rockman-2-dr-wily-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan 3: Dr. Wily no Saigo!?",
        .boxartPath = "assets/images/systems/nes/rockman-3-dr-wily-no-saigo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan 4: Aratanaru Yabou‼",
        .boxartPath = "assets/images/systems/nes/rockman-4-aratanaru-yabou.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan 5: Blues no Wana!?",
        .boxartPath = "assets/images/systems/nes/rockman-5-blues-no-wana.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RockMan 6: Shijō Saidai no Tatakai!!",
        .boxartPath = "assets/images/systems/nes/rockman-6-shij-saidai-no-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RodLand",
        .boxartPath = "assets/images/systems/nes/rodland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Roger Clemens' MVP Baseball",
        .boxartPath = "assets/images/systems/nes/roger-clemens-mvp-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rokudenashi Blues",
        .boxartPath = "assets/images/systems/nes/rokudenashi-blues.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rollerball",
        .boxartPath = "assets/images/systems/nes/rollerball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rollerblade Racer",
        .boxartPath = "assets/images/systems/nes/rollerblade-racer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RollerGames",
        .boxartPath = "assets/images/systems/nes/rollergames.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rolling Thunder (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/rolling-thunder-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Romance of the Three Kingdoms",
        .boxartPath = "assets/images/systems/nes/romance-of-the-three-kingdoms.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Romance of the Three Kingdoms II",
        .boxartPath = "assets/images/systems/nes/romance-of-the-three-kingdoms-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Romancia",
        .boxartPath = "assets/images/systems/nes/romancia.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Roundball: 2 on 2 Challenge",
        .boxartPath = "assets/images/systems/nes/roundball-2-on-2-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Route-16 Turbo",
        .boxartPath = "assets/images/systems/nes/route-16-turbo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Royal Blood",
        .boxartPath = "assets/images/systems/nes/royal-blood.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RPG Jinsei Game",
        .boxartPath = "assets/images/systems/nes/rpg-jinsei-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rush'n Attack",
        .boxartPath = "assets/images/systems/nes/rush-n-attack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rygar",
        .boxartPath = "assets/images/systems/nes/rygar.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "S.C.A.T.: Special Cybernetic Attack Team",
        .boxartPath = "assets/images/systems/nes/s-c-a-t-special-cybernetic-attack-team.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Saint Seiya: Ōgon Densetsu",
        .boxartPath = "assets/images/systems/nes/saint-seiya-gon-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Saint Seiya: Ōgon Densetsu Kanketsu Hen",
        .boxartPath = "assets/images/systems/nes/saint-seiya-gon-densetsu-kanketsu-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Saiyūki World",
        .boxartPath = "assets/images/systems/nes/saiy-ki-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Saiyuuki World 2: Tenjoukai no Majin",
        .boxartPath = "assets/images/systems/nes/saiyuuki-world-2-tenjoukai-no-majin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sakigake‼ Otokojuku: Shippu Ichi Gō Sei",
        .boxartPath = "assets/images/systems/nes/sakigake-otokojuku-shippu-ichi-g-sei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Salad no Kuni no Tomato Hime",
        .boxartPath = "assets/images/systems/nes/salad-no-kuni-no-tomato-hime.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Salamander",
        .boxartPath = "assets/images/systems/nes/salamander.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "San Goku Shi",
        .boxartPath = "assets/images/systems/nes/san-goku-shi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "San Goku Shi II",
        .boxartPath = "assets/images/systems/nes/san-goku-shi-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sanada Jū Yūshi",
        .boxartPath = "assets/images/systems/nes/sanada-j-y-shi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sangokushi: Chūgen no Hasha",
        .boxartPath = "assets/images/systems/nes/sangokushi-ch-gen-no-hasha.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sangokushi II: Haō no Tairiku",
        .boxartPath = "assets/images/systems/nes/sangokushi-ii-ha-no-tairiku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sanma no Meitantei",
        .boxartPath = "assets/images/systems/nes/sanma-no-meitantei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sanrio Carnival",
        .boxartPath = "assets/images/systems/nes/sanrio-carnival.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sanrio Carnival 2",
        .boxartPath = "assets/images/systems/nes/sanrio-carnival-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sanrio Cup: Pon Pon Volley",
        .boxartPath = "assets/images/systems/nes/sanrio-cup-pon-pon-volley.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sansara Naga",
        .boxartPath = "assets/images/systems/nes/sansara-naga.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Satomi Hakkenden",
        .boxartPath = "assets/images/systems/nes/satomi-hakkenden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Satoru Nakajima: F-1 Hero",
        .boxartPath = "assets/images/systems/nes/satoru-nakajima-f-1-hero.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Satoru Nakajima F-1 Hero 2",
        .boxartPath = "assets/images/systems/nes/satoru-nakajima-f-1-hero-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Satsui no Kaisō: Soft House Renzoku Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/satsui-no-kais-soft-house-renzoku-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Battle Ōzumō: Heisei Hero Basho",
        .boxartPath = "assets/images/systems/nes/sd-battle-zum-heisei-hero-basho.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam Gaiden: Knight Gundam Monogatari",
        .boxartPath = "assets/images/systems/nes/sd-gundam-gaiden-knight-gundam-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam Gaiden: Knight Gundam Monogatari 2 - Knight of Light",
        .boxartPath = "assets/images/systems/nes/sd-gundam-gaiden-knight-gundam-monogatari-2-knight-of-light.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam Gaiden: Knight Gundam Monogatari 3 - The Legendary Knights",
        .boxartPath = "assets/images/systems/nes/sd-gundam-gaiden-knight-gundam-monogatari-3-the-legendary-knights.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam: Gundam Wars",
        .boxartPath = "assets/images/systems/nes/sd-gundam-gundam-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam World: Capsule Warriors 2 – Capsule Warfare",
        .boxartPath = "assets/images/systems/nes/sd-gundam-world-capsule-warriors-2-capsule-warfare.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam World: Capsule Warriors 3 – Hero Warfare",
        .boxartPath = "assets/images/systems/nes/sd-gundam-world-capsule-warriors-3-hero-warfare.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam World: Capsule Warriors 4 – New Type Story",
        .boxartPath = "assets/images/systems/nes/sd-gundam-world-capsule-warriors-4-new-type-story.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Gundam World: Capsule Warriors 5 – Battle of Universal Century",
        .boxartPath = "assets/images/systems/nes/sd-gundam-world-capsule-warriors-5-battle-of-universal-century.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Hero Sōkessen: Taose! Aku no Gundan",
        .boxartPath = "assets/images/systems/nes/sd-hero-s-kessen-taose-aku-no-gundan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Keiji: Blader",
        .boxartPath = "assets/images/systems/nes/sd-keiji-blader.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SD Sengoku Bushō Retsuden: Rekka no Gotoku Tenka o Nusure!",
        .boxartPath = "assets/images/systems/nes/sd-sengoku-bush-retsuden-rekka-no-gotoku-tenka-o-nusure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Secret Scout in the Temple of Demise (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/secret-scout-in-the-temple-of-demise-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Secret Ties",
        .boxartPath = "assets/images/systems/nes/secret-ties.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Section-Z",
        .boxartPath = "assets/images/systems/nes/section-z.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Seicross",
        .boxartPath = "assets/images/systems/nes/seicross.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Seikima II Akuma no Gyakushū!",
        .boxartPath = "assets/images/systems/nes/seikima-ii-akuma-no-gyakush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Seirei Densetsu Lickle",
        .boxartPath = "assets/images/systems/nes/seirei-densetsu-lickle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Seirei Gari",
        .boxartPath = "assets/images/systems/nes/seirei-gari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Seiryaku Simulation: Inbō no Wakusei: Shancara",
        .boxartPath = "assets/images/systems/nes/seiryaku-simulation-inb-no-wakusei-shancara.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sekiryūō",
        .boxartPath = "assets/images/systems/nes/sekiry.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Senjou no Ookami",
        .boxartPath = "assets/images/systems/nes/senjou-no-ookami.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sensha Senryaku: Sabaku no Kitsune",
        .boxartPath = "assets/images/systems/nes/sensha-senryaku-sabaku-no-kitsune.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: 123",
        .boxartPath = "assets/images/systems/nes/sesame-street-123.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: ABC",
        .boxartPath = "assets/images/systems/nes/sesame-street-abc.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: ABC & 123",
        .boxartPath = "assets/images/systems/nes/sesame-street-abc-123.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: Big Bird's Hide & Speak",
        .boxartPath = "assets/images/systems/nes/sesame-street-big-bird-s-hide-speak.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: Countdown",
        .boxartPath = "assets/images/systems/nes/sesame-street-countdown.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadow Brain",
        .boxartPath = "assets/images/systems/nes/shadow-brain.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadow of the Ninja",
        .boxartPath = "assets/images/systems/nes/shadow-of-the-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadow Warriors",
        .boxartPath = "assets/images/systems/nes/shadow-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadow Warriors II: The Dark Sword of Chaos",
        .boxartPath = "assets/images/systems/nes/shadow-warriors-ii-the-dark-sword-of-chaos.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadowgate",
        .boxartPath = "assets/images/systems/nes/shadowgate.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shanghai",
        .boxartPath = "assets/images/systems/nes/shanghai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shanghai II",
        .boxartPath = "assets/images/systems/nes/shanghai-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shatterhand",
        .boxartPath = "assets/images/systems/nes/shatterhand.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sherlock Holmes: Hakushaku Reijō Yūkai Jiken",
        .boxartPath = "assets/images/systems/nes/sherlock-holmes-hakushaku-reij-y-kai-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shikinjō",
        .boxartPath = "assets/images/systems/nes/shikinj.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shin 4nin Uchi Mahjong: Yakuman Tengoku",
        .boxartPath = "assets/images/systems/nes/shin-4nin-uchi-mahjong-yakuman-tengoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shin Moero‼ Pro Yakyū",
        .boxartPath = "assets/images/systems/nes/shin-moero-pro-yaky.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shin Satomi Hakkenden: Hikari to Yami no Tatakai",
        .boxartPath = "assets/images/systems/nes/shin-satomi-hakkenden-hikari-to-yami-no-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shingen the Ruler",
        .boxartPath = "assets/images/systems/nes/shingen-the-ruler.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shinjinrui: The New Type",
        .boxartPath = "assets/images/systems/nes/shinjinrui-the-new-type.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shinobi (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/shinobi-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shinsenden",
        .boxartPath = "assets/images/systems/nes/shinsenden.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "ShockWave (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/shockwave-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shogi Meikan '92",
        .boxartPath = "assets/images/systems/nes/shogi-meikan-92.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shogi Meikan '93",
        .boxartPath = "assets/images/systems/nes/shogi-meikan-93.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shogun",
        .boxartPath = "assets/images/systems/nes/shogun.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shooting Range",
        .boxartPath = "assets/images/systems/nes/shooting-range.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Short Order + Eggsplode!",
        .boxartPath = "assets/images/systems/nes/short-order-eggsplode.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shōkōshi Cedie",
        .boxartPath = "assets/images/systems/nes/sh-k-shi-cedie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shōnen Ashibe: Nepal Daibōken no Maki",
        .boxartPath = "assets/images/systems/nes/sh-nen-ashibe-nepal-daib-ken-no-maki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shuffle Fight",
        .boxartPath = "assets/images/systems/nes/shuffle-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shufflepuck Café",
        .boxartPath = "assets/images/systems/nes/shufflepuck-caf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Side Pocket",
        .boxartPath = "assets/images/systems/nes/side-pocket.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Silent Assault (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/silent-assault-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Silent Service",
        .boxartPath = "assets/images/systems/nes/silent-service.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Silkworm",
        .boxartPath = "assets/images/systems/nes/silkworm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Silva Saga",
        .boxartPath = "assets/images/systems/nes/silva-saga.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Silver Surfer",
        .boxartPath = "assets/images/systems/nes/silver-surfer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SimCity",
        .boxartPath = "assets/images/systems/nes/simcity.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Skate or Die",
        .boxartPath = "assets/images/systems/nes/skate-or-die.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Skate or Die 2: The Search for Double Trouble",
        .boxartPath = "assets/images/systems/nes/skate-or-die-2-the-search-for-double-trouble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ski or Die",
        .boxartPath = "assets/images/systems/nes/ski-or-die.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Skull & Crossbones (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/skull-crossbones-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sky Destroyer",
        .boxartPath = "assets/images/systems/nes/sky-destroyer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sky Kid",
        .boxartPath = "assets/images/systems/nes/sky-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sky Shark",
        .boxartPath = "assets/images/systems/nes/sky-shark.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Slalom",
        .boxartPath = "assets/images/systems/nes/slalom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Smash TV",
        .boxartPath = "assets/images/systems/nes/smash-tv.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Smurfs",
        .boxartPath = "assets/images/systems/nes/the-smurfs.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snake Rattle 'n' Roll",
        .boxartPath = "assets/images/systems/nes/snake-rattle-n-roll.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snake's Revenge",
        .boxartPath = "assets/images/systems/nes/snake-s-revenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snoopy's Silly Sports Spectacular!",
        .boxartPath = "assets/images/systems/nes/snoopy-s-silly-sports-spectacular.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snow Bros.: Nick & Tom",
        .boxartPath = "assets/images/systems/nes/snow-bros-nick-tom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snow Brothers",
        .boxartPath = "assets/images/systems/nes/snow-brothers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snowboard Challenge",
        .boxartPath = "assets/images/systems/nes/snowboard-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Soccer",
        .boxartPath = "assets/images/systems/nes/soccer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Soccer League - Winner's Cup",
        .boxartPath = "assets/images/systems/nes/soccer-league-winner-s-cup.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Softball Tengoku",
        .boxartPath = "assets/images/systems/nes/softball-tengoku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solar Jetman: Hunt for the Golden Warpship",
        .boxartPath = "assets/images/systems/nes/solar-jetman-hunt-for-the-golden-warpship.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solitaire (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/solitaire-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solomon no Kagi",
        .boxartPath = "assets/images/systems/nes/solomon-no-kagi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solomon's Key",
        .boxartPath = "assets/images/systems/nes/solomon-s-key.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solomon's Key 2",
        .boxartPath = "assets/images/systems/nes/solomon-s-key-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solomon's Key 2: Ku-Rumintō Kyūshutsu Sakusen",
        .boxartPath = "assets/images/systems/nes/solomon-s-key-2-ku-rumint-ky-shutsu-sakusen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solstice: Sanjigen Meikyuu no Kyoujuu",
        .boxartPath = "assets/images/systems/nes/solstice-sanjigen-meikyuu-no-kyoujuu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Solstice: The Quest for the Staff of Demnos",
        .boxartPath = "assets/images/systems/nes/solstice-the-quest-for-the-staff-of-demnos.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SonSon",
        .boxartPath = "assets/images/systems/nes/sonson.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Soreike! Anpanman: Minna de Hiking Game!",
        .boxartPath = "assets/images/systems/nes/soreike-anpanman-minna-de-hiking-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Harrier",
        .boxartPath = "assets/images/systems/nes/space-harrier.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Hunter",
        .boxartPath = "assets/images/systems/nes/space-hunter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Invaders",
        .boxartPath = "assets/images/systems/nes/space-invaders.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Shadow",
        .boxartPath = "assets/images/systems/nes/space-shadow.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Shuttle Project",
        .boxartPath = "assets/images/systems/nes/space-shuttle-project.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spartan X",
        .boxartPath = "assets/images/systems/nes/spartan-x.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spartan X 2",
        .boxartPath = "assets/images/systems/nes/spartan-x-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spelunker",
        .boxartPath = "assets/images/systems/nes/spelunker.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spelunker II: Yūsha e no Chōsen",
        .boxartPath = "assets/images/systems/nes/spelunker-ii-y-sha-e-no-ch-sen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spider-Man: Return of the Sinister Six",
        .boxartPath = "assets/images/systems/nes/spider-man-return-of-the-sinister-six.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spiritual Warfare (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/spiritual-warfare-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Splatterhouse: Wanpaku Graffiti",
        .boxartPath = "assets/images/systems/nes/splatterhouse-wanpaku-graffiti.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spot: The Game",
        .boxartPath = "assets/images/systems/nes/spot-the-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spot: The Video Game",
        .boxartPath = "assets/images/systems/nes/spot-the-video-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spy Hunter",
        .boxartPath = "assets/images/systems/nes/spy-hunter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spy vs. Spy",
        .boxartPath = "assets/images/systems/nes/spy-vs-spy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sqoon",
        .boxartPath = "assets/images/systems/nes/sqoon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Square no Tom Sawyer",
        .boxartPath = "assets/images/systems/nes/square-no-tom-sawyer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stack-Up",
        .boxartPath = "assets/images/systems/nes/stack-up.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stadium Events",
        .boxartPath = "assets/images/systems/nes/stadium-events.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stanley: The Search for Dr. Livingston",
        .boxartPath = "assets/images/systems/nes/stanley-the-search-for-dr-livingston.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Force",
        .boxartPath = "assets/images/systems/nes/star-force.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Luster",
        .boxartPath = "assets/images/systems/nes/star-luster.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Soldier",
        .boxartPath = "assets/images/systems/nes/star-soldier.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Trek: 25th Anniversary",
        .boxartPath = "assets/images/systems/nes/star-trek-25th-anniversary.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Trek: The Next Generation",
        .boxartPath = "assets/images/systems/nes/star-trek-the-next-generation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Trek V: The Final Frontier",
        .boxartPath = "assets/images/systems/nes/star-trek-v-the-final-frontier.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Voyager",
        .boxartPath = "assets/images/systems/nes/star-voyager.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars",
        .boxartPath = "assets/images/systems/nes/star-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars: Teikoku no Gyakushuu",
        .boxartPath = "assets/images/systems/nes/star-wars-teikoku-no-gyakushuu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars: The Empire Strikes Back",
        .boxartPath = "assets/images/systems/nes/star-wars-the-empire-strikes-back.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stargate",
        .boxartPath = "assets/images/systems/nes/stargate.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Starship Hector",
        .boxartPath = "assets/images/systems/nes/starship-hector.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "StarTropics",
        .boxartPath = "assets/images/systems/nes/startropics.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stealth ATF",
        .boxartPath = "assets/images/systems/nes/stealth-atf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "STED: Iseki Wakusei no Yabō",
        .boxartPath = "assets/images/systems/nes/sted-iseki-wakusei-no-yab.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stick Hunter: Exciting Ice Hockey",
        .boxartPath = "assets/images/systems/nes/stick-hunter-exciting-ice-hockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stinger",
        .boxartPath = "assets/images/systems/nes/stinger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Street Cop",
        .boxartPath = "assets/images/systems/nes/street-cop.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Street Fighter 2010: The Final Fight",
        .boxartPath = "assets/images/systems/nes/street-fighter-2010-the-final-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Street Gangs",
        .boxartPath = "assets/images/systems/nes/street-gangs.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Strider",
        .boxartPath = "assets/images/systems/nes/strider.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Stunt Kids (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/stunt-kids-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sugoro Quest: Dice no Senshi Tachi",
        .boxartPath = "assets/images/systems/nes/sugoro-quest-dice-no-senshi-tachi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Suikoden: Tenmei no Chikai",
        .boxartPath = "assets/images/systems/nes/suikoden-tenmei-no-chikai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sukeban Deka III",
        .boxartPath = "assets/images/systems/nes/sukeban-deka-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Summer Carnival '92: Recca",
        .boxartPath = "assets/images/systems/nes/summer-carnival-92-recca.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sunday Funday: The Ride (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/sunday-funday-the-ride-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sunman",
        .boxartPath = "assets/images/systems/nes/sunman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Adventure Quests",
        .boxartPath = "assets/images/systems/nes/super-adventure-quests.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Arabian",
        .boxartPath = "assets/images/systems/nes/super-arabian.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Black Onyx",
        .boxartPath = "assets/images/systems/nes/super-black-onyx.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Cars",
        .boxartPath = "assets/images/systems/nes/super-cars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Chinese",
        .boxartPath = "assets/images/systems/nes/super-chinese.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Chinese 2: Dragon Kid",
        .boxartPath = "assets/images/systems/nes/super-chinese-2-dragon-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Chinese 3",
        .boxartPath = "assets/images/systems/nes/super-chinese-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Contra",
        .boxartPath = "assets/images/systems/nes/super-contra.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Dodge Ball",
        .boxartPath = "assets/images/systems/nes/super-dodge-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Dog Booby",
        .boxartPath = "assets/images/systems/nes/super-dog-booby.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Dyna'mix Badminton",
        .boxartPath = "assets/images/systems/nes/super-dyna-mix-badminton.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Glove Ball",
        .boxartPath = "assets/images/systems/nes/super-glove-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Jeopardy!",
        .boxartPath = "assets/images/systems/nes/super-jeopardy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. 2",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. 3",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. & Duck Hunt",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-duck-hunt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. / Duck Hunt",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-duck-hunt-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. / Duck Hunt / World Class Track Meet",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-duck-hunt-world-class-track-meet.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Bros. + Tetris + Nintendo World Cup",
        .boxartPath = "assets/images/systems/nes/super-mario-bros-tetris-nintendo-world-cup.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario USA",
        .boxartPath = "assets/images/systems/nes/super-mario-usa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mogura Tataki‼ Pokkun Mogura",
        .boxartPath = "assets/images/systems/nes/super-mogura-tataki-pokkun-mogura.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Momotaro Dentetsu",
        .boxartPath = "assets/images/systems/nes/super-momotaro-dentetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Pinball",
        .boxartPath = "assets/images/systems/nes/super-pinball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Pitfall",
        .boxartPath = "assets/images/systems/nes/super-pitfall.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Real Baseball '88",
        .boxartPath = "assets/images/systems/nes/super-real-baseball-88.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Rugby",
        .boxartPath = "assets/images/systems/nes/super-rugby.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Spike V'Ball",
        .boxartPath = "assets/images/systems/nes/super-spike-v-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Spike V'Ball / Nintendo World Cup",
        .boxartPath = "assets/images/systems/nes/super-spike-v-ball-nintendo-world-cup.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Sports Challenge",
        .boxartPath = "assets/images/systems/nes/super-sports-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Sprint (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/super-sprint-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Spy Hunter",
        .boxartPath = "assets/images/systems/nes/super-spy-hunter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Star Force: Jikūreki no Himitsu",
        .boxartPath = "assets/images/systems/nes/super-star-force-jik-reki-no-himitsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Star Pro Wrestling",
        .boxartPath = "assets/images/systems/nes/super-star-pro-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Team Games",
        .boxartPath = "assets/images/systems/nes/super-team-games.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Turrican",
        .boxartPath = "assets/images/systems/nes/super-turrican.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Xevious: GAMP no Nazo",
        .boxartPath = "assets/images/systems/nes/super-xevious-gamp-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Superman",
        .boxartPath = "assets/images/systems/nes/superman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Swamp Thing",
        .boxartPath = "assets/images/systems/nes/swamp-thing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SWAT: Special Weapons and Tactics",
        .boxartPath = "assets/images/systems/nes/swat-special-weapons-and-tactics.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sweet Home",
        .boxartPath = "assets/images/systems/nes/sweet-home.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sword Master",
        .boxartPath = "assets/images/systems/nes/sword-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Swords and Serpents",
        .boxartPath = "assets/images/systems/nes/swords-and-serpents.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Taboo: The Sixth Sense",
        .boxartPath = "assets/images/systems/nes/taboo-the-sixth-sense.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tag Team Match: M.U.S.C.L.E.",
        .boxartPath = "assets/images/systems/nes/tag-team-match-m-u-s-c-l-e.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tag Team Pro Wrestling",
        .boxartPath = "assets/images/systems/nes/tag-team-pro-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tag Team Wrestling",
        .boxartPath = "assets/images/systems/nes/tag-team-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tagin' Dragon (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/tagin-dragon-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Taito Basketball",
        .boxartPath = "assets/images/systems/nes/taito-basketball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Taito Grand Prix: Eikō Heno Licence",
        .boxartPath = "assets/images/systems/nes/taito-grand-prix-eik-heno-licence.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Taiyou no Shinden: Asteka II",
        .boxartPath = "assets/images/systems/nes/taiyou-no-shinden-asteka-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Taiyō no Yūsha Fighbird",
        .boxartPath = "assets/images/systems/nes/taiy-no-y-sha-fighbird.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takahashi Meijin no Bouken Jima",
        .boxartPath = "assets/images/systems/nes/takahashi-meijin-no-bouken-jima.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takahashi Meijin no Bouken Jima II",
        .boxartPath = "assets/images/systems/nes/takahashi-meijin-no-bouken-jima-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takahashi Meijin no Bouken Jima III",
        .boxartPath = "assets/images/systems/nes/takahashi-meijin-no-bouken-jima-iii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takahashi Meijin no Bōken Jima IV",
        .boxartPath = "assets/images/systems/nes/takahashi-meijin-no-b-ken-jima-iv.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takahashi Meijin no Bug-tte Honey",
        .boxartPath = "assets/images/systems/nes/takahashi-meijin-no-bug-tte-honey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takeda Shingen",
        .boxartPath = "assets/images/systems/nes/takeda-shingen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takeda Shingen 2",
        .boxartPath = "assets/images/systems/nes/takeda-shingen-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takeshi no Chōsenjō",
        .boxartPath = "assets/images/systems/nes/takeshi-no-ch-senj.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Takeshi no Sengoku Fūunji",
        .boxartPath = "assets/images/systems/nes/takeshi-no-sengoku-f-unji.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "TaleSpin",
        .boxartPath = "assets/images/systems/nes/talespin.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tamura Mitsuaki no Mahjong Seminar",
        .boxartPath = "assets/images/systems/nes/tamura-mitsuaki-no-mahjong-seminar.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tanigawa Kōji no Shōgi Shinan II: Meijin e no Michi",
        .boxartPath = "assets/images/systems/nes/tanigawa-k-ji-no-sh-gi-shinan-ii-meijin-e-no-michi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tantei Jingūji Saburō: Toki no Sugiyuku Mama ni",
        .boxartPath = "assets/images/systems/nes/tantei-jing-ji-sabur-toki-no-sugiyuku-mama-ni.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tantei Jingūji Saburō: Yokohama-kō Renzoku Satsujin Jiken",
        .boxartPath = "assets/images/systems/nes/tantei-jing-ji-sabur-yokohama-k-renzoku-satsujin-jiken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tao",
        .boxartPath = "assets/images/systems/nes/tao.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Target: Renegade",
        .boxartPath = "assets/images/systems/nes/target-renegade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tashiro Masashi no Princess ga Ippai",
        .boxartPath = "assets/images/systems/nes/tashiro-masashi-no-princess-ga-ippai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tatakae‼ Ramenman: Sakuretsu Chōjin 102 Gei",
        .boxartPath = "assets/images/systems/nes/tatakae-ramenman-sakuretsu-ch-jin-102-gei.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tatakai no Banka",
        .boxartPath = "assets/images/systems/nes/tatakai-no-banka.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tech Town",
        .boxartPath = "assets/images/systems/nes/tech-town.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo Baseball",
        .boxartPath = "assets/images/systems/nes/tecmo-baseball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo Bowl",
        .boxartPath = "assets/images/systems/nes/tecmo-bowl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo Cup Football Game",
        .boxartPath = "assets/images/systems/nes/tecmo-cup-football-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo Cup Soccer Game",
        .boxartPath = "assets/images/systems/nes/tecmo-cup-soccer-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo NBA Basketball",
        .boxartPath = "assets/images/systems/nes/tecmo-nba-basketball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo Super Bowl",
        .boxartPath = "assets/images/systems/nes/tecmo-super-bowl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo World Cup Soccer",
        .boxartPath = "assets/images/systems/nes/tecmo-world-cup-soccer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tecmo World Wrestling",
        .boxartPath = "assets/images/systems/nes/tecmo-world-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Hero Turtles",
        .boxartPath = "assets/images/systems/nes/teenage-mutant-hero-turtles.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Hero Turtles II: The Arcade Game",
        .boxartPath = "assets/images/systems/nes/teenage-mutant-hero-turtles-ii-the-arcade-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles 2: The Manhattan Project",
        .boxartPath = "assets/images/systems/nes/teenage-mutant-ninja-turtles-2-the-manhattan-project.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tenchi wo Kurau",
        .boxartPath = "assets/images/systems/nes/tenchi-wo-kurau.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tenchi wo Kurau II: Shokatsu Kōmei Den",
        .boxartPath = "assets/images/systems/nes/tenchi-wo-kurau-ii-shokatsu-k-mei-den.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tenka no Goikenban: Mito Kōmon",
        .boxartPath = "assets/images/systems/nes/tenka-no-goikenban-mito-k-mon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tenkaichi Bushi Keru Nagūru",
        .boxartPath = "assets/images/systems/nes/tenkaichi-bushi-keru-nag-ru.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tennis",
        .boxartPath = "assets/images/systems/nes/tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Terao no Dosukoi Ōzumō",
        .boxartPath = "assets/images/systems/nes/terao-no-dosukoi-zum.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Terminator",
        .boxartPath = "assets/images/systems/nes/the-terminator.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Terminator 2",
        .boxartPath = "assets/images/systems/nes/terminator-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Terminator 2: Judgment Day",
        .boxartPath = "assets/images/systems/nes/terminator-2-judgment-day.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Terminator 2: The Arcade Game",
        .boxartPath = "assets/images/systems/nes/terminator-2-the-arcade-game.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Terra Cresta",
        .boxartPath = "assets/images/systems/nes/terra-cresta.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetra Star: The Fighter",
        .boxartPath = "assets/images/systems/nes/tetra-star-the-fighter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetris (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/tetris-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetris 2",
        .boxartPath = "assets/images/systems/nes/tetris-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetris 2 + Bombliss",
        .boxartPath = "assets/images/systems/nes/tetris-2-bombliss.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetris Flash",
        .boxartPath = "assets/images/systems/nes/tetris-flash.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetris (Nintendo)",
        .boxartPath = "assets/images/systems/nes/tetris-nintendo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetsudō Ō",
        .boxartPath = "assets/images/systems/nes/tetsud.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetsuwan Atom",
        .boxartPath = "assets/images/systems/nes/tetsuwan-atom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Thexder",
        .boxartPath = "assets/images/systems/nes/thexder.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Thomas the Tank Engine & Friends",
        .boxartPath = "assets/images/systems/nes/thomas-the-tank-engine-friends.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Three Stooges",
        .boxartPath = "assets/images/systems/nes/the-three-stooges.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Thunder & Lightning",
        .boxartPath = "assets/images/systems/nes/thunder-lightning.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Thunderbirds",
        .boxartPath = "assets/images/systems/nes/thunderbirds.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Thundercade",
        .boxartPath = "assets/images/systems/nes/thundercade.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiles of Fate (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/tiles-of-fate-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Time Lord",
        .boxartPath = "assets/images/systems/nes/time-lord.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Time Zone",
        .boxartPath = "assets/images/systems/nes/time-zone.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Times of Lore",
        .boxartPath = "assets/images/systems/nes/times-of-lore.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tiny Toon Adventures 2: Montana Land e Youkoso",
        .boxartPath = "assets/images/systems/nes/tiny-toon-adventures-2-montana-land-e-youkoso.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Titan",
        .boxartPath = "assets/images/systems/nes/titan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Titan Warriors",
        .boxartPath = "assets/images/systems/nes/titan-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "TM Network: Live in Power Bowl",
        .boxartPath = "assets/images/systems/nes/tm-network-live-in-power-bowl.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "To the Earth",
        .boxartPath = "assets/images/systems/nes/to-the-earth.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Toki",
        .boxartPath = "assets/images/systems/nes/toki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Toki no Tabibito: Time Stranger",
        .boxartPath = "assets/images/systems/nes/toki-no-tabibito-time-stranger.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tokkyuu Shirei Solbrain",
        .boxartPath = "assets/images/systems/nes/tokkyuu-shirei-solbrain.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tokoro-san no Mamoru mo Semeru mo",
        .boxartPath = "assets/images/systems/nes/tokoro-san-no-mamoru-mo-semeru-mo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tokyo Pachi-Slot Adventure",
        .boxartPath = "assets/images/systems/nes/tokyo-pachi-slot-adventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tom and Jerry: The Ultimate Game of Cat and Mouse!",
        .boxartPath = "assets/images/systems/nes/tom-and-jerry-the-ultimate-game-of-cat-and-mouse.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tom Sawyer no Bouken",
        .boxartPath = "assets/images/systems/nes/tom-sawyer-no-bouken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tom to Jerry & Tuffy",
        .boxartPath = "assets/images/systems/nes/tom-to-jerry-tuffy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tombs & Treasure",
        .boxartPath = "assets/images/systems/nes/tombs-treasure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Toobin' (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/toobin-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gun: Dual Fighters",
        .boxartPath = "assets/images/systems/nes/top-gun-dual-fighters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gun: The Second Mission",
        .boxartPath = "assets/images/systems/nes/top-gun-the-second-mission.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Management",
        .boxartPath = "assets/images/systems/nes/top-management.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Players' Tennis",
        .boxartPath = "assets/images/systems/nes/top-players-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Rider",
        .boxartPath = "assets/images/systems/nes/top-rider.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Striker",
        .boxartPath = "assets/images/systems/nes/top-striker.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Total Recall",
        .boxartPath = "assets/images/systems/nes/total-recall.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Totally Rad",
        .boxartPath = "assets/images/systems/nes/totally-rad.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Totsuzen! Macho Man",
        .boxartPath = "assets/images/systems/nes/totsuzen-macho-man.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Touchdown Fever",
        .boxartPath = "assets/images/systems/nes/touchdown-fever.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tōhōken Bunroku",
        .boxartPath = "assets/images/systems/nes/t-h-ken-bunroku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tōjin Makyō Den: Heracles no Eikō",
        .boxartPath = "assets/images/systems/nes/t-jin-maky-den-heracles-no-eik.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tōkon Club",
        .boxartPath = "assets/images/systems/nes/t-kon-club.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Tower of Druaga",
        .boxartPath = "assets/images/systems/nes/the-tower-of-druaga.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Town & Country Surf Designs: Thrilla's Surfari",
        .boxartPath = "assets/images/systems/nes/town-country-surf-designs-thrilla-s-surfari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Town & Country Surf Designs: Wood & Water Rage",
        .boxartPath = "assets/images/systems/nes/town-country-surf-designs-wood-water-rage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Toxic Crusaders",
        .boxartPath = "assets/images/systems/nes/toxic-crusaders.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Track & Field",
        .boxartPath = "assets/images/systems/nes/track-field.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Track & Field II",
        .boxartPath = "assets/images/systems/nes/track-field-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Track & Field in Barcelona",
        .boxartPath = "assets/images/systems/nes/track-field-in-barcelona.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Transformers: Convoy no Nazo",
        .boxartPath = "assets/images/systems/nes/transformers-convoy-no-nazo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Treasure Master",
        .boxartPath = "assets/images/systems/nes/treasure-master.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Triathron",
        .boxartPath = "assets/images/systems/nes/the-triathron.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Trog!",
        .boxartPath = "assets/images/systems/nes/trog.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Trojan",
        .boxartPath = "assets/images/systems/nes/trojan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Trolls in Crazyland",
        .boxartPath = "assets/images/systems/nes/the-trolls-in-crazyland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Trolls on Treasure Island (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/trolls-on-treasure-island-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tsuppari Ōzumō",
        .boxartPath = "assets/images/systems/nes/tsuppari-zum.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tsuppari Wars",
        .boxartPath = "assets/images/systems/nes/tsuppari-wars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tsurikichi Sanpei: Blue Marlin Hen",
        .boxartPath = "assets/images/systems/nes/tsurikichi-sanpei-blue-marlin-hen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tsuru Pika Hagemaru: Mezase! Tsuru Seko no Akashi",
        .boxartPath = "assets/images/systems/nes/tsuru-pika-hagemaru-mezase-tsuru-seko-no-akashi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turbo Racing",
        .boxartPath = "assets/images/systems/nes/turbo-racing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Twin Cobra",
        .boxartPath = "assets/images/systems/nes/twin-cobra.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Twin Eagle",
        .boxartPath = "assets/images/systems/nes/twin-eagle.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "TwinBee",
        .boxartPath = "assets/images/systems/nes/twinbee.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "TwinBee 3: Poko Poko Daimaō",
        .boxartPath = "assets/images/systems/nes/twinbee-3-poko-poko-daima.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "U.S. Championship V'Ball",
        .boxartPath = "assets/images/systems/nes/u-s-championship-v-ball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Uchūkeibitai SDF",
        .boxartPath = "assets/images/systems/nes/uch-keibitai-sdf.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Uchūsen Cosmo Carrier",
        .boxartPath = "assets/images/systems/nes/uch-sen-cosmo-carrier.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ufouria: The Saga",
        .boxartPath = "assets/images/systems/nes/ufouria-the-saga.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultima: Exodus",
        .boxartPath = "assets/images/systems/nes/ultima-exodus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultima: Kyoufu no Exodus",
        .boxartPath = "assets/images/systems/nes/ultima-kyoufu-no-exodus.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultima: Quest of the Avatar",
        .boxartPath = "assets/images/systems/nes/ultima-quest-of-the-avatar.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultima: Seisha he no Michi",
        .boxartPath = "assets/images/systems/nes/ultima-seisha-he-no-michi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultima: Warriors of Destiny",
        .boxartPath = "assets/images/systems/nes/ultima-warriors-of-destiny.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultimate Air Combat",
        .boxartPath = "assets/images/systems/nes/ultimate-air-combat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultimate Basketball",
        .boxartPath = "assets/images/systems/nes/ultimate-basketball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultimate Journey",
        .boxartPath = "assets/images/systems/nes/ultimate-journey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultimate League Soccer (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/ultimate-league-soccer-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Ultimate Stuntman (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/the-ultimate-stuntman-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultraman Club 2: Kaette Kita Ultraman Club",
        .boxartPath = "assets/images/systems/nes/ultraman-club-2-kaette-kita-ultraman-club.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultraman Club 3: Mata Mata Shutsugeki‼ Ultra Kyōdai",
        .boxartPath = "assets/images/systems/nes/ultraman-club-3-mata-mata-shutsugeki-ultra-ky-dai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultraman Club: Kaijū Dai Kessen‼",
        .boxartPath = "assets/images/systems/nes/ultraman-club-kaij-dai-kessen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultraman Club: Supokon Fight!",
        .boxartPath = "assets/images/systems/nes/ultraman-club-supokon-fight.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Uncanny X-Men",
        .boxartPath = "assets/images/systems/nes/the-uncanny-x-men.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Uncharted Waters",
        .boxartPath = "assets/images/systems/nes/uncharted-waters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Uninvited",
        .boxartPath = "assets/images/systems/nes/uninvited.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Universal Wrestling Corporation",
        .boxartPath = "assets/images/systems/nes/universal-wrestling-corporation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Untouchables",
        .boxartPath = "assets/images/systems/nes/the-untouchables.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Urban Champion",
        .boxartPath = "assets/images/systems/nes/urban-champion.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Urusei Yatsura: Lum's Wedding Bell",
        .boxartPath = "assets/images/systems/nes/urusei-yatsura-lum-s-wedding-bell.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "USA Bowling",
        .boxartPath = "assets/images/systems/nes/usa-bowling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "USA Ice Hockey in FC",
        .boxartPath = "assets/images/systems/nes/usa-ice-hockey-in-fc.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ushio to Tora: Shin'en no Daiyō",
        .boxartPath = "assets/images/systems/nes/ushio-to-tora-shin-en-no-daiy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Utsurun Desu.: Kawauso Hawaii e Iku!!!",
        .boxartPath = "assets/images/systems/nes/utsurun-desu-kawauso-hawaii-e-iku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Valkyrie no Bōken: Toki no Kagi Densetsu",
        .boxartPath = "assets/images/systems/nes/valkyrie-no-b-ken-toki-no-kagi-densetsu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vegas Connection: Casino Kara Ai wo Komete",
        .boxartPath = "assets/images/systems/nes/vegas-connection-casino-kara-ai-wo-komete.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vegas Dream",
        .boxartPath = "assets/images/systems/nes/vegas-dream.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Venice Beach Volleyball (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/venice-beach-volleyball-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Venus Senki",
        .boxartPath = "assets/images/systems/nes/venus-senki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vice: Project Doom",
        .boxartPath = "assets/images/systems/nes/vice-project-doom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Videomation",
        .boxartPath = "assets/images/systems/nes/videomation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vindicators (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/vindicators-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Viva! Las Vegas",
        .boxartPath = "assets/images/systems/nes/viva-las-vegas.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Volguard II",
        .boxartPath = "assets/images/systems/nes/volguard-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Volleyball",
        .boxartPath = "assets/images/systems/nes/volleyball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wacky Races",
        .boxartPath = "assets/images/systems/nes/wacky-races.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wagyan Land",
        .boxartPath = "assets/images/systems/nes/wagyan-land.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wagyan Land 2",
        .boxartPath = "assets/images/systems/nes/wagyan-land-2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wagyan Land 3",
        .boxartPath = "assets/images/systems/nes/wagyan-land-3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wai Wai World",
        .boxartPath = "assets/images/systems/nes/wai-wai-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wai Wai World 2: SOS‼ Parsley Jō",
        .boxartPath = "assets/images/systems/nes/wai-wai-world-2-sos-parsley-j.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wall Street Kid",
        .boxartPath = "assets/images/systems/nes/wall-street-kid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wally Bear and the NO! Gang (Unlicensed)",
        .boxartPath = "assets/images/systems/nes/wally-bear-and-the-no-gang-unlicensed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wanpaku Dakku Yume Bōken",
        .boxartPath = "assets/images/systems/nes/wanpaku-dakku-yume-b-ken.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wanpaku Kokkun no Gourmet World",
        .boxartPath = "assets/images/systems/nes/wanpaku-kokkun-no-gourmet-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "War on Wheels",
        .boxartPath = "assets/images/systems/nes/war-on-wheels.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wario no Mori",
        .boxartPath = "assets/images/systems/nes/wario-no-mori.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wario's Woods",
        .boxartPath = "assets/images/systems/nes/wario-s-woods.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Warpman",
        .boxartPath = "assets/images/systems/nes/warpman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wayne Gretzky Hockey",
        .boxartPath = "assets/images/systems/nes/wayne-gretzky-hockey.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wayne's World",
        .boxartPath = "assets/images/systems/nes/wayne-s-world.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WCW World Championship Wrestling",
        .boxartPath = "assets/images/systems/nes/wcw-world-championship-wrestling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Werewolf: The Last Warrior",
        .boxartPath = "assets/images/systems/nes/werewolf-the-last-warrior.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Western Kids",
        .boxartPath = "assets/images/systems/nes/western-kids.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wheel of Fortune",
        .boxartPath = "assets/images/systems/nes/wheel-of-fortune.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wheel of Fortune: Family Edition",
        .boxartPath = "assets/images/systems/nes/wheel-of-fortune-family-edition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wheel of Fortune: Featuring Vanna White",
        .boxartPath = "assets/images/systems/nes/wheel-of-fortune-featuring-vanna-white.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wheel of Fortune: Junior Edition",
        .boxartPath = "assets/images/systems/nes/wheel-of-fortune-junior-edition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Where in Time is Carmen Sandiego?",
        .boxartPath = "assets/images/systems/nes/where-in-time-is-carmen-sandiego.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Where's Waldo?",
        .boxartPath = "assets/images/systems/nes/where-s-waldo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "White Lion Densetsu: Pyramid no Kanata ni",
        .boxartPath = "assets/images/systems/nes/white-lion-densetsu-pyramid-no-kanata-ni.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Who Framed Roger Rabbit",
        .boxartPath = "assets/images/systems/nes/who-framed-roger-rabbit.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Whomp 'Em",
        .boxartPath = "assets/images/systems/nes/whomp-em.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Widget",
        .boxartPath = "assets/images/systems/nes/widget.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wild Boys",
        .boxartPath = "assets/images/systems/nes/wild-boys.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wild Gunman",
        .boxartPath = "assets/images/systems/nes/wild-gunman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Willow",
        .boxartPath = "assets/images/systems/nes/willow.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wily & Right no RockBoard: That's Paradise",
        .boxartPath = "assets/images/systems/nes/wily-right-no-rockboard-that-s-paradise.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Win, Lose or Draw",
        .boxartPath = "assets/images/systems/nes/win-lose-or-draw.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Winter Games",
        .boxartPath = "assets/images/systems/nes/winter-games.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wit's",
        .boxartPath = "assets/images/systems/nes/wit-s.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizardry II: Legacy of Llylgamyn - The Third Scenario",
        .boxartPath = "assets/images/systems/nes/wizardry-ii-legacy-of-llylgamyn-the-third-scenario.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizardry III: Diamond no Kishi — The Second Scenario",
        .boxartPath = "assets/images/systems/nes/wizardry-iii-diamond-no-kishi-the-second-scenario.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizardry: Knight of Diamonds - The Second Scenario",
        .boxartPath = "assets/images/systems/nes/wizardry-knight-of-diamonds-the-second-scenario.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizardry: Proving Grounds of the Mad Overlord",
        .boxartPath = "assets/images/systems/nes/wizardry-proving-grounds-of-the-mad-overlord.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizards & Warriors",
        .boxartPath = "assets/images/systems/nes/wizards-warriors.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wizards & Warriors III: Kuros: Visions of Power",
        .boxartPath = "assets/images/systems/nes/wizards-warriors-iii-kuros-visions-of-power.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wolverine",
        .boxartPath = "assets/images/systems/nes/wolverine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Woody Poco",
        .boxartPath = "assets/images/systems/nes/woody-poco.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Boxing",
        .boxartPath = "assets/images/systems/nes/world-boxing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Champ",
        .boxartPath = "assets/images/systems/nes/world-champ.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Games",
        .boxartPath = "assets/images/systems/nes/world-games.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Grand-Prix: Pole to Finish",
        .boxartPath = "assets/images/systems/nes/world-grand-prix-pole-to-finish.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Super Tennis",
        .boxartPath = "assets/images/systems/nes/world-super-tennis.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wrath of the Black Manta",
        .boxartPath = "assets/images/systems/nes/wrath-of-the-black-manta.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wrecking Crew",
        .boxartPath = "assets/images/systems/nes/wrecking-crew.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wurm: Journey to the Center of the Earth",
        .boxartPath = "assets/images/systems/nes/wurm-journey-to-the-center-of-the-earth.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WWF King of the Ring",
        .boxartPath = "assets/images/systems/nes/wwf-king-of-the-ring.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WWF WrestleMania",
        .boxartPath = "assets/images/systems/nes/wwf-wrestlemania.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WWF WrestleMania Challenge",
        .boxartPath = "assets/images/systems/nes/wwf-wrestlemania-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WWF WrestleMania: Steel Cage Challenge",
        .boxartPath = "assets/images/systems/nes/wwf-wrestlemania-steel-cage-challenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Xenophobe",
        .boxartPath = "assets/images/systems/nes/xenophobe.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Xevious",
        .boxartPath = "assets/images/systems/nes/xevious.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Xexyz",
        .boxartPath = "assets/images/systems/nes/xexyz.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Xybots",
        .boxartPath = "assets/images/systems/nes/xybots.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yami no Shigotonin Kage",
        .boxartPath = "assets/images/systems/nes/yami-no-shigotonin-kage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yie Ar Kung-Fu",
        .boxartPath = "assets/images/systems/nes/yie-ar-kung-fu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yo! Noid",
        .boxartPath = "assets/images/systems/nes/yo-noid.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi",
        .boxartPath = "assets/images/systems/nes/yoshi.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi no Cookie",
        .boxartPath = "assets/images/systems/nes/yoshi-no-cookie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi no Tamago",
        .boxartPath = "assets/images/systems/nes/yoshi-no-tamago.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi's Cookie",
        .boxartPath = "assets/images/systems/nes/yoshi-s-cookie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yōichi Masuzoe: Asa Made Famicom",
        .boxartPath = "assets/images/systems/nes/y-ichi-masuzoe-asa-made-famicom.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yōkai Club",
        .boxartPath = "assets/images/systems/nes/y-kai-club.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yōkai Dōchūki",
        .boxartPath = "assets/images/systems/nes/y-kai-d-ch-ki.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Young Indiana Jones Chronicles",
        .boxartPath = "assets/images/systems/nes/the-young-indiana-jones-chronicles.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yousei Monogatari: RodLand",
        .boxartPath = "assets/images/systems/nes/yousei-monogatari-rodland.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yōsuke Ide Meijin no Jissen Mahjong",
        .boxartPath = "assets/images/systems/nes/y-suke-ide-meijin-no-jissen-mahjong.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yōsuke Ide Meijin no Jissen Mahjong II",
        .boxartPath = "assets/images/systems/nes/y-suke-ide-meijin-no-jissen-mahjong-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ys",
        .boxartPath = "assets/images/systems/nes/ys.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ys II",
        .boxartPath = "assets/images/systems/nes/ys-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ys III: Wanderers from Ys",
        .boxartPath = "assets/images/systems/nes/ys-iii-wanderers-from-ys.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yu Yu Hakusho: Bakutō Ankoku Bujutsu Kai",
        .boxartPath = "assets/images/systems/nes/yu-yu-hakusho-bakut-ankoku-bujutsu-kai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yume Penguin Monogatari",
        .boxartPath = "assets/images/systems/nes/yume-penguin-monogatari.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zanac",
        .boxartPath = "assets/images/systems/nes/zanac.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zelda II: The Adventure of Link",
        .boxartPath = "assets/images/systems/nes/zelda-ii-the-adventure-of-link.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zelda no Densetsu 1",
        .boxartPath = "assets/images/systems/nes/zelda-no-densetsu-1.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zen: Intergalactic Ninja",
        .boxartPath = "assets/images/systems/nes/zen-intergalactic-ninja.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zenbei‼ Pro Basketball",
        .boxartPath = "assets/images/systems/nes/zenbei-pro-basketball.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zippy Race",
        .boxartPath = "assets/images/systems/nes/zippy-race.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zoda's Revenge: StarTropics II",
        .boxartPath = "assets/images/systems/nes/zoda-s-revenge-startropics-ii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zoids 2: Zenebasu no Gyakushū",
        .boxartPath = "assets/images/systems/nes/zoids-2-zenebasu-no-gyakush.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zoids: Chūō Tairiku no Tatakai",
        .boxartPath = "assets/images/systems/nes/zoids-ch-tairiku-no-tatakai.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zoids: Mokushiroku",
        .boxartPath = "assets/images/systems/nes/zoids-mokushiroku.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zombie Hunter",
        .boxartPath = "assets/images/systems/nes/zombie-hunter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zombie Nation",
        .boxartPath = "assets/images/systems/nes/zombie-nation.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Zunō Senkan Galg",
        .boxartPath = "assets/images/systems/nes/zun-senkan-galg.png",
        .orient = SLOT_ORIENT_INHERIT,
    },

};

const int nes_game_count = (int)(sizeof(nes_games) / sizeof(nes_games[0]));

#endif /* NES_IMPLEMENTATION_ONCE */
#endif /* NES_IMPLEMENTATION */
