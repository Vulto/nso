/* order_test.c - grid navigation, Y Move reorder, saved-order parsing. */
#define ORDER_IMPLEMENTATION
#include "order.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

/* The grid's original inline navigation, kept here as the reference. */
static int OldNav(int idx, int count, int cols, OrderNav d)
{
    int row = idx / cols, col = idx % cols, rows = (count + cols - 1) / cols;
    if (d == ORDER_NAV_RIGHT) {
        if (col < cols - 1 && idx + 1 < count) return idx + 1;
        int t = ((row + 1) % rows) * cols;
        return t < count ? t : idx;
    }
    if (d == ORDER_NAV_LEFT) return col > 0 ? idx - 1 : idx;
    int t = (d == ORDER_NAV_DOWN ? (row + 1) % rows : (row - 1 + rows) % rows) * cols + col;
    return t >= count ? count - 1 : t;
}

int main(void)
{
    for (int count = 1; count <= 23; count++)
        for (int cols = 1; cols <= 7; cols++)
            for (int i = 0; i < count; i++)
                for (int d = 0; d < 4; d++)
                    CHECK(Order_GridNav(i, count, cols, (OrderNav)d) == OldNav(i, count, cols, (OrderNav)d));
    CHECK(Order_GridNav(0, 0, 5, ORDER_NAV_RIGHT) == 0);
    CHECK(Order_GridNav(3, 5, 0, ORDER_NAV_RIGHT) == 3);
    CHECK(Order_GridNav(4, 5, 5, ORDER_NAV_RIGHT) == 0);  /* one row: wraps to start */
    CHECK(Order_GridNav(7, 8, 5, ORDER_NAV_UP) == 2);
    CHECK(Order_GridNav(3, 8, 5, ORDER_NAV_DOWN) == 7);   /* clamped to last */

    int a[5] = { 0, 1, 2, 3, 4 };
    Order_Move(a, sizeof(int), 5, 1, 3);
    CHECK(a[0] == 0 && a[1] == 2 && a[2] == 3 && a[3] == 1 && a[4] == 4);
    Order_Move(a, sizeof(int), 5, 3, 0);
    CHECK(a[0] == 1 && a[1] == 0 && a[2] == 2 && a[3] == 3 && a[4] == 4);
    Order_Move(a, sizeof(int), 5, 4, 9);  /* out of range: untouched */
    Order_Move(a, sizeof(int), 5, -1, 2);
    CHECK(a[0] == 1 && a[4] == 4);

    const char *t[4] = { "Super Mario Bros", "The Wing of Madoola", "Fire 'n Ice", "Zelda" };
    int p[4];
    Order_Perm(NULL, t, 4, p);
    CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2 && p[3] == 3);
    Order_Perm("Fire 'n Ice\r\nGone\nSuper Mario Bros\nFire 'n Ice\n", t, 4, p);
    CHECK(p[0] == 2 && p[1] == 0 && p[2] == 1 && p[3] == 3);
    Order_Perm("Zelda", t, 4, p);  /* no trailing newline */
    CHECK(p[0] == 3 && p[1] == 0 && p[2] == 1 && p[3] == 2);
    Order_Perm("Super Mario\n", t, 4, p);  /* prefix is not a match */
    CHECK(p[0] == 0 && p[3] == 3);

    char buf[128];
    const char *u[3] = { t[2], t[0], t[1] };
    CHECK(Order_Format(buf, sizeof(buf), u, 3) == 0);
    CHECK(strcmp(buf, "Fire 'n Ice\nSuper Mario Bros\nThe Wing of Madoola\n") == 0);
    Order_Perm(buf, t, 3, p);
    CHECK(p[0] == 2 && p[1] == 0 && p[2] == 1);
    CHECK(Order_Format(buf, 8, u, 3) == -1 && buf[0] == '\0');

    if (fails) return 1;
    puts("order_test: ok");
    return 0;
}
