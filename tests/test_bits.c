#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define WORDS 12u

static const size_t CAPACITIES[] = {1u,  15u, 16u, 17u, 23u,
                                    31u, 32u, 33u, 192u};

static void
fill(bitmap_word_t *w, size_t n, bitmap_word_t value)
{
        for (size_t i = 0u; i < n; i++) {
                w[i] = value;
        }
}

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

static void
exercise_capacity(size_t capacity)
{
        bitmap_word_t storage[WORDS];
        bitmap_word_t before[WORDS];
        bool value;
        size_t count;
        size_t at;

        fill(storage, WORDS, (bitmap_word_t)0u);
        bitmap_t map;
        BM_CHECK(bitmap_init(&map, storage, WORDS, capacity));

        /* Freshly initialized bits are clear. */
        for (size_t i = 0u; i < capacity; i++) {
                value = true;
                BM_CHECK(bitmap_test(&map, i, &value));
                BM_CHECK(!value);
        }

        /* Setting bit i sets exactly one bit. */
        for (size_t i = 0u; i < capacity; i++) {
                BM_CHECK(bitmap_set(&map, i));
                BM_CHECK(bitmap_test(&map, i, &value));
                BM_CHECK(value);
                BM_CHECK(bitmap_count(&map, &count));
                BM_CHECK(count == (i + 1u));
                BM_CHECK(bitmap_scan(&map, 0u, &at));
                BM_CHECK(at == 0u);
                BM_CHECK(bitmap_scan(&map, i, &at));
                BM_CHECK(at == i);
        }

        /* Clearing bit i removes exactly that bit. */
        for (size_t i = 0u; i < capacity; i++) {
                BM_CHECK(bitmap_clear(&map, i));
                BM_CHECK(bitmap_test(&map, i, &value));
                BM_CHECK(!value);
                BM_CHECK(bitmap_count(&map, &count));
                BM_CHECK(count == (capacity - (i + 1u)));
        }

        /* Read-modify-write returns the previous value. */
        for (size_t i = 0u; i < capacity; i++) {
                value = true;
                BM_CHECK(bitmap_test_and_set(&map, i, &value));
                BM_CHECK(!value);
                BM_CHECK(bitmap_test_and_set(&map, i, &value));
                BM_CHECK(value);
                BM_CHECK(bitmap_test(&map, i, &value));
                BM_CHECK(value);

                value = false;
                BM_CHECK(bitmap_test_and_clear(&map, i, &value));
                BM_CHECK(value);
                BM_CHECK(bitmap_test_and_clear(&map, i, &value));
                BM_CHECK(!value);
                BM_CHECK(bitmap_test(&map, i, &value));
                BM_CHECK(!value);
        }

        /* The invalid index leaves storage and output parameters alone. */
        fill(before, WORDS, (bitmap_word_t)0x5A5Au);
        fill(storage, WORDS, (bitmap_word_t)0x5A5Au);
        value = true;
        count = 123u;
        at = 456u;
        BM_CHECK(!bitmap_set(&map, capacity));
        BM_CHECK(!bitmap_clear(&map, capacity));
        BM_CHECK(!bitmap_test(&map, capacity, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_test_and_set(&map, capacity, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_test_and_clear(&map, capacity, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_set(&map, SIZE_MAX));
        BM_CHECK(!bitmap_clear(&map, SIZE_MAX));
        BM_CHECK(!bitmap_test(&map, SIZE_MAX, &value));
        BM_CHECK(!bitmap_scan(&map, capacity + 1u, &at));
        BM_CHECK(at == 456u);
        BM_CHECK(!bitmap_any(&map, NULL));
        BM_CHECK(!bitmap_count(&map, NULL));
        BM_CHECK(words_equal(storage, before, WORDS));

        /* Null descriptor and null outputs fail without crashing. */
        value = true;
        BM_CHECK(!bitmap_set(NULL, 0u));
        BM_CHECK(!bitmap_clear(NULL, 0u));
        BM_CHECK(!bitmap_test(NULL, 0u, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_test_and_set(NULL, 0u, &value));
        BM_CHECK(!bitmap_test_and_clear(NULL, 0u, &value));
        BM_CHECK(!bitmap_test(&map, 0u, NULL));
        BM_CHECK(!bitmap_test_and_set(&map, 0u, NULL));
        BM_CHECK(!bitmap_test_and_clear(&map, 0u, NULL));
}

void
test_bits(void)
{
        bitmap_word_t storage[WORDS];
        bitmap_t map;
        bool value;

        bm_suite("bits");

        for (size_t i = 0u; i < (sizeof CAPACITIES / sizeof CAPACITIES[0]);
             i++) {
                exercise_capacity(CAPACITIES[i]);
        }

        /* Bit zero is the least significant bit of word zero. */
        BM_CHECK(bitmap_init(&map, storage, WORDS, 32u));
        BM_CHECK(bitmap_set(&map, 0u));
        BM_CHECK(storage[0] == (bitmap_word_t)0x0001u);
        BM_CHECK(bitmap_clear(&map, 0u));
        BM_CHECK(bitmap_set(&map, 15u));
        BM_CHECK(storage[0] == (bitmap_word_t)0x8000u);
        BM_CHECK(storage[1] == (bitmap_word_t)0u);
        BM_CHECK(bitmap_clear(&map, 15u));
        BM_CHECK(bitmap_set(&map, 16u));
        BM_CHECK(storage[0] == (bitmap_word_t)0u);
        BM_CHECK(storage[1] == (bitmap_word_t)0x0001u);

        /* A 17-bit map never touches bit 17 or above. */
        BM_CHECK(bitmap_init(&map, storage, WORDS, 17u));
        BM_CHECK(bitmap_set(&map, 16u));
        BM_CHECK(storage[1] == (bitmap_word_t)0x0001u);
        BM_CHECK(bitmap_test(&map, 16u, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_set(&map, 17u));
        BM_CHECK(storage[1] == (bitmap_word_t)0x0001u);
}
