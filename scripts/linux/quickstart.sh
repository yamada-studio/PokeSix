#!/usr/bin/env bash
# [Linux] Everything after a fresh clone in one go:
# setup (dependencies + Qt) → release build + tests → AppImage → run it.
#
#   scripts/linux/quickstart.sh            # setup → package → run
#   scripts/linux/quickstart.sh --no-run   # stop after packaging
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

"$POKESIX_ROOT/scripts/linux/setup.sh"
"$POKESIX_ROOT/scripts/linux/package.sh" # release build + tests + AppImage

version="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9.]*\)$/\1/p' "$POKESIX_ROOT/CMakeLists.txt" | head -1)"
appimage="$POKESIX_ROOT/build/package/PokeSix-$version-x86_64.AppImage"
if ((do_run)); then
    step "run: $appimage"
    exec "$appimage"
fi
step "ready: $appimage"
