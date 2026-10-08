#!/usr/bin/env bash
set -eu

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build}"
CONFIGURATION="${CONFIGURATION:-Release}"

if command -v cmake >/dev/null 2>&1 && command -v ctest >/dev/null 2>&1; then
    CMAKE="$(command -v cmake)"
    CTEST="$(command -v ctest)"
elif command -v cmake.exe >/dev/null 2>&1 && command -v ctest.exe >/dev/null 2>&1; then
    CMAKE="$(command -v cmake.exe)"
    CTEST="$(command -v ctest.exe)"
else
    echo "error: cmake and ctest were not found in PATH." >&2
    echo "Install CMake in this shell or run the equivalent commands from Windows PowerShell." >&2
    exit 127
fi

if [[ "$CMAKE" == *.exe ]]; then
    if ! command -v wslpath >/dev/null 2>&1; then
        echo "error: wslpath is required when using Windows CMake from WSL." >&2
        exit 127
    fi
    CMAKE_ROOT="$(wslpath -w "$ROOT")"
    CMAKE_BUILD="$(wslpath -w "$BUILD")"
else
    CMAKE_ROOT="$ROOT"
    CMAKE_BUILD="$BUILD"
fi

"$CMAKE" -S "$CMAKE_ROOT" -B "$CMAKE_BUILD" -DBUILD_TESTING=ON
"$CMAKE" --build "$CMAKE_BUILD" --config "$CONFIGURATION"
"$CTEST" --test-dir "$CMAKE_BUILD" -C "$CONFIGURATION" --output-on-failure

echo "=== bench_lpm"
BENCH="$BUILD/bench_lpm"
if [ ! -f "$BENCH" ] && [ -f "$BUILD/bench_lpm.exe" ]; then
    BENCH="$BUILD/bench_lpm.exe"
fi
if [ ! -f "$BENCH" ] && [ -f "$BUILD/$CONFIGURATION/bench_lpm" ]; then
    BENCH="$BUILD/$CONFIGURATION/bench_lpm"
fi
if [ ! -f "$BENCH" ] && [ -f "$BUILD/$CONFIGURATION/bench_lpm.exe" ]; then
    BENCH="$BUILD/$CONFIGURATION/bench_lpm.exe"
fi

if [ ! -f "$BENCH" ]; then
    echo "error: bench_lpm was not produced at '$BENCH'." >&2
    exit 1
fi

"$BENCH" 5000 1
