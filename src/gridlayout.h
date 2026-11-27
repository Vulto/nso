/* gridlayout.h - per-system games-grid slots, 5% size limit, and row packing.
 * Pure C, no raylib: unit-tested in gridlayout_test.c. */
#ifndef NSO_GRIDLAYOUT_H
#define NSO_GRIDLAYOUT_H

#include <math.h>
#include <stddef.h>
#include <string.h>

#ifndef GRID_PACK_MAX
#ifdef MAX_GAMES
#define GRID_PACK_MAX MAX_GAMES
#else
#define GRID_PACK_MAX 4096
#endif
#endif

#define GRID_FLEX 0.05f
#define GRID_GAP 1.0f
#define GRID_VISIBLE_ROWS 3.1f
#define GRID_MIN_ROW 4

typedef struct {
    const char *id;
    float baseW;
    float baseH;
    int mix; /* 1: SNES — wide base, tall is the same slot flipped */
} GridSysBase;

typedef struct {
    float x, y, w, h;   /* exact-aspect slot, screen pixels */
    float fullW, fullH; /* same as w/h: the image fills the slot */
    float fullScale;    /* always 1: slot shape matches the cover */
    int row;
} GridLaidItem;

/* Typical US cover size for this system. mix is only set for SNES. */
static const GridSysBase kGridSysBase[] = {
    { "nes",       512.0f, 731.0f, 0 }, /* tall only */
    { "snes",      512.0f, 364.0f, 1 }, /* wide / flipped tall */
    { "n64",       512.0f, 374.0f, 0 }, /* wide only */
    { "gamecube",  466.0f, 672.0f, 0 }, /* tall only */
    { "megadrive", 479.0f, 680.0f, 0 }, /* tall only */
};

static const GridSysBase kGridSysBaseFallback = { "", 512.0f, 364.0f, 0 };

static const GridSysBase *Grid_Lookup(const char *id)
{
    if (id) {
        for (int i = 0; i < (int)(sizeof(kGridSysBase) / sizeof(kGridSysBase[0])); i++) {
            if (strcmp(kGridSysBase[i].id, id) == 0) return &kGridSysBase[i];
        }
    }
    return &kGridSysBaseFallback;
}

/* Base slot in the system's typical orientation. SNES (mix) uses the flipped
 * slot when the cover is tall; NES / GameCube / Mega Drive stay tall, N64 stays
 * wide, even if a particular cover's aspect is the other way. */
static void __attribute__((unused)) Grid_SlotBase(const GridSysBase *b, float aspect, float *bw, float *bh)
{
    if (!b) b = &kGridSysBaseFallback;
    if (b->mix) {
        if (aspect >= 1.0f) {
            *bw = b->baseW;
            *bh = b->baseH;
        } else {
            *bw = b->baseH;
            *bh = b->baseW;
        }
    } else {
        *bw = b->baseW;
        *bh = b->baseH;
    }
}

/* Fit (w/h) = aspect inside a cell, no crop or stretch. */
static void __attribute__((unused)) Grid_FitAspect(float cellW, float cellH, float aspect,
                           float *outW, float *outH)
{
    if (aspect <= 0.0f) aspect = 4.0f / 3.0f;
    if (cellW <= 0.0f) cellW = 1.0f;
    if (cellH <= 0.0f) cellH = 1.0f;
    if (cellW / cellH > aspect) {
        *outH = cellH;
        *outW = cellH * aspect;
    } else {
        *outW = cellW;
        *outH = cellW / aspect;
    }
}

/* Slot is always the cover's aspect (the image fills it). Size, not shape,
 * is limited to the base ± GRID_FLEX: lock height to the base, then scale
 * uniformly so no side exceeds +5% and the slot is not under -5% on both
 * sides when that would still fit. Returns 1 (always exact aspect). */
static int __attribute__((unused)) Grid_FlexSlot(float bw, float bh, float aspect, float *sw, float *sh)
{
    if (bw < 1.0f) bw = 1.0f;
    if (bh < 1.0f) bh = 1.0f;
    if (aspect <= 0.0f) aspect = bw / bh;
    float w = aspect * bh, h = bh;
    float maxRel = fmaxf(w / bw, h / bh);
    float minRel = fminf(w / bw, h / bh);
    float s = 1.0f;
    if (maxRel > 1.0f + GRID_FLEX) {
        s = (1.0f + GRID_FLEX) / maxRel;
    } else if (minRel < 1.0f - GRID_FLEX) {
        s = (1.0f - GRID_FLEX) / minRel;
        if (s * maxRel > 1.0f + GRID_FLEX) {
            s = (1.0f + GRID_FLEX) / maxRel;
        }
    }
    *sw = w * s;
    *sh = h * s;
    return 1;
}

/* Last row of the list that still has room for another cover at H0: keep H0
 * so a handful of leftover games are not stretched huge. */
static int Grid_IsShortLast(int contentW, float gap, float H0, float minAspect,
                            const float *ps, int lo, int hi, int count)
{
    int k = hi - lo;
    float sumA;
    if (hi != count || k <= 0) return 0;
    sumA = ps[hi] - ps[lo];
    return (sumA + minAspect) * H0 + (float)(k + 2) * gap <= (float)contentW + 0.01f;
}

static float Grid_RowH(int contentW, float gap, float H0, float minAspect,
                       const float *ps, int lo, int hi, int count)
{
    int k = hi - lo;
    float sumA, inner;
    if (k <= 0) return H0;
    if (Grid_IsShortLast(contentW, gap, H0, minAspect, ps, lo, hi, count)) return H0;
    sumA = ps[hi] - ps[lo];
    inner = (float)contentW - gap * (float)(k + 1);
    if (inner < 1.0f) inner = 1.0f;
    if (sumA <= 1e-6f) return H0;
    return inner / sumA;
}

/* Cost of one split of [lo, hi) into one or more rows (2 or 3), plus a
 * fixed neighbor height on either side (neg = none). */
static float Grid_SplitCost(int contentW, float gap, float H0, float minAspect,
                            const float *ps, int count,
                            const int *cut, int ncut,
                            float hAbove, float hBelow)
{
    float cost = 0.0f, prev = hAbove;
    for (int r = 0; r < ncut - 1; r++) {
        float H = Grid_RowH(contentW, gap, H0, minAspect, ps, cut[r], cut[r + 1], count);
        float d = H - H0;
        cost += d * d;
        if (prev >= 0.0f) {
            float n = H - prev;
            cost += n * n;
        }
        prev = H;
    }
    if (hBelow >= 0.0f && prev >= 0.0f) {
        float n = prev - hBelow;
        cost += n * n;
    }
    return cost;
}

/* Pixel size from an image header, without decoding: PNG (IHDR) or JPEG
 * (first SOFn marker within the bytes given). Returns 1 on success. Lets the
 * grid lay covers out at their real shape before the textures load. */
static int __attribute__((unused)) Grid_ImageHeaderSize(const unsigned char *b, size_t n, int *w, int *h)
{
    if (!b || !w || !h) return 0;
    static const unsigned char png[8] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A };
    if (n >= 24 && memcmp(b, png, 8) == 0 && memcmp(b + 12, "IHDR", 4) == 0) {
        unsigned long pw = ((unsigned long)b[16] << 24) | ((unsigned long)b[17] << 16) | ((unsigned long)b[18] << 8) | b[19];
        unsigned long ph = ((unsigned long)b[20] << 24) | ((unsigned long)b[21] << 16) | ((unsigned long)b[22] << 8) | b[23];
        if (pw == 0 || ph == 0 || pw > 0x7fffffffUL || ph > 0x7fffffffUL) return 0;
        *w = (int)pw;
        *h = (int)ph;
        return 1;
    }
    if (n >= 4 && b[0] == 0xFF && b[1] == 0xD8) {
        size_t i = 2;
        while (i + 4 <= n) {
            if (b[i] != 0xFF) return 0;
            unsigned char m = b[i + 1];
            if (m == 0xFF) { i++; continue; }
            if (m == 0xD8 || m == 0x01 || (m >= 0xD0 && m <= 0xD7)) { i += 2; continue; }
            size_t len = ((size_t)b[i + 2] << 8) | b[i + 3];
            if (len < 2) return 0;
            if (m >= 0xC0 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC) {
                if (i + 9 > n) return 0;
                int jh = (b[i + 5] << 8) | b[i + 6];
                int jw = (b[i + 7] << 8) | b[i + 8];
                if (jw <= 0 || jh <= 0) return 0;
                *w = jw;
                *h = jh;
                return 1;
            }
            i += 2 + len;
        }
    }
    return 0;
}

/* Size a w x h cover is decoded to: shrunk (aspect kept, rounded) to fit in
 * maxW x maxH, never enlarged. The same math as the loader, so a slot sized
 * from the header matches the texture exactly. */
static void __attribute__((unused)) Grid_BoxartFitSize(int w, int h, int maxW, int maxH, int *nw, int *nh)
{
    *nw = w;
    *nh = h;
    if (w <= 0 || h <= 0) return;
    if (w > maxW || h > maxH) {
        float sx = (float)maxW / (float)w;
        float sy = (float)maxH / (float)h;
        float s = fminf(sx, sy);
        if (s < 1.0f) {
            int rw = (int)(w * s + 0.5f);
            int rh = (int)(h * s + 0.5f);
            *nw = rw < 1 ? 1 : rw;
            *nh = rh < 1 ? 1 : rh;
        }
    }
}

/* Pack covers into rows of a common height so every slot is cover-shaped
 * (the image fills it) and full rows fill contentW exactly (1px gaps between
 * covers and on both sides). Each non-final row has at least GRID_MIN_ROW
 * covers; a short last row may have fewer if the list runs out, and stays
 * at H0. Row breaks are chosen jointly over a sliding window of 3 rows
 * (and 2 at the ends) so heights stay close to the base and to neighbors,
 * instead of filling each row in isolation (which can make a skinny row
 * gigantic). List order is preserved. */
static void Grid_Pack(const char *sysId,
                      int screenW, int screenH,
                      int originX, int contentW,
                      const float *aspects, int count,
                      float gap, float visibleRows,
                      GridLaidItem *out, int cap,
                      float *totalH, int *rowCount)
{
    (void)screenW;
    if (count > cap) count = cap;
    if (count > GRID_PACK_MAX) count = GRID_PACK_MAX;
    if (totalH) *totalH = 0.0f;
    if (rowCount) *rowCount = 0;
    if (!out || !aspects || count <= 0) return;
    if (contentW < 1) contentW = 1;
    if (screenH < 1) screenH = 1;
    if (gap < 0.0f) gap = 0.0f;
    if (visibleRows < 1.0f) visibleRows = 1.0f;

    const GridSysBase *sys = Grid_Lookup(sysId);
    float a[GRID_PACK_MAX];
    float ps[GRID_PACK_MAX + 1];
    float fallbackA = sys->baseW / sys->baseH;
    if (fallbackA <= 0.0f) fallbackA = 4.0f / 3.0f;
    ps[0] = 0.0f;
    for (int i = 0; i < count; i++) {
        a[i] = aspects[i] > 0.0f ? aspects[i] : fallbackA;
        ps[i + 1] = ps[i] + a[i];
    }

    float innerH = (float)screenH - (visibleRows - 1.0f) * gap;
    if (innerH < 1.0f) innerH = 1.0f;
    float H0 = innerH / visibleRows;
    if (H0 <= 0.0f) H0 = 1.0f;

    float minAspect = (sys->baseW / sys->baseH) * (1.0f - GRID_FLEX);
    if (sys->mix) {
        float tallA = (sys->baseH / sys->baseW) * (1.0f - GRID_FLEX);
        if (tallA < minAspect) minAspect = tallA;
    }
    if (minAspect < 0.05f) minAspect = 0.05f;

    /* Initial pack: as many as fit at H0, but at least GRID_MIN_ROW when
     * enough titles remain (the last row may be shorter). */
    int start[GRID_PACK_MAX + 1];
    int rows = 0;
    int i = 0;
    while (i < count) {
        int rem = count - i;
        int k = 0;
        float sumA = 0.0f;
        while (i + k < count) {
            float next = sumA + a[i + k];
            if (k >= GRID_MIN_ROW
                && next * H0 + (float)(k + 2) * gap > (float)contentW + 0.01f) {
                break;
            }
            sumA = next;
            k++;
        }
        if (k < GRID_MIN_ROW && rem >= GRID_MIN_ROW) {
            k = GRID_MIN_ROW;
        }
        if (k < 1) k = 1;
        if (k > rem) k = rem;
        start[rows++] = i;
        i += k;
    }
    start[rows] = count;

    /* Jointly re-split windows of 2 and 3 rows so no row is filled alone. */
    for (int pass = 0; pass < 16; pass++) {
        int changed = 0;
        for (int win = 2; win <= 3; win++) {
            for (int r = 0; r + win <= rows; r++) {
                int lo = start[r];
                int hi = start[r + win];
                int listEnd = (r + win == rows);
                float hAbove = (r > 0)
                    ? Grid_RowH(contentW, gap, H0, minAspect, ps, start[r - 1], start[r], count)
                    : -1.0f;
                float hBelow = (r + win < rows)
                    ? Grid_RowH(contentW, gap, H0, minAspect, ps, start[r + win], start[r + win + 1], count)
                    : -1.0f;
                int cur[4];
                for (int t = 0; t <= win; t++) cur[t] = start[r + t];
                float best = Grid_SplitCost(contentW, gap, H0, minAspect, ps, count,
                                            cur, win + 1, hAbove, hBelow);
                int bestCuts[2] = { start[r + 1], win > 2 ? start[r + 2] : 0 };

                if (win == 2) {
                    int minL = GRID_MIN_ROW;
                    int maxA = listEnd ? (hi - 1) : (hi - GRID_MIN_ROW);
                    for (int cut = lo + minL; cut <= maxA; cut++) {
                        int trial[3] = { lo, cut, hi };
                        if (cut - lo < GRID_MIN_ROW) continue;
                        if (!listEnd && hi - cut < GRID_MIN_ROW) continue;
                        if (hi - cut < 1) continue;
                        float c = Grid_SplitCost(contentW, gap, H0, minAspect, ps, count,
                                                 trial, 3, hAbove, hBelow);
                        if (c + 1e-4f < best) {
                            best = c;
                            bestCuts[0] = cut;
                        }
                    }
                    if (bestCuts[0] != start[r + 1]) {
                        start[r + 1] = bestCuts[0];
                        changed = 1;
                    }
                } else {
                    int maxA = listEnd ? (hi - 1 - 1) : (hi - 2 * GRID_MIN_ROW);
                    for (int a1 = lo + GRID_MIN_ROW; a1 <= maxA; a1++) {
                        int maxB = listEnd ? (hi - 1) : (hi - GRID_MIN_ROW);
                        for (int b1 = a1 + GRID_MIN_ROW; b1 <= maxB; b1++) {
                            int trial[4] = { lo, a1, b1, hi };
                            if (!listEnd && hi - b1 < GRID_MIN_ROW) continue;
                            if (hi - b1 < 1) continue;
                            float c = Grid_SplitCost(contentW, gap, H0, minAspect, ps, count,
                                                     trial, 4, hAbove, hBelow);
                            if (c + 1e-4f < best) {
                                best = c;
                                bestCuts[0] = a1;
                                bestCuts[1] = b1;
                            }
                        }
                    }
                    if (bestCuts[0] != start[r + 1] || bestCuts[1] != start[r + 2]) {
                        start[r + 1] = bestCuts[0];
                        start[r + 2] = bestCuts[1];
                        changed = 1;
                    }
                }
            }
        }
        if (!changed) break;
    }

    float y = 0.0f;
    for (int r = 0; r < rows; r++) {
        int lo = start[r], hi = start[r + 1];
        float H = Grid_RowH(contentW, gap, H0, minAspect, ps, lo, hi, count);
        if (H <= 0.0f) H = H0;
        float x = (float)originX + gap;
        for (int n = lo; n < hi; n++) {
            float w = a[n] * H, h = H;
            out[n].x = x;
            out[n].y = y;
            out[n].w = w;
            out[n].h = h;
            out[n].fullW = w;
            out[n].fullH = h;
            out[n].fullScale = 1.0f;
            out[n].row = r;
            x += w + gap;
        }
        y += H;
        if (r + 1 < rows) y += gap;
    }
    if (totalH) *totalH = y;
    if (rowCount) *rowCount = rows;
}

#endif /* NSO_GRIDLAYOUT_H */
