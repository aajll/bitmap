/*
 * Compile-time capacity check for BITMAP_INITIALIZER. The copy-in test
 * compiles this file twice: with enough words it must build, and with one
 * word too few it must fail to compile.
 */

#include <bitmap.h>

#define FIXTURE_BITS 33u

#ifndef BITMAP_FIXTURE_WORDS
#define BITMAP_FIXTURE_WORDS BITMAP_WORDS_FOR_BITS(FIXTURE_BITS)
#endif

static bitmap_word_t fixture_words[BITMAP_FIXTURE_WORDS];

const bitmap_t fixture_map = BITMAP_INITIALIZER(fixture_words, FIXTURE_BITS);
