/* update.h - self-update helpers: version compare and GitHub release JSON
 * parsing. Pure functions (no I/O), unit-tested in update_test.c. */
#ifndef NSO_UPDATE_H
#define NSO_UPDATE_H

#include <stddef.h>
#include <string.h>

#define UPDATE_ASSET_SUFFIX "linux-x86_64.tar.gz"
#define UPDATE_URL_PREFIX "https://github.com/Vulto/nso/"

/* "v0.2.1" / "0.2" -> {0,2,1}; missing parts are 0. 0 if no digit leads. */
static inline int Update_VersionParse(const char *s, int v[3])
{
    int i;
    v[0] = v[1] = v[2] = 0;
    if (!s) return 0;
    if (*s == 'v' || *s == 'V') s++;
    if (*s < '0' || *s > '9') return 0;
    for (i = 0; i < 3; i++) {
        long n = 0;
        if (*s < '0' || *s > '9') break;
        while (*s >= '0' && *s <= '9') {
            n = n * 10 + (*s - '0');
            if (n > 100000) n = 100000;
            s++;
        }
        v[i] = (int)n;
        if (*s != '.') break;
        s++;
    }
    return 1;
}

/* 1 when tag is a strictly newer version than cur (both must parse). */
static inline int Update_VersionNewer(const char *tag, const char *cur)
{
    int a[3], b[3], i;
    if (!Update_VersionParse(tag, a) || !Update_VersionParse(cur, b)) return 0;
    for (i = 0; i < 3; i++) {
        if (a[i] != b[i]) return a[i] > b[i];
    }
    return 0;
}

static inline const char *Update_JsonString(const char *p, const char *end, char *dst, size_t n)
{
    size_t u = 0;
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ':')) p++;
    if (p >= end || *p != '"') return NULL;
    p++;
    while (p < end && *p != '"') {
        if (*p == '\\' || u + 1 >= n) return NULL;
        dst[u++] = *p++;
    }
    if (p >= end) return NULL;
    dst[u] = '\0';
    return p + 1;
}

static inline int Update_SafeText(const char *s, int url)
{
    if (!s || !s[0]) return 0;
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || c == '.' || c == '-' || c == '_') continue;
        if (url && (c == ':' || c == '/' || c == '%' || c == '+' || c == '~')) continue;
        return 0;
    }
    return 1;
}

/* Reads tag_name and the Linux x86_64 tarball asset (browser_download_url and,
 * when GitHub reports it, the sha256 digest as bare hex; digest is "" if none).
 * Asset URLs must live under github.com/Vulto/nso unless allowAnyUrl (tests). */
static inline int Update_ParseRelease(const char *json, char *tag, size_t tn,
                                      char *url, size_t un, char *digest, size_t dn,
                                      int allowAnyUrl)
{
    static const char kUrl[] = "\"browser_download_url\"";
    const char *end, *p, *segStart;
    size_t sl = strlen(UPDATE_ASSET_SUFFIX);
    if (!json || !tag || !url || !digest || tn == 0 || un == 0 || dn == 0) return 0;
    tag[0] = url[0] = digest[0] = '\0';
    end = json + strlen(json);
    p = strstr(json, "\"tag_name\"");
    if (!p || !Update_JsonString(p + 10, end, tag, tn) || !Update_SafeText(tag, 0)) return 0;
    segStart = json;
    for (p = json; (p = strstr(p, kUrl)) != NULL; ) {
        char u[1024];
        const char *after = Update_JsonString(p + sizeof(kUrl) - 1, end, u, sizeof(u));
        size_t ul = strlen(u);
        if (after && ul > sl && strcmp(u + ul - sl, UPDATE_ASSET_SUFFIX) == 0
            && Update_SafeText(u, 1)
            && (allowAnyUrl || strncmp(u, UPDATE_URL_PREFIX, strlen(UPDATE_URL_PREFIX)) == 0)
            && ul < un) {
            const char *d = NULL, *q;
            for (q = segStart; (q = strstr(q, "\"digest\"")) != NULL && q < p; q += 8) d = q;
            memcpy(url, u, ul + 1);
            if (d) {
                char dv[128];
                if (Update_JsonString(d + 8, end, dv, sizeof(dv))
                    && strncmp(dv, "sha256:", 7) == 0 && strlen(dv + 7) == 64
                    && Update_SafeText(dv + 7, 0))
                    memcpy(digest, dv + 7, 65);
            }
            return 1;
        }
        segStart = after ? after : p + 1;
        p = after ? after : p + 1;
    }
    return 0;
}

#endif /* NSO_UPDATE_H */
