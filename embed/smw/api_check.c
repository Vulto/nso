/* api_check.c - compile-time check that repos/snes/smw still offers what
 * smwmod.c uses (pinned by header sha256, see Makefile). Never linked. */
#include "assets/smw_assets.h"
#include "snes/ppu.h"
#include "snes/snes.h"
#include "types.h"
#include "common_cpu_infra.h"
#include "common_rtl.h"
#include "config.h"
#include "util.h"
#include "smw_spc_player.h"

#define SAME(expr, type, what) \
    _Static_assert(_Generic((expr), type: 1, default: 0), "smw API changed: " what)
#define FIELD(T, f) (((T *)0)->f)

SAME(&SnesInit, Snes *(*)(const uint8 *, int), "SnesInit");
SAME(&RtlRunFrame, bool (*)(uint32), "RtlRunFrame");
SAME(&RtlRenderAudio, void (*)(int16 *, int, int), "RtlRenderAudio");
SAME(&RtlSaveSnapshot, bool (*)(const char *, bool), "RtlSaveSnapshot");
SAME(&RtlLoadStateFile, bool (*)(const char *), "RtlLoadStateFile (NSO patch)");
SAME(&ReadWholeFile, uint8 *(*)(const char *, size_t *), "ReadWholeFile");
SAME(&PpuBeginDrawing, void (*)(Ppu *, uint8_t *, size_t, uint32_t), "PpuBeginDrawing");
SAME(&SmwSpcPlayer_Create, SpcPlayer *(*)(void), "SmwSpcPlayer_Create");
SAME(&snes_free, void (*)(Snes *), "snes_free");
SAME(&ppu_free, void (*)(Ppu *), "ppu_free");
SAME(&ppu_copy, void (*)(Ppu *, Ppu *), "ppu_copy");
SAME(g_snes, Snes *, "g_snes");
SAME(g_my_ppu, Ppu *, "g_my_ppu");
SAME(g_rtl_game_info, const RtlGameInfo *, "g_rtl_game_info");
SAME(FIELD(RtlGameInfo, draw_ppu_frame), RunOneFrameOfGameFunc *, "RtlGameInfo.draw_ppu_frame");
SAME(FIELD(SpcPlayer, initialize), SpcPlayer_Initialize_Func *, "SpcPlayer.initialize");
SAME(FIELD(Config, new_renderer), bool, "Config.new_renderer");
SAME(FIELD(Config, no_sprite_limits), bool, "Config.no_sprite_limits");
SAME(FIELD(Config, has_keyboard_controls), uint8, "Config.has_keyboard_controls");
_Static_assert(kNumberOfAssets == 178, "smw API changed: kNumberOfAssets");
static const char kSig[] = { kAssets_Sig };
_Static_assert(sizeof(kSig) == 48, "smw API changed: kAssets_Sig");
