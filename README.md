# NSO

NSO is a Nintendo Switch Online-style launcher, written in C with Raylib, for
source ports and decompilations of NES, SNES, N64, and GameCube games.

This repository does not include ROMs or copyrighted art. You must supply your
own legally obtained ROMs. Cover images are fetched or added locally; a missing
cover is shown as a "?" placeholder.

## Build

You need:

- A C23 compiler: GCC 14+ or Clang 18+ (the Makefile uses `-std=c23`).
- GNU Make.
- [Raylib](https://www.raylib.com/) **6.0 or newer** (NSO uses `FileMove` and
  `FileRemove`, which Raylib 5.5 and older do not have). The Makefile links
  with plain `-lraylib -lm`, so the library must be on the default linker path.
- At run time: `curl` (cover scraping) and `git` (repository downloads).

Distro packages (compiler, make, curl, git, and what Raylib needs to build):

| Distro | Command |
| --- | --- |
| Gentoo | `emerge --ask sys-devel/gcc dev-build/make net-misc/curl dev-vcs/git dev-build/cmake x11-libs/libX11 x11-libs/libXrandr x11-libs/libXinerama x11-libs/libXcursor x11-libs/libXi media-libs/mesa` |
| Debian / Ubuntu | `sudo apt install build-essential curl git cmake libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev` |
| Arch | `sudo pacman -S --needed base-devel curl git cmake libx11 libxrandr libxinerama libxcursor libxi mesa` (or `raylib` if the repo package is 6.0+) |
| Fedora | `sudo dnf install gcc make curl git cmake libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel` |

Most distros still ship Raylib 5.5, so build Raylib from source:

```
git clone --depth 1 https://github.com/raysan5/raylib.git
cmake -S raylib -B raylib/build -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF
cmake --build raylib/build -j"$(nproc)"
sudo cmake --install raylib/build     # installs to /usr/local
sudo ldconfig                         # if /usr/local/lib64 is new to the loader
```

Then build and test NSO:

```
git clone https://github.com/Vulto/nso.git
cd nso
make
make test
./nso
```

`./nso` works from any directory: data paths are resolved next to the binary
(override with `NSO_BASE_DIR=/path/to/nso`). NSO creates `assets/cache`,
`assets/images/...`, `repos/`, `logs/`, `config/` and `saves/` as needed.

## Usage

| Action | Keyboard | Gamepad (Nintendo layout) |
| --- | --- | --- |
| Move | Arrow keys | D-pad / stick |
| Confirm / start game | Enter | A (right face) or Start |
| Back | B | B (bottom face) |
| Move a game (hold to sort, arrows move it) | Y (hold) | Y |
| Game details panel | X | X |
| Save states of the selected game | S | (from the Details panel) |
| Organization sidebar | O | |

In supported ports (the patched SNES games) F5 saves a state without leaving
and F12 (or Home) saves and returns to NSO.

### Settings screen (gear icon)

On the main menu press Down to select the gear, then Enter/A. Left/Right
switches system (NES, SNES, Nintendo 64, GameCube).

- **Image scraping:** "Missing box art" downloads covers that are not on disk;
  "Replace box art" downloads all of them again. Covers come from
  [libretro-thumbnails](https://github.com/libretro-thumbnails) (index via the
  GitHub API, images from `thumbnails.libretro.com`); no account or API key is
  needed. They are saved to `assets/images/systems/<system>/`.
- **Repository download:** pick a port (or "Download all") to clone it into
  `repos/<system>/...` and build it with `tools/repo_sync.sh`. The status column
  shows progress; full logs are in `logs/repos/<name>.log`. Missing build tools
  are listed as "Missing deps: ...". N64 and GameCube ports are large and can take a
  long time to build.
- **Language:** English / Português.

### ROMs

NSO never downloads ROMs. When a port needs one the status shows
"ROM needed: <file>"; copy your own dump to the path shown (for example
`repos/nes/smbvanilla/smb.nes`, `repos/snes/smw/smw.sfc`,
`repos/n64/sm64ex/baserom.us.z64`) and run the download again to build it.
Fire 'n Ice (OpenDana) takes the European *Solomon's Key 2* ROM dropped directly
in `repos/nes/opendana/`; it is imported on first launch.

### System images

System art is not included. Without it the main menu and the system splash
show the console name as text. To use your own art, save a 1920x1080 PNG at:

- `assets/images/systems/nes/nes.png`
- `assets/images/systems/snes/snes.png`
- `assets/images/systems/n64/n64.png`
- `assets/images/systems/gamecube/gamecube.png`

The menu card shows a square centre crop of the image. The gear icon
(`assets/images/ui/gear.png`, optional) falls back to a drawn gear.

## Supported projects

The launcher can clone, build, and launch the following source ports (see
`src/repos.h`):

**SNES:** smw, zelda3, sm

**NES:** SMB Vanilla, OpenMadoola, OpenDana

**N64:** sm64ex, Ship of Harkinian, 2 Ship 2 Harkinian, Perfect Dark, SpaghettiKart

**GameCube:** Dusklight, melee-pc, Metroid Prime, Paper Cut

### Static recompilations

Also built from source (never prebuilt releases), from the static
recompilations listed on
[retroreversing.com](https://www.retroreversing.com/source-code/decompiled-retail-console-games)
that document a Linux build. Each needs your own ROM dump: put it in the
game's `repos/<system>/<game>-recomp/` folder under the name the gear screen
shows (NES ones need no ROM to build and ask for it when the game starts).

**SNES:** F-Zero, Go Go Ackman, Kirby Super Star, Super Mario All-Stars,
Super Mario RPG, Mega Man X2, Mega Man X3, Star Fox

**NES:** Dr. Mario, Faxanadu, The Legend of Zelda, Mega Man 3, Metroid

**N64:** Bomberman 64, Bomberman Hero, Chameleon Twist, Donkey Kong 64,
Dr. Mario 64, Duke Nukem: Zero Hour, Extreme-G, Harvest Moon 64, Mystical
Ninja Starring Goemon, Quest 64, Snowboard Kids, Snowboard Kids 2, Space
Station Silicon Valley, Mega Man 64, Sin and Punishment, BattleTanx: Global
Assault, Battlezone: Rise of the Black Dogs, Turok: Rage Wars, Yoshi's Story,
Pilotwings 64, Body Harvest, Hybrid Heaven, Wave Race 64, GoldenEye 007, Jet
Force Gemini, Conker's Bad Fur Day, Banjo-Tooie

Most N64 ones need `clang`, `lld` (`ld.lld`), `cmake`, `ninja`, SDL2 and
GTK 3; GoldenEye 007 also needs `xdelta3`. A missing tool is reported in the
gear screen before anything is built.

## Third-party font

`assets/fonts/NotoSansCJKkr-Regular.otf` is Noto Sans CJK by Google/Adobe, licensed under the SIL Open Font License 1.1 (see `assets/fonts/LICENSE-NotoSansCJK.txt`).
