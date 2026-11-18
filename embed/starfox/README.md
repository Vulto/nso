# embed/starfox — Star Fox SNES inside NSO

`libstarfoxmod.so` runs the Star Fox SNES recomp in NSO's window through
`embed/nsomod.h`. It links the recomp's SNES runner and generated Star Fox
code, replacing the desktop main loop, window renderer, and in-game menus with
an NSO frame/input/audio adapter.

- **ROM:** `config/nso.cfg` key `snes.starfox_rom` (default:
  `repos/snes/starfox-recomp/starfox.sfc`). Use your own ROM to generate the
  recompiled sources and as the runtime image.
- **Build:** `make embed` builds the module target from the existing Star Fox
  CMake tree into `embed/starfox/build/libstarfoxmod.so`.
- **Controls:** NSO maps the SNES face buttons, D-pad, shoulders, Select, and
  Start directly to the recomp's 12-bit controller layout. End/Guide opens
  NSO's shared pause menu.
- **Battery save:** `saves/snes/starfox/save.srm`.
- **Suspend states:** NSO stores named `.sav` files in `saves/snes/starfox`.
  The adapter wraps the recomp's native, versioned snapshot with a format
  version, ROM identity, payload length, and CRC32. Loads validate the wrapper
  and Star Fox's native snapshot preflight before restoring game state.

The module uses the native 256x224 PPU output and standard SNES audio. Enhanced
desktop rendering, SDL window/audio devices, and the game's own state menu are
not started inside NSO; NSO owns pacing, presentation, and pause UI.
