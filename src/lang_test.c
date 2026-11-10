/* lang_test.c - string table completeness + config fallback. */
#define _DEFAULT_SOURCE
#define LANG_IMPLEMENTATION
#include "lang.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static int count(const char *s, const char *sub) {
    int n = 0;
    for (const char *p = strstr(s, sub); p; p = strstr(p + 1, sub)) n++;
    return n;
}

int main(void)
{
    for (int l = 0; l < LANG_COUNT; l++) {
        for (int i = 0; i < S_COUNT; i++) {
            const char *s = nso_strings[l][i];
            if (!s || !s[0]) { fprintf(stderr, "lang %d id %d empty\n", l, i); fails++; continue; }
            /* Same printf arguments in every language (Msg_Format relies on it). */
            const char *en = nso_strings[LANG_EN][i];
            CHECK(count(s, "%") == count(en, "%"));
            CHECK(count(s, "%d") == count(en, "%d") && count(s, "%s") == count(en, "%s"));
        }
    }

    CHECK(Lang_Parse(NULL) == LANG_EN);
    CHECK(Lang_Parse("") == LANG_EN);
    CHECK(Lang_Parse("garbage") == LANG_EN);
    CHECK(Lang_Parse("language=fr\n") == LANG_EN);
    CHECK(Lang_Parse("language=ptx") == LANG_EN);
    CHECK(Lang_Parse("language=pt") == LANG_PT);
    CHECK(Lang_Parse("# nso\r\nlanguage=pt-BR\r\n") == LANG_PT);
    CHECK(Lang_Load("/nonexistent/nso.cfg") == LANG_EN);

    char dir[] = "/tmp/nso-lang-test-XXXXXX";
    char path[256];
    CHECK(mkdtemp(dir) != NULL);
    snprintf(path, sizeof(path), "%s/nso.cfg", dir);
    CHECK(Lang_Save(path, LANG_PT) && Lang_Load(path) == LANG_PT);
    CHECK(Lang_Save(path, LANG_EN) && Lang_Load(path) == LANG_EN);
    FILE *f = fopen(path, "w");
    if (f) { fputs("language=\n", f); fclose(f); }
    CHECK(Lang_Load(path) == LANG_EN);
    unlink(path);
    CHECK(rmdir(dir) == 0);

    NsoMsg m;
    char out[256];
    Msg_Clear(&m); Msg_Format(&m, out, sizeof(out)); CHECK(out[0] == '\0');
    Lang_Set(LANG_EN);
    Msg_Set(&m, S_SCRAPE_DONE_MISSED, 3, NULL); Msg_Format(&m, out, sizeof(out));
    CHECK(strcmp(out, "Done, 3 missed") == 0);
    Msg_Set(&m, S_REPO_ROM_NEEDED, 0, "sm.smc"); Msg_Format(&m, out, sizeof(out));
    CHECK(strcmp(out, "ROM needed: sm.smc") == 0);
    Lang_Set(LANG_PT);
    Msg_Format(&m, out, sizeof(out));
    CHECK(strcmp(out, "Precisa da ROM: sm.smc") == 0);   /* same msg re-renders */
    Msg_Set(&m, S_RAW, 0, "Super Metroid  1/3"); Msg_Format(&m, out, sizeof(out));
    CHECK(strcmp(out, "Super Metroid  1/3") == 0);
    Lang_Set((NsoLang)7);
    CHECK(Lang_Get() == LANG_EN);

    if (fails) return 1;
    puts("lang_test: ok");
    return 0;
}
