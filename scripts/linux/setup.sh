#!/usr/bin/env bash
# [Linux] 외부 의존성 한 번에 설치 — Ubuntu 24.04 기준 (apt).
#
#   scripts/linux/setup.sh              # 시스템 패키지(sudo) + Qt
#   scripts/linux/setup.sh --no-system  # Qt 만 (sudo 없이)
#   scripts/linux/setup.sh --no-qt      # 시스템 패키지만
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

# ── 1. 시스템 패키지 (apt) ─────────────────────────────────────
if ((do_system)); then
    command -v apt-get >/dev/null \
        || die "apt 기반 배포판(Ubuntu 24.04)만 지원합니다. docs/build.md 를 참고해 직접 설치하세요."

    pkgs=(
        build-essential cmake ninja-build git     # 툴체인
        libgl1-mesa-dev libxkbcommon-dev          # find_package(Qt6 Gui) 가 요구
        libxcb-cursor0                            # Qt 6.5+ xcb platform plugin
        clang-format clang-tidy gdb               # 코드 품질 · 디버거
    )
    # aqtinstall 은 uvx 로 실행한다. uv 가 없을 때만 python venv 가 필요하다
    command -v uvx >/dev/null 2>&1 || pkgs+=(python3-venv)
    missing=()
    for p in "${pkgs[@]}"; do
        dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q "install ok installed" || missing+=("$p")
    done

    if ((${#missing[@]} == 0)); then
        step "시스템 패키지: 모두 설치되어 있음"
    else
        step "시스템 패키지 설치 (sudo): ${missing[*]}"
        sudo apt-get update
        sudo apt-get install -y "${missing[@]}"
    fi
fi

# ── 2. Qt (aqtinstall) ─────────────────────────────────────────
# Ubuntu 24.04 의 apt Qt 는 6.4 라서 쓰지 않는다 (docs/build.md §2-2)
if ((do_qt)); then
    if [[ -n "${QT_ROOT_DIR:-}" ]] && is_qt_dir "$QT_ROOT_DIR"; then
        step "Qt: QT_ROOT_DIR 사용 ($QT_ROOT_DIR)"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        step "Qt $QT_VERSION 이미 설치됨: $QT_DEFAULT_DIR"
    else
        if command -v uvx >/dev/null 2>&1; then
            aqt=(uvx --from aqtinstall aqt)           # 격리된 임시 venv
        else
            venv="${TMPDIR:-/tmp}/pokesix-aqt-venv"
            python3 -m venv "$venv" || die "python3 -m venv 실패 (sudo apt install python3-venv)"
            "$venv/bin/pip" install --quiet aqtinstall
            aqt=("$venv/bin/aqt")
        fi
        step "Qt $QT_VERSION 설치 → $QT_INSTALL_DIR"
        # aqt 는 실행 위치에 aqtinstall.log 를 남기므로 임시 폴더에서 실행한다
        (cd "${TMPDIR:-/tmp}" && "${aqt[@]}" install-qt linux desktop "$QT_VERSION" "$QT_AQT_ARCH" \
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
  scripts/linux/build.sh      # configure + build + test
  scripts/linux/run.sh        # 실행

scripts/linux/*.sh 는 QT_ROOT_DIR 이 없어도 기본 위치($QT_DEFAULT_DIR)를 찾는다.
cmake --preset 을 직접 쓰거나 IDE 에서 빌드하려면 ~/.bashrc 에 추가:
  export QT_ROOT_DIR="$QT_DEFAULT_DIR"
EOF
