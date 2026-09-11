#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise real bitmap consumers in three integration modes.

Each mode compiles consumer.c once per capacity and links it with peer.c,
which uses a different capacity, against a single bitmap library build.
Success demonstrates a capacity-independent library with no generated
configuration:

* copy-in    - only include/bitmap.h and src/bitmap.c, no build system;
* subproject - a real Meson subproject dependency;
* installed  - an installed library consumed through pkg-config.
"""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
import subprocess
import tempfile
from collections.abc import Sequence
from pathlib import Path
from typing import final

CAPACITIES = (0, 1, 15, 16, 17, 37, 192, 193)
PEER_BITS = 300
PROJECT_FILES = ("meson.build", "meson_options.txt")
PROJECT_DIRS = ("include", "src", "config")


@final
class Arguments(argparse.Namespace):
    def __init__(self) -> None:
        super().__init__()
        self.mode = ""
        self.source_root = Path()
        self.status_root = Path()
        self.meson = ""
        self.pkg_config = ""
        self.cc: list[str] = []


def command(
    args: Sequence[str | Path],
    *,
    env: dict[str, str] | None = None,
    expect_failure: bool = False,
) -> str:
    result = subprocess.run(
        [str(arg) for arg in args],
        env=env,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        timeout=300,
        check=False,
    )
    good = result.returncode != 0 if expect_failure else result.returncode == 0
    if not good:
        message = "".join(
            [
                f"Command: {shlex.join(map(str, args))}\n",
                f"Expected failure: {expect_failure}\n",
                f"Exit: {result.returncode}\n{result.stdout}",
            ]
        )
        raise AssertionError(message)
    return result.stdout


@final
class Tests:
    def __init__(self, args: Arguments, work: Path) -> None:
        self.args = args
        self.work = work
        self.source = args.source_root
        self.status = args.status_root
        self.cc = args.cc
        self.flags = ["-std=c11", "-O2", "-Wall", "-Wextra", "-Werror"]
        self.env = os.environ.copy()
        self.env.update(
            CC=shlex.join(self.cc),
            CFLAGS="",
            CPPFLAGS="",
            LDFLAGS="",
            PKG_CONFIG_PATH="",
        )
        self.count = 0

    def consumer(self) -> Path:
        return self.source / "tests" / "integration" / "consumer.c"

    def peer(self) -> Path:
        return self.source / "tests" / "integration" / "peer.c"

    def copy_library(self, dest: Path) -> None:
        dest.mkdir(parents=True)
        for directory in PROJECT_DIRS:
            _ = shutil.copytree(self.source / directory, dest / directory)
        for name in PROJECT_FILES:
            _ = shutil.copy2(self.source / name, dest / name)

    def copy_in(self) -> None:
        include = self.work / "include"
        source = self.work / "src"
        include.mkdir(parents=True)
        source.mkdir(parents=True)
        _ = shutil.copy2(self.source / "include/bitmap.h", include / "bitmap.h")
        _ = shutil.copy2(self.source / "src/bitmap.c", source / "bitmap.c")

        flags = self.flags + ["-I" + str(include)]
        lib = self.work / "bitmap.o"
        _ = command(self.cc + flags + ["-c", source / "bitmap.c", "-o", lib])

        peer = self.work / "peer.o"
        _ = command(
            self.cc
            + flags
            + [f"-DBITMAP_PEER_BITS={PEER_BITS}", "-c", self.peer(), "-o", peer]
        )

        for capacity in CAPACITIES:
            obj = self.work / f"consumer-{capacity}.o"
            _ = command(
                self.cc
                + flags
                + [f"-DBITMAP_TEST_BITS={capacity}", "-c", self.consumer(), "-o", obj]
            )
            exe = self.work / f"consumer-{capacity}"
            _ = command(self.cc + [obj, peer, lib, "-o", exe])
            _ = command([exe])
            self.count += 1

        # The same fixture must build with exact-fit storage and fail to
        # compile with one word too few, so the failure is the size check.
        fixture = self.source / "tests" / "integration" / "initializer.c"
        for words, too_small in ((3, False), (2, True)):
            _ = command(
                self.cc
                + flags
                + [
                    f"-DBITMAP_FIXTURE_WORDS={words}",
                    "-c",
                    fixture,
                    "-o",
                    self.work / f"initializer-{words}.o",
                ],
                expect_failure=too_small,
            )
        print("copy-in: BITMAP_INITIALIZER rejects undersized storage")

        assert not (self.work / "bitmap_version.h").exists()
        print(f"copy-in: {self.count} capacities linked against one bitmap.o")

    def subproject(self) -> None:
        parent = self.work / "parent"
        self.copy_library(parent / "subprojects" / "bitmap")
        _ = shutil.copy2(self.consumer(), parent / "consumer.c")
        _ = shutil.copy2(self.peer(), parent / "peer.c")

        capacities = ", ".join(str(capacity) for capacity in CAPACITIES)
        text = (
            "project('bitmap-consumer', 'c',\n"
            "        default_options: ['c_std=c11', 'buildtype=release'])\n"
            "bitmap_dep = subproject('bitmap',\n"
            "    default_options: ['build_tests=false']).get_variable('bitmap_dep')\n"
            "peer = static_library('peer', 'peer.c', dependencies: bitmap_dep,\n"
            f"    c_args: ['-DBITMAP_PEER_BITS={PEER_BITS}'])\n"
            f"foreach cap : [{capacities}]\n"
            "  exe = executable('consumer' + cap.to_string(), 'consumer.c',\n"
            "      link_with: peer, dependencies: bitmap_dep,\n"
            "      c_args: ['-DBITMAP_TEST_BITS=' + cap.to_string()])\n"
            "  test('consumer' + cap.to_string(), exe)\n"
            "endforeach\n"
        )
        _ = (parent / "meson.build").write_text(text)

        build = parent / "build"
        _ = command([self.args.meson, "setup", build, parent], env=self.env)
        _ = command([self.args.meson, "compile", "-C", build], env=self.env)
        _ = command(
            [self.args.meson, "test", "-C", build, "--print-errorlogs"],
            env=self.env,
        )
        self.count += len(CAPACITIES)
        print(f"subproject: {self.count} capacities share one bitmap_dep")

    def installed(self) -> None:
        source = self.work / "library"
        self.copy_library(source)
        build = self.work / "build"
        prefix = self.work / "prefix"
        _ = command(
            [
                self.args.meson,
                "setup",
                build,
                source,
                "--buildtype=release",
                "-Dbuild_tests=false",
                f"--prefix={prefix}",
                "--libdir=lib",
            ],
            env=self.env,
        )
        _ = command([self.args.meson, "compile", "-C", build], env=self.env)
        _ = command([self.args.meson, "install", "-C", build], env=self.env)

        env = dict(self.env, PKG_CONFIG_LIBDIR=str(prefix / "lib" / "pkgconfig"))
        cflags = shlex.split(
            command([self.args.pkg_config, "--cflags", "bitmap"], env=env)
        )
        libs = shlex.split(
            command([self.args.pkg_config, "--libs", "--static", "bitmap"], env=env)
        )

        peer = self.work / "peer.o"
        _ = command(
            self.cc
            + self.flags
            + cflags
            + [f"-DBITMAP_PEER_BITS={PEER_BITS}", "-c", self.peer(), "-o", peer]
        )
        for capacity in CAPACITIES:
            obj = self.work / f"consumer-{capacity}.o"
            _ = command(
                self.cc
                + self.flags
                + cflags
                + [f"-DBITMAP_TEST_BITS={capacity}", "-c", self.consumer(), "-o", obj]
            )
            exe = self.work / f"consumer-{capacity}"
            _ = command(self.cc + [obj, peer] + libs + ["-o", exe])
            _ = command([exe])
            self.count += 1

        print(f"installed: {self.count} capacities via pkg-config")

    def coexistence(self) -> None:
        if not (self.status / "include" / "status.h").exists():
            print("coexistence: second library checkout absent, skipped")
            return

        bitmap_obj = self.work / "bitmap.o"
        _ = command(
            self.cc
            + self.flags
            + [
                "-I" + str(self.source / "include"),
                "-c",
                self.source / "src" / "bitmap.c",
                "-o",
                bitmap_obj,
            ]
        )
        status_obj = self.work / "status.o"
        _ = command(
            self.cc
            + self.flags
            + [
                "-I" + str(self.status / "include"),
                "-c",
                self.status / "src" / "status.c",
                "-o",
                status_obj,
            ]
        )
        fixture = self.source / "tests" / "integration" / "coexistence.c"
        obj = self.work / "coexistence.o"
        _ = command(
            self.cc
            + self.flags
            + [
                "-DBITMAP_COEXISTENCE_STATUS=1",
                "-I" + str(self.source / "include"),
                "-I" + str(self.status / "include"),
                "-c",
                fixture,
                "-o",
                obj,
            ]
        )
        exe = self.work / "coexistence"
        _ = command(self.cc + [obj, bitmap_obj, status_obj, "-o", exe])
        _ = command([exe])
        self.count += 1
        print("coexistence: bitmap + status linked in one program")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    _ = parser.add_argument(
        "mode", choices=("copy-in", "subproject", "installed", "coexistence")
    )
    _ = parser.add_argument("--source-root", type=Path, required=True)
    _ = parser.add_argument("--status-root", type=Path, default=Path())
    _ = parser.add_argument("--meson", required=True)
    _ = parser.add_argument("--pkg-config", required=True)
    _ = parser.add_argument("--cc", nargs="+", required=True)
    args = parser.parse_args(namespace=Arguments())

    with tempfile.TemporaryDirectory(prefix="bitmap-integration-") as directory:
        tests = Tests(args, Path(directory))
        modes = {
            "copy-in": tests.copy_in,
            "subproject": tests.subproject,
            "installed": tests.installed,
            "coexistence": tests.coexistence,
        }
        modes[args.mode]()
        print(f"{args.mode}: ok")


if __name__ == "__main__":
    main()
