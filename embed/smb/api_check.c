/* api_check.c - compile-time check that repos/nes/smbvanilla still offers
 * the API smbmod.c was written against (pinned upstream commit 43777eb).
 * Never linked; `make` runs it first so an upstream change fails here with a
 * named assertion instead of somewhere inside the adapter. */
#include "smbcore/api.h"
#include "smbcore/interface.h"
#include "render_raster.h"
#include "palette_builtin.h"

#define SAME(expr, type, what) \
    _Static_assert(_Generic((expr), type: 1, default: 0), \
                   "smbvanilla API changed (pinned 43777eb): " what)

typedef struct SMB_state_options Opts;
#define FIELD(f) (((Opts *)0)->f)

SAME(&SMB_assets_size, size_t (*)(void), "SMB_assets_size");
SAME(&SMB_assets_init_from_rom_file, bool (*)(struct SMB_assets *, const char *), "SMB_assets_init_from_rom_file");
SAME(&SMB_assets_fini, bool (*)(struct SMB_assets *), "SMB_assets_fini");
SAME(&SMB_state_size, size_t (*)(void), "SMB_state_size");
SAME(&SMB_state_init, bool (*)(struct SMB_state *, const struct SMB_state_options *), "SMB_state_init");
SAME(&SMB_which_game, int (*)(const struct SMB_state *), "SMB_which_game");
SAME(&SMB_start_on_level, void (*)(struct SMB_state *, u8, u8), "SMB_start_on_level");
SAME(&SMB_tick, void (*)(struct SMB_state *), "SMB_tick");
SAME(&SMB_ram, u8 *(*)(struct SMB_state *), "SMB_ram");
SAME(&SMB_ppuram, u8 *(*)(struct SMB_state *), "SMB_ppuram");
SAME(&SMB_ram_finishwrite, void (*)(struct SMB_state *), "SMB_ram_finishwrite");

SAME(FIELD(assets), struct SMB_assets *, "SMB_state_options.assets");
SAME(FIELD(userdata), void *, "SMB_state_options.userdata");
SAME(FIELD(smb2j_load_games_beaten), u8 (*)(void *), "SMB_state_options.smb2j_load_games_beaten");
SAME(FIELD(smb2j_save_games_beaten), bool (*)(void *, u8), "SMB_state_options.smb2j_save_games_beaten");
SAME(FIELD(update_pattern_tables), void (*)(void *, const u8 *), "SMB_state_options.update_pattern_tables");
SAME(FIELD(update_palette), void (*)(void *, const u8 *), "SMB_state_options.update_palette");
SAME(FIELD(draw_tile), void (*)(void *, const struct SMB_tile), "SMB_state_options.draw_tile");
SAME(FIELD(apu_write_register), void (*)(void *, u16, u8), "SMB_state_options.apu_write_register");
SAME(FIELD(apu_end_frame), void (*)(void *), "SMB_state_options.apu_end_frame");
SAME(FIELD(joy1), void (*)(void *, struct SMB_buttons *), "SMB_state_options.joy1");
SAME(FIELD(joy2), void (*)(void *, struct SMB_buttons *), "SMB_state_options.joy2");

/* smbmod.c's state files read these internals (not in api.h) */
#define STATE(f) (((struct SMB_state *)0)->f)
SAME(STATE(ppu), struct ppu_state, "SMB_state.ppu");
SAME(STATE(ppu.scroll_x), u16, "ppu_state.scroll_x");
SAME(STATE(ppu.scroll_y), u16, "ppu_state.scroll_y");
SAME(STATE(ppu.screen_on), bool, "ppu_state.screen_on");
SAME(STATE(reset_occurred), bool, "SMB_state.reset_occurred");
SAME(STATE(which_game), int, "SMB_state.which_game");

SAME(&SMBraster_size, size_t (*)(void), "SMBraster_size");
SAME(&SMBraster_init, bool (*)(struct SMBraster *), "SMBraster_init");
SAME(&SMBraster_fini, void (*)(struct SMBraster *), "SMBraster_fini");
SAME(&SMBraster_provide_palette_lookup, void (*)(struct SMBraster *, const unsigned char *), "SMBraster_provide_palette_lookup");
SAME(&SMBraster_update_palette, void (*)(struct SMBraster *, const unsigned char *), "SMBraster_update_palette");
SAME(&SMBraster_update_pattern_tables, void (*)(struct SMBraster *, const unsigned char *), "SMBraster_update_pattern_tables");
SAME(&SMBraster_draw_tile, void (*)(struct SMBraster *, const struct SMB_tile), "SMBraster_draw_tile");
SAME(&SMBraster_set_buffer, void (*)(struct SMBraster *, unsigned char *, int), "SMBraster_set_buffer");
SAME(&SMBraster_clear, void (*)(struct SMBraster *), "SMBraster_clear");

_Static_assert(sizeof(PALETTE_BUILTIN) == 64 * 3, "smbvanilla API changed (pinned 43777eb): PALETTE_BUILTIN size");
