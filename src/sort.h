/* sort.h - game sort modes and played-time stats (pure C, no raylib). */
#ifndef NSO_SORT_H
#define NSO_SORT_H

/* Standalone pure-C tests: capacity fallback. main.c defines the real
 * MAX_GAMES from the catalog sizes before including this header. */
#ifndef MAX_GAMES
#define MAX_GAMES 4096
#endif


#include <stddef.h>

typedef enum { SORT_CUSTOM = 0, SORT_TITLE, SORT_RELEASE, SORT_PLAYED, SORT_TIME, SORT_COUNT } SortMode;

typedef struct { long long release; long long lastPlayed; long long playSecs; } SortKeys;  /* 0 = unknown */

const char *Sort_ModeName(SortMode m);
SortMode Sort_ModeParse(const char *s);
void Sort_Perm(SortMode mode, const char *const *titles, const SortKeys *keys, int count, int *perm);
void Sort_StatsParse(const char *text, const char *const *titles, SortKeys *keys, int count);
int Sort_StatsFormat(char *dst, size_t n, const char *const *titles, const SortKeys *keys, int count);
void Sort_DatesParse(const char *text, const char *const *titles, SortKeys *keys, int count);
int Sort_DatesFormat(char *dst, size_t n, const char *const *titles, const SortKeys *keys, int count);

#endif /* NSO_SORT_H */

#ifdef SORT_IMPLEMENTATION
#ifndef SORT_IMPLEMENTATION_ONCE
#define SORT_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char Sort_Lower(unsigned char c)
{
    return (c >= 'A' && c <= 'Z') ? (unsigned char)(c - 'A' + 'a') : c;
}

static int Sort_CiCmp(const char *a, const char *b)
{
    const unsigned char *pa = (const unsigned char *)(a ? a : "");
    const unsigned char *pb = (const unsigned char *)(b ? b : "");
    for (;;) {
        unsigned char ca = Sort_Lower(*pa), cb = Sort_Lower(*pb);
        if (ca != cb) return ca < cb ? -1 : 1;
        if (!*pa) return 0;
        pa++;
        pb++;
    }
}

static int Sort_TitleCmp(const char *a, const char *b)
{
    int c = Sort_CiCmp(a, b);
    if (c) return c;
    return strcmp(a ? a : "", b ? b : "");
}

static long long Sort_Key(const SortKeys *keys, int i, int which)
{
    if (!keys) return 0;
    if (which == 0) return keys[i].release;
    if (which == 1) return keys[i].lastPlayed;
    return keys[i].playSecs;
}

static int Sort_Before(SortMode mode, int a, int b, const char *const *titles, const SortKeys *keys)
{
    const char *ta = (titles && titles[a]) ? titles[a] : "";
    const char *tb = (titles && titles[b]) ? titles[b] : "";
    int c = 0;
    if (mode == SORT_RELEASE) {
        long long ka = Sort_Key(keys, a, 0), kb = Sort_Key(keys, b, 0);
        if (ka != kb) {
            if (ka == 0) return 1;
            if (kb == 0) return -1;
            c = ka < kb ? -1 : 1;
        }
    } else if (mode == SORT_PLAYED || mode == SORT_TIME) {
        int which = mode == SORT_PLAYED ? 1 : 2;
        long long ka = Sort_Key(keys, a, which), kb = Sort_Key(keys, b, which);
        if (ka != kb) {
            if (ka == 0) return 1;
            if (kb == 0) return -1;
            c = ka > kb ? -1 : 1;
        }
    }
    if (c) return c;
    if (mode == SORT_TITLE || mode == SORT_RELEASE || mode == SORT_PLAYED || mode == SORT_TIME) {
        c = Sort_TitleCmp(ta, tb);
        if (c) return c;
    }
    return a < b ? -1 : (a > b ? 1 : 0);
}

const char *Sort_ModeName(SortMode m)
{
    static const char *names[] = { "custom", "title", "release", "played", "time" };
    if ((int)m < 0 || m >= SORT_COUNT) return "custom";
    return names[m];
}

SortMode Sort_ModeParse(const char *s)
{
    if (!s) return SORT_CUSTOM;
    for (int i = 0; i < SORT_COUNT; i++)
        if (strcmp(s, Sort_ModeName((SortMode)i)) == 0) return (SortMode)i;
    return SORT_CUSTOM;
}

void Sort_Perm(SortMode mode, const char *const *titles, const SortKeys *keys, int count, int *perm)
{
    if (!perm || count <= 0) return;
    if (count > MAX_GAMES) count = MAX_GAMES;
    for (int i = 0; i < count; i++) perm[i] = i;
    if ((int)mode <= SORT_CUSTOM || mode >= SORT_COUNT) return;
    for (int i = 1; i < count; i++) {
        int key = perm[i];
        int j = i - 1;
        while (j >= 0 && Sort_Before(mode, perm[j], key, titles, keys) > 0) {
            perm[j + 1] = perm[j];
            j--;
        }
        perm[j + 1] = key;
    }
}

void Sort_StatsParse(const char *text, const char *const *titles, SortKeys *keys, int count)
{
    if (!text || !keys || count <= 0) return;
    if (count > MAX_GAMES) count = MAX_GAMES;
    for (const char *p = text; *p;) {
        const char *e = strchr(p, '\n');
        if (!e) e = p + strlen(p);
        const char *tab1 = NULL, *tab2 = NULL;
        for (const char *q = p; q < e; q++) {
            if (*q != '\t') continue;
            if (!tab1) tab1 = q;
            else { tab2 = q; break; }
        }
        if (tab1 && tab2) {
            long long lp = strtoll(p, NULL, 10);
            long long ps = strtoll(tab1 + 1, NULL, 10);
            const char *title = tab2 + 1;
            size_t len = (size_t)(e - title);
            while (len && title[len - 1] == '\r') len--;
            for (int i = 0; i < count; i++) {
                const char *t = (titles && titles[i]) ? titles[i] : "";
                if (strlen(t) == len && strncmp(t, title, len) == 0) {
                    keys[i].lastPlayed = lp;
                    keys[i].playSecs = ps;
                    break;
                }
            }
        }
        p = *e ? e + 1 : e;
    }
}

int Sort_StatsFormat(char *dst, size_t n, const char *const *titles, const SortKeys *keys, int count)
{
    size_t off = 0;
    if (!dst || n == 0) return -1;
    dst[0] = '\0';
    for (int i = 0; i < count; i++) {
        long long lp = Sort_Key(keys, i, 1);
        long long ps = Sort_Key(keys, i, 2);
        if (lp == 0 && ps == 0) continue;
        int seen = 0;   /* titles are keys: a repeated one is written once */
        for (int j = 0; j < i && titles; j++)
            if (titles[i] && titles[j] && strcmp(titles[i], titles[j]) == 0) { seen = 1; break; }
        if (seen) continue;
        const char *t = (titles && titles[i]) ? titles[i] : "";
        int w = snprintf(dst + off, n - off, "%lld\t%lld\t%s\n", lp, ps, t);
        if (w < 0 || (size_t)w >= n - off) { dst[0] = '\0'; return -1; }
        off += (size_t)w;
    }
    return 0;
}

void Sort_DatesParse(const char *text, const char *const *titles, SortKeys *keys, int count)
{
    if (!text || !keys || count <= 0) return;
    if (count > MAX_GAMES) count = MAX_GAMES;
    for (const char *p = text; *p;) {
        const char *e = strchr(p, '\n');
        const char *tab = NULL;
        if (!e) e = p + strlen(p);
        for (const char *q = p; q < e; q++) {
            if (*q == '\t') { tab = q; break; }
        }
        if (tab) {
            char *end = NULL;
            long long release = strtoll(p, &end, 10);
            if (end == tab) {
                const char *title = tab + 1;
                size_t len = (size_t)(e - title);
                while (len && title[len - 1] == '\r') len--;
                for (int i = 0; i < count; i++) {
                    const char *t = (titles && titles[i]) ? titles[i] : "";
                    if (strlen(t) == len && strncmp(t, title, len) == 0) {
                        keys[i].release = release;
                        break;
                    }
                }
            }
        }
        p = *e ? e + 1 : e;
    }
}

int Sort_DatesFormat(char *dst, size_t n, const char *const *titles, const SortKeys *keys, int count)
{
    size_t off = 0;
    if (!dst || n == 0) return -1;
    dst[0] = '\0';
    for (int i = 0; i < count; i++) {
        long long release = Sort_Key(keys, i, 0);
        if (release == 0) continue;
        int seen = 0;   /* titles are keys: a repeated one is written once */
        for (int j = 0; j < i && titles; j++)
            if (titles[i] && titles[j] && strcmp(titles[i], titles[j]) == 0) { seen = 1; break; }
        if (seen) continue;
        const char *t = (titles && titles[i]) ? titles[i] : "";
        int w = snprintf(dst + off, n - off, "%lld\t%s\n", release, t);
        if (w < 0 || (size_t)w >= n - off) { dst[0] = '\0'; return -1; }
        off += (size_t)w;
    }
    return 0;
}

#endif /* SORT_IMPLEMENTATION_ONCE */
#endif /* SORT_IMPLEMENTATION */
