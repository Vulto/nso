# embed/smw/Makefile - builds build/libsmwmod.so (snesrev smw, Super Mario
# World, in-process module loaded by src/embed.h) from repos/snes/smw WITHOUT
# touching that tree: its sources are compiled as they are, objects go to build/.
#
#   make          API check, then build/libsmwmod.so
#   make check    API check only
#   make clean
#
# Pin: repos/snes/smw is not a git checkout, so the adapter is pinned to the
# sha256 (first 12) of the headers it uses (PIN_FILES). When they change,
# `make` says so and re-runs the API check (api_check.c), which names what moved.
SMW ?= ../../repos/snes/smw
PIN := 2bdf1f9f632c
PIN_FILES := $(addprefix $(SMW)/,src/common_rtl.h src/common_cpu_infra.h src/smw_spc_player.h \
             src/snes/ppu.h src/snes/snes.h src/config.h src/util.h assets/smw_assets.h)
B := build

CC ?= cc
SDLINC := $(shell sdl2-config --cflags 2>/dev/null)
# As the port's Makefile (-O2 -fno-strict-aliasing), minus -Werror; SDL only
# for <SDL_keycode.h> in config.h (nothing links SDL).
CORE := -std=gnu11 -O2 -fno-strict-aliasing -fPIC -fvisibility=hidden -w -U_FORTIFY_SOURCE \
        -DSYSTEM_VOLUME_MIXER_AVAILABLE=0 -I$(SMW) -I$(SMW)/src $(SDLINC)
# The port's main.c / config.c (SDL platform, smw.ini) and its GL renderer
# are replaced by smwmod.c.
SKIP := main.c config.c opengl.c glsl_shader.c
SRC_SRC   := $(filter-out $(addprefix $(SMW)/src/,$(SKIP)),$(wildcard $(SMW)/src/*.c))
SNES_SRC  := $(wildcard $(SMW)/src/snes/*.c)
SMB1_SRC  := $(wildcard $(SMW)/smb1/*.c)
SMBLL_SRC := $(wildcard $(SMW)/smbll/*.c)
OBJS := $(patsubst $(SMW)/src/%.c,$(B)/src_%.o,$(SRC_SRC)) \
        $(patsubst $(SMW)/src/snes/%.c,$(B)/snes_%.o,$(SNES_SRC)) \
        $(patsubst $(SMW)/smb1/%.c,$(B)/smb1_%.o,$(SMB1_SRC)) \
        $(patsubst $(SMW)/smbll/%.c,$(B)/smbll_%.o,$(SMBLL_SRC)) \
        $(B)/smwmod.o

NEEDED := $(PIN_FILES) $(SMW)/src/common_rtl.c $(SMW)/src/common_cpu_infra.c $(SMW)/src/smw_rtl.c
MISSING := $(filter-out $(wildcard $(NEEDED)),$(NEEDED))

all: $(B)/libsmwmod.so

$(B):
	mkdir -p $(B)

ifneq ($(MISSING),)
ifneq ($(MAKECMDGOALS),clean)
$(error smw layout changed (pinned $(PIN)) or not present: missing $(MISSING))
endif
endif
$(B)/api.ok: api_check.c $(NEEDED) | $(B)
	@sum=$$(cat $(PIN_FILES) | sha256sum | cut -c1-12); \
	if [ "$$sum" != "$(PIN)" ]; then \
		echo "note: smw headers are $$sum, adapter pinned to $(PIN); checking API" >&2; fi
	@$(CC) -std=gnu11 -fsyntax-only -w -I$(SMW) -I$(SMW)/src $(SDLINC) api_check.c || \
	 { echo "smw API changed (pinned $(PIN)): update embed/smw/smwmod.c, see the assertion above" >&2; exit 1; }
	@touch $@

check: $(B)/api.ok

$(B)/src_%.o: $(SMW)/src/%.c $(B)/api.ok
	$(CC) $(CORE) -c $< -o $@
$(B)/snes_%.o: $(SMW)/src/snes/%.c $(B)/api.ok
	$(CC) $(CORE) -c $< -o $@
$(B)/smb1_%.o: $(SMW)/smb1/%.c $(B)/api.ok
	$(CC) $(CORE) -c $< -o $@
$(B)/smbll_%.o: $(SMW)/smbll/%.c $(B)/api.ok
	$(CC) $(CORE) -c $< -o $@
$(B)/smwmod.o: smwmod.c ../nsomod.h $(B)/api.ok
	$(CC) -std=gnu11 -O2 -Wall -Wextra -Wno-unknown-pragmas -fPIC -fvisibility=hidden -I$(SMW) -I$(SMW)/src $(SDLINC) -c $< -o $@

# Core's relative fopen/rename land in the port's directory and its console
# output goes nowhere (smwmod.c): NSO prints nothing.
WRAP := fopen rename printf puts putchar fprintf
$(B)/libsmwmod.so: $(OBJS)
	$(CC) -shared -Wl,--no-undefined -Wl,-Bsymbolic $(WRAP:%=-Wl,--wrap=%) -o $@ $(OBJS) -lm

clean:
	rm -rf $(B)

.PHONY: all check clean
