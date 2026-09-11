# Contributing to bitmap

`bitmap` is a small C11 library for fixed-capacity sets of bits over caller-owned storage. Two invariants carry most of the design weight: the descriptor layout is independent of capacity, and the portable baseline requires caller serialisation. Keep both in mind when you change anything.

## Getting started

Install a C11 compiler, Meson, and Ninja. Native integration tests also need Python 3.10 or later, `pkg-config` or `pkgconf`, and a C++17 compiler. The same commands CI runs, locally:

```sh
# Configure with tests and sanitisers (CI default)
meson setup build --buildtype=debug -Dbuild_tests=true \
                  -Db_sanitize=address,undefined
meson compile -C build
meson test -C build --verbose

# Coverage (CI gate is 100% line and 100% branch for src/ and include/)
meson setup build_cov --buildtype=debug -Dbuild_tests=true -Db_coverage=true
meson compile -C build_cov && meson test -C build_cov
gcovr --root . --filter 'src/' --filter 'include/' --print-summary

# MISRA C:2023
meson setup build --buildtype=debug -Dbuild_tests=true   # compile_commands.json
misch run -v
misch deviations

# Formatting (pinned clang-format through uvx)
scripts/format.sh --fix
```

## Source style

- Formatting is mandatory. Run `scripts/format.sh --fix` before submitting, or `scripts/format.sh` to check without writing, as CI does. The script pins the clang-format release in `CLANG_FORMAT_VERSION` and runs it through `uvx`, so it needs [uv](https://docs.astral.sh/uv/). An editor formatter gives the same result only when it runs the same release.
- 8-space indent, Linux brace style, 80-column limit. Match the existing conventions and do not reformat unrelated code.
- The Meson build system is the single source of truth. Update `meson.build` and `tests/meson.build` when you add or remove files.
- No CMake, no Make, and no other build systems.

## C language rules

- C11 only. The header must also compile as C++17.
- Keep the public header valid for a conforming freestanding implementation. Do not add a hosted-only standard-library dependency for a compile-time facility.
- Use fixed-width types from `<stdint.h>` and `<stdbool.h>`. `bitmap_word_t` is exactly `uint16_t`. Do not use `uint8_t`.
- No heap allocation (`malloc`, `free`, VLAs), no hidden state, and no singleton.
- Validate every public pointer argument and every bit index at the API boundary. On failure, leave storage and output parameters unchanged.
- Do not add a global capacity macro or make the `bitmap_t` layout depend on capacity.
- Do not assume a C byte is an octet. Keep shifts within the 16-bit word. Compute a shift in `unsigned int` and narrow the result afterwards.
- Do not claim that `bitmap_test_and_set` or `bitmap_test_and_clear` are atomic, and do not add a hidden lock.

## MISRA C:2023

The library is analysed with `misch` (cppcheck-backed MISRA C:2023 analysis), configured by `misra.toml`. The audit is clean, and `misch run` must report zero findings. The analysed scope is `include/` and `src/`; `tests/` and `bench/` are excluded.

If your change introduces a finding:

1. Fix it where practical.
2. Otherwise suppress it at the point of use with `/* cppcheck-suppress misra-c2012-<rule> ; @deviation <rationale> */`, or add a justified project-wide entry to `misra-deviations.txt` for a house-style rule that spans a file.
3. Verify with `misch run` and `misch deviations`. Every suppression must carry a rationale, and `misch deviations` fails on an unjustified or stale one.
4. Flag any new deviation in the pull-request description.

The existing deviations are advisory Rules 2.5, 8.7, and 15.5, each justified in `misra-deviations.txt`. Full tool-driven compliance beyond this analysis needs a certified static analyser, which this repository does not vendor.

## Tests and coverage

- Add a test for every bug fix and every new behaviour, including the error path.
- CI enforces 100 percent line and branch coverage of `src/` and `include/`.
- Unit tests live in `tests/test_*.c` and are registered in `tests/meson.build`.
- Integration fixtures live in `tests/integration/` and are driven by `tests/integration/test_integration.py`. The integration tests are native-only and are skipped in cross builds.
- The `bitmap docs examples` test compiles every C code block in `README.md`, `CONTRIBUTING.md`, and `docs/` against the public header, through `tests/docs/check_snippets.py`. A block compiles at file scope by default. On the line directly above the fence, put `<!-- snippet: body -->` for a block of statements, or `<!-- snippet: skip -->` for pseudo-code or a copy of a header definition.
- An optional coexistence test links a second local library when its checkout is present, to confirm that headers and symbols do not collide. It is skipped when that checkout is absent.
- Run `bench/measure.sh` after a library change and update `docs/design.md` when code size or recorded host operation costs change. Target linking and timing remain consumer-owned.

## Documentation

| File | Purpose |
| --- | --- |
| `README.md` | Landing page: quick start, description, validated toolchains, and links. Keep it short. |
| `docs/integration.md` | Installation, building, and cross builds. |
| `docs/api.md` | Reference for every public function and macro. |
| `docs/design.md` | Storage model, operation contracts, concurrency, portability, and measured cost. |

Update `docs/api.md` and the header Doxygen together when an interface changes. Update `docs/design.md` when behaviour or cost changes. Keep it standalone: describe how the library works, not how it was built or which other projects use it. Write prose in British English, one line per paragraph.

## Commits

Use Conventional Commits:

- `feat: ...` new feature
- `fix: ...` bug fix
- `doc: ...` documentation only
- `test: ...` test-only changes
- `chore: ...` build, CI, release work
- `refactor: ...` a change that neither fixes a bug nor adds a feature

Keep the subject under about 70 characters. Use the body to explain why the change is needed.

## Pull requests

- Open an issue first for non-trivial changes so the design can be agreed before implementation.
- Keep pull requests focused: one feature or one fix.
- All CI checks must pass.

## When in doubt

Open an issue. The library is small enough that a new field on `bitmap_t` or a new public function has outsized consequences.
