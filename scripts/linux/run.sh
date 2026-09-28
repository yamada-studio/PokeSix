#!/usr/bin/env bash
# [Linux] PokeSix 실행. 빌드된 실행 파일이 없으면 먼저 빌드한다.
#
#   scripts/linux/run.sh                      # debug 빌드 실행
#   scripts/linux/run.sh release
#   scripts/linux/run.sh --rebuild            # 실행 전에 항상 빌드 (테스트 생략)
#   scripts/linux/run.sh --log                # pokesix.* debug 로그 켜기
#   scripts/linux/run.sh --gdb                # gdb 에서 실행
#   scripts/linux/run.sh --offscreen          # 창 없이 실행 (CI, SSH, 캡처)
#   scripts/linux/run.sh -- --screenshot home 1440x900 out.png   # -- 뒤는 앱 인자
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,10p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

config=debug
rebuild=0
debugger=0
app_args=()

while (($#)); do
    case "$1" in
        debug|release) config="$1" ;;
        --rebuild)     rebuild=1 ;;
        --log)         export QT_LOGGING_RULES="${QT_LOGGING_RULES:+$QT_LOGGING_RULES;}pokesix.*.debug=true" ;;
        --gdb)         debugger=1 ;;
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
    "$POKESIX_ROOT/scripts/linux/build.sh" "$config" --no-test
fi
[[ -x "$binary" ]] || die "실행 파일이 없습니다: $binary"

step "run: ${binary#"$POKESIX_ROOT"/} ${app_args[*]:-}"
if ((debugger)); then
    command -v gdb >/dev/null || die "gdb 가 없습니다 (scripts/linux/setup.sh)"
    exec gdb --args "$binary" "${app_args[@]}"
fi
exec "$binary" "${app_args[@]}"
