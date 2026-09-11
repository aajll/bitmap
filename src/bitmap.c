/**
 * SPDX-License-Identifier: MIT
 *
 * @file bitmap.c
 *
 * @brief
 *    Implementation of the capacity-independent bitmap set.
 *
 * @par MISRA C:2023 deviation record
 *    bitmap is written to be MISRA C:2023 aware (fixed-width unsigned types,
 *    explicit @c u suffixes, no heap, no recursion, no @c errno, no
 *    undefined-behaviour shifts, no hidden state). The machine-checked
 *    deviation record is @c misra-deviations.txt plus the inline suppression
 *    comments:
 *
 *    @li Rule 15.5 (single point of exit): public entry points use early
 *        guard-clause returns for the defensive argument contract. No
 *        function acquires a resource or has teardown to share.
 *
 *    Full tool-driven compliance additionally requires a certified static
 *    analyser, which this repository does not vendor.
 */

/* ================ INCLUDES ================================================ */

#include "bitmap.h"

/* ================ STATIC FUNCTIONS ======================================== */

/**
 * @brief Whether a descriptor can be dereferenced safely.
 *
 * @details
 *    A zero-bit descriptor is valid with a null word pointer; a non-empty
 *    descriptor is valid only with storage. The real length of @c words is a
 *    documented caller precondition the library cannot check.
 */
static bool
bitmap_view_ok(const bitmap_t *map)
{
        if (map == NULL) {
                return false;
        }
        if ((map->bit_count != (size_t)0) && (map->words == NULL)) {
                return false;
        }
        return true;
}

/**
 * @brief Mask covering the logical low bits of the final word.
 *
 * @return 0xFFFF when @p bit_count is a multiple of the word width, else
 *         @c (1 @c << @c remainder) @c - @c 1.
 */
static bitmap_word_t
bitmap_tail_mask(size_t bit_count)
{
        size_t remainder = bit_count % BITMAP_BITS_PER_WORD;

        if (remainder == (size_t)0) {
                return (bitmap_word_t)0xFFFFu;
        }
        return (bitmap_word_t)((1u << remainder) - 1u);
}

/**
 * @brief Population count of one 16-bit word, branch-free and constant time.
 */
static size_t
bitmap_word_popcount(bitmap_word_t word)
{
        uint32_t value = (uint32_t)word;

        value = value - ((value >> 1u) & (uint32_t)0x5555u);
        value =
            (value & (uint32_t)0x3333u) + ((value >> 2u) & (uint32_t)0x3333u);
        value = (value + (value >> 4u)) & (uint32_t)0x0F0Fu;
        value = (value + (value >> 8u)) & (uint32_t)0x001Fu;
        return (size_t)value;
}

/**
 * @brief Index of the lowest set bit. Undefined for a zero word; callers
 *        check first.
 */
static size_t
bitmap_word_ctz(bitmap_word_t word)
{
        size_t bit = 0;
        bitmap_word_t value = word;

        while ((value & (bitmap_word_t)1u) == (bitmap_word_t)0u) {
                value = (bitmap_word_t)(value >> 1u);
                bit++;
        }
        return bit;
}

/* ================ CAPACITY ================================================ */

size_t
bitmap_words_for_bits(size_t bit_count)
{
        size_t words = bit_count / BITMAP_BITS_PER_WORD;

        if ((bit_count % BITMAP_BITS_PER_WORD) != (size_t)0) {
                words = words + (size_t)1;
        }
        return words;
}

/* ================ LIFECYCLE =============================================== */

bool
bitmap_init(bitmap_t *map, bitmap_word_t *storage, size_t storage_words,
            size_t bit_count)
{
        size_t required;

        if (map == NULL) {
                return false;
        }

        required = bitmap_words_for_bits(bit_count);
        if (storage == NULL) {
                if ((storage_words != (size_t)0) || (bit_count != (size_t)0)) {
                        return false;
                }
        }
        if ((storage != NULL) && (storage_words < required)) {
                return false;
        }

        if (required != (size_t)0) {
                for (size_t i = (size_t)0; i < required; i++) {
                        storage[i] = (bitmap_word_t)0;
                }
                map->words = storage;
        } else {
                map->words = NULL;
        }
        map->bit_count = bit_count;
        return true;
}

/* ================ WHOLE-MAP OPERATIONS ==================================== */

bool
bitmap_clear_all(const bitmap_t *map)
{
        size_t word_count;
        size_t i;

        if (!bitmap_view_ok(map)) {
                return false;
        }

        word_count = bitmap_words_for_bits(map->bit_count);
        for (i = (size_t)0; i < word_count; i++) {
                map->words[i] = (bitmap_word_t)0;
        }
        return true;
}

bool
bitmap_fill_all(const bitmap_t *map)
{
        size_t word_count;
        size_t i;

        if (!bitmap_view_ok(map)) {
                return false;
        }

        word_count = bitmap_words_for_bits(map->bit_count);
        for (i = (size_t)0; i < word_count; i++) {
                map->words[i] = (bitmap_word_t)0xFFFFu;
        }
        if (word_count != (size_t)0) {
                map->words[word_count - (size_t)1] &=
                    bitmap_tail_mask(map->bit_count);
        }
        return true;
}

/* ================ SINGLE-BIT OPERATIONS =================================== */

bool
bitmap_set(const bitmap_t *map, size_t index)
{
        bitmap_word_t mask;

        if (!bitmap_view_ok(map) || (index >= map->bit_count)) {
                return false;
        }

        mask = (bitmap_word_t)(1u << (index % BITMAP_BITS_PER_WORD));
        map->words[index / BITMAP_BITS_PER_WORD] |= mask;
        return true;
}

bool
bitmap_clear(const bitmap_t *map, size_t index)
{
        bitmap_word_t mask;

        if (!bitmap_view_ok(map) || (index >= map->bit_count)) {
                return false;
        }

        mask = (bitmap_word_t)(1u << (index % BITMAP_BITS_PER_WORD));
        map->words[index / BITMAP_BITS_PER_WORD] &= (bitmap_word_t)~mask;
        return true;
}

bool
bitmap_test(const bitmap_t *map, size_t index, bool *value)
{
        bitmap_word_t mask;

        if (!bitmap_view_ok(map) || (index >= map->bit_count)
            || (value == NULL)) {
                return false;
        }

        mask = (bitmap_word_t)(1u << (index % BITMAP_BITS_PER_WORD));
        *value = (map->words[index / BITMAP_BITS_PER_WORD] & mask)
                 != (bitmap_word_t)0u;
        return true;
}

bool
bitmap_test_and_set(const bitmap_t *map, size_t index, bool *previous)
{
        bitmap_word_t *word;
        bitmap_word_t mask;

        if (!bitmap_view_ok(map) || (index >= map->bit_count)
            || (previous == NULL)) {
                return false;
        }

        word = &map->words[index / BITMAP_BITS_PER_WORD];
        mask = (bitmap_word_t)(1u << (index % BITMAP_BITS_PER_WORD));
        *previous = (*word & mask) != (bitmap_word_t)0u;
        *word = (bitmap_word_t)(*word | mask);
        return true;
}

bool
bitmap_test_and_clear(const bitmap_t *map, size_t index, bool *previous)
{
        bitmap_word_t *word;
        bitmap_word_t mask;

        if (!bitmap_view_ok(map) || (index >= map->bit_count)
            || (previous == NULL)) {
                return false;
        }

        word = &map->words[index / BITMAP_BITS_PER_WORD];
        mask = (bitmap_word_t)(1u << (index % BITMAP_BITS_PER_WORD));
        *previous = (*word & mask) != (bitmap_word_t)0u;
        *word = (bitmap_word_t)(*word & (bitmap_word_t)~mask);
        return true;
}

/* ================ AGGREGATE QUERIES ======================================= */

bool
bitmap_any(const bitmap_t *map, bool *any)
{
        size_t word_count;

        if (!bitmap_view_ok(map) || (any == NULL)) {
                return false;
        }

        *any = false;
        word_count = bitmap_words_for_bits(map->bit_count);
        for (size_t i = (size_t)0; i < word_count; i++) {
                bitmap_word_t word = map->words[i];
                if (i == (word_count - (size_t)1)) {
                        word &= bitmap_tail_mask(map->bit_count);
                }
                if (word != (bitmap_word_t)0u) {
                        *any = true;
                        break;
                }
        }
        return true;
}

bool
bitmap_count(const bitmap_t *map, size_t *count)
{
        size_t word_count;
        size_t total = 0;

        if (!bitmap_view_ok(map) || (count == NULL)) {
                return false;
        }

        word_count = bitmap_words_for_bits(map->bit_count);
        for (size_t i = (size_t)0; i < word_count; i++) {
                bitmap_word_t word = map->words[i];
                if (i == (word_count - (size_t)1)) {
                        word &= bitmap_tail_mask(map->bit_count);
                }
                total += bitmap_word_popcount(word);
        }
        *count = total;
        return true;
}

bool
bitmap_scan(const bitmap_t *map, size_t start, size_t *index)
{
        size_t word_count;
        size_t first_word;

        if (!bitmap_view_ok(map) || (index == NULL)
            || (start > map->bit_count)) {
                return false;
        }

        word_count = bitmap_words_for_bits(map->bit_count);
        first_word = start / BITMAP_BITS_PER_WORD;
        for (size_t i = first_word; i < word_count; i++) {
                bitmap_word_t word = map->words[i];
                if (i == (word_count - (size_t)1)) {
                        word &= bitmap_tail_mask(map->bit_count);
                }
                if (i == first_word) {
                        word &=
                            (bitmap_word_t)(0xFFFFu
                                            << (start % BITMAP_BITS_PER_WORD));
                }
                if (word != (bitmap_word_t)0u) {
                        *index =
                            (i * BITMAP_BITS_PER_WORD) + bitmap_word_ctz(word);
                        return true;
                }
        }
        *index = map->bit_count;
        return true;
}

/* ================ SNAPSHOT ================================================ */

bool
bitmap_snapshot(const bitmap_t *map, bitmap_word_t *dest, size_t dest_words)
{
        size_t word_count;

        if (!bitmap_view_ok(map) || (dest == NULL)) {
                return false;
        }

        word_count = bitmap_words_for_bits(map->bit_count);
        if (dest_words < word_count) {
                return false;
        }

        for (size_t i = (size_t)0; i < word_count; i++) {
                dest[i] = map->words[i];
        }
        if (word_count != (size_t)0) {
                dest[word_count - (size_t)1] &=
                    bitmap_tail_mask(map->bit_count);
        }
        return true;
}
