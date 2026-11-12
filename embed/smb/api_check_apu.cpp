// api_check_apu.cpp - compile-time check of the Nes_Snd_Emu calls
// smbmod_audio.cpp makes (submodule as of smbvanilla 43777eb). Never linked.
#include "nes_apu/Nes_Apu.h"
#include <cstdint>
#include <functional>
#include <type_traits>

#define SAME(what, ...) static_assert(std::is_same_v<__VA_ARGS__>, \
    "Nes_Snd_Emu API changed (pinned with smbvanilla 43777eb): " what)

using apu_time = Nes_Apu::nes_time_t;
using WriteReg = void (Nes_Apu::*)(apu_time, uint16_t, uint8_t);
using ApuEnd   = void (Nes_Apu::*)(apu_time);
using SetOut   = void (Nes_Apu::*)(Blip_Buffer *);
using Read     = int (Blip_Buffer::*)(blip_sample_t *, int, bool);
using BufEnd   = void (Blip_Buffer::*)(blip_time_t);
using Clock    = void (Blip_Buffer::*)(int);

// Each member must resolve to exactly this overload (else: compile error here).
[[maybe_unused]] static constexpr WriteReg write_register = &Nes_Apu::write_register;
[[maybe_unused]] static constexpr ApuEnd   apu_end_frame  = &Nes_Apu::end_frame;
[[maybe_unused]] static constexpr SetOut   set_output     = &Nes_Apu::set_output;
[[maybe_unused]] static constexpr Read     read_samples   = &Blip_Buffer::read_samples;
[[maybe_unused]] static constexpr BufEnd   buf_end_frame  = &Blip_Buffer::end_frame;
[[maybe_unused]] static constexpr Clock    clock_rate     = &Blip_Buffer::clock_rate;

SAME("blip_sample_t", blip_sample_t, std::int16_t);
SAME("Nes_Apu::dmc_reader", decltype(Nes_Apu::dmc_reader), std::function<int(int)>);
