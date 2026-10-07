#!/usr/bin/env bash
# [macOS] Install all external dependencies in one go — Xcode Command Line Tools + Homebrew.
#
#   scripts/macos/setup.sh              # Homebrew packages + Qt
#   scripts/macos/setup.sh --no-system  # Qt only
#   scripts/macos/setup.sh --no-qt      # Homebrew packages only
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

# ── 1. Xcode CLT and Homebrew packages ────────────────────────
if ((do_system)); then
    if ! xcode-select -p >/dev/null 2>&1; then
        step "opening the Xcode Command Line Tools installer. Run this script again when it finishes."
        xcode-select --install || true
        exit 1
    fi
    command -v brew >/dev/null || die "Homebrew is required: https://brew.sh"

    pkgs="cmake ninja clang-format uv"          # git comes with Xcode CLT; uv runs aqtinstall
    missing=""
    for p in $pkgs; do
        brew list --formula "$p" >/dev/null 2>&1 || missing="$missing $p"
    done

    if [[ -z "$missing" ]]; then
        step "Homebrew packages: all installed"
    else
        step "installing Homebrew packages:$missing"
        # shellcheck disable=SC2086  # intentional word splitting into separate arguments
        brew install $missing
    fi
fi

# ── 2. Qt (aqtinstall) ────────────────────────────────────────
# Homebrew's qt is always the latest release and cannot be pinned, so use aqtinstall
if ((do_qt)); then
    if [[ -n "${QT_ROOT_DIR:-}" ]] && is_qt_dir "$QT_ROOT_DIR"; then
        step "Qt: using QT_ROOT_DIR ($QT_ROOT_DIR)"
    elif is_qt_dir "$QT_DEFAULT_DIR"; then
        step "Qt $QT_VERSION already installed: $QT_DEFAULT_DIR"
    else
        command -v uvx >/dev/null || die "uv not found. Run again without --no-system, or 'brew install uv'"
        step "installing Qt $QT_VERSION → $QT_INSTALL_DIR"
        echo "    (aqt prints a line only when an archive finishes. qtbase and qtdeclarative are"
        echo "     hundreds of MB, so it can stay silent for 10+ minutes while they download and"
        echo "     extract — it is not stuck.)"
        # aqt leaves aqtinstall.log in the working directory, so run it from a temp directory
        (cd "${TMPDIR:-/tmp}" && uvx --from aqtinstall aqt install-qt mac desktop "$QT_VERSION" "$QT_AQT_ARCH" \
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
  scripts/macos/build.sh      # configure + build + test
  scripts/macos/run.sh        # run

scripts/macos/*.sh find Qt at the default location ($QT_DEFAULT_DIR) even without QT_ROOT_DIR.
To use cmake --preset directly or build from an IDE, add this to ~/.zshrc:
  export QT_ROOT_DIR="$QT_DEFAULT_DIR"
EOF
