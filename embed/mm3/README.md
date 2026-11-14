# embed/mm3 - Mega Man 3 inside NSO's window

`libmm3mod.so` runs [Megaman3NESRecomp](https://github.com/mstan/Megaman3NESRecomp)
(a static recompilation built with NESRecomp, `repos/nes/mm3-recomp`) in NSO's
own process through `embed/nsomod.h`. The port's tree is never edited.

- `CMakeLists.txt` - runs the port's own NESRecomp code generation on your ROM
  (`nesrecomp_add_cycle_game`, the port's `game.toml` and `cyc_seeds.txt`) and
  builds that code with the cycle runtime as a shared library, headless (no
  SDL, no recomp-ui launcher).
- `mm3mod.c` - takes the place of the port's window (`cyc_sdl.c`): one
  `cyc_run_frame()` per NES frame, the 256x240 picture, mono audio, and the
  runtime's own state files for Suspend Points (`saves/nes/mm3`).
- Stock game: the port's window extras (widescreen, co-op, Proto Man, quick
  weapon switch, netplay) are not built in.

## Build

    make embed            # from the NSO root, or `make` here

Needs Linux `cmake`, a C/C++ compiler and Python 3.11+, and
`repos/nes/mm3-recomp` with its submodules (its `setup.sh`). Generated code and
objects go to `embed/mm3/build/` (gitignored).

## ROM

Never copied or committed. `config/nso.cfg` names it:

    nes.mm3_rom=repos/nes/mm3-recomp/Mega Man 3 (USA).nes

It must be the USA cartridge (the port checks CRC 603AAA57, mapper 4); the
module also refuses a ROM that is not the one the code was generated from.
