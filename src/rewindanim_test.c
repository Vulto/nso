/* rewindanim_test.c - the rewind screen's timeline against the reference measurements. */
#include <stdio.h>
#include "rewindanim.h"

static int fails = 0;
static void near(const char *what, float got, float want, float tol)
{
    if (fabsf(got - want) > tol) { fprintf(stderr, "FAIL %s: %.4f, want %.4f\n", what, (double)got, (double)want); fails++; }
}

int main(void)
{
    /* the table: reference frames, interpolated */
    near("scale 86", RewindAnim_LookAt(86).scale, 1.0f, 0.0001f);
    near("scale 89", RewindAnim_LookAt(89).scale, 767.0f / 875.0f, 0.001f);
    near("scale 92", RewindAnim_LookAt(92).scale, 655.0f / 875.0f, 0.001f);
    near("scale 100", RewindAnim_LookAt(100).scale, 657.0f / 875.0f, 0.001f);
    near("scale 106", RewindAnim_LookAt(106).scale, 733.0f / 875.0f, 0.001f);
    near("scale 111", RewindAnim_LookAt(111).scale, 1.0f, 0.0001f);
    near("scale 89.5", RewindAnim_LookAt(89.5f).scale, (767.0f + 711.0f) / 2.0f / 875.0f, 0.002f);
    near("panel top 92", RewindAnim_LookAt(92).panelTop, 514.0f / 720.0f, 0.001f);
    near("panel top 106", RewindAnim_LookAt(106).panelTop, 560.0f / 720.0f, 0.001f);
    near("panel alpha 91", RewindAnim_LookAt(91).panelAlpha, 0.85f, 0.001f);
    near("bar 95", RewindAnim_LookAt(95).bar, 0.20f, 0.001f);
    near("bar 102", RewindAnim_LookAt(102).bar, 1.0f, 0.001f);
    near("cap1 96", RewindAnim_LookAt(96).cap1, 0.30f, 0.001f);
    near("cap1 98", RewindAnim_LookAt(98).cap1, 0.0f, 0.001f);
    near("cap2 100", RewindAnim_LookAt(100).cap2, 0.82f, 0.001f);
    near("cap2 109", RewindAnim_LookAt(109).cap2, 0.0f, 0.001f);
    near("idle bar", RewindAnim_LookAt(0).bar, 1.0f, 0.001f);
    near("idle cap1", RewindAnim_LookAt(500).cap1, 1.0f, 0.001f);

    /* press and hold: reference frame 92 is 6 frames after 86, 103 is 17 */
    RewindAnim a;
    RewindAnim_Init(&a);
    near("idle f", a.f, 86.0f, 0.001f);
    if (RewindAnim_Active(&a)) { fprintf(stderr, "FAIL idle active\n"); fails++; }
    for (int i = 0; i < 6; i++) RewindAnim_Step(&a, true, 1.0f / REWIND_REF_FPS);
    near("pressed 6 frames", a.f, 92.0f, 0.01f);
    if (RewindAnim_RealWindowShows(&a)) { fprintf(stderr, "FAIL real window shown while shrunk\n"); fails++; }
    for (int i = 0; i < 100; i++) RewindAnim_Step(&a, true, 1.0f / 60.0f);
    near("hold", a.f, 103.0f, 0.001f);
    /* released from the hold: 103 -> 118 */
    for (int i = 0; i < 8; i++) RewindAnim_Step(&a, false, 1.0f / REWIND_REF_FPS);
    near("exit 8 frames", a.f, 111.0f, 0.01f);
    if (!RewindAnim_RealWindowShows(&a)) { fprintf(stderr, "FAIL real window hidden at full size\n"); fails++; }
    for (int i = 0; i < 20; i++) RewindAnim_Step(&a, false, 1.0f / REWIND_REF_FPS);
    if (RewindAnim_Active(&a)) { fprintf(stderr, "FAIL still active after exit\n"); fails++; }

    /* early release plays the enter backwards, back to idle */
    RewindAnim_Init(&a);
    for (int i = 0; i < 4; i++) RewindAnim_Step(&a, true, 1.0f / REWIND_REF_FPS);
    RewindAnim_Step(&a, false, 1.0f / REWIND_REF_FPS);
    near("early release", a.f, 89.0f, 0.01f);
    for (int i = 0; i < 10; i++) RewindAnim_Step(&a, false, 1.0f / REWIND_REF_FPS);
    if (RewindAnim_Active(&a)) { fprintf(stderr, "FAIL early release stuck\n"); fails++; }

    /* pressed again while leaving: back to the matching shrink, not a jump */
    RewindAnim_Init(&a);
    for (int i = 0; i < 40; i++) RewindAnim_Step(&a, true, 1.0f / REWIND_REF_FPS);
    for (int i = 0; i < 4; i++) RewindAnim_Step(&a, false, 1.0f / REWIND_REF_FPS);
    float before = RewindAnim_Look(&a).scale;
    RewindAnim_Step(&a, true, 0.0f);
    near("re-press scale", RewindAnim_Look(&a).scale, before, 0.01f);
    if (a.phase != 1) { fprintf(stderr, "FAIL re-press phase\n"); fails++; }

    /* strip */
    near("strip newest", RewindStrip_X(640, 176, 100, 96, 12), 640 - 176 * (1.0f + 4.0f / 12.0f), 0.01f);
    near("strip older", RewindStrip_X(640, 176, 100, 84, 12), 640 - 176 * 2.0f - 176 * 4.0f / 12.0f, 0.01f);
    near("strip alpha past", RewindStrip_Alpha(100, 96, 12), 1.0f, 0.001f);
    near("strip alpha future", RewindStrip_Alpha(100, 103, 12), 0.5f, 0.001f);
    near("strip alpha gone", RewindStrip_Alpha(100, 110, 12), 0.0f, 0.001f);

    if (fails) { fprintf(stderr, "%d failures\n", fails); return 1; }
    puts("rewindanim_test: ok");
    return 0;
}
