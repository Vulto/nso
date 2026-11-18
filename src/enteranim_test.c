/* enteranim_test.c - the enter-a-system timeline against the reference profile. */
#include <math.h>
#include <stdio.h>
#define ENTERANIM_IMPLEMENTATION
#include "enteranim.h"

static int fails = 0;
static float at(int frame) { return (float)(frame - 106) / ENTER_REF_FPS * ENTER_STRETCH; }

static void near(const char *what, float got, float want, float tol)
{
    if (fabsf(got - want) > tol) { fprintf(stderr, "FAIL %s: %.3f, want %.3f\n", what, (double)got, (double)want); fails++; }
}

int main(void)
{
    near("cover 122", EnterAnim_Cover(at(122)), 0.0f, 0.001f);
    near("cover 125", EnterAnim_Cover(at(125)), 0.12f, 0.005f);
    near("cover 128", EnterAnim_Cover(at(128)), 0.22f, 0.005f);
    near("cover 130", EnterAnim_Cover(at(130)), 0.33f, 0.005f);
    near("cover 132", EnterAnim_Cover(at(132)), 0.50f, 0.005f);
    near("cover 133", EnterAnim_Cover(at(133)), 0.62f, 0.005f);
    near("cover 134", EnterAnim_Cover(at(134)), 1.0f, 0.001f);
    near("cover 150", EnterAnim_Cover(at(150)), 1.0f, 0.001f);
    near("logo before", EnterAnim_LogoAlpha(at(134)), 0.0f, 0.001f);
    near("logo 143", EnterAnim_LogoAlpha(at(143)), 1.0f, 0.001f);
    near("logo 175", EnterAnim_LogoAlpha(at(175)), 1.0f, 0.001f);
    near("logo 184", EnterAnim_LogoAlpha(at(184)), 0.0f, 0.001f);
    near("logo angle in", EnterAnim_LogoAngle(at(135)), -ENTER_FLIP_DEG, 0.01f);
    near("logo angle hold", EnterAnim_LogoAngle(at(160)), 0.0f, 0.01f);
    near("cross 175", EnterAnim_Cross(at(175)), 0.0f, 0.001f);
    near("cross 202", EnterAnim_Cross(at(202)), 26.0f / 27.0f, 0.001f);
    near("cross 203", EnterAnim_Cross(at(203)), 1.0f, 0.001f);
    near("ghost start", EnterAnim_GhostAlpha(at(106)), 0.0f, 0.001f);
    near("ghost end", EnterAnim_GhostAlpha(at(134)), 0.0f, 0.001f);
    near("total", EnterAnim_Total(), 97.0f / ENTER_REF_FPS + 0.020f, 0.0005f);
    near("letters start edge-on", EnterAnim_LetterAngle(at(135), 0, 7), -ENTER_FLIP_DEG, 0.01f);
    near("last letter later", EnterAnim_LetterAngle(at(136), 6, 7), -ENTER_FLIP_DEG, 0.01f);
    if (!(EnterAnim_LetterAngle(at(137), 0, 7) > EnterAnim_LetterAngle(at(137), 6, 7))) { fprintf(stderr, "FAIL letters not staggered\n"); fails++; }
    for (int i = 0; i < 7; i++) {   /* readable word between 143 and 176, all letters */
        near("letter face-on 143", EnterAnim_LetterAngle(at(143), i, 7), 0.0f, 0.01f);
        near("letter face-on 175", EnterAnim_LetterAngle(at(175), i, 7), 0.0f, 0.01f);
        near("letter out 184", EnterAnim_LetterAngle(at(184), i, 7), ENTER_FLIP_DEG, 0.01f);
    }
    for (int f = 106; f < 203; f++) {   /* never goes backwards */
        if (EnterAnim_Cover(at(f + 1)) + 1e-6f < EnterAnim_Cover(at(f))) { fprintf(stderr, "FAIL cover monotonic %d\n", f); fails++; }
        if (EnterAnim_Cross(at(f + 1)) + 1e-6f < EnterAnim_Cross(at(f))) { fprintf(stderr, "FAIL cross monotonic %d\n", f); fails++; }
    }
    if (fails) return 1;
    puts("enteranim_test: ok");
    return 0;
}
