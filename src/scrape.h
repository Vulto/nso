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
int Scrape_ParseIndex(const char *json, char ***outNames, int *outCount);
int Scrape_DatUrl(char *dst, size_t n, const char *field, const char *playlist);
int Scrape_ParseDat(const char *text, const char *field, char ***outNames, int **outVals, int *outCount);
void Scrape_FreeNames(char **names, int count);

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
    char encPlay[512];
    char encFile[1024];
    int wrote;
    if (UrlEncode(encPlay, sizeof(encPlay), playlist) != 0) return -1;
    if (UrlEncode(encFile, sizeof(encFile), file) != 0) return -1;
    wrote = snprintf(dst, n, "https://thumbnails.libretro.com/%s/Named_Boxarts/%s",
                     encPlay, encFile);
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
        }
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

static void Scrape_Compact(char *dst, size_t n, const char *src)
{
    size_t u = 0;
    if (!dst || n == 0) return;
    for (const char *s = src ? src : ""; *s && u + 1 < n; s++) {
        if (*s != ' ') dst[u++] = *s;
    }
    dst[u] = '\0';
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

const char *Scrape_Match(const char *title, const char **names, int count)
{
    char normTitle[512];
    char compactTitle[512];
    char baseTitle[512];
    char tagBuf[128];
    const char *tag;
    const char *best = NULL;
    int bestRank = -1;
    size_t bestStem = 0;
    size_t bestFile = 0;
    if (!title || !names || count <= 0) return NULL;
    /* A title that is a database name (without its ".png" / ".dat") is that entry. */
    for (int i = 0; i < count; i++) {
        size_t fileLen = names[i] ? strlen(names[i]) : 0;
        if (fileLen > 4 && strncmp(names[i], title, fileLen - 4) == 0 && title[fileLen - 4] == '\0')
            return names[i];
    }
    tag = Scrape_TitleTag(&title, baseTitle, sizeof(baseTitle), tagBuf, sizeof(tagBuf));
    Scrape_Normalize(normTitle, sizeof(normTitle), title);
    if (normTitle[0] == '\0') return NULL;
    for (int i = 0; i < count; i++) {
        char stem[512];
        char normStem[512];
        int rank;
        size_t stemLen;
        size_t fileLen;
        int better = 0;
        if (!names[i] || (tag && !strstr(names[i], tag))) continue;
        Scrape_Stem(stem, sizeof(stem), names[i]);
        Scrape_Normalize(normStem, sizeof(normStem), stem);
        if (!EndsWithWord(normStem, normTitle)) continue;
        rank = RegionRank(names[i]);
        stemLen = strlen(normStem);
        fileLen = strlen(names[i]);
        if (!best || rank > bestRank) better = 1;
        else if (rank == bestRank && stemLen < bestStem) better = 1;
        else if (rank == bestRank && stemLen == bestStem && fileLen < bestFile) better = 1;
        if (!better) continue;
        best = names[i];
        bestRank = rank;
        bestStem = stemLen;
        bestFile = fileLen;
    }
    if (best) return best;
    /* Fallback: some libretro names omit spaces ("SpaceStation Silicon Valley"). */
    Scrape_Compact(compactTitle, sizeof(compactTitle), normTitle);
    if (compactTitle[0] == '\0') return NULL;
    for (int i = 0; i < count; i++) {
        char stem[512];
        char normStem[512];
        char compactStem[512];
        int rank;
        size_t stemLen;
        size_t fileLen;
        int better = 0;
        if (!names[i] || (tag && !strstr(names[i], tag))) continue;
        Scrape_Stem(stem, sizeof(stem), names[i]);
        Scrape_Normalize(normStem, sizeof(normStem), stem);
        Scrape_Compact(compactStem, sizeof(compactStem), normStem);
        if (strcmp(compactStem, compactTitle) != 0) continue;
        rank = RegionRank(names[i]);
        stemLen = strlen(compactStem);
        fileLen = strlen(names[i]);
        if (!best || rank > bestRank) better = 1;
        else if (rank == bestRank && stemLen < bestStem) better = 1;
        else if (rank == bestRank && stemLen == bestStem && fileLen < bestFile) better = 1;
        if (!better) continue;
        best = names[i];
        bestRank = rank;
        bestStem = stemLen;
        bestFile = fileLen;
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

#endif /* SCRAPE_IMPLEMENTATION_ONCE */
#endif /* SCRAPE_IMPLEMENTATION */
