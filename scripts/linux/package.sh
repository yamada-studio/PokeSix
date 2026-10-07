#!/usr/bin/env bash
# [Linux] Package a release build as an AppImage.
#
#   scripts/linux/package.sh              # release build (with tests) → AppImage
#   scripts/linux/package.sh --no-build   # reuse the existing release build
#
# Output: build/package/PokeSix-<version>-x86_64.AppImage
# linuxdeploy and its Qt plugin are downloaded once into build/package/tools.
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,8p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

do_build=1
for arg in "$@"; do
    case "$arg" in
        --no-build) do_build=0 ;;
        -h|--help)  usage; exit 0 ;;
        *) usage; die "unknown argument: $arg" ;;
    esac
done

# The version lives in one place: project(VERSION x.y.z) in CMakeLists.txt
version="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9.]*\)$/\1/p' "$POKESIX_ROOT/CMakeLists.txt" | head -1)"
[[ -n "$version" ]] || die "cannot read VERSION from CMakeLists.txt"

if ((do_build)); then
    "$POKESIX_ROOT/scripts/linux/build.sh" release
fi
[[ -x "$POKESIX_ROOT/build/linux-release/src/PokeSix" ]] \
    || die "no release build. Run scripts/linux/build.sh release first (or drop --no-build)."

resolve_qt
package_dir="$POKESIX_ROOT/build/package"
appdir="$package_dir/AppDir"
tools="$package_dir/tools"

step "install → $appdir"
rm -rf "$appdir"
cmake --install "$POKESIX_ROOT/build/linux-release" --prefix "$appdir/usr"

step "linuxdeploy tools → $tools"
mkdir -p "$tools"
fetch() { # <file> <url> — keep what is already there (downloaded once)
    [[ -x "$tools/$1" ]] && return 0
    curl -L --fail -o "$tools/$1" "$2" || die "download failed: $2"
    chmod +x "$tools/$1"
}
fetch linuxdeploy-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
fetch linuxdeploy-plugin-qt-x86_64.AppImage \
    https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage

output="PokeSix-$version-x86_64.AppImage"
step "AppImage → build/package/$output"
# Three steps so the qt plugin can take --exclude-library (running it via --plugin qt cannot
# pass arguments): base AppDir → Qt runtime → AppImage.
# APPIMAGE_EXTRACT_AND_RUN: the tools are AppImages themselves — run without FUSE.
# EXTRA_PLUGINS: the qt plugin does not detect the TLS backends (HTTPS for the first-run
# data download) or networkinformation on its own.
# --exclude-library: we only use SQLite; qsqlmimer/odbc/psql drag in client libraries that
# are not installed (libmimerapi.so …) and abort the deployment. The exclusion skips their
# dependencies but still copies the driver .so — the rm below drops those dead files.
# libqoffscreen.so (depends only on QtGui, already bundled) lets the AppImage run headless
# (CI · screenshots); EXTRA_PLUGINS cannot add single files, so it is copied by hand.
(cd "$package_dir" \
 && export QMAKE="$QT_ROOT_DIR/bin/qmake" APPIMAGE_EXTRACT_AND_RUN=1 \
           EXTRA_PLUGINS="tls;networkinformation" OUTPUT="$output" VERSION="$version" \
 && "$tools/linuxdeploy-x86_64.AppImage" --appdir AppDir \
 && "$tools/linuxdeploy-plugin-qt-x86_64.AppImage" --appdir AppDir \
        --exclude-library "*qsqlmimer*" --exclude-library "*qsqlodbc*" \
        --exclude-library "*qsqlpsql*" \
 && cp "$QT_ROOT_DIR/plugins/platforms/libqoffscreen.so" AppDir/usr/plugins/platforms/ \
 && rm -f AppDir/usr/plugins/sqldrivers/libqsqlmimer.so \
          AppDir/usr/plugins/sqldrivers/libqsqlodbc.so \
          AppDir/usr/plugins/sqldrivers/libqsqlpsql.so \
          AppDir/usr/plugins/sqldrivers/libqsqlmysql.so \
 && "$tools/linuxdeploy-x86_64.AppImage" --appdir AppDir --output appimage)

step "done"
sha256sum "$package_dir/$output"
