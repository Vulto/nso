/* input_test.c - Nintendo confirm/back mapping. */
#include "nso.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT) == 1); /* Nintendo A */
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_RIGHT_FACE_DOWN) == -1); /* Nintendo B */
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_MIDDLE_RIGHT) == 1);     /* Start */
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_RIGHT_FACE_UP) == 0);
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_RIGHT_FACE_LEFT) == 0);
    CHECK(Nso_PadConfirmBack(GAMEPAD_BUTTON_MIDDLE_LEFT) == 0);
    CHECK(NSO_BTN_A == GAMEPAD_BUTTON_RIGHT_FACE_RIGHT);
    CHECK(NSO_BTN_B == GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    CHECK(NSO_BTN_X == GAMEPAD_BUTTON_RIGHT_FACE_UP);
    CHECK(NSO_BTN_Y == GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
    CHECK(strstr(NSO_PAD_MAPPINGS, "05000000c82d00006928000000010000,8BitDo N64,a:b1,b:b0,") != NULL);
    CHECK(Nso_PadAxisEdge(0.0f, 0.4f) == 0);    /* inside deadzone */
    CHECK(Nso_PadAxisEdge(0.0f, 0.5f) == 0);    /* equal to deadzone is not an edge */
    CHECK(Nso_PadAxisEdge(0.0f, 0.51f) == 1);
    CHECK(Nso_PadAxisEdge(0.6f, 0.8f) == 0);    /* already past: no repeat */
    CHECK(Nso_PadAxisEdge(0.6f, 0.2f) == 0);
    CHECK(Nso_PadAxisEdge(0.0f, -0.51f) == -1);
    CHECK(Nso_PadAxisEdge(-0.6f, -0.8f) == 0);
    CHECK(Nso_PadAxisEdge(-0.51f, 0.51f) == 1);
    CHECK(NSO_PAD_DEADZONE == 0.5f);
    if (fails) return 1;
    puts("input_test: ok");
    return 0;
}
