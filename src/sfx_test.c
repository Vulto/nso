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
    puts("sfx_test: ok");
    return 0;
}
