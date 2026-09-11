#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define WORDS 12u

static bool
words_equal(const bitmap_word_t *a, const bitmap_word_t *b, size_t n)
{
        for (size_t i = 0u; i < n; i++) {
                if (a[i] != b[i]) {
                        return false;
                }
        }
        return true;
}

static bool
words_all(const bitmap_word_t *w, size_t n, bitmap_word_t value)
{
        for (size_t i = 0u; i < n; i++) {
                if (w[i] != value) {
                        return false;
                }
        }
        return true;
}

static void
fill(bitmap_word_t *w, size_t n, bitmap_word_t value)
{
        for (size_t i = 0u; i < n; i++) {
                w[i] = value;
        }
}

void
test_init(void)
{
        bitmap_word_t storage[WORDS];
        bitmap_word_t sentinel[WORDS];
        bitmap_word_t expected[WORDS];
        bitmap_t map;

        bm_suite("init");

        /* Null descriptor. */
        BM_CHECK(!bitmap_init(NULL, storage, WORDS, 17u));

        /* A decoy descriptor must survive a failed init untouched. */
        fill(sentinel, WORDS, (bitmap_word_t)0x1234u);
        fill(expected, WORDS, (bitmap_word_t)0x1234u);
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(!bitmap_init(&map, NULL, 0u, 17u));
        BM_CHECK(map.words == sentinel);
        BM_CHECK(map.bit_count == 5u);
        BM_CHECK(words_equal(sentinel, expected, WORDS));

        /* Null storage cannot declare a non-zero word capacity. */
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(!bitmap_init(&map, NULL, 1u, 0u));
        BM_CHECK(map.words == sentinel);
        BM_CHECK(map.bit_count == 5u);

        /* Undersized storage: descriptor and storage unchanged. */
        fill(storage, WORDS, (bitmap_word_t)0xABCDu);
        fill(expected, WORDS, (bitmap_word_t)0xABCDu);
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(!bitmap_init(&map, storage, 1u, 17u));
        BM_CHECK(map.words == sentinel);
        BM_CHECK(map.bit_count == 5u);
        BM_CHECK(words_equal(storage, expected, WORDS));

        /* Unrepresentable capacity with a small array: rejected. */
        fill(storage, WORDS, (bitmap_word_t)0xABCDu);
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(!bitmap_init(&map, storage, WORDS, SIZE_MAX));
        BM_CHECK(map.words == sentinel);
        BM_CHECK(map.bit_count == 5u);
        BM_CHECK(words_equal(storage, expected, WORDS));

        /* Zero bits, no storage: canonical empty descriptor. */
        fill(expected, WORDS, (bitmap_word_t)0x1234u);
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(bitmap_init(&map, NULL, 0u, 0u));
        BM_CHECK(map.words == NULL);
        BM_CHECK(map.bit_count == (size_t)0);
        BM_CHECK(words_equal(sentinel, expected, WORDS));

        /* Zero bits with real storage: accepted, ignored, untouched. */
        fill(storage, WORDS, (bitmap_word_t)0xABCDu);
        fill(expected, WORDS, (bitmap_word_t)0xABCDu);
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(bitmap_init(&map, storage, WORDS, 0u));
        BM_CHECK(map.words == NULL);
        BM_CHECK(map.bit_count == (size_t)0);
        BM_CHECK(words_equal(storage, expected, WORDS));

        /* A real pointer with a declared zero-word capacity is also ignored. */
        map.words = sentinel;
        map.bit_count = 5u;
        BM_CHECK(bitmap_init(&map, storage, 0u, 0u));
        BM_CHECK(map.words == NULL);
        BM_CHECK(map.bit_count == (size_t)0);
        BM_CHECK(words_equal(storage, expected, WORDS));

        /* Success clears exactly the required words and no more. */
        fill(storage, WORDS, (bitmap_word_t)0xABCDu);
        fill(expected, WORDS, (bitmap_word_t)0xABCDu);
        expected[0] = (bitmap_word_t)0u;
        expected[1] = (bitmap_word_t)0u;
        BM_CHECK(bitmap_init(&map, storage, WORDS, 17u));
        BM_CHECK(map.words == storage);
        BM_CHECK(map.bit_count == 17u);
        BM_CHECK(words_equal(storage, expected, WORDS));

        /* Exact-fit storage is accepted. */
        fill(storage, WORDS, (bitmap_word_t)0xFFFFu);
        BM_CHECK(bitmap_init(&map, storage, 2u, 17u));
        BM_CHECK(storage[0] == (bitmap_word_t)0u);
        BM_CHECK(storage[1] == (bitmap_word_t)0u);
        BM_CHECK(storage[2] == (bitmap_word_t)0xFFFFu);

        /* Reinitialization clears the words again. */
        BM_CHECK(bitmap_set(&map, 0u));
        BM_CHECK(bitmap_set(&map, 16u));
        BM_CHECK(bitmap_init(&map, storage, WORDS, 17u));
        BM_CHECK(storage[0] == (bitmap_word_t)0u);
        BM_CHECK(storage[1] == (bitmap_word_t)0u);

        /* Full-capacity init clears every required word. */
        fill(storage, WORDS, (bitmap_word_t)0xFFFFu);
        BM_CHECK(bitmap_init(&map, storage, WORDS, WORDS * 16u));
        BM_CHECK(words_all(storage, WORDS, (bitmap_word_t)0u));

        /* One bit needs one word. */
        fill(storage, WORDS, (bitmap_word_t)0x5555u);
        BM_CHECK(bitmap_init(&map, storage, WORDS, 1u));
        BM_CHECK(storage[0] == (bitmap_word_t)0u);
        BM_CHECK(storage[1] == (bitmap_word_t)0x5555u);
}
