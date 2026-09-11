/*
 * Integration peer translation unit. Uses a capacity different from the
 * consumer's, so linking both against one bitmap library build demonstrates
 * that the library is capacity-independent.
 */

#include <bitmap.h>

#include <stdbool.h>
#include <stddef.h>

#ifndef BITMAP_PEER_BITS
#define BITMAP_PEER_BITS 192
#endif

int
bitmap_peer_run(void)
{
        static bitmap_word_t storage[BITMAP_STORAGE_WORDS(BITMAP_PEER_BITS)];
        bitmap_t map;
        bool any = false;
        size_t count = 99u;
        size_t at = 99u;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage),
                         (size_t)BITMAP_PEER_BITS)) {
                return 1;
        }
        if (!bitmap_fill_all(&map)) {
                return 1;
        }
        if (!bitmap_any(&map, &any)) {
                return 1;
        }
        if (any != (BITMAP_PEER_BITS != 0)) {
                return 1;
        }
        if (!bitmap_count(&map, &count)) {
                return 1;
        }
        if (count != (size_t)BITMAP_PEER_BITS) {
                return 1;
        }
        if (!bitmap_scan(&map, 0u, &at)) {
                return 1;
        }
        if (at != 0u) {
                return 1;
        }
        return 0;
}
