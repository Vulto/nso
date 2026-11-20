/* crtparams.h - the "CRT Geometry" fine settings (shaders/crtgeom.glsl, from MAME-PSGS).
 *
 * Twelve parameters, each a slider of CRTP_STEPS steps whose middle step (CRTP_MID) is the shader's
 * stock value, so the ranges are symmetric around the defaults. Per game a step is stored, or
 * "unset" (-1): an unset parameter inherits the system default (config/crtgeom/<system>/_default.cfg),
 * and when that is unset too, the stock value. The angle keeps MAME-PSGS.vsh's `const vec2 angle`
 * (0, -0.01) unless the user set it (angleSet).
 *
 * Mednafen runs as its own process: NSO writes the resolved values to a small "run" file
 * (13 numbers: the twelve values, then angleSet) that the patched Mednafen re-reads whenever
 * the file changes (tools/mednafen-nsocrt.patch). */
#ifndef NSO_CRTPARAMS_H
#define NSO_CRTPARAMS_H
#include <stdbool.h>

enum { CRTP_GAMMA, CRTP_MGAMMA, CRTP_OVERSCAN, CRTP_SAT, CRTP_CURV, CRTP_DIST, CRTP_ANGLE, CRTP_ASPECT,
       CRTP_CORNER, CRTP_CSMOOTH, CRTP_HALATION, CRTP_FILMIC, CRTP_COUNT };
#define CRTP_STEPS 40
#define CRTP_MID (CRTP_STEPS / 2)

typedef struct { const char *key; float def, half; } CrtParamInfo;
/* value = def + half * (step - MID) / MID (half < 0: the slider runs backwards).
 * gamma / monitorgamma: CRTgamma 2.4, monitorgamma 2.2.  overscan: ONE scale for both axes of
 * overscan vec2(1,1).  saturation 0.7.  curvature = R 1.75 (the slider runs backwards: right = rounder,
 * a smaller R), distance = d 2.0.  angle: the tilt's
 * y, default -0.01 rad (x stays 0).  aspect: a factor on the y of aspect vec2(1, 0.75) (1 = the
 * picture's own shape; the shader's aspect follows the screen rectangle).  corner size 0.021,
 * corner smooth 25, halation 0.3, filmic Reinhard 1.0. */
static const CrtParamInfo crtParamInfo[CRTP_COUNT] = {
    [CRTP_GAMMA]    = { "gamma",          2.4f,   1.0f },
    [CRTP_MGAMMA]   = { "monitor_gamma",  2.2f,   1.0f },
    [CRTP_OVERSCAN] = { "overscan",       1.0f,   0.15f },
    [CRTP_SAT]      = { "saturation",     0.7f,   0.7f },
    [CRTP_CURV]     = { "curvature",      1.75f, -0.75f },   /* R: smaller = rounder, so the slider runs backwards */
    [CRTP_DIST]     = { "distance",       2.0f,   1.0f },
    [CRTP_ANGLE]    = { "angle",         -0.01f,  0.2f },
    [CRTP_ASPECT]   = { "aspect",         1.0f,   0.25f },
    [CRTP_CORNER]   = { "corner_size",    0.021f, 0.021f },
    [CRTP_CSMOOTH]  = { "corner_smooth",  25.0f,  22.0f },
    [CRTP_HALATION] = { "halation",       0.3f,   0.3f },
    [CRTP_FILMIC]   = { "filmic",         1.0f,   1.0f },
};

static inline int CrtStep_Clamp(int s) { return s < 0 ? 0 : s > CRTP_STEPS ? CRTP_STEPS : s; }

/* The value of parameter i at a slider step; the middle step is exactly the stock value. */
static inline float CrtParam_Value(int i, int step)
{
    step = CrtStep_Clamp(step);
    if (step == CRTP_MID) return crtParamInfo[i].def;
    return crtParamInfo[i].def + crtParamInfo[i].half * (float)(step - CRTP_MID) / (float)CRTP_MID;
}

/* The step shown for parameter i: the game's, else the system default's, else the middle. */
static inline int CrtSteps_Shown(const signed char *game, const signed char *def, int i)
{
    if (game && game[i] >= 0) return game[i];
    if (def && def[i] >= 0) return def[i];
    return CRTP_MID;
}

static inline void CrtSteps_Resolve(const signed char *game, const signed char *def, float val[CRTP_COUNT], bool *angleSet)
{
    for (int i = 0; i < CRTP_COUNT; i++) val[i] = CrtParam_Value(i, CrtSteps_Shown(game, def, i));
    if (angleSet) *angleSet = (game && game[CRTP_ANGLE] >= 0) || (def && def[CRTP_ANGLE] >= 0);
}

typedef struct {
    bool active;                          /* CRT Geometry is on for the running game and it can be adjusted */
    char gamePath[1100], defPath[1100], runPath[1100];
    signed char game[CRTP_COUNT], def[CRTP_COUNT];
    float val[CRTP_COUNT];
    bool angleSet;
    bool rawFrames;   /* a hosted game whose captured frames are unfiltered (the port draws them plain): NSO applies the CRT look itself */
    bool fast;        /* cheaper shader path */
} CrtSession;
extern CrtSession crtSession;

void CrtSession_End(void);
/* defPath / runPath may be NULL (runPath: only Mednafen has one). Loads the steps and resolves them. */
void CrtSession_Begin(const char *gamePath, const char *defPath, const char *runPath);
void CrtSession_SetFast(bool fast);
/* The game's new steps (-1 = unset): saved, resolved, and written to the run file. false: a file failed. */
bool CrtSession_Commit(const signed char game[CRTP_COUNT]);

bool CrtSteps_Load(const char *path, signed char steps[CRTP_COUNT]);
bool CrtSteps_Save(const char *path, const signed char steps[CRTP_COUNT]);
bool CrtRun_Write(const char *path, const float val[CRTP_COUNT], bool angleSet);

#ifdef CRTPARAMS_IMPLEMENTATION
#ifndef CRTPARAMS_IMPLEMENTATION_ONCE
#define CRTPARAMS_IMPLEMENTATION_ONCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#endif

CrtSession crtSession;

static void CrtParams_MkdirsFor(const char *path)
{
#ifndef _WIN32
    char dir[1200];
    snprintf(dir, sizeof(dir), "%s", path);
    for (char *p = dir + 1; *p; p++) {
        if (*p != '/') continue;
        *p = '\0';
        mkdir(dir, 0755);
        *p = '/';
    }
#else
    (void)path;
#endif
}

bool CrtSteps_Load(const char *path, signed char steps[CRTP_COUNT])
{
    for (int i = 0; i < CRTP_COUNT; i++) steps[i] = -1;
    FILE *f = path && path[0] ? fopen(path, "r") : NULL;
    if (!f) return false;
    char line[96];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        for (int i = 0; i < CRTP_COUNT; i++)
            if (strcmp(line, crtParamInfo[i].key) == 0) {
                char *end = NULL;
                long v = strtol(eq + 1, &end, 10);
                if (end != eq + 1 && v >= 0 && v <= CRTP_STEPS) steps[i] = (signed char)v;
            }
    }
    fclose(f);
    return true;
}

bool CrtSteps_Save(const char *path, const signed char steps[CRTP_COUNT])
{
    if (!path || !path[0]) return false;
    char tmp[1200];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    CrtParams_MkdirsFor(path);
    FILE *f = fopen(tmp, "w");
    if (!f) return false;
    bool ok = true;
    for (int i = 0; i < CRTP_COUNT; i++)
        if (steps[i] >= 0) ok &= fprintf(f, "%s=%d\n", crtParamInfo[i].key, steps[i]) > 0;
    ok &= fclose(f) == 0;
    if (ok) ok = rename(tmp, path) == 0;
    if (!ok) remove(tmp);
    return ok;
}

bool CrtRun_Write(const char *path, const float val[CRTP_COUNT], bool angleSet)
{
    if (!path || !path[0]) return true;
    char tmp[1200];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);
    CrtParams_MkdirsFor(path);
    FILE *f = fopen(tmp, "w");
    if (!f) return false;
    bool ok = true;
    for (int i = 0; i < CRTP_COUNT; i++) ok &= fprintf(f, "%.7g ", (double)val[i]) > 0;
    ok &= fprintf(f, "%d %d\n", angleSet ? 1 : 0, crtSession.fast ? 1 : 0) > 0;
    ok &= fclose(f) == 0;
    if (ok) ok = rename(tmp, path) == 0;
    if (!ok) remove(tmp);
    return ok;
}

void CrtSession_End(void) { memset(&crtSession, 0, sizeof(crtSession)); }

void CrtSession_Begin(const char *gamePath, const char *defPath, const char *runPath)
{
    memset(&crtSession, 0, sizeof(crtSession));
    snprintf(crtSession.gamePath, sizeof(crtSession.gamePath), "%s", gamePath ? gamePath : "");
    snprintf(crtSession.defPath, sizeof(crtSession.defPath), "%s", defPath ? defPath : "");
    snprintf(crtSession.runPath, sizeof(crtSession.runPath), "%s", runPath ? runPath : "");
    CrtSteps_Load(crtSession.gamePath, crtSession.game);
    CrtSteps_Load(crtSession.defPath, crtSession.def);
    CrtSteps_Resolve(crtSession.game, crtSession.def, crtSession.val, &crtSession.angleSet);
    crtSession.active = true;
    CrtRun_Write(crtSession.runPath, crtSession.val, crtSession.angleSet);
}

void CrtSession_SetFast(bool fast)
{
    crtSession.fast = fast;
    if (crtSession.active)
        CrtRun_Write(crtSession.runPath, crtSession.val, crtSession.angleSet);
}

bool CrtSession_Commit(const signed char game[CRTP_COUNT])
{
    if (!crtSession.active) return false;
    memcpy(crtSession.game, game, CRTP_COUNT);
    CrtSteps_Resolve(crtSession.game, crtSession.def, crtSession.val, &crtSession.angleSet);
    bool ok = CrtSteps_Save(crtSession.gamePath, crtSession.game);
    ok &= CrtRun_Write(crtSession.runPath, crtSession.val, crtSession.angleSet);
    return ok;
}
#endif
#endif
#endif
