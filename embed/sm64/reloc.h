/* reloc.h - suspend point pointer fix-up (sm64mod.c), pure so reloc_test.c
 * can check it without the port. */
#ifndef SM64_RELOC_H
#define SM64_RELOC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Moves every pointer in p[0..n) into [lo[i], lo[i] + size[i]] (one past the
 * end included) by d[i]. The game's pools hand out 4-aligned blocks, so a
 * pointer may sit on any 4-byte boundary; p itself is 4-aligned. */
static inline void Sm64_Relocate(uint8_t *p, size_t n, const uint64_t lo[2], const uint64_t size[2],
                                 const uint64_t d[2])
{
    if (!d[0] && !d[1]) return;
    for (size_t k = 0; k + 8 <= n;) {
        uint64_t v;
        memcpy(&v, p + k, 8);
        int i = v - lo[0] <= size[0] ? 0 : v - lo[1] <= size[1] ? 1 : -1;
        if (i < 0) { k += 4; continue; }
        v += d[i];
        memcpy(p + k, &v, 8);
        k += 8;
    }
}

#endif
