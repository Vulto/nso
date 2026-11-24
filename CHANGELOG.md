# Changelog

All notable changes to NSO. Newest first. Versions follow the Git tags.

## [Unreleased]

### Fixes
- NSO and the unit tests compile with `-O3 -Werror`: GameSel's ROM/title pointers are initialized, CRT Geometry path buffers are large enough for a full data path, and dat URLs are formatted in a full-size local buffer before being copied to the caller.
- The Home key toggles the main games menu from anywhere. The first press leaves submenus, settings, controls, the End menu or a running game and shows the grid (a running game is parked with the same exit animation as To Game Selection). The second press returns to where you were, resuming a parked game with the same opening animation.
- Leaving a game no longer flashes the screen. A full-window black rectangle fades in over the still-visible game in 0.2 s; only then is the game closed or parked. Once it is gone and the visible covers are ready, that black rectangle shrinks into the game's cover in 0.15 s and reveals the grid. For Mednafen and reparented ports an override-redirect black window is raised above the child so the fade covers it (NSO's own draw sits under that window). The previous exit zoom unmapped the child and finished the shrink before a covering frame was drawn, which is what let a stale or empty frame show through.
- Opening a game uses the same transition in reverse: a black rectangle grows from the selected cover to the full window in 0.15 s, the game is launched or resumed only once the screen is fully black, and after its first frame is showing the black fades out in 0.2 s. Child windows stay under the same overlay, with the same opacity-0 unmap, leftover sweep and focus return as on exit. The previous launch zoom (a dark panel growing out of the card into a 4:3 frame) is gone. Confirming "Close %s?" to open another game uses that same opening animation: the parked game is killed while still hidden and stopped (never remapped or resumed), behind the full black, then the new one launches.
- The exit overlay is now dropped at opacity 0 before unmap (so picom cannot leave a faded ghost), leftover overlay windows from a previous leave are swept, and keyboard focus is returned to NSO after every exit.

### Performance
- Mednafen CRT Geometry (two-pass): the 960×720 pass used a projection-matrix ortho, but Mednafen stores its window transform on MODELVIEW, so the CRT draw missed the FBO and the screen was black with CRT on. The pass now sets MODELVIEW, and the present uses the textured mask blit.
- While a Mednafen game or a reparented port owns the screen, NSO no longer redraws the grid at 60 fps (it waits on input). End, the close prompt and park/resume still work; enter/leave animations still play.
- When a game is running, NSO unloads interface images (covers and their CPU images, platform art, retro/pad/sort/sidebar textures, mosaic and blur targets) to free RAM. The current game's cover is kept so the launch and return-card animations still play. Fonts, the embed/rewind frames and CRT render targets are left alone. On return, covers reload in the background: the visible screen around the cursor first (starting as soon as the user presses the button to leave, during the exit animation), then the first and last screens of the list, then the middle, without blocking input or frames. The grid is shown only once that visible screen is complete — the previous frame is held, with no loading screen. The same visible-first reveal applies at startup, after the existing enter animation.
- CRT Geometry is rendered at about 960×720 (4:3) into a non-MSAA target and scaled up, with the 2-pixel phosphor mask applied at the output. Applied in NSO, Mednafen (`tools/mednafen-nsocrt.patch`) and the hooked ports (SoH, Lighthouse, SpaghettiKart, Perfect Dark).
- CRT Geometry is a plain On/Off toggle again. When it is on, the shader always uses the faster path (no 3-tap scanline filter, cheaper glow, no filmic tone map). The High quality path, the Off/High/Fast cycle and `<system>.crtgeom_fast` are gone; leftover keys in existing configs are ignored.

### Games
- To Game Selection keeps the game paused in the background (embedded modules, Mednafen and external ports); starting it again resumes it instantly.
- Starting another game asks "Close <game>?" (Yes / No) before closing the paused one. Only one game stays alive.
- Quitting NSO closes the paused game.

## [0.2.0] - 2026-11-19

Everything since `v0.1.0`.

### Rewind (NES, SNES)
- Rewind up to 10 seconds in Mednafen games.
- Pad: hold ZL + ZR to rewind. A short ZL + ZR tap still opens the Suspend Menu.
- Keyboard: hold Backspace to rewind.
- The rewind screen follows the reference: the game shrinks, a gray panel rises, a strip of thumbnails runs along the bottom.
- The strip shows one real game frame per second of history. The selected thumbnail is the current frame.
- The caption bar follows "Show controls in game". It shows during the first 5 seconds of a game and while rewinding.
- The game window is always full size when the bar is hidden.

### Controllers
- N64 pad over USB in S mode (8BitDo, hid-nintendo layout): recognized by layout, with the N64 mapping, labels and C buttons.
- 8BitDo N64 mapping, C buttons as UI actions on every gamepad, N64-style hint bar.
- The hint bar follows the connected controller and shows USB or Bluetooth.
- A pop-up shows when a controller connects. USB connect glitches are ignored.
- The Rumble setting now works. Embedded games export an optional Rumble Pak motor, played as an evdev force-feedback effect.
- Rumble off: external games get `nso-norumble.so` preloaded, so SDL rumble does nothing.
- The left analog stick navigates the in-game Suspend Menu like the d-pad.

### Main menu and interface
- System cards are twice as big. The row is re-centered and capped to fit.
- Entering a system plays a timed transition: tile ghost, gray layer, Nintendo wordmark and flipping "PC Ports" letters, crossfade. Leaving fades back.
- Sidebar with video-style rail icons, labelled menu and a per-system Settings panel. The gear icon is gone.
- Details: new game info card with a frosted grid, two-column light panel and fade-in.
- Move mode: hold or tap Y, A confirms, B cancels. The lifted cover glides between slots.
- Sort panel: custom, title, release date, last played, play time.
- Release dates are scraped with the covers and can be used to sort.
- Port status overlay (S): counts per port type, legend and per-game list for the current system.
- NES/SNES Settings: Game Screen cards with thumbnails. Pixel Perfect and Scanlines are independent toggles.
- Menu sound effects (CC0 Kenney UI Audio) with a Sound on/off setting, English and Portuguese. A sound counts as finished after its real length.
- Settings: "Hide unplayable" per system.

### Games
- In-process modules with NSO's pause menu, Load / Create Suspend Point, audio and a zoom out to the card: Super Mario Bros. (vanilla), Super Mario World, Super Mario 64, Star Fox 64 (Starship), Star Fox (SNES), Mega Man 3, AeroGauge.
- NES and SNES games without a native port run in Mednafen: direct start, NSO menu, suspend points, Game Screen filters.
- Every playable child game gets a direct start and the NSO menu (End or the Guide button).
- Source builds with their own config and saves under `saves/n64/<port>`: Banjo-Kazooie, Mario Kart 64, Star Fox 64.
- GoldenEye 007 runs through Project Midas, windowed, with the stored ROM.
- Bomberman 64: The Second Attack starts with `--boot`.
- Blast Corps and Perfect Dark launch from their port roots.
- Static recompilations added for N64, SNES and NES.
- Launch zoom for every game: a dark panel grows out of the card.
- Catalog: 249 display-only N64 entries and 50 NES entries.

### Scraper
- Settings > Image scraping > Select games: check ROMs, download covers and dates, install ports.
- Loose title matching (other releases of the same game, trailing ", The", names without spaces).
- Curl retries and longer timeouts for the flaky NES index and dat downloads.
- Catalog titles are the scrape database names.

### Fixes
- Games launch when the running binary was replaced on disk.
- The menu launcher takes the child's first window that maps. This fixes N64Recomp ports that opened outside NSO.
- The game window is never managed by the tiling window manager.
- Quitting an external game from the NSO menu no longer shows a black cover or a second enter intro.
- Status overlay heading no longer draws a "?".
- Fixes for Star Fox 64, Wave Race 64 and 1080 cover matches.

### Development
- `make check` runs the tests headless and muted. `make test` then opens NSO on `:1`.
- New tests: rewind animation, enter animation, status, sort, hide, game selection, launch, scrape, catalog.
- `EMBED.md` documents the in-process module API.
- `tools/game_install.sh` and `tools/n64rom.py` install ports and convert ROM byte order.

## [0.1.0] - 2026-11-10

- First tagged version: a fresh clone is usable with text placeholders, cwd-independent paths, scrape directories and a README.
