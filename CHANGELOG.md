# Changelog

All notable changes to PokeSix are documented here.
The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and the project uses [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Documentation
- README refreshed: the four screenshots are recaptured (sprite-free, as always) showing the new
  squad top bar, move-source badges, 1:1 column split and the item/dex preview panes, and the
  Squad section now covers the HM shuttle, resource investment, undo/redo/clear and
  export/import/image sharing. The note about the removed search box is gone.

### Fixed
- The release workflow passes on real runners (run 37600397225, all three OSes): macos-14 ships AppleClang 15, which rejects a lambda capturing a structured binding in moveeffect.cpp (copied to a plain local), and windows-latest became the windows-2025-vs2026 image with no VS 2022, so the Windows job is pinned to windows-2022 until the presets support VS 2026.
- Windows quickstart runs end to end (verified 2026-10-07: setup → release build → 96 tests →
  windeployqt → `PokeSix-0.0.1-win64.zip`, 17.6 MB, and the packaged exe starts on a PATH
  without Qt). Three things were broken on MSVC:
  - `cardbarrel.cpp` declared `QColor shade(QColor(tok::kInk));`, which MSVC reads as a function
    declaration (most vexing parse) and rejects because the "parameter name" is qualified; GCC
    happened to accept it. Brace initialization now.
  - `package.bat` called `cmake --install` without looking for the VS-bundled CMake (`build.bat`
    does, but its PATH change is local to it), so a machine without cmake on PATH stopped right
    after the build.
  - Two `RepositoryTest` squad-session tests failed because `AppState` used the default
    `QSettings` format: on Windows that is the registry, and with no organization name set in the
    test binary every write is silently dropped (Linux's file backend writes to "Unknown
    Organization" and passed). The app and the test main now set `QSettings::IniFormat` as
    conventions §8 always said, the test main gives the process an organization/application
    name, and the tests redirect `IniFormat` (not `NativeFormat`) to a temporary folder. Settings
    on Windows therefore move from the registry to `%APPDATA%\YamadaStudio\PokeSix.ini`; the
    generation and game chosen before this change are not carried over.
- The AppImage shows the desktop's native file dialogs (Nautilus-style on GNOME) instead of
  Qt's built-in fallback: the platform theme plugins (libqgtk3, libqxdgdesktopportal) are now
  bundled — copied without their dependency walk, so the host's GTK3 is used and a host
  without it simply falls back to the Qt dialog.

### Changed
- Qt setup downloads a quarter of what it used to: aqtinstall now fetches only the archives
  PokeSix uses (`qtbase`, `qtsvg`, `qttools`, plus `icu` on Linux) instead of the full desktop
  set — qtdeclarative and friends were hundreds of MB and minutes of extraction for nothing.
  Verified on Linux: a fresh minimal install (339MB) configures, builds and passes all 96
  tests, and the app runs.

### Added
- The app layer exists (A3): `src/app/Application` is the composition root — it owns the
  QApplication, parses the command line, sets the log line format
  (`hh:mm:ss.zzz L category: message`), applies language and theme, creates AppState,
  Repository and DataUpdater and injects them into MainWindow, whose constructor now takes
  them. `main.cpp` shrinks to two lines. `--screenshot <screen> <WxH> <out.png>` (screens:
  intro, dex, items, map, squad, settings; `--screenshot-delay <ms>`, default 1200 so the intro's
  card fan has finished spreading) opens that screen at that size, grabs the window to a PNG and
  exits — the capture tool the roadmap's A10 design comparison needs. Works offscreen.
- A release workflow (`.github/workflows/release.yml`): pushing a `v*` tag builds the AppImage,
  the Windows ZIP and the macOS dmg on three GitHub runners (minimal Qt archives, Linux on
  ubuntu-22.04 for wider glibc compatibility) and attaches them to the GitHub Release — end
  users download one file and never need Qt, a compiler or the setup scripts. First exercised on 2026-10-07 via workflow_dispatch: all three jobs pass and upload the
  AppImage, dmg and ZIP as artifacts (the tag path is unchanged and still untested).
- macOS packaging and quickstart: `scripts/macos/package.sh` installs the release build (which
  runs macdeployqt, so the .app carries the Qt frameworks), adds the licenses and an
  /Applications link, and builds `PokeSix-<version>-macos.dmg` with hdiutil + SHA-256;
  `scripts/macos/quickstart.sh` chains setup → package → open. Unsigned: Gatekeeper warns on
  first launch (right-click → Open). Untested in this environment — no Mac at hand.
- Unova joins the town map atlas (82 places): the B2W2-era map serves both BW and B2W2 —
  places that don't exist yet in BW simply show no encounter data there. Pokéwood and the PWT
  have no PokéAPI location and wait for the landmark dictionary.
- The town map's location panel grows three tabs in its header — [야생] [아이템] [랜드마크].
  야생 is the encounter list as before; 아이템 lists what the acquisition book records at that
  location (grouped per item, with the TM's move and how/cost lines — fed by the 606 HGSS
  sources that carry a location id); 랜드마크 is a new hand-written dictionary
  (`townmap/landmarks/heartgold-soulsilver.json`, 33 places) of gyms, shops and events — the
  Day Care on Route 34, Goldenrod's department store and Game Corner, the Move Reminder in
  Blackthorn and so on.
- The combined map wears region name tags: "성도" at the Johto layer's top-left and "관동" at
  the Kanto layer's top-right, anchored to the map so they travel with the drag.
- The town map atlas joins two-region games into one continuous map: Johto and Kanto are
  stitched on a single canvas (Kanto at x+158 — Mt. Silver and Tohjo Falls sit at the same
  latitude in both source images, so the seam is invisible) and explored by dragging, with no
  region buttons; border places keep both map fragments outlined. The default scale now fills
  the panel's height, so wide maps overflow sideways into the drag instead of leaving empty
  space. Sinnoh (DP·Pt, 69 places) is mapped too, and the page's game chips go back to the
  grouped style ([신오 D|P] [성도 HG|SS] — pieces still pick the version).
- A new "타운맵 백과" page (fifth tab, between Items and Squad, also on the intro menu): the
  in-game town map as original pixel art with modern controls. The map image is downloaded at
  first use to the user cache (never committed — same policy as sprites, from Serebii Pokéarth's
  marker-free per-region renders), drawn at a crisp integer nearest-neighbor scale with wheel
  zoom and drag pan; while it downloads (or offline) a schematic of the nodes stands in. Hovering
  a place shows its name bubble, clicking lists that game's wild encounters (grouped by method,
  with level ranges and best rates — gifts and in-game trades included) on the right. The page
  follows the app's generation and game with per-version chips, and two-region games (GSC, HGSS)
  get 성도/관동 region chips. 45 Johto and 49 Kanto places are mapped
  (`resources/data/townmap/*.json`, coordinates from Serebii's imagemap as factual data);
  location names go through the place-name dictionary, so 성도/관동 towns read in Korean.
- Squad export files wear their own extension: `.pks` (the content stays the same JSON). The
  save dialog suggests `pokesix-squad-4-heartgold.pks` and appends `.pks` when no suffix is
  typed; import still accepts the early `.json` exports.
- Slot cards say how each assigned move is learned, right before the 물/특 badge: a Heart Scale
  item icon when the move needs the Move Reminder, "Lv.55" for level-up moves, "TM26"/"HM03"
  for machines, and NPC/알 for tutor and egg moves. To give the wider labels room, the wide
  layout splits the card area and the analysis panel 1:1 (the card column keeps a 612px
  minimum; the heatmap shrinks its cells down to 24px as needed).
- Squad editing gets undo / redo / clear-all, as three borderless icon buttons right of the
  HM-shuttle button (the classic round arrows). Every committed edit — Pokémon, move, memo,
  ability, nature, item, shuttle, name, drag reorder, import — is one history step (up to 50),
  kept per squad and reset when the generation or game changes. "일괄 비우기" empties the six
  members and the shuttle (the name stays) and is itself undoable, so it asks no confirmation.
- Packaging (Phase G lite): `scripts/linux/package.sh` builds the release (tests included),
  installs into an AppDir and runs linuxdeploy + its Qt plugin (downloaded once into
  `build/package/tools`) to produce `build/package/PokeSix-<version>-x86_64.AppImage`;
  `scripts\windows\package.bat` builds the release, installs with the new
  `qt_generate_deploy_app_script` hook (windeployqt runs at install, with
  `--compiler-runtime`, D3D/OpenGL-sw and unused SQL drivers excluded), ships LICENSE and
  THIRD_PARTY_NOTICES.md, and zips `PokeSix-<version>-win64.zip`. Both print a SHA-256.
  The TLS plugins ride along (`EXTRA_PLUGINS`) so the first-run data download works on a
  machine without Qt, the offscreen platform plugin is bundled for headless runs, and the
  unused SQL drivers (mimer/odbc/psql/mysql) are dropped from the AppImage.
- `scripts/linux/package.sh --install` registers the AppImage in the desktop's application
  menu: a stable copy at `~/.local/bin/PokeSix.AppImage`, the `.desktop` entry rewritten to
  its absolute path (this replaces the entry a dev-time `--install ~/.local` left behind,
  which kept launching a stale binary) and the hicolor icons. The AppImage is now built
  under a temporary name and moved into place, so repackaging works while the app is
  running. quickstart passes `--install` by default.
- One script from a fresh clone to a running app: `scripts/linux/quickstart.sh` and
  `scripts\windows\quickstart.bat` chain setup → release build with tests → package →
  launch (`--no-run` stops after packaging). The README's Building section now leads with
  them.
- The HM-shuttle button only appears through generation 6: from generation 7 the games replaced
  party-taught HMs with built-in systems (Poké Ride in SM/USUM, the Rotom Bike in SwSh, the
  Pokétch hidden-move app in BDSP — even though its data still lists HM items — and ride Pokémon
  in LA/SV), so a shuttle has nothing to carry. The rule lives in the generation-features table
  (`GenerationFeatures::hiddenMachines`), not in scattered `if (gen >= 7)` checks.
- The generation button (intro and app bar) wears its generation's colors: the body is split into
  equal solid stripes of that generation's series badge colors in release order (gen 1
  [red|green|blue|yellow], gen 4 [diamond|pearl|platinum|gold|silver] …), saturated a step above
  the pale badge tints. A smooth gradient was tried first and rejected — blended, the colors stop
  being recognizable. The generation menu shows the same stripes as a small swatch per row. The
  lists live in `dexstyle.json` (`generations`), reusing the version badge palette.
- Squads travel: the squad top bar grows a share group — "불러오기" / "내보내기" move a squad
  through a portable .json file (generation, game and the squad itself; importing switches to the
  file's generation/game and asks before overwriting a non-empty squad), and "이미지 ▾" renders the
  six cards plus the analysis panel into one titled picture, copied to the clipboard or saved as
  PNG. The status label flashes the outcome ("✓ 스쿼드를 내보냈어요") and the top bar's right edge
  now lines up with the analysis panel (it used to stick out past it by the scrollbar's width).
- The HM-shuttle dialog got a design pass: the shuttle Pokémon shows as a large margin-trimmed box
  icon with its type chips, the HM table sits in a bordered panel with column headers and a type
  chip per move, coverage texts are colored by state, HMs the chosen shuttle cannot learn show a
  red "배울 수 없어요" warning instead of a silent disabled checkbox, and the top-bar button
  carries the shuttle's small icon next to its name.
- The squad gains an HM-shuttle slot ("비전셔틀", a seventh member): a top-bar button opens a
  dialog where you pick the shuttle Pokémon and check which of the game's HMs it carries (up to
  four). Each HM row shows who covers it — the shuttle, a main member burning a move slot on it
  ("셔틀이 들어요 — 갸라도스의 기술 칸을 비워도 돼요"), or nobody. The shuttle stays out of the
  six-slot analysis and the member count, and is saved per version under a `shuttle` key that old
  files simply don't have (`Repository::hiddenMachineMoves` lists a game's HMs).
- The squad analysis panel gains a "리소스 투자" (resource investment) section: it tallies what the
  move layout consumes — Heart Scales (move-reminder moves), TM purchases and NPC move-tutor fees —
  with per-currency totals (BP, coins, shards, money) beside the section title. A TM the game drops
  only once but placed on two members raises a warning ("이 게임에 1개뿐이에요 ⚠"); a TM with a
  repeatable source (shop, exchange, prize) adds its purchase cost instead, and TMs whose every
  source sits in the post-game region are marked "관동 — 엔딩 후". HM moves and level-up moves cost
  nothing and stay out of the list. Counting lives in core (`resourceledger`, unit tested); the
  supply side comes from the acquisition books via `guidebook::itemSupply` and
  `guidebook::tutorCostAmounts`, and the HGSS book's TM/HM sources now carry a `region` field
  (johto/kanto, progression-based: Routes 26–27, Victory Road and Indigo Plateau count as johto).
- README becomes an introduction and user guide: a hero screenshot, what the app does, a walkthrough
  of first launch, choosing a generation and game, the Pokédex, Squad and Items screens, keyboard
  shortcuts, language options and where the data comes from. Screenshots live in
  `docs/screenshots/` (home, dex, squad, items).
- Home generation cards show every series of the generation: the first series' starters stand in
  front and each later series is stacked one tier further back and up (4th: Sinnoh DP·Pt →
  Johto HGSS; 8th: Galar → BDSP Sinnoh → Hisui; 1st: Kanto → Yellow's Pikachu; 7th: Alola → Let's
  Go Pikachu/Eevee …), and the caption lists the regions ("신오 · 성도 · 관동").
  `homecards.json` now has `layers` and `regions` instead of `pokemon` and `region`.
- The Items detail shows Pokémon: a TM/HM lists every Pokémon that can learn it in the chosen game
  group as a box-icon grid ("배울 수 있는 포켓몬 · 169마리", names in tooltips, version exclusives
  faded with "SS 한정"), and evolution items draw their targets as icon rows ("[무우마] → [무우마직]
  P 한정", "지니고 교환"). The detail scrolls inside its panel when it grows long
  (`Repository::machineLearners`, `ItemEvolution` gains the species' default Pokémon ids).
- Items picks a game group with the Pokédex's dex-chip look — [신오 D|P] [신오 Pt] [성도 HG|SS] —
  local to the page (it starts from the app's game and follows it when the squad changes it, but
  does not change it). Version differences inside the group are spelled out in the detail:
  evolutions of version-exclusive Pokémon get "(P 한정)", and version-only sources are prefixed
  with the version ("W2 — 13번 도로").
- The Pokédex detail (and the squad member modal) and the Pokédex list use one chip per version
  like the squad page; the modal still opens on the squad's version and its chips stay local.
- The squad page picks its game with one chip per version — [D] [P] [Pt] [HG] [SS], [B] [W]
  [B2] [W2] — instead of the split group chips (the Pokédex, detail and Items pages keep the group
  chips). Repeated abbreviations get the group's short name only after their first use (Sw · Sh ·
  Sw 외딴섬 …). Switching squads while the name field has focus no longer carries the old name
  into the new squad.
- Games are now played per version, not per version group: the game chips keep one chip per group
  but split into version parts (HG | SS, D | P, B2 | W2) — click a part to pick that version
  (hovering a faded part tints it; the keyboard cycles through parts). The choice is app-wide
  (`AppState::game`, saved per generation) and shared by the squad, Pokédex, Pokédex detail and
  Items pages. Squads are stored per version ("소울실버 스쿼드"); a squad saved for a whole group
  before this change is the starting point for both of its versions and splits on the first edit.
- Version differences are filtered: the Pokédex list and the squad's Pokémon picker drop species
  that only the paired version can obtain (computed per evolution chain from encounter data — HG
  hides Vulpix, Meowth, Teddiursa…; D hides Slowpoke, Misdreavus…), the detail's 획득법 lists only
  the chosen version's encounters ("이 버전에서는 만날 수 없어요. 하트골드에서 만나요"), and item
  sources marked with the new `versions` field (B2W2's Plasma Frigate / Route 13 entries) are
  hidden in the other version. The squad member modal's chips stay local to the modal.
- Heart Scale marks now follow how you actually get the Pokémon in the chosen game: a level-up
  move below the earliest level you can catch, receive or evolve it at, that it does not already
  know then (the game's last-four default moveset), needs the Move Reminder — HGSS's Lv 20 fossil
  Aerodactyl marks its Lv 1 fangs, Wing Attack and Supersonic but not Bite, Scary Face, Roar or
  Agility. Earlier stages' moves carry through evolution; breeding is not counted. The rule lives
  in `core/rules/movereach` (unit-tested); same-level moves are listed in the game's order. The
  database schema goes to 11 (learnset order column) and is rebuilt from the cached CSV.
- The evolution chain shows where to get each required item, right under its step: "물의돌 사용"
  gets a "물의돌 — 경품 · 자연공원 (벌레잡기대회 1등) / 42번 도로" line (from the chosen game's
  acquisition book; tooltips carry the full text), for both use-items and held trade items
  (왕의징표석 — 야돈우물). The squad member modal also grows to 1320×840.
- Squad slot cards gain a [+] button in the header that opens the member's full Pokédex detail as
  a modal — picture, stats, abilities, matchups, evolution chain with conditions, and every move
  table (level-up with the Heart Scale mark, TMs with locations, NPC tutors with costs), based on
  the squad's game; [← 목록] closes the modal.
- Acquisition books for every main game (30 version groups), built from a full crawl of Serebii's
  ItemDex (2,088 item pages, fetched politely) by `scripts/i18n/serebii_items.py` and
  `serebii_books.py`: 22,556 location rows resolved to PokéAPI locations (→ Korean place names),
  wild held-item entries (→ Korean species names), game-text names or translated feature labels;
  the untranslated remainder stays in English for review. The blog-sourced Platinum/HGSS entries
  win over the crawl. Because the crawl misses some sources (BP shops, some DP rows), these books
  only feed the 입수처 display — the per-game existence filter now needs a book to declare
  `"completeItemList": true`, so nothing obtainable disappears from the Items list.
- Acquisition books for HeartGold/SoulSilver and Platinum tutors, from the same Korean guide blog
  as the Platinum TM list (user-provided): all 100 HGSS TMs/HMs with how to get them (the Goldenrod
  Game Corner's 10,000-coin 냉동빔 included), 56 HGSS tutor moves (Battle Frontier BP costs plus
  the free Headbutt / Draco Meteor / ultimate-move tutors) and 42 Platinum tutor moves with their
  shard combinations and house locations. TM numbering was verified against PokéAPI's machine
  table before install.
- Korean descriptions for 88 moves, 45 abilities and 6 items that PokéAPI lacks (mostly Legends:
  Arceus and Scarlet/Violet additions), taken line-for-line from Scarlet/Violet 3.0.1, Legends:
  Arceus and Legends: Z-A text dumps and cross-checked against a second dump or wiki. The name
  dictionary can now also correct PokéAPI typos (`"fix": true`): 베껴그리기 (Doodle) and
  액셀브레이크 (Collision Course).
- Korean names for 600 places and 58 items/moves/abilities/versions that PokéAPI lacks (the
  Pokédex encounter list now reads 회색시티 rather than "Pewter City"). Values were cross-checked
  between PKHeX's game-extracted location strings, the WikiDex all-language lists and the Korean
  fan wikis; places never released in Korean (Orre, Sevii Islands, Gen 3 mail and key items) use
  fan-wiki names and are listed in `docs/i18n/review-*.md` together with every entry left out.
  The audit also fixed four Sinnoh names that came from the Platinum blog (험한 샛길,
  숲의 양옥집, 봉신마을, 유적마니아굴). `scripts/i18n/` holds the merge and validation tools.
- The Items page picks a game, not just a generation: version chips in the list header switch
  TM/HM contents to that game's machine table (Let's Go's TM01 is Headbutt, Sun/Moon's is Work Up;
  TMs the game does not have disappear), and the detail pane gains a "입수처" section from the
  game's acquisition book. Per-game acquisition books (`resources/data/acquisition/<game>.json`)
  replace the Platinum TM-location and tutor-cost dictionaries with one schema — method, PokéAPI
  location, place text, content, cost (money · BP · coins · shards …) and conditions — rendered in
  the current language ("상점 · 장막시티 · 10,000원"). When a game's book lists items beyond
  TMs, the Items page shows only the items obtainable in that game. Request templates for all 30
  games (with PokéAPI's TM and tutor lists prefilled) and identifier references live in
  `docs/i18n/`.
- A fill-in dictionary for Korean (and Japanese) names and descriptions PokéAPI lacks
  (`resources/data/names.json`, read by `data/text/namebook.h`): item, move, ability and version
  names plus item/move/ability descriptions get their empty language slots filled at run time, so
  no data re-download is needed. `docs/i18n/` holds the audit — request files listing every entry
  still missing Korean (680 places, 60 names, 186 descriptions) and the prompt for gathering
  official Korean text.
- Tutor moves are labeled "NPC 가르침" (to separate them from the Heart Scale move reminder) and
  show their cost where known — a new 비용 column right beside PP in the Pokédex detail and a
  suffix in the squad move picker. Costs carry the content they come from ("배틀프런티어 48BP").
  PokéAPI has no tutor cost data, so costs live in our own dictionary
  (`resources/data/tutor-costs.json`, per game → move, localized); it starts with Emerald's
  Battle Frontier elemental punches and grows like the TM location dictionary.
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
- The squad's physical/special split drops the rule commentary: no more "4세대~ 기술마다 판정"
  pill or "1–3세대 규칙(타입 기준)이었다면 물리 8 · 특수 12" line — the bar simply shows the split
  under the generation you are playing (the rule itself stays in the "N세대 규칙" pill's tooltip).
  The other-rule computation is removed from the analyzer.
- Copyright clean-up for the public repository: the README screenshots are re-captured without any
  game pictures; the HeartGold/SoulSilver TM/HM sources that quoted a walkthrough blog sentence by
  sentence are rewritten as facts in PokeSix's structured format (method · place · content · cost ·
  condition); `THIRD_PARTY_NOTICES.md` states what the MIT License covers and credits PokéAPI (with
  its BSD-3-Clause text, also next to the test fixtures), the guide sources, Qt and the fonts.
- Home cards with several series no longer float the later series in mid-air: each later series
  stands on a stepped riser (a bleacher tier in the card's accent tint) behind the first, the front
  centre is a little smaller, and every row stands on its own ground with a foot shadow.
- Items detail lists only Pokémon met in the chosen game: TM learners and evolution targets are
  limited to the game's regional dex plus species caught in the wild there (whole evolution chains),
  so HGSS no longer shows Giratina, Rayquaza or Arceus (event/static specials outside the Johto dex)
  while Kanto routes and the Safari Zone still count (`Repository::gameSpecies`).
- NPC tutor costs are no longer cut off: the cost column takes the width of its longest text (up
  to 380 px, beyond that it elides with a tooltip) and pushes the effect column right. Sources whose
  place and content are the same name print it once ("배틀프런티어 · 배틀프런티어 32BP" →
  "배틀프런티어 32BP"), in tutor costs and item sources alike.
- Switching the squad's game no longer stutters (about 150 ms → 5–25 ms in a debug build). Each
  Pokémon detail used to scan the whole move, move-history, stat-change and move-effect tables
  four times and the whole type/stat tables once; those queries now read only the moves and
  Pokémon they need (28 ms → 3.6 ms per detail), and `Repository` keeps the details it has read
  (HG ↔ SS share them), cleared when the database is reopened.
- Clicking another generation card on Home no longer stutters: the barrel used to switch the app's
  generation the moment the turn began, and the squad session's reload (about 120 ms in a debug
  build) froze the first frames so the card jumped. The yellow frame now moves at once and the
  generation is committed when the turn ends (or immediately when leaving Home).
- The squad member modal gets the same page margins as the Pokédex screen, so [← 목록], the game
  chips and the scrollbar no longer touch the window edges; it opens at 1360×876, shrunk to fit
  smaller screens.
- Switching the game chips (Items) or the dex chips (Pokédex list) no longer drops the selection:
  the item or species you were reading is re-selected in the refreshed list, scrolled into view,
  and the detail/preview pane stays on it. It only clears when the entry does not exist in the
  newly chosen game or dex.
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
- The HM-shuttle dialog: the shuttle's icon fits inside the pick button (36px, with a breath
  before the name), the coverage column is titled "채용" and shows each carrying member as a
  small box icon + name instead of "본편: 이름" text.
- The share image no longer shows white boxes around the cards and the analysis panel: it is
  composed with `render(DrawChildren)` instead of `grab()`, so the unpainted ring insets and
  rounded corners keep the paper background.
- The generation button reads [▾ 4세대] (chevron first), and its dropdown is a custom popup the
  same width as the button: each row's background is that generation's color stripes (replacing
  the small swatch icons), rows are separated by a 1px ink line except after the last, labels
  are centered, the current generation gets a ✓ right before its label, and arrow keys/Enter
  still work.
- The app bar drops the search box — it was a non-functional placeholder (only Ctrl+K focus was
  wired). The generation button moves to the bar's right end, with the bar's right padding set so
  the button's edge lines up with the page content below (the squad's "자동 저장됨" and the
  analysis panel).
- The squad page's two columns now end flush: slot cards paint inside a 3px selection-ring
  inset, so the analysis column is inset by the same amount top and bottom
  (`SlotCard::kRing`), and the page's bottom margin shrinks (20→12, column margin 8→4) —
  the cards grow by the difference and the window-bottom gap tightens.
- The setup scripts say so when the Qt download goes quiet: aqtinstall only prints a line per
  finished archive, and qtbase/qtdeclarative are large enough that the install can look stuck for
  10+ minutes while it is actually downloading.
- The squad analysis panel is titled "스쿼드 분석" (was "실시간 분석"), and its sections sit a
  little tighter (spacing 8→6, frame padding 12/14→10/12) to make room for the resource section.
- Squad slot card headers trade the ⋯ menu for three direct buttons — swap (⇄), clear (trash) and
  details (+), with tooltips — and the member detail modal grew to 1200×800 so its right panel is
  not clipped. "도감에서 보기", the move up/down actions (drag reorder covers them) and the old
  menu go away.
- The home generation cards became little dioramas instead of one blown-up sprite: each card shows
  its generation's three starters (the middle one front and larger) standing on shadow ellipses,
  over an accent-tinted stage with diagonal stripes and a ghosted generation numeral, with the
  region name and the national dex range (No.387–493) below. Card contents stay in
  `resources/theme/homecards.json` (now a Pokémon list plus a dex range per card).
- The chosen generation card reads at a glance: besides the yellow ring it now rises higher out of
  the fan's silhouette (lift 0.75), grows slightly with the lift (up to 1.1×, hover included) and
  carries the menu's ▶ cursor before the band title ("▶ 3세대").
- Text typed into inputs now uses the app's display font: line edits (search fields included) render
  in Do Hyeon 15px/14px instead of NanumGothic 13px, which looked like the system default. The
  role memo field keeps the body font (it holds sentences), and the squad name keeps its 28px
  title styling.
- The home screen drops its quit row and the keyboard hint strip (↑↓ · ←→ · Enter · 1–3) — the
  shortcuts themselves still work, and quitting is the window's close button; the footer keeps the
  version and the PokéAPI credit.
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
