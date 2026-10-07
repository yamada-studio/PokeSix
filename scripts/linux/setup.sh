#!/usr/bin/env bash
# [Linux] Install all external dependencies in one go — Ubuntu 24.04 (apt).
#
#   scripts/linux/setup.sh              # system packages (sudo) + Qt
#   scripts/linux/setup.sh --no-system  # Qt only (no sudo)
#   scripts/linux/setup.sh --no-qt      # system packages only
#
# Anything already installed is skipped. Safe to run repeatedly.
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
        *) usage; die "unknown argument: $arg" ;;
    esac
done

# ── 1. System packages (apt) ──────────────────────────────────
if ((do_system)); then
    command -v apt-get >/dev/null \
        || die "only apt-based distributions (Ubuntu 24.04) are supported. See docs/build.md to install manually."

    pkgs=(
        build-essential cmake ninja-build git     # toolchain
        libgl1-mesa-dev libxkbcommon-dev          # required by find_package(Qt6 Gui)
        libxcb-cursor0                            # required by the Qt 6.5+ xcb platform plugin
        clang-format clang-tidy gdb               # code quality, debugger
    )
    # aqtinstall runs through uvx. A Python venv is needed only when uv is missing
    command -v uvx >/dev/null 2>&1 || pkgs+=(python3-venv)
    missing=()
    for p in "${pkgs[@]}"; do
        dpkg-query -W -f='${Status}' "$p" 2>/dev/null | grep -q "install ok installed" || missing+=("$p")
    done

    if ((${#missing[@]} == 0)); then
        step "system packages: all installed"
    else
        step "installing system packages (sudo): ${missing[*]}"
        sudo apt-get update
        sudo apt-get install -y "${missing[@]}"
    fi
fi

# ── 2. Qt (aqtinstall) ────────────────────────────────────────
# Ubuntu 24.04 ships Qt 6.4 via apt, which is too old (docs/build.md §2-2)
if ((do_qt)); then
    if [[ -n "${QT_ROOT_DIR:-}" ]] && is_qt_dir "$QT_ROOT_DIR"; then
        step "Qt: using QT_ROOT_DIR ($QT_ROOT_DIR)"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        step "Qt $QT_VERSION already installed: $QT_DEFAULT_DIR"
    else
        if command -v uvx >/dev/null 2>&1; then
            aqt=(uvx --from aqtinstall aqt)           # isolated throwaway venv
        else
            venv="${TMPDIR:-/tmp}/pokesix-aqt-venv"
            python3 -m venv "$venv" || die "python3 -m venv failed (sudo apt install python3-venv)"
            "$venv/bin/pip" install --quiet aqtinstall
            aqt=("$venv/bin/aqt")
        fi
        step "installing Qt $QT_VERSION → $QT_INSTALL_DIR"
        echo "    (aqt prints a line only when an archive finishes. qtbase and qtdeclarative are"
        echo "     hundreds of MB, so it can stay silent for 10+ minutes while they download and"
        echo "     extract — it is not stuck.)"
        # aqt leaves aqtinstall.log in the working directory, so run it from a temp directory
        # --archives: only what PokeSix uses — qtbase (Core/Gui/Widgets/Sql/Network),
        # qtsvg, qttools (lupdate/lrelease) and icu (separate archive on Linux).
        # Skips qtdeclarative & friends: hundreds of MB and minutes of extraction.
        (cd "${TMPDIR:-/tmp}" && "${aqt[@]}" install-qt linux desktop "$QT_VERSION" "$QT_AQT_ARCH" \
            --archives qtbase qtsvg qttools icu \
            --outputdir "$QT_INSTALL_DIR")
        is_qt_dir "$QT_DEFAULT_DIR" || die "Qt still not found at $QT_DEFAULT_DIR after installation."
    fi
fi

# ── 3. Check ──────────────────────────────────────────────────
step "check"
cmake --version | head -1
if command -v ninja >/dev/null; then echo "ninja $(ninja --version)"; else warn "ninja not found"; fi
if ((do_qt)); then
    resolve_qt
    echo "Qt    $QT_ROOT_DIR"
fi

cat <<EOF

Next:
  scripts/linux/build.sh      # configure + build + test
  scripts/linux/run.sh        # run

scripts/linux/*.sh find Qt at the default location ($QT_DEFAULT_DIR) even without QT_ROOT_DIR.
To use cmake --preset directly or build from an IDE, add this to ~/.bashrc:
  export QT_ROOT_DIR="$QT_DEFAULT_DIR"
EOF
