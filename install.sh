#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 0 ]]; then
    printf 'Usage: INSTALL_PREFIX=... CMAKE_PREFIX_PATH=... ./install.sh\n' >&2
    exit 2
fi

source_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$source_dir/build"
install_prefix="${INSTALL_PREFIX:-$HOME/.local/SDK/iiVoiceOver}"
search_prefixes="${CMAKE_PREFIX_PATH:-}"
parallel_jobs="${CMAKE_BUILD_PARALLEL_LEVEL:-8}"
cmake -S "$source_dir" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DIIVOICEOVER_DOWNLOAD_VOICE=ON \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=TRUE \
    "-DCMAKE_INSTALL_PREFIX=$install_prefix" "-DCMAKE_PREFIX_PATH=$search_prefixes"
cmake --build "$build_dir" --config Release --parallel "$parallel_jobs"
ctest --test-dir "$build_dir" -C Release --output-on-failure
cmake --install "$build_dir" --config Release

consumer_build_dir="$build_dir/consumer/build"
cmake -S "$source_dir/tests/consumer" -B "$consumer_build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_DISABLE_FIND_PACKAGE_Qt6=TRUE -DCMAKE_DISABLE_FIND_PACKAGE_Qt5=TRUE \
    "-DiiVoiceOver_DIR=$install_prefix/lib/cmake/iiVoiceOver" \
    "-DCMAKE_PREFIX_PATH=$install_prefix${search_prefixes:+;$search_prefixes}"
cmake --build "$consumer_build_dir" --config Release --parallel "$parallel_jobs"
ctest --test-dir "$consumer_build_dir" -C Release --output-on-failure
