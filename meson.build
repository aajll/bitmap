# SPDX-License-Identifier: MIT

project(
  'bitmap',
  'c',
  version: '0.1.0',
  meson_version: '>=0.57.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'Capacity-independent, caller-owned bitmap sets for embedded C'

# === Version ==================================================================
# include/bitmap.h defines the version macros, so a copy-in build has them.
# Configuration fails when they differ from the project version.

fs = import('fs')
version_parts = meson.project_version().split('.')
version_macros = {
  'BITMAP_VERSION_MAJOR': version_parts[0],
  'BITMAP_VERSION_MINOR': version_parts[1],
  'BITMAP_VERSION_PATCH': version_parts[2],
  'BITMAP_VERSION': '"' + meson.project_version() + '"',
}
header_lines = []
foreach line : fs.read('include' / 'bitmap.h').split('\n')
  header_lines += ' '.join(line.split())
endforeach
foreach name, value : version_macros
  if '#define @0@ @1@'.format(name, value) not in header_lines
    error('include/bitmap.h must define @0@ as @1@'.format(name, value))
  endif
endforeach

# === Include directories ======================================================

public_headers = include_directories('include')

# === Library ==================================================================

library_sources = files('src/bitmap.c')

bitmap_lib = static_library(
  'bitmap',
  library_sources,
  include_directories: [public_headers],
  install: true,
)

install_headers(
  'include/bitmap.h',
  subdir: 'bitmap',
)

bitmap_dep = declare_dependency(
  include_directories: [public_headers],
  link_with: bitmap_lib,
)

meson.override_dependency('bitmap', bitmap_dep)

# === pkg-config ===============================================================

pkgconfig = import('pkgconfig')
pkgconfig.generate(
  bitmap_lib,
  description: project_description,
  filebase: 'bitmap',
  name: 'bitmap',
  subdirs: 'bitmap',
)

# === Host unit tests ==========================================================

if get_option('build_tests')
  subdir('tests')
endif
