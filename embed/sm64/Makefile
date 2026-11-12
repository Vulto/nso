# embed/sm64/Makefile - builds build/libsm64mod.so + build/libsm64game.so
# (sm64ex, Super Mario 64, in-process module loaded by src/embed.h) from
# repos/n64/sm64ex WITHOUT editing that checkout: the port's own Makefile
# builds its objects (-fPIC) into build/port, then they are linked here with
# sm64mod.c / sm64glue.c in place of src/pc/pc_main.c and the SDL backends.
#
#   make          port objects, then build/libsm64mod.so
#   make clean
#
# The port's Makefile extracts the assets from your own ROM into the port's
# tree (gitignored there): SM64_ROM, default <n64.sm64_dir>/baserom.us.z64
# from config/nso.cfg, is symlinked as the port's baserom.us.z64 (never copied).
#
# Upstream pin: written against sm64ex d7ca2c0. After a `git pull` of the port
# `make` says so; a changed pc_main.c / backend API fails the link (undefined
# or duplicate symbols) or the compile of sm64mod.c, naming what moved.
SM64 ?= ../../repos/n64/sm64ex
PIN  := d7ca2c0
B    := build
PB   := $(abspath $(B))/port/us_pc
CFG_DIR := $(shell sed -n 's/^n64\.sm64_dir=//p' ../../config/nso.cfg 2>/dev/null)
SM64_ROM ?= $(CFG_DIR)/baserom.us.z64

CC ?= cc
# The port's flags for its sources (VERSION=us, default backends' headers).
PORT_CFLAGS := -O2 -fPIC -U_FORTIFY_SOURCE -fno-strict-aliasing -fwrapv -I$(SM64)/include -I$(PB) -I$(PB)/include -I$(SM64)/src \
               -I$(SM64) -DRAPI_GL=1 -DWAPI_SDL2=1 -DAAPI_SDL2=1 -DCAPI_SDL2=1 -DHAVE_SDL2=1 \
               $(shell sdl2-config --cflags) -DVERSION_US -D_LANGUAGE_C -DNON_MATCHING -DAVOID_UB -DF3DEX_GBI_2E

# Port objects: the game library gets everything but the PC platform layer
# (except its N64 OS stubs, audio mixer and cheats, which are game state).
PC_GAME := src/pc/ultra_reimplementation.o src/pc/mixer.o src/pc/cheats.o
PC_MOD  := src/pc/gfx/gfx_pc.o src/pc/gfx/gfx_cc.o src/pc/gfx/gfx_opengl.o src/pc/configfile.o src/pc/ini.o \
           src/pc/cliopts.o src/pc/platform.o $(patsubst $(PB)/%,%,$(wildcard $(PB)/src/pc/fs/*.o))
ALL_O   := $(patsubst $(PB)/%,%,$(shell find $(PB) -name '*.o' 2>/dev/null))
GAME_O  := $(filter-out src/pc/%,$(ALL_O)) $(PC_GAME)

# -U_FORTIFY_SOURCE: printf stays printf (not __printf_chk), so the wraps catch it.
# Console output dropped, fatal errors caught, the user's config never rewritten (sm64mod.c).
WRAP := printf puts putchar fprintf sys_fatal configfile_save

all: port
	@$(MAKE) --no-print-directory $(B)/libsm64mod.so

port: | $(SM64)/baserom.us.z64
	@head=$$(git -C $(SM64) rev-parse --short=7 HEAD 2>/dev/null); \
	if [ "$$head" != "$(PIN)" ]; then echo "note: sm64ex is $$head, adapter pinned to $(PIN)" >&2; fi
	$(MAKE) -C $(SM64) VERSION=us BUILD_DIR_BASE=$(abspath $(B))/port OPT_FLAGS="-O2 -fPIC -U_FORTIFY_SOURCE"

$(SM64)/baserom.us.z64:
	@test -f "$(SM64_ROM)" || { echo "sm64: no ROM at '$(SM64_ROM)' (set n64.sm64_dir in config/nso.cfg or SM64_ROM=)" >&2; exit 1; }
	ln -s "$(abspath $(SM64_ROM))" $@

$(B)/sm64glue.o: sm64glue.c
	$(CC) $(PORT_CFLAGS) -c $< -o $@
$(B)/sm64mod.o: sm64mod.c reloc.h ../nsomod.h
	$(CC) $(PORT_CFLAGS) -Wall -Wextra -c $< -o $@

$(B)/libsm64game.so: $(addprefix $(PB)/,$(GAME_O)) $(B)/sm64glue.o
	$(CC) -shared -o $@ $^ -Wl,-soname,libsm64game.so -Wl,-z,now -Wl,-z,relro -Wl,-Bsymbolic \
	    -Wl,--build-id=sha1 $(WRAP:%=-Wl,--wrap=%) -lm

$(B)/libsm64mod.so: $(B)/sm64mod.o $(addprefix $(PB)/,$(PC_MOD)) $(B)/libsm64game.so
	$(CC) -shared -o $@ $(B)/sm64mod.o $(addprefix $(PB)/,$(PC_MOD)) -L$(B) -lsm64game -Wl,-rpath,'$$ORIGIN' \
	    -Wl,--no-undefined -Wl,-Bsymbolic -Wl,--build-id=sha1 $(WRAP:%=-Wl,--wrap=%) -lGL -lX11 -lz $(shell sdl2-config --libs) -lm

clean:
	rm -rf $(B)

.PHONY: all port clean
