#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
/* version.h - group catalog / ROM / wiki titles into one game with versions.
 * Pure C, no raylib. Unit-tested in version_test.c.
 *
 * Grouping key: a normalized base title after stripping No-Intro region,
 * revision, dump, and generic hack/patch tags, plus per-system patch tags.
 * SNES tags (FastRom, SA-1, FastRom + SA-1, …) live in that system's table;
 * other systems have their own tables (empty unless a tag is well known)
 * and otherwise fall back to generic region / revision / named-hack override.
 *
 * A catalog row may set GameEntry.group and GameEntry.version to force the
 * group and the version label (named hacks or odd titles). */
#ifndef NSO_VERSION_H
#define NSO_VERSION_H

#include <stdbool.h>
#include <stddef.h>

#ifndef VERSION_NAME
#define VERSION_NAME 160
#endif

typedef struct {
    char base[VERSION_NAME];   /* display base title (punctuation kept) */
    char key[VERSION_NAME];    /* lowercase grouping key */
    char label[VERSION_NAME];  /* "USA", "USA · FastRom + SA-1", hack name */
    char region[VERSION_NAME];           /* canonical region, or empty */
    char patch[80];            /* canonical patch string, or empty */
    bool hack;                 /* (Hack) / [h] / named hack */
    bool patched;              /* generic patch / system patch / hack */
} VersionInfo;

/* Patch-tag table for `sysId` ("snes", "nes", …). Count 0 if none / unknown. */
const char *const *Version_Tags(const char *sysId, int *count);

/* Parse a catalog title and optional ROM path. Overrides force group / label
 * when non-empty. `sysId` selects the per-system tag table. */
void Version_Parse(const char *title, const char *rom, const char *sysId,
                   const char *groupOverride, const char *labelOverride,
                   VersionInfo *out);

/* True when the version is a USA (or USA, …) release. */
bool Version_IsUsa(const VersionInfo *v);
/* True when there is no patch, hack, or generic "patched" tag. */
bool Version_IsUnpatched(const VersionInfo *v);

/* Index of the preferred version: a saved label only if that member is
 * playable, else playable, then stock dumps before hacks/mods, then
 * USA > World > Europe > Japan, then unpatched, then stable order. */
int Version_Prefer(const VersionInfo *v, const bool *playable, int n,
                   const char *savedLabel);

/* "Title (USA)", "Title (Japan) [Hack]" — title plus tags from the file. */
void Version_FormatDisplay(const char *title, const VersionInfo *v,
                           char *dst, size_t n);

/* config/versions-<id>.txt helpers: each line is "groupTitle<TAB>file"
 * (the exact ROM file name). Older files stored a version label instead. */
const char *Version_SelGet(const char *text, const char *groupTitle, const char *groupKey);
int Version_SelSet(const char *text, const char *groupTitle, const char *label,
                   char *out, size_t n);
/* Copy a label returned by Version_SelGet (stops at the end of the field). */
void Version_SelLabelCopy(const char *p, char *dst, size_t n);


/* User ROM tree is <NSO>/roms/<System>/ (loose files or one folder per game). */
#ifndef VERSION_ROM_ROOT
#define VERSION_ROM_ROOT "roms"
#endif

bool Version_IsRomExt(const char *name);
bool Version_IsPatchExt(const char *name);
void Version_FileName(const char *path, char *dst, size_t n);
void Version_FileStem(const char *path, char *dst, size_t n);
void Version_PrettifyFolder(const char *folder, char *dst, size_t n);
/* repos/roms/... → roms/...; other paths are copied as-is. */
void Version_RewriteRomPrefix(const char *rel, char *dst, size_t n);
/* If exists(rel) copy rel, else try the roms/ prefix rewrite. exists() is
 * optional; a NULL exists only rewrites the string. Returns 1 if dst is set. */
int Version_ResolveRel(const char *rel, char *dst, size_t n,
                       int (*exists)(const char *rel, void *u), void *u);

typedef struct {
    char folder[256]; /* empty = loose file in the system dir */
    char name[256];
    char rel[768];             /* roms/<System>/[folder/]file */
    int isPatch;
} VersionScanItem;

/* Read-only listing of loose files and one level of per-game folders.
 * sysFolder is "SNES", "NES", ...  Returns the number of items written. */
int Version_ScanDir(const char *absSysDir, const char *sysFolder,
                    VersionScanItem *out, int cap);

/* Patch applicators write dst only; src and the patch file are never modified. */
int Version_ApplyIps(const char *srcPath, const char *ipsPath, const char *dstPath);
int Version_ApplyBps(const char *srcPath, const char *bpsPath, const char *dstPath);
int Version_ApplyUps(const char *srcPath, const char *upsPath, const char *dstPath);
/* Build a launch ROM in destDir (created). IPS uses a same-basename sidecar
 * so Mednafen can soft-patch; BPS/UPS write a patched copy. 0 = ok. */
int Version_PreparePatchedRom(const char *srcAbs, const char *patchAbs,
                              const char *destDir, char *outAbs, size_t n);


#endif /* NSO_VERSION_H */

#ifdef VERSION_IMPLEMENTATION
#ifndef VERSION_IMPLEMENTATION_ONCE
#define VERSION_IMPLEMENTATION_ONCE

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- per-system patch-tag tables (longer forms first) ---- */

static const char *const VersionTags_snes[] = {
    "FastRom + SA-1",
    "FastROM + SA-1",
    "FastRom+SA-1",
    "FastROM+SA-1",
    "FastRom",
    "FastROM",
    "SA-1",
    "SA1",
    "MSU-1",
    "MSU1",
};

const char *const *Version_Tags(const char *sysId, int *count)
{
    static const char *const empty[] = { NULL };
    if (sysId && strcmp(sysId, "snes") == 0) {
        if (count) *count = (int)(sizeof(VersionTags_snes) / sizeof(VersionTags_snes[0]));
        return VersionTags_snes;
    }
    (void)empty;
    if (count) *count = 0;
    return empty;
}

/* Collapse spaces and trim in place. */
static void Version_CleanSpaces(char *s)
{
    char *d, *src;
    int sp;
    if (!s) return;
    src = s;
    while (*src && isspace((unsigned char)*src)) src++;
    d = s;
    sp = 0;
    for (; *src; src++) {
        if (isspace((unsigned char)*src)) { sp = 1; continue; }
        if (sp && d > s) *d++ = ' ';
        *d++ = *src;
        sp = 0;
    }
    *d = '\0';
}

static void Version_MakeKey(const char *base, char *dst, size_t n)
{
    size_t u = 0;
    int sp = 1;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!base) return;
    for (const char *p = base; *p && u + 1 < n; p++) {
        unsigned char c = (unsigned char)*p;
        if (isalnum(c)) {
            if (sp && u > 0) dst[u++] = ' ';
            if (u + 1 >= n) break;
            dst[u++] = (char)tolower(c);
            sp = 0;
        } else {
            sp = 1;
        }
    }
    dst[u] = '\0';
}

static void Version_Copy(char *dst, size_t n, const char *s)
{
    size_t len;
    if (!dst || n == 0) return;
    if (!s) { dst[0] = '\0'; return; }
    len = strlen(s);
    if (len >= n) len = n - 1;
    memcpy(dst, s, len);
    dst[len] = '\0';
}

static int Version_EqFold(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *b) {
        unsigned char ca = (unsigned char)*a++, cb = (unsigned char)*b++;
        if (tolower(ca) != tolower(cb)) return 0;
    }
    return *a == '\0' && *b == '\0';
}

/* Lowercase, drop spaces, '+', and '-'. Used only to match known tags. */
static void Version_TagFold(const char *s, char *dst, size_t n)
{
    size_t u = 0;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!s) return;
    for (; *s && u + 1 < n; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == ' ' || c == '+' || c == '-' || c == '_' || c == '.') continue;
        if (!isalnum(c)) continue;
        dst[u++] = (char)tolower(c);
    }
    dst[u] = '\0';
}

static bool Version_IsLangCode(const char *s, size_t n)
{
    static const char *const codes[] = {
        "en", "fr", "de", "es", "it", "ja", "jp", "nl", "pt", "sv", "no", "da",
        "fi", "zh", "ko", "pl", "ru", "enfr", /* not used as 4 */
    };
    char buf[8];
    if (n == 0 || n > 3) return false;
    for (size_t i = 0; i < n; i++) buf[i] = (char)tolower((unsigned char)s[i]);
    buf[n] = '\0';
    /* "jp" as a lone language-ish code is treated as Japan in region; not here. */
    if (strcmp(buf, "jp") == 0 || strcmp(buf, "ja") == 0) {
        /* "Ja" is language; "JP" as a 2-letter all-caps region is handled elsewhere.
         * Both Ja and JP appear as language and region. Prefer language only in
         * a comma list; a lone "JP" / "J" is a region. */
        return n == 2 && (buf[0] == 'j' && buf[1] == 'a');
    }
    for (size_t i = 0; i < sizeof(codes) / sizeof(codes[0]); i++)
        if (strcmp(buf, codes[i]) == 0) return true;
    return false;
}

static bool Version_IsLangTag(const char *s)
{
    const char *p = s;
    int parts = 0;
    if (!s || !s[0]) return false;
    while (*p) {
        const char *a;
        size_t n;
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        a = p;
        while (*p && *p != ',') p++;
        n = (size_t)(p - a);
        while (n && a[n - 1] == ' ') n--;
        if (!Version_IsLangCode(a, n)) return false;
        parts++;
    }
    return parts > 0;
}

static const struct { const char *in; const char *out; } VersionRegions[] = {
    { "usa", "USA" }, { "us", "USA" }, { "u", "USA" },
    { "america", "USA" }, { "na", "USA" },
    { "japan", "Japan" }, { "jpn", "Japan" }, { "jp", "Japan" }, { "j", "Japan" },
    { "europe", "Europe" }, { "eur", "Europe" }, { "eu", "Europe" }, { "e", "Europe" },
    { "world", "World" }, { "w", "World" },
    { "germany", "Germany" }, { "ger", "Germany" }, { "de", "Germany" },
    { "france", "France" }, { "fra", "France" },
    { "spain", "Spain" }, { "spa", "Spain" },
    { "italy", "Italy" }, { "ita", "Italy" },
    { "uk", "UK" }, { "gb", "UK" }, { "england", "UK" }, { "britain", "UK" },
    { "australia", "Australia" }, { "aus", "Australia" }, { "au", "Australia" },
    { "korea", "Korea" }, { "kor", "Korea" }, { "kr", "Korea" },
    { "china", "China" }, { "chn", "China" }, { "cn", "China" },
    { "brazil", "Brazil" }, { "bra", "Brazil" }, { "br", "Brazil" },
    { "canada", "Canada" }, { "can", "Canada" }, { "ca", "Canada" },
    { "taiwan", "Taiwan" }, { "tw", "Taiwan" },
    { "asia", "Asia" }, { "as", "Asia" },
    { "netherlands", "Netherlands" }, { "holland", "Netherlands" },
    { "sweden", "Sweden" },
    { "ue", "USA, Europe" }, { "ju", "Japan, USA" }, { "je", "Japan, Europe" },
};

static bool Version_OneRegion(const char *s, size_t n, char *dst, size_t cap)
{
    char buf[64];
    if (n == 0 || n >= sizeof(buf)) return false;
    for (size_t i = 0; i < n; i++) buf[i] = (char)tolower((unsigned char)s[i]);
    buf[n] = '\0';
    Version_CleanSpaces(buf);
    for (size_t i = 0; i < sizeof(VersionRegions) / sizeof(VersionRegions[0]); i++) {
        if (strcmp(buf, VersionRegions[i].in) == 0) {
            Version_Copy(dst, cap, VersionRegions[i].out);
            return true;
        }
    }
    return false;
}

/* A region tag, or a comma list of regions (optionally mixed with langs). */
static bool Version_IsRegionTag(const char *s, char *dst, size_t cap)
{
    const char *p;
    char acc[64];
    int got = 0;
    if (!s || !s[0]) return false;
    if (Version_IsLangTag(s)) return false;
    acc[0] = '\0';
    p = s;
    while (*p) {
        const char *a;
        size_t n;
        char one[32];
        while (*p == ' ' || *p == ',') p++;
        if (!*p) break;
        a = p;
        while (*p && *p != ',') p++;
        n = (size_t)(p - a);
        while (n && a[n - 1] == ' ') n--;
        if (Version_IsLangCode(a, n)) continue;
        if (!Version_OneRegion(a, n, one, sizeof(one))) return false;
        if (got) {
            size_t used = strlen(acc);
            snprintf(acc + used, sizeof(acc) - used, ", %s", one);
        } else {
            Version_Copy(acc, sizeof(acc), one);
        }
        got++;
    }
    if (!got) return false;
    Version_Copy(dst, cap, acc);
    return true;
}

static bool Version_IsRevTag(const char *s, char *dst, size_t cap)
{
    const char *p = s;
    char body[32];
    if (!s || !s[0]) return false;
    if (Version_EqFold(s, "rev") ) return false;
    if ((p[0] == 'v' || p[0] == 'V') && isdigit((unsigned char)p[1])) {
        snprintf(dst, cap, "Rev %s", p + 1);
        return true;
    }
    if (strncmp(s, "Rev", 3) == 0 || strncmp(s, "REV", 3) == 0 || strncmp(s, "rev", 3) == 0) {
        p = s + 3;
        if (*p == '.' || *p == '-' || *p == ' ') p++;
        if (!*p) return false;
        Version_Copy(body, sizeof(body), p);
        Version_CleanSpaces(body);
        if (!body[0]) return false;
        snprintf(dst, cap, "Rev %s", body);
        return true;
    }
    if (strncmp(s, "Ver", 3) == 0 || strncmp(s, "VER", 3) == 0 || strncmp(s, "ver", 3) == 0) {
        p = s + 3;
        if (*p == 's' || *p == 'S') { /* Version */
            if (p[1] == 'i' || p[1] == 'I') p += 4;
            else p++;
        }
        if (*p == '.' || *p == '-' || *p == ' ') p++;
        if (!*p || !isdigit((unsigned char)*p)) return false;
        snprintf(dst, cap, "Rev %s", p);
        return true;
    }
    return false;
}

static bool Version_IsDumpTag(const char *s)
{
    char c;
    const char *p;
    if (!s || !s[0]) return false;
    if (strcmp(s, "!") == 0) return true;
    c = (char)tolower((unsigned char)s[0]);
    p = s + 1;
    if (c == 'b' || c == 'f' || c == 'o' || c == 'a' || c == 'p' || c == 't') {
        if (*p == '\0') return true;
        if (c == 't' && (*p == '+' || *p == '-')) return true; /* T+Eng */
        while (isdigit((unsigned char)*p)) p++;
        return *p == '\0';
    }
    return false;
}

static bool Version_StartsFold(const char *s, const char *pref)
{
    if (!s || !pref) return false;
    while (*s && *pref) {
        if (tolower((unsigned char)*s) != tolower((unsigned char)*pref)) return false;
        s++;
        pref++;
    }
    return *pref == '\0';
}

static bool Version_IsDiscTag(const char *s)
{
    const char *p = s;
    if (!s) return false;
    if (Version_StartsFold(s, "disc")) p = s + 4;
    else if (Version_StartsFold(s, "disk")) p = s + 4;
    else return false;
    if (*p == ' ' || *p == '-' || *p == '.') p++;
    return *p != '\0'; /* Disc 1, Disk 2 of 2, … */
}

static bool Version_IsGenericPatch(const char *s, char *dst, size_t cap)
{
    if (!s || !s[0]) return false;
    if (Version_EqFold(s, "patched") || Version_EqFold(s, "patch")
        || Version_EqFold(s, "fix") || Version_EqFold(s, "fixed")) {
        Version_Copy(dst, cap, "patched");
        return true;
    }
    if (Version_EqFold(s, "hack") || (s[0] == 'h' || s[0] == 'H') ) {
        const char *p = s + 1;
        if (Version_EqFold(s, "hack")) {
            Version_Copy(dst, cap, "Hack");
            return true;
        }
        /* [h], [h1], [h2] */
        if ((s[0] == 'h' || s[0] == 'H') && (s[1] == '\0' || isdigit((unsigned char)s[1]))) {
            while (isdigit((unsigned char)*p)) p++;
            if (*p == '\0') {
                Version_Copy(dst, cap, "Hack");
                return true;
            }
        }
    }
    return false;
}

static bool Version_IsRerelease(const char *s, char *dst, size_t cap)
{
    char f[80];
    if (!s) return false;
    Version_TagFold(s, f, sizeof(f));
    if (strcmp(f, "vc") == 0 || strcmp(f, "virtualconsole") == 0
        || strcmp(f, "wiivirtualconsole") == 0
        || strcmp(f, "wiiuvirtualconsole") == 0) {
        Version_Copy(dst, cap, "Virtual Console");
        return true;
    }
    if (strcmp(f, "switchonline") == 0 || strcmp(f, "nintendoswitchonline") == 0
        || strcmp(f, "nso") == 0) {
        Version_Copy(dst, cap, "Switch Online");
        return true;
    }
    if (strstr(f, "classicmini") || strcmp(f, "snesclassic") == 0
        || strcmp(f, "classic") == 0) {
        Version_Copy(dst, cap, "Classic Mini");
        return true;
    }
    return false;
}

static bool Version_IsBeta(const char *s, char *dst, size_t cap)
{
    if (!s) return false;
    if (Version_EqFold(s, "beta") || Version_EqFold(s, "alpha")
        || Version_EqFold(s, "proto") || Version_EqFold(s, "prototype")
        || Version_EqFold(s, "sample") || Version_EqFold(s, "demo")
        || Version_EqFold(s, "promo") || Version_EqFold(s, "prerelease")
        || Version_EqFold(s, "pre-release") || Version_EqFold(s, "unl")
        || Version_EqFold(s, "unlicensed") || Version_EqFold(s, "pirate")) {
        /* Canonical: first letter upper, rest as given for short tokens. */
        if (Version_EqFold(s, "unl") || Version_EqFold(s, "unlicensed"))
            Version_Copy(dst, cap, "Unl");
        else if (Version_EqFold(s, "proto") || Version_EqFold(s, "prototype"))
            Version_Copy(dst, cap, "Proto");
        else if (Version_EqFold(s, "pre-release") || Version_EqFold(s, "prerelease"))
            Version_Copy(dst, cap, "Pre-Release");
        else {
            char buf[40];
            Version_Copy(buf, sizeof(buf), s);
            buf[0] = (char)toupper((unsigned char)buf[0]);
            Version_Copy(dst, cap, buf);
        }
        return true;
    }
    return false;
}

/* Longest matching system tag; writes canonical spelling. */
static bool Version_MatchSysTag(const char *s, const char *sysId, char *dst, size_t cap)
{
    int n = 0;
    const char *const *tags = Version_Tags(sysId, &n);
    char want[80], have[80];
    int best = -1;
    size_t bestLen = 0;
    if (!s || !tags || n <= 0) return false;
    Version_TagFold(s, want, sizeof(want));
    if (!want[0]) return false;
    for (int i = 0; i < n; i++) {
        Version_TagFold(tags[i], have, sizeof(have));
        if (strcmp(want, have) == 0 && strlen(have) > bestLen) {
            best = i;
            bestLen = strlen(have);
        }
    }
    if (best < 0) return false;
    /* Canonical spellings for SNES. */
    Version_TagFold(tags[best], have, sizeof(have));
    if (strcmp(have, "fastromsa1") == 0) Version_Copy(dst, cap, "FastRom + SA-1");
    else if (strcmp(have, "fastrom") == 0) Version_Copy(dst, cap, "FastRom");
    else if (strcmp(have, "sa1") == 0) Version_Copy(dst, cap, "SA-1");
    else if (strcmp(have, "msu1") == 0) Version_Copy(dst, cap, "MSU-1");
    else Version_Copy(dst, cap, tags[best]);
    return true;
}

static void Version_Join(char *dst, size_t n, const char *a, const char *sep, const char *b)
{
    size_t used = 0, la, ls, lb;
    if (!dst || n == 0) return;
    if (!a || !a[0]) { Version_Copy(dst, n, b ? b : ""); return; }
    if (!b || !b[0]) { Version_Copy(dst, n, a); return; }
    la = strlen(a); ls = sep ? strlen(sep) : 0; lb = strlen(b);
    if (la >= n) { Version_Copy(dst, n, a); return; }
    memcpy(dst, a, la); used = la;
    if (used + ls >= n) { dst[used] = '\0'; return; }
    if (ls) { memcpy(dst + used, sep, ls); used += ls; }
    if (used + lb >= n) lb = n - used - 1;
    memcpy(dst + used, b, lb); used += lb;
    dst[used] = '\0';
}

static void Version_AddPart(char *dst, size_t n, const char *part)
{
    char tmp[VERSION_NAME];
    if (!part || !part[0]) return;
    if (dst[0] && strstr(dst, part)) return;
    Version_Join(tmp, sizeof(tmp), dst, " · ", part);
    Version_Copy(dst, n, tmp);
}

static void Version_AddRegion(char *dst, size_t n, const char *part)
{
    char tmp[VERSION_NAME];
    if (!part || !part[0]) return;
    if (dst[0] && strstr(dst, part)) return;
    Version_Join(tmp, sizeof(tmp), dst, ", ", part);
    Version_Copy(dst, n, tmp);
}

static void Version_StripExt(char *s)
{
    static const char *const ext[] = {
        ".sfc", ".smc", ".fig", ".swc", ".snes", ".nes", ".unf", ".fds",
        ".n64", ".z64", ".v64", ".rom", ".iso", ".gcm", ".gcz", ".ciso",
        ".rvz", ".wbfs", ".wad", ".md", ".gen", ".smd", ".bin", ".32x",
        ".cue", ".chd", ".zip", ".7z", ".gz", ".rar", ".gba", ".gb", ".gbc"
    };
    size_t n = s ? strlen(s) : 0;
    if (!s || n < 3) return;
    for (size_t i = 0; i < sizeof(ext) / sizeof(ext[0]); i++) {
        size_t e = strlen(ext[i]);
        if (n > e && Version_EqFold(s + n - e, ext[i])) {
            s[n - e] = '\0';
            return;
        }
    }
}

static const char *Version_Basename(const char *path, char *dst, size_t n)
{
    const char *s = path, *slash;
    if (!path || !dst || n == 0) return NULL;
    slash = strrchr(path, '/');
    if (!slash) slash = strrchr(path, '\\');
    s = slash ? slash + 1 : path;
    Version_Copy(dst, n, s);
    Version_StripExt(dst);
    return dst;
}

typedef struct {
    char region[VERSION_NAME];
    char rev[80];
    char patch[80];
    char extra[80];
    char rere[40];
    char beta[32];
    char hackName[VERSION_NAME];
    bool hack;
    bool patched;
} VersionBits;

static void Version_FoldPatches(char *patch, size_t n)
{
    /* If both FastRom and SA-1 were collected, show the combo. */
    char f[80];
    int hasF = 0, hasS = 0, hasC = 0;
    if (!patch || !patch[0]) return;
    Version_TagFold(patch, f, sizeof(f));
    if (strstr(f, "fastromsa1")) hasC = 1;
    if (strstr(f, "fastrom")) hasF = 1;
    if (strstr(f, "sa1")) hasS = 1;
    if (hasC || (hasF && hasS)) Version_Copy(patch, n, "FastRom + SA-1");
}

static bool Version_Classify(const char *tag, const char *sysId, VersionBits *b)
{
    char canon[80];
    if (!tag || !tag[0] || !b) return false;
    if (Version_IsDiscTag(tag)) return false;          /* keep in base */
    if (Version_IsDumpTag(tag)) return true;           /* drop */
    if (Version_IsLangTag(tag)) return true;           /* drop */
    if (Version_IsRegionTag(tag, canon, sizeof(canon))) {
        Version_AddRegion(b->region, sizeof(b->region), canon);
        return true;
    }
    if (Version_IsRevTag(tag, canon, sizeof(canon))) {
        if (!b->rev[0]) Version_Copy(b->rev, sizeof(b->rev), canon);
        return true;
    }
    if (Version_MatchSysTag(tag, sysId, canon, sizeof(canon))) {
        Version_AddPart(b->patch, sizeof(b->patch), canon);
        Version_FoldPatches(b->patch, sizeof(b->patch));
        b->patched = true;
        return true;
    }
    if (Version_IsGenericPatch(tag, canon, sizeof(canon))) {
        if (strcmp(canon, "Hack") == 0) b->hack = true;
        else {
            b->patched = true;
            if (!b->extra[0]) Version_Copy(b->extra, sizeof(b->extra), canon);
        }
        return true;
    }
    if (Version_IsRerelease(tag, canon, sizeof(canon))) {
        if (!b->rere[0]) Version_Copy(b->rere, sizeof(b->rere), canon);
        return true;
    }
    if (Version_IsBeta(tag, canon, sizeof(canon))) {
        if (!b->beta[0]) Version_Copy(b->beta, sizeof(b->beta), canon);
        return true;
    }
    return false;
}

/* After (Hack), an unknown parenthetical is the hack's name. */
static bool Version_TakeHackName(const char *tag, VersionBits *b)
{
    if (!tag || !tag[0] || !b || !b->hack) return false;
    if (b->hackName[0]) return false;
    if (Version_IsDiscTag(tag) || Version_IsDumpTag(tag) || Version_IsLangTag(tag))
        return false;
    Version_Copy(b->hackName, sizeof(b->hackName), tag);
    Version_CleanSpaces(b->hackName);
    return b->hackName[0] != '\0';
}

static bool Version_TakeTrailingBracket(char *s, char *tag, size_t n)
{
    size_t len;
    char close, open;
    int depth;
    size_t i, start;
    Version_CleanSpaces(s);
    len = strlen(s);
    if (len < 3) return false;
    close = s[len - 1];
    if (close == ')') open = '(';
    else if (close == ']') open = '[';
    else return false;
    depth = 1;
    i = len - 1;
    while (i-- > 0) {
        if (s[i] == close) depth++;
        else if (s[i] == open) {
            depth--;
            if (depth == 0) break;
        }
    }
    if (s[i] != open || depth != 0) return false;
    start = i;
    /* copy inner */
    if (len - start - 2 >= n) return false;
    memcpy(tag, s + start + 1, len - start - 2);
    tag[len - start - 2] = '\0';
    Version_CleanSpaces(tag);
    s[start] = '\0';
    Version_CleanSpaces(s);
    return tag[0] != '\0';
}

static bool Version_TakeTrailingSys(char *s, const char *sysId, char *tag, size_t n)
{
    int nTags = 0;
    const char *const *tags = Version_Tags(sysId, &nTags);
    size_t len;
    int best = -1;
    size_t bestLen = 0;
    if (!s || !tags || nTags <= 0) return false;
    Version_CleanSpaces(s);
    len = strlen(s);
    if (!len) return false;
    for (int i = 0; i < nTags; i++) {
        char foldT[80], suf[160];
        size_t tl, k, start;
        Version_TagFold(tags[i], foldT, sizeof(foldT));
        tl = strlen(tags[i]);
        if (tl == 0 || tl > len) continue;
        /* Compare a suffix of s, allowing loose punctuation in the tag. */
        /* Walk back from end, folding as we go, until fold matches or fails. */
        k = len;
        {
            char acc[160];
            size_t u = 0;
            int found = 0;
            while (k > 0) {
                unsigned char c = (unsigned char)s[k - 1];
                k--;
                if (c == ' ' || c == '+' || c == '-' || c == '_' || c == '.') continue;
                if (!isalnum(c)) break;
                if (u + 1 >= sizeof(acc)) break;
                memmove(acc + 1, acc, u);
                acc[0] = (char)tolower(c);
                u++;
                acc[u] = '\0';
                if (strcmp(acc, foldT) == 0) { found = 1; break; }
                if (u > strlen(foldT)) break;
            }
            if (!found) continue;
            start = k; /* first char of the matched tag (after walking into it) */
            /* must be at start or preceded by space / dash */
            while (start > 0 && (s[start] == ' ' || s[start] == '+' || s[start] == '-'
                                 || s[start] == '_' || s[start] == '.'))
                start++; /* this is wrong; k already points at last unconsumed */
            /* k is the index of the first character consumed of the tag. */
            start = k;
            if (start > 0 && !isspace((unsigned char)s[start - 1])
                && s[start - 1] != '-' && s[start - 1] != '(' && s[start - 1] != '[')
                continue;
            if (strlen(foldT) > bestLen) {
                best = i;
                bestLen = strlen(foldT);
                (void)suf;
            }
        }
    }
    if (best < 0) return false;
    /* Recompute start for the winner and cut. */
    {
        char foldT[80], acc[160];
        size_t u = 0, k = len, start;
        int found = 0;
        Version_TagFold(tags[best], foldT, sizeof(foldT));
        while (k > 0) {
            unsigned char c = (unsigned char)s[k - 1];
            k--;
            if (c == ' ' || c == '+' || c == '-' || c == '_' || c == '.') continue;
            if (!isalnum(c)) break;
            if (u + 1 >= sizeof(acc)) break;
            memmove(acc + 1, acc, u);
            acc[0] = (char)tolower(c);
            u++;
            acc[u] = '\0';
            if (strcmp(acc, foldT) == 0) { found = 1; break; }
        }
        if (!found) return false;
        start = k;
        /* drop a separating space/dash before the tag */
        while (start > 0 && (s[start - 1] == ' ' || s[start - 1] == '-')) start--;
        if (tag && n) Version_Copy(tag, n, tags[best]);
        s[start] = '\0';
        Version_CleanSpaces(s);
        return true;
    }
}

static void Version_ParseOne(const char *src, const char *sysId, char *base, size_t baseN,
                             VersionBits *bits)
{
    char work[512];
    char keep[256];
    char tag[VERSION_NAME];
    int guard = 0;
    keep[0] = '\0';
    Version_Copy(work, sizeof(work), src ? src : "");
    Version_StripExt(work);
    Version_CleanSpaces(work);
    while (guard++ < 24) {
        size_t len = strlen(work);
        if (len >= 2 && (work[0] == '(' || work[0] == '[')) {
            char open = work[0], close = (open == '(') ? ')' : ']';
            char *end = strchr(work + 1, close);
            if (end && (end[1] == '\0' || end[1] == ' ' || end[1] == '(' || end[1] == '[')) {
                size_t inner = (size_t)(end - (work + 1));
                if (inner >= sizeof(tag)) break;
                memcpy(tag, work + 1, inner);
                tag[inner] = '\0';
                Version_CleanSpaces(tag);
                memmove(work, end + 1, strlen(end + 1) + 1);
                Version_CleanSpaces(work);
                if (Version_IsDiscTag(tag)) {
                    char tmp[512];
                    if (keep[0])
                        snprintf(tmp, sizeof(tmp), "(%s) %s", tag, keep);
                    else
                        snprintf(tmp, sizeof(tmp), "(%s)", tag);
                    Version_Copy(keep, sizeof(keep), tmp);
                } else if (!Version_Classify(tag, sysId, bits) && !Version_TakeHackName(tag, bits)) {
                    if (tag[0] && !bits->hackName[0]) {
                        if (!bits->extra[0]) Version_Copy(bits->extra, sizeof(bits->extra), tag);
                        else Version_AddPart(bits->extra, sizeof(bits->extra), tag);
                    }
                }
                continue;
            }
        }
        if (Version_TakeTrailingBracket(work, tag, sizeof(tag))) {
            if (Version_IsDiscTag(tag)) {
                char tmp[256];
                if (keep[0])
                    snprintf(tmp, sizeof(tmp), "(%s) %s", tag, keep);
                else
                    snprintf(tmp, sizeof(tmp), "(%s)", tag);
                Version_Copy(keep, sizeof(keep), tmp);
            } else if (!Version_Classify(tag, sysId, bits) && !Version_TakeHackName(tag, bits)) {
                if (tag[0] && !bits->hackName[0]) {
                    if (!bits->extra[0]) Version_Copy(bits->extra, sizeof(bits->extra), tag);
                    else Version_AddPart(bits->extra, sizeof(bits->extra), tag);
                }
            }
            continue;
        }
        if (Version_TakeTrailingSys(work, sysId, tag, sizeof(tag))) {
            Version_Classify(tag, sysId, bits);
            continue;
        }
        break;
    }
    Version_CleanSpaces(work);
    {
        size_t n = strlen(work);
        while (n && (work[n - 1] == '-' || work[n - 1] == ':' || work[n - 1] == ','
                     || work[n - 1] == '/')) {
            work[--n] = '\0';
            Version_CleanSpaces(work);
            n = strlen(work);
        }
    }
    if (keep[0]) {
        char tmp[512];
        if (work[0]) snprintf(tmp, sizeof(tmp), "%s %s", work, keep);
        else Version_Copy(tmp, sizeof(tmp), keep);
        Version_Copy(work, sizeof(work), tmp);
        Version_CleanSpaces(work);
    }
    /* (Hack) + leftover unclassified extra → hack name. */
    if (bits->hack && !bits->hackName[0] && bits->extra[0]) {
        Version_Copy(bits->hackName, sizeof(bits->hackName), bits->extra);
        bits->extra[0] = '\0';
    }
    Version_Copy(base, baseN, work);
    Version_CleanSpaces(base);
}

static void Version_BuildLabel(const VersionBits *b, const char *labelOverride,
                               char *dst, size_t n)
{
    if (labelOverride && labelOverride[0]) {
        Version_Copy(dst, n, labelOverride);
        return;
    }
    dst[0] = '\0';
    if (b->hackName[0]) {
        Version_Copy(dst, n, b->hackName);
        return;
    }
    Version_AddPart(dst, n, b->region);
    Version_AddPart(dst, n, b->rev);
    Version_AddPart(dst, n, b->rere);
    Version_AddPart(dst, n, b->beta);
    Version_AddPart(dst, n, b->patch);
    Version_AddPart(dst, n, b->extra);
    if (b->hack && !b->hackName[0]) Version_AddPart(dst, n, "Hack");
    if (!dst[0]) Version_Copy(dst, n, "Original");
}

void Version_Parse(const char *title, const char *rom, const char *sysId,
                   const char *groupOverride, const char *labelOverride,
                   VersionInfo *out)
{
    VersionBits bits;
    char baseT[VERSION_NAME], baseR[VERSION_NAME], romBase[256];
    VersionBits bitsR;
    if (!out) return;
    memset(out, 0, sizeof(*out));
    memset(&bits, 0, sizeof(bits));
    Version_ParseOne(title, sysId, baseT, sizeof(baseT), &bits);
    if (rom && rom[0] && Version_Basename(rom, romBase, sizeof(romBase)) && romBase[0]) {
        memset(&bitsR, 0, sizeof(bitsR));
        Version_ParseOne(romBase, sysId, baseR, sizeof(baseR), &bitsR);
        /* Merge tags from the ROM name; keep the title's base when present. */
        Version_AddRegion(bits.region, sizeof(bits.region), bitsR.region);
        if (!bits.rev[0]) Version_Copy(bits.rev, sizeof(bits.rev), bitsR.rev);
        Version_AddPart(bits.patch, sizeof(bits.patch), bitsR.patch);
        Version_FoldPatches(bits.patch, sizeof(bits.patch));
        Version_AddPart(bits.extra, sizeof(bits.extra), bitsR.extra);
        if (!bits.rere[0]) Version_Copy(bits.rere, sizeof(bits.rere), bitsR.rere);
        if (!bits.beta[0]) Version_Copy(bits.beta, sizeof(bits.beta), bitsR.beta);
        if (bitsR.hack) bits.hack = true;
        if (bitsR.patched) bits.patched = true;
        if (!bits.hackName[0]) Version_Copy(bits.hackName, sizeof(bits.hackName), bitsR.hackName);
        if (!baseT[0]) Version_Copy(baseT, sizeof(baseT), baseR);
    }
    if (groupOverride && groupOverride[0]) Version_Copy(out->base, sizeof(out->base), groupOverride);
    else Version_Copy(out->base, sizeof(out->base), baseT[0] ? baseT : (title ? title : ""));
    Version_MakeKey(out->base, out->key, sizeof(out->key));
    Version_BuildLabel(&bits, labelOverride, out->label, sizeof(out->label));
    Version_Copy(out->region, sizeof(out->region), bits.region);
    Version_Copy(out->patch, sizeof(out->patch), bits.patch);
    out->hack = bits.hack || bits.hackName[0];
    out->patched = bits.patched || bits.hack || (bits.patch[0] != '\0') || (bits.extra[0] != '\0');
}

bool Version_IsUsa(const VersionInfo *v)
{
    return v && v->region[0] && strstr(v->region, "USA") != NULL;
}

bool Version_IsUnpatched(const VersionInfo *v)
{
    return v && !v->patched && !v->hack && !v->patch[0];
}

static bool Version_IsHackOrMod(const VersionInfo *v)
{
    return v && v->hack;
}

/* USA > World > Europe > Japan > other. "USA, Europe" counts as USA. */
static int Version_RegionRank(const VersionInfo *v)
{
    if (!v || !v->region[0]) return 0;
    if (strstr(v->region, "USA")) return 4;
    if (strstr(v->region, "World")) return 3;
    if (strstr(v->region, "Europe")) return 2;
    if (strstr(v->region, "Japan")) return 1;
    return 0;
}

static void Version_AppendTag(char *dst, size_t n, char open, const char *s)
{
    size_t used, cap, i;
    if (!dst || n == 0 || !s || !s[0]) return;
    used = strlen(dst);
    if (used + 4 >= n) return;
    dst[used++] = ' ';
    dst[used++] = open;
    cap = n - 2; /* room for closer and NUL */
    for (i = 0; s[i] && used < cap; i++) dst[used++] = s[i];
    if (used < n - 1) dst[used++] = (open == '[' ? ']' : ')');
    dst[used] = '\0';
}

void Version_FormatDisplay(const char *title, const VersionInfo *v,
                           char *dst, size_t n)
{
    const char *t = (title && title[0]) ? title : (v && v->base[0] ? v->base : "");
    const char *p;
    Version_Copy(dst, n, t);
    if (!v || !v->label[0] || strcmp(v->label, "Original") == 0) return;
    /* Named hacks use the hack name as the whole label. */
    if (v->hack && strcmp(v->label, "Hack") != 0 && strstr(v->label, " · ") == NULL) {
        if (v->region[0]) Version_AppendTag(dst, n, '(', v->region);
        Version_AppendTag(dst, n, '[', v->label);
        return;
    }
    p = v->label;
    while (*p) {
        const char *sep = strstr(p, " · ");
        size_t len = sep ? (size_t)(sep - p) : strlen(p);
        char part[VERSION_NAME];
        int bracket;
        if (len >= sizeof(part)) len = sizeof(part) - 1;
        memcpy(part, p, len);
        part[len] = '\0';
        bracket = (strcmp(part, "Hack") == 0)
            || (part[0] == 'T' && (part[1] == '+' || part[1] == '-'));
        Version_AppendTag(dst, n, bracket ? '[' : '(', part);
        p = sep ? sep + 3 : p + len;
    }
}

int Version_Prefer(const VersionInfo *v, const bool *playable, int n,
                   const char *savedLabel)
{
    int best = 0;
    if (!v || n <= 0) return 0;
    if (savedLabel && savedLabel[0]) {
        for (int i = 0; i < n; i++) {
            if (strcmp(v[i].label, savedLabel) != 0) continue;
            if (playable && !playable[i]) break;
            return i;
        }
    }
    for (int i = 1; i < n; i++) {
        bool pI = playable ? playable[i] : false;
        bool pB = playable ? playable[best] : false;
        int rI, rB;
        if (pI != pB) { if (pI) best = i; continue; }
        if (Version_IsHackOrMod(&v[i]) != Version_IsHackOrMod(&v[best])) {
            if (!Version_IsHackOrMod(&v[i])) best = i;
            continue;
        }
        rI = Version_RegionRank(&v[i]);
        rB = Version_RegionRank(&v[best]);
        if (rI != rB) { if (rI > rB) best = i; continue; }
        if (Version_IsUnpatched(&v[i]) != Version_IsUnpatched(&v[best])) {
            if (Version_IsUnpatched(&v[i])) best = i;
            continue;
        }
    }
    return best;
}

static size_t Version_LineLen(const char *p, const char **next)
{
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    *next = e ? e + 1 : NULL;
    while (len && p[len - 1] == '\r') len--;
    return len;
}

const char *Version_SelGet(const char *text, const char *groupTitle, const char *groupKey)
{
    if (!text) return NULL;
    for (const char *p = text; p && *p; ) {
        const char *next, *tab;
        size_t len = Version_LineLen(p, &next);
        tab = NULL;
        for (size_t i = 0; i < len; i++) if (p[i] == '\t') { tab = p + i; break; }
        if (tab) {
            size_t gl = (size_t)(tab - p);
            const char *lab = tab + 1;
            if (groupTitle && strlen(groupTitle) == gl && strncmp(p, groupTitle, gl) == 0)
                return lab; /* until EOL; caller should copy — see below */
            if (groupKey && strlen(groupKey) == gl && strncmp(p, groupKey, gl) == 0)
                return lab;
            (void)lab;
        }
        p = next;
    }
    return NULL;
}

/* Version_SelGet returns a pointer into `text` that may run past the label
 * (it includes up to the next newline). Copy helpers use a bounded extract. */
static void Version_SelCopyLabel(const char *p, char *dst, size_t n)
{
    size_t len = 0;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!p) return;
    while (p[len] && p[len] != '\n' && p[len] != '\r' && p[len] != '\t') len++;
    if (len >= n) len = n - 1;
    memcpy(dst, p, len);
    dst[len] = '\0';
}

int Version_SelSet(const char *text, const char *groupTitle, const char *label,
                   char *out, size_t n)
{
    size_t used = 0;
    bool written = false;
    char *owned = NULL;
    const char *p;
    if (!out || n == 0) return -1;
    if (text && text >= out && text < out + n) {
        owned = (char *)malloc(strlen(text) + 1);
        if (!owned) { out[0] = '\0'; free(owned); return -1; }
        memcpy(owned, text, strlen(text) + 1);
        text = owned;
    }
    p = text ? text : "";
    out[0] = '\0';
    if (!groupTitle || !groupTitle[0] || !label || !label[0]) { free(owned); return 0; }
    while (p && *p) {
        const char *next, *tab;
        size_t len = Version_LineLen(p, &next);
        tab = NULL;
        for (size_t i = 0; i < len; i++) if (p[i] == '\t') { tab = p + i; break; }
        if (tab && (size_t)(tab - p) == strlen(groupTitle)
            && strncmp(p, groupTitle, (size_t)(tab - p)) == 0) {
            if (!written) {
                int w = snprintf(out + used, n - used, "%s\t%s\n", groupTitle, label);
                if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; free(owned); return -1; }
                used += (size_t)w;
                written = true;
            }
        } else if (len) {
            int w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
            if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; free(owned); return -1; }
            used += (size_t)w;
        }
        p = next;
    }
    if (!written) {
        int w = snprintf(out + used, n - used, "%s\t%s\n", groupTitle, label);
        if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; free(owned); return -1; }
    }
    free(owned);
    return 0;
}

/* Export label copy for callers that use Version_SelGet. */
void Version_SelLabelCopy(const char *p, char *dst, size_t n);
void Version_SelLabelCopy(const char *p, char *dst, size_t n)
{
    Version_SelCopyLabel(p, dst, n);
}


#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *Version_DotExt(const char *name)
{
    const char *base, *dot;
    if (!name || !name[0]) return NULL;
    base = strrchr(name, '/');
    base = base ? base + 1 : name;
    dot = strrchr(base, '.');
    return (dot && dot != base) ? dot : NULL;
}

bool Version_IsRomExt(const char *name)
{
    const char *e = Version_DotExt(name);
    static const char *const ext[] = {
        ".sfc", ".smc", ".swc", ".fig", ".nes", ".fds", ".unf", ".unif",
        ".gen", ".md", ".smd", ".32x", ".bin", ".z64", ".n64", ".v64",
        ".cue", ".chd", ".iso", ".gcm", NULL
    };
    if (!e) return false;
    for (int i = 0; ext[i]; i++) if (Version_StartsFold(e, ext[i]) && e[strlen(ext[i])] == '\0') return true;
    return false;
}

bool Version_IsPatchExt(const char *name)
{
    const char *e = Version_DotExt(name);
    if (!e) return false;
    return (Version_StartsFold(e, ".ips") && e[4] == '\0')
        || (Version_StartsFold(e, ".bps") && e[4] == '\0')
        || (Version_StartsFold(e, ".ups") && e[4] == '\0');
}

void Version_FileName(const char *path, char *dst, size_t n)
{
    const char *base;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!path || !path[0]) return;
    base = strrchr(path, '/');
    if (!base) base = strrchr(path, '\\');
    snprintf(dst, n, "%s", base ? base + 1 : path);
}

void Version_FileStem(const char *path, char *dst, size_t n)
{
    char *dot;
    Version_FileName(path, dst, n);
    if (!dst[0]) return;
    dot = strrchr(dst, '.');
    if (dot && dot != dst) *dot = '\0';
}

void Version_PrettifyFolder(const char *folder, char *dst, size_t n)
{
    char tmp[VERSION_NAME];
    size_t o = 0;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!folder) return;
    for (size_t i = 0; folder[i] && o + 2 < sizeof tmp; i++) {
        unsigned char c = (unsigned char)folder[i];
        unsigned char p = o ? (unsigned char)tmp[o - 1] : 0;
        if (c == '_' || c == '-') c = ' ';
        if (o && tmp[o - 1] != ' ') {
            if ((islower(p) && isupper(c))
                || (isalpha(p) && isdigit(c))
                || (isdigit(p) && isalpha(c)))
                tmp[o++] = ' ';
        }
        if (c == ' ' && (o == 0 || tmp[o - 1] == ' ')) continue;
        tmp[o++] = (char)c;
    }
    while (o && tmp[o - 1] == ' ') o--;
    tmp[o] = '\0';
    snprintf(dst, n, "%s", tmp);
}

void Version_RewriteRomPrefix(const char *rel, char *dst, size_t n)
{
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (!rel) return;
    if (strncmp(rel, "repos/roms/", 11) == 0)
        snprintf(dst, n, "%s/%s", VERSION_ROM_ROOT, rel + 11);
    else
        snprintf(dst, n, "%s", rel);
}

int Version_ResolveRel(const char *rel, char *dst, size_t n,
                       int (*exists)(const char *rel, void *u), void *u)
{
    char alt[1200];
    if (!dst || n == 0) return 0;
    dst[0] = '\0';
    if (!rel || !rel[0]) return 0;
    if (!exists) {
        Version_RewriteRomPrefix(rel, dst, n);
        return dst[0] != '\0';
    }
    if (exists(rel, u)) { snprintf(dst, n, "%s", rel); return 1; }
    Version_RewriteRomPrefix(rel, alt, sizeof(alt));
    if (alt[0] && strcmp(alt, rel) != 0 && exists(alt, u)) {
        snprintf(dst, n, "%s", alt);
        return 1;
    }
    return 0;
}

static int Version_ScanPush(VersionScanItem *out, int cap, int n,
                            const char *folder, const char *name, const char *rel, int isPatch)
{
    if (!out || n < 0 || n >= cap) return n;
    memset(&out[n], 0, sizeof(out[n]));
    if (folder) snprintf(out[n].folder, sizeof(out[n].folder), "%s", folder);
    snprintf(out[n].name, sizeof(out[n].name), "%s", name ? name : "");
    snprintf(out[n].rel, sizeof(out[n].rel), "%s", rel ? rel : "");
    out[n].isPatch = isPatch;
    return n + 1;
}

int Version_ScanDir(const char *absSysDir, const char *sysFolder,
                    VersionScanItem *out, int cap)
{
    DIR *d;
    struct dirent *de;
    int n = 0;
    if (!absSysDir || !out || cap <= 0) return 0;
    if (!sysFolder) sysFolder = "";
    d = opendir(absSysDir);
    if (!d) return 0;
    while ((de = readdir(d)) != NULL && n < cap) {
        char child[4096];
        struct stat st;
        const char *name = de->d_name;
        if (!name[0] || name[0] == '.') continue;
        snprintf(child, sizeof(child), "%s/%s", absSysDir, name);
        if (stat(child, &st) != 0) continue;
        if (S_ISDIR(st.st_mode)) {
            DIR *sub = opendir(child);
            struct dirent *se;
            if (!sub) continue;
            while ((se = readdir(sub)) != NULL && n < cap) {
                char rel[768], leaf[4352];
                struct stat st2;
                const char *fn = se->d_name;
                int patch, rom;
                if (!fn[0] || fn[0] == '.') continue;
                snprintf(leaf, sizeof(leaf), "%s/%s", child, fn);
                if (stat(leaf, &st2) != 0 || !S_ISREG(st2.st_mode)) continue;
                patch = Version_IsPatchExt(fn);
                rom = Version_IsRomExt(fn);
                if (!patch && !rom) continue;
                snprintf(rel, sizeof(rel), "%s/%s/%s/%s", VERSION_ROM_ROOT, sysFolder, name, fn);
                n = Version_ScanPush(out, cap, n, name, fn, rel, patch);
            }
            closedir(sub);
        } else if (S_ISREG(st.st_mode)) {
            char rel[768];
            int patch = Version_IsPatchExt(name);
            int rom = Version_IsRomExt(name);
            if (!patch && !rom) continue;
            snprintf(rel, sizeof(rel), "%s/%s/%s", VERSION_ROM_ROOT, sysFolder, name);
            n = Version_ScanPush(out, cap, n, "", name, rel, patch);
        }
    }
    closedir(d);
    return n;
}

static int Version_ReadAll(const char *path, uint8_t **data, size_t *len)
{
    FILE *f;
    long sz;
    uint8_t *buf;
    *data = NULL;
    *len = 0;
    f = fopen(path, "rb");
    if (!f) return -1;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return -1; }
    sz = ftell(f);
    if (sz < 0 || sz > 512 * 1024 * 1024) { fclose(f); return -1; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return -1; }
    buf = (uint8_t *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return -1; }
    if (sz > 0 && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        free(buf); fclose(f); return -1;
    }
    fclose(f);
    buf[sz] = 0;
    *data = buf;
    *len = (size_t)sz;
    return 0;
}

static int Version_WriteAll(const char *path, const uint8_t *data, size_t len)
{
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    if (len && fwrite(data, 1, len, f) != len) { fclose(f); return -1; }
    if (fclose(f) != 0) return -1;
    return 0;
}

static int Version_MkdirP(const char *dir)
{
    char tmp[2400];
    size_t i, n;
    if (!dir || !dir[0]) return -1;
    snprintf(tmp, sizeof(tmp), "%s", dir);
    n = strlen(tmp);
    while (n && tmp[n - 1] == '/') tmp[--n] = '\0';
    for (i = 1; i <= n; i++) {
        if (tmp[i] == '/' || tmp[i] == '\0') {
            char c = tmp[i];
            tmp[i] = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST) return -1;
            tmp[i] = c;
        }
    }
    return 0;
}

static int Version_CopyFile(const char *src, const char *dst)
{
    uint8_t *data;
    size_t len;
    int rc;
    if (Version_ReadAll(src, &data, &len) != 0) return -1;
    rc = Version_WriteAll(dst, data, len);
    free(data);
    return rc;
}

static int Version_LinkOrCopy(const char *src, const char *dst)
{
    unlink(dst);
    if (symlink(src, dst) == 0) return 0;
    return Version_CopyFile(src, dst);
}

int Version_ApplyIps(const char *srcPath, const char *ipsPath, const char *dstPath)
{
    uint8_t *src = NULL, *ips = NULL, *out = NULL;
    size_t srcN = 0, ipsN = 0, outN, i;
    int rc = -1;
    if (Version_ReadAll(srcPath, &src, &srcN) != 0) goto done;
    if (Version_ReadAll(ipsPath, &ips, &ipsN) != 0) goto done;
    if (ipsN < 8 || memcmp(ips, "PATCH", 5) != 0) goto done;
    outN = srcN;
    out = (uint8_t *)malloc(outN ? outN : 1);
    if (!out) goto done;
    if (srcN) memcpy(out, src, srcN);
    i = 5;
    while (i + 3 <= ipsN) {
        uint32_t off;
        uint16_t size;
        if (i + 3 <= ipsN && memcmp(ips + i, "EOF", 3) == 0) {
            i += 3;
            if (i + 3 <= ipsN) {
                uint32_t trunc = ((uint32_t)ips[i] << 16) | ((uint32_t)ips[i + 1] << 8) | ips[i + 2];
                if (trunc < outN) outN = trunc;
            }
            break;
        }
        off = ((uint32_t)ips[i] << 16) | ((uint32_t)ips[i + 1] << 8) | ips[i + 2];
        i += 3;
        if (i + 2 > ipsN) goto done;
        size = (uint16_t)((ips[i] << 8) | ips[i + 1]);
        i += 2;
        if (size == 0) {
            uint16_t count;
            uint8_t val;
            if (i + 3 > ipsN) goto done;
            count = (uint16_t)((ips[i] << 8) | ips[i + 1]);
            val = ips[i + 2];
            i += 3;
            if ((size_t)off + count > outN) {
                uint8_t *nbuf = (uint8_t *)realloc(out, (size_t)off + count);
                if (!nbuf) goto done;
                memset(nbuf + outN, 0, (size_t)off + count - outN);
                out = nbuf;
                outN = (size_t)off + count;
            }
            memset(out + off, val, count);
        } else {
            if (i + size > ipsN) goto done;
            if ((size_t)off + size > outN) {
                uint8_t *nbuf = (uint8_t *)realloc(out, (size_t)off + size);
                if (!nbuf) goto done;
                memset(nbuf + outN, 0, (size_t)off + size - outN);
                out = nbuf;
                outN = (size_t)off + size;
            }
            memcpy(out + off, ips + i, size);
            i += size;
        }
    }
    if (Version_WriteAll(dstPath, out, outN) == 0) rc = 0;
done:
    free(src);
    free(ips);
    free(out);
    return rc;
}

static uint32_t Version_Crc32(const uint8_t *p, size_t n)
{
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < n; i++) {
        c ^= p[i];
        for (int b = 0; b < 8; b++)
            c = (c >> 1) ^ (0xEDB88320u & (uint32_t)(-(int)(c & 1)));
    }
    return ~c;
}

static int Version_BpsVli(const uint8_t *p, size_t n, size_t *i, uint64_t *out)
{
    uint64_t shift = 1, v = 0;
    for (;;) {
        uint8_t x;
        if (*i >= n) return -1;
        x = p[(*i)++];
        v += (uint64_t)(x & 0x7f) * shift;
        if (x & 0x80) { *out = v; return 0; }
        shift <<= 7;
        v += shift;
    }
}

int Version_ApplyBps(const char *srcPath, const char *bpsPath, const char *dstPath)
{
    uint8_t *src = NULL, *bps = NULL, *out = NULL;
    size_t srcN = 0, bpsN = 0, i = 4;
    uint64_t srcSize = 0, tgtSize = 0, meta = 0;
    int64_t srcRel = 0, tgtRel = 0;
    size_t o = 0;
    int rc = -1;
    if (Version_ReadAll(srcPath, &src, &srcN) != 0) goto done;
    if (Version_ReadAll(bpsPath, &bps, &bpsN) != 0) goto done;
    if (bpsN < 16 || memcmp(bps, "BPS1", 4) != 0) goto done;
    if (Version_BpsVli(bps, bpsN, &i, &srcSize) != 0) goto done;
    if (Version_BpsVli(bps, bpsN, &i, &tgtSize) != 0) goto done;
    if (Version_BpsVli(bps, bpsN, &i, &meta) != 0) goto done;
    if (srcSize != srcN) goto done;
    if (i + meta + 12 > bpsN) goto done;
    i += (size_t)meta;
    if (tgtSize > 512 * 1024 * 1024) goto done;
    out = (uint8_t *)calloc((size_t)tgtSize ? (size_t)tgtSize : 1, 1);
    if (!out) goto done;
    while (i < bpsN - 12) {
        uint64_t data = 0, length;
        int action;
        if (Version_BpsVli(bps, bpsN - 12, &i, &data) != 0) goto done;
        action = (int)(data & 3);
        length = (data >> 2) + 1;
        if (o + (size_t)length > (size_t)tgtSize) goto done;
        if (action == 0) { /* SourceRead */
            if (o + (size_t)length > srcN) goto done;
            memcpy(out + o, src + o, (size_t)length);
            o += (size_t)length;
        } else if (action == 1) { /* TargetRead */
            if (i + (size_t)length > bpsN - 12) goto done;
            memcpy(out + o, bps + i, (size_t)length);
            i += (size_t)length;
            o += (size_t)length;
        } else if (action == 2 || action == 3) {
            uint64_t offv = 0;
            int64_t rel;
            if (Version_BpsVli(bps, bpsN - 12, &i, &offv) != 0) goto done;
            rel = (int64_t)(offv >> 1);
            if (offv & 1) rel = -rel;
            if (action == 2) {
                srcRel += rel;
                if (srcRel < 0 || (uint64_t)srcRel + length > srcN) goto done;
                memcpy(out + o, src + (size_t)srcRel, (size_t)length);
                srcRel += (int64_t)length;
                o += (size_t)length;
            } else {
                tgtRel += rel;
                if (tgtRel < 0) goto done;
                for (uint64_t k = 0; k < length; k++) {
                    if ((uint64_t)tgtRel + k >= o + k && (uint64_t)tgtRel + k >= (size_t)tgtSize)
                        goto done;
                    out[o + (size_t)k] = out[(size_t)tgtRel + (size_t)k];
                }
                tgtRel += (int64_t)length;
                o += (size_t)length;
            }
        }
    }
    if (o != (size_t)tgtSize) goto done;
    if (Version_WriteAll(dstPath, out, (size_t)tgtSize) == 0) rc = 0;
    (void)Version_Crc32;
done:
    free(src);
    free(bps);
    free(out);
    return rc;
}

int Version_ApplyUps(const char *srcPath, const char *upsPath, const char *dstPath)
{
    uint8_t *src = NULL, *ups = NULL, *out = NULL;
    size_t srcN = 0, upsN = 0, i = 4, o;
    uint64_t srcSize = 0, tgtSize = 0;
    int rc = -1;
    if (Version_ReadAll(srcPath, &src, &srcN) != 0) goto done;
    if (Version_ReadAll(upsPath, &ups, &upsN) != 0) goto done;
    if (upsN < 16 || memcmp(ups, "UPS1", 4) != 0) goto done;
    if (Version_BpsVli(ups, upsN, &i, &srcSize) != 0) goto done;
    if (Version_BpsVli(ups, upsN, &i, &tgtSize) != 0) goto done;
    if (srcSize != srcN) goto done;
    if (tgtSize > 512 * 1024 * 1024) goto done;
    out = (uint8_t *)calloc((size_t)tgtSize ? (size_t)tgtSize : 1, 1);
    if (!out) goto done;
    memcpy(out, src, srcN < (size_t)tgtSize ? srcN : (size_t)tgtSize);
    o = 0;
    while (i + 12 < upsN) {
        uint64_t skip = 0;
        if (Version_BpsVli(ups, upsN - 12, &i, &skip) != 0) break;
        o += (size_t)skip;
        while (i < upsN - 12) {
            uint8_t x = ups[i++];
            if (o < (size_t)tgtSize) out[o] ^= x;
            if (x == 0) { o++; break; }
            o++;
        }
    }
    if (Version_WriteAll(dstPath, out, (size_t)tgtSize) == 0) rc = 0;
done:
    free(src);
    free(ups);
    free(out);
    return rc;
}

int Version_PreparePatchedRom(const char *srcAbs, const char *patchAbs,
                              const char *destDir, char *outAbs, size_t n)
{
    char leaf[256], destRom[2400], destPatch[2400];
    const char *ext, *pExt;
    if (!outAbs || n == 0) return -1;
    outAbs[0] = '\0';
    if (!srcAbs || !patchAbs || !destDir) return -1;
    if (Version_MkdirP(destDir) != 0) return -1;
    ext = Version_DotExt(srcAbs);
    if (!ext) ext = "";
    snprintf(leaf, sizeof(leaf), "launch%s", ext);
    snprintf(destRom, sizeof(destRom), "%s/%s", destDir, leaf);
    pExt = Version_DotExt(patchAbs);
    if (pExt && Version_StartsFold(pExt, ".ips") && pExt[4] == '\0') {
        char stem[256];
        Version_FileStem(leaf, stem, sizeof(stem));
        snprintf(destPatch, sizeof(destPatch), "%s/%s.ips", destDir, stem);
        if (Version_LinkOrCopy(srcAbs, destRom) != 0) return -1;
        if (Version_LinkOrCopy(patchAbs, destPatch) != 0) return -1;
        snprintf(outAbs, n, "%s", destRom);
        return 0;
    }
    if (pExt && Version_StartsFold(pExt, ".bps") && pExt[4] == '\0') {
        unlink(destRom);
        if (Version_ApplyBps(srcAbs, patchAbs, destRom) != 0) return -1;
        snprintf(outAbs, n, "%s", destRom);
        return 0;
    }
    if (pExt && Version_StartsFold(pExt, ".ups") && pExt[4] == '\0') {
        unlink(destRom);
        if (Version_ApplyUps(srcAbs, patchAbs, destRom) != 0) return -1;
        snprintf(outAbs, n, "%s", destRom);
        return 0;
    }
    return -1;
}


#endif /* VERSION_IMPLEMENTATION_ONCE */
#endif /* VERSION_IMPLEMENTATION */
