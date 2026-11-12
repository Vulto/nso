/* embed_test.c - embed.h's pure helpers: picture placement, frame pacing, menu easing. */
#include "embed.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static bool RectIs(Rectangle r, float x, float y, float w, float h)
{
    return r.x == x && r.y == y && r.width == w && r.height == h;
}

int main(void)
{
    /* 1920x1080: Pixel Perfect = 4x (1024x960), 4:3 and CRT = 1440x1080. */
    CHECK(RectIs(Embed_DestRect(EMBED_SCREEN_PIXEL, 1920, 1080), 448, 60, 1024, 960));
    CHECK(RectIs(Embed_DestRect(EMBED_SCREEN_43, 1920, 1080), 240, 0, 1440, 1080));
    CHECK(RectIs(Embed_DestRect(EMBED_SCREEN_CRT, 1920, 1080), 240, 0, 1440, 1080));
    /* Narrow / tiny windows still fit and never scale below 1x. */
    CHECK(RectIs(Embed_DestRect(EMBED_SCREEN_43, 800, 1080), 0, 240, 800, 600));
    CHECK(RectIs(Embed_DestRect(EMBED_SCREEN_PIXEL, 100, 100), -78, -70, 256, 240));

    /* 60 FPS ticks give ~60.0988 game frames per second, never more than 2
     * per tick, and a stall is dropped instead of fast-forwarded. */
    double acc = 0.0;
    int frames = 0;
    for (int i = 0; i < 600; i++) {
        int n = Embed_PaceFrames(&acc, 1.0 / 60.0);
        CHECK(n >= 0 && n <= 2);
        frames += n;
    }
    CHECK(frames >= 600 && frames <= 602);
    acc = 0.0;
    CHECK(Embed_PaceFrames(&acc, 5.0) == 2);
    CHECK(acc < 1.0 / SMBMOD_NES_FPS);
    acc = 0.0;
    CHECK(Embed_PaceFrames(&acc, 0.001) == 0);

    if (fails) return 1;
    /* menu ease: 0 -> 1, monotonic, clamped, ease-out (ahead of linear) */
    CHECK(Embed_MenuEase(0.0f) == 0.0f && Embed_MenuEase(1.0f) == 1.0f);
    CHECK(Embed_MenuEase(-1.0f) == 0.0f && Embed_MenuEase(2.0f) == 1.0f);
    CHECK(Embed_MenuEase(0.5f) > 0.5f && Embed_MenuEase(0.5f) < Embed_MenuEase(0.6f));
    CHECK(Embed_BackEase(0.0f) == 0.0f && Embed_BackEase(1.0f) == 1.0f);
    CHECK(Embed_BackEase(0.2f) > 0.5f && Embed_BackEase(0.2f) < Embed_BackEase(0.3f));
    puts("embed_test: ok");
    return 0;
}
