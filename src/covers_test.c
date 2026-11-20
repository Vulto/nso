/* covers_test.c - reload order: visible screen, first, last, then the middle. */
#define COVERS_IMPLEMENTATION
#include "covers.h"
#include <stdio.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static void fill_rows(CoversRect *s, int count, float h)
{
    for (int i = 0; i < count; i++) {
        s[i].x = 0.0f;
        s[i].y = (float)i * h;
        s[i].width = 100.0f;
        s[i].height = h;
    }
}

static int is_perm(const int *a, int n)
{
    for (int v = 0; v < n; v++) {
        int found = 0;
        for (int i = 0; i < n; i++) if (a[i] == v) found++;
        if (found != 1) return 0;
    }
    return 1;
}

int main(void)
{
    {   /* 20 rows of 100px, a screen is 300px (3 titles). Cursor at the top. */
        CoversRect s[20];
        int o[20], vis[8];
        fill_rows(s, 20, 100.0f);
        CHECK(Covers_Screen(s, 20, 0.0f, 300.0f, vis, 8) == 3);
        CHECK(vis[0] == 0 && vis[1] == 1 && vis[2] == 2);
        CHECK(Covers_Screen(s, 20, 1700.0f, 300.0f, vis, 8) == 3);
        CHECK(vis[0] == 17 && vis[1] == 18 && vis[2] == 19);
        CHECK(Covers_ReloadOrder(s, 20, 300.0f, 2000.0f, 0.0f, o, 20) == 20);
        CHECK(is_perm(o, 20));
        /* visible == first screen, then last, then middle */
        CHECK(o[0] == 0 && o[1] == 1 && o[2] == 2);
        CHECK(o[3] == 17 && o[4] == 18 && o[5] == 19);
        CHECK(o[6] == 3 && o[7] == 4 && o[19] == 16);
    }
    {   /* Cursor in the middle: visible is 8..10, then first, last, leftover. */
        CoversRect s[20];
        int o[20];
        fill_rows(s, 20, 100.0f);
        CHECK(Covers_ReloadOrder(s, 20, 300.0f, 2000.0f, 800.0f, o, 20) == 20);
        CHECK(is_perm(o, 20));
        CHECK(o[0] == 8 && o[1] == 9 && o[2] == 10);
        CHECK(o[3] == 0 && o[4] == 1 && o[5] == 2);
        CHECK(o[6] == 17 && o[7] == 18 && o[8] == 19);
        CHECK(o[9] == 3 && o[10] == 4 && o[19] == 16);
    }
    {   /* Everything fits on one screen: a single block, list order. */
        CoversRect s[5];
        int o[5];
        fill_rows(s, 5, 100.0f);
        CHECK(Covers_ReloadOrder(s, 5, 1000.0f, 500.0f, 0.0f, o, 5) == 5);
        CHECK(o[0] == 0 && o[1] == 1 && o[2] == 2 && o[3] == 3 && o[4] == 4);
        CHECK(is_perm(o, 5));
    }
    {   /* Empty / bad args */
        CoversRect s[2];
        int o[2];
        fill_rows(s, 2, 100.0f);
        CHECK(Covers_ReloadOrder(s, 0, 300.0f, 0.0f, 0.0f, o, 2) == 0);
        CHECK(Covers_ReloadOrder(NULL, 2, 300.0f, 200.0f, 0.0f, o, 2) == 0);
        CHECK(Covers_Screen(s, 2, 0.0f, 0.0f, o, 2) == 0);
        CHECK(Covers_ReloadOrder(s, 2, 300.0f, 200.0f, 0.0f, NULL, 2) == 0);
    }
    {   /* cursorViewY is clamped to the last screen */
        CoversRect s[10];
        int o[10], vis[8];
        fill_rows(s, 10, 100.0f);
        CHECK(Covers_Screen(s, 10, 700.0f, 300.0f, vis, 8) == 3);
        CHECK(vis[0] == 7 && vis[2] == 9);
        CHECK(Covers_ReloadOrder(s, 10, 300.0f, 1000.0f, 5000.0f, o, 10) == 10);
        CHECK(o[0] == 7 && o[1] == 8 && o[2] == 9);   /* visible == last */
        CHECK(o[3] == 0 && o[4] == 1 && o[5] == 2);   /* first */
        CHECK(is_perm(o, 10));
    }
    if (fails) { fprintf(stderr, "covers_test: %d failed\n", fails); return 1; }
    printf("covers_test: ok\n");
    return 0;
}
