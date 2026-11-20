/* crtgeom.h - NSO "CRT Geometry" for what NSO draws itself (embedded games, the
 * rewind overlay): shaders/crtgeom.glsl, built for raylib (NSO_RAYLIB variant).
 * Mednafen (NES/SNES) runs the same file through its own patched shader path.
 *
 * The shader is derived from MAME-PSGS (Apache-2.0, Marco Gomez); see the file. */
#ifndef NSO_CRTGEOM_H
#define NSO_CRTGEOM_H
#include <stdbool.h>
#include "raylib.h"
#include "crtparams.h"

/* Compile shaders/crtgeom.glsl (path to the file) once, after the window exists. */
bool CrtGeom_Load(const char *glslPath);
void CrtGeom_Unload(void);
/* The fine settings of the next CrtGeom_Draw calls (crtparams.h values; angleSet: use the angle value). */
void CrtGeom_SetParams(const float val[CRTP_COUNT], bool angleSet);
bool CrtGeom_Ready(void);
/* Draw the sub-rectangle src (texels) of tex into dst with the CRT look; beamScan
 * false = smooth between lines. tint fades like DrawTexturePro's. */
void CrtGeom_Draw(Texture2D tex, Rectangle src, Rectangle dst, bool beamScan, Color tint);

#ifdef CRTGEOM_IMPLEMENTATION
#ifndef CRTGEOM_IMPLEMENTATION_ONCE
#define CRTGEOM_IMPLEMENTATION_ONCE
#include <stdlib.h>
#include <string.h>

static struct {
    Shader sh;
    bool tried;
    int locTex, locTexSize, locOrigin, locSize, locAspect, locScan;
    int locP[CRTP_COUNT], locAngleSet;
    float val[CRTP_COUNT];
    float angleSet;
} crtGeom;

/* the uniform each CRTP_* parameter feeds (crtgeom.glsl) */
static const char *const crtGeomUniform[CRTP_COUNT] = {
    [CRTP_GAMMA] = "CRTgamma", [CRTP_MGAMMA] = "monitorgamma", [CRTP_OVERSCAN] = "overscanK",
    [CRTP_SAT] = "saturationAmt", [CRTP_CURV] = "curvR", [CRTP_DIST] = "curvD", [CRTP_ANGLE] = "angleY",
    [CRTP_ASPECT] = "aspectK", [CRTP_CORNER] = "cornersize", [CRTP_CSMOOTH] = "cornersmooth",
    [CRTP_HALATION] = "halation", [CRTP_FILMIC] = "filmicReinhardAmt",
};

void CrtGeom_SetParams(const float val[CRTP_COUNT], bool angleSet)
{
    memcpy(crtGeom.val, val, sizeof(crtGeom.val));
    crtGeom.angleSet = angleSet ? 1.0f : 0.0f;
}

bool CrtGeom_Ready(void) { return crtGeom.sh.id > 0; }

void CrtGeom_Unload(void)
{
    if (crtGeom.sh.id) UnloadShader(crtGeom.sh);
    memset(&crtGeom, 0, sizeof(crtGeom));
}

bool CrtGeom_Load(const char *glslPath)
{
    if (crtGeom.tried) return CrtGeom_Ready();
    crtGeom.tried = true;
    char *src = glslPath ? LoadFileText(glslPath) : NULL;
    if (!src) return false;
    static const char vhead[] = "#version 330\n#define NSO_RAYLIB 1\n#define NSO_VERT 1\n";
    static const char fhead[] = "#version 330\n#define NSO_RAYLIB 1\n";
    size_t n = strlen(src);
    char *vs = malloc(sizeof(vhead) + n), *fs = malloc(sizeof(fhead) + n);
    if (vs && fs) {
        memcpy(vs, vhead, sizeof(vhead) - 1);
        memcpy(vs + sizeof(vhead) - 1, src, n + 1);
        memcpy(fs, fhead, sizeof(fhead) - 1);
        memcpy(fs + sizeof(fhead) - 1, src, n + 1);
        crtGeom.sh = LoadShaderFromMemory(vs, fs);
    }
    free(vs);
    free(fs);
    UnloadFileText(src);
    if (crtGeom.sh.id <= 0) { crtGeom.sh = (Shader){ 0 }; return false; }
    crtGeom.locTex = GetShaderLocation(crtGeom.sh, "Tex0");
    crtGeom.locTexSize = GetShaderLocation(crtGeom.sh, "TexSize");
    crtGeom.locOrigin = GetShaderLocation(crtGeom.sh, "VidOrigin");
    crtGeom.locSize = GetShaderLocation(crtGeom.sh, "VidSize");
    crtGeom.locAspect = GetShaderLocation(crtGeom.sh, "OutAspect");
    crtGeom.locScan = GetShaderLocation(crtGeom.sh, "ScanAmt");
    if (crtGeom.val[CRTP_GAMMA] == 0.0f)   /* the stock values until somebody sets others */
        for (int i = 0; i < CRTP_COUNT; i++) crtGeom.val[i] = crtParamInfo[i].def;
    for (int i = 0; i < CRTP_COUNT; i++) crtGeom.locP[i] = GetShaderLocation(crtGeom.sh, crtGeomUniform[i]);
    crtGeom.locAngleSet = GetShaderLocation(crtGeom.sh, "angleSet");
    return true;
}

void CrtGeom_Draw(Texture2D tex, Rectangle src, Rectangle dst, bool beamScan, Color tint)
{
    if (!CrtGeom_Ready() || !tex.id || dst.width < 1.0f || dst.height < 1.0f) {
        DrawTexturePro(tex, src, dst, (Vector2){ 0 }, 0.0f, tint);
        return;
    }
    float texSize[2] = { (float)tex.width, (float)tex.height };
    float origin[2] = { src.x, src.y }, size[2] = { src.width, src.height };
    float aspect = dst.height / dst.width, scan = beamScan ? 1.0f : 0.0f;
    int unit = 0;
    BeginShaderMode(crtGeom.sh);
    SetShaderValue(crtGeom.sh, crtGeom.locTex, &unit, SHADER_UNIFORM_INT);
    SetShaderValue(crtGeom.sh, crtGeom.locTexSize, texSize, SHADER_UNIFORM_VEC2);
    SetShaderValue(crtGeom.sh, crtGeom.locOrigin, origin, SHADER_UNIFORM_VEC2);
    SetShaderValue(crtGeom.sh, crtGeom.locSize, size, SHADER_UNIFORM_VEC2);
    SetShaderValue(crtGeom.sh, crtGeom.locAspect, &aspect, SHADER_UNIFORM_FLOAT);
    SetShaderValue(crtGeom.sh, crtGeom.locScan, &scan, SHADER_UNIFORM_FLOAT);
    for (int i = 0; i < CRTP_COUNT; i++) SetShaderValue(crtGeom.sh, crtGeom.locP[i], &crtGeom.val[i], SHADER_UNIFORM_FLOAT);
    SetShaderValue(crtGeom.sh, crtGeom.locAngleSet, &crtGeom.angleSet, SHADER_UNIFORM_FLOAT);
    DrawTexturePro(tex, src, dst, (Vector2){ 0 }, 0.0f, tint);
    EndShaderMode();
}
#endif
#endif
#endif
