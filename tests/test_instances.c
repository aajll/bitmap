#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

void
test_instances(void)
{
        bitmap_word_t small_store[BITMAP_STORAGE_WORDS(1u)];
        bitmap_word_t mid_store[BITMAP_STORAGE_WORDS(37u)];
        bitmap_word_t large_store[BITMAP_STORAGE_WORDS(192u)];
        bitmap_t small;
        bitmap_t mid;
        bitmap_t large;
        bitmap_t alias;
        bool value;
        size_t count;

        bm_suite("independent instances");

        BM_CHECK(bitmap_init(&small, small_store,
                             BITMAP_ARRAY_WORDS(small_store), 1u));
        BM_CHECK(
            bitmap_init(&mid, mid_store, BITMAP_ARRAY_WORDS(mid_store), 37u));
        BM_CHECK(bitmap_init(&large, large_store,
                             BITMAP_ARRAY_WORDS(large_store), 192u));

        BM_CHECK(bitmap_set(&small, 0u));
        BM_CHECK(bitmap_set(&mid, 36u));
        BM_CHECK(bitmap_set(&large, 191u));

        BM_CHECK(bitmap_count(&small, &count) && (count == 1u));
        BM_CHECK(bitmap_count(&mid, &count) && (count == 1u));
        BM_CHECK(bitmap_count(&large, &count) && (count == 1u));

        BM_CHECK(bitmap_test(&small, 0u, &value) && value);
        BM_CHECK(bitmap_test(&mid, 36u, &value) && value);
        BM_CHECK(bitmap_test(&large, 191u, &value) && value);

        /* Each instance rejects indices beyond its own capacity. */
        BM_CHECK(!bitmap_set(&small, 1u));
        BM_CHECK(!bitmap_set(&mid, 37u));
        BM_CHECK(!bitmap_set(&large, 192u));

        BM_CHECK(bitmap_scan(&small, 0u, &count) && (count == 0u));
        BM_CHECK(bitmap_scan(&mid, 0u, &count) && (count == 36u));
        BM_CHECK(bitmap_scan(&large, 0u, &count) && (count == 191u));

        /* A copied descriptor aliases the same backing words. */
        alias = mid;
        BM_CHECK(bitmap_set(&alias, 5u));
        BM_CHECK(bitmap_test(&mid, 5u, &value) && value);
        BM_CHECK(bitmap_count(&mid, &count) && (count == 2u));
        BM_CHECK(bitmap_count(&small, &count) && (count == 1u));
        BM_CHECK(bitmap_count(&large, &count) && (count == 1u));
}
