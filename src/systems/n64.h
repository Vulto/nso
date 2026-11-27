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
    {   /* sm64ex (repos/n64/sm64ex) inside NSO's window: `make embed` */
        .title = "Super Mario 64",
        .boxartPath = "assets/images/systems/n64/sm64.png",
        .dir = "embed/sm64",
        .binary = "build/libsm64mod.so",
        .stateDir = "saves/n64/sm64",
        .orient = SLOT_ORIENT_INHERIT,
        .embedRomKey = "n64.sm64_dir",   /* EEPROM save + sm64config.txt; ROM for the build */
    },
    {
        .title = "Zelda Ocarina of Time",
        .boxartPath = "assets/images/systems/n64/oot.png",
        .dir = "repos/n64/soh-src",
        .binary = "./build-cmake/soh/soh.elf",
        .stateDir = "saves/n64/oot",
        .externalMenu = true,
        .externalSlots = true,
        .crtHook = true,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Majora's Mask",
        .boxartPath = "assets/images/systems/n64/mm.png",
        .dir = "repos/n64/2s2h-src/build-cmake/mm",
        .binary = "lib2shipmod.so",
        .stateDir = "saves/n64/mm",
        .orient = SLOT_ORIENT_INHERIT,
        .embedRomKey = "n64.mm_data",
    },
    {
        .title = "Perfect Dark",
        .boxartPath = "assets/images/systems/n64/pd.png",
        .dir = "repos/n64/perfect_dark-src",
        .binary = "build/pd.x86_64",
        .stateDir = "saves/n64/perfect-dark",
        .externalMenu = true,
        .crtHook = true,
        .env = "N64PAD_SDL",   /* the 8BitDo N64 in S mode: not a Pro Controller with a stuck right stick */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Smash Bros",
        .boxartPath = "assets/images/systems/n64/ssb64.png",
        .dir = "repos/n64/BattleShip/build-us",
        .binary = "./BattleShip",
        .stateDir = "saves/n64/BattleShip",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Fox 64",
        .boxartPath = "assets/images/systems/n64/starfox.png",
        .dir = "embed/starship",   /* Starship inside NSO's window: `make embed` */
        .binary = "build/libstarshipmod.so",
        .stateDir = "saves/n64/starship",
        .orient = SLOT_ORIENT_INHERIT,
        .embedRomKey = "n64.starship_dir",   /* config, saves, sf64.o2r and starship.o2r */
    },
    {
        .title = "1080 Snowboarding (Japan, USA) (En,Ja)",
        .boxartPath = "assets/images/systems/n64/1080.png",
        .dir = "repos/n64/1080-recomp/build/pc",
        .binary = "./1080SnowboardingRecompiled",
        .appDir = "saves/n64/1080-recomp",   /* config, saves and the stored ROM */
        .env = "GAME_AUTOSTART=1",   /* starts the stored ROM, no launcher */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Banjo Kazooie",
        .boxartPath = "assets/images/systems/n64/bk.png",
        .dir = "repos/n64/Lighthouse/build-cmake",
        .binary = "./Lighthouse",
        .appDir = "saves/n64/bk",   /* config, saves and bk.o2r */
        .env = "SHIP_HOME=@saves/n64/bk",   /* libultraship app dir */
        .orient = SLOT_ORIENT_INHERIT,
        .crtHook = true,
    },
    {
        .title = "Banjo-Tooie",
        .boxartPath = "assets/images/systems/n64/Banjo-Tooie.png",
        .dir = "repos/n64/bt-recomp",
        .binary = "./build-cmake/BanjoTooieRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dr. Mario 64",
        .boxartPath = "assets/images/systems/n64/Dr._Mario_64_NSO.png",
        .dir = "repos/n64/drmario64-recomp",
        .binary = "build-cmake/drmario64_recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "GoldenEye 007",
        .boxartPath = "assets/images/systems/n64/GoldenEye_007_NSO.png",
        .dir = "repos/n64/ProjectMidas-v1.0.2b-linux",
        .binary = "./ProjectMidas",   /* release build, 32-bit; boots straight into the game */
        .env = "GE_ROM=@saves/n64/goldeneye/GoldenEye_007_USA.z64",   /* the stock (U) cart, 12 MB z64 */
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
        .dir = "repos/n64/spaghettikart/build-cmake",
        .binary = "./Spaghettify",
        .appDir = "saves/n64/mk64",   /* config, saves and mk64.o2r */
        .env = "SHIP_HOME=@saves/n64/mk64",   /* libultraship app dir */
        .orient = SLOT_ORIENT_INHERIT,
        .crtHook = true,
    },
    {
        .title = "Mario Party 3",
        .boxartPath = "assets/images/systems/n64/Mario_Party_3_NSO.png",
        .dir = "repos/n64/mp3-recomp",
        .binary = "build/MarioParty3Recompiled",
        .appDir = "saves/n64/mp3-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",   /* starts the stored ROM, no launcher */
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Mortal Kombat Mythologies: Sub-Zero",
        .boxartPath = "assets/images/systems/n64/MortalKombatMythologiesSubZero.png",
        .dir = NULL,
        .binary = NULL,
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paper Mario",
        .boxartPath = "assets/images/systems/n64/Paper_Mario_NSO.png",
        .dir = "repos/n64/paperboat/build/ninja-debug",
        .binary = "./Paperboat",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pilotwings 64",
        .boxartPath = "assets/images/systems/n64/Pilotwings_N64_NSO.png",
        .dir = "repos/n64/pw64-recomp",
        .binary = "build-linux/Pilotwings64Recomp",
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
        .title = "Tony Hawk's Pro Skater 2",
        .boxartPath = "assets/images/systems/n64/tonyhawksproskater2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tony Hawk's Pro Skater 3",
        .boxartPath = "assets/images/systems/n64/tonyhawksproskater3.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wave Race 64 - Kawasaki Jet Ski",
        .boxartPath = "assets/images/systems/n64/Wave_Race_64_NSO.png",
        .dir = "repos/n64/waverace64-recomp",
        .binary = "build-linux/WaveRace64Recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Extreme-G",
        .boxartPath = "assets/images/systems/n64/extremeg.png",
        .dir = "repos/n64/extremeg-recomp",
        .binary = "build-cmake/ExtremeGRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Station Silicon Valley",
        .boxartPath = "assets/images/systems/n64/sssv.png",
        .dir = "repos/n64/sssv-recomp",
        .binary = "build-cmake/SSSVRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Yoshi's Story",
        .boxartPath = "assets/images/systems/n64/yoshisstory.png",
        .dir = "repos/n64/yoshisstory-recomp",
        .binary = "build/DinoTale",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Conker's Bad Fur Day",
        .boxartPath = "assets/images/systems/n64/conker.png",
        .dir = "repos/n64/conker-recomp",
        .binary = "host/build/ConkerRecomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blast Corps",
        .boxartPath = "assets/images/systems/n64/blastcorps.png",
        .dir = "repos/n64/blastcorps-recomp",          /* assets/ are read from the cwd */
        .binary = "build-cmake/BlastCorpsRecompiled",
        .startKeys = "Return",   /* the launcher has no autostart: Start Game */
        .appDir = "saves/n64/blastcorps-recomp",       /* bc.n64.us.z64 (Select games) */
        .stateDir = "saves/n64/blastcorps",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "007: The World Is Not Enough",
        .boxartPath = "assets/images/systems/n64/007theworldisnotenough.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "AeroGauge",
        .boxartPath = "assets/images/systems/n64/aerogauge.png",
        .dir = "repos/n64/aerogauge-recomp/build",
        .binary = "libaerogaugemod.so",
        .embedRomKey = "n64.aerogauge_rom",
        .stateDir = "saves/n64/aerogauge",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Beetle Adventure Racing!",
        .boxartPath = "assets/images/systems/n64/beetleadventureracing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Big Mountain 2000",
        .boxartPath = "assets/images/systems/n64/bigmountain2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bio F.R.E.A.K.S.",
        .boxartPath = "assets/images/systems/n64/biofreaks.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Blues Brothers 2000",
        .boxartPath = "assets/images/systems/n64/bluesbrothers2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman 64: The Second Attack!",
        .boxartPath = "assets/images/systems/n64/bomberman64thesecondattack.png",
        .dir = "repos/n64/bm64sa-recomp/build-cmake",
        .binary = "./BM64U2Recompiled",
        .appDir = "saves/n64/bm64sa-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",   /* starts the stored ROM, no launcher click */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Buck Bumble",
        .boxartPath = "assets/images/systems/n64/buckbumble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "A Bug's Life",
        .boxartPath = "assets/images/systems/n64/abugslife.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bust-A-Move '99",
        .boxartPath = "assets/images/systems/n64/bustamove99.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bust-A-Move 2: Arcade Edition",
        .boxartPath = "assets/images/systems/n64/bustamove2arcadeedition.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "California Speed",
        .boxartPath = "assets/images/systems/n64/californiaspeed.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Carmageddon 64",
        .boxartPath = "assets/images/systems/n64/carmageddon64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Castlevania: Legacy of Darkness",
        .boxartPath = "assets/images/systems/n64/castlevanialegacyofdarkness.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chameleon Twist 2",
        .boxartPath = "assets/images/systems/n64/chameleontwist2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Charlie Blast's Territory",
        .boxartPath = "assets/images/systems/n64/charlieblaststerritory.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chopper Attack",
        .boxartPath = "assets/images/systems/n64/chopperattack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Clay Fighter: Sculptor's Cut",
        .boxartPath = "assets/images/systems/n64/clayfightersculptorscut.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Clay Fighter 63 1-3",
        .boxartPath = "assets/images/systems/n64/clayfighter6313.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Command & Conquer",
        .boxartPath = "assets/images/systems/n64/commandconquer.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cruis'n Exotica",
        .boxartPath = "assets/images/systems/n64/cruisnexotica.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cruis'n USA",
        .boxartPath = "assets/images/systems/n64/cruisnusa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Cruis'n World",
        .boxartPath = "assets/images/systems/n64/cruisnworld.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Dark Rift",
        .boxartPath = "assets/images/systems/n64/darkrift.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Deadly Arts",
        .boxartPath = "assets/images/systems/n64/deadlyarts.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Destruction Derby 64",
        .boxartPath = "assets/images/systems/n64/destructionderby64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Diddy Kong Racing",
        .boxartPath = "assets/images/systems/n64/diddykongracing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donald Duck: Goin' Quackers",
        .boxartPath = "assets/images/systems/n64/donaldduckgoinquackers.png",
        .dir = "repos/n64/donaldduck-recomp",
        .binary = "build/GoinQuackersRecompiled",
        .appDir = "saves/n64/donaldduck-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Doom 64",
        .boxartPath = "assets/images/systems/n64/doom64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Dual Heroes",
        .boxartPath = "assets/images/systems/n64/dualheroes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duck Dodgers Starring Daffy Duck",
        .boxartPath = "assets/images/systems/n64/duckdodgersstarringdaffyduck.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duke Nukem 64",
        .boxartPath = "assets/images/systems/n64/dukenukem64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Earthworm Jim 3D",
        .boxartPath = "assets/images/systems/n64/earthwormjim3d.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Excitebike 64",
        .boxartPath = "assets/images/systems/n64/excitebike64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Extreme-G XG2",
        .boxartPath = "assets/images/systems/n64/extremegxg2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Glover",
        .boxartPath = "assets/images/systems/n64/glover.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Goemon's Great Adventure",
        .boxartPath = "assets/images/systems/n64/goemonsgreatadventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Golden Nugget 64",
        .boxartPath = "assets/images/systems/n64/goldennugget64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "GT 64: Championship Edition",
        .boxartPath = "assets/images/systems/n64/gt64championshipedition.png",
        .dir = "repos/n64/gt64-recomp",
        .binary = "build/app/GT64Recompiled",
        .appDir = "saves/n64/gt64-recomp",   /* config, saves and the stored ROM */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hercules: The Legendary Journeys",
        .boxartPath = "assets/images/systems/n64/herculesthelegendaryjourneys.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hexen",
        .boxartPath = "assets/images/systems/n64/hexen.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hey You, Pikachu!",
        .boxartPath = "assets/images/systems/n64/heyyoupikachu.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hot Wheels: Turbo Racing",
        .boxartPath = "assets/images/systems/n64/hotwheelsturboracing.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Hydro Thunder",
        .boxartPath = "assets/images/systems/n64/hydrothunder.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Indiana Jones and the Infernal Machine",
        .boxartPath = "assets/images/systems/n64/indianajonesandtheinfernalmachine.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Indy Racing 2000",
        .boxartPath = "assets/images/systems/n64/indyracing2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "International Superstar Soccer '98",
        .boxartPath = "assets/images/systems/n64/internationalsuperstarsoccer98.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "International Superstar Soccer 2000",
        .boxartPath = "assets/images/systems/n64/internationalsuperstarsoccer2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "International Superstar Soccer 64",
        .boxartPath = "assets/images/systems/n64/internationalsuperstarsoccer64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Jeopardy!",
        .boxartPath = "assets/images/systems/n64/jeopardy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jeremy McGrath Supercross 2000",
        .boxartPath = "assets/images/systems/n64/jeremymcgrathsupercross2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "John Romero's Daikatana",
        .boxartPath = "assets/images/systems/n64/johnromerosdaikatana.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ken Griffey Jr.'s Slugfest",
        .boxartPath = "assets/images/systems/n64/kengriffeyjrsslugfest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "King Hill 64: Extreme Snowboarding",
        .boxartPath = "assets/images/systems/n64/kinghill64extremesnowboarding.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Kirby 64: The Crystal Shards",
        .boxartPath = "assets/images/systems/n64/kirby64thecrystalshards.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Knife Edge: Nose Gunner",
        .boxartPath = "assets/images/systems/n64/knifeedgenosegunner.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Mario Party",
        .boxartPath = "assets/images/systems/n64/marioparty.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mario Party 2",
        .boxartPath = "assets/images/systems/n64/marioparty2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mickey's Speedway USA",
        .boxartPath = "assets/images/systems/n64/mickeysspeedwayusa.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mortal Kombat 4",
        .boxartPath = "assets/images/systems/n64/mortalkombat4.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mortal Kombat Trilogy",
        .boxartPath = "assets/images/systems/n64/mortalkombattrilogy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "MRC: Multi-Racing Championship",
        .boxartPath = "assets/images/systems/n64/mrcmultiracingchampionship.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ms. Pac-Man: Maze Madness",
        .boxartPath = "assets/images/systems/n64/mspacmanmazemadness.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Namco Museum 64",
        .boxartPath = "assets/images/systems/n64/namcomuseum64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NASCAR 2000",
        .boxartPath = "assets/images/systems/n64/nascar2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "NASCAR 99",
        .boxartPath = "assets/images/systems/n64/nascar99.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nightmare Creatures",
        .boxartPath = "assets/images/systems/n64/nightmarecreatures.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Nuclear Strike 64",
        .boxartPath = "assets/images/systems/n64/nuclearstrike64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Off Road Challenge",
        .boxartPath = "assets/images/systems/n64/offroadchallenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Ogre Battle 64: Person of Lordly Caliber",
        .boxartPath = "assets/images/systems/n64/ogrebattle64personoflordlycaliber.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Paperboy",
        .boxartPath = "assets/images/systems/n64/paperboy.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Penny Racers",
        .boxartPath = "assets/images/systems/n64/pennyracers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Pokemon Puzzle League",
        .boxartPath = "assets/images/systems/n64/pokemonpuzzleleague.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pokemon Snap",
        .boxartPath = "assets/images/systems/n64/pokemonsnap.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Pokemon Stadium 2",
        .boxartPath = "assets/images/systems/n64/pokemonstadium2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Polaris SnoCross",
        .boxartPath = "assets/images/systems/n64/polarissnocross.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Power Rangers: Lightspeed Rescue",
        .boxartPath = "assets/images/systems/n64/powerrangerslightspeedrescue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Powerpuff Girls: Chemical X-Traction",
        .boxartPath = "assets/images/systems/n64/thepowerpuffgirlschemicalxtraction.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quake",
        .boxartPath = "assets/images/systems/n64/quake.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quake II",
        .boxartPath = "assets/images/systems/n64/quakeii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Resident Evil 2",
        .boxartPath = "assets/images/systems/n64/residentevil2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Road Rash 64",
        .boxartPath = "assets/images/systems/n64/roadrash64.png",
        .dir = "repos/n64/roadrash-recomp/native/build/bin",
        .binary = "./RoadRash64Recompiled",
        .appDir = "saves/n64/roadrash-recomp",   /* config, saves and the stored ROM */
        .args = "--skip-launcher",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Roadsters",
        .boxartPath = "assets/images/systems/n64/roadsters.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Robotron 64",
        .boxartPath = "assets/images/systems/n64/robotron64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rocket: Robot on Wheels",
        .boxartPath = "assets/images/systems/n64/rocketrobotonwheels.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "RR64: Ridge Racer 64",
        .boxartPath = "assets/images/systems/n64/rr64ridgeracer64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rugrats: Scavenger Hunt",
        .boxartPath = "assets/images/systems/n64/rugratsscavengerhunt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rush 2: Extreme Racing USA",
        .boxartPath = "assets/images/systems/n64/rush2extremeracingusa.png",
        .dir = "repos/n64/rush2-recomp/build",
        .binary = "./Rush2Recompiled",
        .appDir = "saves/n64/rush2-recomp",   /* config, saves and the stored ROM */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "San Francisco Rush 2049",
        .boxartPath = "assets/images/systems/n64/sanfranciscorush2049.png",
        .dir = "repos/n64/rush2049-recomp",
        .binary = "build/Rush2049Recompiled",
        .appDir = "saves/n64/rush2049-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadow Man",
        .boxartPath = "assets/images/systems/n64/shadowman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Spider-Man",
        .boxartPath = "assets/images/systems/n64/spiderman.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear Rally",
        .boxartPath = "assets/images/systems/n64/topgearrally.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vigilante 8: 2nd Offense",
        .boxartPath = "assets/images/systems/n64/vigilante82ndoffense.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "WinBack: Covert Operations",
        .boxartPath = "assets/images/systems/n64/winbackcovertoperations.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wipeout 64",
        .boxartPath = "assets/images/systems/n64/wipeout64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tetrisphere",
        .boxartPath = "assets/images/systems/n64/tetrisphere.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tigger's Honey Hunt",
        .boxartPath = "assets/images/systems/n64/tiggershoneyhunt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tom and Jerry in Fists of Furry",
        .boxartPath = "assets/images/systems/n64/tomandjerryinfistsoffurry.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tom Clancy's Rainbow Six",
        .boxartPath = "assets/images/systems/n64/tomclancysrainbowsix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tonic Trouble",
        .boxartPath = "assets/images/systems/n64/tonictrouble.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Transformers: Beast Wars Transmetals",
        .boxartPath = "assets/images/systems/n64/transformersbeastwarstransmetals.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Triple Play 2000",
        .boxartPath = "assets/images/systems/n64/tripleplay2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Virtual Pool 64",
        .boxartPath = "assets/images/systems/n64/virtualpool64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "War Gods",
        .boxartPath = "assets/images/systems/n64/wargods.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Wetrix",
        .boxartPath = "assets/images/systems/n64/wetrix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Wheel of Fortune",
        .boxartPath = "assets/images/systems/n64/wheeloffortune.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Cup 98",
        .boxartPath = "assets/images/systems/n64/worldcup98.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "World Driver Championship",
        .boxartPath = "assets/images/systems/n64/worlddriverchampionship.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Worms Armageddon",
        .boxartPath = "assets/images/systems/n64/wormsarmageddon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Xena: Warrior Princess - The Talisman of Fate",
        .boxartPath = "assets/images/systems/n64/xenawarriorprincessthetalismanoffate.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Aidyn Chronicles: The First Mage",
        .boxartPath = "assets/images/systems/n64/aidynchroniclesthefirstmage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Harvest Moon 64",
        .boxartPath = "assets/images/systems/n64/hm64.png",
        .dir = "repos/n64/hm64-recomp",
        .binary = "build-cmake/HarvestMoon64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Chameleon Twist",
        .boxartPath = "assets/images/systems/n64/ct1.png",
        .dir = "repos/n64/ct1-recomp",
        .binary = "build-cmake/ChameleonTwistJPRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mystical Ninja Starring Goemon",
        .boxartPath = "assets/images/systems/n64/goemon64.png",
        .dir = "repos/n64/goemon64-recomp",
        .binary = "build-cmake/Goemon64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Quest 64",
        .boxartPath = "assets/images/systems/n64/quest64.png",
        .dir = "repos/n64/quest64-recomp",
        .binary = "build-cmake/Quest64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snowboard Kids",
        .boxartPath = "assets/images/systems/n64/sk1.png",
        .dir = "repos/n64/sk1-recomp",
        .binary = "build-cmake/SnowboardKidsRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Snowboard Kids 2",
        .boxartPath = "assets/images/systems/n64/sk2.png",
        .dir = "repos/n64/sk2-recomp",
        .binary = "build-cmake/SnowboardKids2Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "All-Star Baseball 2000",
        .boxartPath = "assets/images/systems/n64/allstarbaseball2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },

    {
        .title = "All-Star Baseball 2001",
        .boxartPath = "assets/images/systems/n64/allstarbaseball2001.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "All-Star Baseball 99",
        .boxartPath = "assets/images/systems/n64/allstarbaseball99.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Superman: The New Superman Aventures",
        .boxartPath = "assets/images/systems/n64/supermanthenewsupermanaventures.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "The New Tetris",
        .boxartPath = "assets/images/systems/n64/thenewtetris.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Monopoly",
        .boxartPath = "assets/images/systems/n64/monopoly.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monster Truck Madness 64",
        .boxartPath = "assets/images/systems/n64/monstertruckmadness64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "SimCity 2000",
        .boxartPath = "assets/images/systems/n64/simcity2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "South Park",
        .boxartPath = "assets/images/systems/n64/southpark.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "South Park: Chef's Luv Shack",
        .boxartPath = "assets/images/systems/n64/southparkchefsluvshack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "South Park Rally",
        .boxartPath = "assets/images/systems/n64/southparkrally.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Space Invaders",
        .boxartPath = "assets/images/systems/n64/spaceinvaders.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Magical Tetris Challenge",
        .boxartPath = "assets/images/systems/n64/magicaltetrischallenge.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Major League Baseball Featuring Ken Griffey Jr.",
        .boxartPath = "assets/images/systems/n64/majorleaguebaseballfeaturingkengriffeyjr.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman 64",
        .boxartPath = "assets/images/systems/n64/bm64.png",
        .dir = "repos/n64/bm64-recomp",
        .binary = "build-cmake/BM64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Bomberman Hero",
        .boxartPath = "assets/images/systems/n64/bmhero.png",
        .dir = "repos/n64/bmhero-recomp",
        .binary = "build-cmake/BMHeroRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Donkey Kong 64",
        .boxartPath = "assets/images/systems/n64/dk64.png",
        .dir = "repos/n64/dk64-recomp",
        .binary = "build-cmake/DK64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mega Man 64",
        .boxartPath = "assets/images/systems/n64/mm64.png",
        .dir = "repos/n64/mm64-recomp",
        .binary = "build-cmake/MegaMan64Recompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tsumi to Batsu - Hoshi no Keishousha",
        .boxartPath = "assets/images/systems/n64/snp.png",
        .dir = "repos/n64/snp-recomp",
        .binary = "build/SnPRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "BattleTanx: Global Assault",
        .boxartPath = "assets/images/systems/n64/btga.png",
        .dir = "repos/n64/btga-recomp",
        .binary = "build/BattleTanxGARecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Battlezone: Rise of the Black Dogs",
        .boxartPath = "assets/images/systems/n64/bz.png",
        .dir = "repos/n64/bz-recomp",
        .binary = "build/BattlezoneRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turok: Rage Wars",
        .boxartPath = "assets/images/systems/n64/turokrw.png",
        .dir = "repos/n64/turokrw-recomp",
        .binary = "build/PaxDino",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Toy Story 2: Buzz Lightyear to the Rescue!",
        .boxartPath = "assets/images/systems/n64/toystory2buzzlightyeartotherescue.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turok: Dinosaur Hunter",
        .boxartPath = "assets/images/systems/n64/turokdinosaurhunter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turok 2: Seeds of Evil",
        .boxartPath = "assets/images/systems/n64/turok2seedsofevil.png",
        .dir = "repos/n64/turok2-recomp",
        .binary = "build/Turok2Recompiled",
        .appDir = "saves/n64/turok2-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Turok 3: Shadow of Oblivion",
        .boxartPath = "assets/images/systems/n64/turok3shadowofoblivion.png",
        .dir = "repos/n64/turok3-recomp",
        .binary = "build/Turok3Recompiled",
        .appDir = "saves/n64/turok3-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Twisted Edge: Extreme Snowboarding",
        .boxartPath = "assets/images/systems/n64/twistededgeextremesnowboarding.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "V-Rally Edition 99",
        .boxartPath = "assets/images/systems/n64/vrallyedition99.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Tarzan",
        .boxartPath = "assets/images/systems/n64/tarzan.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Bottom of the 9th",
        .boxartPath = "assets/images/systems/n64/bottomofthe9th.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Brunswick Circuit Pro Bowling",
        .boxartPath = "assets/images/systems/n64/brunswickcircuitprobowling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "BattleTanx",
        .boxartPath = "assets/images/systems/n64/battletanx.png",
        .dir = "repos/n64/battletanx-recomp",
        .binary = "build/BattleTanxRecompiled",
        .appDir = "saves/n64/battletanx-recomp",   /* config, saves and the stored ROM */
        .startKeys = "Return",   /* the launcher has no autostart: Start Game */
        .stateDir = "saves/n64/battletanx",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Body Harvest",
        .boxartPath = "assets/images/systems/n64/bodyharvest.png",
        .dir = "repos/n64/bodyharvest-recomp",
        .binary = "build-linux/body-harvest-recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Supercross 2000",
        .boxartPath = "assets/images/systems/n64/supercross2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear Hyper-Bike",
        .boxartPath = "assets/images/systems/n64/topgearhyperbike.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear Overdrive",
        .boxartPath = "assets/images/systems/n64/topgearoverdrive.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Top Gear Rally 2",
        .boxartPath = "assets/images/systems/n64/topgearrally2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Vigilante 8",
        .boxartPath = "assets/images/systems/n64/vigilante8.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Soldier: Vanishing Earth",
        .boxartPath = "assets/images/systems/n64/starsoldiervanishingearth.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars: Rogue Squadron",
        .boxartPath = "assets/images/systems/n64/starwarsroguesquadron.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars: Shadows of the Empire",
        .boxartPath = "assets/images/systems/n64/starwarsshadowsoftheempire.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Star Wars Episode I: Battle for Naboo",
        .boxartPath = "assets/images/systems/n64/starwarsepisodeibattlefornaboo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "StarCraft 64",
        .boxartPath = "assets/images/systems/n64/starcraft64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Starshot: Space Circus Fever",
        .boxartPath = "assets/images/systems/n64/starshotspacecircusfever.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Super Bowling",
        .boxartPath = "assets/images/systems/n64/superbowling.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mia Hamm Soccer 64",
        .boxartPath = "assets/images/systems/n64/miahammsoccer64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Michael Owen's WLS 2000",
        .boxartPath = "assets/images/systems/n64/michaelowenswls2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rally Challenge 2000",
        .boxartPath = "assets/images/systems/n64/rallychallenge2000.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rampage: World Tour",
        .boxartPath = "assets/images/systems/n64/rampageworldtour.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rampage 2: Universal Tour",
        .boxartPath = "assets/images/systems/n64/rampage2universaltour.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rat Attack!",
        .boxartPath = "assets/images/systems/n64/ratattack.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rayman 2: The Great Escape",
        .boxartPath = "assets/images/systems/n64/rayman2thegreatescape.png",
        .dir = "repos/n64/rayman2-recomp",
        .binary = "build/Rayman2Recompiled",
        .appDir = "saves/n64/rayman2-recomp",   /* config, saves and the stored ROM */
        .args = "--boot",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Razor Freestyle Scooter",
        .boxartPath = "assets/images/systems/n64/razorfreestylescooter.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Re-Volt",
        .boxartPath = "assets/images/systems/n64/revolt.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Hybrid Heaven",
        .boxartPath = "assets/images/systems/n64/hybridheaven.png",
        .dir = "repos/n64/hybridheaven-recomp",
        .binary = "build-linux/hybrid-heaven-recomp",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Jet Force Gemini",
        .boxartPath = "assets/images/systems/n64/jfg.png",
        .dir = "repos/n64/jfg-recomp",
        .binary = "bin/JetForceGeminiRecompiled",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "The Legend of Zelda: Ocarina of Time - Master Quest",
        .boxartPath = "assets/images/systems/n64/thelegendofzeldaocarinaoftimemasterquest.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "LEGO Racers",
        .boxartPath = "assets/images/systems/n64/legoracers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Lode Runner 3-D",
        .boxartPath = "assets/images/systems/n64/loderunner3d.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mace: The Dark Age",
        .boxartPath = "assets/images/systems/n64/macethedarkage.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Scooby-Doo!: Classic Creep Capers",
        .boxartPath = "assets/images/systems/n64/scoobydooclassiccreepcapers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: Elmo's Letter Adventure",
        .boxartPath = "assets/images/systems/n64/sesamestreetelmosletteradventure.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Sesame Street: Elmo's Number Journey",
        .boxartPath = "assets/images/systems/n64/sesamestreetelmosnumberjourney.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Rugrats in Paris: The Movie",
        .boxartPath = "assets/images/systems/n64/rugratsinparisthemovie.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Shadowgate 64: Trials of the Four Towers",
        .boxartPath = "assets/images/systems/n64/shadowgate64trialsofthefourtowers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "S.C.A.R.S.",
        .boxartPath = "assets/images/systems/n64/scars.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Duke Nukem: Zero Hour",
        .boxartPath = "assets/images/systems/n64/Duke_Nukem_Zero_Hour.png",
        .dir = "repos/n64/dnzh-recomp",
        .binary = "build-cmake/DNZHRecompiled",
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
        .title = "AeroFighters Assault",
        .boxartPath = "assets/images/systems/n64/aerofightersassault.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Armorines: Project S.W.A.R.M.",
        .boxartPath = "assets/images/systems/n64/armorinesprojectswarm.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Army Men: Air Combat",
        .boxartPath = "assets/images/systems/n64/armymenaircombat.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Army Men: Sarge's Heroes",
        .boxartPath = "assets/images/systems/n64/armymensargesheroes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Army Men: Sarge's Heroes 2",
        .boxartPath = "assets/images/systems/n64/armymensargesheroes2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Asteroids Hyper 64",
        .boxartPath = "assets/images/systems/n64/asteroidshyper64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Automobili Lamborghini",
        .boxartPath = "assets/images/systems/n64/automobililamborghini.png",
        .dir = "repos/n64/lambo-recomp",
        .binary = "build/lamborghini_modern",
        .env = "XDG_CONFIG_HOME=@saves/n64/lambo-recomp XDG_STATE_HOME=@saves/n64/lambo-recomp",   /* starts the race by itself */
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Batman Beyond: Return of the Joker",
        .boxartPath = "assets/images/systems/n64/batmanbeyondreturnofthejoker.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Micro Machines 64 Turbo",
        .boxartPath = "assets/images/systems/n64/micromachines64turbo.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mike Piazza's Strike Zone",
        .boxartPath = "assets/images/systems/n64/mikepiazzasstrikezone.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Milo's Astro Lanes",
        .boxartPath = "assets/images/systems/n64/milosastrolanes.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mischief Makers",
        .boxartPath = "assets/images/systems/n64/mischiefmakers.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Mission: Impossible",
        .boxartPath = "assets/images/systems/n64/missionimpossible.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Monaco Grand Prix",
        .boxartPath = "assets/images/systems/n64/monacograndprix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-1 World Grand Prix",
        .boxartPath = "assets/images/systems/n64/f1worldgrandprix.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F-1 World Grand Prix II",
        .boxartPath = "assets/images/systems/n64/f1worldgrandprixii.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "F1 Pole Position 64",
        .boxartPath = "assets/images/systems/n64/f1poleposition64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "FIFA: Road to World Cup 98",
        .boxartPath = "assets/images/systems/n64/fifaroadtoworldcup98.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "FIFA 99",
        .boxartPath = "assets/images/systems/n64/fifa99.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "FIFA Soccer 64",
        .boxartPath = "assets/images/systems/n64/fifasoccer64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fighter Destiny 2",
        .boxartPath = "assets/images/systems/n64/fighterdestiny2.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fighters Destiny",
        .boxartPath = "assets/images/systems/n64/fightersdestiny.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Fighting Force 64",
        .boxartPath = "assets/images/systems/n64/fightingforce64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Flying Dragon",
        .boxartPath = "assets/images/systems/n64/flyingdragon.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Forsaken 64",
        .boxartPath = "assets/images/systems/n64/forsaken64.png",
        .orient = SLOT_ORIENT_INHERIT,
    },    {
        .title = "Gauntlet Legends",
        .boxartPath = "assets/images/systems/n64/gauntletlegends.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gex 3: Deep Cover Gecko",
        .boxartPath = "assets/images/systems/n64/gex3deepcovergecko.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
    {
        .title = "Gex 64: Enter the Gecko",
        .boxartPath = "assets/images/systems/n64/gex64enterthegecko.png",
        .orient = SLOT_ORIENT_INHERIT,
    },
};

const int n64_game_count = (int)(sizeof(n64_games) / sizeof(n64_games[0]));

#endif /* N64_IMPLEMENTATION_ONCE */
#endif /* N64_IMPLEMENTATION */
