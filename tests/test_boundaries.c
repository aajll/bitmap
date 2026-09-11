#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define DATA_WORDS  13u
#define GUARD_WORDS 3u

typedef struct {
        bitmap_word_t before[GUARD_WORDS];
        bitmap_word_t data[DATA_WORDS];
        bitmap_word_t after[GUARD_WORDS];
} guarded_t;

static void
fill(bitmap_word_t *w, size_t n, bitmap_word_t value)
{
        for (size_t i = 0u; i < n; i++) {
                w[i] = value;
        }
}

static bool
guards_ok(const guarded_t *g)
{
        for (size_t i = 0u; i < GUARD_WORDS; i++) {
                if ((g->before[i] != (bitmap_word_t)0xC0DEu)
                    || (g->after[i] != (bitmap_word_t)0xC0DEu)) {
                        return false;
                }
        }
        return true;
}

static bool
extras_ok(const guarded_t *g, size_t words)
{
        for (size_t i = words; i < DATA_WORDS; i++) {
                if (g->data[i] != (bitmap_word_t)0x5A5Au) {
                        return false;
                }
        }
        return true;
}

static void
exercise_capacity(size_t capacity)
{
        guarded_t g;
        bitmap_t map;
        size_t words = bitmap_words_for_bits(capacity);
        bool value;

        fill(g.before, GUARD_WORDS, (bitmap_word_t)0xC0DEu);
        fill(g.after, GUARD_WORDS, (bitmap_word_t)0xC0DEu);
        fill(g.data, DATA_WORDS, (bitmap_word_t)0x5A5Au);

        BM_CHECK(bitmap_init(&map, g.data, DATA_WORDS, capacity));
        BM_CHECK(guards_ok(&g));
        BM_CHECK(extras_ok(&g, words));

        BM_CHECK(bitmap_clear_all(&map));
        BM_CHECK(extras_ok(&g, words));
        BM_CHECK(bitmap_fill_all(&map));
        BM_CHECK(extras_ok(&g, words));
        if (capacity != 0u) {
                BM_CHECK(bitmap_set(&map, 0u));
                BM_CHECK(bitmap_set(&map, capacity - 1u));
                BM_CHECK(bitmap_test(&map, capacity - 1u, &value));
                BM_CHECK(value);
                BM_CHECK(!bitmap_test(&map, capacity, &value));
                BM_CHECK(!bitmap_set(&map, capacity));
                BM_CHECK(!bitmap_set(&map, SIZE_MAX));
        } else {
                BM_CHECK(!bitmap_set(&map, 0u));
                BM_CHECK(!bitmap_test(&map, 0u, &value));
        }
        BM_CHECK(bitmap_clear_all(&map));
        BM_CHECK(extras_ok(&g, words));
        BM_CHECK(guards_ok(&g));
}

void
test_boundaries(void)
{
        guarded_t g;
        bitmap_t map;
        size_t at;

        bm_suite("boundaries");

        /* Capacities 0..200 exercise every tail remainder and word count. */
        for (size_t capacity = 0u; capacity <= 200u; capacity++) {
                exercise_capacity(capacity);
        }

        /* Last valid index is capacity - 1; capacity is not an index. */
        BM_CHECK(bitmap_init(&map, g.data, DATA_WORDS, DATA_WORDS * 16u));
        BM_CHECK(bitmap_set(&map, (DATA_WORDS * 16u) - 1u));
        BM_CHECK(!bitmap_set(&map, DATA_WORDS * 16u));
        BM_CHECK(!bitmap_test(&map, DATA_WORDS * 16u, &(bool){false}));
        BM_CHECK(!bitmap_scan(&map, SIZE_MAX, &at));
        BM_CHECK(!bitmap_scan(&map, (DATA_WORDS * 16u) + 1u, &at));

        /* Storage sized one word too small is rejected. */
        BM_CHECK(bitmap_init(&map, g.data, DATA_WORDS, DATA_WORDS * 16u));
        BM_CHECK(!bitmap_init(&map, g.data, DATA_WORDS - 1u, DATA_WORDS * 16u));
        BM_CHECK(map.words == g.data);
        BM_CHECK(map.bit_count == (DATA_WORDS * 16u));

        /* Huge capacities fail cleanly against real storage. */
        BM_CHECK(!bitmap_init(&map, g.data, DATA_WORDS, SIZE_MAX));
        BM_CHECK(map.words == g.data);
        BM_CHECK(map.bit_count == (DATA_WORDS * 16u));

        /* A forged descriptor (bits but no storage) is rejected everywhere. */
        bitmap_t forged;
        bool value = true;
        size_t count = 77u;

        forged.words = NULL;
        forged.bit_count = 8u;
        BM_CHECK(!bitmap_clear_all(&forged));
        BM_CHECK(!bitmap_fill_all(&forged));
        BM_CHECK(!bitmap_set(&forged, 0u));
        BM_CHECK(!bitmap_clear(&forged, 0u));
        BM_CHECK(!bitmap_test(&forged, 0u, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_test_and_set(&forged, 0u, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_test_and_clear(&forged, 0u, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_any(&forged, &value));
        BM_CHECK(value);
        BM_CHECK(!bitmap_count(&forged, &count));
        BM_CHECK(count == 77u);
        at = 123u;
        BM_CHECK(!bitmap_scan(&forged, 0u, &at));
        BM_CHECK(at == 123u);
        BM_CHECK(!bitmap_snapshot(&forged, g.data, DATA_WORDS));

        /* A valid map rejects a null scan output. */
        BM_CHECK(bitmap_init(&map, g.data, DATA_WORDS, 8u));
        BM_CHECK(!bitmap_scan(&map, 0u, NULL));
}
