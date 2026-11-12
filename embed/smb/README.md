# embed/smb - SMB Vanilla inside NSO's window

`libsmbmod.so` runs [SMB Vanilla](https://codeberg.org/dannysp/SMB-Vanilla)
(a C decompilation of Super Mario Bros, no emulation) in NSO's own process:
NSO calls one game frame per NES frame and draws the 256x240 picture itself
(`src/embed.h`, NES card "Super Mario Bros (SMB Vanilla)").

- `smbmod.h` - the module API shared with NSO (`smbmod_get_api()`).
- `smbmod.c`, `smbmod_audio.*` - take the place of SMB Vanilla's SDL platform
  layer (`src/platform/main_sdl.c`, `audio_sdl.cpp`).
- `api_check.c`, `api_check_apu.cpp` - compile-time checks of every upstream
  function / field the adapter uses.

## Build

    make embed            # from the NSO root, or `make` here

Needs `repos/nes/smbvanilla` (Install it from the gear screen, or clone it
with its `Nes_Snd_Emu` submodule). Its files are only read and compiled;
objects and the `.so` go to `embed/smb/build/` (gitignored).

## ROM

Never copied or committed. `config/nso.cfg` names it:

    nes.smb_rom=repos/nes/smb/Super Mario Bros (U).nes

(NSO-relative or absolute; SHA-1 ea343f4e445a9050d4b4fbac2c77d0693b1d0922.)

## Upstream updates

Written against SMB Vanilla **43777eb**. After a `git pull` in
`repos/nes/smbvanilla`, run `make embed` again:

- same API: it rebuilds against the new sources (a note says the checkout is
  no longer at the pinned commit);
- changed API: the build stops at the API check with
  `smbvanilla API changed (pinned 43777eb): <function or field>`, and the
  previous `build/libsmbmod.so` is left in place; adapt `smbmod.c`, then
  move `PIN` in the Makefile;
- moved / missing files: `smbvanilla layout changed ... missing <files>`.

The checkout is never edited, so `git pull` there never conflicts.

License: SMB Vanilla is GPL-3.0, Nes_Snd_Emu LGPL-2.1; loading the module
in-process makes a combined work when distributed.
