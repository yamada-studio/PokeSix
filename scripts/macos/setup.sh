#!/usr/bin/env bash
# [macOS] 외부 의존성 한 번에 설치 — Xcode Command Line Tools + Homebrew.
#
#   scripts/macos/setup.sh              # Homebrew 패키지 + Qt
#   scripts/macos/setup.sh --no-system  # Qt 만
#   scripts/macos/setup.sh --no-qt      # Homebrew 패키지만
#
# 이미 설치된 것은 건너뛴다. 여러 번 실행해도 안전하다.
set -euo pipefail
# shellcheck source=env.sh
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"

usage() { sed -n '2,8p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; }

do_system=1
do_qt=1
for arg in "$@"; do
    case "$arg" in
        --no-system) do_system=0 ;;
        --no-qt)     do_qt=0 ;;
        -h|--help)   usage; exit 0 ;;
        *) usage; die "알 수 없는 인자: $arg" ;;
    esac
done

# ── 1. Xcode CLT · Homebrew 패키지 ─────────────────────────────
if ((do_system)); then
    if ! xcode-select -p >/dev/null 2>&1; then
        step "Xcode Command Line Tools 설치 창을 엽니다. 설치가 끝나면 이 스크립트를 다시 실행하세요."
        xcode-select --install || true
        exit 1
    fi
    command -v brew >/dev/null || die "Homebrew 가 필요합니다: https://brew.sh"

    pkgs="cmake ninja clang-format uv"          # git 은 Xcode CLT 에 포함, uv 는 aqtinstall 실행용
    missing=""
    for p in $pkgs; do
        brew list --formula "$p" >/dev/null 2>&1 || missing="$missing $p"
    done

    if [[ -z "$missing" ]]; then
        step "Homebrew 패키지: 모두 설치되어 있음"
    else
        step "Homebrew 패키지 설치:$missing"
        # shellcheck disable=SC2086  # 공백으로 나눠 여러 인자로 넘기는 것이 의도
        brew install $missing
    fi
fi

# ── 2. Qt (aqtinstall) ─────────────────────────────────────────
# Homebrew 의 qt 는 항상 최신이라 버전을 고정할 수 없으므로 aqtinstall 을 쓴다
if ((do_qt)); then
    if [[ -n "${QT_ROOT_DIR:-}" ]] && is_qt_dir "$QT_ROOT_DIR"; then
        step "Qt: QT_ROOT_DIR 사용 ($QT_ROOT_DIR)"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        step "Qt $QT_VERSION 이미 설치됨: $QT_DEFAULT_DIR"
    else
        command -v uvx >/dev/null || die "uv 가 없습니다. --no-system 없이 다시 실행하거나 'brew install uv'"
        step "Qt $QT_VERSION 설치 → $QT_INSTALL_DIR"
        # aqt 는 실행 위치에 aqtinstall.log 를 남기므로 임시 폴더에서 실행한다
        (cd "${TMPDIR:-/tmp}" && uvx --from aqtinstall aqt install-qt mac desktop "$QT_VERSION" "$QT_AQT_ARCH" \
            --outputdir "$QT_INSTALL_DIR")
        is_qt_dir "$QT_DEFAULT_DIR" || die "설치 후에도 $QT_DEFAULT_DIR 에서 Qt 를 찾지 못했습니다."
    fi
fi

# ── 3. 확인 ───────────────────────────────────────────────────
step "확인"
cmake --version | head -1
if command -v ninja >/dev/null; then echo "ninja $(ninja --version)"; else warn "ninja 없음"; fi
if ((do_qt)); then
    resolve_qt
    echo "Qt    $QT_ROOT_DIR"
fi

cat <<EOF

다음 단계:
  scripts/macos/build.sh      # configure + build + test
  scripts/macos/run.sh        # 실행

scripts/macos/*.sh 는 QT_ROOT_DIR 이 없어도 기본 위치($QT_DEFAULT_DIR)를 찾는다.
cmake --preset 을 직접 쓰거나 IDE 에서 빌드하려면 ~/.zshrc 에 추가:
  export QT_ROOT_DIR="$QT_DEFAULT_DIR"
EOF
