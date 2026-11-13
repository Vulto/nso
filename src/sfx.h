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

#endif /* NSO_SFX_H */

#ifdef SFX_IMPLEMENTATION
#ifndef SFX_IMPLEMENTATION_ONCE
#define SFX_IMPLEMENTATION_ONCE

const char *GetDataPath(const char *relativePath);

static Sound sfxSounds[SFX_COUNT];
static bool sfxOwnDevice;

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
}

#endif /* SFX_IMPLEMENTATION_ONCE */
#endif /* SFX_IMPLEMENTATION */
