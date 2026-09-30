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

- `pokesix_data` library with `CsvDownloader`: downloads 35 PokéAPI CSV files (12.5 MB) pinned to one
  commit, verifies each file's size and SHA-256, resumes per file and writes atomically.
- `pokesix-fetch-csv` developer tool that runs the same downloader from the command line.
- `CsvReader` (RFC 4180) and `CsvImporter`: builds `pokesix.sqlite` with generations, types, the type
  chart, stats, species (ko/en/ja names), pokemon with forms, pokemon types and base stats, storing
  per-generation values as `gen_from`/`gen_to` ranges derived from PokéAPI's `*_past` tables.
- `pokesix-import-csv` developer tool that turns a downloaded CSV folder into the database.
- First run: without a game database the intro shows a panel that downloads and imports the data
  (ready, downloading with a 10-segment bar and cancel, failed with retry) and locks Dex, Items and
  SixSquad until it finishes; the import runs on a worker thread. `ShadowButton` and
  `SegmentProgress` widgets.

- Main screen app bar: capsule sticker mark and wordmark (back to the intro), four folder tabs, a
  compact generation button and a search field; intro menu entries, tabs, Ctrl+1–4 and Ctrl+K switch
  pages. Pages other than the intro are placeholders for now.

- Dex list with real data: `Repository` reads species with their types and base stats for a
  generation from the read-only game database, `SpeciesTableModel` and `SpeciesFilterProxy` (search by
  number or ko/en/ja name, numeric sort) feed a table drawn by a row delegate with type chips; the
  panel header shows the count. Generation 4 is fixed until the generation popup lands.
- `PanelFrame` title header band and a reusable `typechip` painter.
- Dex list icons: `SpriteCache` downloads small Pokémon icons from PokéAPI/sprites into the user cache
  folder on demand (never into the repository); the list sorts by number, uses fixed, slightly wider
  columns with taller rows and sits centred on the page.

### Changed
- The main window opens at 920×840 (just wide enough for the Dex list); its minimum size now comes
  from the layouts. The app bar search field shrinks from 280 to 160 px on narrow windows and hides its
  `Ctrl K` badge below 220 px.
- The paper background in `app.qss` applies to `QMainWindow` only, so plain container widgets no
  longer paint paper-coloured rectangles over panels.
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
