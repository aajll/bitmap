# Design

`bitmap` is a C11 library for fixed-capacity sets of bits over storage the caller owns. A descriptor holds a pointer to an array of `uint16_t` words and a logical bit count. The descriptor layout is the same for every capacity, so one compiled library serves any bit count and no capacity macro or per-capacity linker symbol is needed.

## Goals and non-goals

Goals:

- Caller-owned static or automatic storage, with no allocation.
- Capacity independence, so bitmaps of different sizes share one library build.
- Bounds-checked, zero-based bit operations with explicit success results.
- Deterministic, bounded work per operation.
- Portability across 8-bit, 16-bit, and 32-bit targets, including targets where a C byte is not an octet.

Non-goals:

- Fault, warning, and info classification.
- Latching, acknowledgement, event history, and last-event tracking.
- Callbacks, logging, and hidden singleton state.
- Heap allocation, resizing, sparse containers, and compression.
- Bit-range allocation and resource-pool policy.
- Wire codecs, schema generation, and identifier catalogues.
- General set algebra and multiple configurable word widths.

## Storage model

### Descriptor

`bitmap_word_t` is exactly `uint16_t`. A compile-time assertion rejects a platform without an exact 16-bit type.

<!-- snippet: skip -->
```c
typedef struct {
        bitmap_word_t *words;
        size_t bit_count;
} bitmap_t;
```

The layout does not depend on `bit_count`, on feature flags, or on any backend selection. The fields are public so callers can size snapshots and inspect capacity. `bitmap_init` sets `words` to `NULL` when `bit_count` is zero.

A descriptor aliases its backing words. Copying a descriptor copies the view, not the contents, so two descriptors can refer to the same bitmap.

Only `bitmap_init` writes a descriptor. Every other operation takes `const bitmap_t *`, so a descriptor can be a `const` object. `BITMAP_INITIALIZER(array, bits)` builds one at compile time with no call to `bitmap_init`. A target built without position-independent code places such a descriptor in read-only memory, and only the backing words occupy RAM. A 16-bit-MAU DSP compiler places it in `.const`, and a non-PIC GCC host build places it in `.rodata`. A position-independent host build places it in `.data.rel.ro`, which the loader relocates and then write-protects.

Capacities, word counts, indices, and scan positions are `size_t`. Stored words use a fixed-width type.

### Words and indices

A bitmap is a packed array of 16-bit words, least significant bit first. Bit `n` is bit `n % 16` of word `n / 16`. Bit zero is the least significant bit of word zero. A valid index satisfies `index < bit_count`.

The final word can hold bits that have no logical index. Every query, count, scan, and snapshot masks those bits off. `bitmap_init`, `bitmap_fill_all`, and `bitmap_snapshot` write zeroes into them. Storage written by a caller can carry arbitrary values in those positions without affecting any result.

### Sizing helpers

`bitmap_words_for_bits(bit_count)` returns the ceiling word count. It evaluates its argument once and uses quotient plus remainder, so it cannot overflow near `SIZE_MAX`. The common `(bit_count + 15) / 16` form overflows in that range.

`BITMAP_WORDS_FOR_BITS(bits)` is the constant-expression form for array declarations and static assertions. `BITMAP_STORAGE_WORDS(bits)` returns the same value but never zero, so a declared array is never zero-length when the bit count is a compile-time zero. `BITMAP_ARRAY_WORDS(array)` returns the word count of an array object, which lets a caller pass the array's own capacity to `bitmap_init` so the declared array and the reported capacity cannot disagree.

`BITMAP_INITIALIZER(array, bits)` expands to a braced `bitmap_t` initialiser. Compilation fails when `BITMAP_ARRAY_WORDS(array)` is less than `BITMAP_WORDS_FOR_BITS(bits)`. C11 has no static assertion that can appear inside an expression, so the check is a zero-valued `sizeof` term whose array size becomes negative when the storage is too small. Every conforming C and C++ compiler must reject a negative array size. `array` must name an array object rather than a pointer.

The macro forms evaluate their arguments more than once. Use them with side-effect-free constants. Use `bitmap_words_for_bits` for runtime values.

## Memory cost

The host measurements use x86_64 Linux, where `CHAR_BIT == 8`, with GCC 16.2.1 and Clang 22.1.8 at `-Os`. On this host an addressable unit is one octet, so the following columns are both C object-size units and octets. Run `bench/measure.sh` to reproduce them.

| Logical bits | Words | Backing octets | Descriptor octets | Total octets |
| ---: | ---: | ---: | ---: | ---: |
| 0 | 0 | 0 | 16 | 16 |
| 1 | 1 | 2 | 16 | 18 |
| 16 | 1 | 2 | 16 | 18 |
| 17 | 2 | 4 | 16 | 20 |
| 192 | 12 | 24 | 16 | 40 |
| 1024 | 64 | 128 | 16 | 144 |

`sizeof(bitmap_word_t)` is 2 and `sizeof(bitmap_t)` is 16 on this host: an 8-octet pointer at offset 0 and an 8-octet `size_t` at offset 8, with no padding.

A one-off compile with TI C2000 Code Generation Tools 25.11.1.LTS, C28x EABI, large memory model, C11 mode, and the consumer's optimisation options produced `CHAR_BIT == 16`, `sizeof(bitmap_word_t) == 1`, and `sizeof(bitmap_t) == 4`. These are 16-bit addressable units, so the descriptor occupies 64 bits or 8 octets. A 192-bit bitmap uses 12 backing units plus 4 descriptor units, for 256 bits or 32 octets in total. This is a compile-time layout result, not a claim about a linked firmware image.

The descriptor columns count RAM only for a descriptor that `bitmap_init` writes. A `const` descriptor built with `BITMAP_INITIALIZER` occupies read-only memory instead on a target built without position-independent code, as described under Descriptor.

The validation command was:

```sh
TI_CGT_DIR=/path/to/ti-cgt-c2000_25.11.1.LTS
"$TI_CGT_DIR/bin/cl2000" \
  --c11 -v28 -ml -mt --abi=eabi \
  --cla_support=cla1 --float_support=fpu32 \
  --tmu_support=tmu0 --vcu_support=vcu2 \
  -O3 --opt_for_speed=5 --fp_mode=relaxed \
  --gen_func_subsections=on --strict_ansi --issue_remarks \
  --emit_warnings_as_errors --display_error_number \
  -I"$TI_CGT_DIR/include" -I"$TI_CGT_DIR/include/sys" -Iinclude \
  --compile_only src/bitmap.c --output_file=/tmp/bitmap.obj
```

No cross file ships with this project. The consuming firmware owns its target definition, linker command, memory placement, and final validation.

The library object has no `.data` and no `.bss`. There is no hidden state and no read-only table.

Whole-object `.text` at `-Os`:

| Compiler | `bitmap` | Explicit-capacity prototype | Delta |
| --- | ---: | ---: | ---: |
| GCC 16.2.1 | 1513 | 1348 | +165 |
| Clang 22.1.8 | 1802 | 1442 | +360 |

The delta is the cost of the descriptor API and its initialisation contract: descriptor validation, descriptor indirection in each operation, and validation of the declared storage relationship. `bench/prototype.c` implements the same operation set with the capacity passed at each call and exists only for this comparison.

A minimal consumer that uses only clear-all, set, test, and count:

| Compiler | Section GC | `bitmap` `.text` | Prototype `.text` |
| --- | --- | ---: | ---: |
| GCC 16.2.1 | off | 3131 | 2950 |
| GCC 16.2.1 | on | 2135 | 2048 |
| Clang 22.1.8 | off | 3532 | 3152 |
| Clang 22.1.8 | on | 2208 | 2034 |

The library is one translation unit. Linking any symbol from the archive keeps every public function unless the caller compiles with `-ffunction-sections` and links with `-Wl,--gc-sections`. With both, the GCC minimal consumer drops unused operations and shrinks by about 32 percent.

The TI C2000 25.11.1.LTS compile produced 1466 octets of executable ELF sections for `bitmap` and 1308 octets for the explicit-capacity prototype, a delta of 158 octets. The command used C28x EABI, the large memory model, `--c11`, `-O3`, `--opt_for_speed=5`, `--fp_mode=relaxed`, and function subsections. A linked target footprint depends on the consumer's linker command file, runtime library, placement, and garbage collection, so the consuming firmware must record it from its map file.

## Operation reference

Signatures, usage examples, and the full argument rules are in [api.md](api.md).

| Function | Contract |
| --- | --- |
| `bitmap_words_for_bits` | Ceiling word count. Single evaluation, overflow-safe. |
| `bitmap_init` | Validate the descriptor, declared storage, and request; clear the required words; publish the descriptor. |
| `bitmap_clear_all` | Zero every required word. |
| `bitmap_fill_all` | Set every logical bit and leave the unused tail bits zero. |
| `bitmap_set` | Set one logical bit; other bits are unaffected. |
| `bitmap_clear` | Clear one logical bit; other bits are unaffected. |
| `bitmap_test` | Report success and write the bit value. |
| `bitmap_test_and_set` | Report success and write the previous value, then set the bit. |
| `bitmap_test_and_clear` | Report success and write the previous value, then clear the bit. |
| `bitmap_any` | Report success and write whether any logical bit is set. |
| `bitmap_count` | Report success and write the population count. |
| `bitmap_scan` | Report success and write the lowest set index at or after `start`, or the end sentinel. |
| `bitmap_snapshot` | Copy the words and zero the unused tail bits; reject an undersized destination. |

## Error model

Public functions return `bool`. `true` means the request was valid and any output parameter was written. `false` means an argument was invalid, and both storage and output parameters are unchanged. An invalid index never looks like a successful state transition.

Queries keep validity separate from values. `bitmap_test` and `bitmap_any` report success through the return value and write the result through an output pointer. The test-and-modify pair report success and write the previous value. A caller can always distinguish "the bit is clear" from "the call failed".

There is no `errno`, no exception, no callback, and no logging.

Every public pointer argument is checked for `NULL`. The library cannot discover the real length of an arbitrary pointer. A descriptor whose `bit_count` exceeds its actual storage, or storage shorter than the caller declared to `bitmap_init`, is undefined behaviour and a caller precondition. A forged descriptor is caller misuse even though the descriptor layout is capacity-independent.

## Boundary semantics

### Empty bitmap

A bitmap with `bit_count == 0` has no storage, no set bits, a count of zero, and an empty scan. `bitmap_any` reports `false`, `bitmap_count` reports zero, and `bitmap_scan` writes the sentinel zero. All of these operations succeed.

`bitmap_init` accepts `(NULL, 0)` and also accepts a non-`NULL` storage pointer with a zero bit count, regardless of the declared word count. It rejects a null storage pointer paired with a non-zero declared word count. Every accepted empty form becomes `words == NULL`, `bit_count == 0`, and no storage is written. The canonical empty view keeps every empty bitmap identical. `BITMAP_INITIALIZER` does not canonicalise: a zero-bit descriptor keeps its array pointer, and no operation reads or writes through it.

### Initialisation

`bitmap_init` validates the descriptor pointer, the relationship between the storage pointer and its declared word count, and the required capacity before writing. It rejects a null storage pointer with a non-zero declared word count, a non-zero bit count with null storage, and any storage shorter than the required word count. On failure the descriptor and the storage are unchanged.

On success it clears exactly the required words and leaves later words untouched. Initialisation and reinitialisation require exclusive access. `storage` must be aligned for `bitmap_word_t` and must not overlap the descriptor.

### Compile-time initialisation

`BITMAP_INITIALIZER` replaces `bitmap_init` when the storage array is known at compile time. Its storage check runs at compile time, so it has no run-time failure path. It does not clear the words. Objects with static storage duration start zeroed; automatic storage must be cleared with `bitmap_clear_all` before the first query.

### Scan

`bitmap_scan(map, start, &index)` returns request validity, not discovery. A valid request succeeds even when no bit is found.

- `start < bit_count` searches from that index.
- `start == bit_count` is valid and finds nothing.
- `start > bit_count` is invalid and fails.

On success `*index` is the lowest set index at or after `start`, or `bit_count` when none exists. Callers detect "no result" by comparing `*index` with `bit_count`. Iteration is a loop that reuses the found index as the next start.

The first candidate word is masked twice: once for the unused tail bits and once for the bits below `start`. When `start == bit_count`, those masks are disjoint and the word becomes zero, so the sentinel falls out of the normal loop with no special case.

### Snapshot

`bitmap_snapshot` writes exactly `bitmap_words_for_bits(bit_count)` words and zeroes the unused tail bits of the last word. Words of the destination past that count are untouched.

A destination smaller than the required word count fails with no partial write. The destination must not overlap the bitmap's words. An empty bitmap still requires a valid destination pointer, which is not written.

### Imported words

The caller owns the backing words and can write them directly. Words imported with dirty tail bits are safe, because every query masks the tail. A caller that wants the canonical tail-zeroed form can snapshot into a separate array.

## Ownership and lifecycle

Backing storage must outlive every descriptor and operation that refers to it. Descriptor fields must not change while another context uses the bitmap. Reinitialisation clears the bitmap, so it is only safe when no other context holds a view of it. A `const` descriptor cannot be reinitialised; `bitmap_clear_all` resets its bits.

## Concurrency

The baseline operates on ordinary `uint16_t` words and requires caller serialisation for every conflicting access. That includes two writers that modify different bits of one word, and any reader that runs concurrently with a writer.

`volatile` does not repair these races. `bitmap_test_and_set` and `bitmap_test_and_clear` are not atomic despite their names; the caller must protect the whole call. The library owns no lock, interrupt mask, or memory-ordering policy.

A consumer that needs lock-free atomic bit operations without external serialisation requires a separate atomic extension. Such an extension must:

- keep the atomic storage contract distinct from the portable one, with its own storage type;
- validate target support and alignment, and pin the backend at the platform level;
- avoid casts between ordinary and atomic arrays;
- avoid hidden locks, which would not provide an ISR lock-free guarantee;
- specify single-word atomicity separately from whole-bitmap consistency, because a bulk snapshot is not coherent across words;
- account for another CPU or a DMA engine, since local interrupt masking does not protect against either.

### Keeping the guard in one owner

The library cannot check that callers serialise access, so a consumer should make an unguarded call impossible rather than merely discouraged. One owner file holds the storage and the descriptor with internal linkage, and exports only functions that take the guard:

```c
/* faults.c is the only file that can reach the fault words. */
#include "bitmap.h"
#include "faults.h"

#define FAULT_BITS 192u

static bitmap_word_t fault_words[BITMAP_STORAGE_WORDS(FAULT_BITS)];
static const bitmap_t faults = BITMAP_INITIALIZER(fault_words, FAULT_BITS);

bool
fault_raise(size_t position)
{
        faults_key_t key;
        bool ok;

        key = faults_lock();
        ok = bitmap_set(&faults, position);
        faults_unlock(key);
        return ok;
}
```

- No header declares the words or the descriptor, and no function returns a pointer to either. A caller that needs the bits receives a snapshot copied into its own storage under the guard.
- Every exported function holds the guard for the whole bitmap call, reads included. A sequence that must see one state, such as a test followed by a clear, holds the guard across every call in it.
- `faults_lock` and `faults_unlock` are consumer policy. A threaded host can use a mutex. A single-core interrupt-driven target can save and mask interrupts, then restore the saved state; the key carries that state from lock to unlock. The guard must cover every context that can reach the words, including each interrupt priority, another CPU, and a DMA engine.
- Aggregate operations hold the guard for `O(words)` work. Include that time in interrupt-latency and WCET budgets.

## Portability

- C11. The header also compiles as C++17 and the library links from C++.
- Exact 16-bit `uint16_t` is required and asserted at compile time.
- A C byte need not be an octet. Capacities are words and logical capacities are bits, and no in-memory object size is treated as a wire length.
- `uint8_t` is not used, because on some targets it aliases a wider unit.
- Shifts are computed in `unsigned int` and narrowed afterwards, so a shift of 15 into the top bit of a 16-bit word is well-defined even when `int` is 16 bits. Shift counts stay in `0..15` and never reach a sign bit.
- There are no VLAs, no heap, no flexible-array tricks, and no untyped byte buffers.
- No compiler intrinsic is required for the population count. A host build with a redefined `CHAR_BIT` is not a valid simulation of a 16-bit-MAU target, so no such simulation is provided.

## Implementation notes

The tail mask returns `0xFFFF` when `bit_count` is a multiple of 16. Callers apply it only to the last word, so a full mask is the correct result there and no branch is needed.

The population count is a branch-free 16-bit SWAR reduction over a `uint32_t` intermediate. The reduction adds the two bytes of the intermediate and masks with `0x1F`. The familiar 32-bit `value * 0x01010101 >> 24` step does not work at 16 bits, because the high byte would land above bit 15 of the product.

The single-bit search walks from the least significant bit and runs at most 15 times. It is only called with a non-zero word.

`bitmap_init` rejects a request whose required word count cannot be satisfied by any real array. `bitmap_words_for_bits(SIZE_MAX)` is finite and does not wrap, so a huge bit count simply fails the storage comparison.

`BITMAP_WORDS_FOR_BITS` and `BITMAP_STORAGE_WORDS` evaluate their argument more than once. This is the only way to keep them usable as integer constant expressions in C.

## Performance

- `set`, `clear`, `test`: one word load or store plus a shift and a mask. Constant time.
- `test_and_set`, `test_and_clear`: the same plus one comparison. Constant time, but not atomic.
- `any`: `O(words)`, with early exit on the first set word.
- `count`: `O(words)`, with constant branch-free work per word.
- `scan`: `O(words)` worst case, plus a bounded search of at most 15 steps inside the matching word.
- `init`, `clear_all`, `fill_all`, `snapshot`: `O(words)`.

`bench/measure.sh` also builds `bench/operations.c`. The following host sample used an AMD Ryzen 7 5800X3D, a 192-bit bitmap, two million iterations per sample, five samples, and the minimum wall-clock time. The build used `-Os` without LTO. These numbers compare host compiler output; they are not target WCET values.

| Operation | GCC 16.2.1 ns/op | Clang 22.1.8 ns/op |
| --- | ---: | ---: |
| `init` | 11.55 | 4.06 |
| `clear_all` | 6.20 | 3.98 |
| `fill_all` | 7.05 | 4.19 |
| `set` | 2.64 | 1.46 |
| `clear` | 2.42 | 1.55 |
| `test` | 2.64 | 1.55 |
| `test_and_set` | 2.42 | 1.85 |
| `test_and_clear` | 2.44 | 1.78 |
| `any`, first word set | 3.75 | 1.78 |
| `count`, 12 words | 22.94 | 16.07 |
| `scan`, final word set | 12.13 | 17.84 |
| `snapshot`, 12 words | 9.92 | 4.42 |
| mutex lock/unlock only | 6.17 | 5.95 |
| mutex plus `set` | 7.93 | 7.27 |

The mutex rows separate host synchronisation cost from the bitmap call. They do not model interrupt masking or cross-core synchronisation. The consuming firmware must measure those mechanisms and operation WCET with its own memory placement, compiler, and interrupt policy before adoption.

## Adoption gate

The portable library is ready for a consumer only when that consumer records the bitmap owner, number of instances, capacities, all task, interrupt, CPU, and DMA accessors, the serialisation mechanism, identifier mapping, snapshot-consistency requirement, and target build evidence. No consumer record is included in v0.1.0, so this project does not claim adoption readiness for an interrupt-driven or shared-memory use. Adoption is blocked when any writer can preempt another writer or overlap a reader without a proven external guard. It is also blocked when another CPU or DMA engine can access the words and the proposed guard only masks local interrupts.

Application policy stays above this library. Classification, latching, acknowledgement, debounce, last-event tracking, error reporting, and transport encoding belong in a consumer-owned module. A consumer with bank-and-bit identifiers can preserve a stable position as `bank * 16 + bit`; that mapping does not make banks part of the bitmap API.

A multiword snapshot is a sequence of ordinary loads, not a transaction. A consumer that needs one coherent instant must quiesce writers, hold a suitable guard for the full copy, use ownership handoff or double buffering, or add a sequence protocol. The choice and its bounded cost belong to the consumer.

If external serialisation cannot meet the timing or execution-context requirements, adoption remains blocked until a separate atomic extension is specified and validated. That extension must not change `bitmap_word_t` through consumer-local macros or add hidden locks.

## Verification

The unit suite covers capacities 0 through 200 plus the documented boundary sizes, every failure path, poisoned tail bits, canaries around storage and extra backing words, a fixed-seed randomised model comparison, several independent instances in one translation unit, and compile-time `const` descriptors, including a table of descriptors with different capacities. `src/` reaches 100 percent line and branch coverage.

Integration tests build real consumers in three ways: copy-in with only `include/bitmap.h` and `src/bitmap.c`, a Meson subproject, and an installed `pkg-config` package. Each serves eight different capacities from one library build. The copy-in test also compiles one fixture twice: with exact-fit storage it must build, and with one word too few it must fail to compile. A C++17 test links the same header and library. A documentation test compiles every C example in the README and `docs/` against the header, so a changed signature or macro cannot leave a stale example. A threaded test uses one mutex around every conflicting access and is clean under ThreadSanitizer. An optional coexistence test links a second local library when its checkout is present, to confirm that headers and symbols do not collide. The optional fixture has its own compile-database entry so language tools receive the required include path.

The library source compiles without diagnostics under TI C2000 Code Generation Tools 22.6.2.LTS and 25.11.1.LTS in C11 C28x EABI mode. This validates the source dialect and type assumptions only. Linking, target execution, operation WCET, interrupt behaviour, and final map-file footprint remain consumer-owned checks.
