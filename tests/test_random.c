#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>
#include <stdint.h>

#define WORDS            13u
#define REF_MAX          208u
#define OPS_PER_CAPACITY 1500u

static const size_t CAPACITIES[] = {1u,  2u,  15u,  16u,  17u,  31u, 32u,
                                    33u, 64u, 127u, 128u, 192u, 200u};

static uint32_t rng_state = 0x12345678u;

static uint32_t
rng_next(void)
{
        rng_state = (rng_state * 1664525u) + 1013904223u;
        return rng_state;
}

static size_t
rng_below(size_t bound)
{
        return (size_t)(rng_next() % (uint32_t)bound);
}

static size_t
ref_count(const bool *bits, size_t capacity)
{
        size_t total = 0u;

        for (size_t i = 0u; i < capacity; i++) {
                if (bits[i]) {
                        total++;
                }
        }
        return total;
}

static size_t
ref_scan(const bool *bits, size_t capacity, size_t start)
{
        for (size_t i = start; i < capacity; i++) {
                if (bits[i]) {
                        return i;
                }
        }
        return capacity;
}

static void
ref_pack(const bool *bits, size_t capacity, bitmap_word_t *words)
{
        size_t count = bitmap_words_for_bits(capacity);

        for (size_t i = 0u; i < count; i++) {
                words[i] = (bitmap_word_t)0u;
        }
        for (size_t i = 0u; i < capacity; i++) {
                if (bits[i]) {
                        words[i / 16u] |= (bitmap_word_t)(1u << (i % 16u));
                }
        }
}

static void
compare_aggregates(const bitmap_t *map, const bool *bits)
{
        bool any = false;
        size_t count = 0u;
        size_t at = 0u;
        size_t start = rng_below(map->bit_count + 1u);

        BM_CHECK(bitmap_any(map, &any));
        BM_CHECK(any == (ref_count(bits, map->bit_count) != 0u));
        BM_CHECK(bitmap_count(map, &count));
        BM_CHECK(count == ref_count(bits, map->bit_count));
        BM_CHECK(bitmap_scan(map, 0u, &at));
        BM_CHECK(at == ref_scan(bits, map->bit_count, 0u));
        BM_CHECK(bitmap_scan(map, start, &at));
        BM_CHECK(at == ref_scan(bits, map->bit_count, start));
        BM_CHECK(bitmap_scan(map, map->bit_count, &at));
        BM_CHECK(at == map->bit_count);
}

static void
exercise_capacity(size_t capacity)
{
        static bool bits[REF_MAX];
        bitmap_word_t storage[WORDS];
        bitmap_word_t snap[WORDS];
        bitmap_word_t packed[WORDS];
        bitmap_t map;

        for (size_t i = 0u; i < REF_MAX; i++) {
                bits[i] = false;
        }
        BM_CHECK(bitmap_init(&map, storage, WORDS, capacity));

        for (size_t op = 0u; op < OPS_PER_CAPACITY; op++) {
                size_t index = rng_below(capacity);
                bool value = false;
                bool previous = false;

                switch (rng_next() % 10u) {
                case 0u:
                case 1u:
                case 2u:
                        BM_CHECK(bitmap_set(&map, index));
                        bits[index] = true;
                        break;
                case 3u:
                case 4u:
                        BM_CHECK(bitmap_clear(&map, index));
                        bits[index] = false;
                        break;
                case 5u:
                        BM_CHECK(bitmap_test(&map, index, &value));
                        BM_CHECK(value == bits[index]);
                        break;
                case 6u:
                        BM_CHECK(bitmap_test_and_set(&map, index, &previous));
                        BM_CHECK(previous == bits[index]);
                        bits[index] = true;
                        break;
                case 7u:
                        BM_CHECK(bitmap_test_and_clear(&map, index, &previous));
                        BM_CHECK(previous == bits[index]);
                        bits[index] = false;
                        break;
                case 8u:
                        if ((rng_next() & 1u) != 0u) {
                                BM_CHECK(bitmap_clear_all(&map));
                                for (size_t i = 0u; i < REF_MAX; i++) {
                                        bits[i] = false;
                                }
                        } else {
                                BM_CHECK(bitmap_fill_all(&map));
                                for (size_t i = 0u; i < capacity; i++) {
                                        bits[i] = true;
                                }
                        }
                        break;
                default:
                        /* Invalid indices never look like a transition. */
                        value = true;
                        previous = true;
                        BM_CHECK(!bitmap_set(&map, capacity));
                        BM_CHECK(!bitmap_clear(&map, capacity));
                        BM_CHECK(!bitmap_test(&map, capacity, &value));
                        BM_CHECK(value);
                        BM_CHECK(
                            !bitmap_test_and_set(&map, capacity, &previous));
                        BM_CHECK(previous);
                        BM_CHECK(
                            !bitmap_test_and_clear(&map, capacity, &previous));
                        BM_CHECK(previous);
                        break;
                }

                compare_aggregates(&map, bits);

                if ((op % 64u) == 0u) {
                        ref_pack(bits, capacity, packed);
                        BM_CHECK(bitmap_snapshot(&map, snap, WORDS));
                        for (size_t i = 0u; i < bitmap_words_for_bits(capacity);
                             i++) {
                                BM_CHECK(snap[i] == packed[i]);
                        }
                }
        }

        ref_pack(bits, capacity, packed);
        BM_CHECK(bitmap_snapshot(&map, snap, WORDS));
        for (size_t i = 0u; i < bitmap_words_for_bits(capacity); i++) {
                BM_CHECK(snap[i] == packed[i]);
        }
}

void
test_random(void)
{
        bm_suite("randomized reference model");

        for (size_t i = 0u; i < (sizeof CAPACITIES / sizeof CAPACITIES[0]);
             i++) {
                exercise_capacity(CAPACITIES[i]);
        }
}
