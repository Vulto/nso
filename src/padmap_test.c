#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
/* padmap_test.c - NES controller mapping: config, Mednafen words, ports' config.ini. */
#define LANG_IMPLEMENTATION
#include "padmap.h"
#include <unistd.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    NesPadMap m, r;
    char buf[2048], val[200], *w[40];

    /* defaults: nothing is handed to the games */
    PadMap_NesDefaults(&m);
    CHECK(m.b[NES_BTN_A].key == KEY_X && m.b[NES_BTN_B].key == KEY_Z && m.b[NES_BTN_START].key == KEY_ENTER);
    CHECK(m.b[NES_BTN_A].pad == GAMEPAD_BUTTON_RIGHT_FACE_RIGHT && m.b[NES_BTN_B].pad == GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
    CHECK(!m.custom && PadMap_MednafenWords(&m, buf, sizeof(buf), w, 40) == 0);
    PadMap_NesLoad(&r, "nes.rumble=1\nlanguage=pt\n");
    CHECK(!r.custom && r.b[NES_BTN_UP].key == KEY_UP);

    /* names */
    CHECK(PadMap_KeyByName("left shift") == KEY_LEFT_SHIFT && PadMap_KeyByName("Z") == KEY_Z);
    CHECK(PadMap_KeyByName("Keypad 4") == KEY_KP_4 && PadMap_KeyByName("nope") == 0 && PadMap_KeyByName(NULL) == 0);
    CHECK(!PadMap_KeyBindable(KEY_ESCAPE) && !PadMap_KeyBindable(KEY_BACKSPACE) && !PadMap_KeyBindable(KEY_END));
    CHECK(PadMap_KeyBindable(KEY_RIGHT_SHIFT) && PadMap_KeyInfo(KEY_ENTER)->scan == 40);
    CHECK(PadMap_PadByName("dpup") == GAMEPAD_BUTTON_LEFT_FACE_UP && PadMap_PadByName("lefttrigger+") == GAMEPAD_BUTTON_LEFT_TRIGGER_2);
    CHECK(PadMap_PadByName("guide") == -1 && !PadMap_PadBindable(GAMEPAD_BUTTON_MIDDLE));
    CHECK(strcmp(PadMap_PadInfo(GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)->label, "A") == 0);

    /* raw controls */
    {
        int kind, idx;
        CHECK(PadMap_JsParse("b12", &kind, &idx) && kind == PAD_JS_BUTTON && idx == 12);
        CHECK(PadMap_JsParse("a2-", &kind, &idx) && kind == PAD_JS_AXIS_NEG && idx == 2);
        CHECK(PadMap_JsParse("a3+", &kind, &idx) && kind == PAD_JS_AXIS_POS && idx == 3);
        CHECK(PadMap_JsParse("none", &kind, &idx) && kind == PAD_JS_NONE);
        CHECK(!PadMap_JsParse("b", &kind, &idx) && !PadMap_JsParse("a2", &kind, &idx) && !PadMap_JsParse("x1", &kind, &idx));
        CHECK(!PadMap_JsParse("b3x", &kind, &idx) && !PadMap_JsParse("b999", &kind, &idx));
    }

    /* config round trip, odd key names included */
    {
        char dir[] = "/tmp/nso-padmap-test-XXXXXX", path[256], text[8192];
        FILE *f;
        size_t n;
        CHECK(mkdtemp(dir) != NULL);
        snprintf(path, sizeof(path), "%s/nso.cfg", dir);
        f = fopen(path, "w");
        fputs("language=pt\nnes.rumble=1\n", f);
        fclose(f);
        m.b[NES_BTN_A].key = KEY_EQUAL;
        m.b[NES_BTN_B].key = KEY_BACKSLASH;
        m.b[NES_BTN_START].key = KEY_RIGHT_CONTROL;
        m.b[NES_BTN_UP].key = 0;
        m.b[NES_BTN_A].pad = GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
        m.b[NES_BTN_B].pad = -1;
        m.b[NES_BTN_A].jsKind = PAD_JS_BUTTON; m.b[NES_BTN_A].jsIdx = 5;
        m.b[NES_BTN_LEFT].jsKind = PAD_JS_AXIS_NEG; m.b[NES_BTN_LEFT].jsIdx = 2;
        snprintf(m.joyId, sizeof(m.joyId), "0003057e200981110004000d00000000");
        CHECK(PadMap_NesSave(&m, path));
        f = fopen(path, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = '\0';
        CHECK(strstr(text, "language=pt\n") && strstr(text, "nes.rumble=1\n"));
        CHECK(strstr(text, "nes.map.a.key==\n") && strstr(text, "nes.map.b.key=\\\n"));
        PadMap_NesLoad(&r, text);
        CHECK(r.custom && !memcmp(r.b, m.b, sizeof(m.b)) && !strcmp(r.joyId, m.joyId));
        CHECK(r.b[NES_BTN_UP].key == 0 && r.b[NES_BTN_B].pad == -1 && r.b[NES_BTN_START].key == KEY_RIGHT_CONTROL);
        CHECK(PadMap_NesClear(path));   /* back to the default, the other keys stay */
        f = fopen(path, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = '\0';
        CHECK(strstr(text, "nes.map") == NULL && strstr(text, "language=pt\n") && strstr(text, "nes.rumble=1\n"));
        PadMap_NesLoad(&r, text);
        CHECK(!r.custom && r.b[NES_BTN_A].key == KEY_X);
        CHECK(PadMap_NesClear("/nonexistent/nso.cfg"));
        unlink(path);
        rmdir(dir);
    }
    PadMap_NesLoad(&r, "nes.map.a.key=Nonsense\nnes.map.a.pad=zz\nnes.map.a.js=q\n");
    CHECK(r.custom && r.b[NES_BTN_A].key == KEY_X && r.b[NES_BTN_A].pad == GAMEPAD_BUTTON_RIGHT_FACE_RIGHT
          && r.b[NES_BTN_A].jsKind == PAD_JS_NONE);

    m.custom = true;

    /* Mednafen: the keyboard by scancode, the pad by js index and device id */
    PadMap_MednafenValue(&m.b[NES_BTN_A], m.joyId, val, sizeof(val));
    CHECK(strcmp(val, "keyboard 0x0 46 || joystick 0x0003057e200981110004000d00000000 button_5") == 0);
    PadMap_MednafenValue(&m.b[NES_BTN_LEFT], m.joyId, val, sizeof(val));
    CHECK(strcmp(val, "keyboard 0x0 80 || joystick 0x0003057e200981110004000d00000000 abs_2-") == 0);
    PadMap_MednafenValue(&m.b[NES_BTN_UP], m.joyId, val, sizeof(val));
    CHECK(strcmp(val, "") == 0);
    PadMap_MednafenValue(&m.b[NES_BTN_A], "", val, sizeof(val));
    CHECK(strcmp(val, "keyboard 0x0 46") == 0);
    {
        int n = PadMap_MednafenWords(&m, buf, sizeof(buf), w, 40);
        CHECK(n == 16 && strcmp(w[0], "-nes.input.port1.gamepad.up") == 0 && strcmp(w[1], "") == 0);
        CHECK(strcmp(w[14], "-nes.input.port1.gamepad.a") == 0 && strstr(w[15], "button_5") != NULL);
        CHECK(PadMap_MednafenWords(&m, buf, 40, w, 40) == 0 && PadMap_MednafenWords(&m, buf, sizeof(buf), w, 15) == 0);
    }

    /* the ports' config.ini: only player 1's two sections change */
    {
        const char *ini = "# settings\n[Display]\nWindowScale = 3\n[Keyboard.Player1]\nA = Z\nb = X\nselect = Tab\n"
                          "[Keyboard.Player2]\nA = Q\n[Gamepad.Player1]\nup = dpup\n[Rewind]\nEnabled = 1";
        char out[4096];
        PadMap_NesDefaults(&r);
        r.b[NES_BTN_A].key = KEY_SPACE;
        r.b[NES_BTN_A].pad = GAMEPAD_BUTTON_LEFT_TRIGGER_2;
        r.b[NES_BTN_B].pad = -1;
        CHECK(PadMap_IniMerge(ini, &r, out, sizeof(out)));
        CHECK(strstr(out, "# settings\n[Display]\nWindowScale = 3\n[Keyboard.Player1]\n"));
        CHECK(strstr(out, "[Keyboard.Player2]\nA = Q\n") && strstr(out, "[Rewind]\nEnabled = 1\n"));
        CHECK(strstr(out, "a = Space\nb = Z\nselect = Right Shift\n") != NULL || strstr(out, "a = Space\n") != NULL);
        CHECK(strstr(out, "b = Z\n") && strstr(out, "select = Right Shift\n") && strstr(out, "start = Return\n"));
        CHECK(strstr(out, "up = dpup\n") && strstr(out, "a = lefttrigger+\n") && strstr(out, "b = none\n"));
        CHECK(!strstr(out, "A = Z") && !strstr(out, "select = Tab"));
        {   /* each key once per section */
            int c = 0;
            for (const char *p = strstr(out, "start = "); p; p = strstr(p + 1, "start = ")) c++;
            CHECK(c == 2);
        }
        CHECK(!PadMap_IniMerge(ini, &r, out, 60));
        CHECK(PadMap_IniMerge(NULL, &r, out, sizeof(out)));   /* no file yet: both sections */
        CHECK(strstr(out, "[Keyboard.Player1]\n") == out && strstr(out, "[Gamepad.Player1]\n") != NULL);
        {   /* merging twice changes nothing */
            char again[4096];
            CHECK(PadMap_IniMerge(out, &r, again, sizeof(again)) && strcmp(out, again) == 0);
        }
    }

    /* SNES: twelve buttons for the games, ZL / ZR only kept in the config */
    {
        NesPadMap sm, sr;
        char dir[] = "/tmp/nso-padmap-test-XXXXXX", path[256], text[8192], out[4096], again[4096];
        FILE *f;
        size_t n;
        PadMap_Defaults(&PADSYS_SNES, &sm);
        CHECK(sm.b[SNES_BTN_A].key == KEY_X && sm.b[SNES_BTN_B].key == KEY_Z && sm.b[SNES_BTN_X].key == KEY_S);
        CHECK(sm.b[SNES_BTN_Y].key == KEY_A && sm.b[SNES_BTN_L].key == KEY_C && sm.b[SNES_BTN_R].key == KEY_V);
        CHECK(sm.b[SNES_BTN_A].pad == GAMEPAD_BUTTON_RIGHT_FACE_RIGHT && sm.b[SNES_BTN_Y].pad == GAMEPAD_BUTTON_RIGHT_FACE_LEFT);
        CHECK(sm.b[SNES_BTN_ZR].pad == GAMEPAD_BUTTON_RIGHT_TRIGGER_2 && sm.b[SNES_BTN_ZL].key == 0);
        CHECK(PADSYS_SNES.btnA == SNES_BTN_A && PADSYS_SNES.btnZR == SNES_BTN_ZR && PADSYS_NES.btnZR == -1);
        CHECK(PadMap_MednafenWordsSys(&PADSYS_SNES, &sm, buf, sizeof(buf), w, 40) == 0);   /* default: nothing handed over */
        sm.custom = true;
        sm.b[SNES_BTN_Y].key = KEY_G;
        sm.b[SNES_BTN_L].key = 0;
        sm.b[SNES_BTN_ZL].key = KEY_Q;
        sm.b[SNES_BTN_X].jsKind = PAD_JS_BUTTON; sm.b[SNES_BTN_X].jsIdx = 7;
        snprintf(sm.joyId, sizeof(sm.joyId), "0003057e200981110004000d00000000");
        {
            int k = PadMap_MednafenWordsSys(&PADSYS_SNES, &sm, buf, sizeof(buf), w, 40);
            CHECK(k == 24 && strcmp(w[0], "-snes.input.port1.gamepad.up") == 0);
            CHECK(strcmp(w[12], "-snes.input.port1.gamepad.y") == 0 && strcmp(w[13], "keyboard 0x0 10") == 0);
            CHECK(strcmp(w[16], "-snes.input.port1.gamepad.x") == 0 && strstr(w[17], "button_7") != NULL);
            CHECK(strcmp(w[20], "-snes.input.port1.gamepad.l") == 0 && strcmp(w[21], "") == 0);
            CHECK(PadMap_MednafenWordsSys(&PADSYS_SNES, &sm, buf, sizeof(buf), w, 23) == 0);
        }
        CHECK(mkdtemp(dir) != NULL);
        snprintf(path, sizeof(path), "%s/nso.cfg", dir);
        f = fopen(path, "w");
        fputs("language=pt\nnes.map.a.key=Q\n", f);
        fclose(f);
        CHECK(PadMap_Save(&PADSYS_SNES, &sm, path));
        f = fopen(path, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = '\0';
        CHECK(strstr(text, "snes.map.y.key=G\n") && strstr(text, "snes.map.zl.key=Q\n") && strstr(text, "snes.map.zr.pad=righttrigger+\n"));
        PadMap_Load(&PADSYS_SNES, &sr, text);
        CHECK(sr.custom && !memcmp(sr.b, sm.b, sizeof(sm.b)) && !strcmp(sr.joyId, sm.joyId));
        PadMap_NesLoad(&r, text);   /* the two systems do not share keys */
        CHECK(r.b[NES_BTN_A].key == KEY_Q && r.b[NES_BTN_UP].key == KEY_UP);
        CHECK(PadMap_Clear(&PADSYS_SNES, path));
        f = fopen(path, "r");
        n = fread(text, 1, sizeof(text) - 1, f);
        fclose(f);
        text[n] = '\0';
        CHECK(!strstr(text, "snes.map") && strstr(text, "nes.map.a.key=Q\n") && strstr(text, "language=pt\n"));
        unlink(path);
        rmdir(dir);

        /* snesrecomp keybinds.ini: [player1] only, keyboard */
        {
            const char *ini = "# SNES\n[player1]\na = X\nb = Z\nl = C\n[player2]\na = None\n";
            CHECK(PadMap_SnesKeybinds(ini, &sm, out, sizeof(out)));
            CHECK(strstr(out, "[player1]\n") && strstr(out, "y = G\n") && strstr(out, "l = none\n") && strstr(out, "x = S\n"));
            CHECK(strstr(out, "[player2]\na = None\n") && !strstr(out, "zl"));
            CHECK(PadMap_SnesKeybinds(out, &sm, again, sizeof(again)) && strcmp(out, again) == 0);
            CHECK(PadMap_SnesKeybinds(NULL, &sm, out, sizeof(out)) && strstr(out, "[player1]\n") == out);
        }
        /* zelda3 / sm ini: the two Controls lines */
        {
            const char *ini = "[General]\nWindow = 1\n[KeyMap]\n#Controls = a, b\nControls = Up, Down, Left, Right, Right Shift, Return, x, z, s, a, c, v\n"
                              "CheatLife = w\n[GamepadMap]\nControls = DpadUp, DpadDown\nTurbo = L2\n[Other]\nx = 1\n";
            CHECK(PadMap_ZeldaMerge(ini, &sm, out, sizeof(out)));
            CHECK(strstr(out, "[General]\nWindow = 1\n[KeyMap]\n#Controls = a, b\n"));
            CHECK(strstr(out, ", Return, x, z, s, g, , c") == NULL);
            CHECK(strstr(out, "Return, x, z, s, g, , v\nCheatLife = w\n") != NULL);
            CHECK(strstr(out, "[GamepadMap]\nControls = DpadUp, DpadDown, DpadLeft, DpadRight, Back, Start, B, A, Y, X, Lb, Rb\nTurbo = L2\n[Other]\nx = 1\n"));
            CHECK(PadMap_ZeldaMerge(out, &sm, again, sizeof(again)) && strcmp(out, again) == 0);
            CHECK(PadMap_ZeldaMerge("", &sm, out, sizeof(out)) && strstr(out, "[KeyMap]\nControls = ") == out && strstr(out, "[GamepadMap]\nControls = "));
            CHECK(!PadMap_ZeldaMerge(ini, &sm, out, 50));
            PadMap_Defaults(&PADSYS_SNES, &sr);
            CHECK(PadMap_ZeldaMerge(ini, &sr, out, sizeof(out)));
            CHECK(strstr(out, "Controls = Up, Down, Left, Right, Right Shift, Return, x, z, s, a, c, v\n") != NULL);
        }
    }

    /* N64: D-pad, stick directions, Start, A, B, Z, L, R, C buttons; nothing for Mednafen */
    {
        PadSysMap nm, nr;
        char path[] = "/tmp/padmap_n64_test.cfg", *text = NULL;
        FILE *f;
        long len;
        CHECK(PADSYS_N64.count == N64_BTN_COUNT && N64_BTN_COUNT <= PAD_MAX_BTN && PADSYS_N64.btnA == N64_BTN_A && PADSYS_N64.btnB == N64_BTN_B);
        PadMap_Defaults(&PADSYS_N64, &nm);
        CHECK(nm.b[N64_BTN_SUP].key == KEY_UP && nm.b[N64_BTN_CUP].key == KEY_I && nm.b[N64_BTN_Z].key == KEY_LEFT_SHIFT);
        CHECK(nm.b[N64_BTN_UP].key == 0 && nm.b[N64_BTN_UP].pad == GAMEPAD_BUTTON_LEFT_FACE_UP && nm.b[N64_BTN_SUP].pad == -1);
        CHECK(nm.b[N64_BTN_A].pad == GAMEPAD_BUTTON_RIGHT_FACE_RIGHT && nm.b[N64_BTN_B].pad == GAMEPAD_BUTTON_RIGHT_FACE_DOWN);
        CHECK(strcmp(PADMAP_N64_NAMES[N64_BTN_CRIGHT], "cright") == 0 && strcmp(PADMAP_N64_NAMES[N64_BTN_SLEFT], "stickleft") == 0);
        nm.b[N64_BTN_Z].key = KEY_Q;
        nm.b[N64_BTN_CLEFT].key = KEY_G;
        nm.b[N64_BTN_SDOWN].jsKind = PAD_JS_AXIS_POS; nm.b[N64_BTN_SDOWN].jsIdx = 1;
        remove(path);
        CHECK(PadMap_Save(&PADSYS_N64, &nm, path));
        f = fopen(path, "r");
        CHECK(f != NULL);
        if (f) {
            fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
            text = calloc(1, (size_t)len + 1);
            CHECK(fread(text, 1, (size_t)len, f) == (size_t)len);
            fclose(f);
            CHECK(strstr(text, "n64.map.z.key=Q\n") && strstr(text, "n64.map.cleft.key=G\n") && strstr(text, "n64.map.stickdown.js=a1+\n"));
            PadMap_Load(&PADSYS_N64, &nr, text);
            CHECK(nr.custom && nr.b[N64_BTN_Z].key == KEY_Q && nr.b[N64_BTN_CLEFT].key == KEY_G && nr.b[N64_BTN_CUP].key == KEY_I);
            CHECK(nr.b[N64_BTN_SDOWN].jsKind == PAD_JS_AXIS_POS && nr.b[N64_BTN_SDOWN].jsIdx == 1);
            free(text);
        }
        CHECK(PadMap_Clear(&PADSYS_N64, path));
        f = fopen(path, "r");
        if (f) { char buf2[64] = { 0 }; CHECK(fread(buf2, 1, sizeof(buf2) - 1, f) == 0); fclose(f); }
        remove(path);
    }

    if (fails) { fprintf(stderr, "%d failure(s)\n", fails); return 1; }
    puts("padmap_test: ok");
    return 0;
}
