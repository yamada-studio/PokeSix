#!/usr/bin/env bash
# [Linux] Run PokeSix. Builds first if the executable does not exist yet.
#
#   scripts/linux/run.sh                      # run the debug build
#   scripts/linux/run.sh release
#   scripts/linux/run.sh --rebuild            # always build before running (tests skipped)
#   scripts/linux/run.sh --log                # enable pokesix.* debug logs
#   scripts/linux/run.sh --gdb                # run under gdb
#   scripts/linux/run.sh --offscreen          # run without a window (CI, SSH, screenshots)
#   scripts/linux/run.sh -- --screenshot home 1440x900 out.png   # everything after -- goes to the app
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
        *) usage; die "unknown argument: $1 (app arguments go after --)" ;;
    esac
    shift
done

preset="$(preset_for "$config")"
binary="$(app_binary "$preset")"

if ((rebuild)) || [[ ! -x "$binary" ]]; then
    "$POKESIX_ROOT/scripts/linux/build.sh" "$config" --no-test
fi
[[ -x "$binary" ]] || die "executable not found: $binary"

step "run: ${binary#"$POKESIX_ROOT"/} ${app_args[*]:-}"
if ((debugger)); then
    command -v gdb >/dev/null || die "gdb not found (scripts/linux/setup.sh)"
    exec gdb --args "$binary" "${app_args[@]}"
fi
exec "$binary" "${app_args[@]}"
