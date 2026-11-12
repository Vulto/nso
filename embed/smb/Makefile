# embed/smb/Makefile - builds build/libsmbmod.so (SMB Vanilla in-process
# module, loaded by src/embed.h) from repos/nes/smbvanilla WITHOUT touching
# that checkout: its sources are compiled as they are, objects go to build/.
#
#   make          API check, then build/libsmbmod.so
#   make check    API check only
#   make clean
#
# Upstream pin: the adapter was written against SMB Vanilla 43777eb. After a
# `git pull` of the port, `make` re-runs the API check; a changed API stops
# the build with "smbvanilla API changed ..." naming the function or field.
SMBV ?= ../../repos/nes/smbvanilla
PIN  := 43777eb
SNDE := $(SMBV)/Nes_Snd_Emu
B    := build

CC  ?= cc
CXX ?= c++
PIC  := -fPIC -fvisibility=hidden
# smbcore needs wrapping signed arithmetic (its meson.build requires -fwrapv).
# No -DPRINT_WARNINGS_AND_ERRORS: the core's warning() compiles to nothing.
CORE := -std=gnu17 -O2 -fwrapv $(PIC) -I$(SMBV)/src
INC  := -I. -I$(SMBV)/src -I$(SMBV)/src/platform -I$(SNDE)

CORE_SRC := common.c common_sound.c area.c
APU_SRC  := Blip_Buffer.cpp Multi_Buffer.cpp Nes_Apu.cpp Nes_Oscs.cpp
NEEDED := $(addprefix $(SMBV)/src/smbcore/,$(CORE_SRC) smb1only.c smb2jonly.c smbcore.c api.h) \
          $(SMBV)/src/platform/render_raster.c $(SMBV)/src/platform/render_raster.h \
          $(SMBV)/src/platform/palette_builtin.h $(addprefix $(SNDE)/nes_apu/,$(APU_SRC) Nes_Apu.h)
MISSING := $(filter-out $(wildcard $(NEEDED)),$(NEEDED))

SMB1_O  := $(CORE_SRC:%.c=$(B)/smb1_%.o)  $(B)/smb1_smb1only.o
SMB2J_O := $(CORE_SRC:%.c=$(B)/smb2j_%.o) $(B)/smb2j_smb2jonly.o
APU_O   := $(APU_SRC:%.cpp=$(B)/apu_%.o)
OBJS := $(SMB1_O) $(SMB2J_O) $(B)/smbcore.o $(B)/render_raster.o $(APU_O) \
        $(B)/smbmod_audio.o $(B)/smbmod.o

all: $(B)/libsmbmod.so

$(B):
	mkdir -p $(B)

ifneq ($(MISSING),)
ifneq ($(MAKECMDGOALS),clean)
$(error smbvanilla layout changed (pinned $(PIN)) or not downloaded: missing $(MISSING))
endif
endif
$(B)/api.ok: api_check.c api_check_apu.cpp $(NEEDED) | $(B)
	@head=$$(git -C $(SMBV) rev-parse --short=7 HEAD 2>/dev/null); \
	if [ -n "$$head" ] && [ "$$head" != "$(PIN)" ]; then \
		echo "note: smbvanilla is at $$head, adapter pinned to $(PIN); checking API" >&2; fi
	@$(CC) -std=c11 -fsyntax-only $(INC) api_check.c && \
	 $(CXX) -std=c++17 -fsyntax-only -I$(SNDE) api_check_apu.cpp || \
	 { echo "smbvanilla API changed (pinned $(PIN)): update embed/smb/smbmod.c, see the assertion above" >&2; exit 1; }
	@touch $@

check: $(B)/api.ok

# Same source compiled twice, as smbvanilla's meson does (smb1core / smb2jcore)
$(B)/smb1_%.o: $(SMBV)/src/smbcore/%.c $(B)/api.ok
	$(CC) $(CORE) -DSMB1_MODE -c $< -o $@
$(B)/smb2j_%.o: $(SMBV)/src/smbcore/%.c $(B)/api.ok
	$(CC) $(CORE) -DSMB2J_MODE -c $< -o $@
$(B)/smbcore.o: $(SMBV)/src/smbcore/smbcore.c $(B)/api.ok
	$(CC) $(CORE) -c $< -o $@
$(B)/render_raster.o: $(SMBV)/src/platform/render_raster.c $(B)/api.ok
	$(CC) -O2 $(PIC) $(INC) -c $< -o $@
$(B)/apu_%.o: $(SNDE)/nes_apu/%.cpp $(B)/api.ok
	$(CXX) -std=c++17 -O2 $(PIC) -I$(SNDE)/nes_apu -c $< -o $@
$(B)/smbmod_audio.o: smbmod_audio.cpp smbmod_audio.h $(B)/api.ok
	$(CXX) -std=c++17 -O2 $(PIC) $(INC) -c $< -o $@
$(B)/smbmod.o: smbmod.c smbmod.h smbmod_audio.h $(B)/api.ok
	$(CC) -std=c11 -O2 -Wall -Wextra $(PIC) $(INC) -c $< -o $@

$(B)/libsmbmod.so: $(OBJS)
	$(CXX) -shared -o $@ $(OBJS)

clean:
	rm -rf $(B)

.PHONY: all check clean
