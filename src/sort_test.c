/* sort_test.c - sort modes, permutations, and played-time stats. */
#define SORT_IMPLEMENTATION
#include "sort.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    {
        const char *t[4] = { "banana", "Apple", "cherry", "apple" };
        int p[4];
        Sort_Perm(SORT_TITLE, t, NULL, 4, p);
        CHECK(p[0] == 1 && p[1] == 3 && p[2] == 0 && p[3] == 2);
        const char *n[3] = { "b", NULL, "A" };
        Sort_Perm(SORT_TITLE, n, NULL, 3, p);
        CHECK(p[0] == 1 && p[1] == 2 && p[2] == 0);
    }
    {
        const char *t[4] = { "b", "a", "c", "d" };
        SortKeys k[4] = { {0, 0, 0}, {5, 0, 0}, {0, 0, 0}, {2, 0, 0} };
        int p[4];
        Sort_Perm(SORT_RELEASE, t, k, 4, p);
        CHECK(p[0] == 3 && p[1] == 1 && p[2] == 0 && p[3] == 2);
        SortKeys z[4] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
        Sort_Perm(SORT_RELEASE, t, z, 4, p);
        CHECK(p[0] == 1 && p[1] == 0 && p[2] == 2 && p[3] == 3);
        Sort_Perm(SORT_RELEASE, t, NULL, 4, p);
        CHECK(p[0] == 1 && p[1] == 0 && p[2] == 2 && p[3] == 3);
    }
    {
        const char *t[4] = { "a", "b", "c", "d" };
        SortKeys k[4] = { {0, 10, 0}, {0, 0, 0}, {0, 10, 0}, {0, 5, 0} };
        int p[4];
        Sort_Perm(SORT_PLAYED, t, k, 4, p);
        CHECK(p[0] == 0 && p[1] == 2 && p[2] == 3 && p[3] == 1);
    }
    {
        const char *t[4] = { "a", "b", "c", "d" };
        SortKeys k[4] = { {0, 0, 100}, {0, 0, 0}, {0, 0, 300}, {0, 0, 100} };
        int p[4];
        Sort_Perm(SORT_TIME, t, k, 4, p);
        CHECK(p[0] == 2 && p[1] == 0 && p[2] == 3 && p[3] == 1);
    }
    {
        const char *t[3] = { "c", "b", "a" };
        int p[3];
        Sort_Perm(SORT_CUSTOM, t, NULL, 3, p);
        CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2);
        Sort_Perm((SortMode)SORT_COUNT, t, NULL, 3, p);
        CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2);
        Sort_Perm((SortMode)99, t, NULL, 3, p);
        CHECK(p[0] == 0 && p[1] == 1 && p[2] == 2);
    }
    {
        static const char *names[] = { "custom", "title", "release", "played", "time" };
        for (int i = 0; i < SORT_COUNT; i++) {
            CHECK(strcmp(Sort_ModeName((SortMode)i), names[i]) == 0);
            CHECK(Sort_ModeParse(names[i]) == (SortMode)i);
        }
        CHECK(strcmp(Sort_ModeName(SORT_COUNT), "custom") == 0);
        CHECK(strcmp(Sort_ModeName((SortMode)99), "custom") == 0);
        CHECK(Sort_ModeParse(NULL) == SORT_CUSTOM);
        CHECK(Sort_ModeParse("bogus") == SORT_CUSTOM);
    }
    {
        const char *t[3] = { "Alpha", "Beta", "Gamma" };
        SortKeys k[3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
        Sort_StatsParse("123\t45\tBeta\n999\t1\tUnknown\n7\t8\tAlpha\n", t, k, 3);
        CHECK(k[0].lastPlayed == 7 && k[0].playSecs == 8);
        CHECK(k[1].lastPlayed == 123 && k[1].playSecs == 45);
        CHECK(k[2].lastPlayed == 0 && k[2].playSecs == 0);
        char buf[128];
        CHECK(Sort_StatsFormat(buf, sizeof(buf), t, k, 3) == 0);
        CHECK(strcmp(buf, "7\t8\tAlpha\n123\t45\tBeta\n") == 0);
        SortKeys k2[3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
        Sort_StatsParse(buf, t, k2, 3);
        CHECK(k2[0].lastPlayed == 7 && k2[0].playSecs == 8);
        CHECK(k2[1].lastPlayed == 123 && k2[1].playSecs == 45);
        CHECK(k2[2].lastPlayed == 0 && k2[2].playSecs == 0);
        char small[8];
        CHECK(Sort_StatsFormat(small, sizeof(small), t, k, 3) == -1);
        CHECK(small[0] == '\0');
    }
    {
        const char *t[3] = { "Alpha", "Beta", "Gamma" };
        SortKeys k[3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
        Sort_DatesParse("199806\tBeta\r\nnope\nabc\tGamma\n199912\tAlpha\n", t, k, 3);
        CHECK(k[0].release == 199912);
        CHECK(k[1].release == 199806);
        CHECK(k[2].release == 0);
        char buf[128];
        CHECK(Sort_DatesFormat(buf, sizeof(buf), t, k, 3) == 0);
        CHECK(strcmp(buf, "199912\tAlpha\n199806\tBeta\n") == 0);
        SortKeys k2[3] = { {0, 0, 0}, {0, 0, 0}, {0, 0, 0} };
        Sort_DatesParse(buf, t, k2, 3);
        CHECK(k2[0].release == 199912);
        CHECK(k2[1].release == 199806);
        CHECK(k2[2].release == 0);
        char small[8];
        CHECK(Sort_DatesFormat(small, sizeof(small), t, k, 3) == -1);
        CHECK(small[0] == '\0');
    }
    if (fails) return 1;
    return 0;
}
