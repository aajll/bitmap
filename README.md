# bitmap

[![CI](https://github.com/aajll/bitmap/actions/workflows/ci.yml/badge.svg)](https://github.com/aajll/bitmap/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

**Fixed-capacity bit sets over caller-owned storage, for embedded C.**

## Quick start

Copy `include/bitmap.h` and `src/bitmap.c` into your project. For a Meson subproject or an installed `pkg-config` package, see the [integration guide](docs/integration.md).

```c
#include "bitmap.h"

#define FAULT_BITS 192u

/* Static storage starts zeroed. The build fails if the array is too small. */
static bitmap_word_t fault_words[BITMAP_STORAGE_WORDS(FAULT_BITS)];
static const bitmap_t faults = BITMAP_INITIALIZER(fault_words, FAULT_BITS);

void
fault_example(void)
{
        bool active = false;
        size_t count = 0u;

        (void)bitmap_set(&faults, 42u);
        (void)bitmap_test(&faults, 42u, &active);
        (void)bitmap_count(&faults, &count);
}
```

## Description

`bitmap` keeps a set of bits in an array of 16-bit words that you own. A `bitmap_t` is only a word pointer and a bit count, so its layout never depends on capacity. One compiled library serves bitmaps of every size, and separately built code cannot disagree about the layout. Typical uses are fault and event flags, slot maps, and dirty tracking.

- Any capacity against one build, with no capacity macro and no per-capacity symbols
- Caller-owned static or automatic storage, with no heap, no VLAs, and no hidden state
- Every call checks its arguments and returns `bool`, and a failed call changes nothing
- `const` descriptors built at compile time, which can live in read-only memory
- Set, clear, test, test-and-set, test-and-clear, fill, any, count, scan, and snapshot
- C11, with a header that also compiles as C++17
- Safe with a 16-bit `int` and with bytes wider than 8 bits
- 1.5 to 1.8 KiB of code at `-Os` on x86_64, with no `.data` or `.bss`
- 100% line and branch coverage, sanitiser-clean tests, and zero MISRA C:2023 findings from [`misch`](https://pypi.org/project/misch/)

The library is not thread-safe or interrupt-safe on its own, so the caller serialises every conflicting access. It has no set algebra, bit ranges, or resizing.

## Validated toolchains

| Toolchain               | Target                        | Validation                             |
| ----------------------- | ----------------------------- | -------------------------------------- |
| GCC, Clang              | Linux and macOS hosts         | Unit, integration, and sanitiser tests |
| `arm-none-eabi-gcc`     | Cortex-M0, freestanding       | Library compile                        |
| DSP vendor C11 compiler | 16-bit MAU (`CHAR_BIT == 16`) | Library compile                        |

Target linking, execution, and timing belong to the consuming firmware. See [cross builds](docs/integration.md#cross-builds).

## Documentation

- [Integration guide](docs/integration.md) — copy-in, Meson subproject, installed package, builds, and cross builds
- [API reference](docs/api.md) — every function and macro, with its rules
- [Design](docs/design.md) — storage model, contracts, concurrency, portability, and measured cost
- [Changelog](CHANGELOG.md) — release history

## Contributing

Read [CONTRIBUTING.md](CONTRIBUTING.md) before you open a pull request. Report security issues as described in [SECURITY.md](.github/SECURITY.md).

## License

MIT. See [LICENSE](LICENSE).
