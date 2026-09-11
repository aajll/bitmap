# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and this project adheres to [Semantic Versioning](https://semver.org/).

## [0.1.0] - 2026-09-11

Initial release.

### Added

- Capacity-independent `bitmap_t` descriptor (a word pointer and a `size_t` bit count) over caller-owned `uint16_t` storage. The layout does not depend on the bit count, so one compiled library serves bitmaps of any capacity.
- Lifecycle: `bitmap_init` validates the descriptor, declared storage, and request before writing. It clears exactly the required words and leaves extra words and the descriptor unchanged on failure. An empty bitmap is canonicalised to a null word pointer.
- `BITMAP_INITIALIZER`, a braced `bitmap_t` initialiser that fails to compile when the array is too small. Only `bitmap_init` writes a descriptor and every other operation takes `const bitmap_t *`, so a descriptor can be a `const` object in read-only memory.
- Whole-map operations: `bitmap_clear_all` and `bitmap_fill_all`.
- Single-bit operations: `bitmap_set`, `bitmap_clear`, `bitmap_test`, and the read-modify-write pair `bitmap_test_and_set` and `bitmap_test_and_clear`, which return the previous value.
- Aggregate queries: `bitmap_any`, `bitmap_count` (branch-free SWAR population count), and `bitmap_scan` (ascending set-bit search with `bit_count` as the end sentinel).
- `bitmap_snapshot` into caller-owned word storage, with tail-bit zeroing, no partial writes on a short destination, and untouched extra destination words.
- Sizing helpers: `bitmap_words_for_bits` (single evaluation, overflow-safe ceiling division) and the constant-expression macros `BITMAP_WORDS_FOR_BITS`, `BITMAP_STORAGE_WORDS`, and `BITMAP_ARRAY_WORDS`.
- Documented error, ownership, bit, boundary, and concurrency contracts, and the explicit conditions for a future atomic extension.
- Documentation layout: a short `README.md` landing page, with an integration guide, an API reference, and the design document in `docs/`.
- Test suite: capacities 0 through 200 plus the documented boundary sizes, boundary and failure paths, tail poisoning, canaries, a fixed-seed randomised reference model, multiple independent instances in one translation unit, and compile-time `const` descriptors. `src/` reaches 100 percent line and branch coverage.
- Integration tests: copy-in (only `include/bitmap.h` and `src/bitmap.c`), a real Meson subproject, and an installed `pkg-config` consumer, each serving eight capacities from one library build. The copy-in test also checks that undersized `BITMAP_INITIALIZER` storage fails to compile.
- Documentation test that compiles every C example in `README.md` and `docs/` against the public header, so the examples stay in step with the interface.
- C++17 header and link smoke test, and a caller-serialised threaded test that is clean under ThreadSanitizer.
- MISRA C:2023 analysis through `misch` with zero findings and justified deviations (Rules 2.5, 8.7, and 15.5).
- Footprint and host operation-cost measurement tools in `bench/`, with recorded GCC, Clang, and TI C2000 evidence in `docs/design.md`, including a comparison with an explicit-capacity prototype and separate host mutex cost.
