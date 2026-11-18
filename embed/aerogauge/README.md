# AeroGauge NSO adapter

Build the in-process module with:

~~~sh
cmake -S repos/n64/aerogauge-recomp -B repos/n64/aerogauge-recomp/build -DCMAKE_BUILD_TYPE=Release
cmake --build repos/n64/aerogauge-recomp/build --target aerogauge_nso --parallel
~~~

The NSO entry reads `n64.aerogauge_rom` from `config/nso.cfg`. The current
local value points to `repos/n64/aerogauge-recomp/AeroGauge (USA).z64`. AeroGauge
stores its editable `graphics.json` and `enhancements.json` in
`repos/n64/aerogauge-recomp/.nso-data/` for this launch path.

`repos/n64/aerogauge-recomp/nso.cfg` controls the game's initial scene.
`start_race=1:1` starts CANYON RUSH with craft 1, bypassing the ROM's title/menu
flow with its own race transition. Change the track to 1-6 and craft to 1-10;
set `start_race=off` to boot to the title/menu instead.

The adapter runs AeroGauge's game runtime on a worker thread and keeps its RT64
Vulkan surface hidden. RT64 copies its final presented image back into NSO's
frame buffer; the diagnostic software rasterizer is only used if RT64 cannot
start. NSO owns the controller, pause, and exit UI, and AeroGauge's launcher is
suppressed for this launch. The NSO launch also points the frontend at the
recomp build's assets so the unused launcher does not fail during startup.

Suspend slots remain unavailable. AeroGauge's existing debug snapshot only
copies RDRAM and cannot safely restore its native game threads and runtime
queues.
