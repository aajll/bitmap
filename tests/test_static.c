#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define TABLE_BITS 192u
#define ODD_BITS   33u
#define EXACT_BITS 32u

static bitmap_word_t table_words[BITMAP_STORAGE_WORDS(TABLE_BITS)];
static bitmap_word_t odd_words[BITMAP_STORAGE_WORDS(ODD_BITS) + 1u];
static bitmap_word_t exact_words[2];
static bitmap_word_t empty_words[1];

static const bitmap_t table_map = BITMAP_INITIALIZER(table_words, TABLE_BITS);

/* A read-only table of descriptors with different capacities. */
static const bitmap_t maps[] = {
    BITMAP_INITIALIZER(empty_words, 0u),
    BITMAP_INITIALIZER(odd_words, ODD_BITS),
    BITMAP_INITIALIZER(table_words, TABLE_BITS),
    BITMAP_INITIALIZER(exact_words, EXACT_BITS),
};

void
test_static(void)
{
        bitmap_word_t snap[BITMAP_STORAGE_WORDS(ODD_BITS)];
        bool value = false;
        size_t count = 0u;
        size_t index = 0u;

        bm_suite("static");

        /* The initializer binds storage without clearing or canonicalizing. */
        BM_CHECK(table_map.words == table_words);
        BM_CHECK(table_map.bit_count == TABLE_BITS);
        BM_CHECK(maps[0].words == empty_words);
        BM_CHECK(maps[0].bit_count == (size_t)0);
        BM_CHECK(maps[1].words == odd_words);
        BM_CHECK(maps[1].bit_count == ODD_BITS);
        BM_CHECK(maps[3].bit_count == EXACT_BITS);

        /* Static storage starts zeroed, so no init call is needed. */
        BM_CHECK(bitmap_count(&table_map, &count) && (count == (size_t)0));

        /* Every mutating operation works through a const descriptor. */
        BM_CHECK(bitmap_set(&table_map, TABLE_BITS - 1u));
        BM_CHECK(table_words[11] == (bitmap_word_t)0x8000u);
        BM_CHECK(bitmap_test_and_set(&table_map, 0u, &value) && !value);
        BM_CHECK(bitmap_test_and_clear(&table_map, 0u, &value) && value);
        BM_CHECK(bitmap_clear(&table_map, TABLE_BITS - 1u));
        BM_CHECK(bitmap_any(&table_map, &value) && !value);
        BM_CHECK(bitmap_fill_all(&table_map));
        BM_CHECK(bitmap_count(&table_map, &count) && (count == TABLE_BITS));
        BM_CHECK(bitmap_clear_all(&table_map));
        BM_CHECK(bitmap_any(&table_map, &value) && !value);

        /* Descriptors in the table alias the same words as table_map. */
        BM_CHECK(bitmap_set(&maps[2], 40u));
        BM_CHECK(bitmap_test(&table_map, 40u, &value) && value);
        BM_CHECK(bitmap_clear_all(&table_map));

        /* Bounds checks still apply through a const descriptor. */
        BM_CHECK(!bitmap_set(&maps[1], ODD_BITS));
        BM_CHECK(!bitmap_clear(&maps[1], ODD_BITS));

        /* Exact-fit storage covers the final bit. */
        BM_CHECK(bitmap_set(&maps[3], EXACT_BITS - 1u));
        BM_CHECK(exact_words[1] == (bitmap_word_t)0x8000u);
        BM_CHECK(!bitmap_set(&maps[3], EXACT_BITS));

        /* Fill zeroes the tail and leaves the spare word untouched. */
        odd_words[3] = (bitmap_word_t)0xA5A5u;
        BM_CHECK(bitmap_fill_all(&maps[1]));
        BM_CHECK(odd_words[2] == (bitmap_word_t)0x0001u);
        BM_CHECK(odd_words[3] == (bitmap_word_t)0xA5A5u);
        BM_CHECK(bitmap_scan(&maps[1], 32u, &index) && (index == 32u));
        BM_CHECK(bitmap_snapshot(&maps[1], snap, BITMAP_ARRAY_WORDS(snap)));
        BM_CHECK(snap[2] == (bitmap_word_t)0x0001u);

        /* A zero-bit descriptor keeps its pointer but never uses it. */
        empty_words[0] = (bitmap_word_t)0xFFFFu;
        BM_CHECK(bitmap_any(&maps[0], &value) && !value);
        BM_CHECK(bitmap_count(&maps[0], &count) && (count == (size_t)0));
        BM_CHECK(bitmap_scan(&maps[0], 0u, &index) && (index == (size_t)0));
        BM_CHECK(!bitmap_set(&maps[0], 0u));
        BM_CHECK(bitmap_fill_all(&maps[0]));
        BM_CHECK(bitmap_clear_all(&maps[0]));
        BM_CHECK(empty_words[0] == (bitmap_word_t)0xFFFFu);
        snap[0] = (bitmap_word_t)0x1234u;
        BM_CHECK(bitmap_snapshot(&maps[0], snap, 0u));
        BM_CHECK(snap[0] == (bitmap_word_t)0x1234u);

        /* Automatic storage is not zeroed: clear it before the first query. */
        {
                bitmap_word_t local_words[BITMAP_STORAGE_WORDS(ODD_BITS)];
                const bitmap_t local =
                    BITMAP_INITIALIZER(local_words, ODD_BITS);

                for (size_t i = 0u; i < BITMAP_ARRAY_WORDS(local_words); i++) {
                        local_words[i] = (bitmap_word_t)0xFFFFu;
                }
                BM_CHECK(bitmap_count(&local, &count) && (count == ODD_BITS));
                BM_CHECK(bitmap_clear_all(&local));
                BM_CHECK(bitmap_count(&local, &count) && (count == (size_t)0));
        }
}
