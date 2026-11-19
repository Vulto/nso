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

    if (fails) { fprintf(stderr, "%d failure(s)\n", fails); return 1; }
    puts("padmap_test: ok");
    return 0;
}
