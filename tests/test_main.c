#include "test_harness.h"

void test_sizing(void);
void test_init(void);
void test_bits(void);
void test_bulk(void);
void test_snapshot(void);
void test_boundaries(void);
void test_random(void);
void test_instances(void);
void test_static(void);

int
main(void)
{
        test_sizing();
        test_init();
        test_bits();
        test_bulk();
        test_snapshot();
        test_boundaries();
        test_random();
        test_instances();
        test_static();
        return bm_report();
}
