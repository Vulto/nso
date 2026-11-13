/* input_test.c - Nintendo confirm/back mapping. */
#include "nso.h"
#include <stdio.h>

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
    CHECK(Nso_PadNameIsN64("8BitDo N64"));
    CHECK(Nso_PadNameIsN64("8BitDo N64 Modkit"));
    CHECK(Nso_PadNameIsN64("NSO N64 Controller"));
    CHECK(!Nso_PadNameIsN64("Xbox Controller"));
    CHECK(!Nso_PadNameIsN64(NULL));
    CHECK(strstr(NSO_PAD_MAPPINGS, "05000000c82d00006928000000010000,8BitDo N64,a:b1,b:b0,") != NULL);
    if (fails) return 1;
    puts("input_test: ok");
    return 0;
}
