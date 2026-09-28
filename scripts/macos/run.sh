#!/usr/bin/env bash
# [macOS] PokeSix 실행. 빌드된 .app 이 없으면 먼저 빌드한다.
#
#   scripts/macos/run.sh                      # debug 빌드 실행 (터미널에 로그 출력)
#   scripts/macos/run.sh release
#   scripts/macos/run.sh --rebuild            # 실행 전에 항상 빌드 (테스트 생략)
#   scripts/macos/run.sh --log                # pokesix.* debug 로그 켜기
#   scripts/macos/run.sh --lldb               # lldb 에서 실행
#   scripts/macos/run.sh --open               # Finder 처럼 .app 으로 실행 (로그는 Console.app)
#   scripts/macos/run.sh --offscreen          # 창 없이 실행 (CI, 캡처)
#   scripts/macos/run.sh -- --screenshot home 1440x900 out.png   # -- 뒤는 앱 인자
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,11p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

config=debug
rebuild=0
mode=direct          # direct | lldb | open
app_args=()

while (($#)); do
    case "$1" in
        debug|release) config="$1" ;;
        --rebuild)     rebuild=1 ;;
        --log)         export QT_LOGGING_RULES="${QT_LOGGING_RULES:+$QT_LOGGING_RULES;}pokesix.*.debug=true" ;;
        --lldb)        mode=lldb ;;
        --open)        mode=open ;;
        --offscreen)   export QT_QPA_PLATFORM=offscreen ;;
        --)            shift; app_args=("$@"); break ;;
        -h|--help)     usage; exit 0 ;;
        *) usage; die "알 수 없는 인자: $1 (앱 인자는 -- 뒤에)" ;;
    esac
    shift
done

preset="$(preset_for "$config")"
binary="$(app_binary "$preset")"

if ((rebuild)) || [[ ! -x "$binary" ]]; then
    "$POKESIX_ROOT/scripts/macos/build.sh" "$config" --no-test
fi
[[ -x "$binary" ]] || die "실행 파일이 없습니다: $binary"

step "run ($mode): ${binary#"$POKESIX_ROOT"/} ${app_args[*]:-}"
# ${a[@]+"${a[@]}"}: bash 3.2 + set -u 에서 빈 배열을 안전하게 전개
case "$mode" in
    lldb) exec lldb -- "$binary" ${app_args[@]+"${app_args[@]}"} ;;
    open) exec open -W "$(app_bundle "$preset")" --args ${app_args[@]+"${app_args[@]}"} ;;
    *)    exec "$binary" ${app_args[@]+"${app_args[@]}"} ;;
esac
