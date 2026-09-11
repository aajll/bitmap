#include "test_harness.h"

#include <stdio.h>

static unsigned long bm_checks;
static unsigned long bm_failures;
static const char *bm_current = "(none)";

void
bm_suite(const char *name)
{
        bm_current = name;
        (void)printf("\n== %s ==\n", name);
}

void
bm_check(bool condition, const char *expression, const char *file, int line)
{
        bm_checks++;
        if (!condition) {
                bm_failures++;
                (void)printf("FAIL [%s] %s:%d: %s\n", bm_current, file, line,
                             expression);
        }
}

int
bm_report(void)
{
        (void)printf("\n%lu checks, %lu failures\n", bm_checks, bm_failures);
        return (bm_failures == 0UL) ? 0 : 1;
}
