/* sfx.h - optional UI sounds from assets/sounds/. Missing files are skipped. */
#ifndef NSO_SFX_H
#define NSO_SFX_H

#include "raylib.h"

typedef enum {
    SFX_NAVIGATE = 0,
    SFX_CLICK,
    SFX_PLUCK,
    SFX_COIN,
    SFX_AUDIO,
    SFX_COUNT
} SfxId;

void Sfx_Init(void);
void Sfx_Shutdown(void);
void Sfx_Play(SfxId id);
int  Sfx_Has(SfxId id);       /* 1 when the sound is loaded (audio device and file present) */
int  Sfx_Playing(SfxId id);   /* 1 until the whole sound has actually been heard (see Sfx_StillAudible) */

/* raylib clears "playing" the moment the mixer has handed the LAST frame to the output device, but
 * the device still holds that audio (period buffers + backend latency): the sound is heard for a
 * while after IsSoundPlaying() turns false. So a sound counts as finished only when the mixer is done
 * AND its real length (frames / sample rate) plus the output latency margin has elapsed since the
 * moment it was started. */
#define SFX_OUTPUT_LATENCY_SEC 0.25
int  Sfx_StillAudible(int mixerPlaying, double elapsedSec, double lengthSec);

#endif /* NSO_SFX_H */

#ifdef SFX_IMPLEMENTATION
#ifndef SFX_IMPLEMENTATION_ONCE
#define SFX_IMPLEMENTATION_ONCE

const char *GetDataPath(const char *relativePath);

static Sound sfxSounds[SFX_COUNT];
static bool sfxOwnDevice;
static double sfxStartedAt[SFX_COUNT];   /* GetTime() of the last Sfx_Play, 0 = never */

void Sfx_Init(void)
{
    static const char *const files[SFX_COUNT] = {
        [SFX_NAVIGATE] = "assets/sounds/navigate.ogg",
        [SFX_CLICK]    = "assets/sounds/click.ogg",
        [SFX_PLUCK]    = "assets/sounds/pluck.ogg",
        [SFX_COIN]     = "assets/sounds/coin.ogg",
        [SFX_AUDIO]    = "assets/sounds/audio.ogg",
    };

    if (!IsAudioDeviceReady()) {
        InitAudioDevice();
        sfxOwnDevice = IsAudioDeviceReady();
    }
    if (!IsAudioDeviceReady()) return;

    for (int i = 0; i < SFX_COUNT; i++) {
        const char *path = GetDataPath(files[i]);
        if (!FileExists(path)) continue;
        Sound s = LoadSound(path);
        if (IsSoundValid(s)) sfxSounds[i] = s;
    }
}

void Sfx_Shutdown(void)
{
    for (int i = 0; i < SFX_COUNT; i++) {
        if (IsSoundValid(sfxSounds[i])) UnloadSound(sfxSounds[i]);
        sfxSounds[i] = (Sound){ 0 };
    }
    if (sfxOwnDevice) {
        CloseAudioDevice();
        sfxOwnDevice = false;
    }
}

void Sfx_Play(SfxId id)
{
    if ((int)id < 0 || id >= SFX_COUNT) return;
    if (!IsSoundValid(sfxSounds[id])) return;
    PlaySound(sfxSounds[id]);
    sfxStartedAt[id] = GetTime() + 1e-9;   /* never exactly 0 */
}

int Sfx_Has(SfxId id)
{
    return (int)id >= 0 && id < SFX_COUNT && IsSoundValid(sfxSounds[id]);
}

int Sfx_StillAudible(int mixerPlaying, double elapsedSec, double lengthSec)
{
    return mixerPlaying || elapsedSec < lengthSec + SFX_OUTPUT_LATENCY_SEC;
}

int Sfx_Playing(SfxId id)
{
    if (!Sfx_Has(id) || sfxStartedAt[id] == 0.0) return 0;
    Sound s = sfxSounds[id];
    double length = s.stream.sampleRate ? (double)s.frameCount / (double)s.stream.sampleRate : 0.0;
    return Sfx_StillAudible(IsSoundPlaying(s), GetTime() - sfxStartedAt[id], length);
}

#endif /* SFX_IMPLEMENTATION_ONCE */
#endif /* SFX_IMPLEMENTATION */
