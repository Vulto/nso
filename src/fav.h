/* fav.h - per-system favorite titles (pure C, no raylib).
 * The list is saved as config/favorites-<id>.txt, one game title per line
 * (same format as order.h). The Favorites filter switch is "<id>.favorites"
 * (1 / 0) in config/nso.cfg. Unit-tested in fav_test.c. */
#ifndef NSO_FAV_H
#define NSO_FAV_H

#ifndef MAX_GAMES
#define MAX_GAMES 4096
#endif

#include <stdbool.h>
#include <stddef.h>

/* True when `title` is listed in the favorites file text. */
bool Fav_Has(const char *text, const char *title);
/* fav[i] is set from `text` for titles[0..count). Returns how many are on. */
int Fav_Marks(const char *text, const char *const *titles, int count, bool *fav);
/* Rewrite `text` with `title` added (`want` true) or removed. Other lines,
 * including titles not in the current catalog, are kept. Returns 0, or -1
 * if the result did not fit. */
int Fav_Write(const char *text, const char *title, bool want, char *out, size_t n);
/* Toggle `title` in `text` into `out`. *on is the new state (unchanged if
 * the write does not fit). Returns 0, or -1 on overflow. */
int Fav_Toggle(const char *text, const char *title, char *out, size_t n, bool *on);
/* config value of the Favorites filter: "1" / "0"; parse treats anything but "1" as off. */
const char *Fav_CfgValue(bool on);
bool Fav_CfgParse(const char *value);

#endif /* NSO_FAV_H */

#ifdef FAV_IMPLEMENTATION
#ifndef FAV_IMPLEMENTATION_ONCE
#define FAV_IMPLEMENTATION_ONCE

#include <stdio.h>
#include <string.h>

static size_t Fav_LineLen(const char *p, const char **next)
{
    const char *e = strchr(p, '\n');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    *next = e ? e + 1 : NULL;
    while (len && p[len - 1] == '\r') len--;
    return len;
}

static bool Fav_TitleEq(const char *p, size_t len, const char *title)
{
    return title && strlen(title) == len && (len == 0 || strncmp(p, title, len) == 0);
}

bool Fav_Has(const char *text, const char *title)
{
    if (!text || !title || !title[0]) return false;
    for (const char *p = text; p && *p; ) {
        const char *next;
        size_t len = Fav_LineLen(p, &next);
        if (Fav_TitleEq(p, len, title)) return true;
        p = next;
    }
    return false;
}

int Fav_Marks(const char *text, const char *const *titles, int count, bool *fav)
{
    int n = 0;
    if (!fav || count <= 0) return 0;
    if (count > MAX_GAMES) count = MAX_GAMES;
    for (int i = 0; i < count; i++) {
        fav[i] = Fav_Has(text, titles ? titles[i] : NULL);
        if (fav[i]) n++;
    }
    return n;
}

int Fav_Write(const char *text, const char *title, bool want, char *out, size_t n)
{
    size_t used = 0;
    bool written = false;
    const char *p = text ? text : "";
    if (!out || n == 0) return -1;
    out[0] = '\0';
    if (!title || !title[0]) return 0;
    while (p && *p) {
        const char *next;
        size_t len = Fav_LineLen(p, &next);
        if (Fav_TitleEq(p, len, title)) {
            if (want && !written) {
                int w = snprintf(out + used, n - used, "%s\n", title);
                if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; return -1; }
                used += (size_t)w;
                written = true;
            }
        } else if (len) {
            int w = snprintf(out + used, n - used, "%.*s\n", (int)len, p);
            if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; return -1; }
            used += (size_t)w;
        }
        p = next;
    }
    if (want && !written) {
        int w = snprintf(out + used, n - used, "%s\n", title);
        if (w < 0 || (size_t)w >= n - used) { out[0] = '\0'; return -1; }
    }
    return 0;
}

int Fav_Toggle(const char *text, const char *title, char *out, size_t n, bool *on)
{
    bool want = !Fav_Has(text, title);
    if (Fav_Write(text, title, want, out, n) != 0) return -1;
    if (on) *on = want;
    return 0;
}

const char *Fav_CfgValue(bool on) { return on ? "1" : "0"; }

bool Fav_CfgParse(const char *value) { return value && strcmp(value, "1") == 0; }

#endif /* FAV_IMPLEMENTATION_ONCE */
#endif /* FAV_IMPLEMENTATION */
