/* Prints descriptor layout facts for docs/design.md. */

#include <bitmap.h>

#include <limits.h>
#include <stddef.h>
#include <stdio.h>

int
main(void)
{
        (void)printf("CHAR_BIT              = %d\n", CHAR_BIT);
        (void)printf(
            "sizeof(bitmap_word_t) = %zu addressable units (%zu bits)\n",
            sizeof(bitmap_word_t), sizeof(bitmap_word_t) * (size_t)CHAR_BIT);
        (void)printf(
            "sizeof(bitmap_t)      = %zu addressable units (%zu bits)\n",
            sizeof(bitmap_t), sizeof(bitmap_t) * (size_t)CHAR_BIT);
        (void)printf("offsetof(words)       = %zu\n",
                     offsetof(bitmap_t, words));
        (void)printf("offsetof(bit_count)   = %zu\n",
                     offsetof(bitmap_t, bit_count));
        return 0;
}
