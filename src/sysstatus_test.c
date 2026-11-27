/* sysstatus_test.c — time format, sysfs parsers, Wi-Fi classification. */
#define SYSSTATUS_IMPLEMENTATION
#include "sysstatus.h"
#include <stdio.h>
#include <string.h>

static int fails = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #c); fails++; } } while (0)

int main(void)
{
    char buf[8];

    CHECK(SysStatus_FormatTime(buf, sizeof(buf), 19, 42) > 0);
    CHECK(strcmp(buf, "19:42") == 0);
    CHECK(SysStatus_FormatTime(buf, sizeof(buf), 0, 0) > 0);
    CHECK(strcmp(buf, "00:00") == 0);
    CHECK(SysStatus_FormatTime(buf, sizeof(buf), 23, 59) > 0);
    CHECK(strcmp(buf, "23:59") == 0);
    CHECK(SysStatus_FormatTime(buf, sizeof(buf), -1, 99) > 0);
    CHECK(strcmp(buf, "00:59") == 0);

    CHECK(SysStatus_ParseIntLine("100\n") == 100);
    CHECK(SysStatus_ParseIntLine("  7") == 7);
    CHECK(SysStatus_ParseIntLine("0") == 0);
    CHECK(SysStatus_ParseIntLine("200") == 100);
    CHECK(SysStatus_ParseIntLine("abc") == -1);
    CHECK(SysStatus_ParseIntLine("") == -1);
    CHECK(SysStatus_ParseIntLine(NULL) == -1);

    CHECK(SysStatus_StatusIsCharging("Charging\n"));
    CHECK(SysStatus_StatusIsCharging("Charging"));
    CHECK(!SysStatus_StatusIsCharging("Full\n"));
    CHECK(!SysStatus_StatusIsCharging("Discharging"));
    CHECK(!SysStatus_StatusIsCharging("Not charging"));
    CHECK(!SysStatus_StatusIsCharging(NULL));

    CHECK(SysStatus_ClassifyWifi(false, false, -1) == SYSSTATUS_WIFI_OFF);
    CHECK(SysStatus_ClassifyWifi(true, true, 70) == SYSSTATUS_WIFI_ON);
    CHECK(SysStatus_ClassifyWifi(true, true, -1) == SYSSTATUS_WIFI_ON);
    CHECK(SysStatus_ClassifyWifi(true, true, 0) == SYSSTATUS_WIFI_DISCONNECTED);
    CHECK(SysStatus_ClassifyWifi(true, false, 70) == SYSSTATUS_WIFI_DISCONNECTED);

    if (fails) {
        fprintf(stderr, "%d checks failed\n", fails);
        return 1;
    }
    puts("sysstatus_test: ok");
    return 0;
}
