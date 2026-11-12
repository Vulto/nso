CC ?= cc
CFLAGS := -std=c23 -Wall -Wextra -O2 -Isrc
SRC := src/main.c
HDRS := src/nso.h src/launch.h src/scrape.h src/systems/nes.h src/systems/snes.h \
	src/systems/n64.h src/systems/gamecube.h src/savestate.h src/repos.h src/lang.h src/order.h \
	src/embed.h embed/nsomod.h

ifeq ($(OS),Windows_NT)
WINCC := $(CC)
else
WINCC := x86_64-w64-mingw32-gcc
endif
# `make windows CC=...` overrides the cross compiler.
ifeq ($(origin CC),command line)
WINCC := $(CC)
endif

.PHONY: all linux windows embed test clean

all: linux

linux: nso

nso: $(SRC) $(HDRS)
	$(CC) $(CFLAGS) -o nso $(SRC) -lraylib -lm -ldl

# In-process modules: SMB Vanilla (repos/nes/smbvanilla), smw (repos/snes/smw);
# never edits them
embed:
	$(MAKE) -C embed/smb
	$(MAKE) -C embed/smw

windows:
	$(WINCC) $(CFLAGS) -o nso.exe $(SRC) -lraylib -lopengl32 -lgdi32 -lwinmm -lpthread

test: src/scrape_test.c src/scrape.h src/savestate_test.c src/savestate.h \
		src/repos_test.c src/repos.h tools/repo_sync.sh src/lang_test.c src/lang.h \
		src/launch_test.c src/launch.h src/input_test.c src/nso.h \
		src/order_test.c src/order.h src/embed_test.c src/embed.h embed/nsomod.h
	$(CC) $(CFLAGS) -o scrape_test src/scrape_test.c
	./scrape_test
	$(CC) $(CFLAGS) -o savestate_test src/savestate_test.c
	./savestate_test
	$(CC) $(CFLAGS) -o repos_test src/repos_test.c
	./repos_test
	$(CC) $(CFLAGS) -o lang_test src/lang_test.c
	./lang_test
	$(CC) $(CFLAGS) -o launch_test src/launch_test.c
	./launch_test
	$(CC) $(CFLAGS) -o input_test src/input_test.c
	./input_test
	$(CC) $(CFLAGS) -o order_test src/order_test.c
	./order_test
	$(CC) $(CFLAGS) -o embed_test src/embed_test.c -lm
	./embed_test

clean:
	rm -f nso nso.exe scrape_test savestate_test repos_test lang_test launch_test input_test order_test embed_test
