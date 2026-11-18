# Shared child-game menu launcher

The Linux/X11 launcher wraps a standalone or recompiled game executable and
connects it to NSO's shared **Continue**, **Reset Game**, and **To Game
Selection** menu. It reparents the SDL X11 window into NSO, grabs End to open
the menu, pauses the game's process group while the menu is open, and handles
the host's resume, restart, and quit actions. Suspend points remain disabled
for these entries.

Build NSO normally; the shared launcher is built alongside it:

```sh
make
```

On Linux/X11, NSO automatically uses this launcher for child-process games
that do not already provide their own external-menu integration. Embedded
modules continue to use their in-process menu.
