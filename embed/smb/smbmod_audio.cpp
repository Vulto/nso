// smbmod_audio.cpp - platform/audio_sdl.cpp's NesAudioImpl without SDL.
// Samples go to a FIFO; the host (NSO's raylib AudioStream) pulls them with
// smbmod_audio_pull() on its main thread.
#include "smbmod_audio.h"
#include "nes_apu/Nes_Apu.h"
#include <cstring>
#include <new>

namespace {
constexpr size_t FIFO_CAP = 48000 / 4;   // ~250 ms at 48 kHz; overrun drops the oldest

int null_dmc_reader(int) { return 0x55; }   // Nes_Apu::dmc_reader is std::function<int(int)>
}

struct SmbmodAudio {
    Blip_Buffer buf;
    Nes_Apu apu;
    blip_time_t clock = 0;
    int frame = 0;
    int16_t tmp[2048];
    int16_t fifo[FIFO_CAP];
    size_t head = 0, count = 0;
};

extern "C" SmbmodAudio *smbmod_audio_new(int32_t samplerate) {
    SmbmodAudio *a = new (std::nothrow) SmbmodAudio();
    if (!a) return nullptr;
    a->buf.clock_rate(1789773);
    if (a->buf.set_sample_rate(samplerate)) { delete a; return nullptr; }
    a->apu.dmc_reader = null_dmc_reader;            // as audio_sdl.cpp
    a->apu.set_output(&a->buf);
    return a;
}

extern "C" void smbmod_audio_free(SmbmodAudio *a) { delete a; }

extern "C" void smbmod_audio_write_register(SmbmodAudio *a, uint16_t addr, uint8_t data) {
    a->apu.write_register(a->clock, addr, data);
    a->clock += 4;                                   // as audio_sdl.cpp
}

extern "C" void smbmod_audio_end_frame(SmbmodAudio *a) {
    a->clock = 0;
    a->frame = (a->frame + 1) % 2;
    blip_time_t len = a->frame == 0 ? 29780 : 29781; // NTSC 29780.5 clocks per frame
    a->apu.end_frame(len);
    a->buf.end_frame(len);
    long n = a->buf.read_samples(a->tmp, sizeof a->tmp / sizeof a->tmp[0]);
    for (long i = 0; i < n; i++) {
        size_t tail = (a->head + a->count) % FIFO_CAP;
        a->fifo[tail] = a->tmp[i];
        if (a->count < FIFO_CAP) a->count++;
        else a->head = (a->head + 1) % FIFO_CAP;     // overrun: drop the oldest
    }
}

extern "C" size_t smbmod_audio_pull(SmbmodAudio *a, int16_t *out, size_t max) {
    size_t n = a->count < max ? a->count : max;
    for (size_t i = 0; i < n; i++) out[i] = a->fifo[(a->head + i) % FIFO_CAP];
    a->head = (a->head + n) % FIFO_CAP;
    a->count -= n;
    return n;
}
