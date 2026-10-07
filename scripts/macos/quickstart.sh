#!/usr/bin/env bash
# [macOS] Everything after a fresh clone in one go: setup (Homebrew deps + Qt) →
# release build + tests → .app packaged into a .dmg → launch the app.
#
#   scripts/macos/quickstart.sh            # setup → package → run
#   scripts/macos/quickstart.sh --no-run   # stop after packaging
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,7p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

do_run=1
for arg in "$@"; do
    case "$arg" in
        --no-run)  do_run=0 ;;
        -h|--help) usage; exit 0 ;;
        *) usage; die "unknown argument: $arg" ;;
    esac
done

"$POKESIX_ROOT/scripts/macos/setup.sh"
"$POKESIX_ROOT/scripts/macos/package.sh" # release build + tests + .dmg

app="$POKESIX_ROOT/build/package/PokeSix-macos/PokeSix.app"
if ((do_run)); then
    step "run: $app"
    exec open "$app"
fi
step "ready: $app (install by dragging it onto Applications in the .dmg)"
