# Integration guide

This guide shows how to add `bitmap` to a project, build it, and run its tests. The API is described in [api.md](api.md), and the storage model and contracts in [design.md](design.md).

## Requirements

- A C11 compiler with an exact 16-bit `uint16_t`. The header checks this at compile time.
- For the tests: Meson, Ninja, Python 3.10 or later, `pkg-config` or `pkgconf`, and a C++17 compiler.

## Copy-in

Copy-in is the recommended method for embedded targets. Copy two files into your source tree:

```
include/bitmap.h
src/bitmap.c
```

Compile `bitmap.c` with your other sources and include the header:

```c
#include "bitmap.h"
```

No generated header and no build configuration are needed.

## Meson subproject

Put the repository in `subprojects/bitmap`, as a checkout or through a wrap file, then declare the dependency:

```meson
bitmap_dep = dependency('bitmap', fallback: ['bitmap', 'bitmap_dep'])
```

The build calls `meson.override_dependency('bitmap', ...)`, so other subprojects in the same build find `bitmap` by name. Tests are off by default, so a subproject build compiles only the library.

## Installed package

```sh
meson setup build --buildtype=release -Dbuild_tests=false --prefix=/usr/local
meson compile -C build
meson install -C build
cc app.c $(pkg-config --cflags --libs --static bitmap) -o app
```

After installation the header is available as `#include <bitmap.h>`. An optional `bitmap_version.h` is installed beside it, and the library sources do not include it.

## Build and test

```sh
# Library only
meson setup build --buildtype=release -Dbuild_tests=false
meson compile -C build

# Unit and integration tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build --verbose
```

The integration tests build real consumers in copy-in, subproject, and installed modes, and serve several capacities from one library build. They are native-only and are skipped in cross builds. Sanitiser, coverage, and MISRA commands are in [CONTRIBUTING.md](../CONTRIBUTING.md).

## Cross builds

The consuming firmware owns its cross file, linker command file, and memory placement. No cross file ships with this project.

- CI compiles the library for `arm-none-eabi` (Cortex-M0, freestanding, `-Os`).
- The library source compiles without diagnostics with a vendor C11 compiler for a 16-bit-MAU DSP target, where `CHAR_BIT` is 16. The recorded command is in [design.md](design.md#memory-cost).

These results cover the source dialect and type assumptions only. Linking, execution, operation timing, and the final map-file footprint remain checks for the consumer.

## Code size

`bitmap` is one translation unit. When a program uses any function, the linker keeps all of them unless you compile with `-ffunction-sections` and link with `-Wl,--gc-sections`. With both, a minimal consumer drops the operations it does not call. Embedded vendor compilers usually have an equivalent per-function section option. Measured sizes are in [design.md](design.md#memory-cost).

A descriptor built with `BITMAP_INITIALIZER` can be `const`. On a target built without position-independent code it then lives in read-only memory, so only the backing words use RAM.
