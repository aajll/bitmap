#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define WORDS 12u

static const size_t CAPACITIES[] = {0u,  1u,  15u, 16u, 17u,
                                    23u, 31u, 32u, 33u, 192u};

static bitmap_word_t
tail_mask(size_t bits)
{
        size_t remainder = bits % 16u;

        if (remainder == 0u) {
                return (bitmap_word_t)0xFFFFu;
        }
        return (bitmap_word_t)((1u << remainder) - 1u);
}

static void
poison_tail(bitmap_word_t *storage, size_t bits)
{
        size_t words = bitmap_words_for_bits(bits);

        if ((words == 0u) || ((bits % 16u) == 0u)) {
                return;
        }
        storage[words - 1u] |= (bitmap_word_t)~tail_mask(bits);
}

static void
exercise_capacity(size_t capacity)
{
        bitmap_word_t storage[WORDS];
        bitmap_t map;
        bool any = true;
        size_t count = 99u;
        size_t at = 99u;
        size_t words = bitmap_words_for_bits(capacity);

        for (size_t i = 0u; i < WORDS; i++) {
                storage[i] = (bitmap_word_t)0u;
        }
        BM_CHECK(bitmap_init(&map, storage, WORDS, capacity));

        BM_CHECK(bitmap_clear_all(&map));
        BM_CHECK(bitmap_any(&map, &any));
        BM_CHECK(!any);
        BM_CHECK(bitmap_count(&map, &count));
        BM_CHECK(count == (size_t)0);
        BM_CHECK(bitmap_scan(&map, 0u, &at));
        BM_CHECK(at == capacity);

        BM_CHECK(bitmap_fill_all(&map));
        BM_CHECK(bitmap_any(&map, &any));
        BM_CHECK(any == (capacity != 0u));
        BM_CHECK(bitmap_count(&map, &count));
        BM_CHECK(count == capacity);
        BM_CHECK(bitmap_scan(&map, 0u, &at));
        BM_CHECK(at == 0u);
        BM_CHECK(bitmap_scan(&map, capacity, &at));
        BM_CHECK(at == capacity);
        if (words != 0u) {
                BM_CHECK(storage[words - 1u] == tail_mask(capacity));
        }

        /* Scan finds every set position and stops at the sentinel. */
        BM_CHECK(bitmap_clear_all(&map));
        if (capacity >= 3u) {
                BM_CHECK(bitmap_set(&map, 0u));
                BM_CHECK(bitmap_set(&map, capacity / 2u));
                BM_CHECK(bitmap_set(&map, capacity - 1u));
                BM_CHECK(bitmap_scan(&map, 0u, &at));
                BM_CHECK(at == 0u);
                BM_CHECK(bitmap_scan(&map, 1u, &at));
                BM_CHECK(at == (capacity / 2u));
                BM_CHECK(bitmap_scan(&map, (capacity / 2u) + 1u, &at));
                BM_CHECK(at == (capacity - 1u));
                BM_CHECK(bitmap_scan(&map, capacity - 1u, &at));
                BM_CHECK(at == (capacity - 1u));
                BM_CHECK(bitmap_scan(&map, capacity, &at));
                BM_CHECK(at == capacity);
        }

        /* Poisoned unused high bits are invisible to every query. */
        BM_CHECK(bitmap_clear_all(&map));
        poison_tail(storage, capacity);
        BM_CHECK(bitmap_any(&map, &any));
        BM_CHECK(!any);
        BM_CHECK(bitmap_count(&map, &count));
        BM_CHECK(count == (size_t)0);
        BM_CHECK(bitmap_scan(&map, 0u, &at));
        BM_CHECK(at == capacity);
        if (capacity != 0u) {
                BM_CHECK(bitmap_set(&map, 0u));
                BM_CHECK(bitmap_any(&map, &any));
                BM_CHECK(any);
                BM_CHECK(bitmap_count(&map, &count));
                BM_CHECK(count == (size_t)1);
                BM_CHECK(bitmap_scan(&map, 0u, &at));
                BM_CHECK(at == 0u);
        }

        BM_CHECK(!bitmap_clear_all(NULL));
        BM_CHECK(!bitmap_fill_all(NULL));
        BM_CHECK(!bitmap_any(NULL, &any));
        BM_CHECK(!bitmap_count(NULL, &count));
        BM_CHECK(!bitmap_scan(NULL, 0u, &at));
}

void
test_bulk(void)
{
        bitmap_word_t storage[WORDS];
        bitmap_t map;
        size_t at;

        bm_suite("bulk");

        for (size_t i = 0u; i < (sizeof CAPACITIES / sizeof CAPACITIES[0]);
             i++) {
                exercise_capacity(CAPACITIES[i]);
        }

        /* Cross-word scan on a 33-bit map. */
        BM_CHECK(bitmap_init(&map, storage, WORDS, 33u));
        BM_CHECK(bitmap_set(&map, 15u));
        BM_CHECK(bitmap_set(&map, 16u));
        BM_CHECK(bitmap_set(&map, 32u));
        BM_CHECK(bitmap_scan(&map, 0u, &at));
        BM_CHECK(at == 15u);
        BM_CHECK(bitmap_scan(&map, 15u, &at));
        BM_CHECK(at == 15u);
        BM_CHECK(bitmap_scan(&map, 16u, &at));
        BM_CHECK(at == 16u);
        BM_CHECK(bitmap_scan(&map, 17u, &at));
        BM_CHECK(at == 32u);
        BM_CHECK(bitmap_scan(&map, 33u, &at));
        BM_CHECK(at == 33u);

        /* fill_all zeroes the unused tail; count stays exact. */
        BM_CHECK(bitmap_fill_all(&map));
        BM_CHECK(storage[2] == (bitmap_word_t)0x0001u);
        BM_CHECK(bitmap_count(&map, &at) && at == 33u);
}
