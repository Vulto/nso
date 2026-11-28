/* hideanim.h - hide-a-game motion on the grid. Pure C, time-based, unit-tested
 * in hideanim_test.c. The cover flips sideways like a card (horizontal scale
 * to 0 about its vertical centre) in HIDEANIM_FLIP_SEC, vanishes edge-on, then
 * the remaining tiles close the gap over HIDEANIM_CLOSE_SEC. */
#ifndef NSO_HIDEANIM_H
#define NSO_HIDEANIM_H

#define HIDEANIM_FLIP_SEC  0.35f   /* 0.3–0.4 s; 90° card flip, gone mid-flip */
#define HIDEANIM_CLOSE_SEC 0.30f

/* Horizontal scale of the hiding cover at time t into the flip (1 = face-on,
 * 0 = edge-on). Cosine of 0..90°, so the turn starts slow and finishes
 * edge-on at HIDEANIM_FLIP_SEC. t < 0 is face-on; t >= duration is 0. */
float HideAnim_FlipScale(float t);

/* The cover is drawn while the scale is still > 0; it disappears edge-on. */
int HideAnim_CoverVisible(float t);

/* 0..1 cubic ease-in-out for the gap-close, clamped to the close duration. */
float HideAnim_CloseEase(float t);

/* Linear interpolate a to b by u. */
float HideAnim_Lerp(float a, float b, float u);

#endif /* NSO_HIDEANIM_H */

#ifdef HIDEANIM_IMPLEMENTATION
#ifndef HIDEANIM_IMPLEMENTATION_ONCE
#define HIDEANIM_IMPLEMENTATION_ONCE

#include <math.h>

#ifndef HIDEANIM_PI
#define HIDEANIM_PI 3.14159265358979323846f
#endif

static float hideanim_clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

float HideAnim_FlipScale(float t)
{
    float u = t / HIDEANIM_FLIP_SEC;
    if (u <= 0.0f) return 1.0f;
    if (u >= 1.0f) return 0.0f;
    return cosf(u * (HIDEANIM_PI * 0.5f));
}

int HideAnim_CoverVisible(float t)
{
    return t < HIDEANIM_FLIP_SEC && HideAnim_FlipScale(t) > 0.0001f;
}

float HideAnim_CloseEase(float t)
{
    float u = hideanim_clamp01(t / HIDEANIM_CLOSE_SEC);
    if (u <= 0.5f) return 4.0f * u * u * u;
    {
        float x = -2.0f * u + 2.0f;
        return 1.0f - (x * x * x) * 0.5f;
    }
}

float HideAnim_Lerp(float a, float b, float u)
{
    return a + (b - a) * u;
}

#endif /* HIDEANIM_IMPLEMENTATION_ONCE */
#endif /* HIDEANIM_IMPLEMENTATION */
