/* Minimal linked consumer: measures retained code for the descriptor API. */

#include <bitmap.h>

#include <stdbool.h>
#include <stddef.h>

volatile unsigned int bench_sink;

int
main(void)
{
        static bitmap_word_t storage[BITMAP_STORAGE_WORDS(192u)];
        bitmap_t map;
        bool value = false;
        size_t count = 0u;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage), 192u)) {
                return 1;
        }
        (void)bitmap_set(&map, 5u);
        (void)bitmap_test(&map, 5u, &value);
        (void)bitmap_count(&map, &count);
        bench_sink = (unsigned int)count + (value ? 1u : 0u);
        return 0;
}
