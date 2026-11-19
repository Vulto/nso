# embed/starship/Makefile - builds build/libstarshipmod.so + build/libsf64game.so
# (Starship, Star Fox 64) from repos/n64/starship-src WITHOUT editing its
# source: the port's own CMake build (build-nso, -fPIC) compiles everything,
# then its objects are re-linked here in two libraries:
#   libsf64game.so   every decompiled C object (the game's variables: a suspend
#                    point is this library's .data/.bss)
#   libstarshipmod.so libultraship, the port glue and starshipmod.cpp
#
#   make          configure + build the port (build-nso), then both libraries
#   make clean
SS  ?= ../../repos/n64/starship-src
NSO := $(abspath ../..)
SSA := $(abspath $(SS))
B   := build
BN  := $(SSA)/build-nso
OD  := $(BN)/CMakeFiles/Starship.dir
CXX ?= c++
DEPS := $(BN)/_deps
CMAKE_ARGS := -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=OFF \
  -DCMAKE_C_COMPILER_LAUNCHER=$(abspath addpic.sh) -DCMAKE_CXX_COMPILER_LAUNCHER=$(abspath addpic.sh) -DNSO_BUILD=ON -DBUILD_TESTING=OFF \
  -DFETCHCONTENT_SOURCE_DIR_DR_LIBS=$(SSA)/build-cmake/_deps/dr_libs-src \
  -DFETCHCONTENT_SOURCE_DIR_IMGUI=$(SSA)/build-cmake/_deps/imgui-src \
  -DFETCHCONTENT_SOURCE_DIR_PRISM=$(SSA)/build-cmake/_deps/prism-src \
  -DFETCHCONTENT_SOURCE_DIR_THREADPOOL=$(SSA)/build-cmake/_deps/threadpool-src \
  -DFETCHCONTENT_SOURCE_DIR_TINYXML2=$(SSA)/build-cmake/_deps/tinyxml2-src \
  -DFETCHCONTENT_SOURCE_DIR_YAML-CPP=$(SSA)/build-cmake/_deps/yaml-cpp-src

.PHONY: all port clean
all: port
	@$(MAKE) --no-print-directory $(B)/libstarshipmod.so

port:
	@test -f "$(SS)/CMakeLists.txt" || { echo "starship: missing $(SS)"; exit 1; }
	@test -f $(BN)/build.ninja || cmake -S $(SSA) -B $(BN) $(CMAKE_ARGS)
	ninja -C $(BN) Starship

# The port's own compile flags for its C++ files, minus the per-file parts.
FLAGS = $(shell ninja -C $(BN) -t commands Starship | grep 'src/port/Game.cpp' | head -1 \
        | sed 's/^[^ ]* //; s/ -MD .*//; s/ -o [^ ]*//; s/ -c [^ ]*//')
# The port's link line, minus its objects and output: libraries and system flags.
LIBS = $(shell ninja -C $(BN) -t commands Starship | tail -1 | sed 's/ && cd .*//' | tr ' ' '\n' \
        | grep -E '^(/|_deps|tools|-l|libultraship)' | grep -v 'CMakeFiles/Starship.dir\|/usr/bin/c++' | sed 's#^\(_deps\|tools\|libultraship\)#$(BN)/\1#' | tr '\n' ' ')

GAME_O = $(shell find $(OD) -name '*.c.o')
PORT_O = $(shell find $(OD) -name '*.cpp.o')

$(B)/starshipmod.o: starshipmod.cpp ../nsomod.h
	@mkdir -p $(B)
	$(CXX) $(FLAGS) -I$(NSO) -fPIC -c $< -o $@

$(B)/libsf64game.so: $(GAME_O)
	@mkdir -p $(B)
	$(CXX) -shared -o $@ $^ -Wl,-soname,libsf64game.so -Wl,-z,relro -Wl,-Bsymbolic -Wl,--build-id=sha1 -lm \
	    -Wl,--wrap=GameEngine_Malloc -Wl,--wrap=__osMotorAccess

$(B)/libstarshipmod.so: $(B)/starshipmod.o $(PORT_O) $(B)/libsf64game.so
	$(CXX) -shared -o $@ $(B)/starshipmod.o $(PORT_O) -L$(B) -lsf64game -Wl,-rpath,'$$ORIGIN' \
	    -Wl,-Bsymbolic -Wl,--build-id=sha1 -pthread $(LIBS) -lGL -lX11 -lz

clean:
	rm -rf $(B)
