# embed/mm3/Makefile - builds build/libmm3mod.so (Mega Man 3, mstan's
# Megaman3NESRecomp, in-process module loaded by src/embed.h) from
# repos/nes/mm3-recomp WITHOUT touching that checkout: CMakeLists.txt runs the
# port's own NESRecomp code generation on your ROM, everything goes to build/.
#
#   make          build/libmm3mod.so
#   make clean
#
# The ROM (Mega Man 3 USA, the port checks CRC 603AAA57) comes from
# config/nso.cfg nes.mm3_rom (NSO-relative or absolute), or MM3_ROM=/path.
# The port needs its submodules (its setup.sh: nesrecomp, recomp-ui).
NSO := ../..
MM3 ?= $(abspath $(NSO)/repos/nes/mm3-recomp)
MM3_ROM ?= $(shell r=$$(sed -n 's/^nes\.mm3_rom=//p' $(NSO)/config/nso.cfg 2>/dev/null | tail -n 1); \
	if [ -z "$$r" ] || [ "$${r#/}" != "$$r" ]; then printf '%s' "$$r"; \
	else printf '%s/%s' "$$(cd $(NSO) && pwd)" "$$r"; fi)
B := build

all:
	@[ -f "$(MM3)/nesrecomp/runner/cyc/project.cmake" ] || \
	 { echo "$(MM3): missing, or its submodules are not fetched (run its setup.sh)" >&2; exit 1; }
	@[ -f "$(MM3_ROM)" ] || \
	 { echo "Mega Man 3 ROM not found: set nes.mm3_rom in config/nso.cfg (got '$(MM3_ROM)')" >&2; exit 1; }
	@[ -f $(B)/cmake/CMakeCache.txt ] || \
	 cmake -S . -B $(B)/cmake -DCMAKE_BUILD_TYPE=Release -DMM3="$(MM3)" -DMM3_ROM="$(MM3_ROM)"
	cmake --build $(B)/cmake --target mm3mod -j"$$(nproc)"

clean:
	rm -rf $(B)

.PHONY: all clean
