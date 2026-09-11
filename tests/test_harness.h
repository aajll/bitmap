#ifndef BITMAP_TEST_HARNESS_H_
#define BITMAP_TEST_HARNESS_H_

#include <stdbool.h>

/** Select the suite name printed with subsequent failures. */
void bm_suite(const char *name);

/** Record one check. */
void bm_check(bool condition, const char *expression, const char *file,
              int line);

/** Print the totals and return a process exit code. */
int bm_report(void);

#define BM_CHECK(condition)                                                    \
        bm_check((condition), #condition, __FILE__, __LINE__)

#endif /* BITMAP_TEST_HARNESS_H_ */
