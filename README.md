# PokeSix

A desktop companion for playing through the Pokémon series generation by generation.

PokeSix answers the questions you actually have mid-playthrough — *in this generation*:
what a Pokémon's base stats and types are, which moves it learns, and how your
six-member party holds up against every attacking type.

> **Status:** early development. Nothing is usable yet; the project skeleton,
> build system and tests are in place.

<!-- Screenshots go here once the first screens exist.
![Dex screen](docs/images/dex.png)
![SixSquad screen](docs/images/squad.png)
-->

## Features (planned, in priority order)

1. **Generation-aware Pokédex** — base stats, types, learnsets, and what changed between generations.
2. **SixSquad editor** — build a party of six and see type coverage and weakness distribution update live.
3. **Item encyclopedia** — later.
4. **Emulator overlay** — long-term goal; the architecture leaves room for it.

Game data comes from [PokéAPI](https://pokeapi.co). It is fetched on first launch,
cached in a local SQLite database, and used offline afterwards.

## Building

Requirements: a C++20 compiler, CMake 3.21+, Ninja (Linux/macOS), and **Qt 6.8 or newer**
(Widgets, Svg, Sql, Network).

One script per step, one folder per OS:

| | Linux (Ubuntu 24.04) | macOS | Windows (MSVC 2022) |
|---|---|---|---|
| Install dependencies (incl. Qt 6.8.3) | `scripts/linux/setup.sh` | `scripts/macos/setup.sh` | `scripts\windows\setup.bat` |
| Configure, build, test | `scripts/linux/build.sh` | `scripts/macos/build.sh` | `scripts\windows\build.bat` |
| Run | `scripts/linux/run.sh` | `scripts/macos/run.sh` | `scripts\windows\run.bat` |

```bash
git clone https://github.com/yamada-studio/PokeSix.git
cd PokeSix
scripts/linux/setup.sh      # system packages (asks for sudo only if something is missing) + Qt via aqtinstall
scripts/linux/build.sh      # debug build + tests;  add `release`, `--clean`, `--install <prefix>`
scripts/linux/run.sh        # builds first if needed; `--help` for options
```

Prefer plain CMake? Set `QT_ROOT_DIR` to your Qt folder (e.g. `~/Qt/6.8.3/gcc_64`) and use the presets:
`cmake --preset linux-debug && cmake --build --preset linux-debug && ctest --preset linux-debug`.

Windows (MSVC 2022), Homebrew Qt, IDE setup and common errors are covered in
**[docs/build.md](docs/build.md)**.

> Ubuntu 24.04's `apt` ships Qt 6.4, which is too old. Use the script above or the Qt online installer.

## Project layout

```
src/core/   pure C++20 domain logic — type chart, generation rules, team analysis (no Qt)
src/data/   PokéAPI client, SQLite cache, repositories, Qt item models
src/ui/     Qt Widgets screens and reusable widgets
src/app/    main window and application bootstrap
tests/      unit tests (GoogleTest) and a Qt runtime environment check
resources/  app icons, bundled fonts, style sheets (Qt resource system)
docs/       architecture, build guide, conventions, roadmap, decision records
design/      visual design packages from Claude Design (handoff-v1, handoff-v2) and design requests
```

See [docs/architecture.md](docs/architecture.md) for the layering rules.

## License

PokeSix is released under the [MIT License](LICENSE).

### Third-party software

- **Qt 6** — used under the [GNU LGPL v3](https://www.gnu.org/licenses/lgpl-3.0.html).
  PokeSix links to Qt dynamically and does not modify it, so you can replace the Qt
  libraries with any compatible build. Qt source code is available from
  <https://download.qt.io> and <https://code.qt.io>.
- **GoogleTest** — BSD-3-Clause. Downloaded at build time for tests only; not part of the application.
- **Fonts** — Do Hyeon, Nanum Gothic, Nanum Gothic Coding and Silkscreen, bundled under the
  [SIL Open Font License 1.1](https://openfontlicense.org). License texts are in `resources/fonts/OFL-*.txt`.

### Data

Pokémon data is provided by [PokéAPI](https://pokeapi.co), created by Paul Hallett and
the PokéAPI contributors. PokeSix follows PokéAPI's
[fair use policy](https://pokeapi.co/docs/v2): every response is cached
locally and the API is not polled repeatedly.

### Assets policy

This repository contains **no game assets**. Sprites, artwork, sounds and other
material from the Pokémon games are copyrighted by their owners and are never
committed here. Any such content is downloaded at runtime to your local cache for
personal use. Icons and styles under `resources/` are original to this project.

## Disclaimer

PokeSix is an unofficial fan project. It is not affiliated with, endorsed by, or
sponsored by Nintendo, Creatures Inc., GAME FREAK inc., or The Pokémon Company.
Pokémon and Pokémon character names are trademarks of Nintendo.
