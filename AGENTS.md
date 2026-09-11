# AGENTS.md

---

## 1) Project-specific instructions

**Project:** `bitmap`
**Primary goal:** Fixed-capacity, caller-owned bitmap sets whose descriptor layout is independent of capacity, so one compiled library serves any bit count.

### 1.1 Essential commands

#### Configure and build (library only)

```sh
meson setup build --wipe --buildtype=release -Dbuild_tests=false
meson compile -C build
```

#### Configure, build, and run unit and integration tests

```sh
meson setup build --wipe --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

#### MISRA C:2023 (needs a configured build for `compile_commands.json`)

```sh
misch run -v
misch deviations
```

#### Formatting (pinned clang-format through `uvx`)

```sh
scripts/format.sh          # check, as CI does
scripts/format.sh --fix    # rewrite in place
```

#### Footprint evidence

```sh
bench/measure.sh          # native compiler
bench/measure.sh clang
```

### 1.2 Notes

- `meson setup` generates the optional `bitmap_version.h` into the build directory. The library sources do not include it; copy-in needs only `include/bitmap.h` and `src/bitmap.c`.
- Integration tests call `meson`, `pkg-config`, and a C++ compiler. They are native-only and are skipped in cross builds.

---

## 2) Non-negotiable design invariants

- The `bitmap_t` layout must not depend on `bit_count`, feature flags, or any backend selection. Never add a global capacity macro or per-capacity linker symbols.
- All capacities in one process must work against one compiled library.
- Storage is caller-owned. No heap, no VLAs, no hidden storage, no singleton, no allocation.
- `bitmap_word_t` is exactly `uint16_t`. Do not make the word width a build configuration axis without a demonstrated need.
- Public functions return `bool` and write values through output parameters. An invalid argument must never look like a successful state transition.
- The portable baseline requires caller serialisation. Do not claim atomicity or ISR safety for `bitmap_test_and_set` or `bitmap_test_and_clear`.
- Keep the unused-high-bit rules of the final word consistent across queries, counts, scans, and snapshots.

---

## 3) CI / source of truth

- CI definitions live in `.github/workflows/ci.yml`.
- Prefer running the same commands locally as CI runs.
- The CI jobs are: tests under ASan/UBSan on Linux and ASan on macOS, ThreadSanitizer, an `arm-none-eabi` freestanding compile, `clang-format`, a release build with a measurement-tool smoke test, a 100 percent line and branch coverage gate, and `misch`. The isolated packaging consumers use their own compiler flags and are not sanitiser-instrumented.

---

## 4) Docs / commit conventions

- Use Conventional Commits format when asked to commit.
- Keep commits focused and explain why in the message body.
- `README.md` is a short landing page: a quick start, a description, validated toolchains, and links. Put detail in `docs/`, not in the README.
- `docs/integration.md` covers installation, building, and cross builds.
- `docs/api.md` is the API reference. Use fenced C signatures followed by prose, never a table of functions. Update it with the header Doxygen whenever an interface changes.
- C code blocks in `README.md` and `docs/` compile against the header in the `bitmap docs examples` test. On the line directly above the fence, mark a block of statements with `<!-- snippet: body -->` and pseudo-code with `<!-- snippet: skip -->`.
- `docs/design.md` is the single authoritative design document. Keep it standalone and in step with behaviour and measured cost.
- Write prose in British English and do not hard-wrap markdown.

---

## 5) C style expectations

### Build and configuration

- Use the Meson build system. Do not introduce CMake, Make, or other systems.
- Update `meson.build` when adding or removing library source files, and `tests/meson.build` when adding or removing tests.

### Formatting

- `.clang-format` is present and mandatory. Run `scripts/format.sh --fix` before committing. It runs the clang-format release pinned in the script through `uvx`. Do not use a system `clang-format`: another release can format the same code differently.
- Do not reformat unrelated code.
- Key settings: 8-space indent, `BreakBeforeBraces: Linux`, column limit 80.

### Style and correctness

- Match conventions in the existing files (indentation, braces, naming).
- Validate pointer arguments at every public API boundary.
- No heap allocation (`malloc`, `free`, VLAs).
- Use `uint16_t`, `uint32_t`, and `bool` from `<stdint.h>` and `<stdbool.h>`; never plain `int` for fixed-width fields.
- Audit every shift. Counts stay within the 16-bit word and never reach a sign bit. Compute in `unsigned int` before narrowing to `bitmap_word_t`.
- Do not use `uint8_t`, and do not assume a C byte is an octet.

### Error handling

- Public functions return `bool` or validate via an early return.
- No `errno`, no exceptions, no callbacks, no logging.

### Comment placement (Doxygen)

- Inline trailing annotations (`/**< ... */`) on members are allowed only when the line fits 80 columns. If a member would overrun, move all of that aggregate's member docs into a `@details` list above the type.
- Never mix inline and block forms within one aggregate.

### Testing

- Run `meson test -C build` after every change.
- Add a test case for each bug fix and each new behaviour, including the error path.
- Keep `src/` and `include/` at 100 percent line and branch coverage; the CI gate enforces it.
- Tests live in `tests/test_*.c`; integration fixtures live in `tests/integration/`.

---
