/* NSO "CRT Geometry": curved-screen CRT filter (geometry, rounded corners, beam
 * scanlines, halation, dot mask, filmic tone curve).
 *
 * Ported from MAME-PSGS (mame-psgs.vsh / mame-psgs_rgb32_dir.fsh)
 *   https://github.com/TheCodeTherapy/MAME-PSGS
 *   Copyright (C) 2019 Marco Gomez (@TheCodeTherapy)
 *   Licensed under the Apache License, Version 2.0
 *   (https://www.apache.org/licenses/LICENSE-2.0)
 * which is itself based on the "CRT-geom" shader family (cgwg, Themaister, DOLLS).
 *
 * Changes for NSO: the palette-emulation / dither section is dropped; the shader
 * works on a sub-rectangle of the emulator's texture (VidOrigin / VidSize);
 * the aspect follows the destination rectangle (OutAspect); the scanline beam
 * can be switched off (ScanAmt = 0) so that NSO's own Scanlines toggle still
 * means something; taps are clamped to the picture; the MAME-PSGS.vsh parameters
 * are uniforms (End menu > CRT Geometry, crtparams.h) whose stock values are
 * upstream's, and `const vec2 angle` stays upstream's until the user sets one.
 *
 * One source, three users (selected by the macro the host prepends):
 *   NSO_VERT   vertex stage  (Mednafen's ARB GLSL path, GLSL 1.10/1.20)
 *   (nothing)  fragment stage of the same
 *   NSO_RAYLIB both stages for raylib (#version 330); the host sets it (with
 *              NSO_VERT for the vertex stage). The fragment colour is tinted by
 *              raylib's vertex colour (fades) like a normal DrawTexturePro.
 */

#ifdef NSO_RAYLIB
#ifdef NSO_VERT
#define varying out
#else
#define varying in
#endif
#define TEX2D_RAW(c) texture(Tex0, (c))
#else
#define TEX2D_RAW(c) texture2D(Tex0, (c))
#endif

uniform sampler2D Tex0;
uniform vec2 TexSize;     /* texture size in texels */
uniform vec2 VidOrigin;   /* picture's top-left in the texture, texels */
uniform vec2 VidSize;     /* picture size, texels */
uniform float OutAspect;  /* destination height / width */
uniform float ScanAmt;    /* 1 = beam scanlines, 0 = smooth between lines */

/* MAME-PSGS.vsh lines 86-97, as uniforms so NSO can adjust them (crtparams.h). The host always sets
 * them; the stock values are CRTgamma 2.4, monitorgamma 2.2, overscan vec2(1,1), d 2.0, R 1.75,
 * cornersize 0.021, cornersmooth 25.0, halation 0.3, filmicReinhard 1.0, saturation 0.7. */
uniform float CRTgamma;
uniform float monitorgamma;
uniform float overscanK;       /* overscan = vec2(overscanK): one scale for both axes */
uniform float cornersize;
uniform float cornersmooth;
uniform float halation;
uniform float filmicReinhardAmt;
uniform float saturationAmt;
uniform float curvD;
uniform float curvR;
uniform float aspectK;         /* factor on the y of aspect vec2(1, 0.75); the aspect follows OutAspect */
uniform float angleSet;        /* 0: the const angle below, as upstream; 1: the user's AngleY */
uniform float angleY;
#define overscan vec2(overscanK)
#define SCREEN_ASPECT vec2(1.0, OutAspect * aspectK)
#define PI 3.141592653589

const float W = 1.2;
const float T = 7.5;
const vec3 gammaBoost = vec3(1.0 / 1.15);

#ifdef NSO_VERT
/* ---------------------------------------------------------------- vertex */
varying vec2 vUV;
varying vec3 vStretch;
varying vec2 vSin;
varying vec2 vCos;
#ifdef NSO_RAYLIB
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
varying vec4 vColor;
#endif

float intersectV(vec2 xy, vec2 sa, vec2 ca) {
    float A = dot(xy, xy) + curvD * curvD;
    float B = 2.0 * (curvR * (dot(xy, sa) - curvD * ca.x * ca.y) - curvD * curvD);
    float C = curvD * curvD + 2.0 * curvR * curvD * ca.x * ca.y;
    return (-B - sqrt(max(B * B - 4.0 * A * C, 0.0))) / (2.0 * A);
}

vec2 bkwtransV(vec2 xy, vec2 sa, vec2 ca) {
    float c = intersectV(xy, sa, ca);
    vec2 point = vec2(c) * xy;
    point -= vec2(-curvR) * sa;
    point /= vec2(curvR);
    vec2 tang = sa / ca;
    vec2 poc = point / ca;
    float A = dot(tang, tang) + 1.0;
    float B = -2.0 * dot(poc, tang);
    float C = dot(poc, poc) - 1.0;
    float a = (-B + sqrt(max(B * B - 4.0 * A * C, 0.0))) / (2.0 * A);
    vec2 uv = (point - a * sa) / ca;
    float r = curvR * acos(clamp(a, -1.0, 1.0));
    return uv * r / max(sin(r / curvR), 1e-5);
}

vec2 fwtransV(vec2 uv, vec2 sa, vec2 ca) {
    float r = max(abs(sqrt(dot(uv, uv))), 1e-5);
    uv *= sin(r / curvR) / r;
    float x = 1.0 - cos(r / curvR);
    float D = curvD / curvR + x * ca.x * ca.y + dot(uv, sa);
    return curvD * (uv * ca - x * sa) / D;
}

vec3 maxscale(vec2 sa, vec2 ca, vec2 aspect) {
    vec2 c = bkwtransV(-curvR * sa / (1.0 + curvR / curvD * ca.x * ca.y), sa, ca);
    vec2 a = vec2(0.5, 0.5) * aspect;
    vec2 lo = vec2(fwtransV(vec2(-a.x, c.y), sa, ca).x, fwtransV(vec2(c.x, -a.y), sa, ca).y) / aspect;
    vec2 hi = vec2(fwtransV(vec2(+a.x, c.y), sa, ca).x, fwtransV(vec2(c.x, +a.y), sa, ca).y) / aspect;
    return vec3((hi + lo) * aspect * 0.5, max(hi.x - lo.x, hi.y - lo.y));
}

void main() {
    const vec2 angle = vec2(0.0, -0.01);   /* tilt, radians: upstream's, kept unless the user set one */
    vec2 tilt = angleSet > 0.5 ? vec2(0.0, angleY) : angle;
#ifdef NSO_RAYLIB
    gl_Position = mvp * vec4(vertexPosition, 1.0);
    vColor = vertexColor;
    vec2 tc = vertexTexCoord;
#else
    gl_Position = ftransform();
    vec2 tc = gl_MultiTexCoord0.xy;
#endif
    vSin = sin(tilt);
    vCos = cos(tilt);
    vStretch = maxscale(vSin, vCos, SCREEN_ASPECT);
    /* 0..1 over the picture */
    vUV = (tc * TexSize - VidOrigin) / VidSize;
}
#else
/* -------------------------------------------------------------- fragment */
#ifdef NSO_RAYLIB
in vec2 vUV;
in vec3 vStretch;
in vec2 vSin;
in vec2 vCos;
in vec4 vColor;
out vec4 finalColor;
#else
varying vec2 vUV;
varying vec3 vStretch;
varying vec2 vSin;
varying vec2 vCos;
#endif

float filmicReinhardCurve(float x) {
    float q = (T * T + 1.0) * x * x;
    return q / (q + x + T * T);
}

vec3 fReinhard(vec3 x) {
    float w = filmicReinhardCurve(W);
    return vec3(filmicReinhardCurve(x.r), filmicReinhardCurve(x.g), filmicReinhardCurve(x.b)) / w;
}

vec3 sat(vec3 rgb, float adjustment) {
    const vec3 Wt = vec3(0.2125, 0.7154, 0.0721);
    return mix(vec3(dot(rgb, Wt)), rgb, adjustment);
}

float intersectF(vec2 xy) {
    float A = dot(xy, xy) + curvD * curvD;
    float B = 2.0 * (curvR * (dot(xy, vSin) - curvD * vCos.x * vCos.y) - curvD * curvD);
    float C = curvD * curvD + 2.0 * curvR * curvD * vCos.x * vCos.y;
    return (-B - sqrt(max(B * B - 4.0 * A * C, 0.0))) / (2.0 * A);
}

vec2 bkwtransF(vec2 xy) {
    if (length(xy) < 0.001) return xy;
    float c = intersectF(xy);
    vec2 point = vec2(c) * xy;
    point -= vec2(-curvR) * vSin;
    point /= vec2(curvR);
    vec2 tang = vSin / vCos;
    vec2 poc = point / vCos;
    float A = dot(tang, tang) + 1.0;
    float B = -2.0 * dot(poc, tang);
    float C = dot(poc, poc) - 1.0;
    float disc = B * B - 4.0 * A * C;
    if (disc < 0.0) return xy;
    float a = (-B + sqrt(disc)) / (2.0 * A);
    vec2 uv = (point - a * vSin) / vCos;
    float r = curvR * acos(clamp(a, -1.0, 1.0));
    return uv * r / sin(r / curvR);
}

/* 0..1 over the picture -> 0..1 over the curved picture (may leave 0..1) */
vec2 transform(vec2 coord) {
    vec2 aspect = SCREEN_ASPECT;
    coord = (coord - vec2(0.5)) * aspect * vStretch.z + vStretch.xy;
    return bkwtransF(coord) / overscan / aspect + vec2(0.5);
}

float corner(vec2 coord) {
    vec2 aspect = SCREEN_ASPECT;
    coord = (coord - vec2(0.5)) * overscan + vec2(0.5);
    coord = min(coord, vec2(1.0) - coord) * aspect;
    vec2 cdist = vec2(cornersize);
    coord = (cdist - min(coord, cdist));
    float dist = sqrt(dot(coord, coord));
    return clamp((cdist.x - dist) * cornersmooth, 0.0, 1.0);
}

vec4 scanlineWeights(float distance, vec4 color) {
    vec4 wid = 2.0 + 2.0 * pow(color, vec4(4.0));
    vec4 weights = vec4(distance / 0.3);
    return 1.4 * exp(-pow(weights * inversesqrt(0.5 * wid), wid)) / (0.6 + 0.2 * wid);
}

/* source texel (ix, iy) of the picture, clamped to it, linearised */
vec4 fetchLin(vec2 idx) {
    idx = clamp(idx, vec2(0.0), VidSize - vec2(1.0));
    return pow(TEX2D_RAW((VidOrigin + idx + vec2(0.5)) / TexSize), vec4(CRTgamma));
}

void main() {
    vec2 xy = transform(vUV);
    float cval = corner(xy);

    vec2 ratio_scale = xy * VidSize - vec2(0.5);
    float filt = fwidth(ratio_scale.y);
    vec2 uv_ratio = fract(ratio_scale);
    vec2 base = floor(ratio_scale);

    vec4 coeffs = PI * vec4(1.0 + uv_ratio.x, uv_ratio.x, 1.0 - uv_ratio.x, 2.0 - uv_ratio.x);
    coeffs = max(abs(coeffs), vec4(1e-5));
    coeffs = 2.0 * sin(coeffs) * sin(coeffs * 0.5) / (coeffs * coeffs);
    coeffs /= dot(coeffs, vec4(1.0));

    vec4 r0a = fetchLin(base + vec2(-1.0, 0.0));
    vec4 r0b = fetchLin(base);
    vec4 r0c = fetchLin(base + vec2(1.0, 0.0));
    vec4 r0d = fetchLin(base + vec2(2.0, 0.0));
    vec4 r1a = fetchLin(base + vec2(-1.0, 1.0));
    vec4 r1b = fetchLin(base + vec2(0.0, 1.0));
    vec4 r1c = fetchLin(base + vec2(1.0, 1.0));
    vec4 r1d = fetchLin(base + vec2(2.0, 1.0));

    vec4 col  = clamp(mat4(r0a, r0b, r0c, r0d) * coeffs, 0.0, 1.0);
    vec4 col2 = clamp(mat4(r1a, r1b, r1c, r1d) * coeffs, 0.0, 1.0);

    vec4 weights, weights2;
    if (ScanAmt > 0.5) {
        weights  = scanlineWeights(uv_ratio.y, col);
        weights2 = scanlineWeights(1.0 - uv_ratio.y, col2);
        float y = uv_ratio.y + 1.0 / 3.0 * filt;
        weights  = (weights + scanlineWeights(y, col)) / 3.0;
        weights2 = (weights2 + scanlineWeights(abs(1.0 - y), col2)) / 3.0;
        y = y - 2.0 / 3.0 * filt;
        weights  = weights + scanlineWeights(abs(y), col) / 3.0;
        weights2 = weights2 + scanlineWeights(abs(1.0 - y), col2) / 3.0;
    } else {
        weights  = vec4(1.0 - uv_ratio.y);
        weights2 = vec4(uv_ratio.y);
    }

    vec3 mul_res = (col * weights + col2 * weights2).rgb * vec3(cval);

    /* halation: a soft, unfiltered bleed of the neighbourhood */
    vec3 blur = mix(mix(fetchLin(base).rgb, fetchLin(base + vec2(1.0, 0.0)).rgb, uv_ratio.x),
                    mix(fetchLin(base + vec2(0.0, 1.0)).rgb, fetchLin(base + vec2(1.0, 1.0)).rgb, uv_ratio.x),
                    uv_ratio.y);
    mul_res = mix(mul_res, blur, halation);
    mul_res *= vec3(cval);

    vec3 dotMask = mix(vec3(1.0, 0.7, 1.0), vec3(0.7, 1.0, 0.7), floor(mod(gl_FragCoord.x, 2.0)));
    mul_res *= dotMask * 1.05;
    mul_res = clamp(pow(mul_res, vec3(1.0 / monitorgamma)) * 2.0, 0.0, 1.0);

    mul_res = mix(mul_res, fReinhard(mul_res), filmicReinhardAmt);
    mul_res = sat(mul_res, saturationAmt);
    mul_res = pow(mul_res, gammaBoost * 0.8);

    if (xy.x < 0.0 || xy.x > 1.0 || xy.y < 0.0 || xy.y > 1.0) mul_res = vec3(0.0);

#ifdef NSO_RAYLIB
    finalColor = vec4(mul_res * vColor.rgb, vColor.a);
#else
    gl_FragColor = vec4(mul_res, 1.0);
#endif
}
#endif
