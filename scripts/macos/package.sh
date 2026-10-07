#!/usr/bin/env bash
# [macOS] Package a release build as a .dmg.
#
#   scripts/macos/package.sh              # release build (with tests) → .dmg
#   scripts/macos/package.sh --no-build   # reuse the existing release build
#
# Output: build/package/PokeSix-<version>-macos.dmg
# The install step runs macdeployqt (qt_generate_deploy_app_script in src/CMakeLists.txt),
# so the .app carries the Qt frameworks and runs on a Mac without Qt.
# The app is ad-hoc signed, not notarized (no Apple Developer account). macOS quarantines a
# downloaded dmg and blocks the app as "damaged" — right-click → Open does NOT bypass this for
# unsigned/ad-hoc apps. The receiver clears the flag once (documented in README "Download"):
#   xattr -d com.apple.quarantine ~/Downloads/PokeSix-<version>-macos.dmg
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,9p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

do_build=1
for arg in "$@"; do
    case "$arg" in
        --no-build) do_build=0 ;;
        -h|--help)  usage; exit 0 ;;
        *) usage; die "unknown argument: $arg" ;;
    esac
done

version="$(sed -n 's/^[[:space:]]*VERSION[[:space:]]*\([0-9.]*\)$/\1/p' "$POKESIX_ROOT/CMakeLists.txt" | head -1)"
[[ -n "$version" ]] || die "cannot read VERSION from CMakeLists.txt"

if ((do_build)); then
    "$POKESIX_ROOT/scripts/macos/build.sh" release
fi

package_dir="$POKESIX_ROOT/build/package"
staging="$package_dir/PokeSix-macos"
step "install + macdeployqt → $staging"
rm -rf "$staging"
cmake --install "$POKESIX_ROOT/build/macos-release" --prefix "$staging"
[[ -d "$staging/PokeSix.app" ]] || die "PokeSix.app missing after install"
# Ship the licenses with the app (MIT + Qt LGPL / font notices)
cp "$POKESIX_ROOT/LICENSE" "$POKESIX_ROOT/THIRD_PARTY_NOTICES.md" "$staging/"
# Finder convention: drag the app onto this link to install
ln -sfn /Applications "$staging/Applications"

# macdeployqt rewrites install names inside the bundle, which can leave stale ad-hoc
# signatures behind (arm64 refuses to start those at all). Re-sign the whole bundle with a
# fresh ad-hoc signature. This does not silence Gatekeeper (see the header note) — it only
# guarantees the app launches once the quarantine flag is cleared.
step "ad-hoc codesign"
codesign --force --deep --sign - "$staging/PokeSix.app"
codesign --verify --deep --strict "$staging/PokeSix.app"

output="$package_dir/PokeSix-$version-macos.dmg"
step "dmg → $output"
rm -f "$output"
hdiutil create -volname "PokeSix" -srcfolder "$staging" -ov -format UDZO "$output"

step "done"
shasum -a 256 "$output"
