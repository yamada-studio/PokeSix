# shellcheck shell=bash
# shellcheck disable=SC2034  # 여기서 정의한 변수는 이 파일을 source 하는 스크립트가 사용한다
# scripts/macos/*.sh 공통 설정. 직접 실행하지 않고 source 한다.
#
# macOS 기본 bash 는 3.2 다. 이 폴더의 스크립트는 bash 3.2 에서 동작해야 한다:
#   - mapfile / readarray / 연관 배열 금지
#   - set -u 에서 빈 배열은 ${arr[@]+"${arr[@]}"} 로 전개

if [[ "$(uname -s)" != Darwin ]]; then
    echo "error: macOS 전용 스크립트입니다. Linux: scripts/linux/, Windows: scripts\\windows\\" >&2
    exit 1
fi

POKESIX_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Qt 버전은 scripts/QT_VERSION 한 곳에서 관리한다 (환경 변수로 덮어쓸 수 있음)
QT_VERSION="${QT_VERSION:-$(tr -d '[:space:]' <"$POKESIX_ROOT/scripts/QT_VERSION")}"
QT_INSTALL_DIR="${QT_INSTALL_DIR:-$HOME/Qt}"
QT_AQT_ARCH=clang_64                        # Intel + Apple Silicon universal
QT_DEFAULT_DIR="$QT_INSTALL_DIR/$QT_VERSION/macos"

# ── 출력 ──────────────────────────────────────────────────────
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

# QT_ROOT_DIR 을 확정해 export 한다: 환경 변수 → aqtinstall 기본 위치 → Homebrew 순
resolve_qt() {
    if [[ -n "${QT_ROOT_DIR:-}" ]]; then
        is_qt_dir "$QT_ROOT_DIR" \
            || die "QT_ROOT_DIR=$QT_ROOT_DIR 에 Qt 가 없습니다 (lib/cmake/Qt6 확인). docs/build.md §1"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        QT_ROOT_DIR="$QT_DEFAULT_DIR"
    elif command -v brew >/dev/null && is_qt_dir "$(brew --prefix qt 2>/dev/null)"; then
        QT_ROOT_DIR="$(brew --prefix qt)"
        warn "Homebrew Qt 사용: $QT_ROOT_DIR (버전이 $QT_VERSION 과 다를 수 있음)"
    else
        die "Qt 를 찾을 수 없습니다. 먼저 scripts/macos/setup.sh 를 실행하거나 QT_ROOT_DIR 을 설정하세요."
    fi
    export QT_ROOT_DIR
}

# ── 프리셋 · 경로 ──────────────────────────────────────────────
preset_for() {
    case "$1" in
        debug|release) printf 'macos-%s' "$1" ;;
        *) die "빌드 구성은 debug 또는 release 입니다: '$1'" ;;
    esac
}

# MACOSX_BUNDLE 이므로 실행 파일은 .app 번들 안에 있다
app_bundle() { printf '%s/build/%s/src/PokeSix.app' "$POKESIX_ROOT" "$1"; }
app_binary() { printf '%s/Contents/MacOS/PokeSix' "$(app_bundle "$1")"; }
