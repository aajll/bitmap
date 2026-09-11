/**
 * SPDX-License-Identifier: MIT
 *
 * @file prototype.c
 *
 * @brief Explicit-capacity prototype for footprint comparison only.
 *
 * The implementation mirrors src/bitmap.c but takes the capacity as a
 * per-call argument instead of reading it from a descriptor. It is not
 * installed, not tested by the unit suite, and not analysed by misch.
 */

#include "prototype.h"

size_t
proto_words_for_bits(size_t bit_count)
{
        return (bit_count / 16u) + (((bit_count % 16u) != 0u) ? 1u : 0u);
}

static proto_word_t
proto_tail_mask(size_t bit_count)
{
        size_t remainder = bit_count % 16u;

        if (remainder == 0u) {
                return (proto_word_t)0xFFFFu;
        }
        return (proto_word_t)((1u << remainder) - 1u);
}

bool
proto_set(proto_word_t *words, size_t bit_count, size_t index)
{
        if ((words == NULL) || (index >= bit_count)) {
                return false;
        }
        words[index / 16u] |= (proto_word_t)(1u << (index % 16u));
        return true;
}

bool
proto_clear(proto_word_t *words, size_t bit_count, size_t index)
{
        if ((words == NULL) || (index >= bit_count)) {
                return false;
        }
        words[index / 16u] &= (proto_word_t) ~(1u << (index % 16u));
        return true;
}

bool
proto_test(const proto_word_t *words, size_t bit_count, size_t index,
           bool *value)
{
        if ((words == NULL) || (index >= bit_count) || (value == NULL)) {
                return false;
        }
        *value =
            (words[index / 16u] & (proto_word_t)(1u << (index % 16u))) != 0u;
        return true;
}

bool
proto_test_and_set(proto_word_t *words, size_t bit_count, size_t index,
                   bool *previous)
{
        proto_word_t mask;

        if ((words == NULL) || (index >= bit_count) || (previous == NULL)) {
                return false;
        }
        mask = (proto_word_t)(1u << (index % 16u));
        *previous = (words[index / 16u] & mask) != 0u;
        words[index / 16u] |= mask;
        return true;
}

bool
proto_test_and_clear(proto_word_t *words, size_t bit_count, size_t index,
                     bool *previous)
{
        proto_word_t mask;

        if ((words == NULL) || (index >= bit_count) || (previous == NULL)) {
                return false;
        }
        mask = (proto_word_t)(1u << (index % 16u));
        *previous = (words[index / 16u] & mask) != 0u;
        words[index / 16u] &= (proto_word_t)~mask;
        return true;
}

bool
proto_any(const proto_word_t *words, size_t bit_count, bool *any)
{
        size_t word_count;

        if ((words == NULL) || (any == NULL)) {
                return false;
        }
        *any = false;
        word_count = proto_words_for_bits(bit_count);
        for (size_t i = 0u; i < word_count; i++) {
                proto_word_t word = words[i];

                if (i == (word_count - 1u)) {
                        word &= proto_tail_mask(bit_count);
                }
                if (word != 0u) {
                        *any = true;
                        break;
                }
        }
        return true;
}

bool
proto_count(const proto_word_t *words, size_t bit_count, size_t *count)
{
        size_t word_count;
        size_t total = 0u;

        if ((words == NULL) || (count == NULL)) {
                return false;
        }
        word_count = proto_words_for_bits(bit_count);
        for (size_t i = 0u; i < word_count; i++) {
                proto_word_t word = words[i];
                uint32_t value;

                if (i == (word_count - 1u)) {
                        word &= proto_tail_mask(bit_count);
                }
                value = (uint32_t)word;
                value = value - ((value >> 1u) & (uint32_t)0x5555u);
                value = (value & (uint32_t)0x3333u)
                        + ((value >> 2u) & (uint32_t)0x3333u);
                value = (value + (value >> 4u)) & (uint32_t)0x0F0Fu;
                value = (value + (value >> 8u)) & (uint32_t)0x001Fu;
                total += (size_t)value;
        }
        *count = total;
        return true;
}

bool
proto_scan(const proto_word_t *words, size_t bit_count, size_t start,
           size_t *index)
{
        size_t word_count;

        if ((words == NULL) || (index == NULL) || (start > bit_count)) {
                return false;
        }
        word_count = proto_words_for_bits(bit_count);
        for (size_t i = start / 16u; i < word_count; i++) {
                proto_word_t word = words[i];

                if (i == (word_count - 1u)) {
                        word &= proto_tail_mask(bit_count);
                }
                if (i == (start / 16u)) {
                        word &= (proto_word_t)(0xFFFFu << (start % 16u));
                }
                if (word != 0u) {
                        size_t bit = 0u;

                        while ((word & 1u) == 0u) {
                                word = (proto_word_t)(word >> 1u);
                                bit++;
                        }
                        *index = (i * 16u) + bit;
                        return true;
                }
        }
        *index = bit_count;
        return true;
}

bool
proto_clear_all(proto_word_t *words, size_t bit_count)
{
        size_t word_count;

        if (words == NULL) {
                return false;
        }
        word_count = proto_words_for_bits(bit_count);
        for (size_t i = 0u; i < word_count; i++) {
                words[i] = 0u;
        }
        return true;
}

bool
proto_fill_all(proto_word_t *words, size_t bit_count)
{
        size_t word_count;

        if (words == NULL) {
                return false;
        }
        word_count = proto_words_for_bits(bit_count);
        for (size_t i = 0u; i < word_count; i++) {
                words[i] = (proto_word_t)0xFFFFu;
        }
        if (word_count != 0u) {
                words[word_count - 1u] &= proto_tail_mask(bit_count);
        }
        return true;
}

bool
proto_snapshot(const proto_word_t *words, size_t bit_count, proto_word_t *dest,
               size_t dest_words)
{
        size_t word_count;

        if ((words == NULL) || (dest == NULL)) {
                return false;
        }
        word_count = proto_words_for_bits(bit_count);
        if (dest_words < word_count) {
                return false;
        }
        for (size_t i = 0u; i < word_count; i++) {
                dest[i] = words[i];
        }
        if (word_count != 0u) {
                dest[word_count - 1u] &= proto_tail_mask(bit_count);
        }
        return true;
}
