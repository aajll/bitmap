/*
 * Caller-serialized threaded example. The portable bitmap contract requires
 * external serialization for conflicting access, including two writers that
 * touch different bits of the same word. Every worker mutation runs under one
 * mutex; the main thread queries only after all workers join.
 *
 * Worker threads do not touch the check harness (its counters are not
 * synchronized); only the main thread records checks.
 */

#include "test_harness.h"

#include <bitmap.h>
#include <pthread.h>
#include <stddef.h>

#define TOTAL_BITS  256u
#define WORKERS     4u
#define REGION_BITS (TOTAL_BITS / WORKERS)

typedef struct {
        bitmap_t *map;
        pthread_mutex_t *lock;
        size_t first;
} worker_t;

static void *
set_region(void *arg)
{
        worker_t *worker = arg;

        for (size_t i = 0u; i < REGION_BITS; i++) {
                bool changed;
                int unlock_result;

                if (pthread_mutex_lock(worker->lock) != 0) {
                        return worker;
                }
                changed = bitmap_set(worker->map, worker->first + i);
                unlock_result = pthread_mutex_unlock(worker->lock);
                if (!changed || (unlock_result != 0)) {
                        return worker;
                }
        }
        return NULL;
}

static void *
clear_region(void *arg)
{
        worker_t *worker = arg;

        for (size_t i = 0u; i < REGION_BITS; i++) {
                bool changed;
                int unlock_result;

                if (pthread_mutex_lock(worker->lock) != 0) {
                        return worker;
                }
                changed = bitmap_clear(worker->map, worker->first + i);
                unlock_result = pthread_mutex_unlock(worker->lock);
                if (!changed || (unlock_result != 0)) {
                        return worker;
                }
        }
        return NULL;
}

typedef void *(*worker_fn_t)(void *);

static bool
run_workers(pthread_t *threads, worker_t *workers, worker_fn_t function)
{
        size_t created = 0u;
        bool ok = true;

        for (size_t i = 0u; i < WORKERS; i++) {
                int result =
                    pthread_create(&threads[i], NULL, function, &workers[i]);

                BM_CHECK(result == 0);
                if (result != 0) {
                        ok = false;
                        break;
                }
                created++;
        }

        for (size_t i = 0u; i < created; i++) {
                void *worker_result = NULL;
                int result = pthread_join(threads[i], &worker_result);

                BM_CHECK(result == 0);
                if (result != 0) {
                        ok = false;
                } else {
                        BM_CHECK(worker_result == NULL);
                        if (worker_result != NULL) {
                                ok = false;
                        }
                }
        }
        return ok;
}

int
main(void)
{
        bitmap_word_t storage[BITMAP_STORAGE_WORDS(TOTAL_BITS)];
        bitmap_t map;
        pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
        pthread_t threads[WORKERS];
        worker_t workers[WORKERS];
        bool any = false;
        size_t count = 99u;
        size_t at = 0u;
        bool ready;
        bool workers_ok;

        bm_suite("caller-serialized threads");

        ready =
            bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage), TOTAL_BITS);
        BM_CHECK(ready);
        if (!ready) {
                BM_CHECK(pthread_mutex_destroy(&lock) == 0);
                return bm_report();
        }

        for (size_t i = 0u; i < WORKERS; i++) {
                workers[i].map = &map;
                workers[i].lock = &lock;
                workers[i].first = i * REGION_BITS;
        }

        workers_ok = run_workers(threads, workers, set_region);
        if (workers_ok) {
                BM_CHECK(bitmap_count(&map, &count));
                BM_CHECK(count == TOTAL_BITS);
                BM_CHECK(bitmap_any(&map, &any));
                BM_CHECK(any);
                BM_CHECK(bitmap_scan(&map, 0u, &at));
                BM_CHECK(at == 0u);
                BM_CHECK(bitmap_scan(&map, TOTAL_BITS - 1u, &at));
                BM_CHECK(at == (TOTAL_BITS - 1u));

                workers_ok = run_workers(threads, workers, clear_region);
        }

        if (workers_ok) {
                BM_CHECK(bitmap_count(&map, &count));
                BM_CHECK(count == 0u);
                BM_CHECK(bitmap_any(&map, &any));
                BM_CHECK(!any);
                BM_CHECK(bitmap_scan(&map, 0u, &at));
                BM_CHECK(at == TOTAL_BITS);
        }

        BM_CHECK(pthread_mutex_destroy(&lock) == 0);
        return bm_report();
}
