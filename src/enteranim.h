/* enteranim.h - timeline of the "enter a system" transition (A on a system
 * tile). Pure C, time-based (seconds since A was pressed), unit-tested in
 * enteranim_test.c. Profile measured on the reference capture (59.7 fps,
 * frames 106..203, A pressed at 106):
 *   106-134  the tile's art lifts off as a fading ghost while a (17,17,17)
 *            layer rises over the menu, slow then fast (reaches 1 at 134)
 *   134-176  full-screen (17,17,17); logos fade in 135-143 ("PC Ports" flips in)
 *   176-203  linear crossfade to the system's own screen; logos out 176-184 */
#ifndef NSO_ENTERANIM_H
#define NSO_ENTERANIM_H

#define ENTER_REF_FPS 59.7f
/* The lead-in (106-123) and the hold (134-176) are the reference's frames plus 20 ms in total
 * spread proportionally; the two gray-layer fades are fixed at 300 ms each: the dim
 * (123-134, layer gains alpha) and the crossfade (176-203, layer loses alpha). Every curve keeps
 * its shape, only its time is mapped. */
#define ENTER_EXTRA_SEC 0.020f
#define ENTER_STRETCH ((97.0f / ENTER_REF_FPS + ENTER_EXTRA_SEC) / (97.0f / ENTER_REF_FPS))
#define ENTER_FADE_SEC 0.300f   /* the dim, the crossfade, and both halves of the exit */
#define ENTER_COVER_R 17
#define ENTER_COVER_G 17
#define ENTER_COVER_B 17
#define ENTER_FLIP_DEG 90.0f   /* "PC Ports" flips about its horizontal axis: edge-on at -90, face-on at 0, edge-on at +90 */

/* Seconds from A to the end of the crossfade (frame 203). */
float EnterAnim_Total(void);
/* Seconds since A at which reference frame f is shown (inverse of the timeline's time map). */
float EnterAnim_TimeOfFrame(float f);
/* Opacity of the (17,17,17) layer over the main menu / tiles: 0 until frame 123. */
float EnterAnim_Cover(float t);
/* Ghost of the pressed tile: 0..1 progress over frames 106-134 and its opacity. */
float EnterAnim_GhostProgress(float t);
float EnterAnim_GhostAlpha(float t);
/* White logos' opacity (fade in 135-143, out 176-184); the angle (degrees) is only for "PC Ports"
 * (card flip about the X axis); the Nintendo wordmark never rotates. */
float EnterAnim_LogoAlpha(float t);
float EnterAnim_LogoAngle(float t);
/* Same flip for letter i of n of "PC Ports": each letter is a little out of phase with the
 * others (flip-in windows start staggered inside 135-143, flip-out inside 176-184), and all are
 * face-on (0 degrees, the word readable) from frame 143 to 176. */
float EnterAnim_LetterAngle(float t, int i, int n);
/* Opacity of the destination screen over the (17,17,17) layer: 0 until 176, 1 at 203. */
float EnterAnim_Cross(float t);

/* Leaving a system's menu back to the main menu (no logos): the (17,17,17) layer gains alpha for
 * ENTER_FADE_SEC with the dim's curve, the main menu is drawn behind it the instant it is opaque
 * (EnterAnim_ExitSwitched), then it loses alpha for ENTER_FADE_SEC along the mirrored curve. */
float EnterAnim_ExitTotal(void);
float EnterAnim_ExitCover(float t);
int EnterAnim_ExitSwitched(float t);

/* When the giant full-screen system image may slide left to reveal the covers: only when BOTH the
 * system's sound has finished AND every cover is loaded. A system with a select jingle (hasSound)
 * waits for the whole jingle to end; one without stays at least ENTER_MIN_SHOW_SEC, counted from the
 * moment the image is 100% shown (end of the enter crossfade). Whatever is pressed meanwhile waits. */
#define ENTER_MIN_SHOW_SEC 1.0f
int EnterAnim_SlideReady(int coversLoaded, float shownSec, int hasSound, int soundPlaying);

#endif /* NSO_ENTERANIM_H */

#ifdef ENTERANIM_IMPLEMENTATION
#ifndef ENTERANIM_IMPLEMENTATION_ONCE
#define ENTERANIM_IMPLEMENTATION_ONCE

/* Phase boundaries in reference frames and their durations in seconds. */
static const float enter_pf[5] = { 106.0f, 123.0f, 134.0f, 176.0f, 203.0f };
static float enter_pd(int k)
{
    if (k == 1 || k == 3) return ENTER_FADE_SEC;
    return (enter_pf[k + 1] - enter_pf[k]) / ENTER_REF_FPS * ENTER_STRETCH;
}

static float enter_frame(float t)
{
    for (int k = 0; k < 4; k++) {
        float d = enter_pd(k);
        if (t < d || k == 3) return enter_pf[k] + (t / d) * (enter_pf[k + 1] - enter_pf[k]);
        t -= d;
    }
    return enter_pf[4];
}

float EnterAnim_TimeOfFrame(float f)
{
    float t = 0.0f;
    for (int k = 0; k < 4; k++) {
        if (f <= enter_pf[k + 1]) return t + (f - enter_pf[k]) / (enter_pf[k + 1] - enter_pf[k]) * enter_pd(k);
        t += enter_pd(k);
    }
    return t;
}

static float enter_clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }

/* Linear ramp between frames a and b. */
static float enter_ramp(float f, float a, float b) { return enter_clamp01((f - a) / (b - a)); }

float EnterAnim_Total(void) { return enter_pd(0) + enter_pd(1) + enter_pd(2) + enter_pd(3); }

/* The dim's curve over reference frames 123..134, as a function of the frame. */
static float enter_dim_frame(float f)
{
    static const float pts[][2] = { { 123, 0.00f }, { 124, 0.03f }, { 125, 0.12f }, { 128, 0.22f },
                                    { 130, 0.33f }, { 132, 0.50f }, { 133, 0.62f }, { 134, 1.00f } };
    const int n = (int)(sizeof(pts) / sizeof(pts[0]));
    if (f <= pts[0][0]) return 0.0f;
    for (int i = 1; i < n; i++) {
        if (f <= pts[i][0])
            return pts[i - 1][1] + (pts[i][1] - pts[i - 1][1]) * (f - pts[i - 1][0]) / (pts[i][0] - pts[i - 1][0]);
    }
    return 1.0f;
}

float EnterAnim_Cover(float t) { return enter_dim_frame(enter_frame(t)); }

float EnterAnim_ExitTotal(void) { return 2.0f * ENTER_FADE_SEC; }

float EnterAnim_ExitCover(float t)
{
    float u = t / ENTER_FADE_SEC;
    if (u <= 0.0f) return 0.0f;
    if (u < 1.0f) return enter_dim_frame(123.0f + u * 11.0f);
    u -= 1.0f;
    if (u >= 1.0f) return 0.0f;
    return enter_dim_frame(123.0f + (1.0f - u) * 11.0f);
}

int EnterAnim_ExitSwitched(float t) { return t >= ENTER_FADE_SEC; }

int EnterAnim_SlideReady(int coversLoaded, float shownSec, int hasSound, int soundPlaying)
{
    if (!coversLoaded) return 0;
    if (hasSound) return !soundPlaying;
    return shownSec >= ENTER_MIN_SHOW_SEC;
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
    return -ENTER_FLIP_DEG * (1.0f - in) + ENTER_FLIP_DEG * out;
}

float EnterAnim_LetterAngle(float t, int i, int n)
{
    float f = enter_frame(t);
    float lag = n > 1 ? 3.0f * (float)i / (float)(n - 1) : 0.0f;   /* 0..3 frames, letter by letter */
    float in = enter_ramp(f, 135.0f + lag, 140.0f + lag);          /* 5-frame flip, done by 143 */
    float out = enter_ramp(f, 176.0f + lag, 181.0f + lag);         /* done by 184 */
    in = 1.0f - (1.0f - in) * (1.0f - in);
    return -ENTER_FLIP_DEG * (1.0f - in) + ENTER_FLIP_DEG * out;
}

float EnterAnim_Cross(float t) { return enter_ramp(enter_frame(t), 176.0f, 203.0f); }

#endif
#endif
