#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
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

static void nso_menu_request_signal(int sig)
{
    (void)sig;
    nso_menu_requested = 1;
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

static Window nso_menu_find_window(Display *dpy, Window root, pid_t group, int depth)
{
    Window parent, *children = NULL;
    unsigned int count = 0;
    Window found = None;
    if (depth > 8 || !XQueryTree(dpy, root, &root, &parent, &children, &count)) return None;
    for (unsigned int i = 0; i < count && found == None; ++i) {
        if (nso_menu_window_pid(dpy, children[i], group)) found = children[i];
        else found = nso_menu_find_window(dpy, children[i], group, depth + 1);
    }
    if (children) XFree(children);
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

static void nso_menu_send_host_signal(void)
{
    const char *s = getenv("NSO_MENU_PARENT_PID");
    if (!s || !*s) return;
    char *end = NULL;
    long pid = strtol(s, &end, 10);
    if (end && !*end && pid > 1) kill((pid_t)pid, SIGUSR1);
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

/* Slot action while the game is paused. save: the game runs a moment, writes
 * its state, is stopped again and the file is copied to "Suspend point N".
 * load: the file is copied back and the game keeps running (the host does not
 * send "resume" after a load). Returns true if the game was left running. */
static bool nso_menu_emu_state(Display *dpy, Window w, pid_t game, bool save, int slot,
                               const char *emu_dir, const char *menu_dir)
{
    char emu_file[1200], sav[1260];
    struct timespec settle = { .tv_sec = 0, .tv_nsec = 200000000L };
    if (!dpy || w == None || !nso_menu_slot_paths(emu_dir, menu_dir, slot, emu_file,
                                                  sizeof(emu_file), sav, sizeof(sav)))
        return false;
    if (save) remove(emu_file);
    else if (!nso_menu_copy_file(sav, emu_file)) return false;
    nso_menu_game_focus(dpy, w);
    kill(-game, SIGCONT);
    nanosleep(&settle, NULL);
    nso_menu_tap_key(dpy, (KeySym)(XK_0 + slot), 40);
    if (!save) {
        nso_menu_tap_key(dpy, XK_F7, 200);
        return true;
    }
    nso_menu_tap_key(dpy, XK_F5, 200);
    off_t last = -1;
    for (int i = 0; i < 150; ++i) {
        struct stat st;
        off_t size = stat(emu_file, &st) == 0 ? st.st_size : -1;
        if (size > 0 && size == last) break;
        last = size;
        nso_menu_short_sleep();
        nso_menu_short_sleep();
        nso_menu_short_sleep();
    }
    kill(-game, SIGSTOP);
    XUnmapWindow(dpy, w);
    XSync(dpy, False);
    if (last > 0) nso_menu_copy_file(emu_file, sav);
    return false;
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
    struct sigaction st = { 0 };
    st.sa_handler = nso_menu_stop_signal;
    sigemptyset(&st.sa_mask);
    sigaction(SIGTERM, &st, NULL);
    sigaction(SIGHUP, &st, NULL);
    pid_t host = (pid_t)strtol(host_pid_text, NULL, 10);
    const char *emu_dir = getenv("NSO_EMU_STATE_DIR");
    bool centre = emu_dir && *emu_dir;

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

    Display *dpy = XOpenDisplay(NULL);
    Window game_window = None;
    if (dpy) {
        Window root = DefaultRootWindow(dpy);
        for (int i = 0; i < 500 && game_window == None; ++i) {
            game_window = nso_menu_find_window(dpy, root, game, 0);
            if (game_window == None) nso_menu_short_sleep();
        }
        if (game_window != None) {
            XWindowAttributes host;
            if (XGetWindowAttributes(dpy, (Window)parent_id, &host)) {
                XReparentWindow(dpy, game_window, (Window)parent_id, 0, 0);
                if (centre) nso_menu_place(dpy, game_window, (Window)parent_id);
                else XMoveResizeWindow(dpy, game_window, 0, 0,
                                       (unsigned int)host.width, (unsigned int)host.height);
                XSelectInput(dpy, game_window, KeyPressMask | StructureNotifyMask);
                int keycode = XKeysymToKeycode(dpy, XK_End);
                XGrabKey(dpy, keycode, AnyModifier, game_window, False,
                         GrabModeAsync, GrabModeAsync);
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
                if (event.type == KeyPress && event.xkey.keycode == XKeysymToKeycode(dpy, XK_End))
                    nso_menu_requested = 1;
                if (centre && event.type == ConfigureNotify && event.xconfigure.window == game_window)
                    nso_menu_place(dpy, game_window, (Window)parent_id);
            }
        }

        if (keys_at > 0.0 && !paused && nso_menu_now() >= keys_at) {
            keys_at = 0.0;
            nso_menu_type_keys(dpy, game_window, start_keys);
        }

        if (nso_menu_requested && !paused) {
            nso_menu_requested = 0;
            if (kill(-game, SIGSTOP) != 0) continue;
            do ended = waitpid(game, &status, WUNTRACED); while (ended < 0 && errno == EINTR);
            if ((ended == game && !WIFSTOPPED(status))
                || (ended < 0 && !nso_menu_process_group_alive(game))) break;
            paused = true;
            nso_menu_send_host_signal();
        }

        if (paused) {
            char action[64];
            int slot = 0;
            if (nso_menu_read_action(state_dir, action, sizeof(action), &slot)) {
                if (strcmp(action, "save") == 0 || strcmp(action, "load") == 0) {
                    if (nso_menu_emu_state(dpy, game_window, game, action[0] == 's', slot,
                                           emu_dir, state_dir))
                        paused = false;
                } else if (strcmp(action, "resume") == 0) {
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
