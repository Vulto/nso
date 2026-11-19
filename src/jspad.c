/* jspad.c - the device side of jspad.h. No raylib.h here (<linux/input.h> KEY_* clash). */
#define _POSIX_C_SOURCE 200809L
#include "jspad.h"

#if defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define JSP_MAX 8
#define JS_BTN 64
#define JS_AXES 24
#define JS_FAR 24000     /* an axis counts as pressed this far from its rest */
#define JS_NEAR 12000

typedef struct {
    int fd;
    char id[40];
    int buttons, axes;
    unsigned char btn[JS_BTN];
    int rest[JS_AXES], val[JS_AXES];
    bool engaged[JS_AXES];
} JsDev;

static JsDev devs[JSP_MAX];
static int devCount = 0;
static bool havePress = false;
static int pressKind, pressIdx, pressDev;

static unsigned SysHex(int js, const char *file)
{
    char path[128], v[32];
    FILE *f;
    snprintf(path, sizeof(path), "/sys/class/input/js%d/device/id/%s", js, file);
    f = fopen(path, "r");
    if (!f) return 0;
    if (!fgets(v, sizeof(v), f)) v[0] = '\0';
    fclose(f);
    return (unsigned)strtoul(v, NULL, 16);
}

/* Mednafen's Joystick_Linux id: bus, vendor, product, version, axes, buttons (16 bits
 * each, big-endian), then 32 zero bits. */
static bool DevId(int fd, int js, char *out, size_t n, int *axes, int *buttons)
{
    unsigned char a = 0, b = 0;
    if (ioctl(fd, JSIOCGAXES, &a) < 0 || ioctl(fd, JSIOCGBUTTONS, &b) < 0) return false;
    snprintf(out, n, "%04x%04x%04x%04x%04x%04x00000000", SysHex(js, "bustype") & 0xffff, SysHex(js, "vendor") & 0xffff,
             SysHex(js, "product") & 0xffff, SysHex(js, "version") & 0xffff, (unsigned)a, (unsigned)b);
    if (axes) *axes = a;
    if (buttons) *buttons = b;
    return true;
}

void JsPad_Close(void)
{
    for (int i = 0; i < devCount; i++) if (devs[i].fd >= 0) close(devs[i].fd);
    devCount = 0;
    havePress = false;
}

bool JsPad_Open(void)
{
    JsPad_Close();
    for (int js = 0; js < 32 && devCount < JSP_MAX; js++) {
        char path[64];
        JsDev *d = &devs[devCount];
        int fd;
        snprintf(path, sizeof(path), "/dev/input/js%d", js);
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        memset(d, 0, sizeof(*d));
        d->fd = fd;
        if (!DevId(fd, js, d->id, sizeof(d->id), &d->axes, &d->buttons)) { close(fd); continue; }
        devCount++;
    }
    return devCount > 0;
}

void JsPad_Update(void)
{
    for (int i = 0; i < devCount; i++) {
        JsDev *d = &devs[i];
        struct js_event e;
        while (read(d->fd, &e, sizeof(e)) == (ssize_t)sizeof(e)) {
            bool init = (e.type & JS_EVENT_INIT) != 0;
            if ((e.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON && e.number < JS_BTN) {
                d->btn[e.number] = e.value ? 1 : 0;
                if (!init && e.value && !havePress) { havePress = true; pressKind = 1; pressIdx = e.number; pressDev = i; }
            } else if ((e.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS && e.number < JS_AXES) {
                int dv, k = e.number;
                d->val[k] = e.value;
                if (init) { d->rest[k] = e.value; continue; }
                dv = e.value - d->rest[k];
                if (!d->engaged[k] && (dv > JS_FAR || dv < -JS_FAR)) {
                    d->engaged[k] = true;
                    if (!havePress) { havePress = true; pressKind = dv > 0 ? 3 : 2; pressIdx = k; pressDev = i; }
                } else if (d->engaged[k] && dv < JS_NEAR && dv > -JS_NEAR) {
                    d->engaged[k] = false;
                }
            }
        }
    }
}

bool JsPad_AnyDown(void)
{
    for (int i = 0; i < devCount; i++) {
        const JsDev *d = &devs[i];
        for (int b = 0; b < JS_BTN; b++) if (d->btn[b]) return true;
        for (int a = 0; a < JS_AXES; a++) if (d->engaged[a]) return true;
    }
    return false;
}

bool JsPad_Down(int kind, int idx)
{
    for (int i = 0; i < devCount; i++) {
        const JsDev *d = &devs[i];
        if (idx < 0) continue;
        if (kind == 1 && idx < JS_BTN && d->btn[idx]) return true;
        if ((kind == 2 || kind == 3) && idx < JS_AXES) {
            int dv = d->val[idx] - d->rest[idx];
            if (kind == 3 ? dv > JS_FAR : dv < -JS_FAR) return true;
        }
    }
    return false;
}

bool JsPad_TakePress(int *kind, int *idx, char *devId, size_t n)
{
    if (!havePress) return false;
    havePress = false;
    *kind = pressKind;
    *idx = pressIdx;
    if (devId && n) snprintf(devId, n, "%s", devs[pressDev].id);
    return true;
}

bool JsPad_FirstId(char *out, size_t n)
{
    for (int js = 0; js < 32; js++) {
        char path[64];
        int fd;
        bool ok;
        snprintf(path, sizeof(path), "/dev/input/js%d", js);
        fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0) continue;
        ok = DevId(fd, js, out, n, NULL, NULL);
        close(fd);
        if (ok) return true;
    }
    return false;
}

#else /* not Linux: no raw joystick */

bool JsPad_Open(void) { return false; }
void JsPad_Close(void) {}
void JsPad_Update(void) {}
bool JsPad_AnyDown(void) { return false; }
bool JsPad_Down(int kind, int idx) { (void)kind; (void)idx; return false; }
bool JsPad_TakePress(int *kind, int *idx, char *devId, size_t n) { (void)kind; (void)idx; (void)devId; (void)n; return false; }
bool JsPad_FirstId(char *out, size_t n) { (void)out; (void)n; return false; }

#endif
