#!/usr/bin/env bash
# [macOS] Run PokeSix. Builds first if the .app bundle does not exist yet.
#
#   scripts/macos/run.sh                      # run the debug build (logs go to this terminal)
#   scripts/macos/run.sh release
#   scripts/macos/run.sh --rebuild            # always build before running (tests skipped)
#   scripts/macos/run.sh --log                # enable pokesix.* debug logs
#   scripts/macos/run.sh --lldb               # run under lldb
#   scripts/macos/run.sh --open               # launch the .app like Finder does (logs go to Console.app)
#   scripts/macos/run.sh --offscreen          # run without a window (CI, screenshots)
#   scripts/macos/run.sh -- --screenshot home 1440x900 out.png   # everything after -- goes to the app
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
        *) usage; die "unknown argument: $1 (app arguments go after --)" ;;
    esac
    shift
done

preset="$(preset_for "$config")"
binary="$(app_binary "$preset")"

if ((rebuild)) || [[ ! -x "$binary" ]]; then
    "$POKESIX_ROOT/scripts/macos/build.sh" "$config" --no-test
fi
[[ -x "$binary" ]] || die "executable not found: $binary"

step "run ($mode): ${binary#"$POKESIX_ROOT"/} ${app_args[*]:-}"
# ${a[@]+"${a[@]}"}: expands a possibly-empty array safely under bash 3.2 + set -u
case "$mode" in
    lldb) exec lldb -- "$binary" ${app_args[@]+"${app_args[@]}"} ;;
    open) exec open -W "$(app_bundle "$preset")" --args ${app_args[@]+"${app_args[@]}"} ;;
    *)    exec "$binary" ${app_args[@]+"${app_args[@]}"} ;;
esac
