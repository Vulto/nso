/* fav_test.c - favorite title list: has/toggle/write, marks, config. */
#define FAV_IMPLEMENTATION
#define HIDE_IMPLEMENTATION
#define ORDER_IMPLEMENTATION
#include "fav.h"
#include "hide.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    {   /* empty / missing */
        CHECK(!Fav_Has(NULL, "A") && !Fav_Has("", "A") && !Fav_Has("A\n", NULL));
        CHECK(!Fav_Has("A\n", "") && !Fav_Has("AB\n", "A") && !Fav_Has("A\n", "AB"));
        CHECK(Fav_Has("A\n", "A") && Fav_Has("B\r\nA\n", "A") && Fav_Has("A", "A"));
    }
    {   /* write add/remove keeps unknown lines, drops duplicates */
        char out[128];
        CHECK(Fav_Write(NULL, "Mario", true, out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Mario\n") == 0);
        CHECK(Fav_Write("Zelda\nUnknown\n", "Mario", true, out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Zelda\nUnknown\nMario\n") == 0);
        CHECK(Fav_Write("Mario\nZelda\nMario\n", "Mario", true, out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Mario\nZelda\n") == 0);
        CHECK(Fav_Write("Mario\nZelda\n", "Mario", false, out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Zelda\n") == 0);
        CHECK(Fav_Write("Zelda\n", "Mario", false, out, sizeof(out)) == 0);
        CHECK(strcmp(out, "Zelda\n") == 0);
        CHECK(Fav_Write("Mario\n", NULL, true, out, sizeof(out)) == 0);
        CHECK(out[0] == '\0');
    }
    {   /* toggle and overflow */
        char out[16];
        bool on = false;
        CHECK(Fav_Toggle("", "A", out, sizeof(out), &on) == 0 && on && strcmp(out, "A\n") == 0);
        CHECK(Fav_Toggle("A\n", "A", out, sizeof(out), &on) == 0 && !on && out[0] == '\0');
        CHECK(Fav_Toggle("A\n", "AVeryLongTitle", out, 8, &on) == -1 && !on);
        CHECK(Fav_Write("Keep\n", "TooLongTitle", true, out, 8) == -1 && out[0] == '\0');
    }
    {   /* marks */
        const char *t[4] = { "delta", "alpha", "charlie", "bravo" };
        bool fav[4];
        CHECK(Fav_Marks("alpha\nbravo\nmissing\n", t, 4, fav) == 2);
        CHECK(!fav[0] && fav[1] && !fav[2] && fav[3]);
        CHECK(Fav_Marks("", t, 4, fav) == 0);
        CHECK(!fav[0] && !fav[1] && !fav[2] && !fav[3]);
    }
    {   /* compose with Hide_Perm: favorites (and playable) lead */
        const char *t[4] = { "d", "a", "c", "b" };
        bool fav[4], ok[4];
        int order[4] = { 0, 1, 2, 3 }, p[4];
        Fav_Marks("a\nc\n", t, 4, fav);
        bool playable[4] = { true, true, false, true };  /* c is not playable */
        for (int i = 0; i < 4; i++) ok[i] = fav[i] && playable[i];
        CHECK(Hide_Perm(ok, order, 4, p) == 1);
        CHECK(p[0] == 1);  /* only a is a playable favorite */
    }
    {   /* config value: default off, only "1" is on */
        CHECK(strcmp(Fav_CfgValue(true), "1") == 0 && strcmp(Fav_CfgValue(false), "0") == 0);
        CHECK(Fav_CfgParse("1") && !Fav_CfgParse("0") && !Fav_CfgParse("") && !Fav_CfgParse(NULL));
    }
    return fails ? 1 : 0;
}
