project(
  'bitmap',
  'c',
  version: '0.1.0',
  default_options: [
    'c_std=c11',
    'warning_level=3',
  ],
  license: 'MIT',
)

project_description = 'Capacity-independent, caller-owned bitmap sets for embedded C'

# === Version header ===========================================================
# Optional: the library sources do not include this file, so a copy-in build
# needs only include/bitmap.h and src/bitmap.c.

ver = meson.project_version().split('.')

major = ver[0].to_int()
minor = ver[1].to_int()
patch = ver[2].to_int()

conf = configuration_data()
conf.set_quoted('VERSION', meson.project_version())
conf.set('MAJOR', major)
conf.set('MINOR', minor)
conf.set('PATCH', patch)

generated_version_header = configure_file(
  input: 'config' / 'bitmap_version.h.in',
  output: 'bitmap_version.h',
  configuration: conf,
  install: true,
  install_dir: get_option('includedir') / 'bitmap',
)

# === Include directories ======================================================

public_headers = include_directories('include')
generated_headers = include_directories('.')

# === Library ==================================================================

library_sources = files('src/bitmap.c')

bitmap_lib = static_library(
  'bitmap',
  library_sources,
  include_directories: [public_headers],
  install: true,
)

install_headers('include/bitmap.h', subdir: 'bitmap')

bitmap_dep = declare_dependency(
  include_directories: [public_headers, generated_headers],
  link_with: bitmap_lib,
  sources: generated_version_header,
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
