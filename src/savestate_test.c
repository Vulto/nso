#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#define SAVESTATE_IMPLEMENTATION
#include "savestate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int fails = 0;
#define EXPECT(c) do { if (!(c)) { fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

static char dir[64];

static void put(const char *name, const char *ext, const char *data)
{
    char p[1024];
    FILE *f;
    Ss_Path(p, sizeof p, dir, name, ext);
    f = fopen(p, "wb");
    fputs(data, f);
    fclose(f);
}

static int has(const char *name, const char *ext, const char *data)
{
    char p[1024], buf[64] = {0};
    FILE *f;
    Ss_Path(p, sizeof p, dir, name, ext);
    f = fopen(p, "rb");
    if (!f) return 0;
    if (!fgets(buf, sizeof buf, f)) buf[0] = '\0';
    fclose(f);
    return data == NULL || strcmp(buf, data) == 0;
}

int main(void)
{
    char out[64];
    char tooLong[64];

    snprintf(dir, sizeof dir, "/tmp/nso-ss-test-XXXXXX");
    if (!mkdtemp(dir)) { perror("mkdtemp"); return 1; }

    EXPECT(Ss_ValidName("20261110-140501"));
    EXPECT(Ss_ValidName("World 1 (castle)"));
    EXPECT(!Ss_ValidName(""));
    EXPECT(!Ss_ValidName(".hidden"));
    EXPECT(!Ss_ValidName("a/b"));
    EXPECT(!Ss_ValidName("..\\x"));
    EXPECT(!Ss_ValidName("trailing "));
    EXPECT(!Ss_ValidName("under_score"));
    memset(tooLong, 'a', 41); tooLong[41] = '\0';
    EXPECT(!Ss_ValidName(tooLong));

    put("s1", ".sav", "STATE1");
    put("s1", ".bmp", "THUMB1");
    put("s2", ".sav", "STATE2"); /* no thumbnail */

    /* copy: pair copied, numbered names */
    EXPECT(Ss_Copy(dir, "s1", out, sizeof out) == 0 && strcmp(out, "s1 copy") == 0);
    EXPECT(has("s1 copy", ".sav", "STATE1") && has("s1 copy", ".bmp", "THUMB1"));
    EXPECT(Ss_Copy(dir, "s1", out, sizeof out) == 0 && strcmp(out, "s1 copy 2") == 0);
    EXPECT(Ss_Copy(dir, "s2", out, sizeof out) == 0 && has("s2 copy", ".sav", "STATE2")
           && !has("s2 copy", ".bmp", NULL));
    EXPECT(Ss_Copy(dir, "missing", out, sizeof out) != 0);
    EXPECT(Ss_Copy(dir, "s1", out, 4) != 0 && !Ss_Exists(dir, "s1 copy 3"));

    /* rename: moves both, refuses clobber and bad names */
    EXPECT(Ss_Rename(dir, "s1 copy", "boss") == 0);
    EXPECT(!Ss_Exists(dir, "s1 copy") && has("boss", ".sav", "STATE1") && has("boss", ".bmp", "THUMB1"));
    EXPECT(Ss_Rename(dir, "boss", "s2") != 0 && has("s2", ".sav", "STATE2"));
    EXPECT(Ss_Rename(dir, "boss", "../evil") != 0 && Ss_Exists(dir, "boss"));
    EXPECT(Ss_Rename(dir, "nope", "x") != 0);
    EXPECT(Ss_Rename(dir, "boss", "boss") == 0);

    /* delete: pair removed, missing thumb ok, missing state is an error */
    EXPECT(Ss_Delete(dir, "boss") == 0 && !has("boss", ".sav", NULL) && !has("boss", ".bmp", NULL));
    EXPECT(Ss_Delete(dir, "s2") == 0 && !Ss_Exists(dir, "s2"));
    EXPECT(Ss_Delete(dir, "s2") != 0);
    EXPECT(Ss_Delete(dir, "../s1") != 0);

    Ss_Delete(dir, "s1");
    Ss_Delete(dir, "s1 copy 2");
    Ss_Delete(dir, "s2 copy");
    EXPECT(rmdir(dir) == 0); /* nothing left behind */

    if (fails) { fprintf(stderr, "savestate_test: %d failure(s)\n", fails); return 1; }
    printf("savestate_test: ok\n");
    return 0;
}
