/* version_ui.h - one grid tile per loose ROM or per-game folder; included by
 * main.c after GameIsLaunchable and before sortpanel.h / Order_LoadAll. */
#ifndef NSO_VERSION_UI_H
#define NSO_VERSION_UI_H

#include <strings.h>

#define VERSION_MAX_MEMBERS 32

typedef struct {
    char key[VERSION_NAME];
    char title[VERSION_NAME];
    int raw[VERSION_MAX_MEMBERS];
    int n;
    int sel;
} VerGroup;

static const GameEntry *verSrc[sizeof(systems) / sizeof(systems[0])];
static const int *verSrcN[sizeof(systems) / sizeof(systems[0])];
static GameEntry verLive[sizeof(systems) / sizeof(systems[0])][MAX_GAMES];
static int verCount[sizeof(systems) / sizeof(systems[0])];
static VerGroup verGrp[sizeof(systems) / sizeof(systems[0])][MAX_GAMES];
static int verGrpN[sizeof(systems) / sizeof(systems[0])];
static bool verReady[sizeof(systems) / sizeof(systems[0])];
static char *verSelText[sizeof(systems) / sizeof(systems[0])];

/* Writable catalog snapshot + scanned folder/loose extras. */
static GameEntry verRaw[sizeof(systems) / sizeof(systems[0])][MAX_GAMES];
static int verRawN[sizeof(systems) / sizeof(systems[0])];
static const GameEntry *verOrigin[sizeof(systems) / sizeof(systems[0])];
static const int *verOriginN[sizeof(systems) / sizeof(systems[0])];
static int verImported[sizeof(systems) / sizeof(systems[0])];
static char verTitle[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][VERSION_NAME];
static char verRom[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][768];
static char verState[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][320];
static char verPatch[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][768];
static char verBox[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][768];
static char verFolderKey[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][VERSION_NAME];
static char verFolderTitle[sizeof(systems) / sizeof(systems[0])][MAX_GAMES][VERSION_NAME];

static const char *Version_SysId(int sys)
{
    return (sys >= 0 && sys < SYSTEM_COUNT && systems[sys].platform)
        ? systems[sys].platform->id : "";
}

static void Version_SelPath(int sys, char *dst, size_t n)
{
    snprintf(dst, n, "%s/config/versions-%s.txt", GetBaseDirectory(), Version_SysId(sys));
}

static void Version_ParseGame(int sys, const GameEntry *g, VersionInfo *v)
{
    Version_Parse(g ? g->title : NULL, g ? g->rom : NULL, Version_SysId(sys),
                  NULL, NULL, v);
}

static const char *Version_MednafenMod(int sys)
{
    const char *id = Version_SysId(sys);
    if (strcmp(id, "snes") == 0) return "snes";
    if (strcmp(id, "nes") == 0) return "nes";
    if (strcmp(id, "megadrive") == 0) return "md";
    return NULL;
}

static int Version_RawByBasename(int sys, const char *name)
{
    char base[256], have[256];
    if (!name || !name[0]) return -1;
    Version_FileName(name, base, sizeof(base));
    for (int i = 0; i < verRawN[sys]; i++) {
        if (!verRaw[sys][i].rom || !verRaw[sys][i].rom[0]) continue;
        Version_FileName(verRaw[sys][i].rom, have, sizeof(have));
        if (strcasecmp(have, base) == 0) return i;
    }
    return -1;
}

static void Version_NormName(const char *s, char *dst, size_t n)
{
    if (dst && n) dst[0] = '\0';
    if (s && s[0]) Gs_Norm(s, dst, n);
}

static int Version_NormEq(const char *a, const char *b)
{
    char na[VERSION_NAME], nb[VERSION_NAME];
    Version_NormName(a, na, sizeof(na));
    Version_NormName(b, nb, sizeof(nb));
    return na[0] && nb[0] && strcmp(na, nb) == 0;
}

static void Version_RawNorm(int sys, int i, char *dst, size_t n)
{
    VersionInfo v;
    if (dst && n) dst[0] = '\0';
    if (sys < 0 || i < 0 || i >= verRawN[sys]) return;
    if (verFolderTitle[sys][i][0]) {
        Version_NormName(verFolderTitle[sys][i], dst, n);
        if (dst[0]) return;
    }
    Version_ParseGame(sys, &verRaw[sys][i], &v);
    if (v.base[0]) {
        Version_NormName(v.base, dst, n);
        if (dst[0]) return;
    }
    if (verRaw[sys][i].title) Version_NormName(verRaw[sys][i].title, dst, n);
}

static int Version_RawByBaseTitle(int sys, const char *base)
{
    VersionInfo v;
    char want[VERSION_NAME], have[VERSION_NAME];
    if (!base || !base[0]) return -1;
    Version_NormName(base, want, sizeof(want));
    if (!want[0]) return -1;
    for (int i = 0; i < verRawN[sys]; i++) {
        Version_ParseGame(sys, &verRaw[sys][i], &v);
        if (v.base[0]) {
            Version_NormName(v.base, have, sizeof(have));
            if (have[0] && strcmp(have, want) == 0) return i;
        }
        if (verRaw[sys][i].title) {
            Version_NormName(verRaw[sys][i].title, have, sizeof(have));
            if (have[0] && strcmp(have, want) == 0) return i;
        }
        if (verFolderTitle[sys][i][0]) {
            Version_NormName(verFolderTitle[sys][i], have, sizeof(have));
            if (have[0] && strcmp(have, want) == 0) return i;
        }
    }
    return -1;
}

static int Version_RomFileMissing(int sys, int i)
{
    char full[2304];
    if (i < 0 || i >= verRawN[sys] || !verRaw[sys][i].rom || !verRaw[sys][i].rom[0])
        return 1;
    snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), verRaw[sys][i].rom);
    return !FileExists(full);
}

/* Native / embed port: keep its launcher; a folder ROM becomes another version. */
static int Version_IsPort(int sys, int i)
{
    const GameEntry *g;
    if (i < 0 || i >= verRawN[sys]) return 0;
    g = &verRaw[sys][i];
    if (g->mednafen && g->mednafen[0]) return 0;
    if (g->embedRomKey && g->embedRomKey[0]) return 1;
    if (g->dir && g->dir[0] && g->binary && g->binary[0]) return 1;
    return 0;
}

static int Version_CanAttachRom(int sys, int i)
{
    return !Version_IsPort(sys, i);
}

static void Version_GuessCover(int sys, int i, const char *title)
{
    VersionInfo want, have;
    char slug[196], path[256], full[2304];
    if (i < 0 || i >= MAX_GAMES) return;
    if (verRaw[sys][i].boxartPath && verRaw[sys][i].boxartPath[0]) return;
    Version_ParseGame(sys, &verRaw[sys][i], &want);
    for (int j = 0; j < verRawN[sys]; j++) {
        if (j == i || !verRaw[sys][j].boxartPath || !verRaw[sys][j].boxartPath[0]) continue;
        Version_ParseGame(sys, &verRaw[sys][j], &have);
        if ((want.base[0] && have.base[0] && Version_NormEq(want.base, have.base))
            || (title && verRaw[sys][j].title && Version_NormEq(title, verRaw[sys][j].title))
            || (title && have.base[0] && Version_NormEq(title, have.base))) {
            snprintf(verBox[sys][i], sizeof(verBox[sys][i]), "%s", verRaw[sys][j].boxartPath);
            verRaw[sys][i].boxartPath = verBox[sys][i];
            return;
        }
    }
    if (!title || !title[0]) return;
    SysList_Slug(title, slug, sizeof(slug));
    if (!slug[0]) return;
    snprintf(path, sizeof(path), "assets/images/systems/%s/%s.png", Version_SysId(sys), slug);
    snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), path);
    if (FileExists(full)) {
        snprintf(verBox[sys][i], sizeof(verBox[sys][i]), "%s", path);
        verRaw[sys][i].boxartPath = verBox[sys][i];
        return;
    }
    /* Local files often use the catalog/wiki slug (a subtitle, "the-…" article,
     * "vs" for "versus"). Match this title against those names the same way
     * a scrape matches Named_Boxarts. */
    {
        char dir[2304];
        DIR *d;
        struct dirent *ent;
        const char *id = Version_SysId(sys);
        const char *names[MAX_GAMES];
        char *owned[MAX_GAMES];
        int n = 0, k;
        const char *hit;
        snprintf(dir, sizeof(dir), "%s/assets/images/systems/%s",
                 GetBaseDirectory(), id ? id : "");
        d = opendir(dir);
        if (!d) return;
        while ((ent = readdir(d)) != NULL && n < MAX_GAMES) {
            const char *fn = ent->d_name;
            size_t L = strlen(fn);
            if (L < 5) continue;
            if (fn[L - 4] != '.' ) continue;
            if (!(fn[L - 3] == 'p' || fn[L - 3] == 'P')
                || !(fn[L - 2] == 'n' || fn[L - 2] == 'N')
                || !(fn[L - 1] == 'g' || fn[L - 1] == 'G'))
                continue;
            if (id && strncmp(fn, id, strlen(id)) == 0 && strcmp(fn + strlen(id), ".png") == 0)
                continue;
            owned[n] = (char *)malloc(L + 1);
            if (!owned[n]) continue;
            memcpy(owned[n], fn, L + 1);
            names[n] = owned[n];
            n++;
        }
        closedir(d);
        hit = Scrape_MatchLoose(title, names, n);
        if (hit) {
            snprintf(path, sizeof(path), "assets/images/systems/%s/%s", id ? id : "", hit);
            snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), path);
            if (FileExists(full)) {
                snprintf(verBox[sys][i], sizeof(verBox[sys][i]), "%s", path);
                verRaw[sys][i].boxartPath = verBox[sys][i];
            }
        }
        for (k = 0; k < n; k++) free(owned[k]);
    }
}

static void Version_SetRom(int sys, int i, const char *rel)
{
    if (i < 0 || i >= MAX_GAMES || !rel) return;
    snprintf(verRom[sys][i], sizeof(verRom[sys][i]), "%s", rel);
    verRaw[sys][i].rom = verRom[sys][i];
}

static int Version_AppendMednafen(int sys, const char *title, const char *romRel,
                                 const char *patchRel, const char *folderKey,
                                 const char *folderTitle)
{
    int i;
    const char *mod = Version_MednafenMod(sys);
    char stem[256];
    if (sys < 0 || sys >= SYSTEM_COUNT || !mod || verRawN[sys] >= MAX_GAMES) return -1;
    i = verRawN[sys]++;
    if (i < 0 || i >= MAX_GAMES) { verRawN[sys] = MAX_GAMES; return -1; }
    memset(&verRaw[sys][i], 0, sizeof(verRaw[sys][i]));
    snprintf(verTitle[sys][i], sizeof(verTitle[sys][i]), "%s", title ? title : "Unknown");
    Version_SetRom(sys, i, romRel ? romRel : "");
    verPatch[sys][i][0] = '\0';
    if (patchRel && patchRel[0])
        snprintf(verPatch[sys][i], sizeof(verPatch[sys][i]), "%s", patchRel);
    Version_FileName(romRel ? romRel : "", stem, sizeof(stem));
    snprintf(verState[sys][i], sizeof(verState[sys][i]),
             "saves/%s/mednafen/states/%s", mod, stem[0] ? stem : "game");
    verRaw[sys][i].title = verTitle[sys][i];
    verRaw[sys][i].dir = "mednafen/src";
    verRaw[sys][i].binary = "./mednafen";
    verRaw[sys][i].mednafen = mod;
    verRaw[sys][i].stateDir = verState[sys][i];
    verRaw[sys][i].externalSlots = true;
    verRaw[sys][i].orient = SLOT_ORIENT_INHERIT;
    if (folderKey && folderKey[0])
        snprintf(verFolderKey[sys][i], sizeof(verFolderKey[sys][i]), "%s", folderKey);
    if (folderTitle && folderTitle[0])
        snprintf(verFolderTitle[sys][i], sizeof(verFolderTitle[sys][i]), "%s", folderTitle);
    Version_GuessCover(sys, i, verTitle[sys][i]);
    return i;
}

static void Version_ApplyFolderKey(int sys, int i, const char *key, const char *title)
{
    if (i < 0 || i >= MAX_GAMES) return;
    if (key && key[0]) snprintf(verFolderKey[sys][i], sizeof(verFolderKey[sys][i]), "%s", key);
    if (title && title[0]) snprintf(verFolderTitle[sys][i], sizeof(verFolderTitle[sys][i]), "%s", title);
}

static void Version_FolderDisplay(int sys, const VersionScanItem *items, const int *idx, int n,
                                 const char *folderName, char *dst, size_t dstN)
{
    VersionInfo v;
    char common[VERSION_NAME] = "";
    int ok = 1;
    (void)sys;
    dst[0] = '\0';
    for (int k = 0; k < n; k++) {
        const VersionScanItem *it = &items[idx[k]];
        if (it->isPatch) continue;
        Version_Parse(NULL, it->rel, Version_SysId(sys), NULL, NULL, &v);
        if (!v.base[0]) { ok = 0; break; }
        if (!common[0]) snprintf(common, sizeof(common), "%s", v.base);
        else if (strcasecmp(common, v.base) != 0) { ok = 0; break; }
    }
    if (ok && common[0]) snprintf(dst, dstN, "%s", common);
    else Version_PrettifyFolder(folderName, dst, dstN);
}

static void Version_ScanSystemRoms(int sys)
{
    const char *folder = Gs_RomFolder(Version_SysId(sys));
    char abs[2112];
    VersionScanItem *items;
    int n, cap = MAX_GAMES;
    unsigned char *used;
    if (!folder) return;
    snprintf(abs, sizeof(abs), "%s/%s/%s", GetBaseDirectory(), Gs_RomRoot(), folder);
    items = (VersionScanItem *)calloc((size_t)cap, sizeof(*items));
    if (!items) return;
    n = Version_ScanDir(abs, folder, items, cap);
    used = (unsigned char *)calloc((size_t)(n > 0 ? n : 1), 1);
    if (!used) { free(items); return; }

    /* Per-game folders first: one group, each ROM or patch is a version. */
    for (int i = 0; i < n; i++) {
        char key[VERSION_NAME], title[VERSION_NAME];
        int idx[VERSION_MAX_MEMBERS], nn = 0, baseIdx = -1;
        if (used[i] || !items[i].folder[0]) continue;
        for (int j = i; j < n && nn < VERSION_MAX_MEMBERS; j++) {
            if (used[j] || strcmp(items[j].folder, items[i].folder) != 0) continue;
            idx[nn++] = j;
            used[j] = 1;
        }
        snprintf(key, sizeof(key), "folder:%s", items[i].folder);
        Version_FolderDisplay(sys, items, idx, nn, items[i].folder, title, sizeof(title));
        for (int k = 0; k < nn; k++) {
            const VersionScanItem *it = &items[idx[k]];
            if (it->isPatch) continue;
            int r = Version_RawByBasename(sys, it->name);
            VersionInfo v;
            memset(&v, 0, sizeof(v));
            Version_Parse(NULL, it->rel, Version_SysId(sys), NULL, NULL, &v);
            if (r < 0)
                r = Version_RawByBaseTitle(sys, v.base[0] ? v.base : title);
            if (r >= 0 && Version_CanAttachRom(sys, r)) {
                if (Version_RomFileMissing(sys, r))
                    Version_SetRom(sys, r, it->rel);
                Version_ApplyFolderKey(sys, r, key, title);
                if (!verRaw[sys][r].boxartPath || !verRaw[sys][r].boxartPath[0])
                    Version_GuessCover(sys, r, verRaw[sys][r].title ? verRaw[sys][r].title : title);
                if (baseIdx < 0) baseIdx = r;
            } else if (Version_MednafenMod(sys)) {
                const char *tn = v.base[0] ? v.base : title;
                int ni;
                if (r >= 0) Version_ApplyFolderKey(sys, r, key, title);
                ni = Version_AppendMednafen(sys, tn, it->rel, NULL, key, title);
                if (ni >= 0 && baseIdx < 0) baseIdx = ni;
            }
        }
        if (baseIdx < 0) {
            /* pick a previously attached folder member as the patch base */
            for (int r = 0; r < verRawN[sys]; r++)
                if (strcmp(verFolderKey[sys][r], key) == 0 && verRaw[sys][r].rom) {
                    baseIdx = r; break;
                }
        }
        for (int k = 0; k < nn; k++) {
            const VersionScanItem *it = &items[idx[k]];
            const char *baseRom;
            VersionInfo v;
            char labelTitle[256];
            int sameStem = -1;
            if (!it->isPatch) continue;
            if (baseIdx < 0) continue;
            baseRom = verRaw[sys][baseIdx].rom;
            {
                char pstem[256], rstem[256];
                Version_FileStem(it->name, pstem, sizeof(pstem));
                for (int r = 0; r < verRawN[sys]; r++) {
                    if (strcmp(verFolderKey[sys][r], key) != 0 || !verRaw[sys][r].rom) continue;
                    if (verPatch[sys][r][0]) continue;
                    Version_FileStem(verRaw[sys][r].rom, rstem, sizeof(rstem));
                    if (strcasecmp(pstem, rstem) == 0) { sameStem = r; break; }
                }
            }
            if (sameStem >= 0) baseRom = verRaw[sys][sameStem].rom;
            Version_Parse(NULL, it->rel, Version_SysId(sys), NULL, NULL, &v);
            snprintf(labelTitle, sizeof(labelTitle), "%s", title[0] ? title : (v.base[0] ? v.base : it->name));
            Version_AppendMednafen(sys, labelTitle, baseRom, it->rel, key, title);
        }
    }

    /* Loose ROMs: update a catalog path, or add a Mednafen tile unless a
     * same-title port/catalog game already represents that title. */
    for (int i = 0; i < n; i++) {
        VersionInfo v;
        int r, exist;
        if (used[i] || items[i].folder[0] || items[i].isPatch) continue;
        r = Version_RawByBasename(sys, items[i].name);
        if (r >= 0) {
            Version_SetRom(sys, r, items[i].rel);
            continue;
        }
        Version_Parse(NULL, items[i].rel, Version_SysId(sys), NULL, NULL, &v);
        exist = Version_RawByBaseTitle(sys, v.base[0] ? v.base : NULL);
        if (exist >= 0) {
            if (verRaw[sys][exist].rom && verRaw[sys][exist].rom[0]) {
                /* Port or catalog already owns this title; do not add a duplicate. */
                char full[2304];
                snprintf(full, sizeof(full), "%s/%s", GetBaseDirectory(), verRaw[sys][exist].rom);
                if (!FileExists(full)) Version_SetRom(sys, exist, items[i].rel);
            } else {
                Version_SetRom(sys, exist, items[i].rel);
            }
            continue;
        }
        if (Version_MednafenMod(sys)) {
            const char *tn = v.base[0] ? v.base : items[i].name;
            char pretty[VERSION_NAME];
            Version_PrettifyFolder(tn, pretty, sizeof(pretty));
            Version_AppendMednafen(sys, pretty[0] ? pretty : tn, items[i].rel, NULL, NULL, NULL);
        }
    }
    free(used);
    free(items);
}

static void Version_ImportOrigin(int sys)
{
    if (!verOrigin[sys] || !verOriginN[sys]) return;
    int n = *verOriginN[sys];
    if (n > MAX_GAMES) n = MAX_GAMES;
    while (verImported[sys] < n && verRawN[sys] < MAX_GAMES) {
        int i = verRawN[sys];
        int src = verImported[sys]++;
        char fixed[256];
        verRaw[sys][i] = verOrigin[sys][src];
        verFolderKey[sys][i][0] = '\0';
        verFolderTitle[sys][i][0] = '\0';
        verPatch[sys][i][0] = '\0';
        if (verRaw[sys][i].rom && verRaw[sys][i].rom[0]) {
            Version_RewriteRomPrefix(verRaw[sys][i].rom, fixed, sizeof(fixed));
            if (strcmp(fixed, verRaw[sys][i].rom) != 0)
                Version_SetRom(sys, i, fixed);
        }
        verRawN[sys]++;
    }
}

static int Version_GroupByTitle(int sys, const char *title)
{
    if (!title || !title[0] || sys < 0 || sys >= SYSTEM_COUNT) return -1;
    for (int i = 0; i < verGrpN[sys]; i++) {
        if (strcmp(verGrp[sys][i].title, title) == 0) return i;
        if (strcmp(verGrp[sys][i].key, title) == 0) return i;
    }
    return -1;
}

static bool Version_TextHas(int sys, const char *text, const char *groupTitle)
{
    int gi, srcN;
    if (Fav_Has(text, groupTitle)) return true;
    gi = Version_GroupByTitle(sys, groupTitle);
    if (gi < 0 || !verSrc[sys]) return false;
    srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    for (int m = 0; m < verGrp[sys][gi].n; m++) {
        int r = verGrp[sys][gi].raw[m];
        const char *t;
        if (r < 0 || r >= srcN) continue;
        t = verSrc[sys][r].title;
        if (t && Fav_Has(text, t)) return true;
    }
    return false;
}

/* After un-favoriting / un-hiding the group, also drop leftover member titles. */
static int Version_ClearAliases(int sys, const char *groupTitle, const char *text,
                                char *out, size_t n)
{
    int gi, srcN;
    static char tmp[MAX_GAMES * 256], cur[MAX_GAMES * 256];
    if (!out || n == 0) return -1;
    if (!text) text = "";
    if (strlen(text) >= sizeof(cur)) return -1;
    memcpy(cur, text, strlen(text) + 1);
    gi = Version_GroupByTitle(sys, groupTitle);
    if (gi < 0) {
        snprintf(out, n, "%s", cur);
        return 0;
    }
    srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    for (int m = 0; m < verGrp[sys][gi].n; m++) {
        int r = verGrp[sys][gi].raw[m];
        const char *t;
        if (r < 0 || r >= srcN || !verSrc[sys]) continue;
        t = verSrc[sys][r].title;
        if (!t || strcmp(t, groupTitle) == 0) continue;
        if (Fav_Write(cur, t, false, tmp, sizeof(tmp)) != 0) return -1;
        memcpy(cur, tmp, strlen(tmp) + 1);
    }
    if (strlen(cur) >= n) return -1;
    memcpy(out, cur, strlen(cur) + 1);
    return 0;
}

/* Rewrite an order file so member titles become the group title. */
static void Version_RewriteOrder(int sys, const char *text, char *out, size_t n)
{
    const char *p;
    size_t used = 0;
    if (!out || n == 0) return;
    out[0] = '\0';
    if (!text || sys < 0 || sys >= SYSTEM_COUNT) return;
    for (p = text; p && *p; ) {
        const char *eol = strchr(p, '\n');
        size_t len = eol ? (size_t)(eol - p) : strlen(p);
        while (len && p[len - 1] == '\r') len--;
        if (len) {
            char line[VERSION_NAME];
            const char *write = line;
            int matched = 0;
            if (len >= sizeof(line)) len = sizeof(line) - 1;
            memcpy(line, p, len);
            line[len] = '\0';
            for (int gi = 0; gi < verGrpN[sys] && !matched; gi++) {
                int srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
                if (strcmp(verGrp[sys][gi].title, line) == 0) { write = verGrp[sys][gi].title; matched = 1; break; }
                for (int m = 0; m < verGrp[sys][gi].n; m++) {
                    int r = verGrp[sys][gi].raw[m];
                    const char *t;
                    if (r < 0 || r >= srcN || !verSrc[sys]) continue;
                    t = verSrc[sys][r].title;
                    if (t && strcmp(t, line) == 0) { write = verGrp[sys][gi].title; matched = 1; break; }
                }
            }
            {
                int w = snprintf(out + used, n - used, "%s\n", write);
                if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; return; }
                used += (size_t)w;
            }
        }
        p = eol ? eol + 1 : NULL;
    }
}

static void Version_StatsAbsorb(int sys, const char *text, const char *const *titles,
                                SortKeys *keys, int count)
{
    if (!text || !keys || !titles) return;
    for (int i = 0; i < count; i++) {
        int gi, srcN;
        if (keys[i].lastPlayed || keys[i].playSecs) continue;
        gi = Version_GroupByTitle(sys, titles[i]);
        if (gi < 0 || !verSrc[sys]) continue;
        srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
        for (int m = 0; m < verGrp[sys][gi].n; m++) {
            int r = verGrp[sys][gi].raw[m];
            const char *t;
            const char *one[1];
            SortKeys tmp[1] = { 0 };
            if (r < 0 || r >= srcN) continue;
            t = verSrc[sys][r].title;
            if (!t) continue;
            one[0] = t;
            Sort_StatsParse(text, one, tmp, 1);
            if (tmp[0].lastPlayed || tmp[0].playSecs) {
                keys[i] = tmp[0];
                break;
            }
        }
    }
}

static void Version_SetSelText(int sys, const char *text);

static void Version_LoadSel(int sys)
{
    char path[2112];
    Version_SelPath(sys, path, sizeof(path));
    free(verSelText[sys]);
    verSelText[sys] = NULL;
    if (FileExists(path)) {
        char *text = LoadFileText(path);
        if (text) { Version_SetSelText(sys, text); UnloadFileText(text); }
    }
}

static void Version_SetSelText(int sys, const char *text)
{
    free(verSelText[sys]);
    verSelText[sys] = NULL;
    if (text && text[0]) {
        /* LoadFileText-owned pointers are freed with UnloadFileText; keep a malloc copy. */
        size_t n = strlen(text) + 1;
        verSelText[sys] = (char *)malloc(n);
        if (verSelText[sys]) memcpy(verSelText[sys], text, n);
    }
}

static void Version_SaveGroup(int sys, const VerGroup *g)
{
    char path[2112], dir[2048];
    static char buf[MAX_GAMES * 200];
    VersionInfo v;
    const char *label;
    if (!g || g->sel < 0 || g->sel >= g->n || !verSrc[sys]) return;
    Version_ParseGame(sys, &verSrc[sys][g->raw[g->sel]], &v);
    label = v.label[0] ? v.label : "Original";
    if (Version_SelSet(verSelText[sys], g->title, label, buf, sizeof(buf)) != 0) {
        fprintf(stderr, "NSO: versions list too long, not saved\n");
        return;
    }
    snprintf(dir, sizeof(dir), "%s/config", GetBaseDirectory());
    MakeDirectory(dir);
    Version_SelPath(sys, path, sizeof(path));
    if (!SaveFileText(path, buf)) fprintf(stderr, "NSO: cannot save %s\n", path);
    Version_SetSelText(sys, buf);
}

/* Safer fill using group index. */
static void Version_FillLive(int sys, int liveIdx, int gi)
{
    const GameEntry *src;
    VerGroup *g;
    if (gi < 0 || gi >= verGrpN[sys] || !verSrc[sys]) return;
    g = &verGrp[sys][gi];
    if (g->sel < 0 || g->sel >= g->n) g->sel = 0;
    src = &verSrc[sys][g->raw[g->sel]];
    verLive[sys][liveIdx] = *src;
    verLive[sys][liveIdx].title = g->title;
    if (!verLive[sys][liveIdx].boxartPath || !verLive[sys][liveIdx].boxartPath[0]) {
        int srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
        for (int m = 0; m < g->n; m++) {
            int r = g->raw[m];
            const char *bp;
            if (r < 0 || r >= srcN) continue;
            bp = verSrc[sys][r].boxartPath;
            if (bp && bp[0]) {
                verLive[sys][liveIdx].boxartPath = bp;
                break;
            }
        }
    }
}

static void Version_PickTitle(int sys, VerGroup *g)
{
    VersionInfo v;
    int srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    if (!g || !verSrc[sys]) return;
    for (int m = 0; m < g->n; m++) {
        int r = g->raw[m];
        if (r < 0 || r >= srcN) continue;
        if (r >= 0 && r < MAX_GAMES && verFolderTitle[sys][r][0]) {
            snprintf(g->title, sizeof(g->title), "%s", verFolderTitle[sys][r]);
            return;
        }
    }
    for (int m = 0; m < g->n; m++) {
        int r = g->raw[m];
        const char *t;
        if (r < 0 || r >= srcN) continue;
        t = verSrc[sys][r].title;
        Version_ParseGame(sys, &verSrc[sys][r], &v);
        if (t && v.base[0] && strcmp(t, v.base) == 0) {
            snprintf(g->title, sizeof(g->title), "%s", t);
            return;
        }
    }
    Version_ParseGame(sys, &verSrc[sys][g->raw[g->sel < g->n ? g->sel : 0]], &v);
    snprintf(g->title, sizeof(g->title), "%s", v.base[0] ? v.base : (verSrc[sys][g->raw[0]].title ? verSrc[sys][g->raw[0]].title : ""));
}

static void Version_SelectDefault(int sys, VerGroup *g)
{
    VersionInfo info[VERSION_MAX_MEMBERS];
    bool play[VERSION_MAX_MEMBERS];
    char saved[VERSION_NAME] = "";
    const char *p;
    int srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    if (!g || g->n <= 0) return;
    for (int m = 0; m < g->n; m++) {
        int r = g->raw[m];
        memset(&info[m], 0, sizeof(info[m]));
        play[m] = false;
        if (r < 0 || r >= srcN || !verSrc[sys]) continue;
        Version_ParseGame(sys, &verSrc[sys][r], &info[m]);
        play[m] = GameIsLaunchable(&verSrc[sys][r]);
    }
    p = Version_SelGet(verSelText[sys], g->title, g->key);
    Version_SelLabelCopy(p, saved, sizeof(saved));
    g->sel = Version_Prefer(info, play, g->n, saved[0] ? saved : NULL);
    if (g->sel < 0 || g->sel >= g->n) g->sel = 0;
}

static int Version_FindKey(int sys, const char *key, int nGrp, const VerGroup *grp)
{
    if (!key || !key[0]) return -1;
    for (int i = 0; i < nGrp; i++)
        if (strcmp(grp[i].key, key) == 0) return i;
    (void)sys;
    return -1;
}

static void Version_Rebuild(int sys)
{
    int srcN = (verSrc[sys] && verSrcN[sys]) ? *verSrcN[sys] : 0;
    static VerGroup built[MAX_GAMES];
    int nBuilt = 0;
    static int oldSel[MAX_GAMES];
    static char oldKey[MAX_GAMES][VERSION_NAME];
    int oldN = verGrpN[sys];
    static char liveOrder[MAX_GAMES][VERSION_NAME];
    int liveN = verCount[sys];

    if (sys < 0 || sys >= SYSTEM_COUNT) return;
    if (srcN > MAX_GAMES) srcN = MAX_GAMES;
    for (int i = 0; i < oldN && i < MAX_GAMES; i++) {
        oldSel[i] = verGrp[sys][i].sel;
        snprintf(oldKey[i], sizeof(oldKey[i]), "%s", verGrp[sys][i].key);
    }
    for (int i = 0; i < liveN && i < MAX_GAMES; i++) {
        const char *t = verLive[sys][i].title;
        snprintf(liveOrder[i], sizeof(liveOrder[i]), "%s", t ? t : "");
    }

    memset(built, 0, sizeof(built));
    /* Folder groups share a key. Catalog / wiki cover rows (and a loose ROM
     * of the same game) join that group, or each other, by normalized title
     * so "RockMan & Forte" and "Rockman & Forte" are one tile. Unmatched
     * loose ROMs stay individual tiles. */
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < srcN; i++) {
            char key[VERSION_NAME], nk[VERSION_NAME];
            int gi, hasFolder = verFolderKey[sys][i][0] != '\0';
            if (pass == 0 && !hasFolder) continue;
            if (pass == 1 && hasFolder) continue;
            if (hasFolder) {
                snprintf(key, sizeof(key), "%s", verFolderKey[sys][i]);
            } else {
                Version_RawNorm(sys, i, nk, sizeof(nk));
                gi = -1;
                if (nk[0]) {
                    char have[VERSION_NAME], pretty[VERSION_NAME];
                    for (int g = 0; g < nBuilt && gi < 0; g++) {
                        for (int m = 0; m < built[g].n; m++) {
                            Version_RawNorm(sys, built[g].raw[m], have, sizeof(have));
                            if (have[0] && strcmp(have, nk) == 0) { gi = g; break; }
                        }
                        if (gi >= 0) break;
                        if (strncmp(built[g].key, "folder:", 7) == 0) {
                            Version_PrettifyFolder(built[g].key + 7, pretty, sizeof(pretty));
                            Version_NormName(pretty, have, sizeof(have));
                            if (have[0] && strcmp(have, nk) == 0) gi = g;
                        } else if (strncmp(built[g].key, "name:", 5) == 0
                                   && strcmp(built[g].key + 5, nk) == 0) {
                            gi = g;
                        }
                    }
                }
                if (gi >= 0) {
                    if (built[gi].n < VERSION_MAX_MEMBERS)
                        built[gi].raw[built[gi].n++] = i;
                    continue;
                }
                if (i < verImported[sys] && nk[0])
                    snprintf(key, sizeof(key), "name:%s", nk);
                else
                    snprintf(key, sizeof(key), "loose:%d", i);
            }
            gi = Version_FindKey(sys, key, nBuilt, built);
            if (gi < 0) {
                if (nBuilt >= MAX_GAMES) continue;
                gi = nBuilt++;
                snprintf(built[gi].key, sizeof(built[gi].key), "%s", key);
            }
            if (built[gi].n < VERSION_MAX_MEMBERS)
                built[gi].raw[built[gi].n++] = i;
        }
    }

    for (int i = 0; i < nBuilt; i++) {
        int prev = -1;
        Version_PickTitle(sys, &built[i]);
        for (int k = 0; k < oldN; k++)
            if (strcmp(oldKey[k], built[i].key) == 0) { prev = k; break; }
        if (prev >= 0 && oldSel[prev] >= 0 && oldSel[prev] < built[i].n)
            built[i].sel = oldSel[prev];
        else
            Version_SelectDefault(sys, &built[i]);
        /* If we restored a stale index past a shrink, clamp via default already. */
        if (built[i].sel < 0 || built[i].sel >= built[i].n) Version_SelectDefault(sys, &built[i]);
    }

    /* Live order: keep existing groups in place, append new ones. */
    {
        int used[MAX_GAMES] = { 0 };
        int out = 0;
        if (verReady[sys] && liveN > 0) {
            for (int i = 0; i < liveN && out < nBuilt; i++) {
                int gi = -1;
                for (int k = 0; k < nBuilt; k++) {
                    if (used[k]) continue;
                    if (strcmp(built[k].title, liveOrder[i]) == 0
                        || strcmp(built[k].key, liveOrder[i]) == 0) { gi = k; break; }
                }
                if (gi < 0) continue;
                verGrp[sys][out] = built[gi];
                used[gi] = 1;
                verGrpN[sys] = out + 1;
                Version_FillLive(sys, out, out);
                out++;
            }
        }
        for (int k = 0; k < nBuilt; k++) {
            if (used[k]) continue;
            if (out >= MAX_GAMES) break;
            verGrp[sys][out] = built[k];
            verGrpN[sys] = out + 1;
            Version_FillLive(sys, out, out);
            out++;
        }
        verGrpN[sys] = out;
        verCount[sys] = out;
    }
    if (verReady[sys] && sys == currentSystem)
        Covers_SyncToGames(verLive[sys], verCount[sys],
                           hideReady[sys] ? hideShown[sys] : verCount[sys]);
}

static void Version_Install(int sys)
{
    if (sys < 0 || sys >= SYSTEM_COUNT) return;
    if (!verReady[sys]) {
        verOrigin[sys] = systems[sys].games;
        verOriginN[sys] = systems[sys].gameCount;
        verImported[sys] = 0;
        verRawN[sys] = 0;
        Version_ImportOrigin(sys);
        Version_ScanSystemRoms(sys);
        verSrc[sys] = verRaw[sys];
        verSrcN[sys] = &verRawN[sys];
        Version_LoadSel(sys);
    }
    Version_Rebuild(sys);
    systems[sys].games = verLive[sys];
    systems[sys].gameCount = &verCount[sys];
    verReady[sys] = true;
}

static void Version_InstallAll(void)
{
    for (int s = 0; s < SYSTEM_COUNT; s++) Version_Install(s);
}

static void Version_Refresh(int sys)
{
    if (sys < 0 || sys >= SYSTEM_COUNT || !verReady[sys]) return;
    Version_ImportOrigin(sys);
    Version_Rebuild(sys);
}

static int Version_CountAt(int sys, const char *title)
{
    int gi = Version_GroupByTitle(sys, title);
    return gi < 0 ? 1 : verGrp[sys][gi].n;
}

static void Version_LabelAt(int sys, const char *title, char *dst, size_t n, int *index, int *total)
{
    int gi = Version_GroupByTitle(sys, title);
    VersionInfo v;
    if (!dst || n == 0) return;
    dst[0] = '\0';
    if (index) *index = 0;
    if (total) *total = 1;
    if (gi < 0 || !verSrc[sys]) return;
    if (index) *index = verGrp[sys][gi].sel;
    if (total) *total = verGrp[sys][gi].n;
    if (verGrp[sys][gi].sel < 0 || verGrp[sys][gi].sel >= verGrp[sys][gi].n) return;
    Version_ParseGame(sys, &verSrc[sys][verGrp[sys][gi].raw[verGrp[sys][gi].sel]], &v);
    snprintf(dst, n, "%s", v.label[0] ? v.label : "Original");
}

static void Cover_Replace(int gameIdx);

static bool Version_Cycle(int sys, int gameIdx, int delta)
{
    const char *title;
    int gi, srcN;
    VerGroup *g;
    int next;
    if (sys < 0 || gameIdx < 0 || gameIdx >= verCount[sys]) return false;
    title = verLive[sys][gameIdx].title;
    /* After SortPanel_Apply, systems[sys].games is verLive (possibly reordered). */
    title = systems[sys].games[gameIdx].title;
    gi = Version_GroupByTitle(sys, title);
    if (gi < 0 || verGrp[sys][gi].n < 2) return false;
    g = &verGrp[sys][gi];
    next = g->sel + delta;
    while (next < 0) next += g->n;
    next %= g->n;
    if (next == g->sel) return false;
    g->sel = next;
    srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    if (g->raw[g->sel] < 0 || g->raw[g->sel] >= srcN) return false;
    systems[sys].games[gameIdx] = verSrc[sys][g->raw[g->sel]];
    systems[sys].games[gameIdx].title = g->title;
    Version_SaveGroup(sys, g);
    Cover_Replace(gameIdx);
    return true;
}

static const char *Version_PatchOf(int sys, const GameEntry *live)
{
    int gi, r, srcN;
    if (sys < 0 || sys >= SYSTEM_COUNT || !live) return NULL;
    gi = Version_GroupByTitle(sys, live->title);
    if (gi < 0) return NULL;
    srcN = verSrcN[sys] ? *verSrcN[sys] : 0;
    r = verGrp[sys][gi].raw[verGrp[sys][gi].sel];
    if (r < 0 || r >= srcN) return NULL;
    return verPatch[sys][r][0] ? verPatch[sys][r] : NULL;
}

/* useRel is NSO-relative (already remapped). Returns an NSO-relative ROM to
 * pass to GetDataPath: the original, or a temp soft-patched copy. */
static const char *Version_LaunchRom(int sys, const GameEntry *game, const char *useRel,
                                     char *scratchRel, size_t n)
{
    const char *patch = Version_PatchOf(sys, game);
    char baseAbs[2112], patchAbs[2112], destDir[2112], outAbs[2112];
    const char *root;
    size_t rootN;
    if (!useRel || !useRel[0] || !patch || !patch[0] || !scratchRel || n == 0)
        return useRel;
    root = GetBaseDirectory();
    rootN = root ? strlen(root) : 0;
    snprintf(baseAbs, sizeof(baseAbs), "%s/%s", root, useRel);
    if (patch[0] == '/') snprintf(patchAbs, sizeof(patchAbs), "%s", patch);
    else snprintf(patchAbs, sizeof(patchAbs), "%s/%s", root, patch);
    snprintf(destDir, sizeof(destDir), "%s/saves/tmp/patch/%s", root, Version_SysId(sys));
    if (Version_PreparePatchedRom(baseAbs, patchAbs, destDir, outAbs, sizeof(outAbs)) != 0)
        return useRel;
    if (rootN && strncmp(outAbs, root, rootN) == 0 && outAbs[rootN] == '/') {
        snprintf(scratchRel, n, "%s", outAbs + rootN + 1);
        return scratchRel;
    }
    return useRel;
}

#endif /* NSO_VERSION_UI_H */
