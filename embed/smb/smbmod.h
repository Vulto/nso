/* smbmod.h - in-process module API for the SMB Vanilla native port
 * (repos/nes/smbvanilla, a C decompilation: no emulation).
 *
 * libsmbmod.so replaces SMB Vanilla's SDL platform layer (window, keyboard,
 * audio device, timer) with buffers NSO consumes (src/embed.h):
 *  - no window, no SDL, no sleeping, no printing;
 *  - the host calls frame() once per NES frame (~60.0988 Hz);
 *  - one instance, one thread (smbcore keeps a global SMB_STATE pointer,
 *    smbcore/interface.h).
 * Shared by NSO (src/embed.h) and the module (embed/smb/smbmod.c). */
#ifndef SMBMOD_H
#define SMBMOD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMBMOD_API_VERSION 1
#define SMBMOD_FB_W 256
#define SMBMOD_FB_H 240
/* NTSC: 1789773 / 29780.5 (platform/main_sdl.c) */
#define SMBMOD_NES_FPS (1789773.0 / 29780.5)

typedef struct SmbmodInput {        /* = struct SMB_buttons (smbcore/api.h) */
    bool up, down, left, right;
    bool a, b, select, start;
} SmbmodInput;

typedef struct SmbmodConfig {
    const char *rom_path;           /* the user's own smb.nes (read only) */
    const uint8_t *palette_rgb;     /* 64*3 bytes; NULL = built-in palette */
    int sample_rate;                /* e.g. 48000; 0 = no audio */
    int start_world, start_level;   /* 0 = default (1-1) */
    uint8_t smb2j_games_beaten;     /* SMB2J only; persisted by the host */
} SmbmodConfig;

typedef struct SmbmodFrame {
    const uint8_t *rgb24;           /* SMBMOD_FB_W*SMBMOD_FB_H*3, RGB24, stride 256 px */
    int width, height;
} SmbmodFrame;

/* Function table: the .so exports this single symbol for dlsym(). */
typedef struct SmbmodApi {
    int  version;
    bool (*init)(const SmbmodConfig *cfg);           /* false = missing / invalid ROM */
    int  (*which_game)(void);                        /* 1 = SMB1, 2 = SMB2J */
    SmbmodFrame (*frame)(const SmbmodInput *p1, const SmbmodInput *p2);
    /* Copies up to max mono int16 samples produced since the last call. */
    size_t (*audio_pull)(int16_t *out, size_t max);
    /* Plain save state: RAM 0x800 + PPU RAM 0x4000 (as main_sdl.c dump_ram/ppuram). */
    size_t (*state_size)(void);
    bool (*state_save)(void *buf, size_t size);
    bool (*state_load)(const void *buf, size_t size);
    void (*shutdown)(void);
} SmbmodApi;

#define SMBMOD_ENTRY "smbmod_get_api"
typedef const SmbmodApi *(*smbmod_get_api_fn)(void);
const SmbmodApi *smbmod_get_api(void);

#ifdef __cplusplus
}
#endif
#endif
