// C++17 portable-interface smoke test: bitmap.h must compile and link from
// C++ with no special configuration.

#include <bitmap.h>

#include <cstddef>

static bitmap_word_t fixed_storage[BITMAP_STORAGE_WORDS(20)];
static const bitmap_t fixed = BITMAP_INITIALIZER(fixed_storage, 20u);

int
main()
{
        bitmap_word_t storage[BITMAP_STORAGE_WORDS(37)];
        bitmap_t map{};
        bool value = false;
        std::size_t count = 0;

        if (!bitmap_init(&map, storage, BITMAP_ARRAY_WORDS(storage), 37u)) {
                return 1;
        }
        if (!bitmap_set(&map, 36u)) {
                return 1;
        }
        if (!bitmap_test(&map, 36u, &value) || !value) {
                return 1;
        }
        if (!bitmap_count(&map, &count) || (count != 1u)) {
                return 1;
        }
        if (!bitmap_test_and_clear(&map, 36u, &value) || !value) {
                return 1;
        }
        if (!bitmap_count(&map, &count) || (count != 0u)) {
                return 1;
        }
        if (!bitmap_set(&fixed, 19u) || !bitmap_test(&fixed, 19u, &value)
            || !value) {
                return 1;
        }
        return 0;
}
