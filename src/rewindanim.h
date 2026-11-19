/* rewindanim.h - the game-rewind screen (Mednafen NES/SNES) against the reference capture
 * (59.7 fps, frames 84..111, 1280x720). Pure C, unit-tested in rewindanim_test.c.
 * Measured on the reference, per frame f:
 *   86-92   rewind pressed: the game shrinks (ease-out, 100% -> 74.9% of its width, top
 *           anchored), a gray panel (66,66,66) rises from the bottom to y=514 (71.4% of the
 *           height) while fading in, the selected thumbnail (160x124, white border, 22 px under
 *           the panel top) fades in with it.
 *   93-97   the first caption ("ZL + ZR  Suspend Menu (Press and hold to rewind)") and the black
 *           bar under the game (36 px = 5%) fade out
 *   97-102  second caption ("ZL + ZR Suspend Menu   B Back   A OK") and the bar fade in; hold
 *   103-111 rewind released: panel, thumbnail and second caption fade out, the game returns to
 *           100% (ease-in); the reference's bar stays gone.
 * NSO keeps the bar with the first caption whenever "show controls in game" is on, so after
 * the exit they fade back in (frames 109-118, an addition: the reference never shows that). */
#ifndef NSO_REWINDANIM_H
#define NSO_REWINDANIM_H
#include <math.h>
#include <stdbool.h>

#define REWIND_REF_FPS 59.7f
#define REWIND_F0 86          /* first table frame (idle look) */
#define REWIND_FHOLD 103      /* the hold ends here; released: play on to the end */
#define REWIND_FLAST 118      /* last table frame (idle look again) */
#define REWIND_FSPLIT 97      /* released before this: the enter plays backwards */
#define REWIND_FGAME_FULL 111 /* the game is back at 100%: the real window can show again */
#define REWIND_PANEL_Y 0.7139f   /* panel top / height once risen (514/720) */
#define REWIND_BAR_H 0.05f       /* black bar / height (36/720) */
#define REWIND_THUMB_H 124.0f    /* selected thumbnail height at 720p */
#define REWIND_THUMB_DY 22.0f    /* thumbnail top under the panel top at 720p */
#define REWIND_GAP 16.0f         /* gap between thumbnails at 720p */
#define REWIND_PANEL_GRAY 66

typedef struct {
    float scale;        /* game width / its idle width */
    float panelTop;     /* panel top / screen height */
    float panelAlpha;
    float thumbAlpha;   /* selected thumbnail */
    float thumbScale;
    float bar;          /* black bar opacity */
    float cap1, cap2;   /* caption opacities */
} RewindLook;

enum { REWIND_N = REWIND_FLAST - REWIND_F0 + 1 };

static const RewindLook rewind_table[REWIND_N] = {
    /*      scale   top    pA    tA   tS    bar   cap1  cap2 */
    /* 86 */ {1.0000f, 0.889f, 0.00f, 0.00f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 87 */ {0.9817f, 0.850f, 0.32f, 0.00f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 88 */ {0.9360f, 0.797f, 0.41f, 0.25f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 89 */ {0.8766f, 0.769f, 0.51f, 0.45f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 90 */ {0.8126f, 0.742f, 0.65f, 0.75f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 91 */ {0.7669f, 0.721f, 0.85f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 92 */ {0.7486f, 0.7139f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f},
    /* 93 */ {0.7486f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.90f, 0.98f, 0.00f},
    /* 94 */ {0.7486f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.50f, 0.78f, 0.00f},
    /* 95 */ {0.7486f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.20f, 0.52f, 0.00f},
    /* 96 */ {0.7486f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.05f, 0.30f, 0.00f},
    /* 97 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f, 0.46f},
    /* 98 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.00f, 0.00f, 0.51f},
    /* 99 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.15f, 0.00f, 0.66f},
    /*100 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.55f, 0.00f, 0.82f},
    /*101 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 0.88f, 0.00f, 0.95f},
    /*102 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f, 1.00f},
    /*103 */ {0.7509f, 0.7139f, 1.00f, 1.00f, 1.00f, 1.00f, 0.00f, 1.00f},
    /*104 */ {0.7623f, 0.722f, 0.85f, 1.00f, 1.00f, 1.00f, 0.00f, 1.00f},
    /*105 */ {0.7920f, 0.746f, 0.74f, 0.90f, 0.97f, 0.75f, 0.00f, 0.97f},
    /*106 */ {0.8377f, 0.778f, 0.51f, 0.60f, 0.93f, 0.50f, 0.00f, 0.73f},
    /*107 */ {0.8903f, 0.820f, 0.18f, 0.25f, 0.90f, 0.30f, 0.00f, 0.59f},
    /*108 */ {0.9383f, 0.860f, 0.00f, 0.00f, 0.90f, 0.05f, 0.00f, 0.28f},
    /*109 */ {0.9771f, 0.889f, 0.00f, 0.00f, 0.90f, 0.00f, 0.00f, 0.00f},
    /*110 */ {0.9977f, 0.889f, 0.00f, 0.00f, 0.90f, 0.10f, 0.10f, 0.00f},
    /*111 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.25f, 0.25f, 0.00f},
    /*112 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.40f, 0.40f, 0.00f},
    /*113 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.55f, 0.55f, 0.00f},
    /*114 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.70f, 0.70f, 0.00f},
    /*115 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.80f, 0.80f, 0.00f},
    /*116 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.90f, 0.90f, 0.00f},
    /*117 */ {1.0000f, 0.889f, 0.00f, 0.00f, 0.90f, 0.97f, 0.97f, 0.00f},
    /*118 */ {1.0000f, 0.889f, 0.00f, 0.00f, 1.00f, 1.00f, 1.00f, 0.00f},
};

typedef struct {
    float f;      /* position on the reference timeline */
    int phase;    /* 0 idle, 1 rewinding (enter/hold), 2 released (exit) */
} RewindAnim;

static inline float rewind_lerp(float a, float b, float t) { return a + (b - a) * t; }

/* The look at timeline position f (linear between the reference's frames). */
static inline RewindLook RewindAnim_LookAt(float f)
{
    if (f <= (float)REWIND_F0) return rewind_table[0];
    if (f >= (float)REWIND_FLAST) return rewind_table[REWIND_N - 1];
    int i = (int)f;
    float t = f - (float)i;
    const RewindLook *a = &rewind_table[i - REWIND_F0], *b = &rewind_table[i + 1 - REWIND_F0];
    RewindLook r = {
        rewind_lerp(a->scale, b->scale, t), rewind_lerp(a->panelTop, b->panelTop, t),
        rewind_lerp(a->panelAlpha, b->panelAlpha, t), rewind_lerp(a->thumbAlpha, b->thumbAlpha, t),
        rewind_lerp(a->thumbScale, b->thumbScale, t), rewind_lerp(a->bar, b->bar, t),
        rewind_lerp(a->cap1, b->cap1, t), rewind_lerp(a->cap2, b->cap2, t),
    };
    return r;
}

static inline RewindLook RewindAnim_Look(const RewindAnim *a) { return RewindAnim_LookAt(a->f); }
static inline bool RewindAnim_Active(const RewindAnim *a) { return a->phase != 0; }
/* The real emulator window is hidden while this is false. */
static inline bool RewindAnim_RealWindowShows(const RewindAnim *a)
{
    return a->phase == 0 || a->f >= (float)REWIND_FGAME_FULL;
}

static inline void RewindAnim_Init(RewindAnim *a) { a->f = (float)REWIND_F0; a->phase = 0; }

/* Advance by dt seconds; rewinding = the rewind key/chord is held. */
static inline void RewindAnim_Step(RewindAnim *a, bool rewinding, float dt)
{
    float adv = dt * REWIND_REF_FPS;
    if (rewinding) {
        if (a->phase == 2) {   /* pressed again while leaving: resume from the matching enter frame */
            RewindLook now = RewindAnim_LookAt(a->f);
            float best = (float)REWIND_F0, bestd = 1e9f;
            for (int k = REWIND_F0 * 10; k <= 920; k++) {
                float d = fabsf(RewindAnim_LookAt((float)k / 10.0f).scale - now.scale);
                if (d < bestd) { bestd = d; best = (float)k / 10.0f; }
            }
            a->f = best;
        }
        a->phase = 1;
        a->f += adv;
        if (a->f > (float)REWIND_FHOLD) a->f = (float)REWIND_FHOLD;
        return;
    }
    if (a->phase == 1) {
        if (a->f >= (float)REWIND_FSPLIT) { a->phase = 2; if (a->f < (float)REWIND_FHOLD) a->f = (float)REWIND_FHOLD; }
        else a->phase = 3;   /* before the second caption: play the enter backwards */
    }
    if (a->phase == 3) {
        a->f -= adv;
        if (a->f <= (float)REWIND_F0) { a->f = (float)REWIND_F0; a->phase = 0; }
    } else if (a->phase == 2) {
        a->f += adv;
        if (a->f >= (float)REWIND_FLAST) { a->f = (float)REWIND_F0; a->phase = 0; }
    }
}

/* Thumbnail strip: the selected frame sits at cx; the thumbnail taken `slotFrame` (of `frame`
 * recorded frames, one every `stride`) lies to its left, one step (width + gap) per stride, so the
 * strip scrolls as the game goes back. Slots from the future (rewound past) fade out. */
static inline float RewindStrip_X(float cx, float step, int frame, int slotFrame, int stride)
{
    return cx - step * (1.0f + (float)(frame - slotFrame) / (float)stride);
}

static inline float RewindStrip_Alpha(int frame, int slotFrame, int stride)
{
    float d = (float)(frame - slotFrame);
    if (d >= 0.0f) return 1.0f;
    float a = 1.0f + d / ((float)stride * 0.5f);
    return a > 0.0f ? a : 0.0f;
}

#endif
