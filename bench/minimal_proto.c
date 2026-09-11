/* Minimal linked consumer: same operations, explicit-capacity prototype. */

#include "prototype.h"

#include <stdbool.h>
#include <stddef.h>

volatile unsigned int bench_sink;

int
main(void)
{
        static proto_word_t storage[12u];
        bool value = false;
        size_t count = 0u;

        (void)proto_clear_all(storage, 192u);
        (void)proto_set(storage, 192u, 5u);
        (void)proto_test(storage, 192u, 5u, &value);
        (void)proto_count(storage, 192u, &count);
        bench_sink = (unsigned int)count + (value ? 1u : 0u);
        return 0;
}
