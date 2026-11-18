/* hide_test.c - Hide unplayable: filtering, cursor, order interplay, config. */
#define ORDER_IMPLEMENTATION
#define SORT_IMPLEMENTATION
#define HIDE_IMPLEMENTATION
#include "hide.h"
#include "sort.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    {   /* playable first, both groups keep their order */
        bool ok[5] = { false, true, false, true, true };
        int p[5];
        CHECK(Hide_Perm(ok, NULL, 5, p) == 3);
        CHECK(p[0] == 1 && p[1] == 3 && p[2] == 4 && p[3] == 0 && p[4] == 2);
    }
    {   /* everything playable, or nothing playable */
        bool all[3] = { true, true, true }, none[3] = { false, false, false };
        int p[3];
        CHECK(Hide_Perm(all, NULL, 3, p) == 3);
        CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2);
        CHECK(Hide_Perm(none, NULL, 3, p) == 0);
        CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2);
        CHECK(Hide_Perm(NULL, NULL, 3, p) == 0);
        CHECK(Hide_Perm(all, NULL, 0, p) == 0);
    }
    {   /* with a sort: sort first, then the playable ones lead */
        const char *t[4] = { "delta", "alpha", "charlie", "bravo" };
        bool ok[4] = { true, false, true, true };   /* alpha is not playable */
        int sorted[4], p[4];
        Sort_Perm(SORT_TITLE, t, NULL, 4, sorted);   /* alpha bravo charlie delta */
        CHECK(Hide_Perm(ok, sorted, 4, p) == 3);
        CHECK(p[0] == 3 && p[1] == 2 && p[2] == 0 && p[3] == 1);
        SortKeys k[4] = { {0, 0, 0}, {199001, 0, 0}, {199505, 0, 0}, {0, 0, 0} };
        Sort_Perm(SORT_RELEASE, t, k, 4, sorted);    /* alpha charlie bravo delta */
        CHECK(Hide_Perm(ok, sorted, 4, p) == 3);
        CHECK(p[0] == 2 && p[1] == 3 && p[2] == 0 && p[3] == 1);
    }
    {   /* cursor clamp when the list shrinks */
        CHECK(Hide_Clamp(2, 5) == 2);
        CHECK(Hide_Clamp(7, 3) == 2);
        CHECK(Hide_Clamp(3, 3) == 2);
        CHECK(Hide_Clamp(4, 0) == 0);
        CHECK(Hide_Clamp(-1, 3) == 0);
    }
    {   /* saved custom order: hidden titles keep their slots */
        const char *t[4] = { "c", "a", "d", "b" };   /* shown: c a ; hidden: d b */
        const char *out[4];
        Hide_MergeOrder("a\nb\nc\nd\n", t, 4, 2, out);   /* old: a b c d */
        CHECK(strcmp(out[0], "c") == 0 && strcmp(out[1], "b") == 0);
        CHECK(strcmp(out[2], "a") == 0 && strcmp(out[3], "d") == 0);
        Hide_MergeOrder(NULL, t, 4, 2, out);   /* no file: catalog order c a d b */
        CHECK(strcmp(out[0], "c") == 0 && strcmp(out[1], "a") == 0);
        CHECK(strcmp(out[2], "d") == 0 && strcmp(out[3], "b") == 0);
        Hide_MergeOrder("a\nb\nc\nd\n", t, 4, 4, out);   /* nothing hidden: new order wins */
        CHECK(strcmp(out[0], "c") == 0 && strcmp(out[1], "a") == 0);
        CHECK(strcmp(out[2], "d") == 0 && strcmp(out[3], "b") == 0);
        Hide_MergeOrder("a\nb\nc\nd\n", t, 4, 0, out);   /* all hidden: old order kept */
        CHECK(strcmp(out[0], "a") == 0 && strcmp(out[3], "d") == 0);
    }
    {   /* config value: default off, only "1" is on */
        CHECK(strcmp(Hide_CfgValue(true), "1") == 0 && strcmp(Hide_CfgValue(false), "0") == 0);
        CHECK(Hide_CfgParse("1") && !Hide_CfgParse("0") && !Hide_CfgParse("") && !Hide_CfgParse(NULL));
    }
    return fails ? 1 : 0;
}
