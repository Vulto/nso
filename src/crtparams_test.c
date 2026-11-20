#define _DEFAULT_SOURCE
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CRTPARAMS_IMPLEMENTATION
#include "crtparams.h"

static int fails;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)

int main(void)
{
    /* the middle step is exactly the stock value; the ends are symmetric around it */
    for (int i = 0; i < CRTP_COUNT; i++) {
        CHECK(CrtParam_Value(i, CRTP_MID) == crtParamInfo[i].def);
        float lo = CrtParam_Value(i, 0), hi = CrtParam_Value(i, CRTP_STEPS), d = crtParamInfo[i].def;
        CHECK(fabsf((hi - d) - (d - lo)) < 1e-5f);
        CHECK(fminf(lo, hi) >= -0.25f);
        CHECK(CrtParam_Value(i, -5) == lo && CrtParam_Value(i, 99) == hi);
    }
    CHECK(CrtParam_Value(CRTP_GAMMA, CRTP_MID) == 2.4f && CrtParam_Value(CRTP_MGAMMA, CRTP_MID) == 2.2f);
    CHECK(CrtParam_Value(CRTP_CSMOOTH, 0) > 0.0f);   /* a 0 corner smooth would blank the picture */
    CHECK(fminf(CrtParam_Value(CRTP_CURV, 0), CrtParam_Value(CRTP_CURV, CRTP_STEPS)) >= 1.0f);
    CHECK(CrtParam_Value(CRTP_CURV, CRTP_STEPS) < CrtParam_Value(CRTP_CURV, 0));   /* right = rounder */

    /* unset inherits the system default, then the stock value; the angle is only "set" when somebody did */
    signed char game[CRTP_COUNT], def[CRTP_COUNT];
    float val[CRTP_COUNT];
    bool angleSet = true;
    memset(game, -1, sizeof(game));
    memset(def, -1, sizeof(def));
    CrtSteps_Resolve(game, def, val, &angleSet);
    CHECK(!angleSet);
    for (int i = 0; i < CRTP_COUNT; i++) CHECK(val[i] == crtParamInfo[i].def);
    def[CRTP_SAT] = 40;
    game[CRTP_CURV] = 0;
    CrtSteps_Resolve(game, def, val, &angleSet);
    CHECK(val[CRTP_SAT] == CrtParam_Value(CRTP_SAT, 40) && val[CRTP_CURV] == CrtParam_Value(CRTP_CURV, 0));
    CHECK(!angleSet);
    game[CRTP_SAT] = 10;   /* the game's own beats the default */
    game[CRTP_ANGLE] = 30;
    CrtSteps_Resolve(game, def, val, &angleSet);
    CHECK(val[CRTP_SAT] == CrtParam_Value(CRTP_SAT, 10) && angleSet);

    /* files: round trip, unset stays unset, junk is ignored; the run file has 13 numbers */
    char dir[] = "/tmp/crtparams_testXXXXXX";
    CHECK(mkdtemp(dir) != NULL);
    char path[300], run[300];
    snprintf(path, sizeof(path), "%s/sub/deeper/game.cfg", dir);
    snprintf(run, sizeof(run), "%s/run/x.params", dir);
    CHECK(CrtSteps_Save(path, game));
    signed char back[CRTP_COUNT];
    CHECK(CrtSteps_Load(path, back));
    CHECK(memcmp(back, game, sizeof(back)) == 0);
    FILE *f = fopen(path, "a");
    fputs("nonsense=3\ncurvature=99\ncurvature\n", f);
    fclose(f);
    CHECK(CrtSteps_Load(path, back) && back[CRTP_CURV] == 0);
    CHECK(!CrtSteps_Load("/nonexistent/x", back) && back[0] == -1);

    CrtSession_Begin(path, NULL, run);
    CHECK(crtSession.active && crtSession.game[CRTP_SAT] == 10);
    signed char ng[CRTP_COUNT];
    memset(ng, -1, sizeof(ng));
    ng[CRTP_HALATION] = 0;
    CHECK(CrtSession_Commit(ng));
    CHECK(crtSession.val[CRTP_HALATION] == 0.0f && crtSession.val[CRTP_SAT] == crtParamInfo[CRTP_SAT].def && !crtSession.angleSet);
    f = fopen(run, "r");
    float rv[CRTP_COUNT];
    int rs = -1, n = 0;
    for (int i = 0; i < CRTP_COUNT; i++) n += fscanf(f, "%f", &rv[i]);
    n += fscanf(f, "%d", &rs);
    fclose(f);
    CHECK(n == CRTP_COUNT + 1 && rs == 0 && rv[CRTP_HALATION] == 0.0f && fabsf(rv[CRTP_GAMMA] - 2.4f) < 1e-6f);
    CrtSession_End();
    CHECK(!crtSession.active);

    snprintf(path, sizeof(path), "rm -rf %s", dir);
    if (system(path) != 0) fails++;
    if (fails) return 1;
    puts("crtparams_test: ok");
    return 0;
}
