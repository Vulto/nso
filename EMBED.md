# NSO Game Integration Contract and Embedding Plan

**Status:** Normative software contract for adding or adapting games to NSO.  
**Scope:** All supported integration modes, with detailed requirements for **N64 decompilations, N64 recomps, and native PC ports**.  
**Authority:** When this document and implementation disagree, the implementation in code is authoritative for current behavior; new/updated integrations **MUST** conform to this contract and code behavior **SHOULD** be aligned to it. Contract changes that affect ABI, file protocol, or host guarantees require updating this document and relevant code together.

Primary implementation references:
- `src/nso.h` — `GameEntry`, `NsoRomImport`, path/config helpers
- `src/main.c` — data root, launch dispatch, `EMBED`/child lifecycle, external-menu signal handling
- `src/embed.h` — embedded host, pacing, audio pump, pause/load UI, suspend slots, external bridge host side (`EMBED_IMPLEMENTATION` compiled in `main.c`)
- `embed/nsomod.h` — in-process module ABI (`NSOMOD_API_VERSION`, currently **2**)
- `src/savestate.h` — save-state file naming and paired `.sav/.bmp` operations
- `src/launch.h` — process spawn, env helpers, ROM import
- `src/menu_launcher.c` — generic Linux/X11 child window/process-group menu launcher
- `config/nso.cfg` — runtime data/ROM/config keys

Reference integrations:
- `embed/sm64/*` (`src/systems/n64.h`: Super Mario 64 / sm64ex) — canonical **in-process embedded** N64 decomp/PC-port adapter (GLX pbuffer, dual-DSO, wrapped platform APIs, build-specific suspend snapshots)
- `embed/starfox/*` — in-process recomp module with native versioned snapshot wrapping
- `embed/aerogauge/*` / AeroGauge N64Recomp module — in-process recomp, no safe live-session suspend slots
- `embed/smb`, `embed/smw`, `embed/mm3` — other in-process module patterns
- Ocarina of Time / SoH, Perfect Dark entries in `src/systems/n64.h` — **custom external-menu child** patterns (`externalMenu`, optional `externalSlots`)

---

## 1. Goals and non-goals

### 1.1 Goals
1. Run games **inside NSO's window experience** (embedded in-process module) or, where a separate process is required, under NSO-managed menu/window lifecycle.
2. Provide a uniform in-game overlay (Continue / Load & Create Suspend Point / Restart / To Game Selection) where supported.
3. Make integration behavior **predictable, testable, and reproducible** across N64 decomps, N64 recomps, and PC ports.
4. **Strict data containment:** a game/adaptation **MUST NOT** create, modify, or depend on files outside its allowed NSO-managed locations (see §7). Upstream ports often default to user-home/XDG paths; adapters **MUST** redirect or disable such writes.

### 1.2 Non-goals
- NSO does not distribute ROMs, copyrighted assets, or leaked content.
- NSO core does not currently provide OS-level sandboxing (chroot/container/seccomp/namespace). Isolation is enforced by this contract, adapter design, path/env scoping, and review.
- Generic process launching is not a guarantee of in-window rendering on all platforms; see §3.

---

## 2. Terminology and integration modes

| Term | Meaning |
|---|---|
| **Host / NSO** | The `nso` process owning the Raylib window, event loop, UI, and integration lifecycle. |
| **Data root** | Base directory returned by `GetDataPath` logic in `src/main.c` (normally directory of the `nso` binary, overridable with `NSO_BASE_DIR`). Relative `GameEntry` paths resolve here unless stated otherwise. |
| **Embedded module** | Shared object (`.so` on Linux) loaded in-process via `dlopen` implementing `embed/nsomod.h`, driven by `src/embed.h`. NSO keeps its single window; module renders offscreen and returns frames. |
| **Child process** | Separate executable spawned by `LaunchGame` (`src/main.c`, `src/launch.h`). May be unmanaged UI-wise, generically hosted, or custom-bridge hosted. |
| **Generic menu launcher** | Linux/X11 helper mode (`nso-menu-launcher`, `src/menu_launcher.c`) that reparents/finds child window, pauses process group, and handles Continue/Restart/To Game Selection (no suspend-point protocol). |
| **Custom external-menu bridge** | Child implements NSO's signal/window-id/action-file protocol and pause/resume behavior (`externalMenu=true`, optional `externalSlots`). |
| **Suspend point** | Host-managed in-game slot state: `<stateDir>/Suspend point N.sav` with optional `<stateDir>/Suspend point N.bmp` (`N` = 1..4). Distinct from any game-native save. |
| **Module data path** | Path resolved from `config/nso.cfg` key named by `GameEntry.embedRomKey` and passed as `NsomodConfig.data_path`. Despite legacy name, this is **not necessarily a ROM file** (SM64 uses a data directory; ROM-based modules use a ROM path). |

**Mode selection (normative):**
- `GameEntry.embedRomKey != NULL` → **embedded module** path (`StartEmbeddedGame`).
- `GameEntry.embedRomKey == NULL` → **child process** path (`LaunchGame`).
  - `externalMenu == false` → generic launcher on Linux/X11 where applicable, else ordinary spawn/minimize/restore behavior.
  - `externalMenu == true` → custom external bridge; `externalSlots` advertises suspend save/load support.

The host does **not** infer capabilities beyond these fields and runtime checks.

---

## 3. Host guarantees and responsibilities

### 3.1 Window and rendering ownership
- NSO **MUST** remain the sole owner/creator of the top-level application window and its event loop.
- In embedded mode, NSO composites module frames into its presentation rectangle according to screen mode (`EMBED_SCREEN_43`, `EMBED_SCREEN_PIXEL`, `EMBED_SCREEN_CRT` via `src/embed.h` / UI settings).
- In custom external-menu overlay state, NSO captures/displays a frozen frame while the shared menu is open and coordinates visibility/focus per implementation.

### 3.2 Embedded host behavior (`src/embed.h`)
- Loads module with `dlopen(..., RTLD_NOW | RTLD_LOCAL)`, resolves `nsomod_get_api`, validates ABI/version, framebuffer dimensions, and audio channel count.
- Calls `init` with `NsomodConfig { .data_path, .sample_rate }`.
- Paces game ticks via accumulator logic (cap `dt` to 0.1s, at most 2 ticks/update, drop excess after stall). No game ticks while menu is open, during menu-exit deferral, or while a suspend-point load is pending/completing as implemented.
- Reads host input once per update and passes it to ticks in that update.
- Drives audio through a host stream (nominal 48 kHz path) by pulling module samples in bounded chunks.
- Provides shared pause menu, slot scan/thumbs, safe save (tmp+rename), threaded load flow, restart/reload, and return-to-grid transitions.
- Handles `keep_loaded` semantics: preserved session across Game Selection re-entry (re-`init` to reattach), explicit Restart performs shutdown/init.

### 3.3 Child host behavior (`src/main.c`, `src/launch.h`, `src/menu_launcher.c`)
- Spawns in game working directory (`game->dir` resolved under data root as used by host), optionally performs one-shot `NsoRomImport` before normal launch.
- Exports/removes lifecycle env vars around spawn: `NSO_STATE_DIR`, `NSO_STATE_LOAD`, and (menu paths) `NSO_MENU_DIR`, `NSO_MENU_PARENT_PID`, `NSO_PARENT_WINDOW_ID`.
- Monitors process exit, restores UI state, and can relaunch on external restart flow.
- Generic Linux/X11 launcher (when used) attempts to discover/reparent the child window, manage process-group pause/resume, and bridge End/Guide/menu actions. This requires a discoverable X11 window and Linux process-group semantics.

### 3.4 State directory provisioning
- If `GameEntry.stateDir` is set, host resolves it under data root for embedded/external flows and ensures/uses it for suspend points and grid state UI as implemented.
- Host save-state filename operations **MUST** follow `src/savestate.h` constraints.

---

## 4. Game/adapter obligations (normative)

Applies to all integrations unless a clause is explicitly mode-specific.

1. **No host window theft:** Module/child integration code **MUST NOT** create, destroy, resize in a conflicting way, or take exclusive ownership of NSO's top-level window/context in a manner that breaks host composition.
2. **Deterministic lifecycle:** Initialization, per-frame/update, shutdown/restart, and (if supported) save/load **MUST** be explicit and leave no leaked critical resources on normal teardown.
3. **Context safety (embedded):** Before returning from any host-invoked module callback, the module **MUST** restore NSO's graphics context/state as it found it.
4. **Bounded execution:** Frame/audio work **MUST** be bounded and compatible with host pacing; long stalls degrade UX and may be dropped/capped by host policy.
5. **Explicit capability declaration:** Do not advertise unsupported features (e.g. do not provide `stateDir`/suspend callbacks/`externalSlots` unless semantics are correct and safe).
6. **Failure transparency:** Initialization/save/load failures **MUST** be reported via ABI return values/protocol behavior without corrupting host state or partially applying unsafe state.
7. **Data containment:** **MUST** comply with §7 (no writes outside allowed game locations/repo-managed paths).
8. **Upstream isolation:** Prefer adapters/wrappers/build glue over modifying vendored upstream source in `repos/...` unless explicitly required and documented; pin and document upstream revisions when behavior depends on them.

---

## 5. In-process embedded module contract (`embed/nsomod.h`, ABI v2)

### 5.1 Required export and identity
- Module **MUST** export: `const NsomodApi *nsomod_get_api(void)` (symbol name `NSOMOD_ENTRY`).
- `api->version == NSOMOD_API_VERSION` (currently `2`).
- `fb_w > 0`, `fb_h > 0`, `fps > 0`, and valid audio declaration (`channels` 1 or 2 for `audio_pull` contract as used by host).

### 5.2 `NsomodConfig`
- `data_path`: resolved from `embedRomKey` (absolute if configured absolute, else under data root). Module interprets this as its required ROM file, asset/data directory, or save/data root **as documented by that integration**, but all persistent writes derived from runtime use **MUST** satisfy §7.
- `sample_rate`: host audio rate (e.g. 48000); `0` means no audio.

### 5.3 Callbacks
- `init(cfg) -> bool`: allocate/initialize module runtime and resources. Return `false` on failure; host unwinds.
- `frame(input) -> NsomodFrame`: advance exactly one game tick and return packed **RGB24** framebuffer of fixed `fb_w x fb_h` with stable row layout for the session (or as guaranteed by module). `input` may be `NULL`-equivalent semantics only if module handles it; host typically supplies normalized input.
- `audio_pull(out, max) -> size_t`: return number of **sample frames** produced since last call, interleaved per `channels`, 16-bit PCM (`int16_t`), not exceeding `max`. **MUST** be bounded and define underrun/overrun behavior safely (drop/queue with cap, never unbounded growth).
- `state_save(path) -> bool` (optional but paired with `state_load` for suspend support): write complete state to `path` atomically/safely from module perspective as required by host flow. Host provides slot temp/final paths.
- `state_load(path) -> bool` (optional, paired): validate and restore state; **MUST** reject incompatible/corrupt input without partially corrupting live state.
- `shutdown()`: stop workers, release module-owned resources, and leave process in a clean state for unload or repeated lifecycle per `keep_loaded` rules.

### 5.4 `keep_loaded`
- If `true`, module **MUST** tolerate host keeping session alive across Game Selection transitions and **MUST** make repeated `init` idempotent/re-attach-safe as required by host. Explicit Restart still triggers shutdown/init cycle.

### 5.5 Input model (`NsomodInput`)
- Normalized host fields: digital directions/face/shoulders/select/start, analog `lx,ly,rx,ry` in [-1,1] (y down), `z`.
- Adapter **MUST** implement and document a single, consistent mapping from `NsomodInput` to the target game's native controls. Menu/navigation inputs **MUST NOT** be consumed incorrectly by gameplay mapping.

### 5.6 Rendering/graphics constraints
- Render to offscreen target owned by module (examples: GLX pbuffer/context in SM64, hidden Vulkan/RT surface in recomp adapters).
- **MUST** save/restore host GL/GFX context and drawables around every host callback that touches graphics.
- **MUST NOT** present to or poll NSO's window system event queue as the game frontend.
- Returned pixels are host-uploaded; module must ensure format/dimensions match declaration.

### 5.7 Threading
- Host calls API callbacks serialized from its main/update flow unless explicitly stated otherwise by future contract revision.
- Module-internal workers are allowed only if shutdown/load safety is correct and all access is synchronized; `shutdown` (and load failure paths) **MUST** quiesce/join as needed.

---

## 6. Child process and external-menu contract

### 6.1 Environment variables (host -> child)

| Variable | When | Meaning |
|---|---|---|
| `NSO_STATE_DIR` | `GameEntry.stateDir` configured | Absolute/host-resolved directory for this game's states/saves managed by NSO flows. |
| `NSO_STATE_LOAD` | Grid resume selected | Path/name context for requested startup state (consumed by child if supported). |
| `NSO_MENU_DIR` | Hosted/external menu paths | Directory for IPC files (`nso-menu-window`, `nso-menu-action`). |
| `NSO_MENU_PARENT_PID` | Hosted/external menu paths | NSO PID for `SIGUSR1` menu requests where applicable. |
| `NSO_PARENT_WINDOW_ID` | Hosted/external menu paths (platform-dependent) | Host window identifier for reparent/focus coordination. |

Host clears these from its own environment after spawn; child receives inherited values.

### 6.2 Generic Linux/X11 launcher behavior (`src/menu_launcher.c`)
- Re-exec mode `--menu-launcher <binary>`: runs game in its own process group, locates window (e.g. `_NET_WM_PID`), reparents/resizes under NSO, grabs End/menu trigger, writes `nso-menu-window`.
- On menu request, pauses game process group and signals host; watches `nso-menu-action` for `resume`, `restart`, `quit` actions and resumes/terminates accordingly.
- **No suspend-point save/load protocol** is provided by generic launcher. Use custom bridge if slots are required.

### 6.3 Custom external-menu bridge protocol (host <-> child)
**Child obligations:**
1. Discover menu directory and parent/window identifiers from env.
2. Publish child window id by writing `<NSO_MENU_DIR>/nso-menu-window` when ready (atomic/tmp+rename recommended).
3. On user/host menu request, pause game execution safely and signal parent with `SIGUSR1` (`NSO_MENU_PARENT_PID`).
4. Poll/consume `<NSO_MENU_DIR>/nso-menu-action` written atomically by host (`.tmp` + `rename`).
5. Implement actions:
   - `resume 0`
   - `restart 0`
   - `quit 0`
   - if `externalSlots == true`: `save N`, `load N` with zero-based slot index in protocol file (host slot UI is 1..4)
6. Perform save/load using game-appropriate mechanism and keep execution paused while overlay is active until resumed.
7. Maintain focus/visibility compatibility with host overlay transitions.

**Host obligations:** capture/freeze child frame for overlay (when used), present shared menu, emit action file, and apply lifecycle outcomes (`Resume`, `Restart`, `Quit`/return).

**Capability flags:**
- `externalSlots == false`: host hides slot actions and will not issue slot protocol commands as part of normal flow.
- Child **MUST NOT** claim slot support unless save/load yields a correct, resumable live session for that game.

---

## 7. Save, data, and path-containment contract (MANDATORY)

> **Hard requirement:** The game/adaptation **MUST NOT save or write runtime/user data outside its own allowed locations**. It **MUST NOT** write to arbitrary user-home, XDG, Documents, or system paths unless that path is explicitly inside an allowed location defined below.

### 7.1 Allowed write locations

| Data class | Allowed location | Notes |
|---|---|---|
| **NSO suspend points and thumbnails** | `<NSO data root>/<GameEntry.stateDir>/` (or host-provided equivalent) | Host-owned naming/layout for slots: `Suspend point N.sav/.bmp`; grid `.sav/.bmp` pairs. |
| **Game-native persistent saves/config/state required at runtime** | **Default & required target:** `<NSO data root>/<GameEntry.stateDir>/native/` (or a documented subdirectory under `stateDir`) | Adapters **MUST** redirect EEPROM/SRAM/flash/memcard/config/controller profiles/logs produced by the game. |
| **Read-only module/game data inputs** | `data_path` from `embedRomKey` and/or files under the game's registered `dir`/module bundle inside NSO data root | Treat as read-only at runtime unless a specific file is explicitly designated writable inside `stateDir`. |
| **Temporary files** | Inside the same allowed write directory (e.g. `*.tmp` then atomic rename) | No temp writes to global `/tmp` for persistent game data; transient process scratch must not leak user data outside allowed scope. |

**Special case (existing legacy behavior):** some current integrations may read/write user-global port data (e.g. standalone-port shared dirs). New integrations **MUST NOT** do this. Existing integrations **SHOULD** be migrated by redirecting FS base paths, virtualizing file APIs, or isolating config/save roots into `stateDir`.

### 7.2 Prohibited behavior
- Writing to `$HOME`, `$XDG_DATA_HOME`, `$XDG_CONFIG_HOME`, Steam/OS save locations, or port-default user directories outside allowed paths.
- Scattering saves across multiple unmanaged locations.
- Embedding absolute host-user paths into saved data except where unavoidable and documented (prefer relative/virtualized paths).

### 7.3 Implementation techniques (choose as needed)
- **Embedded modules:** override/wrap file-open, config-save, save/load, path-resolution, and logging APIs used by the port; set port FS base to an allowed directory; replace `main`/platform backends (SM64 pattern).
- **Child processes:** set/override env vars, CLI flags, chdir/cwd, config overrides, or launcher wrappers so all persistent outputs go under allowed paths. If a port cannot be redirected, it is **not contract-compliant** until patched/wrapped.
- **Build artifacts:** extraction/caches generated during build **MUST** remain inside the module/port build directories or documented repo-local locations, never in arbitrary user paths.

### 7.4 Host suspend slot file contract (`src/savestate.h`, `src/embed.h`)
- Slot files: `Suspend point 1..4.sav` and optional `Suspend point 1..4.bmp` in `stateDir`.
- Save path safety: host writes via temporary file and rename on success.
- Load safety: asynchronous load flow must not corrupt live state on failure.
- Grid/state management tools operate on valid paired `.sav/.bmp` names only.
- Module/child save formats are integration-specific but **MUST** be versioned/validated strongly enough to reject incompatible restores safely.

---

## 8. `GameEntry` and configuration contract

`GameEntry` is defined in `src/nso.h` and registered in `src/systems/*.h`.

| Field | Requirement |
|---|---|
| `title`, `boxartPath` | UI metadata; boxart path resolved via data root conventions. |
| `dir` | Game/module directory (NSO-relative under data root unless resolved otherwise by host). **MUST** contain required runtime binary/module/layout. |
| `binary` | Executable or module filename/path relative to `dir` as used by host. |
| `stateDir` | NSO-relative directory for suspend/grid states; set only when supported and §7-compliant. Enables host state UX. |
| `embedRomKey` | Config key for module `data_path` (embedded mode). Value may be absolute or NSO-relative per host resolution. |
| `externalMenu` | Child implements custom external-menu bridge. |
| `externalSlots` | Child supports slot save/load protocol (`externalMenu` should be true). |
| `romImport` | Optional one-shot import/importer hook before launch (`src/launch.h`). |
| `appDir` | NSO-relative dir exported as `APP_FOLDER_PATH` (N64Recomp/RecompFrontend ports keep config, saves and the stored ROM `<game_id>.z64` there instead of `~/.config`). |
| `args` | Extra space-separated arguments, e.g. a port's own direct-boot switch (`--boot`, `--skip-launcher`). |
| `env` | Space-separated `NAME=VALUE` pairs set for the child only; a value starting with `@` is an NSO-relative dir (created if missing), e.g. `XDG_CONFIG_HOME=@saves/n64/<game>`. |
| `startKeys` | X11 keysyms the generic menu launcher types once into the game window ~5 s after it appears, for ports whose launcher screen has no autostart (`Return` = its focused Start Game). |
| `orient` | UI/orientation hint. |

`config/nso.cfg` **MUST** provide every required key for enabled embedded entries and **SHOULD** document expected value type (ROM file path, data directory, etc.). Do not commit user-local secrets or copyrighted material.

---

## 9. Integration plan by game type (N64 decomp / N64 recomp / PC port)

Select the narrowest viable mode that satisfies §3, §4, and §7.

### 9.1 Common discovery and preparation (all types)
1. **Classify upstream project:** decompilation PC port, N64Recomp-based recomp, native/open PC port, or emulator/runtime wrapper.
2. **Inventory runtime I/O:** ROM/assets inputs, save files, configs, caches, logs, shader caches, screenshots, temp files, net/storage features.
3. **Identify frontend ownership:** window creation, swapchain/present, input polling, audio device, main loop, quit/menu handling.
4. **Identify persistence semantics:** in-game saves vs instant/live suspend states; thread/allocator/graphics object lifetime.
5. **Choose mode:** embedded module preferred when feasible; custom external child when process isolation/runtime ownership is required; plain child only when in-window overlay integration is not required (and platform limits accepted).
6. **Define allowed storage map (§7):** exact `stateDir`, `stateDir/native`, and read-only inputs.

### 9.2 N64 decompilation PC ports (e.g. sm64ex-style)
**Goal:** run port logic as library/module inside NSO with host-controlled presentation/input/audio/menu.

**Plan:**
1. **Do not fork-edit upstream source in place.** Create `embed/<game>/` adapter (`<game>mod.c`, optional glue/build scripts, README) that compiles/links upstream objects.
2. **Build as shared libraries with PIC** and stable entrypoints. Split state-bearing/game logic and renderer/adapter concerns as needed for suspend strategy.
3. **Replace platform backends:** window/event loop, rendering surface/context integration, controller input source, audio sink, timer/frame pacing hooks, filesystem base paths, config save/load targets, fatal/error/exit handlers.
4. **Rendering:** offscreen target (e.g. GL/Vulkan pbuffer/hidden surface), per-callback host context save/restore, RGB24 frame export at fixed dimensions/fps.
5. **Input/audio:** map `NsomodInput` deterministically to port controls; resample/mix to host `sample_rate` with bounded buffering.
6. **Persistence & containment:**
   - Route EEPROM/flash/SRAM/controller pak/config to `stateDir/native` (or approved subdir), not user-global port dirs.
   - Implement suspend points only with a safe strategy:
     - **Preferred:** game-native serialize/deserialize of resumable live state.
     - **Conditional (advanced):** raw memory snapshot only if all memory, pointers/relocations, threads, handles, GPU/allocator/external resources are fully defined and version-validated (SM64 example is build-specific and fragile by design).
   - Reject incompatible states on build/version/layout mismatch.
7. **Stability hardening:** convert fatal exits/aborts into recoverable module failures where possible; ensure shutdown/unload safety.
8. **Documentation:** pin upstream commit/tag, document controls, data paths, save format/versioning, and known limitations.

### 9.3 N64Recomp-based recomps
**Goal:** host the recomp runtime in-process when the runtime supports headless/library embedding; otherwise integrate as controlled child with explicit bridge/capability limits.

**Plan:**
1. **Assess runtime API:** identify hooks for frame stepping, framebuffer capture/readback, audio callback/buffer, input injection, pause/reset, and save-state/export APIs.
2. **Select integration shape:**
   - **Embedded module:** compile recomp + generated code/runtime as module target (or loadable library) exposing `nsomod_get_api` (Star Fox/AeroGauge patterns).
   - **Child process:** only if runtime cannot be safely librarified; add custom external menu bridge if pause/save/load can be implemented correctly.
3. **Asset/ROM handling:** `data_path` typically points to ROM or package required by recomp; keep user-supplied ROMs external to git and validate presence/config early.
4. **Rendering ownership:** ensure recomp renderer outputs to module-controlled offscreen target and that NSO receives final RGB frames; suppress desktop launcher/UI windows.
5. **State support:**
   - Prefer native versioned snapshots/export APIs; wrap with NSO-visible validation metadata if useful.
   - If runtime state includes non-restorable native threads/queues/GPU objects, **do not enable suspend slots**; expose honest capability (`stateDir`/callbacks/`externalSlots` omitted or disabled accordingly).
6. **Containment:** redirect recomp-generated config, caches, shader/user data, and extracted runtime files into `stateDir` or documented repo-local allowed subpaths (never unmanaged user-global dirs).
7. **Performance/threading:** isolate game runtime threads from host UI thread hazards; guarantee clean stop/join on shutdown/restart.

### 9.4 Native PC ports (non-emulated originals/remasters/source ports)
**Goal:** same as above: in-process module when embeddable, otherwise bridged child with strict containment.

**Plan:**
1. **Refactor/link strategy:** build core game loop as library or inject adapter layer that can be driven by `frame/audio_pull` and fed host input.
2. **Disable/replace desktop shell:** no standalone launcher/window/menu/input/audio device ownership inside NSO session.
3. **Filesystem virtualization:** remap save/config/profile/mod/userdata paths into `stateDir/native`.
4. **Suspend points:** implement only if save/load is semantically complete for active gameplay state; otherwise rely on in-game saves plus host restart/exit flows as applicable.
5. **If child-only:** implement custom external bridge when overlay integration is required; document platform limitations (e.g. Linux/X11 assumptions).

---

## 10. Build, layout, and repository integration

### 10.1 Recommended layout
- **Host registration:** `src/systems/<system>.h` (`GameEntry`)
- **Embedded adapter:** `embed/<game>/` with `README.md`, build script/Makefile/CMake integration, ABI adapter, and any glue
- **Runtime config:** `config/nso.cfg` key(s) for `embedRomKey` / ROM/data paths
- **State storage:** `saves/<system-or-scope>/<game>/` under NSO data root
- **Vendored/upstream source:** `repos/...` (prefer read-only usage from adapter perspective)

### 10.2 Build rules
- Produce deterministic, documented artifacts (module `.so` path must match `GameEntry.dir/binary`).
- Pin upstream revisions/patchsets when adapter relies on internal symbols/layout/APIs; fail or warn clearly on mismatch (SM64-style pin check recommended).
- Keep generated/extracted/build outputs in gitignored build locations.
- Ensure module exports required ABI symbol and links required runtime dependencies.
- Run and document dependency checks; for repo-managed ports use `src/repos.h` / `tools/repo_sync.sh` conventions where applicable.

### 10.3 Registration checklist
1. Add `GameEntry` with correct mode/capability flags.
2. Add config key(s) and README instructions.
3. Wire build target into root `Makefile` / relevant build flow if in-tree embedded.
4. Verify `GetDataPath`-relative resolution for `dir`, `binary`, `stateDir`, and config-derived data paths.

---

## 11. Meticulous implementation checklist (agent/developer)

### 11.1 Pre-integration analysis
- [ ] Identify game type (N64 decomp / N64 recomp / PC port) and upstream revision.
- [ ] Enumerate all input, render, audio, timing, filesystem, network, and persistence touchpoints.
- [ ] Decide mode and capability set (`embed` vs child, suspend support, external slots).
- [ ] Produce storage map proving §7 compliance.

### 11.2 Embedded module implementation
- [ ] Implement `nsomod_get_api` and full ABI v2 struct.
- [ ] Implement fixed-size offscreen rendering + RGB24 readback/export.
- [ ] Save/restore host graphics context on all relevant paths (success/error/fatal).
- [ ] Implement bounded pacing-compatible `frame` and `audio_pull`.
- [ ] Implement normalized input mapping and document it.
- [ ] Implement `init`/`shutdown` resource ownership; support `keep_loaded` only if semantics are correct.
- [ ] Implement `state_save`/`state_load` only if safe, validated, and versioned; otherwise omit.
- [ ] Redirect all writes to allowed paths; suppress/wrap unsafe config/log/save defaults.
- [ ] Add/verify `GameEntry`, config key, build target, and module path.

### 11.3 Child/external-bridge implementation
- [ ] Implement spawn prerequisites, working dir, and required env handling.
- [ ] Honor `NSO_STATE_DIR`/`NSO_STATE_LOAD` if applicable.
- [ ] Implement `nso-menu-window` publication and `SIGUSR1` pause/request flow (custom bridge).
- [ ] Implement atomic polling/consumption of `nso-menu-action` and all declared actions.
- [ ] Set `externalSlots` only with correct live-session save/load behavior.
- [ ] Ensure all persistent writes are contained per §7.

### 11.4 Documentation and verification artifacts
- [ ] Add `embed/<game>/README.md` (or equivalent) with build, config keys, controls, save locations, limitations, and pin/version notes.
- [ ] Update `EMBED.md`/system docs if behavior/contract changes.
- [ ] Record test evidence for §12 matrix.

---

## 12. Verification and acceptance matrix

An integration is **not complete** until all applicable checks pass.

### 12.1 Core functional
- [ ] Launch from NSO UI succeeds with valid config/data.
- [ ] Clean failure path on missing module/data/ROM/config (returns to UI without host corruption).
- [ ] Frame pacing, presentation, input, and audio operate continuously without host event-loop starvation.
- [ ] Pause/open/close menu works; gameplay does not advance while paused/loading as specified.
- [ ] Continue, Restart, To Game Selection behave correctly.
- [ ] Repeated launch/exit/restart cycles do not leak critical resources or deadlock.

### 12.2 Embedded-specific
- [ ] No extra top-level window created; NSO remains responsive.
- [ ] Host graphics context restored after every frame and on error paths.
- [ ] Module unload/reload (and `keep_loaded` re-entry, if enabled) behaves correctly.

### 12.3 Child/external-specific
- [ ] Spawn/exit detection, focus/window handling, and env cleanup correct.
- [ ] Generic launcher or custom bridge protocol functions on supported platform(s).
- [ ] `resume/restart/quit` and, if enabled, `save/load N` actions are correct and atomic from host perspective.

### 12.4 Save/state and containment (mandatory)
- [ ] Suspend slot create/load/overwrite/empty-slot behavior correct (or slots intentionally unavailable and not advertised).
- [ ] Host slot files written only under `stateDir` with safe naming/tmp-rename semantics.
- [ ] Grid save-state listing/resume interop works when supported.
- [ ] **No files written outside allowed locations during startup, gameplay, menu actions, save/load, config changes, or shutdown.** (verify by auditing wrapped paths/APIs and observing runtime file activity in allowed dirs only)
- [ ] Incompatible/corrupt state inputs are rejected without partial live-state corruption.

### 12.5 Build/reproducibility
- [ ] Clean build from documented steps produces expected artifacts at registered paths.
- [ ] Upstream pin/mismatch detection (if used) reports actionable status.
- [ ] No copyrighted/user-local artifacts committed.

---

## 13. Known current gaps and migration notes

- **Current implementation caveat:** some existing integrations may persist native data in user-global locations (e.g. SM64 uses configured `n64.sm64_dir` for EEPROM/config read/write behavior in upstream design, with only partial write suppression). Such behavior is **legacy/non-conforming** to §7 and **MUST** be migrated when those integrations are updated.
- **Platform caveat:** generic child embedding/menu launcher is Linux/X11-specific in key behaviors; other platforms may fall back to ordinary process lifecycle.
- **Suspend snapshots:** raw-memory snapshotting is highly port-specific and fragile; prefer native serialization/versioned formats. Build/layout drift must invalidate old states safely.

---

## 14. Change control

- Any change to `embed/nsomod.h`, menu/action protocol, slot file semantics, required env vars, or capability flags **MUST** be reflected in this document and validated against reference integrations.
- New N64 decomp/recomp/PC-port integrations **MUST** include: mode rationale, containment map, capability declaration, and verification evidence per §12.
