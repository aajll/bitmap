#include "test_harness.h"

#include <bitmap.h>
#include <stddef.h>

/* The sizing macros must be usable in constant expressions. */
_Static_assert(BITMAP_WORDS_FOR_BITS(0) == (size_t)0, "0 bits -> 0 words");
_Static_assert(BITMAP_WORDS_FOR_BITS(1) == (size_t)1, "1 bit -> 1 word");
_Static_assert(BITMAP_WORDS_FOR_BITS(15) == (size_t)1, "15 bits -> 1 word");
_Static_assert(BITMAP_WORDS_FOR_BITS(16) == (size_t)1, "16 bits -> 1 word");
_Static_assert(BITMAP_WORDS_FOR_BITS(17) == (size_t)2, "17 bits -> 2 words");
_Static_assert(BITMAP_WORDS_FOR_BITS(192) == (size_t)12,
               "192 bits -> 12 words");
_Static_assert(BITMAP_WORDS_FOR_BITS(SIZE_MAX)
                   == ((SIZE_MAX / (size_t)16) + (size_t)1),
               "SIZE_MAX does not wrap");

_Static_assert(BITMAP_STORAGE_WORDS(0) == (size_t)1,
               "zero-bit array is 1 word");
_Static_assert(BITMAP_STORAGE_WORDS(1) == (size_t)1, "1 bit array is 1 word");
_Static_assert(BITMAP_STORAGE_WORDS(17) == (size_t)2,
               "17 bit array is 2 words");

void
test_sizing(void)
{
        static const struct {
                size_t bits;
                size_t words;
        } CASES[] = {
            {0u, 0u},  {1u, 1u},    {15u, 1u},   {16u, 1u}, {17u, 2u},
            {23u, 2u}, {31u, 2u},   {32u, 2u},   {33u, 3u}, {48u, 3u},
            {49u, 4u}, {192u, 12u}, {193u, 13u},
        };
        bitmap_word_t array[7];

        bm_suite("sizing");

        for (size_t i = 0u; i < (sizeof CASES / sizeof CASES[0]); i++) {
                BM_CHECK(bitmap_words_for_bits(CASES[i].bits)
                         == CASES[i].words);
                BM_CHECK(BITMAP_WORDS_FOR_BITS(CASES[i].bits)
                         == CASES[i].words);
        }

        BM_CHECK(bitmap_words_for_bits(SIZE_MAX)
                 == ((SIZE_MAX / (size_t)16) + (size_t)1));
        BM_CHECK(bitmap_words_for_bits(SIZE_MAX - (size_t)1)
                 == (SIZE_MAX / (size_t)16) + (size_t)1);
        BM_CHECK(BITMAP_ARRAY_WORDS(array) == (size_t)7);
        BM_CHECK(BITMAP_BITS_PER_WORD == 16u);
        BM_CHECK(BITMAP_STORAGE_WORDS(0) == (size_t)1);
}
