#!/bin/sh
# SPDX-License-Identifier: MIT
#
# Reproduces the host measurements in docs/design.md. It does not measure
# target timing or claim target worst-case execution time.
#
# Usage: bench/measure.sh [CC]

set -eu

root=$(cd "$(dirname "$0")/.." && pwd)
cc=${1:-${CC:-cc}}
out=$(mktemp -d)
trap 'rm -rf "$out"' EXIT

cflags="-std=c11 -Os -Wall -Wextra -Werror -ffunction-sections -fdata-sections"
gcflags="-Wl,--gc-sections"

echo "compiler: $($cc --version | head -1)"
echo "flags:    $cflags"
echo

$cc $cflags -I"$root/include" -c "$root/src/bitmap.c" -o "$out/bitmap.o"
$cc $cflags -I"$root/bench" -c "$root/bench/prototype.c" -o "$out/prototype.o"

echo "== descriptor layout =="
$cc $cflags -I"$root/include" "$root/bench/sizeof_report.c" -o "$out/sizeof"
"$out/sizeof"
echo

echo "== object size (-Os) =="
printf '%-16s ' "bitmap:"; size "$out/bitmap.o" | tail -1
printf '%-16s ' "prototype:"; size "$out/prototype.o" | tail -1
echo

echo "== per-function .text (bytes) =="
echo "-- bitmap --"
nm --print-size --size-sort "$out/bitmap.o" | grep ' [Tt] ' || true
echo "-- prototype --"
nm --print-size --size-sort "$out/prototype.o" | grep ' [Tt] ' || true
echo

echo "== minimal linked consumer =="
for gc in no yes; do
  if [ "$gc" = yes ]; then extra=$gcflags; else extra=; fi
  $cc $cflags $extra -I"$root/include" "$root/bench/minimal_bitmap.c" \
      "$out/bitmap.o" -o "$out/min_bitmap"
  $cc $cflags $extra -I"$root/bench" "$root/bench/minimal_proto.c" \
      "$out/prototype.o" -o "$out/min_proto"
  printf 'gc-sections=%-4s bitmap:    ' "$gc"
  size "$out/min_bitmap" | tail -1
  printf 'gc-sections=%-4s prototype: ' "$gc"
  size "$out/min_proto" | tail -1
done

echo
echo "== host operation costs (-Os, best-effort wall time) =="
$cc $cflags -pthread -I"$root/include" -c "$root/bench/operations.c" \
    -o "$out/operations.o"
$cc $cflags -pthread "$out/operations.o" "$out/bitmap.o" \
    -o "$out/operations"
"$out/operations"
