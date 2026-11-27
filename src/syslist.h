/* syslist.h - Scrapable artbox titles (USA / Japan) from a Nintendo Wiki
 * list, stored in assets/data/<system>-titles.txt. Pure C, no raylib:
 * unit-tested in sneslist_test.c. Shared by SNES and GameCube. */
#ifndef NSO_SYSLIST_H
#define NSO_SYSLIST_H

#include <stdbool.h>
#include <stddef.h>

#define SYSLIST_MAX 4096
#define SYSLIST_NAME 160

typedef enum {
    SYSLIST_ALL = 0,
    SYSLIST_USA,
    SYSLIST_JP
} SysListRegion;

typedef struct {
    char title[SYSLIST_NAME];
    char alias[SYSLIST_NAME]; /* other region's name, or empty */
    char boxartUsa[SYSLIST_NAME]; /* exact libretro Named_Boxarts filename, or empty */
    char boxartJp[SYSLIST_NAME];
    bool usa, jp;
} SysListGame;

/* Parse the data file (comments start with #). Returns how many games, or
 * -1 if a line is malformed. */
int SysList_Parse(const char *text, SysListGame *out, int cap);
bool SysList_InRegion(const SysListGame *g, SysListRegion r);
/* idx[k] = index into list of the k-th title in this region. */
int SysList_Filter(const SysListGame *list, int n, SysListRegion r,
                   int *idx, int cap);
/* Exact Named_Boxarts filename for this title, preferring region r
 * (USA/All → USA file then Japan; Japan → Japan file then USA). Empty if none. */
const char *SysList_Boxart(const SysListGame *g, SysListRegion r);
/* Cover filename stem for a title (lowercase, dashes). */
void SysList_Slug(const char *title, char *dst, size_t n);

#endif /* NSO_SYSLIST_H */

#ifdef SYSLIST_IMPLEMENTATION
#ifndef SYSLIST_IMPLEMENTATION_ONCE
#define SYSLIST_IMPLEMENTATION_ONCE

#include <ctype.h>
#include <string.h>

bool SysList_InRegion(const SysListGame *g, SysListRegion r)
{
    if (!g) return false;
    if (r == SYSLIST_USA) return g->usa;
    if (r == SYSLIST_JP) return g->jp;
    return true;
}

const char *SysList_Boxart(const SysListGame *g, SysListRegion r)
{
    if (!g) return "";
    if (r == SYSLIST_JP)
        return g->boxartJp[0] ? g->boxartJp : g->boxartUsa;
    return g->boxartUsa[0] ? g->boxartUsa : g->boxartJp;
}

int SysList_Filter(const SysListGame *list, int n, SysListRegion r,
                   int *idx, int cap)
{
    int k = 0;
    if (!list || !idx || cap <= 0 || n <= 0) return 0;
    for (int i = 0; i < n && k < cap; i++) {
        if (SysList_InRegion(&list[i], r)) idx[k++] = i;
    }
    return k;
}

void SysList_Slug(const char *title, char *dst, size_t n)
{
    size_t o = 0;
    int dash = 0;
    if (!dst || n == 0) return;
    if (!title) { dst[0] = '\0'; return; }
    for (const unsigned char *s = (const unsigned char *)title; *s && o + 1 < n; s++) {
        unsigned char c = *s;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            dst[o++] = (char)c;
            dash = 0;
        } else if (o > 0 && !dash) {
            dst[o++] = '-';
            dash = 1;
        }
    }
    if (o > 0 && dst[o - 1] == '-') o--;
    dst[o] = '\0';
}

static void SysList_CopyField(char *dst, size_t n, const char *s, const char *end)
{
    size_t i = 0;
    if (!dst || n == 0) return;
    while (s < end && (*s == ' ' || *s == '\t')) s++;
    while (end > s && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r')) end--;
    for (; s < end && i + 1 < n; s++) dst[i++] = *s;
    dst[i] = '\0';
}

int SysList_Parse(const char *text, SysListGame *out, int cap)
{
    int n = 0;
    if (!text || !out || cap <= 0) return 0;
    for (const char *p = text; *p && n < cap; ) {
        const char *eol = p;
        SysListGame *g;
        const char *tab, *tab2, *flags, *fend, *title, *tend;
        bool usa = false, jp = false;
        while (*eol && *eol != '\n') eol++;
        /* skip comments and blank lines */
        const char *s = p;
        while (s < eol && (*s == ' ' || *s == '\t' || *s == '\r')) s++;
        if (s >= eol || *s == '#') {
            p = *eol ? eol + 1 : eol;
            continue;
        }
        tab = s;
        while (tab < eol && *tab != '\t') tab++;
        if (tab >= eol) return -1;
        flags = s;
        fend = tab;
        while (flags < fend) {
            char c = *flags++;
            if (c == 'U' || c == 'u') usa = true;
            else if (c == 'J' || c == 'j') jp = true;
            else if (c != ' ' && c != '\t' && c != '\r') return -1;
        }
        if (!usa && !jp) return -1;
        title = tab + 1;
        tab2 = title;
        while (tab2 < eol && *tab2 != '\t') tab2++;
        tend = tab2;
        g = &out[n];
        memset(g, 0, sizeof(*g));
        SysList_CopyField(g->title, sizeof(g->title), title, tend);
        if (g->title[0] == '\0') return -1;
        if (tab2 < eol) {
            const char *p3 = tab2 + 1, *tab3 = p3, *p4, *tab4;
            while (tab3 < eol && *tab3 != '\t') tab3++;
            SysList_CopyField(g->alias, sizeof(g->alias), p3, tab3);
            if (tab3 < eol) {
                p4 = tab3 + 1;
                tab4 = p4;
                while (tab4 < eol && *tab4 != '\t') tab4++;
                SysList_CopyField(g->boxartUsa, sizeof(g->boxartUsa), p4, tab4);
                if (tab4 < eol)
                    SysList_CopyField(g->boxartJp, sizeof(g->boxartJp), tab4 + 1, eol);
            }
        }
        g->usa = usa;
        g->jp = jp;
        n++;
        p = *eol ? eol + 1 : eol;
    }
    return n;
}

#endif /* SYSLIST_IMPLEMENTATION_ONCE */
#endif /* SYSLIST_IMPLEMENTATION */
