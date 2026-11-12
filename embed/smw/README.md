# embed/smw — Super Mario World inside NSO

`libsmwmod.so` runs snesrev's Super Mario World reimplementation
(`repos/snes/smw`, native C, not an emulator) in NSO's window through
`embed/nsomod.h`. The port's tree is never edited: `make` compiles its sources
as they are into `build/`, replacing only `src/main.c`, `src/config.c` and the
GL renderer with `smwmod.c`.

- **Data:** `config/nso.cfg` `snes.smw_dir=repos/snes/smw` (gitignored). The
  module reads `smw_assets.dat` there (made once from your own ROM by the port's
  `assets/restool.py`); no ROM is loaded at run time and nothing is emulated.
  Battery saves stay in the port's `saves/smw.srm`, shared with the stand-alone port.
- **Pin:** the port is not a git checkout, so the Makefile pins the sha256 of
  the headers it uses; a change re-runs `api_check.c`.
- **States:** `state_save` / `state_load` are the port's own snapshot files
  (`RtlSaveSnapshot` / `RtlLoadStateFile`), the same `.sav` NSO's Save States
  list shows (`saves/snes/smw`).
- **Reset Game** reloads the module (the port cannot reset without a ROM).
- Console output of the port is dropped (link-time wraps); an `assert` in the
  port still aborts NSO.

    make -C embed/smw          # or: make embed
