/* nsomod.h - the one API every in-process game module exports to NSO
 * (src/embed.h dlopen()s embed/<game>/build/lib<game>mod.so and calls
 * nsomod_get_api()). Each module wraps one native port, untouched:
 * embed/smb (SMB Vanilla, NES), embed/mm3 (Megaman3NESRecomp, NES), embed/smw
 * (snesrev smw, SNES), embed/starfox (StarFoxSNESRecomp, SNES), embed/sm64
 * (sm64ex, N64).
 *  - no user-facing window or menu; NSO owns the controller overlay and pause UI;
 *  - the host calls frame() at fps and audio_pull() on its audio path;
 *  - API calls are serialized by NSO. A port may own worker threads internally,
 *    but shutdown() must stop/join work that can access the module. */
#ifndef NSOMOD_H
#define NSOMOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NSOMOD_API_VERSION 2

typedef struct NsomodInput {        /* SNES pad; NES games ignore x, y, l, r */
    bool up, down, left, right;
    bool a, b, x, y, l, r, select, start;
    float lx, ly, rx, ry;           /* N64: sticks, -1..1, y down; right = C buttons */
    bool z;                         /* N64 Z trigger */
} NsomodInput;

typedef struct NsomodConfig {
    const char *data_path;          /* the user's game data, read in place: a ROM
                                       file (SMB, MM3), the port's directory (SMW) or
                                       its save directory (SM64) */
    int sample_rate;                /* e.g. 48000; 0 = no audio */
} NsomodConfig;

typedef struct NsomodFrame {
    const uint8_t *rgb24;           /* width*height*3, RGB24, rows packed */
    int width, height;
} NsomodFrame;

typedef struct NsomodApi {
    int version;
    int fb_w, fb_h;                 /* frame size (fixed per module) */
    double fps;                     /* console frame rate */
    int channels;                   /* audio_pull: 1 = mono, 2 = interleaved stereo */
    bool (*init)(const NsomodConfig *cfg);
    NsomodFrame (*frame)(const NsomodInput *p1);
    /* Copies up to max sample frames (channels int16 each) made since the last call. */
    size_t (*audio_pull)(int16_t *out, size_t max);
    /* The port's own save-state files; NULL = not offered. */
    bool (*state_save)(const char *path);
    bool (*state_load)(const char *path);
    void (*shutdown)(void);
    /* Preserve a live session at Game Selection. NSO does not call shutdown
       on exit and invokes init again on re-entry; explicit Restart still
       calls shutdown/init. */
    bool keep_loaded;
} NsomodApi;

#define NSOMOD_ENTRY "nsomod_get_api"
typedef const NsomodApi *(*nsomod_get_api_fn)(void);
const NsomodApi *nsomod_get_api(void);

#ifdef __cplusplus
}
#endif
#endif
