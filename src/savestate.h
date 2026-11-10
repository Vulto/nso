/* Save-state file pairs: <dir>/<name>.sav (game state) + <name>.bmp
 * (thumbnail, optional). Plain C stdio, no raylib, so it can be tested. */
#ifndef NSO_SAVESTATE_H
#define NSO_SAVESTATE_H

#include <stdbool.h>
#include <stddef.h>

#define SS_NAME_MAX 40

/* Names: 1..SS_NAME_MAX chars of [A-Za-z0-9 .()-], no leading '.' or
 * space, no trailing space (only glyphs the menu font can draw). */
bool Ss_ValidName(const char *name);
/* <dir>/<name><ext>; 0 on success, -1 if it does not fit. */
int Ss_Path(char *out, size_t n, const char *dir, const char *name, const char *ext);
bool Ss_Exists(const char *dir, const char *name);
/* Remove the pair; missing thumbnail is fine. 0 ok, -1 error. */
int Ss_Delete(const char *dir, const char *name);
/* Copy to the first free "<name> copy", "<name> copy 2", ...; the new
 * name is written to outName. 0 ok, -1 error (nothing left behind). */
int Ss_Copy(const char *dir, const char *name, char *outName, size_t n);
/* Rename the pair; refuses invalid names and never overwrites. */
int Ss_Rename(const char *dir, const char *from, const char *to);

#endif /* NSO_SAVESTATE_H */

#ifdef SAVESTATE_IMPLEMENTATION
#ifndef SAVESTATE_IMPLEMENTATION_ONCE
#define SAVESTATE_IMPLEMENTATION_ONCE

#include <errno.h>
#include <stdio.h>
#include <string.h>

bool Ss_ValidName(const char *name)
{
    size_t len;
    if (!name) return false;
    len = strlen(name);
    if (len == 0 || len > SS_NAME_MAX) return false;
    if (name[0] == '.' || name[0] == ' ' || name[len - 1] == ' ') return false;
    for (size_t i = 0; i < len; i++) {
        char c = name[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-'
            || c == '(' || c == ')';
        if (!ok) return false;
    }
    return true;
}

int Ss_Path(char *out, size_t n, const char *dir, const char *name, const char *ext)
{
    int w;
    if (!out || !dir || !name || !ext) return -1;
    w = snprintf(out, n, "%s/%s%s", dir, name, ext);
    return (w < 0 || (size_t)w >= n) ? -1 : 0;
}

static bool Ss_FileExists(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

bool Ss_Exists(const char *dir, const char *name)
{
    char p[1024];
    return Ss_Path(p, sizeof p, dir, name, ".sav") == 0 && Ss_FileExists(p);
}

int Ss_Delete(const char *dir, const char *name)
{
    char sav[1024], bmp[1024];
    if (!Ss_ValidName(name)) return -1;
    if (Ss_Path(sav, sizeof sav, dir, name, ".sav") != 0
        || Ss_Path(bmp, sizeof bmp, dir, name, ".bmp") != 0) return -1;
    if (remove(sav) != 0) return -1;
    if (remove(bmp) != 0 && errno != ENOENT) return -1;
    return 0;
}

static int Ss_CopyFile(const char *from, const char *to)
{
    char buf[16384];
    size_t r;
    int rc = 0;
    FILE *in = fopen(from, "rb");
    FILE *out;
    if (!in) return -1;
    out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }
    while ((r = fread(buf, 1, sizeof buf, in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) {
            rc = -1;
            break;
        }
    }
    if (ferror(in)) rc = -1;
    fclose(in);
    if (fclose(out) != 0) rc = -1;
    if (rc != 0) remove(to);
    return rc;
}

int Ss_Copy(const char *dir, const char *name, char *outName, size_t n)
{
    char cand[SS_NAME_MAX + 1];
    char src[1024], dst[1024];
    if (!Ss_ValidName(name) || !outName || n == 0) return -1;
    if (!Ss_Exists(dir, name)) return -1;

    for (int i = 1; i < 100; i++) {
        int w = (i == 1) ? snprintf(cand, sizeof cand, "%s copy", name)
                         : snprintf(cand, sizeof cand, "%s copy %d", name, i);
        if (w < 0 || (size_t)w >= sizeof cand || !Ss_ValidName(cand)) return -1;
        if (Ss_Exists(dir, cand)) continue;

        if (Ss_Path(src, sizeof src, dir, name, ".sav") != 0
            || Ss_Path(dst, sizeof dst, dir, cand, ".sav") != 0
            || Ss_CopyFile(src, dst) != 0) return -1;
        if (Ss_Path(src, sizeof src, dir, name, ".bmp") != 0) return -1;
        if (Ss_FileExists(src)) {
            char dstBmp[1024];
            if (Ss_Path(dstBmp, sizeof dstBmp, dir, cand, ".bmp") != 0
                || Ss_CopyFile(src, dstBmp) != 0) {
                remove(dst);
                return -1;
            }
        }
        if (strlen(cand) >= n) {
            Ss_Delete(dir, cand);
            return -1;
        }
        strcpy(outName, cand);
        return 0;
    }
    return -1;
}

int Ss_Rename(const char *dir, const char *from, const char *to)
{
    char a[1024], b[1024], ab[1024], bb[1024];
    if (!Ss_ValidName(from) || !Ss_ValidName(to)) return -1;
    if (strcmp(from, to) == 0) return 0;
    if (!Ss_Exists(dir, from) || Ss_Exists(dir, to)) return -1;
    if (Ss_Path(a, sizeof a, dir, from, ".sav") != 0
        || Ss_Path(b, sizeof b, dir, to, ".sav") != 0
        || Ss_Path(ab, sizeof ab, dir, from, ".bmp") != 0
        || Ss_Path(bb, sizeof bb, dir, to, ".bmp") != 0) return -1;
    if (Ss_FileExists(bb)) return -1; /* stray thumbnail would be clobbered */
    if (rename(a, b) != 0) return -1;
    if (Ss_FileExists(ab) && rename(ab, bb) != 0) {
        rename(b, a); /* roll back */
        return -1;
    }
    return 0;
}

#endif /* SAVESTATE_IMPLEMENTATION_ONCE */
#endif /* SAVESTATE_IMPLEMENTATION */
