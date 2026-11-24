#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <X11/extensions/shape.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "menu_launcher.h"

static volatile sig_atomic_t nso_menu_requested;
static volatile sig_atomic_t nso_menu_home;
static volatile sig_atomic_t nso_menu_park;
static int nso_menu_home_held;

static void nso_menu_request_signal(int sig)
{
    (void)sig;
    nso_menu_requested = 1;
}

/* Host Home toggle: stop the game without asking NSO to open the End menu. */
static void nso_menu_park_signal(int sig)
{
    (void)sig;
    nso_menu_park = 1;
}

static volatile sig_atomic_t nso_menu_stop;

static void nso_menu_stop_signal(int sig)
{
    (void)sig;
    nso_menu_stop = 1;
}

/* End the game's whole process group (it may be stopped by the menu). */
static void nso_menu_end_game(pid_t game, int *status);

static unsigned long nso_menu_env_window(const char *key)
{
    const char *s = getenv(key);
    char *end = NULL;
    unsigned long value;
    if (!s || !*s) return 0;
    errno = 0;
    value = strtoul(s, &end, 10);
    return errno || !end || *end ? 0 : value;
}

static bool nso_menu_pid_in_group(unsigned long pid, pid_t group)
{
    char path[64], line[512];
    snprintf(path, sizeof(path), "/proc/%lu/stat", pid);
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool match = false;
    if (fgets(line, sizeof(line), f)) {
        char *end = strrchr(line, ')');
        char state;
        long ppid, pgrp;
        if (end && sscanf(end + 1, " %c %ld %ld", &state, &ppid, &pgrp) == 3)
            match = pgrp == (long)group;
    }
    fclose(f);
    return match;
}

static bool nso_menu_window_pid(Display *dpy, Window w, pid_t group)
{
    Atom atom = XInternAtom(dpy, "_NET_WM_PID", True);
    Atom actual;
    int format;
    unsigned long count, after;
    unsigned char *data = NULL;
    bool match = false;
    if (atom == None || XGetWindowProperty(dpy, w, atom, 0, 1, False, XA_CARDINAL,
                                            &actual, &format, &count, &after,
                                            &data) != Success) return false;
    if (data && actual == XA_CARDINAL && format == 32 && count == 1)
        match = nso_menu_pid_in_group(*(unsigned long *)data, group);
    if (data) XFree(data);
    return match;
}

/* SDL/Vulkan ports (the N64Recomp ones) keep a 10x10 unmapped helper window with
 * the game's PID alive for the whole run, besides the real one that maps later. */
static bool nso_menu_viewable(Display *dpy, Window w)
{
    XWindowAttributes attrs;
    return XGetWindowAttributes(dpy, w, &attrs) && attrs.map_state == IsViewable;
}

static Window nso_menu_find_window(Display *dpy, Window root, pid_t group, int depth)
{
    Window parent, *children = NULL;
    unsigned int count = 0;
    Window found = None;
    if (depth > 8 || !XQueryTree(dpy, root, &root, &parent, &children, &count)) return None;
    for (unsigned int i = 0; i < count && found == None; ++i) {
        if (nso_menu_window_pid(dpy, children[i], group) && nso_menu_viewable(dpy, children[i])) found = children[i];
        else found = nso_menu_find_window(dpy, children[i], group, depth + 1);
    }
    if (children) XFree(children);
    return found;
}

static double nso_menu_now(void);

static int nso_menu_ignore_x_error(Display *dpy, XErrorEvent *ev)
{
    (void)dpy; (void)ev;   /* the game's temporary windows vanish under us */
    return 0;
}

/* Wait for the child's top-level window. Flag it override-redirect the moment
 * it is created so a tiling WM never manages (and re-tiles around) it, then
 * return once it is mapped: the caller reparents it at once. Falls back to
 * scanning the tree for windows that were missed. */
static Window nso_menu_wait_window(Display *dpy, Window root, pid_t group)
{
    Window pending[16], cands[16];
    int npending = 0, ncands = 0;
    Window found = None;
    double scan_at = 0.0;
    int (*old_handler)(Display *, XErrorEvent *) = XSetErrorHandler(nso_menu_ignore_x_error);
    XSelectInput(dpy, root, SubstructureNotifyMask);
    XSync(dpy, False);
    for (int i = 0; i < 5000 && found == None; ++i) {
        while (found == None && XPending(dpy)) {
            XEvent ev;
            XNextEvent(dpy, &ev);
            if (ev.type == CreateNotify && ev.xcreatewindow.parent == root
                && !ev.xcreatewindow.override_redirect && npending < 16) {
                pending[npending++] = ev.xcreatewindow.window;
            } else if (ev.type == DestroyNotify) {
                for (int k = 0; k < npending; ++k)
                    if (pending[k] == ev.xdestroywindow.window) pending[k] = pending[--npending];
                for (int k = 0; k < ncands; ++k)
                    if (cands[k] == ev.xdestroywindow.window) cands[k] = cands[--ncands];
            } else if (ev.type == MapNotify) {
                /* The first of the game's windows to map is the game's: helper
                 * windows that stay unmapped are never taken. */
                for (int k = 0; k < ncands; ++k)
                    if (cands[k] == ev.xmap.window) found = cands[k];
                for (int k = 0; k < npending && found == None; ++k)   /* mapped before it was flagged */
                    if (pending[k] == ev.xmap.window && nso_menu_window_pid(dpy, pending[k], group)) found = pending[k];
            }
        }
        if (found != None) break;
        for (int k = 0; k < npending && ncands < 16;) {   /* the PID is set after creation */
            if (!nso_menu_window_pid(dpy, pending[k], group)) { ++k; continue; }
            XSetWindowAttributes attrs = { 0 };
            attrs.override_redirect = True;
            XChangeWindowAttributes(dpy, pending[k], CWOverrideRedirect, &attrs);
            XFlush(dpy);
            cands[ncands++] = pending[k];
            pending[k] = pending[--npending];
        }
        double now = nso_menu_now();
        if (now >= scan_at) {
            scan_at = now + 0.25;
            Window seen = nso_menu_find_window(dpy, root, group, 0);
            if (seen != None) found = seen;
        }
        if (found == None && !XPending(dpy)) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 1000000L };
            nanosleep(&ts, NULL);
        }
    }
    XSelectInput(dpy, root, 0);
    XSync(dpy, False);
    XSetErrorHandler(old_handler);
    return found;
}

static void nso_menu_write_window(const char *dir, Window w)
{
    char path[1200], tmp[1240];
    if (!dir || !*dir) return;
    if (snprintf(path, sizeof(path), "%s/nso-menu-window", dir) >= (int)sizeof(path)
        || snprintf(tmp, sizeof(tmp), "%s.tmp", path) >= (int)sizeof(tmp)) return;
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    bool ok = fprintf(f, "%lu\n", (unsigned long)w) > 0;
    if (fclose(f) != 0) ok = false;
    if (ok) rename(tmp, path);
    else remove(tmp);
}

static void nso_menu_send_host_signal(int sig)
{
    const char *s = getenv("NSO_MENU_PARENT_PID");
    if (!s || !*s) return;
    char *end = NULL;
    long pid = strtol(s, &end, 10);
    if (end && !*end && pid > 1) kill((pid_t)pid, sig);
}

static bool nso_menu_read_action(const char *dir, char *action, size_t cap, int *slot)
{
    char path[1200];
    if (!dir || !*dir || snprintf(path, sizeof(path), "%s/nso-menu-action", dir) >= (int)sizeof(path))
        return false;
    FILE *f = fopen(path, "rb");
    if (!f) return false;
    bool ok = fgets(action, (int)cap, f) != NULL;
    fclose(f);
    remove(path);
    if (ok) {
        char *arg = action + strcspn(action, " \t\r\n");
        *slot = *arg ? atoi(arg + 1) : 0;
        *arg = '\0';
    }
    return ok;
}

static void nso_menu_short_sleep(void)
{
    struct timespec ts = { .tv_sec = 0, .tv_nsec = 20000000L };
    nanosleep(&ts, NULL);
}

static double nso_menu_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* GameEntry.startKeys: type each keysym once into the game window (its own
 * launcher screen's focused "Start Game" takes Return). Short taps, so the
 * game itself never sees a held key. */
static void nso_menu_type_keys(Display *dpy, Window w, const char *keys)
{
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", keys);
    XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
    XSync(dpy, False);
    for (char *k = strtok(buf, " "); k; k = strtok(NULL, " ")) {
        KeySym sym = XStringToKeysym(k);
        KeyCode code = sym == NoSymbol ? 0 : XKeysymToKeycode(dpy, sym);
        struct timespec tap = { .tv_sec = 0, .tv_nsec = 30000000L };
        struct timespec gap = { .tv_sec = 0, .tv_nsec = 300000000L };
        if (!code) continue;
        XTestFakeKeyEvent(dpy, code, True, CurrentTime);
        XSync(dpy, False);
        nanosleep(&tap, NULL);
        XTestFakeKeyEvent(dpy, code, False, CurrentTime);
        XSync(dpy, False);
        nanosleep(&gap, NULL);
    }
}

/* Emulator windows (NSO_EMU_STATE_DIR set) have a fixed size chosen by NSO, so
 * they are centred in the host window instead of stretched; Mednafen moves and
 * resizes its own window whenever the video mode changes, so this repeats. */
static void nso_menu_place(Display *dpy, Window w, Window host)
{
    XWindowAttributes hattr, gattr;
    if (!XGetWindowAttributes(dpy, host, &hattr) || !XGetWindowAttributes(dpy, w, &gattr)) return;
    int x = (hattr.width - gattr.width) / 2, y = (hattr.height - gattr.height) / 2;
    if (gattr.x != x || gattr.y != y) XMoveWindow(dpy, w, x, y);
}

/* NSO_EMU_STATE_DIR (Mednafen: <home>/mcs, state file "state.mc<slot>"): NSO's
 * "Suspend point N" slots are that file copied to/from "<menu dir>/Suspend point
 * N.sav". The game is resumed just long enough to take Mednafen's own
 * "select state slot" and save/load keys, then stopped again. */
static void nso_menu_tap_key(Display *dpy, KeySym sym, unsigned int hold_ms)
{
    KeyCode code = XKeysymToKeycode(dpy, sym);
    struct timespec hold = { .tv_sec = hold_ms / 1000, .tv_nsec = (long)(hold_ms % 1000) * 1000000L };
    struct timespec gap = { .tv_sec = 0, .tv_nsec = 120000000L };
    if (!code) return;
    XTestFakeKeyEvent(dpy, code, True, CurrentTime);
    XSync(dpy, False);
    nanosleep(&hold, NULL);
    XTestFakeKeyEvent(dpy, code, False, CurrentTime);
    XSync(dpy, False);
    nanosleep(&gap, NULL);
}

static bool nso_menu_copy_file(const char *from, const char *to)
{
    char tmp[1300], buf[65536];
    size_t n;
    bool ok = true;
    FILE *in = fopen(from, "rb");
    if (!in) return false;
    if (snprintf(tmp, sizeof(tmp), "%s.tmp", to) >= (int)sizeof(tmp)) { fclose(in); return false; }
    FILE *out = fopen(tmp, "wb");
    if (!out) { fclose(in); return false; }
    while ((n = fread(buf, 1, sizeof(buf), in)) > 0)
        if (fwrite(buf, 1, n, out) != n) { ok = false; break; }
    ok = ok && !ferror(in);
    fclose(in);
    if (fclose(out) != 0) ok = false;
    if (ok && rename(tmp, to) != 0) ok = false;
    if (!ok) remove(tmp);
    return ok;
}

static bool nso_menu_slot_paths(const char *emu_dir, const char *menu_dir, int slot,
                                char *emu_file, size_t emu_cap, char *sav, size_t sav_cap)
{
    if (!emu_dir || !*emu_dir || !menu_dir || !*menu_dir || slot < 0 || slot > 8) return false;
    return snprintf(emu_file, emu_cap, "%s/state.mc%d", emu_dir, slot) < (int)emu_cap
        && snprintf(sav, sav_cap, "%s/Suspend point %d.sav", menu_dir, slot + 1) < (int)sav_cap;
}

static void nso_menu_game_focus(Display *dpy, Window w)
{
    XMapRaised(dpy, w);
    XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
    XSync(dpy, False);
}

/* Keep the game mapped off the host's visible area for the save. A cover
 * window (black or a snapshot) changes the End menu; XMapRaised on SIGCONT
 * is what used to flash the game over it. As a child of the host, a window
 * at a large negative position is clipped and cannot be seen even if the
 * emulator raises it — as long as we move it back when it recenters. */
static bool nso_menu_shape_ok(Display *dpy)
{
    int ev, er;
    return dpy && XShapeQueryExtension(dpy, &ev, &er);
}

/* Empty bounding shape: the window can be viewable (so it can take focus
 * and F5) but has no visible pixels, even if Mednafen recenters it. */
static void nso_menu_shape_hide(Display *dpy, Window w)
{
    if (nso_menu_shape_ok(dpy))
        XShapeCombineRectangles(dpy, w, ShapeBounding, 0, 0, NULL, 0, ShapeSet, Unsorted);
}

static void nso_menu_shape_restore(Display *dpy, Window w)
{
    if (nso_menu_shape_ok(dpy))
        XShapeCombineMask(dpy, w, ShapeBounding, 0, 0, None, ShapeSet);
}

static void nso_menu_keep_offscreen(Display *dpy, Window w)
{
    XWindowAttributes ga;
    if (!dpy || w == None) return;
    /* Eat the emulator's ConfigureNotify/MapNotify so we can hide again
     * in the same turn, not a frame later. */
    while (XPending(dpy)) {
        XEvent ev;
        XNextEvent(dpy, &ev);
    }
    if (!XGetWindowAttributes(dpy, w, &ga)) return;
    if (ga.x > -10000 || ga.y > -10000)
        XMoveWindow(dpy, w, -20000, -20000);
    nso_menu_shape_hide(dpy, w);
    XLowerWindow(dpy, w);
}

static void nso_menu_offscreen_sleep(Display *dpy, Window w, unsigned int ms)
{
    struct timespec end, now, slice = { .tv_sec = 0, .tv_nsec = 4000000L };
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        nso_menu_keep_offscreen(dpy, w);
        {
            struct timespec hold = { .tv_sec = ms / 1000, .tv_nsec = (long)(ms % 1000) * 1000000L };
            nanosleep(&hold, NULL);
        }
        nso_menu_keep_offscreen(dpy, w);
        XSync(dpy, False);
        return;
    }
    end.tv_nsec += (long)ms * 1000000L;
    while (end.tv_nsec >= 1000000000L) { end.tv_sec++; end.tv_nsec -= 1000000000L; }
    for (;;) {
        nso_menu_keep_offscreen(dpy, w);
        XSync(dpy, False);
        if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) break;
        if (now.tv_sec > end.tv_sec || (now.tv_sec == end.tv_sec && now.tv_nsec >= end.tv_nsec))
            break;
        nanosleep(&slice, NULL);
    }
}

static void nso_menu_tap_key_hidden(Display *dpy, Window game, KeySym sym, unsigned int hold_ms)
{
    KeyCode code = XKeysymToKeycode(dpy, sym);
    if (!code) return;
    nso_menu_keep_offscreen(dpy, game);
    XTestFakeKeyEvent(dpy, code, True, CurrentTime);
    XSync(dpy, False);
    nso_menu_offscreen_sleep(dpy, game, hold_ms);
    XTestFakeKeyEvent(dpy, code, False, CurrentTime);
    XSync(dpy, False);
    nso_menu_offscreen_sleep(dpy, game, 120);
}

/* Slot action while the game is paused. save: the game runs a moment, writes
 * its state, is stopped again and the file is copied to "Suspend point N".
 * load: the file is copied back and the game keeps running (the host does not
 * send "resume" after a load). Returns true if the game was left running. */
static bool nso_menu_emu_state(Display *dpy, Window w, pid_t game, bool save, int slot,
                               const char *emu_dir, const char *menu_dir, Window host)
{
    char emu_file[1200], sav[1260];
    struct timespec settle = { .tv_sec = 0, .tv_nsec = 200000000L };
    int ox = 0, oy = 0, ow = 0, oh = 0;
    bool have_geom = false;
    XWindowAttributes ga;
    (void)host;
    if (!dpy || w == None || !nso_menu_slot_paths(emu_dir, menu_dir, slot, emu_file,
                                                  sizeof(emu_file), sav, sizeof(sav)))
        return false;
    if (save) remove(emu_file);
    else if (!nso_menu_copy_file(sav, emu_file)) return false;
    if (save) {
        if (XGetWindowAttributes(dpy, w, &ga)) {
            ox = ga.x; oy = ga.y; ow = ga.width; oh = ga.height;
            have_geom = (ow > 0 && oh > 0);
        }
        /* Parked position first, then map — never XMapRaised, never on-screen.
         * An empty bounding shape hides any frame if the emulator recenters. */
        XSelectInput(dpy, w, StructureNotifyMask);
        XMoveWindow(dpy, w, -20000, -20000);
        nso_menu_shape_hide(dpy, w);
        XMapWindow(dpy, w);
        XLowerWindow(dpy, w);
        XSetInputFocus(dpy, w, RevertToParent, CurrentTime);
        XSync(dpy, False);
    } else {
        nso_menu_game_focus(dpy, w);
    }
    kill(-game, SIGCONT);
    if (save) nso_menu_offscreen_sleep(dpy, w, 200);
    else nanosleep(&settle, NULL);
    if (save) {
        nso_menu_tap_key_hidden(dpy, w, (KeySym)(XK_0 + slot), 40);
        nso_menu_tap_key_hidden(dpy, w, XK_F5, 200);
    } else {
        nso_menu_tap_key(dpy, (KeySym)(XK_0 + slot), 40);
        nso_menu_tap_key(dpy, XK_F7, 200);
        return true;
    }
    off_t last = -1;
    for (int i = 0; i < 150; ++i) {
        struct stat st;
        off_t size = stat(emu_file, &st) == 0 ? st.st_size : -1;
        if (size > 0 && size == last) break;
        last = size;
        nso_menu_offscreen_sleep(dpy, w, 60);
    }
    kill(-game, SIGSTOP);
    {
        int st = 0;
        pid_t ended;
        do ended = waitpid(game, &st, WUNTRACED); while (ended < 0 && errno == EINTR);
        (void)ended;
    }
    nso_menu_keep_offscreen(dpy, w);
    XUnmapWindow(dpy, w);
    nso_menu_shape_restore(dpy, w);
    if (have_geom) {
        XMoveResizeWindow(dpy, w, ox, oy, (unsigned)ow, (unsigned)oh);
    }
    XSelectInput(dpy, w, KeyPressMask | KeyReleaseMask | StructureNotifyMask);
    XSync(dpy, False);
    if (last > 0) nso_menu_copy_file(emu_file, sav);
    return false;
}

/* A game with the CRT Geometry hook (the tools nsocrt patches) draws its live picture through the shader, but
 * NSO filters the frozen frame it captures itself: before the game is stopped it is asked (a flag file,
 * NSO_CRTGEOM_PLAIN) to show plain frames, and answers with "<flag>.ack" once two are on screen. */
static void nso_menu_plain_request(void)
{
    const char *flag = getenv("NSO_CRTGEOM_PLAIN");
    char ack[1300];
    if (!flag || !*flag || snprintf(ack, sizeof(ack), "%s.ack", flag) >= (int)sizeof(ack)) return;
    remove(ack);
    FILE *f = fopen(flag, "w");
    if (!f) return;
    fclose(f);
    for (int i = 0; i < 100; ++i) {   /* up to ~2 s: a game that never answers is stopped anyway */
        struct stat st;
        if (stat(ack, &st) == 0) break;
        nso_menu_short_sleep();
    }
}

static void nso_menu_plain_release(void)
{
    const char *flag = getenv("NSO_CRTGEOM_PLAIN");
    char ack[1300];
    if (!flag || !*flag || snprintf(ack, sizeof(ack), "%s.ack", flag) >= (int)sizeof(ack)) return;
    remove(flag);
    remove(ack);
}

static bool nso_menu_process_group_alive(pid_t group)
{
    if (kill(-group, 0) == 0) return true;
    return errno == EPERM;
}

static void nso_menu_end_game(pid_t game, int *status)
{
    kill(-game, SIGCONT);
    kill(-game, SIGTERM);
    for (int i = 0; i < 50 && waitpid(game, status, WNOHANG) == 0; ++i)
        nso_menu_short_sleep();
    kill(-game, SIGKILL);
    waitpid(game, status, WNOHANG);
}

/* CapsLock=Lock, NumLock=Mod2, ScrollLock/Level3=Mod5 on Jefferson's map. */
static void nso_menu_grab_locks(Display *dpy, KeyCode code, Window w)
{
    static const unsigned locks[] = {
        0,
        LockMask,
        Mod2Mask,
        Mod5Mask,
        LockMask | Mod2Mask,
        LockMask | Mod5Mask,
        Mod2Mask | Mod5Mask,
        LockMask | Mod2Mask | Mod5Mask,
    };
    if (!dpy || !code || w == None) return;
    for (size_t i = 0; i < sizeof(locks) / sizeof(locks[0]); i++)
        XGrabKey(dpy, code, locks[i], w, False, GrabModeAsync, GrabModeAsync);
}

int NsoMenuLauncherMain(int argc, char **argv)
{
    const char *state_dir = getenv("NSO_MENU_DIR");
    unsigned long parent_id = nso_menu_env_window("NSO_PARENT_WINDOW_ID");
    const char *host_pid_text = getenv("NSO_MENU_PARENT_PID");
    if (argc < 1 || !argv[0] || !*argv[0] || !parent_id || !host_pid_text) return 2;
    char start_keys[256] = { 0 };
    if (getenv("NSO_START_KEYS")) snprintf(start_keys, sizeof(start_keys), "%s", getenv("NSO_START_KEYS"));
    unsetenv("NSO_START_KEYS");

    struct sigaction sa = { 0 };
    sa.sa_handler = nso_menu_request_signal;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGUSR1, &sa, NULL);
    sa.sa_handler = nso_menu_park_signal;
    sigaction(SIGUSR2, &sa, NULL);
    struct sigaction st = { 0 };
    st.sa_handler = nso_menu_stop_signal;
    sigemptyset(&st.sa_mask);
    sigaction(SIGTERM, &st, NULL);
    sigaction(SIGHUP, &st, NULL);
    pid_t host = (pid_t)strtol(host_pid_text, NULL, 10);
    const char *emu_dir = getenv("NSO_EMU_STATE_DIR");
    bool centre = emu_dir && *emu_dir;

    Display *dpy = XOpenDisplay(NULL);   /* before the fork: no window is missed */
    pid_t game = fork();
    if (game < 0) return 2;
    if (game == 0) {
        setpgid(0, 0);
        signal(SIGTERM, SIG_DFL);
        signal(SIGHUP, SIG_DFL);
        execv(argv[0], argv);
        _exit(127);
    }
    setpgid(game, game);

    Window game_window = None;
    KeyCode end_code = 0, home_code = 0;
    if (dpy) {
        Window root = DefaultRootWindow(dpy);
        game_window = nso_menu_wait_window(dpy, root, game);
        if (game_window != None) {
            XWindowAttributes host;
            if (XGetWindowAttributes(dpy, (Window)parent_id, &host)) {
                XReparentWindow(dpy, game_window, (Window)parent_id, 0, 0);
                if (centre) nso_menu_place(dpy, game_window, (Window)parent_id);
                else XMoveResizeWindow(dpy, game_window, 0, 0,
                                       (unsigned int)host.width, (unsigned int)host.height);
                XSelectInput(dpy, game_window, KeyPressMask | KeyReleaseMask | StructureNotifyMask);
                end_code = XKeysymToKeycode(dpy, XK_End);
                home_code = XKeysymToKeycode(dpy, XK_Home);
                /* AnyModifier can miss keys when Caps/NumLock are on.
                 * Grab every lock-modifier combination (0, Lock, Mod2=NumLock,
                 * Mod5, and their unions), same keycodes as before 0f2efa5. */
                nso_menu_grab_locks(dpy, end_code, game_window);
                nso_menu_grab_locks(dpy, home_code, game_window);
                XMapRaised(dpy, game_window);
                XSetInputFocus(dpy, game_window, RevertToParent, CurrentTime);
                XSync(dpy, False);
                nso_menu_write_window(state_dir, game_window);
            }
        }
    }

    /* The port's launcher screen needs a few seconds before it takes input. */
    double keys_at = start_keys[0] && dpy && game_window != None ? nso_menu_now() + 5.0 : 0.0;
    bool paused = false;
    bool leader_exited = false;
    int status = 0;
    for (;;) {
        pid_t ended = 0;
        if (!leader_exited) {
            ended = waitpid(game, &status, WNOHANG);
            if (ended == game || (ended < 0 && errno == ECHILD)) leader_exited = true;
        }
        if (leader_exited && !nso_menu_process_group_alive(game)) break;

        /* NSO closed or killed (no orphaned game windows left behind). */
        if (nso_menu_stop || (host > 1 && kill(host, 0) != 0 && errno == ESRCH)) {
            nso_menu_end_game(game, &status);
            if (dpy) XCloseDisplay(dpy);
            return 0;
        }

        if (dpy && game_window != None) {
            while (XPending(dpy)) {
                XEvent event;
                XNextEvent(dpy, &event);
                /* End: same keycode test as before 0f2efa5 (XLookupKeysym can
                 * mis-identify grabbed keys and swap End/Home). */
                if (event.type == KeyPress && end_code && event.xkey.keycode == end_code)
                    nso_menu_requested = 1;
                else if (event.type == KeyPress && home_code && event.xkey.keycode == home_code
                         && !nso_menu_home_held) {
                    nso_menu_home_held = 1;
                    nso_menu_home = 1;
                } else if (event.type == KeyRelease && home_code && event.xkey.keycode == home_code)
                    nso_menu_home_held = 0;
                if (centre && event.type == ConfigureNotify && event.xconfigure.window == game_window)
                    nso_menu_place(dpy, game_window, (Window)parent_id);
            }
        }

        if (keys_at > 0.0 && !paused && nso_menu_now() >= keys_at) {
            keys_at = 0.0;
            nso_menu_type_keys(dpy, game_window, start_keys);
        }

        if (nso_menu_home) {
            nso_menu_home = 0;
            nso_menu_send_host_signal(SIGUSR2);
        }

        /* End: identical to pre-0f2efa5 — stop the game and SIGUSR1 the host. */
        if (nso_menu_requested && !paused) {
            nso_menu_requested = 0;
            nso_menu_plain_request();
            if (kill(-game, SIGSTOP) != 0) continue;
            do ended = waitpid(game, &status, WUNTRACED); while (ended < 0 && errno == EINTR);
            if ((ended == game && !WIFSTOPPED(status))
                || (ended < 0 && !nso_menu_process_group_alive(game))) break;
            paused = true;
            nso_menu_send_host_signal(SIGUSR1);
        }

        /* Home park only: stop the game, do not open the End menu.
         * If End already stopped the game, drop the flag so a later resume
         * does not immediately SIGSTOP again (that froze re-entry). */
        if (nso_menu_park) {
            if (paused) {
                nso_menu_park = 0;
            } else {
                nso_menu_park = 0;
                nso_menu_plain_request();
                if (kill(-game, SIGSTOP) != 0) continue;
                do ended = waitpid(game, &status, WUNTRACED); while (ended < 0 && errno == EINTR);
                if ((ended == game && !WIFSTOPPED(status))
                    || (ended < 0 && !nso_menu_process_group_alive(game))) break;
                paused = true;
            }
        }

        if (paused) {
            char action[64];
            int slot = 0;
            if (nso_menu_read_action(state_dir, action, sizeof(action), &slot)) {
                if (strcmp(action, "save") == 0 || strcmp(action, "load") == 0) {
                    if (nso_menu_emu_state(dpy, game_window, game, action[0] == 's', slot,
                                           emu_dir, state_dir, (Window)parent_id))
                        paused = false;
                } else if (strcmp(action, "resume") == 0) {
                    nso_menu_plain_release();
                    kill(-game, SIGCONT);
                    paused = false;
                    if (dpy && game_window != None) {
                        XMapRaised(dpy, game_window);
                        XSetInputFocus(dpy, game_window, RevertToParent, CurrentTime);
                        XSync(dpy, False);
                    }
                } else if (strcmp(action, "restart") == 0) {
                    nso_menu_end_game(game, &status);
                    if (dpy) XCloseDisplay(dpy);
                    return 0;
                } else if (strcmp(action, "quit") == 0) {
                    nso_menu_end_game(game, &status);
                    if (dpy) XCloseDisplay(dpy);
                    return 0;
                }
            }
        }
        nso_menu_short_sleep();
    }

    if (dpy) XCloseDisplay(dpy);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

#else
#include "menu_launcher.h"
int NsoMenuLauncherMain(int argc, char **argv) { (void)argc; (void)argv; return 2; }
#endif
