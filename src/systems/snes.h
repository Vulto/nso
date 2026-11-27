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

/* Mednafen (mednafen/src/mednafen, built from the source tree in the NSO root)
 * runs the ROM in roms/SNES. Config and saves: saves/snes/mednafen; NSO's
 * suspend points: saves/snes/mednafen/states/<ROM file>. */
#define MEDNAFEN_SNES(file) \
    .dir = "mednafen/src", .binary = "./mednafen", .mednafen = "snes", \
    .rom = "roms/SNES/" file, \
    .stateDir = "saves/snes/mednafen/states/" file, .externalSlots = true

GameEntry snes_games[] = {
    {   /* snesrev smw (repos/snes/smw) inside NSO's window: `make embed` */
        .title = "Super Mario World",
        .boxartPath = "assets/images/systems/snes/smw.png",
        .dir = "embed/smw",
        .binary = "build/libsmwmod.so",
        .stateDir = "saves/snes/smw",
        .orient = SLOT_ORIENT_INHERIT,
        .embedRomKey = "snes.smw_dir",   /* the port's dir: smw_assets.dat, saves/ */
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
        .orient = SLOT_ORIENT_JAPAN,
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
        .dir = "embed/starfox",
        .binary = "build/libstarfoxmod.so",
        .stateDir = "saves/snes/starfox",
        .orient = SLOT_ORIENT_INHERIT,
        .embedRomKey = "snes.starfox_rom",
    },
    {
        .title = "Aladdin",
        .boxartPath = "assets/images/systems/snes/aladdin.png",
        MEDNAFEN_SNES("Aladdin.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Axelay",
        .boxartPath = "assets/images/systems/snes/axelay.png",
        MEDNAFEN_SNES("Axelay (U).smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Biker Mice From Mars",
        .boxartPath = "assets/images/systems/snes/biker_mice_from_mars.png",
        MEDNAFEN_SNES("Biker Mice From Mars (E) [!].smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Contra III - The Alien Wars",
        .boxartPath = "assets/images/systems/snes/contra_iii_-_the_alien_wars.png",
        MEDNAFEN_SNES("Contra III - The Alien Wars.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Darius Twin",
        .boxartPath = "assets/images/systems/snes/darius_twin.png",
        MEDNAFEN_SNES("Darius Twin.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Demon's Crest",
        .boxartPath = "assets/images/systems/snes/demon's_crest.png",
        MEDNAFEN_SNES("Demon's Crest (E).fig"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Country",
        .boxartPath = "assets/images/systems/snes/donkey_kong_country_1.png",
        MEDNAFEN_SNES("Donkey Kong Country 1.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Country 2 - Diddy's Kong Quest",
        .boxartPath = "assets/images/systems/snes/donkey_kong_country_2_-_diddy's_kong_quest.png",
        MEDNAFEN_SNES("Donkey Kong Country 2 - Diddy's Kong Quest (USA) (En,Fr).sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong Country 3 - Dixie Kong's Double Trouble!",
        .boxartPath = "assets/images/systems/snes/donkey_kong_country_3.png",
        MEDNAFEN_SNES("Donkey Kong Country 3.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fight 2",
        .boxartPath = "assets/images/systems/snes/final_fight_2.png",
        MEDNAFEN_SNES("Final Fight 2 (U).smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fight 3",
        .boxartPath = "assets/images/systems/snes/final_fight_3.png",
        MEDNAFEN_SNES("Final Fight 3 (USA).sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fight 3 (patched)",
        .boxartPath = "assets/images/systems/snes/final_fight_3_patched.png",
        MEDNAFEN_SNES("Final Fight 3 (USA) (patched).sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fight Guy",
        .boxartPath = "assets/images/systems/snes/final_fight_guy.png",
        MEDNAFEN_SNES("Final Fight Guy (U).smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Final Fight",
        .boxartPath = "assets/images/systems/snes/final_fight.png",
        MEDNAFEN_SNES("Final Fight.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gegege no Kitarou - Fukkatsu! Tenma Daiou",
        .boxartPath = "assets/images/systems/snes/gegege_no_kitarou_-_fukkatsu!_tenma_daiou.png",
        MEDNAFEN_SNES("Gegege no Kitarou - Fukkatsu! Tenma Daiou.sfc"),
        .orient = SLOT_ORIENT_JAPAN,
    },
    {
        .title = "Goof Troop",
        .boxartPath = "assets/images/systems/snes/goof_troop.png",
        MEDNAFEN_SNES("Goof Troop.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gradius III",
        .boxartPath = "assets/images/systems/snes/gradius_iii.png",
        MEDNAFEN_SNES("Gradius III.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "International Superstar Soccer Deluxe",
        .boxartPath = "assets/images/systems/snes/international_superstar_soccer_deluxe.png",
        MEDNAFEN_SNES("International Superstar Soccer Deluxe.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Magical Quest Starring Mickey Mouse",
        .boxartPath = "assets/images/systems/snes/magical_quest_starring_mickey_mouse.png",
        MEDNAFEN_SNES("Magical Quest Starring Mickey Mouse.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Marvel Super Heroes - War of the Gems",
        .boxartPath = "assets/images/systems/snes/marvel_super_heroes_-_war_of_the_gems.png",
        MEDNAFEN_SNES("Marvel Super Heroes - War of the Gems.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 7",
        .boxartPath = "assets/images/systems/snes/megaman_vii.png",
        MEDNAFEN_SNES("Megaman VII (USA).sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man X",
        .boxartPath = "assets/images/systems/snes/mega_man_x1.png",
        MEDNAFEN_SNES("Mega Man X1.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey to Donald - Magical Adventure 3",
        .boxartPath = "assets/images/systems/snes/mickey_&_donald_-_magical_adventure_3.png",
        MEDNAFEN_SNES("Mickey & Donald - Magical Adventure 3.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mighty Morphin Power Rangers - The Movie",
        .boxartPath = "assets/images/systems/snes/mighty_morphin_power_rangers_-_the_movie.png",
        MEDNAFEN_SNES("Mighty Morphin Power Rangers - The Movie (U).smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mortal Kombat II",
        .boxartPath = "assets/images/systems/snes/mortal_kombat_ii.png",
        MEDNAFEN_SNES("Mortal Kombat II.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ninja Gaiden Trilogy",
        .boxartPath = "assets/images/systems/snes/ninja_gaiden_trilogy_snes.png",
        MEDNAFEN_SNES("Ninja Gaiden Trilogy SNES.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pilotwings",
        .boxartPath = "assets/images/systems/snes/pilotwings.png",
        MEDNAFEN_SNES("Pilotwings.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rockman & Forte",
        .boxartPath = "assets/images/systems/snes/rockman_&_forte.png",
        MEDNAFEN_SNES("Rockman & Forte (J).swc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rock N' Roll Racing",
        .boxartPath = "assets/images/systems/snes/rock_n'_roll_racing.png",
        MEDNAFEN_SNES("Rock N' Roll Racing.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "R-Type III - The Third Lightning",
        .boxartPath = "assets/images/systems/snes/r-type_iii_-_the_third_lightning.png",
        MEDNAFEN_SNES("R-Type III - The Third Lightning (U).smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Secret of Mana",
        .boxartPath = "assets/images/systems/snes/secret_of_mana.png",
        MEDNAFEN_SNES("Secret of Mana.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Side Pocket",
        .boxartPath = "assets/images/systems/snes/side_pocket.png",
        MEDNAFEN_SNES("Side Pocket.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sonic Wings",
        .boxartPath = "assets/images/systems/snes/sonic_wings.png",
        MEDNAFEN_SNES("Sonic Wings (Japan).sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Fox 2",
        .boxartPath = "assets/images/systems/snes/star_fox_2.png",
        MEDNAFEN_SNES("Star Fox 2.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Street Fighter II",
        .boxartPath = "assets/images/systems/snes/street_fighter_ii.png",
        MEDNAFEN_SNES("Street Fighter II.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Bomberman 5",
        .boxartPath = "assets/images/systems/snes/super_bomberman_5.png",
        MEDNAFEN_SNES("Super Bomberman 5.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Castlevania IV",
        .boxartPath = "assets/images/systems/snes/super_castlevania_iv.png",
        MEDNAFEN_SNES("Super Castlevania IV.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Ghouls 'N Ghosts",
        .boxartPath = "assets/images/systems/snes/super_ghouls_'n_ghosts.png",
        MEDNAFEN_SNES("Super Ghouls 'N Ghosts.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario All-Stars + Super Mario World",
        .boxartPath = "assets/images/systems/snes/super_mario_all-stars_+_super_mario_world.png",
        MEDNAFEN_SNES("Super Mario All-Stars + Super Mario World.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario Kart",
        .boxartPath = "assets/images/systems/snes/super_mario_kart.png",
        MEDNAFEN_SNES("Super Mario Kart.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Mario World 2 - Yoshi's Island",
        .boxartPath = "assets/images/systems/snes/super_mario_world_2_-_yoshi's_island.png",
        MEDNAFEN_SNES("Super Mario World 2 - Yoshi's Island.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super R-Type",
        .boxartPath = "assets/images/systems/snes/super_r-type.png",
        MEDNAFEN_SNES("Super R-Type.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SWAT Kats - The Radical Squadron",
        .boxartPath = "assets/images/systems/snes/swat_kats_-_the_radical_squadron.png",
        MEDNAFEN_SNES("SWAT Kats - The Radical Squadron.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles IV - Turtles in Time",
        .boxartPath = "assets/images/systems/snes/teenage_mutant_ninja_turtles_iv_-_turtles_in_time.png",
        MEDNAFEN_SNES("Teenage Mutant Ninja Turtles IV - Turtles in Time.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Teenage Mutant Ninja Turtles - Tournament Fighters",
        .boxartPath = "assets/images/systems/snes/teenage_mutant_ninja_turtles_-_tournament_fighters.png",
        MEDNAFEN_SNES("Teenage Mutant Ninja Turtles - Tournament Fighters.smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear 2",
        .boxartPath = "assets/images/systems/snes/top_gear_2.png",
        MEDNAFEN_SNES("Top Gear 2.smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear 3000",
        .boxartPath = "assets/images/systems/snes/top_gear_3000.png",
        MEDNAFEN_SNES("Top Gear 3000.smc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ultimate Mortal Kombat 3",
        .boxartPath = "assets/images/systems/snes/ultimate_mortal_kombat_3.png",
        MEDNAFEN_SNES("Ultimate Mortal Kombat 3.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wild Guns",
        .boxartPath = "assets/images/systems/snes/wild_guns.png",
        MEDNAFEN_SNES("Wild Guns.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "X-Men - Mutant Apocalypse",
        .boxartPath = "assets/images/systems/snes/x-men_mutant_apocalipse.png",
        MEDNAFEN_SNES("X-Men Mutant Apocalipse.sfc"),
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int snes_game_count = (int)(sizeof(snes_games) / sizeof(snes_games[0]));

#endif /* SNES_IMPLEMENTATION_ONCE */
#endif /* SNES_IMPLEMENTATION */
