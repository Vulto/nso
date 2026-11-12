/* sm64glue.c - linked into libsm64game.so: the game state the port's
 * src/pc/pc_main.c defines (it is replaced by sm64mod.c), plus the main
 * pool, so all of it sits in the game's .data/.bss that suspend points copy. */
#include "sm64.h"
#include "game/main.h"
#include "game/memory.h"

OSMesg D_80339BEC;
OSMesgQueue gSIEventMesgQueue;

s8 gResetTimer;
s8 D_8032C648;
s8 gDebugLevelSelect;
s8 gShowProfiler;
s8 gShowDebugText;

s32 gRumblePakPfs;
struct RumbleData gRumbleDataQueue[3];
struct StructSH8031D9B0 gCurrRumbleSettings;

/* pc_main.c mallocs it; here it moves with the rest of the game state. */
u64 gNsoMainPool[DEFAULT_POOL_SIZE / sizeof(u64)];
