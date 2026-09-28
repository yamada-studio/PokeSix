#!/usr/bin/env bash
# [macOS] configure → build → test (→ install) 한 번에.
#
#   scripts/macos/build.sh                    # debug: configure + build + test
#   scripts/macos/build.sh release            # release
#   scripts/macos/build.sh --clean            # 빌드 폴더를 지우고 처음부터
#   scripts/macos/build.sh --no-test          # 테스트 생략
#   scripts/macos/build.sh --tidy             # clang-tidy 켜고 빌드 (brew install llvm 필요)
#   scripts/macos/build.sh --format           # 빌드 전에 clang-format 검사 (위반 시 실패)
#   scripts/macos/build.sh release --install ~/Applications
#
# 빌드 폴더: build/macos-<debug|release>  (CMakePresets.json 과 같음)
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
            (($# >= 2)) || die "--install 뒤에 설치 경로가 필요합니다"
            install_prefix="$2"; shift ;;
        -h|--help)     usage; exit 0 ;;
        *) usage; die "알 수 없는 인자: $1" ;;
    esac
    shift
done

preset="$(preset_for "$config")"
build_dir="$POKESIX_ROOT/build/$preset"
resolve_qt
cd "$POKESIX_ROOT"

step "preset=$preset  Qt=$QT_ROOT_DIR"

if ((check_format)); then
    step "clang-format 검사"
    command -v clang-format >/dev/null || die "clang-format 이 없습니다 (scripts/macos/setup.sh)"
    sources=()   # bash 3.2 에는 mapfile 이 없다
    while IFS= read -r f; do sources+=("$f"); done \
        < <(git ls-files --cached --others --exclude-standard -- 'src/*.cpp' 'src/*.h' 'tests/*.cpp' 'tests/*.h')
    clang-format --dry-run --Werror ${sources[@]+"${sources[@]}"}
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

step "완료: $(app_bundle "$preset")"
