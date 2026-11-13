#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
/* input_test.c - Nintendo confirm/back mapping. */
#include "nso.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

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
    CHECK(Nso_PadNameIsN64("8BitDo N64"));
    CHECK(Nso_PadNameIsN64("8BitDo N64 Modkit"));
    CHECK(Nso_PadNameIsN64("NSO N64 Controller"));
    CHECK(!Nso_PadNameIsN64("Xbox Controller"));
    CHECK(!Nso_PadNameIsN64(NULL));
    CHECK(Nso_PadProfileFromName("8BitDo N64 Modkit") == NSO_PAD_PROFILE_N64);
    CHECK(Nso_PadProfileFromName("Xbox Controller") == NSO_PAD_PROFILE_DEFAULT);
    CHECK(Nso_PadProfileFromName(NULL) == NSO_PAD_PROFILE_DEFAULT);
    {
        NsoPadProfile p;
        setenv("NSO_PAD_STYLE", "n64", 1);
        CHECK(Nso_PadStyleEnvOverride(&p) && p == NSO_PAD_PROFILE_N64);
        setenv("NSO_PAD_STYLE", "default", 1);
        CHECK(Nso_PadStyleEnvOverride(&p) && p == NSO_PAD_PROFILE_DEFAULT);
        unsetenv("NSO_PAD_STYLE");
        CHECK(!Nso_PadStyleEnvOverride(&p));
    }
    {
        const char *src = "    🅨  Move      🅧 Details      🅐 Start      🅑 Back   ";
        char buf[256];
        const char *def = Nso_HintTextForProfile(NSO_PAD_PROFILE_DEFAULT, src, buf, sizeof buf);
        CHECK(def == src);
        const char *n64 = Nso_HintTextForProfile(NSO_PAD_PROFILE_N64, src, buf, sizeof buf);
        CHECK(n64 == buf);
        CHECK(strstr(n64, "C-Down") != NULL);
        CHECK(strstr(n64, "C-Up") != NULL);
        CHECK(strstr(n64, "Start") != NULL);
        CHECK(strstr(n64, "C-Down") < strstr(n64, "C-Up"));
    }
    CHECK(Nso_PadAxisEdge(0.0f, 0.4f) == 0);    /* inside deadzone */
    CHECK(Nso_PadAxisEdge(0.0f, 0.5f) == 0);    /* equal to deadzone is not an edge */
    CHECK(Nso_PadAxisEdge(0.0f, 0.51f) == 1);
    CHECK(Nso_PadAxisEdge(0.6f, 0.8f) == 0);    /* already past: no repeat */
    CHECK(Nso_PadAxisEdge(0.6f, 0.2f) == 0);
    CHECK(Nso_PadAxisEdge(0.0f, -0.51f) == -1);
    CHECK(Nso_PadAxisEdge(-0.6f, -0.8f) == 0);
    CHECK(Nso_PadAxisEdge(-0.51f, 0.51f) == 1);
    CHECK(NSO_PAD_DEADZONE == 0.5f);
    CHECK(Nso_PadAxisEdge(0.0f, -0.51f) == -1); /* the connect-time false C-left */
    {
        NsoPadAxisArm a;
        Nso_PadAxisArmReset(&a);
        CHECK(!a.armed && Nso_PadAxisEdgeArmed(&a, -1.0f) == 0); /* first off-centre: no edge */
        CHECK(!a.armed && Nso_PadAxisEdgeArmed(&a, -1.0f) == 0);
        CHECK(Nso_PadAxisEdgeArmed(&a, 0.0f) == 0 && a.armed);
        CHECK(Nso_PadAxisEdgeArmed(&a, -0.51f) == -1);
        CHECK(Nso_PadAxisEdgeArmed(&a, -0.8f) == 0);
        CHECK(Nso_PadAxisEdgeArmed(&a, 0.0f) == 0);
        CHECK(Nso_PadAxisEdgeArmed(&a, 0.51f) == 1);
        Nso_PadAxisArmReset(&a);
        CHECK(!a.armed && Nso_PadAxisEdgeArmed(&a, 0.51f) == 0);
    }
    CHECK(Nso_PadProfileFromLink(NSO_PAD_LINK_BT) == NSO_PAD_PROFILE_N64);
    CHECK(Nso_PadProfileFromLink(NSO_PAD_LINK_USB) == NSO_PAD_PROFILE_N64);
    CHECK(Nso_PadProfileFromLink(NSO_PAD_LINK_NONE) == NSO_PAD_PROFILE_DEFAULT);
    CHECK(Nso_PadNameIsN64("8BitDo  8BitDo N64 Modkit"));
    CHECK(NSO_PAD_SETTLE_SEC > 0.0f);
    {
        NsoPadWatch w = { 0 };
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 0.016f, 0.4f);
        CHECK(w.inited && w.live == NSO_PAD_PROFILE_N64 && w.drawn == NSO_PAD_PROFILE_N64);
        CHECK(w.fade == 1.0f && !w.notice && !w.closing);
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_DEFAULT, 0.016f, 0.4f);
        CHECK(w.live == NSO_PAD_PROFILE_DEFAULT && w.drawn == NSO_PAD_PROFILE_N64);
        CHECK(w.closing && !w.notice && w.fade < 1.0f);
        for (int i = 0; i < 40 && w.closing; i++)
            Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_DEFAULT, 0.016f, 0.4f);
        CHECK(!w.closing && w.drawn == NSO_PAD_PROFILE_DEFAULT && w.fade == 0.0f && !w.notice);
        for (int i = 0; i < 40 && w.fade < 1.0f; i++)
            Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_DEFAULT, 0.016f, 0.4f);
        CHECK(w.fade == 1.0f && !w.notice);
    }
    {
        NsoPadWatch w = { .fade = 1.0f };
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_DEFAULT, 0.0f, 0.4f);
        CHECK(!w.notice && w.drawn == NSO_PAD_PROFILE_DEFAULT);
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 0.0f, 0.4f);
        CHECK(w.notice && w.closing && w.drawn == NSO_PAD_PROFILE_DEFAULT);
        w.notice = false;
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 0.4f, 0.4f); /* complete close */
        CHECK(!w.closing && w.drawn == NSO_PAD_PROFILE_N64 && w.fade == 0.0f && !w.notice);
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 0.4f, 0.4f); /* complete open */
        CHECK(w.fade == 1.0f && w.drawn == NSO_PAD_PROFILE_N64 && !w.notice);
    }
    {
        /* fadeSec 0 snaps; mid-open reconnect to default closes then opens default. */
        NsoPadWatch w = { 0 };
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_DEFAULT, 1.0f, 0.0f);
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 1.0f, 0.0f);
        CHECK(w.notice && w.drawn == NSO_PAD_PROFILE_N64 && w.fade == 0.0f);
        Nso_PadWatchUpdate(&w, NSO_PAD_PROFILE_N64, 1.0f, 0.0f);
        CHECK(w.fade == 1.0f);
    }
    CHECK(Nso_PadPlayerFromIndex(-1) == 0);
    CHECK(Nso_PadPlayerFromIndex(0) == 1);
    CHECK(Nso_PadPlayerFromIndex(2) == 3);
    CHECK(Nso_PadPlayerFromIndex(9) == 4);
    CHECK(Nso_PadPlayerLedOn(1, 1) && !Nso_PadPlayerLedOn(1, 2) && !Nso_PadPlayerLedOn(1, 4));
    CHECK(Nso_PadPlayerLedOn(3, 1) && Nso_PadPlayerLedOn(3, 3) && !Nso_PadPlayerLedOn(3, 4));
    CHECK(Nso_PadTextHasMac("hid-e4:17:d8:9d:7f:79-battery", "E4:17:D8:9D:7F:79"));
    CHECK(Nso_PadTextHasMac("UNIQ=e417d89d7f79", "e4:17:d8:9d:7f:79"));
    CHECK(!Nso_PadTextHasMac("hid-aa:bb:cc:dd:ee:ff-battery", "e4:17:d8:9d:7f:79"));
    {
        const char *bt =
            "I: Bus=0005 Vendor=2dc8 Product=2869 Version=0111\n"
            "N: Name=\"8BitDo N64 Modkit\"\n"
            "P: Phys=c0:3c:59:30:f4:fc\n"
            "S: Sysfs=/devices/virtual/misc/uhid/0005:2DC8:2869.000D/input/input34\n"
            "U: Uniq=e4:17:d8:9d:7f:79\n\n";
        const char *usb =
            "I: Bus=0003 Vendor=2dc8 Product=2869 Version=0111\n"
            "N: Name=\"8BitDo N64 Modkit\"\n"
            "P: Phys=usb-0000:00:14.0-1/input0\n"
            "S: Sysfs=/devices/pci0000:00/usb1/1-1/input/input8\n"
            "U: Uniq=\n\n";
        const char *xbox =
            "I: Bus=0003 Vendor=045e Product=028e Version=0110\n"
            "N: Name=\"Xbox Controller\"\n"
            "P: Phys=usb-0000:00:14.0-2/input0\n"
            "S: Sysfs=/devices/pci0000:00/usb1/1-2/input/input9\n"
            "U: Uniq=\n\n";
        CHECK(Nso_PadLinkFromProc(bt, "e4:17:d8:9d:7f:79") == NSO_PAD_LINK_BT);
        CHECK(Nso_PadLinkFromProc(bt, NULL) == NSO_PAD_LINK_BT);
        CHECK(Nso_PadLinkFromProc(usb, "e4:17:d8:9d:7f:79") == NSO_PAD_LINK_USB);
        CHECK(Nso_PadLinkFromProc(usb, NULL) == NSO_PAD_LINK_USB);
        CHECK(Nso_PadLinkFromProc(xbox, NULL) == NSO_PAD_LINK_NONE);
        CHECK(Nso_PadLinkFromProc(bt, "aa:bb:cc:dd:ee:ff") == NSO_PAD_LINK_NONE);
    }
    CHECK(Nso_PadBatteryParseCapacity("67\n") == 67);
    CHECK(Nso_PadBatteryParseCapacity("100") == 100);
    CHECK(Nso_PadBatteryParseCapacity("101") == -1);
    CHECK(Nso_PadBatteryParseCapacity("") == -1);
    {
        char dir[] = "/tmp/nso-batt-XXXXXX";
        char hid[256], bat[256], path[300];
        CHECK(mkdtemp(dir) != NULL);
        snprintf(hid, sizeof hid, "%s/hid-e4:17:d8:9d:7f:79-battery", dir);
        snprintf(bat, sizeof bat, "%s/BAT0", dir);
        CHECK(mkdir(hid, 0700) == 0 && mkdir(bat, 0700) == 0);
        snprintf(path, sizeof path, "%s/capacity", hid);
        FILE *f = fopen(path, "w");
        if (f) { fputs("67\n", f); fclose(f); }
        snprintf(path, sizeof path, "%s/uevent", hid);
        f = fopen(path, "w");
        if (f) { fputs("POWER_SUPPLY_NAME=hid-e4:17:d8:9d:7f:79-battery\nHID_UNIQ=e4:17:d8:9d:7f:79\n", f); fclose(f); }
        snprintf(path, sizeof path, "%s/capacity", bat);
        f = fopen(path, "w");
        if (f) { fputs("100\n", f); fclose(f); }
        CHECK(Nso_PadBatteryRead(dir, "e4:17:d8:9d:7f:79") == 67);
        CHECK(Nso_PadBatteryRead(dir, NULL) == 67);
        CHECK(Nso_PadBatteryRead(dir, "aa:bb:cc:dd:ee:ff") == -1);
        /* cleanup */
        snprintf(path, sizeof path, "%s/capacity", hid); unlink(path);
        snprintf(path, sizeof path, "%s/uevent", hid); unlink(path);
        snprintf(path, sizeof path, "%s/capacity", bat); unlink(path);
        rmdir(hid); rmdir(bat); rmdir(dir);
    }
    if (fails) return 1;
    puts("input_test: ok");
    return 0;
}
