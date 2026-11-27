/* gridlayout_test.c - per-system base slots, exact-aspect size limit, row pack. */
#include "gridlayout.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)
#define NEAR(a, b, e) CHECK(fabsf((a) - (b)) <= (e))

int main(void)
{
    const GridSysBase *nes = Grid_Lookup("nes");
    const GridSysBase *snes = Grid_Lookup("snes");
    const GridSysBase *n64 = Grid_Lookup("n64");
    const GridSysBase *gc = Grid_Lookup("gamecube");
    const GridSysBase *md = Grid_Lookup("megadrive");
    CHECK(nes && nes->baseW == 512.0f && nes->baseH == 731.0f && !nes->mix);
    CHECK(snes && snes->baseW == 512.0f && snes->baseH == 364.0f && snes->mix);
    CHECK(n64 && n64->baseW == 512.0f && n64->baseH == 374.0f && !n64->mix);
    CHECK(gc && gc->baseW == 466.0f && gc->baseH == 672.0f && !gc->mix);
    CHECK(md && md->baseW == 479.0f && md->baseH == 680.0f && !md->mix);
    CHECK(GRID_FLEX == 0.05f);
    CHECK(GRID_MIN_ROW == 4);

    float bw, bh;
    /* NES / GameCube / Mega Drive stay tall even for a wide cover. */
    Grid_SlotBase(nes, 2.0f, &bw, &bh);
    CHECK(bw == 512.0f && bh == 731.0f);
    Grid_SlotBase(gc, 2.0f, &bw, &bh);
    CHECK(bw == 466.0f && bh == 672.0f);
    Grid_SlotBase(md, 2.0f, &bw, &bh);
    CHECK(bw == 479.0f && bh == 680.0f);
    /* N64 stays wide even for a tall cover. */
    Grid_SlotBase(n64, 0.5f, &bw, &bh);
    CHECK(bw == 512.0f && bh == 374.0f);
    /* SNES is the only system that flips. */
    Grid_SlotBase(snes, 512.0f / 364.0f, &bw, &bh);
    CHECK(bw == 512.0f && bh == 364.0f);
    Grid_SlotBase(snes, 364.0f / 512.0f, &bw, &bh);
    CHECK(bw == 364.0f && bh == 512.0f);

    float sw, sh;
    /* Typical NES cover fills the base slot exactly. */
    CHECK(Grid_FlexSlot(512.0f, 731.0f, 512.0f / 731.0f, &sw, &sh) == 1);
    NEAR(sw, 512.0f, 0.02f);
    NEAR(sh, 731.0f, 0.02f);
    /* A slightly off aspect still has the cover shape, size inside ±5%. */
    CHECK(Grid_FlexSlot(512.0f, 731.0f, (512.0f * 1.04f) / 731.0f, &sw, &sh) == 1);
    NEAR(sw / sh, (512.0f * 1.04f) / 731.0f, 1e-5f);
    CHECK(sw <= 512.0f * 1.05f + 0.01f && sh <= 731.0f * 1.05f + 0.01f);
    /* A very wide NES cover stays cover-shaped; size is capped at +5%. */
    CHECK(Grid_FlexSlot(512.0f, 731.0f, 2.0f, &sw, &sh) == 1);
    NEAR(sw / sh, 2.0f, 1e-5f);
    NEAR(sw, 512.0f * 1.05f, 0.05f);

    /* 16 identical NES covers on 1920x1080: 7 per full row, last 2 at H0. */
    const int originX = 100, contentW = 1820, screenW = 1920, screenH = 1080;
    const float gap = GRID_GAP, vis = GRID_VISIBLE_ROWS;
    float aspects[16];
    for (int i = 0; i < 16; i++) aspects[i] = 512.0f / 731.0f;
    GridLaidItem laid[16];
    float totalH = 0.0f;
    int rows = 0;
    Grid_Pack("nes", screenW, screenH, originX, contentW, aspects, 16,
              gap, vis, laid, 16, &totalH, &rows);
    CHECK(rows == 3);
    for (int i = 0; i < 7; i++) CHECK(laid[i].row == 0);
    for (int i = 7; i < 14; i++) CHECK(laid[i].row == 1);
    CHECK(laid[14].row == 2 && laid[15].row == 2);
    /* Full rows fill the content width exactly (1px side gaps). */
    NEAR(laid[0].x, (float)originX + gap, 0.02f);
    NEAR(laid[6].x + laid[6].w, (float)originX + (float)contentW - gap, 0.05f);
    NEAR(laid[13].x + laid[13].w, (float)originX + (float)contentW - gap, 0.05f);
    /* Gaps between covers in a full row are 1px. */
    NEAR(laid[1].x - (laid[0].x + laid[0].w), gap, 0.02f);
    /* Slots are cover-shaped and share a height in each full row. */
    for (int i = 0; i < 14; i++) {
        NEAR(laid[i].fullScale, 1.0f, 1e-4f);
        NEAR(laid[i].w / laid[i].h, 512.0f / 731.0f, 1e-5f);
        NEAR(laid[i].w, laid[0].w, 0.05f);
        NEAR(laid[i].h, laid[0].h, 0.05f);
    }
    /* Short last row stays at H0 — standard visible-row height. */
    float innerH = (float)screenH - (vis - 1.0f) * gap;
    float H0 = innerH / vis;
    NEAR(laid[14].h, H0, 0.05f);
    NEAR(laid[14].w, (512.0f / 731.0f) * H0, 0.05f);
    NEAR(laid[15].w, laid[14].w, 0.02f);
    CHECK(laid[15].x + laid[15].w + gap < (float)originX + (float)contentW - 1.0f);
    /* Last-row covers are smaller than the stretched full-row slots. */
    CHECK(laid[14].w < laid[0].w - 1.0f);
    NEAR(totalH, laid[0].h + gap + laid[7].h + gap + laid[14].h, 0.05f);

    /* SNES mixed row of two (list shorter than 4): common height, not stretched. */
    float mixA[2] = { 512.0f / 364.0f, 364.0f / 512.0f };
    GridLaidItem mix[2];
    float mixH = 0.0f;
    int mixRows = 0;
    Grid_Pack("snes", screenW, screenH, originX, contentW, mixA, 2,
              gap, vis, mix, 2, &mixH, &mixRows);
    CHECK(mixRows == 1);
    CHECK(mix[0].row == 0 && mix[1].row == 0);
    CHECK(mix[0].w > mix[0].h); /* wide */
    CHECK(mix[1].h > mix[1].w); /* tall */
    NEAR(mix[0].h, mix[1].h, 0.02f); /* common row height */
    NEAR(mix[0].y, mix[1].y, 0.02f);
    NEAR(mix[0].w / mix[0].h, mixA[0], 1e-5f);
    NEAR(mix[1].w / mix[1].h, mixA[1], 1e-5f);
    NEAR(mix[1].x - (mix[0].x + mix[0].w), gap, 0.02f);
    CHECK(mix[1].x + mix[1].w + gap < (float)originX + (float)contentW - 1.0f);

    /* Odd-aspect cover: slot is cover-shaped, image would fill it (fullScale 1). */
    float outA[1] = { 2.0f };
    GridLaidItem outL[1];
    float outHt = 0.0f;
    int outRows = 0;
    Grid_Pack("nes", screenW, screenH, originX, contentW, outA, 1,
              gap, vis, outL, 1, &outHt, &outRows);
    CHECK(outRows == 1);
    NEAR(outL[0].fullScale, 1.0f, 1e-4f);
    NEAR(outL[0].w / outL[0].h, 2.0f, 1e-5f);
    float fitW, fitH;
    Grid_FitAspect(outL[0].w, outL[0].h, 2.0f, &fitW, &fitH);
    NEAR(fitW, outL[0].w, 0.02f);
    NEAR(fitH, outL[0].h, 0.02f);

    /* A justified mixed SNES list fills width with no holes; non-final rows
     * have at least 4 covers; list order is unchanged. */
    float rowA[5] = { 512.0f/364.0f, 364.0f/512.0f, 512.0f/364.0f,
                      512.0f/926.0f, 512.0f/364.0f };
    GridLaidItem rowL[20];
    float manyA[20];
    for (int i = 0; i < 20; i++) manyA[i] = rowA[i % 5];
    float manyH = 0.0f;
    int manyRows = 0;
    Grid_Pack("snes", screenW, screenH, originX, contentW, manyA, 20,
              gap, vis, rowL, 20, &manyH, &manyRows);
    CHECK(manyRows >= 2);
    int k0 = 0, lastRow = rowL[19].row;
    float sumW = 0, rowHeight = 0, holes = 0, letter = 0;
    for (int i = 0; i < 20 && rowL[i].row == 0; i++) {
        k0++;
        sumW += rowL[i].w;
        rowHeight = rowL[i].h;
        NEAR(rowL[i].h, rowL[0].h, 0.02f);
        NEAR(rowL[i].w / rowL[i].h, manyA[i], 1e-5f);
        float fw, fh;
        Grid_FitAspect(rowL[i].w, rowL[i].h, manyA[i], &fw, &fh);
        letter += rowL[i].w * rowL[i].h - fw * fh;
        holes += rowL[i].w * (rowHeight - rowL[i].h);
    }
    CHECK(k0 >= GRID_MIN_ROW);
    NEAR(rowL[0].x, (float)originX + gap, 0.02f);
    NEAR(rowL[k0 - 1].x + rowL[k0 - 1].w, (float)originX + (float)contentW - gap, 0.05f);
    NEAR(letter, 0.0f, 0.5f);
    NEAR(holes, 0.0f, 0.5f);
    NEAR((float)contentW - gap * (float)(k0 + 1) - sumW, 0.0f, 0.05f);
    /* Non-final rows have at least 4 covers; order is list order. */
    int counts[20] = { 0 };
    for (int i = 0; i < 20; i++) {
        CHECK(rowL[i].row >= 0 && rowL[i].row == (i == 0 ? 0 : rowL[i - 1].row + (rowL[i].row != rowL[i - 1].row)));
        counts[rowL[i].row]++;
        if (i > 0) CHECK(rowL[i].row >= rowL[i - 1].row); /* no reordering */
    }
    for (int r = 0; r < lastRow; r++) CHECK(counts[r] >= GRID_MIN_ROW);
    CHECK(counts[lastRow] >= 1);

    /* 4 wide then 4 tall: filling the tall row alone would be gigantic.
     * The 2/3-row joint split must keep heights near H0 (and list order). */
    float eightA[8];
    for (int i = 0; i < 4; i++) eightA[i] = 512.0f / 364.0f;
    for (int i = 4; i < 8; i++) eightA[i] = 364.0f / 512.0f;
    GridLaidItem eight[8];
    float eightH = 0.0f;
    int eightRows = 0;
    Grid_Pack("snes", screenW, screenH, originX, contentW, eightA, 8,
              gap, vis, eight, 8, &eightH, &eightRows);
    CHECK(eightRows >= 1);
    float tallAlone = ((float)contentW - gap * 5.0f) / (4.0f * (364.0f / 512.0f));
    float innerH2 = (float)screenH - (vis - 1.0f) * gap;
    float baseH = innerH2 / vis;
    CHECK(tallAlone > baseH * 1.8f); /* the isolated fill really is huge */
    for (int i = 0; i < 8; i++) {
        NEAR(eight[i].w / eight[i].h, eightA[i], 1e-5f);
        CHECK(eight[i].h < baseH * 1.6f);
        if (i > 0) CHECK(eight[i].row >= eight[i - 1].row);
    }
    if (eightRows >= 2) {
        int firstK = 0;
        while (firstK < 8 && eight[firstK].row == 0) firstK++;
        CHECK(firstK >= GRID_MIN_ROW);
        /* Full (non-short) first row fills the width. */
        NEAR(eight[firstK - 1].x + eight[firstK - 1].w,
             (float)originX + (float)contentW - gap, 0.05f);
    }

    /* Header size (no decode): the grid knows each cover's shape before its
     * texture loads, so slots do not jump when entering a system. */
    {
        int w = 0, h = 0;
        const unsigned char pngH[24] = { 0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A,
                                         0, 0, 0, 13, 'I', 'H', 'D', 'R',
                                         0, 0, 0x01, 0xDF, 0, 0, 0x02, 0xA8 };
        CHECK(Grid_ImageHeaderSize(pngH, sizeof pngH, &w, &h) && w == 479 && h == 680);
        CHECK(!Grid_ImageHeaderSize(pngH, 20, &w, &h));
        const unsigned char jpg[] = { 0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x04, 0x00, 0x00,
                                      0xFF, 0xC2, 0x00, 0x11, 0x08, 0x02, 0xD5, 0x02, 0x00, 0x03 };
        w = h = 0;
        CHECK(Grid_ImageHeaderSize(jpg, sizeof jpg, &w, &h) && w == 512 && h == 725);
        const unsigned char junk[24] = { 'G', 'I', 'F', '8' };
        CHECK(!Grid_ImageHeaderSize(junk, sizeof junk, &w, &h));
        /* Same size the loader shrinks covers to (512x384 box), so the slot
         * from the header equals the slot from the texture. */
        int fw, fh;
        Grid_BoxartFitSize(479, 680, 512, 384, &fw, &fh);
        CHECK(fw == 270 || fw == 271);
        CHECK(fw == (int)(479 * (384.0f / 680.0f) + 0.5f) && fh == 384);
        Grid_BoxartFitSize(300, 200, 512, 384, &fw, &fh);
        CHECK(fw == 300 && fh == 200);
        Grid_BoxartFitSize(1024, 512, 512, 384, &fw, &fh);
        CHECK(fw == 512 && fh == 256);
    }

    if (fails) {
        fprintf(stderr, "gridlayout_test: %d failed\n", fails);
        return 1;
    }
    printf("gridlayout_test: ok\n");
    return 0;
}
