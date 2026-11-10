/* Process spawn. POSIX fork; Win32 has no fork. */
#ifndef NSO_LAUNCH_H
#define NSO_LAUNCH_H

#ifndef _WIN32
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOUSER
#include <windows.h>
#include <direct.h>

typedef HANDLE NsoProc;

static inline NsoProc NsoProcNone(void) { return NULL; }

static inline int NsoChdir(const char *path) { return _chdir(path); }

static inline bool NsoProcOk(NsoProc proc) { return proc != NULL; }

static inline bool AppendArg(char *dst, size_t n, size_t *used, const char *arg)
{
    size_t u = *used;
    if (u > 0) {
        if (u + 1 >= n) return false;
        dst[u++] = ' ';
    }
    if (u + 1 >= n) return false;
    dst[u++] = '"';
    for (const char *s = arg; *s; s++) {
        if (*s == '"') {
            if (u + 2 >= n) return false;
            dst[u++] = '"';
            dst[u++] = '"';
        } else {
            if (u + 1 >= n) return false;
            dst[u++] = *s;
        }
    }
    if (u + 2 >= n) return false;
    dst[u++] = '"';
    dst[u] = '\0';
    *used = u;
    return true;
}

static inline NsoProc NsoSpawnArgv(const char *dir, char *const argv[])
{
    char app[2048];
    char cmd[8192];
    size_t used = 0;
    const char *appName = NULL;
    const char *rel;
    bool pathish;
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    if (!argv || !argv[0]) return NULL;

    rel = argv[0];
    pathish = strchr(rel, '/') != NULL || strchr(rel, '\\') != NULL
        || (rel[0] != '\0' && rel[1] == ':');
    if (pathish) {
        /* lpApplicationName is resolved against the caller cwd, not lpCurrentDirectory. */
        if (rel[0] == '.' && (rel[1] == '/' || rel[1] == '\\')) rel += 2;
        if (!dir || dir[0] == '\0' || rel[0] == '/' || rel[0] == '\\'
            || (rel[0] != '\0' && rel[1] == ':')) {
            snprintf(app, sizeof(app), "%s", argv[0]);
        } else {
            snprintf(app, sizeof(app), "%s/%s", dir, rel);
        }
        appName = app;
    }

    cmd[0] = '\0';
    if (appName) {
        if (!AppendArg(cmd, sizeof(cmd), &used, appName)) return NULL;
    }
    for (int i = appName ? 1 : 0; argv[i]; i++) {
        if (!AppendArg(cmd, sizeof(cmd), &used, argv[i])) return NULL;
    }

    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    if (!CreateProcessA((LPSTR)appName, cmd, NULL, NULL, FALSE, 0, NULL,
                        (dir && dir[0]) ? dir : NULL, &si, &pi)) {
        fprintf(stderr, "CreateProcess failed (%lu)\n", GetLastError());
        return NULL;
    }
    CloseHandle(pi.hThread);
    return pi.hProcess;
}

static inline NsoProc NsoSpawn(const char *dir, const char *binary)
{
    char *argv[2];
    argv[0] = (char *)binary;
    argv[1] = NULL;
    return NsoSpawnArgv(dir, argv);
}

static inline void NsoSpawnKill(NsoProc proc)
{
    if (proc) TerminateProcess(proc, 1);
}

/* Set (value != NULL) or clear an env var inherited by spawned games. */
static inline int NsoSetEnv(const char *name, const char *value)
{
    return _putenv_s(name, value ? value : "");
}

/* True once the child has exited. Reaps the handle. */
static inline bool NsoSpawnExited(NsoProc *proc, int *exitCode)
{
    DWORD code = 1;
    if (!proc || *proc == NULL) return false;
    if (WaitForSingleObject(*proc, 0) != WAIT_OBJECT_0) return false;
    if (GetExitCodeProcess(*proc, &code) && exitCode) *exitCode = (int)code;
    CloseHandle(*proc);
    *proc = NULL;
    return true;
}


#else
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

typedef pid_t NsoProc;

static inline NsoProc NsoProcNone(void) { return -1; }

static inline int NsoChdir(const char *path) { return chdir(path); }

static inline bool NsoProcOk(NsoProc proc) { return proc > 0; }

static inline NsoProc NsoSpawnArgv(const char *dir, char *const argv[])
{
    pid_t pid;
    if (!argv || !argv[0]) return -1;
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }
    if (pid == 0) {
        if (dir && dir[0] != '\0' && chdir(dir) != 0) {
            perror("Failed to chdir to game directory");
        }
        execvp(argv[0], argv);
        perror("execvp failed");
        _exit(127);
    }
    return pid;
}

static inline NsoProc NsoSpawn(const char *dir, const char *binary)
{
    char *argv[2];
    argv[0] = (char *)binary;
    argv[1] = NULL;
    return NsoSpawnArgv(dir, argv);
}

static inline void NsoSpawnKill(NsoProc proc)
{
    if (proc > 0) kill(proc, SIGTERM);
}

/* Set (value != NULL) or clear an env var inherited by spawned games. */
static inline int NsoSetEnv(const char *name, const char *value)
{
    return value ? setenv(name, value, 1) : unsetenv(name);
}

static inline bool NsoSpawnExited(NsoProc *proc, int *exitCode)
{
    int status;
    if (!proc || *proc <= 0) return false;
    if (waitpid(*proc, &status, WNOHANG) != *proc) return false;
    if (exitCode) {
        *exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
    *proc = -1;
    return true;
}
#endif


/* Optional one-shot ROM import before a normal launch (see GameEntry.romImport).
 * If importedRel is not yet present under the user data home and a top-level
 * file matching ext exists in the game directory, run
 *   <binary> importArg <that file>
 * and wait. --import-rom style flags exit after importing, so the caller then
 * launches the game with no extra arguments. A failed import still proceeds
 * to a normal launch (the game can show its own importer). */
typedef struct NsoRomImport {
    const char *ext;         /* e.g. ".nes"; matched case-insensitively */
    const char *importArg;   /* e.g. "--import-rom" */
    const char *importedRel; /* e.g. "opendana/content/base/pack.json" */
} NsoRomImport;

enum {
    NSO_ROM_IMPORT_SKIP = 0, /* already imported, or no matching file */
    NSO_ROM_IMPORT_RAN = 1,  /* importer was run (any exit status) */
    NSO_ROM_IMPORT_FAIL = -1 /* could not spawn the importer */
};

static inline int NsoRunArgv(const char *dir, char *const argv[])
{
    NsoProc proc = NsoSpawnArgv(dir, argv);
    if (!NsoProcOk(proc)) return -1;
#ifdef _WIN32
    {
        DWORD code = 1;
        WaitForSingleObject(proc, INFINITE);
        GetExitCodeProcess(proc, &code);
        CloseHandle(proc);
        return (int)code;
    }
#else
    {
        int status;
        if (waitpid(proc, &status, 0) != proc) return -1;
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    }
#endif
}

static inline bool NsoFileReadable(const char *path)
{
    FILE *f;
    if (!path || !path[0]) return false;
    f = fopen(path, "rb");
    if (!f) return false;
    fclose(f);
    return true;
}

static inline bool NsoUserDataHome(char *out, size_t cap)
{
    const char *p;
    if (!out || cap == 0) return false;
#ifdef _WIN32
    p = getenv("LOCALAPPDATA");
    if (p && p[0]) {
        snprintf(out, cap, "%s", p);
        return true;
    }
    p = getenv("USERPROFILE");
    if (p && p[0]) {
        snprintf(out, cap, "%s/AppData/Local", p);
        return true;
    }
#else
    p = getenv("XDG_DATA_HOME");
    if (p && p[0]) {
        snprintf(out, cap, "%s", p);
        return true;
    }
    p = getenv("HOME");
    if (p && p[0]) {
        snprintf(out, cap, "%s/.local/share", p);
        return true;
    }
#endif
    out[0] = '\0';
    return false;
}

static inline int NsoCiCmp(const char *a, const char *b)
{
    for (; *a && *b; a++, b++) {
        unsigned char ca = (unsigned char)*a, cb = (unsigned char)*b;
        if (ca >= 'A' && ca <= 'Z') ca = (unsigned char)(ca + ('a' - 'A'));
        if (cb >= 'A' && cb <= 'Z') cb = (unsigned char)(cb + ('a' - 'A'));
        if (ca != cb) return (int)ca - (int)cb;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline bool NsoExtMatch(const char *name, const char *ext)
{
    size_t n, e;
    if (!name || !ext || !ext[0]) return false;
    n = strlen(name);
    e = strlen(ext);
    if (e == 0 || n <= e) return false;
    return NsoCiCmp(name + n - e, ext) == 0;
}

/* First (lexicographically) regular file at the top of dir whose name ends
 * with ext. Does not recurse. Writes the joined path into out. */
static inline bool NsoFindTopLevelExt(const char *dir, const char *ext,
                                      char *out, size_t cap)
{
    char best[256] = {0};
    if (!dir || !dir[0] || !ext || !out || cap == 0) return false;
#ifdef _WIN32
    {
        char pattern[2048];
        WIN32_FIND_DATAA f;
        HANDLE h;
        snprintf(pattern, sizeof(pattern), "%s/*", dir);
        h = FindFirstFileA(pattern, &f);
        if (h == INVALID_HANDLE_VALUE) return false;
        do {
            if (f.cFileName[0] == '.') continue;
            if (f.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!NsoExtMatch(f.cFileName, ext)) continue;
            if (!best[0] || strcmp(f.cFileName, best) < 0) {
                snprintf(best, sizeof(best), "%s", f.cFileName);
            }
        } while (FindNextFileA(h, &f));
        FindClose(h);
    }
#else
    {
        DIR *d;
        struct dirent *ent;
        d = opendir(dir);
        if (!d) return false;
        while ((ent = readdir(d)) != NULL) {
            char full[2048];
            struct stat st;
            if (ent->d_name[0] == '.') continue;
            if (!NsoExtMatch(ent->d_name, ext)) continue;
            if (snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name) >= (int)sizeof(full))
                continue;
            if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
            if (!best[0] || strcmp(ent->d_name, best) < 0) {
                snprintf(best, sizeof(best), "%s", ent->d_name);
            }
        }
        closedir(d);
    }
#endif
    if (!best[0]) return false;
    if (snprintf(out, cap, "%s/%s", dir, best) >= (int)cap) {
        if (cap) out[0] = '\0';
        return false;
    }
    return true;
}

static inline bool NsoRomImportDone(const NsoRomImport *spec)
{
    char home[1024], path[2048];
    if (!spec || !spec->importedRel || !spec->importedRel[0]) return false;
    if (!NsoUserDataHome(home, sizeof(home))) return false;
    if (snprintf(path, sizeof(path), "%s/%s", home, spec->importedRel) >= (int)sizeof(path))
        return false;
    return NsoFileReadable(path);
}

/* True when an import should run; romOut receives the top-level ROM path. */
static inline bool NsoRomImportNeeded(const char *gameDir, const NsoRomImport *spec,
                                      char *romOut, size_t cap)
{
    if (!gameDir || !gameDir[0] || !spec || !spec->ext || !spec->importArg
        || !spec->importedRel || !romOut || cap == 0) {
        return false;
    }
    if (NsoRomImportDone(spec)) return false;
    return NsoFindTopLevelExt(gameDir, spec->ext, romOut, cap);
}

/* Run the one-shot importer when needed. Always safe to follow with a normal launch. */
static inline int NsoPrepareRomImport(const char *gameDir, const char *binary,
                                      const NsoRomImport *spec)
{
    char rom[2048];
    char *argv[4];
    int rc;

    if (!NsoRomImportNeeded(gameDir, spec, rom, sizeof(rom)))
        return NSO_ROM_IMPORT_SKIP;
    if (!binary || !binary[0]) return NSO_ROM_IMPORT_FAIL;

    argv[0] = (char *)binary;
    argv[1] = (char *)spec->importArg;
    argv[2] = rom;
    argv[3] = NULL;
    rc = NsoRunArgv(gameDir, argv);
    return (rc < 0) ? NSO_ROM_IMPORT_FAIL : NSO_ROM_IMPORT_RAN;
}

#endif /* NSO_LAUNCH_H */
