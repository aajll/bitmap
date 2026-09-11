/**
 * SPDX-License-Identifier: MIT
 *
 * @file bitmap.h
 *
 * @brief
 *    Capacity-independent, caller-owned bitmap sets for embedded C.
 *
 * @details
 *    A @c bitmap_t is a non-owning view over a caller-provided array of
 *    @c bitmap_word_t words plus a logical bit count. The descriptor layout
 *    does not depend on the bit count, so a single compiled library serves
 *    bitmaps of any capacity. There is no capacity macro, no per-capacity
 *    linker symbol, and no global register layout to keep in step between
 *    separately compiled callers.
 *
 *    ## Storage model
 *    - Callers own the backing words. Static or automatic arrays are the
 *      intended form; the library never allocates, resizes, or frees.
 *    - A descriptor aliases its backing words. Copying a descriptor copies
 *      the view, not the bitmap contents.
 *    - Backing storage must outlive every descriptor and operation that
 *      refers to it.
 *    - Bit zero is the least significant bit of word zero. Word @c n covers
 *      bit indices @c 16n .. @c 16n+15.
 *    - The final word may contain unused high bits. Queries, counts, scans,
 *      and snapshots ignore them. @c bitmap_init, @c bitmap_fill_all, and
 *      @c bitmap_snapshot write zeroes into them.
 *    - Valid bit indices satisfy @c index @c < @c bit_count. A logic error
 *      such as a bad index is not a state transition: the failing call
 *      returns @c false and leaves storage and output parameters unchanged.
 *    - An empty bitmap (@c bit_count @c == @c 0) has no storage, no set
 *      bits, a count of zero, and an empty scan. @c bitmap_init canonicalizes
 *      it to a null word pointer. @c BITMAP_INITIALIZER keeps the pointer it
 *      was given, but no operation reads or writes through it.
 *    - Only @c bitmap_init writes a descriptor. Every other operation takes
 *      it as @c const, so a descriptor built with @c BITMAP_INITIALIZER can
 *      be a @c const object. A target built without position-independent
 *      code can then keep it in read-only memory, leaving only the backing
 *      words in RAM.
 *
 *    ## Error model
 *    Public functions return @c bool. A @c true result means the request was
 *    valid and any output parameter has been written. Queries never fold an
 *    invalid argument into a bit value: @c bitmap_test and @c bitmap_any
 *    report success separately from the value, and the test-and-modify pair
 *    reports success separately from the previous value. No @c errno, no
 *    exceptions, no callbacks, and no logging.
 *
 *    The library cannot discover the real size of an arbitrary pointer. All
 *    pointer arguments are checked for @c NULL, but a caller that forges a
 *    descriptor, or passes storage shorter than it declared to
 *    @c bitmap_init, is invoking undefined behaviour.
 *
 *    ## Concurrency
 *    The portable baseline operates on ordinary @c uint16_t words and
 *    requires caller serialization for every conflicting access. That
 *    includes two writers touching different bits of the same word, and a
 *    reader running concurrently with any writer. @c volatile does not
 *    repair these races, and @c bitmap_test_and_set is not atomic merely
 *    because of its name: the caller must protect the whole call.
 *
 *    This header deliberately owns no interrupt, RTOS, or DMA policy. Where
 *    a consumer needs lock-free atomic bit operations, that is a future
 *    extension with its own storage and alignment contract, not a silent
 *    property of these functions.
 *
 *    ## Portability
 *    Requires exact 16-bit @c uint16_t support and C11. A C byte need not be
 *    an octet; capacities are expressed in words and logical capacities in
 *    bits, and no in-memory object size is treated as a wire length. Shift
 *    operands are unsigned and shift counts stay in the range 0 through 15,
 *    so the expressions are defined even when @c int is 16 bits wide.
 */

#ifndef BITMAP_H_
#define BITMAP_H_

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef UINT16_MAX
#error "bitmap requires an exact 16-bit unsigned type (uint16_t)"
#endif

/* ================ TYPES =================================================== */

/**
 * @brief The storage unit of a bitmap.
 *
 * A bitmap is a packed array of these words, least significant bit first.
 */
typedef uint16_t bitmap_word_t;

/**
 * @def BITMAP_BITS_PER_WORD
 * @brief Number of logical bits carried by one @c bitmap_word_t.
 */
#define BITMAP_BITS_PER_WORD (16u)

/**
 * @brief Non-owning view over caller-provided bitmap storage.
 *
 * @details
 *    The layout is independent of @c bit_count. Two descriptors in the same
 *    translation unit, or in separately compiled translation units, may
 *    describe different capacities without any build configuration. The
 *    fields are public so callers can size snapshots and inspect capacity;
 *    do not modify @c words or @c bit_count while another context is using
 *    the bitmap.
 *
 *    @c bitmap_init sets @c words to @c NULL when the bitmap is empty.
 *    Operations other than @c bitmap_init never write the descriptor, so it
 *    may be defined @c const with @c BITMAP_INITIALIZER.
 */
typedef struct {
        bitmap_word_t *words; /**< Backing words, or NULL when empty.        */
        size_t bit_count;     /**< Logical capacity in bits.                 */
} bitmap_t;

#if defined(__cplusplus)
static_assert((sizeof(bitmap_word_t) * (size_t)CHAR_BIT)
                  == (size_t)BITMAP_BITS_PER_WORD,
              "bitmap_word_t must be exactly 16 bits wide");
#else
_Static_assert((sizeof(bitmap_word_t) * (size_t)CHAR_BIT)
                   == (size_t)BITMAP_BITS_PER_WORD,
               "bitmap_word_t must be exactly 16 bits wide");
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup bitmap_api bitmap Library
 * @brief Capacity-independent, caller-owned bitmap sets.
 * @{
 */

/**
 * @defgroup bitmap_helpers Compile-time and runtime sizing helpers
 * @{
 */

/**
 * @brief Word count needed to hold @p bits logical bits.
 *
 * @warning The argument is evaluated more than once and must be a constant
 *          expression without side effects. Use @c bitmap_words_for_bits for
 *          a runtime value.
 */
#define BITMAP_WORDS_FOR_BITS(bits)                                            \
        (((size_t)(bits) / (size_t)BITMAP_BITS_PER_WORD)                       \
         + ((((size_t)(bits) % (size_t)BITMAP_BITS_PER_WORD) != (size_t)0)     \
                ? (size_t)1                                                    \
                : (size_t)0))

/**
 * @brief Word count for a backing array declaration, never zero.
 *
 * @details
 *    Identical to @c BITMAP_WORDS_FOR_BITS except that a zero-bit request
 *    yields one word so that a declared array is never zero-length.
 *
 * @warning The argument is evaluated more than once and must be a constant
 *          expression without side effects.
 */
#define BITMAP_STORAGE_WORDS(bits)                                             \
        ((BITMAP_WORDS_FOR_BITS(bits) != (size_t)0)                            \
             ? BITMAP_WORDS_FOR_BITS(bits)                                     \
             : (size_t)1)

/**
 * @brief Number of words in a backing array object.
 *
 * @details
 *    Use this to pass the array's own capacity to @c bitmap_init so the
 *    declared array and the reported capacity cannot disagree.
 */
#define BITMAP_ARRAY_WORDS(array) (sizeof(array) / sizeof((array)[0]))

/**
 * @brief Compile-time initializer that binds a descriptor to an array.
 *
 * @details
 *    Expands to a braced initializer for a @c bitmap_t over @p array with
 *    @p bits logical bits, with no call to @c bitmap_init. Use it to define
 *    a @c const descriptor, or a table of them:
 *    @code
 *    static bitmap_word_t fault_words[BITMAP_STORAGE_WORDS(192u)];
 *    static const bitmap_t faults = BITMAP_INITIALIZER(fault_words, 192u);
 *    @endcode
 *
 *    Compilation fails when @p array has fewer words than
 *    @c BITMAP_WORDS_FOR_BITS(@p bits). C11 has no static assertion that
 *    can appear inside an expression, so the check is a zero-valued term
 *    whose array size becomes negative when the storage is too small.
 *
 *    The initializer does not clear the words and does not canonicalize an
 *    empty bitmap. Objects with static storage duration start zeroed. Clear
 *    automatic storage with @c bitmap_clear_all before the first query.
 *
 * @warning @p array must name an array object, not a pointer, because its
 *          capacity comes from @c BITMAP_ARRAY_WORDS. @p bits must be an
 *          integer constant expression without side effects.
 */
#define BITMAP_INITIALIZER(array, bits)                                        \
        {(array), ((size_t)(bits)                                              \
                   + ((size_t)0                                                \
                      * sizeof(char[(BITMAP_ARRAY_WORDS(array)                 \
                                     >= BITMAP_WORDS_FOR_BITS(bits))           \
                                        ? 1                                    \
                                        : -1])))}

/** @} */

/* ================ CAPACITY ================================================ */

/**
 * @brief Number of words required to hold @p bit_count bits.
 *
 * @details
 *    Ceiling division that evaluates @p bit_count once and cannot overflow
 *    near @c SIZE_MAX (no @c bit_count @c + @c 15 intermediate).
 *
 * @param bit_count Logical capacity in bits.
 * @return Required word count; zero when @p bit_count is zero.
 */
size_t bitmap_words_for_bits(size_t bit_count);

/* ================ LIFECYCLE =============================================== */

/**
 * @brief Bind a descriptor to caller-owned storage.
 *
 * @details
 *    Validates the request, clears exactly the required words, and publishes
 *    the descriptor. On success, words beyond @c bitmap_words_for_bits
 *    (@p bit_count) are left untouched, as is all storage when @p bit_count
 *    is zero.
 *
 *    A zero-bit bitmap accepts a @c NULL storage pointer only with a zero word
 *    count, and is canonicalized to @c words @c == @c NULL with
 *    @c bit_count @c == @c 0. A non-@c NULL storage pointer with a zero bit
 *    count is accepted and ignored, regardless of @p storage_words.
 *
 *    On failure the descriptor and the storage are unchanged. Failure means:
 *    @p map is @c NULL; @p storage is @c NULL while @p storage_words is
 *    non-zero; @p bit_count is non-zero and @p storage is @c NULL; or
 *    @p storage_words is smaller than @c bitmap_words_for_bits
 *    (@p bit_count).
 *
 *    Initialization and reinitialization require exclusive access. @p storage
 *    must be aligned for @c bitmap_word_t and must not overlap @p map.
 *
 * @param map           Descriptor to initialize. Must not be @c NULL.
 * @param storage       Backing words, or @c NULL for an empty bitmap.
 * @param storage_words Capacity of @p storage in words.
 * @param bit_count     Logical capacity in bits.
 * @return @c true on success, @c false when the request is invalid.
 */
bool bitmap_init(bitmap_t *map, bitmap_word_t *storage, size_t storage_words,
                 size_t bit_count);

/* ================ WHOLE-MAP OPERATIONS ==================================== */

/**
 * @brief Clear every logical bit, zeroing all required words.
 *
 * @param map Bitmap to clear.
 * @return @c true on success, @c false when @p map is invalid.
 */
bool bitmap_clear_all(const bitmap_t *map);

/**
 * @brief Set every logical bit, leaving unused high bits zero.
 *
 * @param map Bitmap to fill.
 * @return @c true on success, @c false when @p map is invalid.
 */
bool bitmap_fill_all(const bitmap_t *map);

/* ================ SINGLE-BIT OPERATIONS =================================== */

/**
 * @brief Set one logical bit. Other bits are unaffected.
 *
 * @param map   Bitmap to modify.
 * @param index Bit index; must be less than @c map->bit_count.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_set(const bitmap_t *map, size_t index);

/**
 * @brief Clear one logical bit. Other bits are unaffected.
 *
 * @param map   Bitmap to modify.
 * @param index Bit index; must be less than @c map->bit_count.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_clear(const bitmap_t *map, size_t index);

/**
 * @brief Read one logical bit.
 *
 * @param map   Descriptor to read.
 * @param index Bit index; must be less than @c map->bit_count.
 * @param value Receives the bit value. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument. A successful
 *         read of a cleared bit returns @c true with @c *value @c == @c false.
 */
bool bitmap_test(const bitmap_t *map, size_t index, bool *value);

/**
 * @brief Read one logical bit, then set it. Not atomic; see the concurrency
 *        contract above.
 *
 * @param map      Bitmap to modify.
 * @param index    Bit index; must be less than @c map->bit_count.
 * @param previous Receives the value before the update. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_test_and_set(const bitmap_t *map, size_t index, bool *previous);

/**
 * @brief Read one logical bit, then clear it. Not atomic; see the concurrency
 *        contract above.
 *
 * @param map      Bitmap to modify.
 * @param index    Bit index; must be less than @c map->bit_count.
 * @param previous Receives the value before the update. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_test_and_clear(const bitmap_t *map, size_t index, bool *previous);

/* ================ AGGREGATE QUERIES ======================================= */

/**
 * @brief Report whether any logical bit is set.
 *
 * @param map Descriptor to inspect.
 * @param any Receives @c true when at least one bit is set, @c false for an
 *            empty or all-clear bitmap. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_any(const bitmap_t *map, bool *any);

/**
 * @brief Count the set bits.
 *
 * @param map   Descriptor to inspect.
 * @param count Receives the population count. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_count(const bitmap_t *map, size_t *count);

/**
 * @brief Find the lowest set bit at or after @p start.
 *
 * @details
 *    The return value reports request validity, not discovery. A valid
 *    request succeeds even when nothing is found, in which case @c *index is
 *    set to @c map->bit_count (the end sentinel). Callers therefore
 *    distinguish "no result" by comparing @c *index with
 *    @c map->bit_count. Iteration is:
 *    @code
 *    size_t at = 0u;
 *    while (bitmap_scan(&map, at, &at) && (at < map.bit_count)) {
 *            ... use at ...
 *            at++;
 *    }
 *    @endcode
 *
 * @param map   Descriptor to search.
 * @param start First index to consider; must be less than or equal to
 *              @c map->bit_count.
 * @param index Receives the found index, or @c map->bit_count when none is
 *              found. Unchanged on failure.
 * @return @c true on success, @c false on an invalid argument.
 */
bool bitmap_scan(const bitmap_t *map, size_t start, size_t *index);

/* ================ SNAPSHOT ================================================ */

/**
 * @brief Copy the bitmap into caller-owned word storage.
 *
 * @details
 *    Writes exactly @c bitmap_words_for_bits (@c map->bit_count) words and
 *    zeroes the unused high bits of the final copied word. Words of @p dest
 *    beyond that count are left untouched. A destination shorter than the
 *    required word count fails without writing anything.
 *
 *    @p dest must not overlap @c map->words; v0.1.0 defines only the disjoint
 *    case.
 *
 * @param map        Descriptor to copy.
 * @param dest       Destination words. Must not be @c NULL.
 * @param dest_words Capacity of @p dest in words.
 * @return @c true on success, @c false on an invalid argument or an
 *         undersized destination.
 */
bool bitmap_snapshot(const bitmap_t *map, bitmap_word_t *dest,
                     size_t dest_words);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* BITMAP_H_ */
