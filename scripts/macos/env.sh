# shellcheck shell=bash
# shellcheck disable=SC2034  # variables defined here are used by the scripts that source this file
# Shared settings for scripts/macos/*.sh. Source it; do not execute it directly.
#
# The default bash on macOS is 3.2. Scripts in this folder must work on bash 3.2:
#   - no mapfile / readarray / associative arrays
#   - with set -u, expand possibly-empty arrays as ${arr[@]+"${arr[@]}"}

if [[ "$(uname -s)" != Darwin ]]; then
    echo "error: this script is for macOS only. Linux: scripts/linux/, Windows: scripts\\windows\\" >&2
    exit 1
fi

POKESIX_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# The Qt version lives in one place, scripts/QT_VERSION (can be overridden by the environment)
QT_VERSION="${QT_VERSION:-$(tr -d '[:space:]' <"$POKESIX_ROOT/scripts/QT_VERSION")}"
QT_INSTALL_DIR="${QT_INSTALL_DIR:-$HOME/Qt}"
QT_AQT_ARCH=clang_64                        # Intel + Apple Silicon universal
QT_DEFAULT_DIR="$QT_INSTALL_DIR/$QT_VERSION/macos"

# ── Output ────────────────────────────────────────────────────
if [[ -t 1 ]]; then
    _c_blue=$'\e[1;34m'; _c_yellow=$'\e[1;33m'; _c_red=$'\e[1;31m'; _c_reset=$'\e[0m'
else
    _c_blue=''; _c_yellow=''; _c_red=''; _c_reset=''
fi
step() { printf '%s==>%s %s\n' "$_c_blue" "$_c_reset" "$*"; }
warn() { printf '%swarn:%s %s\n' "$_c_yellow" "$_c_reset" "$*" >&2; }
die()  { printf '%serror:%s %s\n' "$_c_red" "$_c_reset" "$*" >&2; exit 1; }

# ── Qt ────────────────────────────────────────────────────────
is_qt_dir() { [[ -n "$1" && -f "$1/lib/cmake/Qt6/Qt6Config.cmake" ]]; }

# Resolve and export QT_ROOT_DIR: environment variable → aqtinstall default location → Homebrew
resolve_qt() {
    if [[ -n "${QT_ROOT_DIR:-}" ]]; then
        is_qt_dir "$QT_ROOT_DIR" \
            || die "QT_ROOT_DIR=$QT_ROOT_DIR does not contain Qt (check lib/cmake/Qt6). See docs/build.md §1"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        QT_ROOT_DIR="$QT_DEFAULT_DIR"
    elif command -v brew >/dev/null && is_qt_dir "$(brew --prefix qt 2>/dev/null)"; then
        QT_ROOT_DIR="$(brew --prefix qt)"
        warn "using Homebrew Qt: $QT_ROOT_DIR (its version may differ from $QT_VERSION)"
    else
        die "Qt not found. Run scripts/macos/setup.sh first, or set QT_ROOT_DIR."
    fi
    export QT_ROOT_DIR
}

# ── Presets and paths ─────────────────────────────────────────
preset_for() {
    case "$1" in
        debug|release) printf 'macos-%s' "$1" ;;
        *) die "build configuration must be 'debug' or 'release': '$1'" ;;
    esac
}

# MACOSX_BUNDLE: the executable lives inside the .app bundle
app_bundle() { printf '%s/build/%s/src/PokeSix.app' "$POKESIX_ROOT" "$1"; }
app_binary() { printf '%s/Contents/MacOS/PokeSix' "$(app_bundle "$1")"; }
