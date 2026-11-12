/* smbmod_audio.h - Nes_Snd_Emu APU without SDL: samples go to a FIFO the
 * host pulls. Takes the place of platform/audio_sdl.cpp. */
#ifndef SMBMOD_AUDIO_H
#define SMBMOD_AUDIO_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct SmbmodAudio SmbmodAudio;
SmbmodAudio *smbmod_audio_new(int32_t samplerate);
void   smbmod_audio_free(SmbmodAudio *a);
void   smbmod_audio_write_register(SmbmodAudio *a, uint16_t addr, uint8_t data);
void   smbmod_audio_end_frame(SmbmodAudio *a);          /* makes ~samplerate/60 samples */
size_t smbmod_audio_pull(SmbmodAudio *a, int16_t *out, size_t max);
void   smbmod_audio_reset(SmbmodAudio *a);                /* state load: silence, drop queued samples */
#ifdef __cplusplus
}
#endif
#endif
