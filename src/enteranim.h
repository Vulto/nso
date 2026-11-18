/* enteranim.h - timeline of the "enter a system" transition (A on a system
 * tile). Pure C, time-based (seconds since A was pressed), unit-tested in
 * enteranim_test.c. Profile measured on the reference capture (59.7 fps,
 * frames 106..203, A pressed at 106):
 *   106-134  the tile's art lifts off as a fading ghost while a (17,17,17)
 *            layer rises over the menu, slow then fast (reaches 1 at 134)
 *   134-176  full-screen (17,17,17); logos fade in 135-143 and rotate in
 *   176-203  linear crossfade to the system's own screen; logos out 176-184 */
#ifndef NSO_ENTERANIM_H
#define NSO_ENTERANIM_H

#define ENTER_REF_FPS 59.7f
#define ENTER_COVER_R 17
#define ENTER_COVER_G 17
#define ENTER_COVER_B 17
#define ENTER_LOGO_TILT_DEG 12.0f   /* rotate-in start angle (and rotate-out end angle) */

/* Seconds from A to the end of the crossfade (frame 203). */
float EnterAnim_Total(void);
/* Opacity of the (17,17,17) layer over the main menu / tiles: 0 until frame 123. */
float EnterAnim_Cover(float t);
/* Ghost of the pressed tile: 0..1 progress over frames 106-134 and its opacity. */
float EnterAnim_GhostProgress(float t);
float EnterAnim_GhostAlpha(float t);
/* White logos' opacity (fade in 135-143, out 176-184) and rotation in degrees. */
float EnterAnim_LogoAlpha(float t);
float EnterAnim_LogoAngle(float t);
/* Opacity of the destination screen over the (17,17,17) layer: 0 until 176, 1 at 203. */
float EnterAnim_Cross(float t);

#endif /* NSO_ENTERANIM_H */

#ifdef ENTERANIM_IMPLEMENTATION
#ifndef ENTERANIM_IMPLEMENTATION_ONCE
#define ENTERANIM_IMPLEMENTATION_ONCE

static float enter_frame(float t) { return 106.0f + t * ENTER_REF_FPS; }

static float enter_clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* Linear ramp between frames a and b. */
static float enter_ramp(float f, float a, float b) { return enter_clamp01((f - a) / (b - a)); }

float EnterAnim_Total(void) { return (203.0f - 106.0f) / ENTER_REF_FPS; }

float EnterAnim_Cover(float t)
{
    static const float pts[][2] = { { 123, 0.00f }, { 124, 0.03f }, { 125, 0.12f }, { 128, 0.22f },
                                    { 130, 0.33f }, { 132, 0.50f }, { 133, 0.62f }, { 134, 1.00f } };
    const int n = (int)(sizeof(pts) / sizeof(pts[0]));
    float f = enter_frame(t);
    if (f <= pts[0][0]) return 0.0f;
    for (int i = 1; i < n; i++) {
        if (f <= pts[i][0])
            return pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * (f - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
    }
    return 1.0f;
}

float EnterAnim_GhostProgress(float t)
{
    float u = enter_ramp(enter_frame(t), 106.0f, 134.0f);
    return 1.0f - (1.0f - u) * (1.0f - u);   /* ease-out: lifts quickly, then drifts */
}

float EnterAnim_GhostAlpha(float t)
{
    float f = enter_frame(t);
    float u = enter_ramp(f, 106.0f, 134.0f);
    return 0.5f * enter_ramp(f, 106.0f, 108.0f) * (1.0f - u);
}

float EnterAnim_LogoAlpha(float t)
{
    float f = enter_frame(t);
    return enter_ramp(f, 135.0f, 143.0f) * (1.0f - enter_ramp(f, 176.0f, 184.0f));
}

float EnterAnim_LogoAngle(float t)
{
    float f = enter_frame(t);
    float in = enter_ramp(f, 135.0f, 143.0f), out = enter_ramp(f, 176.0f, 184.0f);
    in = 1.0f - (1.0f - in) * (1.0f - in);
    return -ENTER_LOGO_TILT_DEG * (1.0f - in) + ENTER_LOGO_TILT_DEG * out;
}

float EnterAnim_Cross(float t) { return enter_ramp(enter_frame(t), 176.0f, 203.0f); }

#endif
#endif
