/* Lightweight clock / Wi-Fi / battery poll for the main-menu HUD.
 * Time comes from the clock; Wi-Fi and battery are read from sysfs/proc
 * at most once every SYSSTATUS_SYS_INTERVAL seconds and otherwise served
 * from a cache (no per-frame file I/O). */
#ifndef SYSSTATUS_H
#define SYSSTATUS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#endif

#define SYSSTATUS_SYS_INTERVAL  3.0
#define SYSSTATUS_TIME_INTERVAL 1.0

enum {
    SYSSTATUS_WIFI_OFF = 0,
    SYSSTATUS_WIFI_DISCONNECTED = 1,
    SYSSTATUS_WIFI_ON = 2,
};

typedef struct {
    char time[8];
    int hour, minute;
    int wifi;
    bool hasBattery;
    int capacity;
    bool charging;
} SysStatus;

int SysStatus_FormatTime(char *dst, size_t n, int hour, int minute);
int SysStatus_ParseIntLine(const char *s);
bool SysStatus_StatusIsCharging(const char *s);
int SysStatus_ClassifyWifi(bool hasIface, bool operUp, int link);
void SysStatus_Poll(SysStatus *out, double now);

#ifdef SYSSTATUS_IMPLEMENTATION

int SysStatus_FormatTime(char *dst, size_t n, int hour, int minute)
{
    if (!dst || n < 6) return 0;
    if (hour < 0) hour = 0;
    if (hour > 23) hour = 23;
    if (minute < 0) minute = 0;
    if (minute > 59) minute = 59;
    return snprintf(dst, n, "%02d:%02d", hour, minute);
}

int SysStatus_ParseIntLine(const char *s)
{
    char *end = NULL;
    long v;

    if (!s) return -1;
    while (*s == ' ' || *s == '\t') s++;
    if (*s < '0' || *s > '9') return -1;
    v = strtol(s, &end, 10);
    if (end == s) return -1;
    if (v < 0) v = 0;
    if (v > 100) v = 100;
    return (int)v;
}

bool SysStatus_StatusIsCharging(const char *s)
{
    if (!s) return false;
    while (*s == ' ' || *s == '\t') s++;
    return strncmp(s, "Charging", 8) == 0
        && (s[8] == '\0' || s[8] == '\n' || s[8] == '\r' || s[8] == ' ');
}

int SysStatus_ClassifyWifi(bool hasIface, bool operUp, int link)
{
    if (!hasIface) return SYSSTATUS_WIFI_OFF;
    if (operUp && link != 0) return SYSSTATUS_WIFI_ON;
    return SYSSTATUS_WIFI_DISCONNECTED;
}

#ifndef _WIN32
static int sysstatus_read_trim(const char *path, char *dst, size_t n)
{
    FILE *f;
    size_t len;

    if (!path || !dst || n == 0) return 0;
    f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(dst, (int)n, f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    len = strlen(dst);
    while (len > 0 && (dst[len - 1] == '\n' || dst[len - 1] == '\r'
                       || dst[len - 1] == ' ' || dst[len - 1] == '\t')) {
        dst[--len] = '\0';
    }
    return 1;
}

static int sysstatus_wireless_link(const char *iface)
{
    FILE *f;
    char line[256], name[64];
    int status = 0, link = -1;
    size_t n;

    if (!iface) return -1;
    n = strlen(iface);
    f = fopen("/proc/net/wireless", "r");
    if (!f) return -1;
    while (fgets(line, (int)sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, iface, n) != 0) continue;
        if (p[n] != ':' && p[n] != ' ' && p[n] != '\t') continue;
        /* "wlan0: 0000  70.  -40.  ..." — link is the first Quality column. */
        if (sscanf(p, "%63[^:]: %x %d.", name, &status, &link) >= 3) {
            fclose(f);
            return link;
        }
    }
    fclose(f);
    return -1;
}

static void sysstatus_poll_wifi(SysStatus *s)
{
    DIR *d;
    struct dirent *ent;
    bool hasIface = false, operUp = false;
    int link = -1;

    d = opendir("/sys/class/net");
    if (d) {
        while ((ent = readdir(d)) != NULL) {
            char path[512], state[64];
            struct stat st;

            if (ent->d_name[0] == '.') continue;
            if (strcmp(ent->d_name, "lo") == 0) continue;
            snprintf(path, sizeof(path), "/sys/class/net/%s/wireless", ent->d_name);
            if (stat(path, &st) != 0) continue;
            hasIface = true;
            snprintf(path, sizeof(path), "/sys/class/net/%s/operstate", ent->d_name);
            if (sysstatus_read_trim(path, state, sizeof(state))
                && strcmp(state, "up") == 0) {
                int l = sysstatus_wireless_link(ent->d_name);
                operUp = true;
                if (l > link) link = l;
            }
        }
        closedir(d);
    }
    s->wifi = SysStatus_ClassifyWifi(hasIface, operUp, link);
}

static void sysstatus_poll_battery(SysStatus *s)
{
    DIR *d;
    struct dirent *ent;

    s->hasBattery = false;
    s->capacity = 0;
    s->charging = false;

    d = opendir("/sys/class/power_supply");
    if (!d) return;
    while ((ent = readdir(d)) != NULL) {
        char path[512], type[64], cap[32], status[64];
        int v;
        bool isBat;

        if (ent->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "/sys/class/power_supply/%s/type", ent->d_name);
        isBat = (strncmp(ent->d_name, "BAT", 3) == 0);
        if (sysstatus_read_trim(path, type, sizeof(type))
            && strcmp(type, "Battery") == 0) {
            isBat = true;
        }
        if (!isBat) continue;
        snprintf(path, sizeof(path), "/sys/class/power_supply/%s/capacity", ent->d_name);
        if (!sysstatus_read_trim(path, cap, sizeof(cap))) continue;
        v = SysStatus_ParseIntLine(cap);
        if (v < 0) continue;
        snprintf(path, sizeof(path), "/sys/class/power_supply/%s/status", ent->d_name);
        sysstatus_read_trim(path, status, sizeof(status));
        s->hasBattery = true;
        s->capacity = v;
        s->charging = SysStatus_StatusIsCharging(status);
        break;
    }
    closedir(d);
}
#endif /* !_WIN32 */

void SysStatus_Poll(SysStatus *out, double now)
{
    static SysStatus cache;
    static double lastSys = -1.0e9, lastTime = -1.0e9;
    static bool inited = false;

    if (!out) return;
    if (!inited || now - lastTime >= SYSSTATUS_TIME_INTERVAL) {
        time_t t = time(NULL);
        struct tm tm = { 0 };
#ifdef _WIN32
        struct tm *tmp = localtime(&t);
        if (tmp) tm = *tmp;
#else
        localtime_r(&t, &tm);
#endif
        cache.hour = tm.tm_hour;
        cache.minute = tm.tm_min;
        SysStatus_FormatTime(cache.time, sizeof(cache.time), cache.hour, cache.minute);
        lastTime = now;
    }
    if (!inited || now - lastSys >= SYSSTATUS_SYS_INTERVAL) {
#ifdef _WIN32
        cache.wifi = SYSSTATUS_WIFI_OFF;
        cache.hasBattery = false;
        cache.capacity = 0;
        cache.charging = false;
#else
        sysstatus_poll_wifi(&cache);
        sysstatus_poll_battery(&cache);
#endif
        lastSys = now;
    }
    inited = true;
    *out = cache;
}

#endif /* SYSSTATUS_IMPLEMENTATION */
#endif /* SYSSTATUS_H */
