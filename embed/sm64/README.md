# embed/sm64 — Super Mario 64 inside NSO

`libsm64mod.so` runs sm64ex (`repos/n64/sm64ex`, the decompiled game in native
C, not an emulator) in NSO's window through `embed/nsomod.h`. The port's tree
is never edited. `make` runs the port's own Makefile, which writes its objects
to `build/port` with `-fPIC`. The objects are then linked here, with
`sm64mod.c` / `sm64glue.c` in place of `src/pc/pc_main.c` and the SDL window,
audio and controller backends.

- **Build data:** the port's Makefile extracts the assets from your own ROM
  into the port's tree (gitignored there) and compiles them into
  `build/libsm64game.so`. The ROM is `<n64.sm64_dir>/baserom.us.z64` (or
  `SM64_ROM=`), symlinked as the port's `baserom.us.z64`, never copied.
  Nothing from it is committed.
- **Run data:** `config/nso.cfg` `n64.sm64_dir` (gitignored) is the port's
  user directory, e.g. `~/.local/share/sm64ex`. The EEPROM save
  `sm64_save_file.bin` is shared with the stand-alone port, and
  `sm64config.txt` is read but never written: the options menu's save is a no-op.
- **Video:** gfx_pc + gfx_opengl draw into a 640x480 GLX pbuffer of the
  module's own (current only inside its calls, NSO's context is restored).
  Each frame is read back as RGB24. 30 fps.
- **Audio:** the game's 32 kHz stereo, resampled to NSO's rate.
- **Pad:** arrows / left stick = stick, X = A, Z = B, Left Shift or ZL / ZR = Z,
  C = L, V = R, Enter = Start, I J K L or right stick = C buttons.
- **States:** the port has no save states, so a suspend point is the game
  library's `.data` / `.bss`, gzip'd. That covers every game variable, the
  main pool and the audio heap. The renderer lives in the other library and
  is never part of a state. The library loads at a different address on
  every run, so a state records the old addresses, and loading moves every
  4-aligned word that pointed into either library. A state fits only the
  build that wrote it (GNU build-id of both libraries). After a rebuild, old suspend points are
  refused, not loaded.
- **Pin:** written against sm64ex `d7ca2c0`; `make` notes a different HEAD.
- Console output of the port is dropped (link-time wraps). `sys_fatal`
  returns to NSO and leaves the module dead. An `assert` in the port still
  aborts NSO.

    make -C embed/sm64          # or: make embed
