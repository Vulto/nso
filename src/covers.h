/* covers.h - the order in which boxarts come back after a game, so the grid is
 * usable at once and the rest fills in without a hitch. Pure C, no raylib;
 * unit-tested in covers_test.c.
 *
 * Unique indices, in this order:
 *   1. The visible screen around the cursor (what the user will see on return)
 *   2. The first screen of the list (top)
 *   3. The last screen of the list (bottom, including wrap-around from the top)
 *   4. The remaining middle, in list order */
#ifndef NSO_COVERS_H
#define NSO_COVERS_H

typedef struct {
    float x, y, width, height;
} CoversRect;

/* Titles whose slot intersects the viewport [viewY, viewY + viewH). */
int Covers_Screen(const CoversRect *slots, int count, float viewY, float viewH,
                  int *out, int cap);

/* Reload order as above. cursorViewY is the viewport that shows the cursor
 * (already clamped). lastY = max(0, totalH - viewH). Returns how many
 * indices were written (a permutation of 0..count-1 when cap >= count). */
int Covers_ReloadOrder(const CoversRect *slots, int count,
                       float viewH, float totalH, float cursorViewY,
                       int *out, int cap);

#endif /* NSO_COVERS_H */

#ifdef COVERS_IMPLEMENTATION
#ifndef COVERS_IMPLEMENTATION_ONCE
#define COVERS_IMPLEMENTATION_ONCE

static int Covers_Contains(const int *a, int n, int v)
{
    for (int i = 0; i < n; i++) {
        if (a[i] == v) return 1;
    }
    return 0;
}

int Covers_Screen(const CoversRect *slots, int count, float viewY, float viewH,
                  int *out, int cap)
{
    int n = 0;
    if (!slots || !out || count <= 0 || cap <= 0 || viewH <= 0.0f) return 0;
    float viewEnd = viewY + viewH;
    for (int i = 0; i < count && n < cap; i++) {
        float y0 = slots[i].y;
        float y1 = y0 + slots[i].height;
        if (y1 > viewY && y0 < viewEnd) out[n++] = i;
    }
    return n;
}

static int Covers_AppendScreen(const CoversRect *slots, int count,
                               float viewY, float viewH,
                               int *out, int n, int cap)
{
    int block[512];
    int bn = Covers_Screen(slots, count, viewY, viewH, block,
                           (int)(sizeof block / sizeof block[0]));
    for (int i = 0; i < bn && n < cap; i++) {
        if (!Covers_Contains(out, n, block[i])) out[n++] = block[i];
    }
    return n;
}

int Covers_ReloadOrder(const CoversRect *slots, int count,
                       float viewH, float totalH, float cursorViewY,
                       int *out, int cap)
{
    if (!slots || !out || count <= 0 || cap <= 0) return 0;
    if (viewH < 1.0f) viewH = 1.0f;
    float lastY = totalH > viewH ? totalH - viewH : 0.0f;
    if (cursorViewY < 0.0f) cursorViewY = 0.0f;
    if (cursorViewY > lastY) cursorViewY = lastY;

    int n = 0;
    n = Covers_AppendScreen(slots, count, cursorViewY, viewH, out, n, cap);
    n = Covers_AppendScreen(slots, count, 0.0f, viewH, out, n, cap);
    n = Covers_AppendScreen(slots, count, lastY, viewH, out, n, cap);
    for (int i = 0; i < count && n < cap; i++) {
        if (!Covers_Contains(out, n, i)) out[n++] = i;
    }
    return n;
}

#endif /* COVERS_IMPLEMENTATION_ONCE */
#endif /* COVERS_IMPLEMENTATION */
