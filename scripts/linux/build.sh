#!/usr/bin/env bash
# [Linux] configure → build → test (→ install) in one go.
#
#   scripts/linux/build.sh                    # debug: configure + build + test
#   scripts/linux/build.sh release            # release
#   scripts/linux/build.sh --clean            # delete the build directory and start over
#   scripts/linux/build.sh --no-test          # skip tests
#   scripts/linux/build.sh --tidy             # build with clang-tidy enabled
#   scripts/linux/build.sh --format           # check clang-format before building (fails on violations)
#   scripts/linux/build.sh release --install ~/.local
#
# Build directory: build/linux-<debug|release>  (same as CMakePresets.json)
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,12p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

config=debug
clean=0
run_tests=1
tidy=OFF
check_format=0
install_prefix=""

while (($#)); do
    case "$1" in
        debug|release) config="$1" ;;
        --clean)       clean=1 ;;
        --no-test)     run_tests=0 ;;
        --tidy)        tidy=ON ;;
        --format)      check_format=1 ;;
        --install)
            (($# >= 2)) || die "--install needs an install path"
            install_prefix="$2"; shift ;;
        -h|--help)     usage; exit 0 ;;
        *) usage; die "unknown argument: $1" ;;
    esac
    shift
done

preset="$(preset_for "$config")"
build_dir="$POKESIX_ROOT/build/$preset"
resolve_qt
cd "$POKESIX_ROOT"

step "preset=$preset  Qt=$QT_ROOT_DIR"

if ((check_format)); then
    step "checking clang-format"
    command -v clang-format >/dev/null || die "clang-format not found (scripts/linux/setup.sh)"
    sources=()
    while IFS= read -r f; do sources+=("$f"); done \
        < <(git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')
    clang-format --dry-run --Werror "${sources[@]}"
fi

if ((clean)) && [[ -d "$build_dir" ]]; then
    step "clean: $build_dir"
    rm -rf "$build_dir"
fi

step "configure"
cmake --preset "$preset" -DPOKESIX_ENABLE_CLANG_TIDY="$tidy"

step "build"
cmake --build --preset "$preset"

if ((run_tests)); then
    step "test"
    ctest --preset "$preset"
fi

if [[ -n "$install_prefix" ]]; then
    step "install → $install_prefix"
    cmake --install "$build_dir" --prefix "$install_prefix"
fi

step "done: $(app_binary "$preset")"
