# Changelog

All notable changes to PokeSix are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added
- The Items page gets its third pane: picking an item (click or ↑/↓) shows a detail pane with the
  icon, name, price, a 1–9 generation-presence strip (current generation ringed), the full effect
  text, the Pokémon that evolve with the item ("가디 → 윈디", trade-hold evolutions marked), and —
  for TMs — the move it holds and where to get it (from the TM location dictionary; Platinum so
  far).
- The Pokédex list becomes three panes: filters | list | preview. The filter pane narrows the list
  by type (the generation's own types — no Fairy chip in generation 4), base stat total (a
  two-handle range slider), "전설 · 환상 제외" and "최종 진화만" (final by that generation's rules:
  Onix counts as final in generation 1). A single click or ↑/↓ fills the preview pane — sprite,
  number, name, types, stat radar and defensive weaknesses (×4/×2) from the generation's type
  chart — and a double click, Enter or "자세히 보기" opens the full-page detail as before.
- The home (intro) screen picks the generation with a fan of nine cards, spread like a hand of
  playing cards: the fan spreads open whenever the screen appears, cards lift on hover and stay
  lifted (with a yellow ring) when chosen, and dragging swings the whole fan, which springs back
  on release. Each card shows the generation, its region and a mascot Pokémon (the default
  96×96 sprite, trimmed and drawn at one size so the nine cards match; card contents live in
  `resources/theme/homecards.json`). The
  generation button leaves the home screen (the app bar keeps one), the menu becomes
  스쿼드 · 도감 백과 · 아이템 백과 (+ 종료) — the Settings row moves to the app bar tab. ←/→ moves
  between generations. (ADR 0015)
- Game chips in the Pokédex detail page ("기준 게임 [BW][B2W2]", the same version-colored chips as
  the squad's dex filter) replace the "기술 기준" label: choosing a game switches level-up moves,
  TM/HM numbers and locations, evolution conditions and wild encounters (only that game's versions)
  to it. DLC chips get short names from `dexstyle.json` (외딴섬 · 설원 · 벽록의 가면 · 남청의 원반).
- Status move effects in the Pokédex move lists ("효과" column), the squad move picker (a second
  line, also searchable) and slot card tooltips: stat stages with red ▲ / blue ▼ (prefixed "상대"
  when they hit the target), ailments ("상대 마비") and healing ("HP ½ 회복"), from PokéAPI move
  meta (schema version 10). Generation differences and special cases live in
  `resources/data/move-effects.json` (Growth raises only Sp. Atk up to generation 4, String Shot
  −1 up to 5, Minimize, Sweet Scent, Charge, Stockpile, Toxic, and Curse — a different effect when
  a Ghost uses it); generation 1 shows one "특수" stat. Moves without stat, ailment or healing data
  get a short note instead (about 140 moves: "물리 피해 반감(벽)", "햇살(5턴): 불꽃 1.5배 · 물 반감",
  "HP ¼로 대타 생성" …); the rest show the first sentence of the generation's game text, with the
  full text in the tooltip. The squad move picker labels
  level-up moves "자력(n레벨)".
- SixSquad editor (Squad tab). Each game keeps its own squad of six (generation → game, e.g. DP,
  Platinum and HGSS separately), picked with version chips at the top and saved to `squads.json`
  in the app data folder shortly after every change; the generation reopens on the last game used. Slot cards show the Pokémon in the
  generation's types, a role note, ability, nature and held item (only where the generation has
  them) and four moves; the Pokémon, move and held item pickers list only what exists in the
  generation, and moves only what the Pokémon learns in the chosen game (level-up, TM/HM, tutor,
  egg — DP, Platinum or HGSS in generation 4), with how each is learned. The live analysis panel
  lists problems first (three or more Pokémon weak to a type, a shared weakness nobody resists,
  4× weaknesses, defending types no move hits super-effectively), then a defensive heatmap over
  all 18 types (types missing from the generation hatched) with weakness, resistance and coverage
  rows, and the physical/special/status split with what it would be under the other split rule.
  The Pokémon picker has dex chips at the top right ([전국][DP][Pt][HGSS] in generation 4, each
  in its versions' colors); it opens on the dex of the squad's game and remembers the last choice.
  The picker also warns, in red, about Pokémon already in the squad, about the evolution or
  pre-evolution of a member (branches such as two Eeveelutions are not flagged), and about a second starter
  of the chosen game (starters come from `resources/data/starters.json`, per game: HGSS offers the
  Johto three, FRLG the Kanto three); slot cards in such a clash show a red "!" with the reason.
  Slot cards can be dragged by their header to reorder the squad: the card lifts with a shadow and
  follows the mouse, the cards in between slide aside, and the dropped card glides into place.
  Hovering a problem outlines the affected slots; empty slots suggest what kind of Pokémon would
  fix the current problems. The analysis lives in core (`SquadAnalyzer`, tested against the
  design's sample squad), and generation differences come from one feature table.
- Abilities and natures in the Pokédex detail page. An "특성" card lists the Pokémon's abilities for
  the current generation with their effects: none before generation 3, hidden abilities from
  generation 5, and past changes applied (Clefable gains Magic Guard in generation 4, Gengar's
  Levitate becomes Cursed Body in generation 7). A "성격" button on the base-stat card opens a 5×5
  nature table (raised stat by row, lowered stat by column); the chosen nature marks the radar axes
  with ▲/▼ and stays selected while browsing other Pokémon. Abilities, ability effects and natures
  are imported (schema version 8).
- White glove mouse cursor (original SVG art, pointing and grabbing) for everything clickable, and row
  hover in the Dex and Items lists: the row under the mouse is tinted and the glove squeezes twice
  when it moves onto a new row.
- Evolution tree in the detail page's acquisition card: the chain from its base stage with each
  step's method for the current generation (level, item, trade, friendship, time of day, place,
  known move …; e.g. Leafeon evolves at Eterna Forest in generation 4 and with a Leaf Stone from
  generation 8). Rows show the Pokémon's icon and open its detail when clicked. Evolution methods are
  imported per generation (schema version 7).
- The Heart Scale marker now appears only on level-1 moves that earlier stages do not learn by
  level-up (Gardevoir's Healing Wish, Garchomp's Fire Fang), not on a base stage's starting moves.
- Pokédex detail page: clicking a Pokémon opens its sprite (generation art), names, genus, types,
  size, base stat bars, wild encounters for every version of the generation, defensive and STAB
  offensive type matchups, level-up moves (Heart Scale marker for level 1) and TM/HM moves with their
  numbers and locations. Moves follow the generation's representative game (Platinum for generation
  4) with per-generation power, accuracy, PP, type and, up to generation 3, type-based category.
  Learnsets, machines, locations and encounters are imported (schema version 6); TM locations,
  Korean place names and encounter method names come from bundled dictionaries. Without
  `--language` the app starts in Korean.
- Display languages Korean, English and Japanese. Game data (Pokémon, item, move, region and version
  names, item effects) is stored in all three languages and switches live through
  `AppState::language`, falling back to another language when one is missing; Japanese uses the kana
  spelling when there is no kanji one and half-width digits. UI strings have English and Japanese
  translations (Qt Linguist) loaded at startup; `--language ko|en|ja` picks and saves the language.
  Korean names for the generation 4 mail and HM08 fill gaps in PokéAPI. Schema version 5.
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
- Regional Pokédexes: the Dex list header has a selector (National, then the generation's regional
  dexes, e.g. Sinnoh D·P, Sinnoh Pt, Johto HG·SS in generation 4) that switches to the regional
  numbering. Dexes per generation come from PokéAPI's dex ↔ version group ↔ generation links; four
  more CSV files are downloaded and the database schema is now version 2 (existing databases are
  re-imported on first run).
- Generation menu and `AppState`: the intro and app bar generation buttons read "N세대 ▾" and open a
  menu of generations 1–9; the choice is saved in the settings and the Dex list (regional dex buttons,
  species, types and stats) follows it.
- Items page (category and list panels): eight category groups, search by name or effect, and a list
  of the items that exist in the current generation, painted by a row delegate with item icons, effect
  text and price. Machines show the move they hold in that generation (type chip and name) with a
  type-coloured disc icon; generations 1–5 use the BW item art. The list reloads when the generation
  changes, like the Dex page. Moves (names and per-generation types) and machines are imported;
  schema version 4. Adds the missing
  check mark icon used by the check box style.
- Item data for the Items page: categories, pockets, ko/en/ja names, per-generation availability and a
  Korean effect text per generation (the first game of each generation; generations 1–5 fall back to
  generation 6). `ItemTableModel` and `ItemFilterProxy` (search, category predicate, "this generation
  only"), and `resources/theme/itemstyle.json` that groups PokéAPI's 55 categories into the Items page
  groups and hides unused data. `SpriteCache` now also fetches item icons. The first download grows to
  18.8 MB and the database schema is version 3.
- Regional dex buttons show split version badges (e.g. Sinnoh [D|P], Johto [HG|SS]); badge labels and
  colours, hidden dexes and label overrides live in `resources/theme/dexstyle.json`.
- Dex list icons: `SpriteCache` downloads small Pokémon icons from PokéAPI/sprites into the user cache
  folder on demand (never into the repository); the list sorts by number, uses taller rows and
  responsive columns: fixed columns (the type column fits any two chips) plus weighted columns for the
  name and stats, in a panel that fills the page up to 1100 px. A custom header view places its labels
  with the same alignment rule and padding as the cells (text left, numbers right) and keeps the sort
  arrow from shifting the label.

### Fixed
- The home fan's bottom arc was cut flat: the widget's height estimate did not cover how far the
  outer cards sag after the fan was widened (a widget cannot paint outside its own rect — no
  z-order involved). The height now follows the real geometry, and dragging clamps the swing so
  the corners stay inside the widget.
- The squad's card column now ends on exactly the same line as the live analysis panel: cards are
  sized when the scroll viewport gets its final size (not from the page's own resize, which ran too
  early and left an 11px mismatch), the upper height cap is gone and leftover pixels go to the top
  rows one by one.
- Picking a generation no longer hides its right neighbor on the home fan: only the hovered card
  comes to the front, while the chosen card slides up in its own place in the stack (like a card
  pushed up in a hand); the fan also spreads a little wider (radius 2.4 → 3.2 × card height).
- "← 목록" in the Pokédex detail did nothing since the three-pane list row: the back switch still
  pointed at the list panel, which is no longer a page of the stack.
- The squad page no longer scrolls as a whole in the wide layout: slot cards also grow with the
  window (row gaps widen, up to 292px) so the six cards fill the column, and when the live
  analysis is taller than the window it scrolls inside its own panel instead of overflowing; the
  narrow layout keeps the single page scroll.
- The squad's bottom card row (slots 05–06) was clipped by the window: slot cards now shrink with
  the window height — only the gaps between their rows compress, down to a 234px minimum — so all
  six cards fit without scrolling at the default window size; smaller windows scroll as before.
- The Pokédex detail picture overflowed its card for generations whose sprite is larger than the
  frame (generation 9's 256×256 renders, and the 96×96 default pictures of generations 7–8 drawn
  at 2×). Sprites larger than the frame now shrink to fit (smoothly — they are not pixel art at
  that size), via a shared sprite-fit helper.
- "도감에서 보기" from a squad slot opened the Pokémon in the game last chosen in the Pokédex
  (Emerald) instead of the squad's game (FireRed/LeafGreen). It now opens the detail in the
  squad's game and switches the Pokédex list to that game's dex, unless the national dex or a dex
  of that game is already selected.
- After a schema upgrade, moves showed blank names and types and every move as special until the
  app was restarted: the squad screen opened the old database at startup and kept that connection
  after the update replaced the file. The repository now refuses a database with another schema
  version and closes its connection when the data update finishes, and the squad reloads.
- TM and HM descriptions in the Items page were shown in English with the Korean language selected
  for generations 1–5: Korean item descriptions only exist from generation 6, so machines now use
  their move's description, borrowed from the nearest generation when needed (the move is the same,
  so the text still matches). Move descriptions are imported (schema version 9; the downloaded CSV
  data grows to 28.9 MB). Moves introduced in generation 9 still lack Korean text in PokéAPI.

### Changed
- Home mascots are upscaled with Scale2x (EPX) — applied twice, then smoothly fitted — instead of
  plain bilinear scaling, so the enlarged pixel art stays crisp.
- The home fan is tighter and bigger: cards grew, the fan gathers toward its pivot like a hand of
  cards and its width shrank, and every card draws the same default sprite art, trimmed and
  bottom-aligned at one size (generation 7's mascot is now Rowlet). The menu became three card
  buttons in the photo reference's style — a red primary Squad card and white Pokédex/Items cards,
  each with an icon badge and a chevron — and hovering over overlapping fan cards no longer
  flickers (hover is hit-tested against resting positions; cards drop more slowly than they lift).
- The Pokédex detail page takes its moves and TMs from the game of the dex chosen in the list
  (HeartGold/SoulSilver from the Johto dex, Diamond/Pearl from the Sinnoh D·P dex); only the national
  dex falls back to the generation's representative game. The basis label adds version abbreviations
  ("기라티나 (Pt)").
- Search fields filter while the first Korean syllable is still being composed (IME preedit), not
  only after the next keystroke commits it; lone jamo are ignored so results do not flash empty.
- The main window opens at 1440×900 (fits the three-pane Pokédex/Items layouts and the home card
  fan; it first opened at 920×840); its minimum size now comes from the layouts. The app bar search field shrinks from 280 to 160 px on narrow windows and hides its
  `Ctrl K` badge below 220 px.
- The paper background in `app.qss` applies to `QMainWindow` only, so plain container widgets no
  longer paint paper-coloured rectangles over panels.
- The home screen is a full-screen intro instead of the v1 dashboard; the app bar will have four tabs.
- Installed Linux binaries keep the Qt library path in their RUNPATH, so the desktop launcher works.

### Changed
- The squad's analysis panel keeps the defensive heatmap pinned at the top, followed by the
  problem list and the physical/special split, so all three stay in view. The problem list now
  scrolls inside its own box (up to four rows in the narrow layout; whatever height is left in the
  wide layout) instead of pushing the heatmap off screen, and a "크게 보기 ↗" link opens the full
  list in a larger dialog where titles and details are no longer elided.

### Fixed
- Windows: `run.bat --log` (and any `build.bat` / `setup.bat` call with arguments) failed with "env.bat is not recognized" because `shift` also shifted `%0`, so `%~dp0` no longer pointed at the script folder. All argument loops now use `shift /1`.
- Windows: the build stopped at `GoogleTestAddTests.cmake … Exit code 0xc0000135` because `gtest_discover_tests()` ran the Qt-linked test binary right after linking, before any Qt DLL was on PATH. The data tests now discover at ctest time (`DISCOVERY_MODE PRE_TEST`), where the test preset sets PATH.
- Windows: `setup.bat` could not find `uvx` right after installing uv, because winget 1.29 places portable packages under `WinGet\Packages` and does not always create the `WinGet\Links` folder. The script now adds both to PATH for the session.

### Documentation
- `docs/overlay-design.md`: review and design of the emulator overlay (melonDS, generations 4–5): reading the party from the `.sav` file first and from the GDB stub later, save and PKM layout as data tables in core, window docking on Windows, move suggestions, legal notes and a Phase H step breakdown.
- The Windows build is now verified end to end (VS 2022 17.14, Qt 6.8.3): `docs/build.md` §4 and §5-7, `scripts/README.md` and `CLAUDE.md` updated.
- `docs/deploy.md`: the run CLI (script options, app arguments, Qt environment variables, where the app keeps its data, cache and settings on each OS) and the Windows `.exe` distribution plan — `windeployqt` by hand today, `qt_generate_deploy_app_script` + CPack ZIP next, installer options and code-signing notes, a pre-release checklist and the CI step for Phase G.

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
