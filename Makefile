CC ?= cc
CFLAGS := -std=c23 -Wall -Wextra -Isrc -Wextra -Werror
SRC := src/main.c src/menu_launcher.c
HDRS := src/nso.h src/launch.h src/scrape.h src/systems/nes.h src/systems/snes.h \
	src/systems/n64.h src/systems/gamecube.h src/savestate.h src/repos.h src/lang.h src/order.h \
	src/embed.h src/sfx.h src/menu_launcher.h embed/nsomod.h src/sort.h src/sortpanel.h
HDRS += src/gamesel.h src/gamesel_ui.h src/hide.h
HDRS += src/status.h src/statuspanel.h

ifeq ($(OS),Windows_NT)
WINCC := $(CC)
else
WINCC := x86_64-w64-mingw32-gcc
endif
# `make windows CC=...` overrides the cross compiler.
ifeq ($(origin CC),command line)
WINCC := $(CC)
endif

.PHONY: all linux windows embed check test clean

all: linux

linux: nso

nso: $(SRC) $(HDRS)
	$(CC) $(CFLAGS) -pthread -o nso $(SRC) -lraylib -lm -ldl -lX11 -lXtst

# In-process modules: SMB Vanilla (repos/nes/smbvanilla), Mega Man 3
# (repos/nes/mm3-recomp), SMW, Star Fox SNES recomp, sm64ex, and AeroGauge
# (repos/n64/aerogauge-recomp, which carries its NSO adapter), and Star Fox 64
# (Starship, embed/starship).
embed:
	$(MAKE) -C embed/smb
	$(MAKE) -C embed/mm3
	$(MAKE) -C embed/smw
	cmake -S repos/snes/starfox-recomp -B repos/snes/starfox-recomp/build -DCMAKE_BUILD_TYPE=Release
	cmake --build repos/snes/starfox-recomp/build --target StarFoxNSOMod --parallel
	$(MAKE) -C embed/sm64
	$(MAKE) -C embed/starship
	cmake -S repos/n64/aerogauge-recomp -B repos/n64/aerogauge-recomp/build -DCMAKE_BUILD_TYPE=Release
	cmake --build repos/n64/aerogauge-recomp/build --target aerogauge_nso --parallel

# Build SoH from source so Zelda's NSO save-state bridge is present.
soh-nso:
	cmake -S repos/n64/soh-src -B repos/n64/soh-src/build-cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
	cmake --build repos/n64/soh-src/build-cmake --target soh GenerateSohOtr --parallel 4
	cmake -E copy_if_different repos/n64/soh-src/build-cmake/soh/soh.o2r repos/n64/soh/soh.o2r

windows:
	$(WINCC) $(CFLAGS) -o nso.exe $(SRC) -lraylib -lopengl32 -lgdi32 -lwinmm -lpthread

# Unit tests only: headless, audio muted (no sound device is opened).
check: export SDL_AUDIODRIVER = dummy
check: export PULSE_SERVER = none
check: export ALSA_CONFIG_PATH = /dev/null
check: src/scrape_test.c src/scrape.h src/savestate_test.c src/savestate.h \
		src/repos_test.c src/repos.h tools/repo_sync.sh src/lang_test.c src/lang.h \
		src/launch_test.c src/launch.h src/input_test.c src/nso.h \
		src/order_test.c src/order.h src/embed_test.c src/embed.h embed/nsomod.h \
		src/sfx_test.c src/sfx.h src/sort_test.c src/sort.h src/catalog_test.c src/systems/nes.h \
		src/systems/snes.h src/systems/n64.h src/systems/gamecube.h \
		embed/sm64/reloc_test.c embed/sm64/reloc.h
	$(CC) $(CFLAGS) -o scrape_test src/scrape_test.c
	./scrape_test
	$(CC) $(CFLAGS) -o savestate_test src/savestate_test.c
	./savestate_test
	$(CC) $(CFLAGS) -o repos_test src/repos_test.c
	./repos_test
	$(CC) $(CFLAGS) -o lang_test src/lang_test.c
	./lang_test
	$(CC) $(CFLAGS) -o launch_test src/launch_test.c -lm
	./launch_test
	$(CC) $(CFLAGS) -o input_test src/input_test.c
	./input_test
	$(CC) $(CFLAGS) -o order_test src/order_test.c
	./order_test
	$(CC) $(CFLAGS) -o sort_test src/sort_test.c
	./sort_test
	$(CC) $(CFLAGS) -o catalog_test src/catalog_test.c
	./catalog_test
	$(CC) $(CFLAGS) -o gamesel_test src/gamesel_test.c
	./gamesel_test
	$(CC) $(CFLAGS) -o hide_test src/hide_test.c
	./hide_test
	$(CC) $(CFLAGS) -o status_test src/status_test.c
	./status_test
	$(CC) $(CFLAGS) -o embed_test src/embed_test.c -lm
	./embed_test
	$(CC) $(CFLAGS) -o sfx_test src/sfx_test.c -lraylib -lm
	./sfx_test
	$(CC) $(CFLAGS) -o sm64reloc_test embed/sm64/reloc_test.c
	./sm64reloc_test

check: src/gamesel_test.c src/gamesel.h tools/game_install.sh src/hide_test.c src/hide.h
check: src/status_test.c src/status.h

# Tests, then build and open NSO on DISPLAY=:1 (a copy already open there is closed first).
test: check nso
	@for p in $$(pgrep -x nso); do \
		if tr '\0' '\n' < /proc/$$p/environ 2>/dev/null | grep -qx 'DISPLAY=:1'; then kill $$p; fi; \
	done; sleep 1
	DISPLAY=:1 setsid nohup ./nso >/dev/null 2>&1 </dev/null &

clean:
	rm -f nso nso.exe scrape_test savestate_test repos_test lang_test launch_test input_test order_test embed_test sfx_test sm64reloc_test sort_test catalog_test gamesel_test hide_test status_test

.PHONY: embed soh-nso windows test clean
