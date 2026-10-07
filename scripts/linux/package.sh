#!/usr/bin/env bash
# [Linux] Package a release build as an AppImage.
#
#   scripts/linux/package.sh              # release build (with tests) → AppImage
#   scripts/linux/package.sh --no-build   # reuse the existing release build
#   scripts/linux/package.sh --install    # … and register it in the application menu:
#                                         # ~/.local/bin/PokeSix.AppImage + .desktop + icons
#
# Output: build/package/PokeSix-<version>-x86_64.AppImage
# linuxdeploy and its Qt plugin are downloaded once into build/package/tools.
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,8p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

do_build=1
do_install=0
for arg in "$@"; do
    case "$arg" in
        --no-build) do_build=0 ;;
        --install)  do_install=1 ;;
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
# platformthemes are also copied by hand, WITHOUT walking their dependencies: libqgtk3.so
# links the host's GTK3 (bundling GTK into an AppImage is asking for theme breakage), and on
# a host without it Qt just falls back to its own file dialog. With it, file dialogs are the
# desktop's native ones instead of Qt's built-in fallback.
(cd "$package_dir" \
 && export QMAKE="$QT_ROOT_DIR/bin/qmake" APPIMAGE_EXTRACT_AND_RUN=1 \
           EXTRA_PLUGINS="tls;networkinformation" OUTPUT="$output.part" VERSION="$version" \
 && "$tools/linuxdeploy-x86_64.AppImage" --appdir AppDir \
 && "$tools/linuxdeploy-plugin-qt-x86_64.AppImage" --appdir AppDir \
        --exclude-library "*qsqlmimer*" --exclude-library "*qsqlodbc*" \
        --exclude-library "*qsqlpsql*" \
 && cp "$QT_ROOT_DIR/plugins/platforms/libqoffscreen.so" AppDir/usr/plugins/platforms/ \
 && mkdir -p AppDir/usr/plugins/platformthemes \
 && cp "$QT_ROOT_DIR"/plugins/platformthemes/*.so AppDir/usr/plugins/platformthemes/ \
 && rm -f AppDir/usr/plugins/sqldrivers/libqsqlmimer.so \
          AppDir/usr/plugins/sqldrivers/libqsqlodbc.so \
          AppDir/usr/plugins/sqldrivers/libqsqlpsql.so \
          AppDir/usr/plugins/sqldrivers/libqsqlmysql.so \
 && "$tools/linuxdeploy-x86_64.AppImage" --appdir AppDir --output appimage \
 && mv -f "$output.part" "$output") # 임시 이름 → 바꿔치기: 앱 메뉴에서 실행 중이어도 패키징된다

step "done"
sha256sum "$package_dir/$output"

if ((do_install)); then
    # 앱 메뉴 등록: AppImage를 버전 없는 이름으로 ~/.local/bin에 두고(.desktop이 버전을 몰라도
    # 되게), .desktop의 Exec를 절대 경로로 바꿔 설치한다 — 옛 `--install ~/.local` 설치가 남긴
    # 항목(Exec=PokeSix, PATH의 옛 바이너리)을 덮어쓴다.
    apps_dir="$HOME/.local/share/applications"
    icons_dir="$HOME/.local/share/icons"
    installed="$HOME/.local/bin/PokeSix.AppImage"
    step "application menu → $installed"
    install -D -m 755 "$package_dir/$output" "$installed"
    for icon in "$POKESIX_ROOT"/resources/icons/app/linux/hicolor/*/apps/pokesix.png; do
        size="$(basename "$(dirname "$(dirname "$icon")")")"
        install -D -m 644 "$icon" "$icons_dir/hicolor/$size/apps/pokesix.png"
    done
    mkdir -p "$apps_dir"
    sed "s|^Exec=.*|Exec=$installed|" \
        "$POKESIX_ROOT/resources/platform/linux/com.yamada.studio.pokesix.desktop" \
        > "$apps_dir/com.yamada.studio.pokesix.desktop"
    command -v update-desktop-database >/dev/null && update-desktop-database "$apps_dir" || true
    if [[ -x "$HOME/.local/bin/PokeSix" ]]; then
        warn "old install found: ~/.local/bin/PokeSix — the menu no longer uses it." \
             "Remove it with: rm ~/.local/bin/PokeSix"
    fi
fi
