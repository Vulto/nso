/* hideanim_test.c - hide-a-game flip and gap-close curves. */
#include <math.h>
#include <stdio.h>
#define HIDEANIM_IMPLEMENTATION
#include "hideanim.h"

static int fails = 0;

static void near(const char *what, float got, float want, float tol)
{
    if (fabsf(got - want) > tol) {
        fprintf(stderr, "FAIL %s: %.5f, want %.5f\n", what, (double)got, (double)want);
        fails++;
    }
}

int main(void)
{
    near("flip start", HideAnim_FlipScale(0.0f), 1.0f, 0.0001f);
    near("flip before", HideAnim_FlipScale(-1.0f), 1.0f, 0.0001f);
    near("flip mid", HideAnim_FlipScale(HIDEANIM_FLIP_SEC * 0.5f),
         cosf(3.14159265358979323846f * 0.25f), 0.0001f);
    near("flip end", HideAnim_FlipScale(HIDEANIM_FLIP_SEC), 0.0f, 0.0001f);
    near("flip after", HideAnim_FlipScale(HIDEANIM_FLIP_SEC + 1.0f), 0.0f, 0.0001f);
    if (HIDEANIM_FLIP_SEC < 0.3f || HIDEANIM_FLIP_SEC > 0.4f) {
        fprintf(stderr, "FAIL flip duration %.3f not in 0.3-0.4 s\n", (double)HIDEANIM_FLIP_SEC);
        fails++;
    }
    if (!HideAnim_CoverVisible(0.0f) || !HideAnim_CoverVisible(HIDEANIM_FLIP_SEC * 0.5f)) {
        fprintf(stderr, "FAIL cover should be visible during the flip\n");
        fails++;
    }
    if (HideAnim_CoverVisible(HIDEANIM_FLIP_SEC) || HideAnim_CoverVisible(HIDEANIM_FLIP_SEC + 0.01f)) {
        fprintf(stderr, "FAIL cover should vanish edge-on\n");
        fails++;
    }
    for (float t = 0.0f; t < HIDEANIM_FLIP_SEC; t += 0.01f) {
        if (HideAnim_FlipScale(t + 0.01f) > HideAnim_FlipScale(t) + 1e-6f) {
            fprintf(stderr, "FAIL flip scale not monotonic at %.2f\n", (double)t);
            fails++;
            break;
        }
    }

    near("close start", HideAnim_CloseEase(0.0f), 0.0f, 0.0001f);
    near("close mid", HideAnim_CloseEase(HIDEANIM_CLOSE_SEC * 0.5f), 0.5f, 0.0001f);
    near("close end", HideAnim_CloseEase(HIDEANIM_CLOSE_SEC), 1.0f, 0.0001f);
    near("close after", HideAnim_CloseEase(HIDEANIM_CLOSE_SEC + 1.0f), 1.0f, 0.0001f);
    near("lerp", HideAnim_Lerp(10.0f, 20.0f, 0.25f), 12.5f, 0.0001f);
    /* ease-in-out: first half slower than linear, second half catches up. */
    if (!(HideAnim_CloseEase(HIDEANIM_CLOSE_SEC * 0.25f) < 0.25f)
        || !(HideAnim_CloseEase(HIDEANIM_CLOSE_SEC * 0.75f) > 0.75f)) {
        fprintf(stderr, "FAIL close ease is not ease-in-out\n");
        fails++;
    }

    if (fails) return 1;
    puts("hideanim_test: ok");
    return 0;
}
