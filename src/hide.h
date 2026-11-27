/* hide.h - "Hide unplayable": the games grid shows only the launchable titles.
 * Pure C, no raylib: unit-tested in hide_test.c. The games array keeps every
 * title; the playable ones are moved to the front (keeping their order) and
 * only that prefix is shown, so covers past it are never loaded. The switch is
 * saved per system as "<id>.hide_unplayable" (1 / 0) in config/nso.cfg. */
#ifndef NSO_HIDE_H
#define NSO_HIDE_H

/* Standalone pure-C tests: capacity fallback. main.c defines the real
 * MAX_GAMES from the catalog sizes before including this header. */
#ifndef MAX_GAMES
#define MAX_GAMES 4096
#endif


#include <stdbool.h>

/* perm[k] = index (into the games array) of the title placed k-th. `order`
 * is the order the user wants (perm of a sort mode / custom order; NULL =
 * 0..count-1) and playable[i] says whether title i can launch. Playable titles
 * come first, each group in `order`. Returns how many are playable (shown). */
int Hide_Perm(const bool *playable, const int *order, int count, int *perm);
/* Cursor after the list shrank to `shown` titles: kept when it still fits,
 * else the last one (0 when the list is empty). */
int Hide_Clamp(int idx, int shown);
/* Saved custom order while some titles are hidden. titles[0..shown) is the new
 * order of the visible ones, titles[shown..count) the hidden ones. out[k] gets
 * the full order: each visible title takes a slot of the old order (oldText,
 * file format of order.h) and the hidden ones keep theirs. */
void Hide_MergeOrder(const char *oldText, const char *const *titles, int count, int shown,
                     const char **out);
/* config value of the switch: "1" / "0"; parse treats anything but "1" as off. */
const char *Hide_CfgValue(bool on);
bool Hide_CfgParse(const char *value);

#endif /* NSO_HIDE_H */

#ifdef HIDE_IMPLEMENTATION
#ifndef HIDE_IMPLEMENTATION_ONCE
#define HIDE_IMPLEMENTATION_ONCE

#include <string.h>
#include "order.h"

int Hide_Perm(const bool *playable, const int *order, int count, int *perm)
{
    int shown = 0, k = 0;
    if (!perm || count <= 0) return 0;
    for (int pass = 0; pass < 2; pass++) {
        for (int i = 0; i < count; i++) {
            int t = order ? order[i] : i;
            bool ok = playable && playable[t];
            if (ok == (pass == 0)) {
                perm[k++] = t;
                if (pass == 0) shown++;
            }
        }
    }
    return shown;
}

int Hide_Clamp(int idx, int shown)
{
    if (shown <= 0 || idx < 0) return 0;
    return idx >= shown ? shown - 1 : idx;
}

void Hide_MergeOrder(const char *oldText, const char *const *titles, int count, int shown,
                     const char **out)
{
    int perm[MAX_GAMES], next = 0;
    if (!out || !titles || count <= 0) return;
    if (count > MAX_GAMES) count = MAX_GAMES;
    Order_Perm(oldText ? oldText : "", titles, count, perm);
    for (int k = 0; k < count; k++) {
        int t = perm[k];
        out[k] = t < shown ? titles[next++] : titles[t];
    }
}

const char *Hide_CfgValue(bool on) { return on ? "1" : "0"; }

bool Hide_CfgParse(const char *value) { return value && strcmp(value, "1") == 0; }

#endif /* HIDE_IMPLEMENTATION_ONCE */
#endif /* HIDE_IMPLEMENTATION */
