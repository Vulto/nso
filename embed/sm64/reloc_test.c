/* reloc_test.c - Sm64_Relocate (reloc.h). */
#include <assert.h>
#include <stdio.h>
#include "reloc.h"

static uint64_t at(const uint8_t *p, size_t k) { uint64_t v; memcpy(&v, p + k, 8); return v; }
static void put(uint8_t *p, size_t k, uint64_t v) { memcpy(p + k, &v, 8); }

int main(void)
{
    const uint64_t lo[2] = { 0x7f0000000000, 0x7f0010000000 }, size[2] = { 0x1000, 0x100 };
    const uint64_t d[2] = { 0x500000, (uint64_t)-0x1000 };
    uint8_t b[64] = { 0 };
    put(b, 0, lo[0] + 0x10);          /* aligned, game */
    put(b, 12, lo[0] + 0x1000);       /* 4-aligned, one past the end */
    put(b, 24, lo[1] + 8);            /* mod */
    put(b, 32, 0x00007f0000000000 - 1);  /* just below: not a pointer */
    put(b, 40, 1234);
    Sm64_Relocate(b, sizeof b, lo, size, d);
    assert(at(b, 0) == lo[0] + 0x10 + d[0]);
    assert(at(b, 12) == lo[0] + 0x1000 + d[0]);
    assert(at(b, 24) == lo[1] + 8 + d[1]);
    assert(at(b, 32) == 0x00007f0000000000 - 1);
    assert(at(b, 40) == 1234);
    const uint64_t none[2] = { 0, 0 };
    Sm64_Relocate(b, sizeof b, lo, size, none);   /* same addresses: untouched */
    assert(at(b, 0) == lo[0] + 0x10 + d[0]);
    puts("sm64reloc_test: ok");
    return 0;
}
