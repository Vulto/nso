# NSO

NSO is a Nintendo Switch Online-style launcher, written in C with Raylib, for
source ports and decompilations of NES, SNES, N64, and GameCube games.

This repository does not include ROMs or copyrighted art. You must supply your
own legally obtained ROMs. Cover images are fetched or added locally; a missing
cover is shown as a "?" placeholder.

## Build

Requires a C compiler, Make, and [Raylib](https://www.raylib.com/).

```
make
make test
```

`make` writes the `nso` binary to the project root. Run it from that directory.

## Supported projects

The launcher can clone, build, and launch the following source ports (see
`src/repos.h`):

**SNES:** smw, zelda3, sm

**NES:** SMB Vanilla, OpenMadoola, OpenDana

**N64:** sm64ex, Ship of Harkinian, 2 Ship 2 Harkinian, Perfect Dark, SpaghettiKart

**GameCube:** Dusklight, melee-pc, Metroid Prime, Paper Cut
