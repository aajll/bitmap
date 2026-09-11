/*
 * Coexistence integration fixture. Links the bitmap library and a second
 * library in one program. Neither library is modified: the second library is
 * compiled as-is from its own checkout with its own public headers.
 *
 * Driven by tests/integration/test_integration.py (mode "coexistence"), which
 * defines BITMAP_COEXISTENCE_STATUS and skips when that checkout is absent. The
 * fallback main keeps this source valid for tools in a standalone checkout.
 */

#include <bitmap.h>

#if defined(BITMAP_COEXISTENCE_STATUS)

#include <status.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define FAULT_ID STATUS_ENCODE(0u, 1u)

int
main(void)
{
        bitmap_word_t storage[BITMAP_STORAGE_WORDS(64u)];
        bitmap_t map;
        status_reg_t reg;
        bool value = false;
        size_t count = 99u;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage), 64u)) {
                return 1;
        }
        if (!bitmap_set(&map, 1u) || !bitmap_set(&map, 63u)) {
                return 1;
        }
        if (!bitmap_test(&map, 1u, &value) || !value) {
                return 1;
        }
        if (!bitmap_count(&map, &count) || (count != 2u)) {
                return 1;
        }

        status_reg_init(&reg);
        status_reg_set_fault(&reg, FAULT_ID);
        if (!status_reg_is_fault_set(&reg, FAULT_ID)) {
                return 1;
        }
        if (!status_reg_any(&reg, STATUS_CLASS_FAULT)) {
                return 1;
        }
        if (status_reg_last_fault(&reg) != FAULT_ID) {
                return 1;
        }

        if (!bitmap_count(&map, &count) || (count != 2u)) {
                return 1;
        }
        (void)printf("coexistence ok\n");
        return 0;
}

#else

int
main(void)
{
        return 0;
}

#endif
