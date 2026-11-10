/* order.h - games-grid navigation and the user's game order (Y Move).
 * Pure C, no raylib: unit-tested in order_test.c. The order is saved per
 * system as config/order-<id>.txt, one game title per line. */
#ifndef NSO_ORDER_H
#define NSO_ORDER_H

#include <stddef.h>

typedef enum { ORDER_NAV_LEFT, ORDER_NAV_RIGHT, ORDER_NAV_UP, ORDER_NAV_DOWN } OrderNav;

/* Grid cursor step (same rules the grid always used): Right wraps to the
 * next row start, Left stops at column 0, Up/Down wrap rows and clamp to
 * the last game. Returns idx unchanged when count/cols are invalid. */
int Order_GridNav(int idx, int count, int cols, OrderNav dir);
/* Move element `from` to slot `to`, shifting the ones between (size <= 256). */
void Order_Move(void *base, size_t size, int count, int from, int to);
/* perm[k] = original index shown k-th: titles listed in `text` first (file
 * order, unknown / duplicate lines ignored), then the rest in original order. */
void Order_Perm(const char *text, const char *const *titles, int count, int *perm);
/* "title\n" per game; returns 0, or -1 if it did not fit. */
int Order_Format(char *dst, size_t n, const char *const *titles, int count);

#endif /* NSO_ORDER_H */

#ifdef ORDER_IMPLEMENTATION
#ifndef ORDER_IMPLEMENTATION_ONCE
#define ORDER_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <string.h>

int Order_GridNav(int idx, int count, int cols, OrderNav dir)
{
    if (count <= 0 || cols < 1 || idx < 0 || idx >= count) return idx;
    int row = idx / cols, col = idx % cols;
    int rows = (count + cols - 1) / cols;
    int t = idx;
    switch (dir) {
        case ORDER_NAV_RIGHT:
            if (col < cols - 1 && idx + 1 < count) t = idx + 1;
            else if (((row + 1) % rows) * cols < count) t = ((row + 1) % rows) * cols;
            break;
        case ORDER_NAV_LEFT:
            if (col > 0) t = idx - 1;
            break;
        case ORDER_NAV_DOWN:
            t = ((row + 1) % rows) * cols + col;
            break;
        case ORDER_NAV_UP:
            t = ((row - 1 + rows) % rows) * cols + col;
            break;
    }
    return t >= count ? count - 1 : t;
}

void Order_Move(void *base, size_t size, int count, int from, int to)
{
    unsigned char tmp[256];
    unsigned char *b = base;
    if (!b || size == 0 || size > sizeof(tmp) || from < 0 || to < 0
        || from >= count || to >= count || from == to) return;
    memcpy(tmp, b + (size_t)from * size, size);
    if (from < to) memmove(b + (size_t)from * size, b + (size_t)(from + 1) * size, (size_t)(to - from) * size);
    else memmove(b + (size_t)(to + 1) * size, b + (size_t)to * size, (size_t)(from - to) * size);
    memcpy(b + (size_t)to * size, tmp, size);
}

void Order_Perm(const char *text, const char *const *titles, int count, int *perm)
{
    unsigned char used[256] = {0};
    int k = 0;
    if (!perm || count <= 0) return;
    if (count > 256) count = 256;
    for (const char *p = text; p && *p && titles;) {
        const char *e = strchr(p, '\n');
        size_t len = e ? (size_t)(e - p) : strlen(p);
        while (len && p[len - 1] == '\r') len--;
        for (int i = 0; i < count; i++) {
            if (!used[i] && titles[i] && strlen(titles[i]) == len && strncmp(titles[i], p, len) == 0) {
                used[i] = 1;
                perm[k++] = i;
                break;
            }
        }
        p = e ? e + 1 : NULL;
    }
    for (int i = 0; i < count; i++) if (!used[i]) perm[k++] = i;
}

int Order_Format(char *dst, size_t n, const char *const *titles, int count)
{
    size_t off = 0;
    if (!dst || n == 0) return -1;
    dst[0] = '\0';
    for (int i = 0; i < count; i++) {
        int w = snprintf(dst + off, n - off, "%s\n", (titles && titles[i]) ? titles[i] : "");
        if (w < 0 || (size_t)w >= n - off) { dst[0] = '\0'; return -1; }
        off += (size_t)w;
    }
    return 0;
}

#endif /* ORDER_IMPLEMENTATION_ONCE */
#endif /* ORDER_IMPLEMENTATION */
