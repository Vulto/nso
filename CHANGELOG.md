# Changelog

All notable changes to NSO. Newest first. Versions follow the Git tags.

## [Unreleased]

### 0.2.1 (not tagged yet)

Fixes for the first end-user test of 0.2.0. This is still an emulated-games
release. The package is now a tarball: `nso`, a bundled `mednafen` next to it,
and `nso-norumble.so`. NSO looks only for that Mednafen, relative to its own
executable (`/proc/self/exe`), and shows "Bundled Mednafen is missing" if it
is not there. `config/` is created on the first run. Box-art scraping prefers
No-Intro names, downloads from the same per-system libretro-thumbnails repo
as the index, auto-picks a valid name for "Download all", previews the focused
candidate in the picker, and reports failures. The CRT Geometry shader is
embedded (and written to disk so Mednafen can load it). The idle `stat()`
storm on the Mednafen path is cached until a launch. The enter transition
shows the current system name, Details version chevrons render, and the Mega
Drive card logo is no longer cropped. README install and key docs are updated
(S opens the status menu; Power offers Shut down / Close NSO / Cancel).

The Downloads button in the dock is now a self-updater. At startup NSO checks
the latest Vulto/nso release in the background (silent offline, never blocks).
When a newer tag exists the icon glows and, while selected, a note names the
new version. Pressing it downloads the linux asset, verifies it, replaces
`nso`, `mednafen` and `nso-norumble.so` atomically (with rollback) and restarts.

Retest fixes. Box-art scraping now decodes downloads by content (raylib chose
the codec from the `.part` extension, so valid PNGs failed with "Data format
not supported"), uses a unique temp file per preview download instead of one
shared `scrape-choice-preview.part`, validates a cover before replacing it, and
no longer respawns a failed preview every frame. The box-art picker now gets
the keys while Select games is open. Missing covers are cached instead of
stat()ed every frame, and the grid caches launchability for a second, which
cuts the idle stat() rate by about 15x. The window is moved onto a real monitor
if the toolkit places it off-screen. T opens the sort panel again (hold Y still
works). The release build hides the N64 and GameCube systems (no Mednafen
core) and the Recompilation / Native source port statistics; the code stays
(`-DNSO_SHOW_ALL_SYSTEMS`). README: version, Downloads, Settings and supported
systems corrected.

The release tarball now includes CHANGELOG.md (the README links to it). Pad
hot-plug reads /proc/bus/input/devices at most about once a second (sooner if
inotify sees a change under /dev/input), instead of every frame.
Selecting Two Players or Play online in the sidebar shows a short
"Coming soon" toast (the same style as the missing-Mednafen notice).

### Unreleased (after 0.2.1)

"Download all box arts" scrapes only titles that have a ROM in `roms/`
(ports and recomps such as Dr. Mario, Faxanadu, Fire 'n Ice, Madoola no
Tsubasa, Metroid, and The Legend of Zelda are included only when a matching
ROM is present). Name matching prefers a retail dump over a beta, and treats
combined tags like "(Japan, USA)" as USA. The NES catalog includes the
complete name list from the Nintendo Wiki (merged with the existing
entries, no duplicates); Statistics lists those titles with no ROM under
Not available. Hide unplayable is on by default for a fresh config
(an existing saved off is kept). Choosing a filter in the hold-Y menu keeps the menu open and updates the list; B still closes it. Games on the grid come from ROM folders on disk (and catalog native/recomp ports), not from the built-in name list: versions are only the files inside a game folder, labels are taken from the file tags (for example `Abadox (USA)`), and a saved choice is the exact file name. If that file is missing, NSO falls back to USA, then World, Europe, Japan, with hacks last, without editing the config. The name list is still used for display names, cover lookup, and Statistics “Not available”. For NES, the auto-pick and the
Select games picker default prefer a 512x731 portrait among the same-game
candidates (closest size if none match exactly; a single image is used as-is),
looking past dated or hack names so an official 512x731 is not missed.
The release binary now also embeds the UI sound effects (the same files
under `assets/sounds/` that the dev build loads from disk), so a tarball
install plays them without a sounds directory. The Emulated tab no longer
has a "Select games" action or an "Image scraping" heading; Up/Down move
only the game list on the right, and A starts "Download all box arts". S only opens the status menu; B closes it. The Emulated tab is labeled Scraper. On the Scraper tab, X (keyboard or gamepad) starts Download all box arts, and that tab's legend includes the action with the official Nerd Font nf-md-alpha_x_circle glyph (U+F0C31). Other tabs keep the previous legend. The X and Y legend glyphs are rasterized at the same on-screen size as A/B and drawn 1:1 (bilinear), so they are no longer scaled from the 36px atlas.

## [0.2.0] - 2026-11-27

This release supports **emulated games only**. NSO is a Nintendo Switch
Online-style frontend for playing your legally obtained ROMs on the desktop.
Put each system's ROMs in that system's folder under `roms/` (for example
`roms/NES`, `roms/SNES`, `roms/N64`, `roms/GameCube`, `roms/MegaDrive`). Folder
names on disk must match exactly. ROMs may sit loose in the system folder or
inside one level of per-game subfolders. Box art and covers are **not**
shipped: scrape them yourself from the Emulated tab. This package is a single
self-contained Linux binary (`make release`) so end users do not install
libraries or unpack a data directory of UI art.

### Systems and ROMs

- Five systems on the main menu: NES, SNES, Nintendo 64, GameCube, and Mega
  Drive (Genesis). Each has its own card, games grid, settings, and ROM folder.
- ROM detection is the same as the games menu: it scans `roms/<System>/` for
  loose ROM files and for one level of per-game folders, and it matches those
  files to the catalog. GameCube uses the `GameCube` folder name.
- Version grouping: several ROM revisions or region dumps of the same title can
  appear as versions of one grid tile. Same-stem `.ips` / `.bps` / `.ups`
  patches travel with their ROM and can be applied at launch without modifying
  the original file.
- Hide unplayable (per system) lists only games that can actually launch, and
  does not load covers for hidden titles. Toggling it remaps cover slot sizes
  so art still fits when the grid blur clears.
- Favorites: in the Details card (X), Y toggles a heart on the current game.
  Favorites are saved per system in `config/favorites-<id>.txt`. The sort menu
  (T) can filter the grid to favorites only, together with any sort mode and
  Hide unplayable. That filter is stored as `<id>.favorites` in `config/nso.cfg`.
- The games grid capacity is computed from catalog sizes at compile time, so
  adding titles no longer overflows the list and hides a system's games.

### Playing a game

- NES, SNES and Mega Drive games run through NSO's tree Mednafen, with a
  direct start, the NSO in-game menu, suspend points, and the Game Screen
  options (Pixel Perfect, Scanlines, and CRT Geometry).
- Mednafen opens ALSA's `default` device (not `hw:0`), so emulated games have
  sound when launched from NSO.
- Launch zoom: a dark panel grows out of the selected cover into the game.
  Leaving a game fades to black, parks or stops the process, and shrinks back
  into the cover. Open and exit transitions match each other.
- The End key (and the existing in-game path) opens the NSO suspend menu:
  Load / Create Suspend Point, Game Screen / CRT options where they apply, and
  Quit. The left analog stick navigates that menu like the d-pad (deadzone,
  edge, armed after centering).
- Creating a suspend point no longer flashes the live game over the End menu.
  The game is mapped off-screen just long enough to accept the save keys; the
  slot thumbnail is the already-frozen frame.
- Home (keyboard Home only) toggles the main systems menu over a running or
  parked game, with a short fade, and unloads games-grid covers until you enter
  a system again. On the systems menu, End quits NSO (and any parked game)
  through the normal shutdown path. Home is grabbed on the game window the same
  way End already was, including Caps/NumLock combinations.
- Rewind (NES and SNES, Mednafen): hold ZL + ZR on the pad or Backspace on the
  keyboard to rewind up to about 10 seconds. A short ZL + ZR tap still opens
  the suspend menu. The rewind screen shrinks the game, raises a gray panel,
  and shows a strip of one real thumbnail per second of history. The caption
  bar follows "Show controls in game": it appears for the first five seconds of
  a game and while rewinding. The game window stays full size when the bar is
  hidden.
- CRT Geometry (shader ported from MAME-PSGS, Apache-2.0) is available for
  Mednafen and for games NSO draws itself. An End-menu sub-screen offers 12
  sliders, per game, with a live preview. Rendering uses a ~960×720 4:3 pass
  and a 2-pixel phosphor mask at the output. The Fast path is used; On/Off
  remains a toggle. Mednafen CRT no longer goes black on the 960×720 pass.
- While a game owns the screen, NSO does not keep redrawing the grid at 60 fps
  (it waits on input). End, Home, the close prompt and park/resume still work;
  enter/leave animations still play.
- When a game is running, NSO unloads interface images (covers, platform art,
  menu textures, mosaic and blur targets) to free RAM, keeping the current
  game's cover for the return animation. Fonts and CRT targets stay loaded. On
  return, covers reload in the background (visible screen first, no loading
  screen). The same visible-first reveal applies at startup after the enter
  animation.

### Main menu

- The system row uses large cards (about twice the earlier size), re-centered
  and capped to fit the screen.
- A dock bar on the main menu holds Downloads and Power. Power opens a
  shutdown dialog with (A) OK / (B) cancel. The bar is content-sized, with a
  5 px end pad and a 20 px gap between circles at 1920×1080 (circle 110 px,
  vertical margin 4.5 px).
- The dock cursor and the system-card select ring use the same pastel rainbow
  rotating gradient (CPU SDF, not a backbuffer shader). Only one selection
  indicator is on screen at a time (the card ring hides while the bar is
  focused).
- Entering a system (A on a card) plays a time-based transition: a tile ghost,
  a dark gray layer, the Nintendo wordmark, and a crossfade to the games
  screen. The enter animation is faster than before (covers still begin
  loading on the A press). Leaving a system fades through the same gray to the
  main menu. Games-menu input is ignored until the enter animation (tile delay
  and grow) has finished, so S cannot be pressed mid-transition.
- A click plays on A for every system; a per-system select jingle plays once
  the system image is fully shown. The splash slides to the covers only after
  that sound ends (or after 1 s if sound is off) and the covers are ready.
- Menu sound effects (CC0 Kenney UI Audio) can be turned off in Settings.
  English and Portuguese strings are included. A sound is treated as finished
  only after its real length plus output latency.

### Games grid and menus

- A mosaic/grid of box arts, with a "?" placeholder when a cover is missing.
  Page slides are linear and slightly faster than earlier builds.
- Move mode (Y): tap or hold Y to lift a cover, move it between slots, A
  confirms, B cancels. The cover glides; drop-slot markers show the target.
- Sort panel (T, or C-Right on an N64 pad): custom order, title, release date,
  last played, and play time, plus the Favorites filter and Hide unplayable
  (Hide unplayable lives here, not in Settings). Release dates are scraped
  together with covers.
- Details (X): a two-column game info card with fade-in. Favorites toggle with
  Y as described above.
- Status and scraper menu (S) is a tabbed card on the current system, with the
  same colors and frame as Details, slightly shorter. Y cycles tabs; the
  legend is B Back, A Accept, Y Change tabs (Y uses the Nerd Font
  `alpha_y_circle` glyph, matched in size and alignment to A and B). The
  default tab is Statistics, so S still feels like the familiar status view.
  Tabs sit inside the right panel. The status legend uses the same show/hide
  animation as the other subtitle bars; only one legend bar is drawn at a
  time.
  - **Statistics:** playable percentage, type breakdown and the per-game list
    for the current system (as in the previous status overlay).
  - **Emulated:** lists the ROMs detected in `roms/<System>/`. The left side
    shows the "Image scraping" title, **Download all box arts**, **Select
    games**, live progress, and a large preview of the focused game's box art.
    The right side lists the detected games (with thumbs when a cover already
    exists). Download all scrapes missing box arts for every detected ROM.
    Select games lets you check just some titles, with the same live preview,
    then download box art for the selection. If a game has several candidate
    images, a picker lets you choose one (A accept, B skip). The old "Replace
    artbox" action is not in this tab. ROM files are never modified; only
    cover images are written where the scraper already wrote them. If no ROMs
    are found, the message names the real folder (for example `roms/NES`).
  - **PC Ports** and **Decomps** tabs are present in the menu but this release
    is for emulated games only.
- The sidebar is a video-style rail with labelled items and a per-system
  Settings panel (Game Screen, Controller, sound, and related rows). Image
  scraping lives in the Emulated tab, not in the sidebar. Controller mapping
  stays on Settings.
- English and Portuguese UI throughout.

### Box-art scraping

- Covers are not included in the binary or the release archive. Users scrape
  every image themselves.
- From the Emulated tab you can download all missing box arts for the current
  system's detected ROMs, or select a subset with a preview.
- Live progress is shown for each step (index, dates, per-game title and
  n/d). When two or more candidate images match a title, you choose one.
- Matching understands other releases of the same game, trailing ", The", and
  names that omit spaces. Catalog titles follow the scrape database names.
- Curl retries (two retries, three seconds apart) and a longer timeout cover
  flaky index and dat downloads.
- Scraped covers and dates go to the usual image and config locations, not
  into ROM files.

### Controllers

- Per-system controller screens in Settings for NES, SNES, N64, and GameCube,
  with on-screen pad art (including GameCube lines and the N64 controller
  graphic).
- Map Controls starts a Mednafen-style sequential capture: one prompt, a
  blinking button, edge-triggered reading, a wait for all inputs to be
  released before and between assignments, hold-to-skip, and Reset to
  Defaults. Hold times are 0.9 s; leave the screen by holding B (a ring on
  the hint bar's B glyph). Only a button's own bindings light it.
- NES, SNES and N64 mappings are applied to Mednafen (and to any in-process
  modules that read the same maps). Files are stored as `<system>.map.*`.
- N64: 8BitDo N64 mapping, C buttons as UI actions, N64-style hints. The hint
  bar follows the connected controller and shows USB or Bluetooth. A pop-up
  appears when a pad connects; USB connect glitches are ignored. An 8BitDo
  N64 in S mode over USB (hid-nintendo layout, even when it reports a Pro
  Controller id) is recognized by layout (C buttons, no right stick) and
  treated as an N64 pad. The Rumble setting works (evdev force-feedback).
  With Rumble off, external processes can be started with `nso-norumble.so`
  preloaded so SDL rumble does nothing.
- Nerd Font C-Up / C-Down marks on the N64 hint bar, and the same hint style
  for the status-menu Y button.

### Self-contained Linux binary

- `make` still builds the normal developer `nso`, which loads fonts and UI
  art from the tree next to the binary (or `NSO_BASE_DIR`).
- `make release` builds `dist/nso`: a single binary with NSO's own assets
  embedded (fonts, font licenses, main-menu system cards, UI icons and pad
  art, and the title lists under `assets/data/`). Scrapable game box arts and
  covers are **not** embedded and are **not** shipped.
- Raylib is linked statically. glibc, OpenGL (`libGL` and related) and X11
  (`libX11`, `libXtst`, `libXext` and their usual transitive libraries) stay
  dynamic, as do `libm`, `libdl` and the dynamic linker. End users do not
  need to install Raylib or unpack a UI-art directory.
- At runtime the binary still uses `NSO_BASE_DIR` (or the directory of the
  executable) for config, saves, logs, ROMs and scraped covers. It starts
  even when that directory has no fonts or system art on disk.

### Other changes and fixes since 0.1.0

- Interface images and covers unload while a game runs and reload on return,
  as described above; this also applies when coming back from Home.
- KEY_END only opens the in-game End menu again (same launcher keycode grab
  and SIGUSR1 path). KEY_HOME is the systems-menu toggle described above.
- Games still launch if the running NSO binary was replaced on disk
  (`/proc/self/exe` would otherwise show `(deleted)`).
- The menu launcher marks the child window override-redirect so a tiling
  window manager does not steal it, reparents as soon as it maps, and uses
  the first window that maps.
- Quitting from the NSO menu no longer shows a black cover or a second enter
  intro while the process is still shutting down.
- Background blur on the side menu is stronger; Details and the status card
  are unchanged.
- The games-menu page slide is linear (constant speed) and a little faster.
- `make check` runs the unit tests headless and muted. `make test` is the
  developer helper that then opens NSO on the local display.
- New and updated tests include rewind and enter animations, status, sort,
  hide, favorites, game selection, launch, scrape, catalog, version scan,
  pad map, CRT parameters, language, and sysstatus.
- NSO and the tests compile with `-O3 -Werror`.

## [0.1.0] - 2026-11-10


- First tagged version: a fresh clone is usable with text placeholders, cwd-independent paths, scrape directories and a README.
