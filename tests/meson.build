thread_dep = dependency('threads')

# Core behavioural suite: sizing, lifecycle, single-bit operations, bulk
# operations, snapshots, boundaries, a randomized reference model, multiple
# independent instances in one translation unit, and compile-time const
# descriptors.
test_sources = files(
  'test_main.c',
  'test_harness.c',
  'test_sizing.c',
  'test_init.c',
  'test_bits.c',
  'test_bulk.c',
  'test_snapshot.c',
  'test_boundaries.c',
  'test_random.c',
  'test_instances.c',
  'test_static.c',
)

test_exe = executable(
  'test_bitmap',
  test_sources,
  dependencies: [bitmap_dep],
  c_args: ['-Werror'],
)
test('bitmap unit', test_exe)

# Caller-serialized concurrency example. Every conflicting access runs under
# one mutex, so the executable is clean under ThreadSanitizer.
test_threads_exe = executable(
  'test_bitmap_threads',
  ['test_threads.c', 'test_harness.c'],
  dependencies: [bitmap_dep, thread_dep],
  c_args: ['-Werror'],
)
test('bitmap caller-serialized threads', test_threads_exe, timeout: 60)

# C++17 portable-interface smoke test (native builds only; it links the host
# library, so it is meaningless in a cross build).
if not meson.is_cross_build() and add_languages('cpp', required: false, native: true)
  test_cpp_exe = executable(
    'test_bitmap_cpp',
    ['test_cpp.cpp'],
    dependencies: [bitmap_dep],
    cpp_args: ['-std=c++17', '-Werror'],
  )
  test('bitmap c++ header', test_cpp_exe)
endif

# Native integration: copy-in, Meson subproject, installed pkg-config, and
# coexistence with a second local library when it is checked out. The mode
# is positional and must precede the trailing variadic --cc list.
if not meson.is_cross_build()
  python = import('python').find_installation()
  meson_program = find_program('meson')
  pkg_config_program = find_program('pkg-config', 'pkgconf')
  fs = import('fs')
  status_root = meson.project_source_root() / '..' / 'status'
  integration_script = files('integration/test_integration.py')
  integration_tail = [
    '--source-root', meson.project_source_root(),
    '--meson', meson_program.full_path(),
    '--pkg-config', pkg_config_program.full_path(),
    '--cc',
  ] + meson.get_compiler('c').cmd_array()

  foreach mode : ['copy-in', 'subproject', 'installed']
    test('bitmap integration ' + mode, python,
         args: [integration_script, mode] + integration_tail,
         timeout: 300)
  endforeach

  coexistence_args = []
  if fs.exists(status_root / 'src' / 'status.c')
    coexistence_args = [
      '-DBITMAP_COEXISTENCE_STATUS=1',
      '-I' + (status_root / 'include'),
    ]
    test('bitmap integration coexistence', python,
         args: [integration_script, 'coexistence',
                '--status-root', status_root] + integration_tail,
         timeout: 300)
  endif

  # Give editors a compile-database entry in both standalone and coexistence
  # checkouts. The target builds the real branch only when status is present.
  static_library(
    'bitmap_coexistence_fixture',
    'integration/coexistence.c',
    dependencies: [bitmap_dep],
    c_args: coexistence_args,
    build_by_default: false,
  )

  # Compile-database entry for the initializer fixture. Its default size is
  # the passing case; the integration test compiles the failing case itself.
  static_library(
    'bitmap_initializer_fixture',
    'integration/initializer.c',
    dependencies: [bitmap_dep],
    build_by_default: false,
  )

  # Every C example in the README and docs/ compiles against the real header,
  # so a changed signature or macro fails here instead of in a reader's build.
  test('bitmap docs examples', python,
       args: [files('docs/check_snippets.py'),
              '--root', meson.project_source_root(),
              '--lang', 'c',
              '--prelude', '#include "bitmap.h"',
              '--']
             + meson.get_compiler('c').cmd_array()
             + ['-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-Werror',
                '-Wno-unused-function', '-Wno-unused-variable',
                '-I' + (meson.project_source_root() / 'include'),
                '-I' + (meson.current_source_dir() / 'docs'),
                '-c', '{source}', '-o', '{object}'],
       timeout: 60)
endif
