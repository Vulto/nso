/* n64pad.c - the device side of n64pad.h: evdev FF_RUMBLE and the Switch-mode
 * N64 pad's GLFW mapping. No raylib.h here (<linux/input.h> KEY_* clash). */
#define _POSIX_C_SOURCE 200809L
#define N64PAD_NO_RAYLIB
#include "n64pad.h"

#if defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <math.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

int SetGamepadMappings(const char *mappings);   /* raylib */

#define N64PAD_MAX 8

static struct {
    bool enabled;
    int fd;                 /* evdev node with FF_RUMBLE, -1 = none */
    int effect;             /* uploaded effect id, -1 = none */
    unsigned magnitude;     /* of the uploaded effect */
    double lastPlay, nextScan;
    bool playing;
    char guids[N64PAD_MAX][33];
    int guidCount;
    char switchName[N64PAD_MAX][128];   /* evdev names of Switch-mode N64 pads */
    int switchCount;
} pad = { .enabled = true, .fd = -1, .effect = -1 };

static double Now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

static bool ReadSys(const char *dev, const char *file, char *out, size_t cap)
{
    char path[600];
    snprintf(path, sizeof(path), "/sys/class/input/%s/device/%s", dev, file);
    FILE *f = fopen(path, "r");
    if (!f) return false;
    size_t n = fread(out, 1, cap - 1, f);
    fclose(f);
    out[n] = '\0';
    char *nl = strchr(out, '\n');
    if (nl) *nl = '\0';
    return n > 0;
}

static unsigned ReadHex(const char *dev, const char *file)
{
    char v[32];
    return ReadSys(dev, file, v, sizeof(v)) ? (unsigned)strtoul(v, NULL, 16) : 0;
}

static void CloseDevice(void)
{
    if (pad.fd >= 0) close(pad.fd);
    pad.fd = -1;
    pad.effect = -1;
    pad.playing = false;
}

static bool Play(int value)
{
    struct input_event ev = { .type = EV_FF, .code = (unsigned short)pad.effect, .value = value };
    if (write(pad.fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) return true;
    CloseDevice();   /* unplugged: the next scan finds it again */
    return false;
}

void N64Pad_Poll(void)
{
    double now = Now();
    if (now < pad.nextScan) return;
    pad.nextScan = now + 1.0;
    DIR *d = opendir("/sys/class/input");
    if (!d) return;
    char best[300] = { 0 };
    bool bestN64 = false;
    int switchCount = 0;
    struct dirent *e;
    while ((e = readdir(d))) {
        if (strncmp(e->d_name, "event", 5) != 0) continue;
        char name[128] = { 0 }, ff[256] = { 0 }, key[1024] = { 0 };
        ReadSys(e->d_name, "name", name, sizeof(name));
        ReadSys(e->d_name, "capabilities/key", key, sizeof(key));
        if (!N64Pad_CapBit(key, BTN_A) && !N64Pad_CapBit(key, BTN_TRIGGER)) continue;   /* not a pad */
        unsigned vendor = ReadHex(e->d_name, "id/vendor");
        unsigned product = ReadHex(e->d_name, "id/product");
        if (vendor == N64PAD_SWITCH_VENDOR && product == N64PAD_SWITCH_PRODUCT) {
            char guid[33];
            N64Pad_Guid(guid, ReadHex(e->d_name, "id/bustype"), vendor, product, ReadHex(e->d_name, "id/version"));
            bool known = false;
            for (int i = 0; i < pad.guidCount; i++) known |= strcmp(pad.guids[i], guid) == 0;
            if (!known && pad.guidCount < N64PAD_MAX) {
                char map[512];
                N64Pad_SwitchMapping(map, sizeof(map), guid);
                SetGamepadMappings(map);   /* also remaps a pad that is already connected */
                snprintf(pad.guids[pad.guidCount++], sizeof(pad.guids[0]), "%s", guid);
            }
            if (switchCount < N64PAD_MAX)
                snprintf(pad.switchName[switchCount++], sizeof(pad.switchName[0]), "%s", name);
        }
        if (pad.fd < 0 && ReadSys(e->d_name, "capabilities/ff", ff, sizeof(ff)) && N64Pad_CapBit(ff, FF_RUMBLE)) {
            bool isN64 = strstr(name, "N64") || strstr(name, "8BitDo 64");
            if (!best[0] || (isN64 && !bestN64)) {
                snprintf(best, sizeof(best), "/dev/input/%s", e->d_name);
                bestN64 = isN64;
            }
        }
    }
    closedir(d);
    pad.switchCount = switchCount;
    if (pad.fd < 0 && best[0]) pad.fd = open(best, O_RDWR | O_NONBLOCK | O_CLOEXEC);
}

bool N64Pad_IsSwitchName(const char *name)
{
    for (int i = 0; name && i < pad.switchCount; i++)
        if (strcmp(name, pad.switchName[i]) == 0) return true;
    return false;
}

void N64Pad_SetEnabled(bool on)
{
    pad.enabled = on;
    if (!on) N64Pad_Rumble(0.0f);
}

void N64Pad_Rumble(float level)
{
    if (pad.fd < 0) return;
    if (!pad.enabled || !(level > 0.0f)) {
        if (pad.playing && Play(0)) pad.playing = false;
        return;
    }
    unsigned mag = (unsigned)(fminf(level, 1.0f) * 0xffff);
    if (pad.effect < 0 || mag != pad.magnitude) {
        struct ff_effect fx = { .type = FF_RUMBLE, .id = (short)pad.effect };
        fx.u.rumble.strong_magnitude = (unsigned short)mag;
        fx.u.rumble.weak_magnitude = (unsigned short)mag;
        fx.replay.length = N64PAD_HOLD_MS;
        if (ioctl(pad.fd, EVIOCSFF, &fx) < 0) { CloseDevice(); return; }
        pad.effect = fx.id;
        pad.magnitude = mag;
        pad.playing = false;
    }
    /* Re-trigger before the hold runs out, not every frame. */
    double now = Now();
    if (!pad.playing || now - pad.lastPlay > N64PAD_HOLD_MS / 2000.0) {
        if (!Play(1)) return;
        pad.lastPlay = now;
        pad.playing = true;
    }
}
#else
void N64Pad_Poll(void) {}
bool N64Pad_IsSwitchName(const char *name) { (void)name; return false; }
void N64Pad_SetEnabled(bool on) { (void)on; }
void N64Pad_Rumble(float level) { (void)level; }
#endif
