# shellcheck shell=bash
# shellcheck disable=SC2034  # 여기서 정의한 변수는 이 파일을 source 하는 스크립트가 사용한다
# scripts/linux/*.sh 공통 설정. 직접 실행하지 않고 source 한다.

if [[ "$(uname -s)" != Linux ]]; then
    echo "error: Linux 전용 스크립트입니다. macOS: scripts/macos/, Windows: scripts\\windows\\" >&2
    exit 1
fi

POKESIX_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Qt 버전은 scripts/QT_VERSION 한 곳에서 관리한다 (환경 변수로 덮어쓸 수 있음)
QT_VERSION="${QT_VERSION:-$(tr -d '[:space:]' <"$POKESIX_ROOT/scripts/QT_VERSION")}"
QT_INSTALL_DIR="${QT_INSTALL_DIR:-$HOME/Qt}"
QT_AQT_ARCH=linux_gcc_64
QT_DEFAULT_DIR="$QT_INSTALL_DIR/$QT_VERSION/gcc_64"

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
# Qt 설치 폴더로 보이는가 (lib/cmake/Qt6/Qt6Config.cmake 존재)
is_qt_dir() { [[ -n "$1" && -f "$1/lib/cmake/Qt6/Qt6Config.cmake" ]]; }

# QT_ROOT_DIR 을 확정해 export 한다: 환경 변수 → 기본 설치 위치 순
resolve_qt() {
    if [[ -n "${QT_ROOT_DIR:-}" ]]; then
        is_qt_dir "$QT_ROOT_DIR" \
            || die "QT_ROOT_DIR=$QT_ROOT_DIR 에 Qt 가 없습니다 (lib/cmake/Qt6 확인). docs/build.md §1"
    else
        is_qt_dir "$QT_DEFAULT_DIR" \
            || die "Qt 를 찾을 수 없습니다. 먼저 scripts/linux/setup.sh 를 실행하거나 QT_ROOT_DIR 을 설정하세요."
        QT_ROOT_DIR="$QT_DEFAULT_DIR"
    fi
    export QT_ROOT_DIR
}

# ── 프리셋 · 경로 ──────────────────────────────────────────────
# preset_for debug|release → linux-debug / linux-release
preset_for() {
    case "$1" in
        debug|release) printf 'linux-%s' "$1" ;;
        *) die "빌드 구성은 debug 또는 release 입니다: '$1'" ;;
    esac
}

app_binary() { printf '%s/build/%s/src/PokeSix' "$POKESIX_ROOT" "$1"; }
