/* sfx_test.c - missing files and out-of-range ids are silent no-ops. */
#include <stdio.h>
#include <string.h>
#include "raylib.h"

static char base[1024];

const char *GetDataPath(const char *relativePath)
{
    static char full[2048];
    if (!relativePath || relativePath[0] == '\0') {
        snprintf(full, sizeof(full), "%s", base);
    } else {
        snprintf(full, sizeof(full), "%s/%s", base, relativePath);
    }
    return full;
}

#define SFX_IMPLEMENTATION
#include "sfx.h"

int main(void)
{
    snprintf(base, sizeof(base), "%s", "/tmp/nso-sfx-empty");
    MakeDirectory(base);
    Sfx_Play(SFX_CLICK);
    Sfx_Play((SfxId)-1);
    Sfx_Play(SFX_COUNT);
    Sfx_Init();
    for (int i = 0; i < SFX_COUNT; i++) Sfx_Play((SfxId)i);
    Sfx_Shutdown();
    /* a sound is audible until its whole length plus the output latency has passed, even after the
     * mixer reports it done (the device still holds the tail) */
    if (!Sfx_StillAudible(1, 0.0, 2.147) || !Sfx_StillAudible(1, 9.0, 2.147)) { fprintf(stderr, "FAIL mixer playing\n"); return 1; }
    if (!Sfx_StillAudible(0, 2.147, 2.147) || !Sfx_StillAudible(0, 2.147 + SFX_OUTPUT_LATENCY_SEC - 0.01, 2.147)) { fprintf(stderr, "FAIL tail cut\n"); return 1; }
    if (Sfx_StillAudible(0, 2.147 + SFX_OUTPUT_LATENCY_SEC + 0.001, 2.147)) { fprintf(stderr, "FAIL never ends\n"); return 1; }
    if (Sfx_Playing(SFX_COIN) || Sfx_Playing((SfxId)-1)) { fprintf(stderr, "FAIL playing without file\n"); return 1; }
    puts("sfx_test: ok");
    return 0;
}
