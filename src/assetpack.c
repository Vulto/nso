/* assetpack.c - look up packed UI assets and hook raylib file loaders. */
#ifdef NSO_RELEASE

#include "assetpack.h"
#include "raylib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern const unsigned char nso_asset_pack[];
extern const unsigned char nso_asset_pack_end[];

static const unsigned char *pack_data(void) { return nso_asset_pack; }
static unsigned int pack_size(void)
{
    return (unsigned int)(nso_asset_pack_end - nso_asset_pack);
}

static const char *asset_rel(const char *path)
{
    const char *p;
    if (!path || !path[0]) return NULL;
    p = strstr(path, "assets/");
    return p;
}

static int pack_ok(const unsigned char *p, unsigned int sz)
{
    unsigned int n, str_off, data_off;
    if (sz < 20 || memcmp(p, "NSOAS01", 7) != 0) return 0;
    memcpy(&n, p + 8, 4);
    memcpy(&str_off, p + 12, 4);
    memcpy(&data_off, p + 16, 4);
    if (n > 4096) return 0;
    if (str_off < 20 + n * 12u) return 0;
    if (data_off < str_off || data_off > sz) return 0;
    return 1;
}

static int asset_find(const char *rel, const unsigned char **data, unsigned int *size)
{
    const unsigned char *p = pack_data();
    unsigned int sz = pack_size(), n, str_off, i;
    if (!rel || !rel[0] || !pack_ok(p, sz)) return 0;
    memcpy(&n, p + 8, 4);
    memcpy(&str_off, p + 12, 4);
    for (i = 0; i < n; i++) {
        unsigned int roff, doff, dsz;
        const char *name;
        memcpy(&roff, p + 20 + i * 12, 4);
        memcpy(&doff, p + 20 + i * 12 + 4, 4);
        memcpy(&dsz, p + 20 + i * 12 + 8, 4);
        if (str_off + roff >= sz) continue;
        name = (const char *)(p + str_off + roff);
        if (strcmp(name, rel) == 0) {
            if (doff + dsz > sz) return 0;
            if (data) *data = p + doff;
            if (size) *size = dsz;
            return 1;
        }
    }
    return 0;
}

bool AssetPack_Has(const char *path)
{
    const char *rel = asset_rel(path);
    return rel && asset_find(rel, NULL, NULL);
}

bool Nso_FileExists(const char *path)
{
    struct stat st;
    if (AssetPack_Has(path)) return true;
    if (!path || !path[0]) return false;
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static unsigned char *load_disk(const char *path, int *dataSize)
{
    FILE *f;
    long len;
    unsigned char *buf;
    if (dataSize) *dataSize = 0;
    if (!path || !path[0]) return NULL;
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return NULL; }
    len = ftell(f);
    if (len < 0 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return NULL; }
    buf = (unsigned char *)MemAlloc((int)len + 1);
    if (!buf) { fclose(f); return NULL; }
    if (len > 0 && fread(buf, 1, (size_t)len, f) != (size_t)len) {
        MemFree(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    buf[len] = 0;
    if (dataSize) *dataSize = (int)len;
    return buf;
}

static unsigned char *nso_load_data(const char *fileName, int *dataSize)
{
    const char *rel = asset_rel(fileName);
    const unsigned char *src;
    unsigned int n;
    unsigned char *out;
    if (rel && asset_find(rel, &src, &n)) {
        out = (unsigned char *)MemAlloc((int)n + 1);
        if (!out) {
            if (dataSize) *dataSize = 0;
            return NULL;
        }
        memcpy(out, src, n);
        out[n] = 0;
        if (dataSize) *dataSize = (int)n;
        return out;
    }
    return load_disk(fileName, dataSize);
}

static char *nso_load_text(const char *fileName)
{
    int n = 0;
    return (char *)nso_load_data(fileName, &n);
}

void AssetPack_Install(void)
{
    SetLoadFileDataCallback(nso_load_data);
    SetLoadFileTextCallback(nso_load_text);
}

#endif /* NSO_RELEASE */
