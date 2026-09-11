/*
 * Integration consumer. Compiled once per capacity against a single bitmap
 * library build, and linked with peer.c, which uses a different capacity.
 * A zero exit status means every documented operation behaved.
 */

#include <bitmap.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#ifndef BITMAP_TEST_BITS
#define BITMAP_TEST_BITS 37
#endif

int bitmap_peer_run(void);

static int
fail(const char *what)
{
        (void)fprintf(stderr, "consumer(bits=%d): %s failed\n",
                      BITMAP_TEST_BITS, what);
        return 1;
}

int
main(void)
{
        static bitmap_word_t storage[BITMAP_STORAGE_WORDS(BITMAP_TEST_BITS)];
        bitmap_t map;
        bool value = false;
        size_t count = 99u;
        size_t at = 99u;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage),
                         (size_t)BITMAP_TEST_BITS)) {
                return fail("init");
        }
        if (!bitmap_clear_all(&map)) {
                return fail("clear_all");
        }
        if (!bitmap_any(&map, &value) || value) {
                return fail("any empty");
        }
        if (!bitmap_count(&map, &count) || (count != 0u)) {
                return fail("count empty");
        }
        if (!bitmap_scan(&map, 0u, &at) || (at != (size_t)BITMAP_TEST_BITS)) {
                return fail("scan empty");
        }

        if (BITMAP_TEST_BITS > 0) {
                const size_t last = (size_t)BITMAP_TEST_BITS - 1u;
                const size_t expected = (BITMAP_TEST_BITS == 1) ? 1u : 2u;

                if (!bitmap_set(&map, 0u) || !bitmap_set(&map, last)) {
                        return fail("set");
                }
                if (!bitmap_count(&map, &count) || (count != expected)) {
                        return fail("count");
                }
                if (!bitmap_test(&map, 0u, &value) || !value) {
                        return fail("test bit 0");
                }
                if (!bitmap_test(&map, last, &value) || !value) {
                        return fail("test last bit");
                }
                if (!bitmap_scan(&map, 0u, &at) || (at != 0u)) {
                        return fail("scan first");
                }
                if (!bitmap_scan(&map, last, &at) || (at != last)) {
                        return fail("scan last");
                }
                if (!bitmap_test_and_clear(&map, last, &value) || !value) {
                        return fail("test_and_clear");
                }
        }

        if (bitmap_peer_run() != 0) {
                return fail("peer");
        }

        (void)printf("consumer bits=%d ok\n", BITMAP_TEST_BITS);
        return 0;
}
