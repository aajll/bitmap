/* Host operation-cost probe. Results are evidence, not target WCET. */

#define _POSIX_C_SOURCE 200809L

#include <bitmap.h>

#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BENCH_BITS       192u
#define BENCH_ITERATIONS 2000000u
#define BENCH_SAMPLES    5u

volatile size_t bitmap_bench_sink;

static uint64_t
now_ns(void)
{
        struct timespec value;

        if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
                perror("clock_gettime");
                exit(EXIT_FAILURE);
        }
        return ((uint64_t)value.tv_sec * UINT64_C(1000000000))
               + (uint64_t)value.tv_nsec;
}

static void
report(const char *name, uint64_t elapsed)
{
        (void)printf("%-24s %9.2f ns/op\n", name,
                     (double)elapsed / (double)BENCH_ITERATIONS);
}

#define MEASURE(name, ...)                                                     \
        do {                                                                   \
                uint64_t measure_best = UINT64_MAX;                            \
                for (size_t measure_sample = 0u;                               \
                     measure_sample < (size_t)BENCH_SAMPLES;                   \
                     measure_sample++) {                                       \
                        uint64_t measure_start = now_ns();                     \
                        for (size_t measure_i = 0u;                            \
                             measure_i < (size_t)BENCH_ITERATIONS;             \
                             measure_i++) {                                    \
                                __VA_ARGS__;                                   \
                        }                                                      \
                        uint64_t measure_elapsed = now_ns() - measure_start;   \
                        if (measure_elapsed < measure_best) {                  \
                                measure_best = measure_elapsed;                \
                        }                                                      \
                }                                                              \
                report((name), measure_best);                                  \
        } while (false)

int
main(void)
{
        bitmap_word_t storage[BITMAP_STORAGE_WORDS(BENCH_BITS)];
        bitmap_word_t snapshot[BITMAP_STORAGE_WORDS(BENCH_BITS)];
        bitmap_t map;
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        bool value = false;
        bool ok = true;
        size_t count = 0u;
        size_t index = 0u;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage),
                         BENCH_BITS)) {
                return EXIT_FAILURE;
        }

        (void)puts("capacity: 192 bits (12 words)");
        (void)printf("iterations: %u; samples: %u; reported: minimum\n",
                     BENCH_ITERATIONS, BENCH_SAMPLES);

        MEASURE("init",
                ok = bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage),
                                 BENCH_BITS)
                     && ok);
        MEASURE("clear_all", ok = bitmap_clear_all(&map) && ok);
        MEASURE("fill_all", ok = bitmap_fill_all(&map) && ok);

        (void)bitmap_clear_all(&map);
        MEASURE("set", ok = bitmap_set(&map, 95u) && ok);

        (void)bitmap_fill_all(&map);
        MEASURE("clear", ok = bitmap_clear(&map, 95u) && ok);

        (void)bitmap_set(&map, 95u);
        MEASURE("test", ok = bitmap_test(&map, 95u, &value) && ok);
        MEASURE("test_and_set",
                ok = bitmap_test_and_set(&map, 95u, &value) && ok);
        MEASURE("test_and_clear",
                ok = bitmap_test_and_clear(&map, 95u, &value) && ok);

        (void)bitmap_fill_all(&map);
        MEASURE("any", ok = bitmap_any(&map, &value) && ok);
        MEASURE("count", ok = bitmap_count(&map, &count) && ok);

        (void)bitmap_clear_all(&map);
        (void)bitmap_set(&map, BENCH_BITS - 1u);
        MEASURE("scan to final word", ok = bitmap_scan(&map, 0u, &index) && ok);
        MEASURE("snapshot", ok = bitmap_snapshot(&map, snapshot,
                                                 BITMAP_ARRAY_WORDS(snapshot))
                                 && ok);

        MEASURE("mutex lock/unlock",
                ok = (pthread_mutex_lock(&lock) == 0) && ok,
                ok = (pthread_mutex_unlock(&lock) == 0) && ok);
        MEASURE("mutex + set", ok = (pthread_mutex_lock(&lock) == 0) && ok,
                ok = bitmap_set(&map, 95u) && ok,
                ok = (pthread_mutex_unlock(&lock) == 0) && ok);

        bitmap_bench_sink =
            count + index + (value ? 1u : 0u) + (ok ? 1u : 0u) + snapshot[0];
        if (pthread_mutex_destroy(&lock) != 0) {
                return EXIT_FAILURE;
        }
        return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
