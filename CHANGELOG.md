# Changelog

All notable changes to PokeSix are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- `MainWindow` in a new `pokesix_ui` library: 1440×900 default size, 960×640 minimum,
  title with the application version.
- `pokesix.ui` logging category (info by default; debug via `QT_LOGGING_RULES`).
- Full-screen intro (home) screen from design handoff v2: capsule mark, wordmark with its offset
  shadow, generation button, menu window with five rows (hover, arrow keys, Enter, 1–4, Esc to quit)
  and an info footer, on a striped background between red bands.
- Design theme: v2 tokens, bundled fonts registered at startup, and an application stylesheet with
  `@token` substitution.
- Capsule app icons for macOS, Windows and Linux, a multi-size window icon and the Linux desktop file name.
- Design packages moved to a top-level `design/` folder (handoff v1, v2 and design requests).

### Changed
- The home screen is a full-screen intro instead of the v1 dashboard; the app bar will have four tabs.
- Installed Linux binaries keep the Qt library path in their RUNPATH, so the desktop launcher works.

## [0.0.1] - 2026-09-28

### Added
- Project skeleton: layered CMake targets (`core` without Qt, `data`, `ui`, `app`), C++20, Qt 6.8 LTS.
- CMake presets for Linux (GCC/Ninja), macOS (Apple Clang/Ninja) and Windows (MSVC 2022).
- Per-OS helper scripts (`scripts/linux`, `scripts/macos`, `scripts/windows`): one-step dependency setup
  (including a pinned Qt 6.8.3 via aqtinstall), build + test (+ install), and run.
- GoogleTest (pinned by SHA256) with a sample type chart test, and a Qt runtime check (SQLite driver, TLS backend).
- clang-format / clang-tidy / EditorConfig configuration.
- Application icons for macOS, Windows and Linux; bundled OFL fonts.
- Documentation: architecture, build guide, conventions, roadmap, versioning, decision records, design handoff.
