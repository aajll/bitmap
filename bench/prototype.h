/**
 * SPDX-License-Identifier: MIT
 *
 * @file prototype.h
 *
 * @brief
 *    Small explicit-capacity prototype used only for footprint comparison.
 *
 *    This is deliberately not a second library: it implements the same core
 *    operation set with the capacity passed at each call instead of stored in
 *    a descriptor, so the RAM and code-size difference can be measured. It is
 *    compiled by bench/measure.sh and is not part of the installed library.
 */

#ifndef BENCH_PROTOTYPE_H_
#define BENCH_PROTOTYPE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef uint16_t proto_word_t;

size_t proto_words_for_bits(size_t bit_count);
bool proto_set(proto_word_t *words, size_t bit_count, size_t index);
bool proto_clear(proto_word_t *words, size_t bit_count, size_t index);
bool proto_test(const proto_word_t *words, size_t bit_count, size_t index,
                bool *value);
bool proto_test_and_set(proto_word_t *words, size_t bit_count, size_t index,
                        bool *previous);
bool proto_test_and_clear(proto_word_t *words, size_t bit_count, size_t index,
                          bool *previous);
bool proto_any(const proto_word_t *words, size_t bit_count, bool *any);
bool proto_count(const proto_word_t *words, size_t bit_count, size_t *count);
bool proto_scan(const proto_word_t *words, size_t bit_count, size_t start,
                size_t *index);
bool proto_clear_all(proto_word_t *words, size_t bit_count);
bool proto_fill_all(proto_word_t *words, size_t bit_count);
bool proto_snapshot(const proto_word_t *words, size_t bit_count,
                    proto_word_t *dest, size_t dest_words);

#endif /* BENCH_PROTOTYPE_H_ */
