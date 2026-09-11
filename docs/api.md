# API reference

Every declaration lives in [`include/bitmap.h`](../include/bitmap.h), which also carries the full Doxygen contract. The reasons behind these rules are in [design.md](design.md).

## Conventions

- Functions return `bool`. `true` means the request was valid and any output parameter was written. `false` means an argument was invalid, and storage and output parameters are unchanged.
- Queries keep validity separate from values, so a clear bit never looks like a failed call.
- Bit zero is the least significant bit of word zero. Word `n` holds indices `16n` to `16n + 15`. A valid index satisfies `index < bit_count`.
- The final word can hold unused high bits. Queries, counts, scans, and snapshots ignore them. `bitmap_init`, `bitmap_fill_all`, and `bitmap_snapshot` write zeroes into them.
- Every pointer argument is checked for `NULL`. The library cannot check the real length of an array behind a pointer, so a forged descriptor or a misdeclared capacity is caller misuse.
- There is no `errno`, no exception, no callback, and no logging.

## Types

<!-- snippet: skip -->
```c
typedef uint16_t bitmap_word_t;
#define BITMAP_BITS_PER_WORD (16u)

typedef struct {
        bitmap_word_t *words;
        size_t bit_count;
} bitmap_t;
```

A `bitmap_t` is a view of caller-owned words. Copying it copies the view, not the bits. The storage must outlive every descriptor that refers to it, and the fields must not change while another context uses the bitmap. Only `bitmap_init` writes a descriptor; every other function takes `const bitmap_t *`.

## Sizing helpers

<!-- snippet: skip -->
```c
size_t bitmap_words_for_bits(size_t bit_count);

BITMAP_WORDS_FOR_BITS(bits)   /* exact word count, constant expression     */
BITMAP_STORAGE_WORDS(bits)    /* at least one word, for array declarations */
BITMAP_ARRAY_WORDS(array)     /* word count of an array object             */
```

`bitmap_words_for_bits` evaluates its argument once and cannot overflow near `SIZE_MAX`. The macros evaluate their arguments more than once, so use them only with side-effect-free constants. `BITMAP_STORAGE_WORDS` never yields zero, so a declared array is never zero-length.

## Creating a bitmap

### Compile-time descriptor

<!-- snippet: skip -->
```c
BITMAP_INITIALIZER(array, bits)
```

Expands to a braced `bitmap_t` initialiser. Compilation fails when `array` holds fewer words than `bits` needs. `array` must name an array object, not a pointer.

```c
static bitmap_word_t fault_words[BITMAP_STORAGE_WORDS(192u)];
static const bitmap_t faults = BITMAP_INITIALIZER(fault_words, 192u);
```

The initialiser does not clear the words. Static storage starts zeroed; clear automatic storage with `bitmap_clear_all` before the first query. A zero-bit descriptor keeps its array pointer, but no function reads or writes through it. On a target built without position-independent code, a `const` descriptor lives in read-only memory and only the words use RAM.

### Runtime initialisation

```c
bool bitmap_init(bitmap_t *map, bitmap_word_t *storage, size_t storage_words,
                 size_t bit_count);
```

Validates the request before it writes anything. On success it clears exactly the required words, leaves later words untouched, and publishes the descriptor. On failure the descriptor and the storage are unchanged.

- It fails when `map` is `NULL`, when `storage` is `NULL` with a non-zero `storage_words` or `bit_count`, or when `storage_words` is less than `bitmap_words_for_bits(bit_count)`.
- A zero-bit request accepts `(NULL, 0)` or any non-null storage, and sets `words` to `NULL`.
- Initialisation and reinitialisation need exclusive access to the descriptor and the storage.

## Whole-map operations

```c
bool bitmap_clear_all(const bitmap_t *map);
bool bitmap_fill_all(const bitmap_t *map);
```

`bitmap_clear_all` zeroes every required word. `bitmap_fill_all` sets every logical bit and leaves the unused tail bits zero.

## Single-bit operations

```c
bool bitmap_set(const bitmap_t *map, size_t index);
bool bitmap_clear(const bitmap_t *map, size_t index);
bool bitmap_test(const bitmap_t *map, size_t index, bool *value);
bool bitmap_test_and_set(const bitmap_t *map, size_t index, bool *previous);
bool bitmap_test_and_clear(const bitmap_t *map, size_t index,
                           bool *previous);
```

Each call reads or changes only the requested bit. `bitmap_test` writes the bit value. The test-and-modify pair write the value from before the update. They are read-modify-write sequences, not atomic operations.

## Aggregate queries

```c
bool bitmap_any(const bitmap_t *map, bool *any);
bool bitmap_count(const bitmap_t *map, size_t *count);
bool bitmap_scan(const bitmap_t *map, size_t start, size_t *index);
```

`bitmap_any` stops at the first set word. `bitmap_count` writes the population count. `bitmap_scan` writes the lowest set index at or after `start`. It reports request validity, not discovery: when nothing is found it succeeds and writes `bit_count`. `start == bit_count` is valid and finds nothing, and `start > bit_count` fails.

Iterate over the set bits like this:

<!-- snippet: body -->
```c
size_t at = 0u;

while (bitmap_scan(&faults, at, &at) && (at < faults.bit_count)) {
        /* at is the next set position */
        at++;
}
```

## Snapshot

```c
bool bitmap_snapshot(const bitmap_t *map, bitmap_word_t *dest,
                     size_t dest_words);
```

Copies `bitmap_words_for_bits(bit_count)` words into `dest` and zeroes the unused tail bits of the last copied word. Later destination words are untouched. A short destination fails with no partial write. `dest` must not overlap the bitmap's words. An empty bitmap still needs a non-null `dest`, which it does not write.

## Concurrency

No function is thread-safe or interrupt-safe on its own. The caller must serialise every conflicting access. That includes two writers that change different bits of one word, and a reader that overlaps a writer. `volatile` does not fix these races, and a multi-word snapshot is a sequence of loads, not one instant. [design.md](design.md#keeping-the-guard-in-one-owner) shows how to keep the guard in one owner file so that no caller can skip it.

## Bank-and-bit identifiers

Banks are a consumer concept. A checked adapter can keep a stable `bank * 16 + bit` position without adding banks to the library:

```c
static bool
set_bank_bit(const bitmap_t *map, size_t bank, size_t bit)
{
        if ((bit >= BITMAP_BITS_PER_WORD)
            || (bank > ((SIZE_MAX - bit) / BITMAP_BITS_PER_WORD))) {
                return false;
        }
        return bitmap_set(map, (bank * BITMAP_BITS_PER_WORD) + bit);
}
```
