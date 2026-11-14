# embed/mm3/CMakeLists.txt - libmm3mod.so from repos/nes/mm3-recomp, untouched
# (run by embed/mm3/Makefile). The port's own CMakeLists makes its SDL window
# and launcher; here NESRecomp's cycle runtime and the code it generates from
# the ROM become a shared library around mm3mod.c instead.
cmake_minimum_required(VERSION 3.20)
project(mm3mod C CXX)

set(MM3 "" CACHE PATH "Megaman3NESRecomp checkout")
set(MM3_ROM "" CACHE FILEPATH "Mega Man 3 (USA) ROM, never copied")

# The port's own ROM check, same arguments as its CMakeLists.txt.
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${MM3}/tools/verify_usa_rom.py"
    --rom "${MM3_ROM}" --crc 603AAA57 --mapper 4 --title "Mega Man III" RESULT_VARIABLE _rom_ok)
if(NOT _rom_ok EQUAL 0)
    message(FATAL_ERROR "The North American Mega Man 3 ROM is required")
endif()

# No recomp-ui and no SDL: NSO is the window.
set(NESRECOMP_HEADLESS ON)
include("${MM3}/nesrecomp/runner/cyc/project.cmake")
# The port's game description and coverage seeds, without its window extras
# (widescreen, co-op, Proto Man, netplay): the stock game.
nesrecomp_add_cycle_game(mm3core HEADLESS ROM "${MM3_ROM}"
    GAME_CONFIG "${MM3}/game.toml" SEED_FILE "${MM3}/cyc_seeds.txt")
set_target_properties(mm3core PROPERTIES EXCLUDE_FROM_ALL TRUE)

get_target_property(_src mm3core SOURCES)
get_target_property(_defs mm3core COMPILE_DEFINITIONS)
get_target_property(_incs mm3core INCLUDE_DIRECTORIES)
get_target_property(_libs mm3core LINK_LIBRARIES)
add_library(mm3mod SHARED ${_src} mm3mod.c)
target_compile_definitions(mm3mod PRIVATE ${_defs})
target_include_directories(mm3mod PRIVATE ${_incs})
if(_libs)
    target_link_libraries(mm3mod PRIVATE ${_libs})
endif()
set_target_properties(mm3mod PROPERTIES C_STANDARD 11 C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build")
# cyc_host.c is the stand-alone program; the runtime uses some of its
# functions, its main() is renamed so it is never an entry point.
set_property(SOURCE "${MM3}/nesrecomp/runner/cyc/cyc_host.c" APPEND
    PROPERTY COMPILE_DEFINITIONS main=mm3mod_unused_main)
