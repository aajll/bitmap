#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

#define WORDS 12u
#define DEST  14u

static const size_t CAPACITIES[] = {0u, 1u, 16u, 17u, 23u, 33u, 192u};

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
        bitmap_word_t snapshot[WORDS];
        bitmap_word_t dest[DEST];
        size_t words = bitmap_words_for_bits(capacity);
        bitmap_t map;

        fill(storage, WORDS, (bitmap_word_t)0u);
        BM_CHECK(bitmap_init(&map, storage, WORDS, capacity));
        BM_CHECK(bitmap_fill_all(&map));

        /* Exact-size destination receives a tail-clean copy. */
        fill(dest, DEST, (bitmap_word_t)0xDEADu);
        BM_CHECK(bitmap_snapshot(&map, dest, words));
        BM_CHECK(words_equal(dest, storage, words));
        if (words != 0u) {
                BM_CHECK(dest[words - 1u] == tail_mask(capacity));
        }
        for (size_t i = words; i < DEST; i++) {
                BM_CHECK(dest[i] == (bitmap_word_t)0xDEADu);
        }

        /* Extra destination words stay untouched. */
        fill(dest, DEST, (bitmap_word_t)0x0F0Fu);
        BM_CHECK(bitmap_snapshot(&map, dest, DEST));
        for (size_t i = words; i < DEST; i++) {
                BM_CHECK(dest[i] == (bitmap_word_t)0x0F0Fu);
        }

        /* An undersized destination fails with no partial write. */
        if (words != 0u) {
                fill(dest, DEST, (bitmap_word_t)0xBEEFu);
                BM_CHECK(!bitmap_snapshot(&map, dest, words - 1u));
                for (size_t i = 0u; i < DEST; i++) {
                        BM_CHECK(dest[i] == (bitmap_word_t)0xBEEFu);
                }
                /* The failed call also left the bitmap unchanged. */
                fill(snapshot, WORDS, (bitmap_word_t)0x0u);
                BM_CHECK(bitmap_snapshot(&map, snapshot, WORDS));
                BM_CHECK(words_equal(snapshot, storage, WORDS));
        }

        /* A poisoned tail is zeroed in the copy. */
        if ((words != 0u) && ((capacity % 16u) != 0u)) {
                storage[words - 1u] |= (bitmap_word_t)~tail_mask(capacity);
                fill(dest, DEST, (bitmap_word_t)0x0u);
                BM_CHECK(bitmap_snapshot(&map, dest, DEST));
                BM_CHECK(dest[words - 1u] == tail_mask(capacity));
        }

        BM_CHECK(!bitmap_snapshot(&map, NULL, DEST));
        BM_CHECK(!bitmap_snapshot(NULL, dest, DEST));
}

void
test_snapshot(void)
{
        bitmap_word_t dest[DEST];
        bitmap_t map;

        bm_suite("snapshot");

        for (size_t i = 0u; i < (sizeof CAPACITIES / sizeof CAPACITIES[0]);
             i++) {
                exercise_capacity(CAPACITIES[i]);
        }

        /* An empty snapshot needs no words and writes nothing. */
        BM_CHECK(bitmap_init(&map, NULL, 0u, 0u));
        fill(dest, DEST, (bitmap_word_t)0x7777u);
        BM_CHECK(bitmap_snapshot(&map, dest, 0u));
        for (size_t i = 0u; i < DEST; i++) {
                BM_CHECK(dest[i] == (bitmap_word_t)0x7777u);
        }
}
