/* Match catalog titles to libretro Named_Boxarts filenames. */
#ifndef NSO_SCRAPE_H
#define NSO_SCRAPE_H

#include <stddef.h>

const char *Scrape_Playlist(const char *id);
void Scrape_Repo(char *dst, size_t n, const char *playlist);
int Scrape_IndexUrl(char *dst, size_t n, const char *playlist);
int Scrape_ImageUrl(char *dst, size_t n, const char *playlist, const char *file);
const char *Scrape_Match(const char *title, const char **names, int count);
const char *Scrape_MatchLoose(const char *title, const char **names, int count);
/* Distinct box-art files for the same game (same core title, other regions
 * or revisions). 0 if nothing matches; 1 means there is no choice to make. */
int Scrape_CollectMatches(const char *title, const char **names, int count,
                          const char **out, int maxOut);
int Scrape_ParseIndex(const char *json, char ***outNames, int *outCount);
int Scrape_DatUrl(char *dst, size_t n, const char *field, const char *playlist);
int Scrape_ParseDat(const char *text, const char *field, char ***outNames, int **outVals, int *outCount);
void Scrape_FreeNames(char **names, int count);
/* True when path starts with a PNG or JPEG magic number. */
int Scrape_LooksLikeImage(const char *path);
/* If path is a short text alias whose body is a Named_Boxarts basename
 * (no slashes), copy that name into dst and return 1. Else 0. */
int Scrape_AliasTarget(const char *path, char *dst, size_t n);

#endif /* NSO_SCRAPE_H */

#ifdef SCRAPE_IMPLEMENTATION
#ifndef SCRAPE_IMPLEMENTATION_ONCE
#define SCRAPE_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *Scrape_Playlist(const char *id)
{
    if (!id) return NULL;
    if (strcmp(id, "nes") == 0) return "Nintendo - Nintendo Entertainment System";
    if (strcmp(id, "snes") == 0) return "Nintendo - Super Nintendo Entertainment System";
    if (strcmp(id, "n64") == 0) return "Nintendo - Nintendo 64";
    if (strcmp(id, "gamecube") == 0) return "Nintendo - GameCube";
    if (strcmp(id, "megadrive") == 0) return "Sega - Mega Drive - Genesis";
    return NULL;
}

void Scrape_Repo(char *dst, size_t n, const char *playlist)
{
    size_t u = 0;
    if (!dst || n == 0) return;
    if (!playlist) {
        dst[0] = '\0';
        return;
    }
    for (const char *s = playlist; *s && u + 1 < n; s++) {
        dst[u++] = (*s == ' ') ? '_' : *s;
    }
    dst[u] = '\0';
}

static int UrlEncode(char *dst, size_t n, const char *src)
{
    size_t u = 0;
    for (const unsigned char *s = (const unsigned char *)src; *s; s++) {
        int plain = (*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')
            || (*s >= '0' && *s <= '9') || *s == '-' || *s == '_' || *s == '.' || *s == '~';
        if (u + (plain ? 1 : 3) >= n) return -1;
        if (plain) {
            dst[u++] = (char)*s;
        } else {
            snprintf(dst + u, n - u, "%%%02X", *s);
            u += 3;
        }
    }
    dst[u] = '\0';
    return 0;
}

int Scrape_IndexUrl(char *dst, size_t n, const char *playlist)
{
    char repo[256];
    int wrote;
    Scrape_Repo(repo, sizeof(repo), playlist);
    wrote = snprintf(dst, n,
        "https://api.github.com/repos/libretro-thumbnails/%s/git/trees/HEAD?recursive=1",
        repo);
    return (wrote > 0 && (size_t)wrote < n) ? 0 : -1;
}

int Scrape_ImageUrl(char *dst, size_t n, const char *playlist, const char *file)
{
    char repo[256];
    char encFile[1024];
    int wrote;
    if (!playlist || !file) return -1;
    if (UrlEncode(encFile, sizeof(encFile), file) != 0) return -1;
    /* The GitHub tree index (Scrape_IndexUrl) lists the per-system
     * libretro-thumbnails repo. The monorepo is a partial/stale subset, so
     * every system uses that same per-system raw URL. */
    Scrape_Repo(repo, sizeof(repo), playlist);
    wrote = snprintf(dst, n,
        "https://raw.githubusercontent.com/libretro-thumbnails/%s/master/Named_Boxarts/%s",
        repo, encFile);
    return (wrote > 0 && (size_t)wrote < n) ? 0 : -1;
}

/* ", The" / ", A" article form used by libretro names (before a subtitle,
   or at end): the article's length after the spaces, or 0. */
static int Scrape_TheArticleDash(const unsigned char *s)
{
    int len;
    while (*s == ' ' || *s == '\t') s++;
    if ((s[0] == 't' || s[0] == 'T') && (s[1] == 'h' || s[1] == 'H') && (s[2] == 'e' || s[2] == 'E'))
        len = 3;
    else if (s[0] == 'a' || s[0] == 'A')
        len = 1;
    else
        return 0;
    s += len;
    if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
        (*s >= '0' && *s <= '9'))
        return 0;
    while (*s == ' ' || *s == '\t') s++;
    return (*s == '-' || *s == '\0') ? len : 0;
}


/* "versus"→"vs", drop "and", roman numerals → digits so "Streets of Rage II"
 * matches a "streets-of-rage-2" stem and "Bulls versus Blazers" matches "vs". */
static void Scrape_FoldWords(char *s)
{
    static const char *const roman[] = { "ii", "iii", "iv", "vi", "vii", "viii", "ix" };
    static const char *const digit[] = { "2", "3", "4", "6", "7", "8", "9" };
    char out[512];
    size_t u = 0;
    const char *p;
    if (!s) return;
    for (p = s; *p; ) {
        const char *start = p;
        char word[128];
        const char *outw;
        size_t wlen;
        while (*p && *p != ' ') p++;
        wlen = (size_t)(p - start);
        if (wlen >= sizeof(word)) wlen = sizeof(word) - 1;
        memcpy(word, start, wlen);
        word[wlen] = '\0';
        outw = word;
        if (strcmp(word, "versus") == 0) outw = "vs";
        else if (strcmp(word, "taisen") == 0) outw = "wars";
        else if (strcmp(word, "and") == 0) outw = NULL;
        else {
            size_t k;
            for (k = 0; k < sizeof roman / sizeof roman[0]; k++)
                if (strcmp(word, roman[k]) == 0) { outw = digit[k]; break; }
        }
        if (outw && outw[0] && u + 2 + strlen(outw) < sizeof(out)) {
            if (u) out[u++] = ' ';
            memcpy(out + u, outw, strlen(outw));
            u += strlen(outw);
        }
        while (*p == ' ') p++;
    }
    out[u] = '\0';
    memcpy(s, out, u + 1);
}

/* Latin-1 accents fold to ASCII so "Pokémon" matches "Pokemon".
   A leading "the " / "a " and the ", The" / ", A" article (before a " - "
   subtitle, or trailing at the end of the name) are dropped so "The Legend
   of Zelda" matches "Legend of Zelda, The", "A Bug's Life" matches "Bug's
   Life, A", and "The Legend of Zelda: A Link to the Past" matches "Legend of
   Zelda, The - A Link to the Past". */
static void Scrape_Normalize(char *dst, size_t n, const char *src)
{
    size_t u = 0;
    int gap = 1;
    const unsigned char *s = (const unsigned char *)(src ? src : "");
    if (!dst || n == 0) return;
    while (*s && u + 1 < n) {
        unsigned char c = *s++;
        int article = c == ',' ? Scrape_TheArticleDash(s) : 0;
        if (article) {
            while (*s == ' ' || *s == '\t') s++;
            s += article; /* "the" / "a" */
            continue;
        }
        if (c == '\'' || c == '`') continue;
        if (c == 0xC3 && *s) {
            unsigned char nch = *s;
            char folded = 0;
            if (nch == 0xA9 || nch == 0x89 || nch == 0xA8 || nch == 0x88) folded = 'e';
            else if (nch == 0xA1 || nch == 0x81 || nch == 0xA0 || nch == 0x80) folded = 'a';
            else if (nch == 0xAD || nch == 0x8D) folded = 'i';
            else if (nch == 0xB3 || nch == 0x93) folded = 'o';
            else if (nch == 0xBA || nch == 0x9A) folded = 'u';
            else if (nch == 0xB1 || nch == 0x91) folded = 'n';
            if (folded) {
                s++;
                c = (unsigned char)folded;
            }
        } else if (c == 0xC4 && *s) {   /* macron vowels: ā ē ī */
            unsigned char nch = *s;
            char folded = 0;
            if (nch == 0x81 || nch == 0x80) folded = 'a';
            else if (nch == 0x93 || nch == 0x92) folded = 'e';
            else if (nch == 0xAB || nch == 0xAA) folded = 'i';
            if (folded) {
                s++;
                c = (unsigned char)folded;
            }
        } else if (c == 0xC5 && *s) {   /* macron vowels: ō ū */
            unsigned char nch = *s;
            char folded = 0;
            if (nch == 0x8D || nch == 0x8C) folded = 'o';
            else if (nch == 0xAB || nch == 0xAA) folded = 'u';
            if (folded) {
                s++;
                c = (unsigned char)folded;
            }
        }
        /* combining macron on a vowel we already kept */
        if (((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) && s[0] == 0xCC && s[1] == 0x84)
            s += 2;
        if (c >= 'A' && c <= 'Z') c = (unsigned char)(c - 'A' + 'a');
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) {
            dst[u++] = (char)c;
            gap = 0;
        } else if (!gap && u + 1 < n) {
            dst[u++] = ' ';
            gap = 1;
        }
    }
    if (u > 0 && dst[u - 1] == ' ') u--;
    dst[u] = '\0';
    if (strncmp(dst, "the ", 4) == 0)
        memmove(dst, dst + 4, strlen(dst + 4) + 1);
    else if (strncmp(dst, "a ", 2) == 0)
        memmove(dst, dst + 2, strlen(dst + 2) + 1);
    Scrape_FoldWords(dst);
    /* Hepburn "ou"/"uu" (Momotarou, Yakyuu) fold to o/u, matching macron forms. */
    {
        char ou[512];
        size_t i = 0, v = 0;
        for (; dst[i] && v + 1 < sizeof(ou); ) {
            if (dst[i] == 'o' && dst[i + 1] == 'u') { ou[v++] = 'o'; i += 2; }
            else if (dst[i] == 'u' && dst[i + 1] == 'u') { ou[v++] = 'u'; i += 2; }
            else ou[v++] = dst[i++];
        }
        ou[v] = '\0';
        memcpy(dst, ou, v + 1);
    }
}

static void Scrape_Stem(char *dst, size_t n, const char *filename)
{
    const char *paren;
    const char *ext;
    size_t len;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!filename) return;
    paren = strstr(filename, " (");
    len = paren ? (size_t)(paren - filename) : strlen(filename);
    ext = strrchr(filename, '.');
    if (!paren && ext && (size_t)(ext - filename) < len) len = (size_t)(ext - filename);
    if (len >= n) len = n - 1;
    memcpy(dst, filename, len);
    dst[len] = '\0';
}

static int EndsWithWord(const char *s, const char *t)
{
    size_t ns = strlen(s);
    size_t nt = strlen(t);
    if (nt == 0 || ns < nt) return 0;
    if (ns == nt) return strcmp(s, t) == 0;
    if (s[ns - nt - 1] != ' ') return 0;
    return memcmp(s + (ns - nt), t, nt) == 0;
}

static int RegionRank(const char *filename)
{
    if (strstr(filename, "(USA")) return 4;
    if (strstr(filename, "(World")) return 3;
    if (strstr(filename, "(Europe")) return 2;
    if (strstr(filename, "(Japan")) return 1;
    return 0;
}

/* Prefer a retail dump over a beta, sample, mini, or collection scan. */
static int ReleaseRank(const char *filename)
{
    if (!filename) return 0;
    if (strstr(filename, "(Beta") || strstr(filename, "(Sample")
        || strstr(filename, "(Proto"))
        return -3;
    if (strstr(filename, "Mega Drive Mini") || strstr(filename, "Genesis Mini")
        || strstr(filename, "Sega Channel"))
        return -2;
    if (strstr(filename, "Collection") || strstr(filename, "(Aftermarket")
        || strstr(filename, "[h ") || strstr(filename, "(Hack"))
        return -1;
    return 0;
}

static void Scrape_Compact(char *dst, size_t n, const char *src)
{
    size_t u = 0;
    if (!dst || n == 0) return;
    for (const char *s = src ? src : ""; *s && u + 1 < n; s++) {
        if (*s != ' ') dst[u++] = *s;
    }
    dst[u] = '\0';
}

static int StartsWithWord(const char *s, const char *t)
{
    size_t ns, nt;
    if (!s || !t) return 0;
    ns = strlen(s);
    nt = strlen(t);
    if (nt == 0 || ns < nt) return 0;
    if (ns == nt) return strcmp(s, t) == 0;
    return memcmp(s, t, nt) == 0 && s[nt] == ' ';
}

/* Remainder after a prefix is a sequel number ("2", "II" already folded to "2"). */
static int Scrape_SequelTail(const char *s, const char *prefix)
{
    static const char *const seq[] = {
        "2", "3", "4", "5", "6", "7", "8", "9", "10", "v"
    };
    const char *r;
    size_t n;
    if (!StartsWithWord(s, prefix) || strcmp(s, prefix) == 0) return 0;
    n = strlen(prefix);
    r = s + n;
    while (*r == ' ') r++;
    for (size_t i = 0; i < sizeof seq / sizeof seq[0]; i++) {
        size_t L = strlen(seq[i]);
        if (strncmp(r, seq[i], L) == 0 && (r[L] == '\0' || r[L] == ' ')) return 1;
    }
    return 0;
}

/* Higher is a tighter name fit. Prefix (title + subtitle, or a shorter
 * local stem) outranks a suffix ("All-Star Baseball 2003" ending in
 * "Baseball 2003", or a publisher stuck on the front). */
static int Scrape_FitRank(const char *stem, const char *title)
{
    if (!stem || !title || !stem[0] || !title[0]) return -1;
    if (strcmp(stem, title) == 0) return 4;
    if ((StartsWithWord(stem, title) && !Scrape_SequelTail(stem, title))
        || (StartsWithWord(title, stem) && !Scrape_SequelTail(title, stem)))
        return 3;
    if (EndsWithWord(stem, title) || EndsWithWord(title, stem)) return 1;
    return -1;
}

static int Scrape_AddUnique(char parts[][512], int n, int cap, const char *s)
{
    if (!s || !s[0] || n >= cap) return n;
    while (*s == ' ') s++;
    if (!s[0]) return n;
    for (int i = 0; i < n; i++) if (strcmp(parts[i], s) == 0) return n;
    snprintf(parts[n], 512, "%s", s);
    return n + 1;
}

static int Scrape_WordCount(const char *s)
{
    char n[512];
    int w = 0, i;
    Scrape_Normalize(n, sizeof(n), s);
    for (i = 0; n[i]; i++)
        if ((i == 0 || n[i - 1] == ' ') && n[i] != ' ') w++;
    return w;
}

/* "Bare Knuckle ~ Streets of Rage": try the whole string and each side. */
static int Scrape_TitleParts(const char *title, char parts[][512], int cap)
{
    int n = 0;
    const char *p, *q;
    if (!title || cap <= 0) return 0;
    n = Scrape_AddUnique(parts, n, cap, title);
    p = title;
    while ((q = strstr(p, "~")) != NULL) {
        char chunk[512];
        size_t len = (size_t)(q - p);
        while (len && (p[len - 1] == ' ' || p[len - 1] == '~')) len--;
        if (len && len < sizeof(chunk)) {
            memcpy(chunk, p, len);
            chunk[len] = '\0';
            n = Scrape_AddUnique(parts, n, cap, chunk);
        }
        p = q + 1;
        while (*p == ' ' || *p == '~') p++;
    }
    if (p != title) n = Scrape_AddUnique(parts, n, cap, p);
    /* Combo discs ("Wind Waker / Metroid Prime") and a long colon subtitle
     * ("Sea World: Shamu's Deep Sea Adventures", "Ty 3: Night of the Quinkan").
     * Short right-hand bits ("RTS") stay out so they cannot steal a match. */
    {
        static const char *const mid[] = { " / ", ": " };
        int d;
        for (d = 0; d < 2; d++) {
            const char *hit, *from = title;
            size_t m = strlen(mid[d]);
            while ((hit = strstr(from, mid[d])) != NULL) {
                char left[512], right[512];
                size_t L = (size_t)(hit - title);
                const char *r = hit + m;
                int words = 0, i;
                if (L && L < sizeof(left)) {
                    memcpy(left, title, L);
                    while (L && left[L - 1] == ' ') L--;
                    left[L] = '\0';
                    /* A short colon left ("The Legend of Zelda") is a series
                     * name and would match every sequel; require 4+ words. */
                    if (d == 0 || Scrape_WordCount(left) >= 4)
                        n = Scrape_AddUnique(parts, n, cap, left);
                }
                snprintf(right, sizeof(right), "%s", r);
                if (d == 0) n = Scrape_AddUnique(parts, n, cap, right);
                else {
                    for (i = 0; right[i]; i++)
                        if ((i == 0 || right[i - 1] == ' ') && right[i] != ' ') words++;
                    if (words >= 3 || strlen(right) >= 16)
                        n = Scrape_AddUnique(parts, n, cap, right);
                }
                from = hit + m;
            }
        }
    }
    return n;
}

/* "Ultimate Fighting Championship Throwdown" -> "ufc throwdown" and "ufc". */
static int Scrape_AcronymPhrase(const char *norm, char *acr, size_t na, char *phrase, size_t np)
{
    const char *w[16];
    int nw = 0, i, k = 0;
    size_t L;
    char buf[512];
    if (!norm || !acr || !phrase || na < 3 || np < 8) return 0;
    acr[0] = phrase[0] = '\0';
    snprintf(buf, sizeof(buf), "%s", norm);
    {
        char *p = buf;
        while (*p && nw < 16) {
            while (*p == ' ') p++;
            if (!*p) break;
            w[nw++] = p;
            while (*p && *p != ' ') p++;
            if (*p) *p++ = '\0';
        }
    }
    if (nw < 3) return 0;
    for (i = 0; i < nw - 1 && k + 1 < (int)na; i++) {
        const char *s = w[i];
        if (!s[0] || !s[1]) continue;
        if (strcmp(s, "the") == 0 || strcmp(s, "of") == 0 || strcmp(s, "and") == 0
            || strcmp(s, "to") == 0 || strcmp(s, "in") == 0 || strcmp(s, "no") == 0
            || strcmp(s, "da") == 0 || strcmp(s, "de") == 0 || strcmp(s, "for") == 0)
            continue;
        acr[k++] = s[0];
    }
    acr[k] = '\0';
    if (k < 2 || k > 4) { acr[0] = '\0'; return 0; }
    L = strlen(w[nw - 1]);
    if (L < 3 || (size_t)k + 1 + L + 1 >= np) return 0;
    snprintf(phrase, np, "%s %s", acr, w[nw - 1]);
    return 1;
}

/* Dated TOSEC-style names, GoodTools flags, hacks and translations are not
 * used by libretro-thumbnails (No-Intro names such as "Excitebike (Japan, USA)"). */
static int Scrape_LooksDated(const char *s)
{
    const char *p;
    if (!s) return 0;
    for (p = s; (p = strchr(p, '(')) != NULL; p++) {
        if (p[1] >= '0' && p[1] <= '9' && p[2] >= '0' && p[2] <= '9'
            && p[3] >= '0' && p[3] <= '9' && p[4] >= '0' && p[4] <= '9'
            && p[5] == '-' && p[6] >= '0' && p[6] <= '9' && p[7] >= '0' && p[7] <= '9'
            && p[8] == '-')
            return 1;
    }
    return 0;
}

static int Scrape_NameQuality(const char *filename)
{
    if (!filename) return 0;
    if (Scrape_LooksDated(filename) || strstr(filename, ")(")
        || strstr(filename, "[h") || strstr(filename, "[b")
        || strstr(filename, "[t ") || strstr(filename, "[t+")
        || strstr(filename, "(Hack") || strstr(filename, "(Translation")
        || strstr(filename, "(Pirate") || strstr(filename, "Bad Dump")
        || strstr(filename, "(PD)"))
        return -2;
    if (strstr(filename, "(USA") || strstr(filename, "(World")
        || strstr(filename, "(Europe") || strstr(filename, "(Japan")
        || strstr(filename, "(France") || strstr(filename, "(Germany")
        || strstr(filename, "(Spain") || strstr(filename, "(Australia")
        || strstr(filename, "(Korea") || strstr(filename, "(China")
        || strstr(filename, "(Taiwan") || strstr(filename, "(Italy")
        || strstr(filename, "(Sweden") || strstr(filename, "(Asia")
        || strstr(filename, "(Brazil") || strstr(filename, "(Netherlands")
        || strstr(filename, "(Canada") || strstr(filename, "(United Kingdom")
        || strstr(filename, "(Hong Kong") || strstr(filename, "(Russia")
        || strstr(filename, "(Mexico") || strstr(filename, "(En"))
        return 2;
    return 0;
}

static int Scrape_Better(int fit, int qual, int rank, int rel, size_t dist, size_t fileLen,
                         int bestFit, int bestQual, int bestRank, int bestRel,
                         size_t bestDist, size_t bestFile)
{
    if (fit != bestFit) return fit > bestFit;
    if (qual != bestQual) return qual > bestQual;
    if (rank != bestRank) return rank > bestRank;
    if (rel != bestRel) return rel > bestRel;
    if (dist != bestDist) return dist < bestDist;
    return fileLen < bestFile;
}

/* A trailing " (...)" in a title is not part of the name: it names the
   one release to use ("Ms. Pac-Man (Tengen)", "Road Fighter (Japan)"), and
   only files carrying that tag match. Returns the tag, or NULL. */
static const char *Scrape_TitleTag(const char **title, char *base, size_t nb, char *tag, size_t nt)
{
    const char *t = *title;
    const char *open = strrchr(t, '(');
    size_t len = strlen(t);
    if (!open || open == t || open[-1] != ' ' || t[len - 1] != ')') return NULL;
    if ((size_t)(t + len - open) >= nt || (size_t)(open - t) >= nb) return NULL;
    snprintf(tag, nt, "%s", open);
    snprintf(base, nb, "%.*s", (int)(open - 1 - t), t);
    *title = base;
    return tag;
}

static const char *Scrape_MatchOne(const char *title, const char **names, int count,
                                  const char *tag)
{
    char normTitle[512];
    char compactTitle[512];
    const char *best = NULL;
    int bestFit = -1, bestQual = -999, bestRank = -1, bestRel = -999;
    size_t bestDist = (size_t)-1, bestFile = 0, titleLen;
    if (!title || !names || count <= 0) return NULL;
    Scrape_Normalize(normTitle, sizeof(normTitle), title);
    if (normTitle[0] == '\0') return NULL;
    titleLen = strlen(normTitle);
    for (int i = 0; i < count; i++) {
        char stem[512];
        char normStem[512];
        int rank, rel, qual;
        size_t stemLen, fileLen, dist;
        if (!names[i] || (tag && !strstr(names[i], tag))) continue;
        Scrape_Stem(stem, sizeof(stem), names[i]);
        Scrape_Normalize(normStem, sizeof(normStem), stem);
        {
            int fit = Scrape_FitRank(normStem, normTitle);
            if (fit < 0) continue;
            qual = Scrape_NameQuality(names[i]);
            rank = RegionRank(names[i]);
            rel = ReleaseRank(names[i]);
            stemLen = strlen(normStem);
            fileLen = strlen(names[i]);
            dist = stemLen > titleLen ? stemLen - titleLen : titleLen - stemLen;
            if (best && !Scrape_Better(fit, qual, rank, rel, dist, fileLen,
                                       bestFit, bestQual, bestRank, bestRel, bestDist, bestFile))
                continue;
            best = names[i];
            bestFit = fit;
            bestQual = qual;
            bestRank = rank;
            bestRel = rel;
            bestDist = dist;
            bestFile = fileLen;
        }
    }
    if (best) return best;
    /* Fallback: some libretro names omit spaces ("SpaceStation Silicon Valley"). */
    Scrape_Compact(compactTitle, sizeof(compactTitle), normTitle);
    if (compactTitle[0] == '\0') return NULL;
    for (int i = 0; i < count; i++) {
        char stem[512];
        char normStem[512];
        char compactStem[512];
        int rank, rel, qual;
        size_t stemLen, fileLen, dist;
        if (!names[i] || (tag && !strstr(names[i], tag))) continue;
        Scrape_Stem(stem, sizeof(stem), names[i]);
        Scrape_Normalize(normStem, sizeof(normStem), stem);
        Scrape_Compact(compactStem, sizeof(compactStem), normStem);
        {
            int fit;
            if (strcmp(compactStem, compactTitle) == 0) fit = 4;
            else fit = Scrape_FitRank(compactStem, compactTitle);
            if (fit < 0) continue;
            qual = Scrape_NameQuality(names[i]);
            rank = RegionRank(names[i]);
            rel = ReleaseRank(names[i]);
            stemLen = strlen(compactStem);
            fileLen = strlen(names[i]);
            dist = stemLen > titleLen ? stemLen - titleLen : titleLen - stemLen;
            if (best && !Scrape_Better(fit, qual, rank, rel, dist, fileLen,
                                       bestFit, bestQual, bestRank, bestRel, bestDist, bestFile))
                continue;
            best = names[i];
            bestFit = fit;
            bestQual = qual;
            bestRank = rank;
            bestRel = rel;
            bestDist = dist;
            bestFile = fileLen;
        }
    }
    return best;
}

const char *Scrape_Match(const char *title, const char **names, int count)
{
    char baseTitle[512];
    char tagBuf[128];
    char parts[8][512];
    const char *tag;
    const char *best = NULL;
    int bestFit = -1, bestQual = -999, bestRank = -1, bestRel = -999, nParts, p;
    size_t bestDist = (size_t)-1, bestFile = 0;
    if (!title || !names || count <= 0) return NULL;
    /* A title that is a database name (without its ".png" / ".dat") is that entry. */
    for (int i = 0; i < count; i++) {
        size_t fileLen = names[i] ? strlen(names[i]) : 0;
        if (fileLen > 4 && strncmp(names[i], title, fileLen - 4) == 0 && title[fileLen - 4] == '\0')
            return names[i];
    }
    tag = Scrape_TitleTag(&title, baseTitle, sizeof(baseTitle), tagBuf, sizeof(tagBuf));
    nParts = Scrape_TitleParts(title, parts, 8);
    {
        char normA[512], acr[16], phrase[512];
        Scrape_Normalize(normA, sizeof(normA), title);
        if (Scrape_AcronymPhrase(normA, acr, sizeof(acr), phrase, sizeof(phrase))) {
            nParts = Scrape_AddUnique(parts, nParts, 8, phrase);
            nParts = Scrape_AddUnique(parts, nParts, 8, acr);
        }
    }
    for (p = 0; p < nParts; p++) {
        const char *hit = Scrape_MatchOne(parts[p], names, count, tag);
        int fit, rank, rel;
        size_t fileLen, dist;
        char norm[512];
        if (!hit) continue;
        rank = RegionRank(hit);
        rel = ReleaseRank(hit);
        fileLen = strlen(hit);
        Scrape_Normalize(norm, sizeof(norm), parts[p]);
        {
            char stem[512], nstem[512], fullN[512];
            size_t a, b;
            Scrape_Stem(stem, sizeof(stem), hit);
            Scrape_Normalize(nstem, sizeof(nstem), stem);
            Scrape_Normalize(fullN, sizeof(fullN), title);
            fit = Scrape_FitRank(nstem, fullN);
            if (fit < 0) {
                fit = Scrape_FitRank(nstem, norm);
                if (fit < 0) fit = 1;
            }
            a = strlen(nstem);
            b = strlen(fullN[0] ? fullN : norm);
            dist = a > b ? a - b : b - a;
        }
        {
            int qual = Scrape_NameQuality(hit);
            if (!best || Scrape_Better(fit, qual, rank, rel, dist, fileLen,
                                       bestFit, bestQual, bestRank, bestRel, bestDist, bestFile)) {
                best = hit;
                bestFit = fit;
                bestQual = qual;
                bestRank = rank;
                bestRel = rel;
                bestDist = dist;
                bestFile = fileLen;
            }
        }
    }
    return best;
}

/* Scrape_Match, then, if the title's trailing "(tag)" groups name no release in
   the list, the same game under its other releases: one group is dropped at a
   time ("1080 Snowboarding (Japan, USA) (En,Ja)" finds the date of the only
   release the date list has). Exact database names still win. */
const char *Scrape_MatchLoose(const char *title, const char **names, int count)
{
    char cur[512];
    const char *hit = Scrape_Match(title, names, count);
    if (hit || !title || strlen(title) >= sizeof(cur)) return hit;
    snprintf(cur, sizeof(cur), "%s", title);
    for (;;) {
        char *open = strrchr(cur, '(');
        size_t len = strlen(cur);
        if (!open || open == cur || open[-1] != ' ' || cur[len - 1] != ')') return NULL;
        open[-1] = '\0';
        hit = Scrape_Match(cur, names, count);
        if (hit) return hit;
    }
}

/* File stem with trailing " (tag)" groups removed, then normalized. */
static void Scrape_CoreStem(const char *file, char *dst, size_t n)
{
    char stem[512], norm[512];
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!file) return;
    Scrape_Stem(stem, sizeof(stem), file);
    for (;;) {
        char *open = strrchr(stem, '(');
        size_t len = strlen(stem);
        if (!open || open == stem || open[-1] != ' ' || stem[len - 1] != ')') break;
        open[-1] = '\0';
    }
    Scrape_Normalize(norm, sizeof(norm), stem);
    snprintf(dst, n, "%s", norm);
}

int Scrape_CollectMatches(const char *title, const char **names, int count,
                          const char **out, int maxOut)
{
    const char *best = Scrape_MatchLoose(title, names, count);
    char core[512], cur[512];
    int n = 0, i, j;
    if (!out || maxOut <= 0) return 0;
    if (!best) return 0;
    Scrape_CoreStem(best, core, sizeof(core));
    if (!core[0]) {
        out[0] = best;
        return 1;
    }
    for (i = 0; i < count && n < maxOut; i++) {
        int dup = 0;
        if (!names[i]) continue;
        Scrape_CoreStem(names[i], cur, sizeof(cur));
        if (strcmp(cur, core) != 0) continue;
        for (j = 0; j < n; j++) if (strcmp(out[j], names[i]) == 0) { dup = 1; break; }
        if (!dup) out[n++] = names[i];
    }
    if (n == 0) {
        out[0] = best;
        return 1;
    }
    {
        int pref = 0, w = 0;
        for (i = 0; i < n; i++)
            if (Scrape_NameQuality(out[i]) > 0) pref++;
        if (pref > 0 && pref < n) {
            for (i = 0; i < n; i++)
                if (Scrape_NameQuality(out[i]) > 0)
                    out[w++] = out[i];
            n = w;
        }
    }
    return n;
}

int Scrape_ParseIndex(const char *json, char ***outNames, int *outCount)
{
    const char *key = "Named_Boxarts/";
    size_t keyLen = strlen(key);
    const char *p;
    char **names = NULL;
    int count = 0;
    int cap = 0;
    if (!outNames || !outCount) return -1;
    *outNames = NULL;
    *outCount = 0;
    if (!json) return -1;
    if (strstr(json, "\"truncated\":true") || strstr(json, "\"truncated\": true")) return -1;
    p = json;
    while ((p = strstr(p, key)) != NULL) {
        const char *start = p + keyLen;
        const char *end = strchr(start, '"');
        size_t len;
        char *name;
        if (!end) break;
        len = (size_t)(end - start);
        p = end + 1;
        if (len < 4 || memcmp(end - 4, ".png", 4) != 0) continue;
        if (count == cap) {
            int ncap = cap ? cap * 2 : 64;
            char **grown = realloc(names, (size_t)ncap * sizeof(*grown));
            if (!grown) {
                Scrape_FreeNames(names, count);
                return -1;
            }
            names = grown;
            cap = ncap;
        }
        name = malloc(len + 1);
        if (!name) {
            Scrape_FreeNames(names, count);
            return -1;
        }
        memcpy(name, start, len);
        name[len] = '\0';
        names[count++] = name;
    }
    *outNames = names;
    *outCount = count;
    return 0;
}

int Scrape_DatUrl(char *dst, size_t n, const char *field, const char *playlist)
{
    char encPlay[512];
    char url[1024];
    int wrote;
    if (!dst || n == 0 || !field || !playlist) return -1;
    if (UrlEncode(encPlay, sizeof(encPlay), playlist) != 0) return -1;
    wrote = snprintf(url, sizeof(url),
        "https://raw.githubusercontent.com/libretro/libretro-database/master/metadat/%s/%s.dat",
        field, encPlay);
    if (wrote < 0 || (size_t)wrote >= sizeof(url)) return -1;
    if ((size_t)wrote >= n) {
        memcpy(dst, url, n - 1);
        dst[n - 1] = '\0';
        return -1;
    }
    memcpy(dst, url, (size_t)wrote + 1);
    return 0;
}

static int Scrape_DatAdd(char ***names, int **vals, int *count, int *cap,
                         const char *comment, size_t commentLen, long value)
{
    char *name;
    if (*count == *cap) {
        int ncap = *cap ? *cap * 2 : 32;
        char **grownNames = realloc(*names, (size_t)ncap * sizeof(*grownNames));
        int *grownVals;
        if (!grownNames) return -1;
        *names = grownNames;
        grownVals = realloc(*vals, (size_t)ncap * sizeof(*grownVals));
        if (!grownVals) return -1;
        *vals = grownVals;
        *cap = ncap;
    }
    name = malloc(commentLen + 5);
    if (!name) return -1;
    memcpy(name, comment, commentLen);
    memcpy(name + commentLen, ".dat", 5);
    (*names)[*count] = name;
    (*vals)[*count] = (int)value;
    (*count)++;
    return 0;
}

static int Scrape_DatFlush(char ***names, int **vals, int *count, int *cap,
                           const char *comment, size_t commentLen,
                           int hasComment, long value, int hasField)
{
    if (!hasComment || !hasField) return 0;
    return Scrape_DatAdd(names, vals, count, cap, comment, commentLen, value);
}

int Scrape_ParseDat(const char *text, const char *field, char ***outNames, int **outVals, int *outCount)
{
    char **names = NULL;
    int *vals = NULL;
    int count = 0;
    int cap = 0;
    int inGame = 0;
    int hasComment = 0;
    int hasField = 0;
    const char *comment = NULL;
    size_t commentLen = 0;
    long value = 0;
    size_t fieldLen;
    if (!outNames || !outVals || !outCount) return -1;
    *outNames = NULL;
    *outVals = NULL;
    *outCount = 0;
    if (!text || !field) return -1;
    fieldLen = strlen(field);
    for (const char *p = text; *p;) {
        const char *e = strchr(p, '\n');
        const char *q;
        size_t len;
        if (!e) e = p + strlen(p);
        q = p;
        while (q < e && (*q == ' ' || *q == '\t')) q++;
        len = (size_t)(e - q);
        while (len && q[len - 1] == '\r') len--;
        if (len >= 4 && memcmp(q, "game", 4) == 0
            && (len == 4 || q[4] == ' ' || q[4] == '\t' || q[4] == '(')) {
            if (Scrape_DatFlush(&names, &vals, &count, &cap, comment, commentLen,
                                hasComment, value, hasField) != 0)
                goto fail;
            inGame = 1;
            hasComment = 0;
            hasField = 0;
            comment = NULL;
            commentLen = 0;
            value = 0;
        } else if (inGame && len >= 7 && memcmp(q, "comment", 7) == 0
                   && (q[7] == ' ' || q[7] == '\t')) {
            const char *open = memchr(q, '"', len);
            if (open) {
                const char *close = NULL;
                for (const char *r = open + 1; r < q + len; r++) {
                    if (*r == '"') { close = r; break; }
                }
                if (close) {
                    comment = open + 1;
                    commentLen = (size_t)(close - comment);
                    hasComment = 1;
                }
            }
        } else if (inGame && len > fieldLen && memcmp(q, field, fieldLen) == 0
                   && (q[fieldLen] == ' ' || q[fieldLen] == '\t')) {
            const char *open = memchr(q + fieldLen, '"', len - fieldLen);
            if (open) {
                value = (long)strtol(open + 1, NULL, 10);
                hasField = 1;
            }
        } else if (inGame && len == 1 && q[0] == ')') {
            if (Scrape_DatFlush(&names, &vals, &count, &cap, comment, commentLen,
                                hasComment, value, hasField) != 0)
                goto fail;
            inGame = 0;
            hasComment = 0;
            hasField = 0;
        }
        p = *e ? e + 1 : e;
    }
    if (Scrape_DatFlush(&names, &vals, &count, &cap, comment, commentLen,
                        hasComment, value, hasField) != 0)
        goto fail;
    *outNames = names;
    *outVals = vals;
    *outCount = count;
    return 0;
fail:
    Scrape_FreeNames(names, count);
    free(vals);
    return -1;
}

void Scrape_FreeNames(char **names, int count)
{
    if (!names) return;
    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
}

int Scrape_LooksLikeImage(const char *path)
{
    unsigned char mag[8];
    size_t n;
    FILE *f;
    if (!path || !path[0]) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    n = fread(mag, 1, 8, f);
    fclose(f);
    if (n >= 8 && mag[0] == 0x89 && mag[1] == 'P' && mag[2] == 'N' && mag[3] == 'G')
        return 1;
    if (n >= 3 && mag[0] == 0xFF && mag[1] == 0xD8 && mag[2] == 0xFF)
        return 1;
    return 0;
}

int Scrape_AliasTarget(const char *path, char *dst, size_t n)
{
    char buf[256];
    long len;
    size_t i, a, b;
    FILE *f;
    int ok = 0;

    if (!path || !dst || n < 8) return 0;
    dst[0] = '\0';
    f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return 0; }
    len = ftell(f);
    if (len <= 0 || len > 200) { fclose(f); return 0; }
    rewind(f);
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) { fclose(f); return 0; }
    fclose(f);
    buf[len] = '\0';
    for (i = 0; i < (size_t)len; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c == 0 || (c < 32 && c != '\t' && c != '\n' && c != '\r'))
            return 0;
    }
    a = 0;
    while (a < (size_t)len && (buf[a] == ' ' || buf[a] == '\t' || buf[a] == '\n' || buf[a] == '\r'))
        a++;
    b = (size_t)len;
    while (b > a && (buf[b - 1] == ' ' || buf[b - 1] == '\t' || buf[b - 1] == '\n' || buf[b - 1] == '\r'))
        b--;
    if (b <= a || b - a >= n || b - a < 5 || buf[a] == '.') return 0;
    for (i = a; i < b; i++) {
        unsigned char c = (unsigned char)buf[i];
        if (c == '/' || c == '\\' || c == ':' || c < 32) return 0;
    }
    if (b - a >= 4) {
        const char *e = buf + b - 4;
        if (e[0] == '.' && (e[1] == 'p' || e[1] == 'P') && (e[2] == 'n' || e[2] == 'N') &&
            (e[3] == 'g' || e[3] == 'G'))
            ok = 1;
        if (e[0] == '.' && (e[1] == 'j' || e[1] == 'J') && (e[2] == 'p' || e[2] == 'P') &&
            (e[3] == 'g' || e[3] == 'G'))
            ok = 1;
    }
    if (!ok && b - a >= 5) {
        const char *e = buf + b - 5;
        if (e[0] == '.' && (e[1] == 'j' || e[1] == 'J') && (e[2] == 'p' || e[2] == 'P') &&
            (e[3] == 'e' || e[3] == 'E') && (e[4] == 'g' || e[4] == 'G'))
            ok = 1;
    }
    if (!ok) return 0;
    memcpy(dst, buf + a, b - a);
    dst[b - a] = '\0';
    return 1;
}

#endif /* SCRAPE_IMPLEMENTATION_ONCE */
#endif /* SCRAPE_IMPLEMENTATION */
