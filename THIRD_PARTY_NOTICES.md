# Third-party notices

PokeSix's own source code, documentation and original artwork are released under the
[MIT License](LICENSE). The MIT License does **not** cover the third-party material listed
below; each item keeps its own terms.

## Pokémon names, game text and artwork

Pokémon, Pokémon character names, place names, item, move and ability names and their in-game
descriptions are © Nintendo, Creatures Inc. and GAME FREAK inc. and are trademarks of Nintendo.
PokeSix is an unofficial, non-commercial fan tool and is not affiliated with or endorsed by them.

- The repository contains **no game sprites, artwork, sounds or ROM data**. Pictures are downloaded
  at runtime from the [PokeAPI/sprites](https://github.com/PokeAPI/sprites) repository into your
  local cache, for personal use only.
- Names and short descriptions in `resources/data/names.json` and `resources/data/place-names.json`
  fill gaps in PokéAPI's data so the app can show them in Korean and Japanese. They are used as
  identifiers and reference only and are not licensed under MIT.
- The screenshots in `docs/screenshots/` are captured without any game pictures.

## PokéAPI data

Game data (stats, types, learnsets, evolutions, encounters, items, TM tables …) comes from the
[PokéAPI](https://github.com/PokeAPI/pokeapi) CSV files, pinned to commit
`168b1e89467054cda2e7df43ccebbb69b459497a`. The application downloads them at first launch.
The repository redistributes excerpts of those files in `tests/fixtures/pokeapi-csv/` and lists
derived from them (identifier tables under `docs/i18n/`). They are used under PokéAPI's license:

```text
Copyright (c) © 2013–2023 Paul Hallett and PokéAPI contributors (https://github.com/PokeAPI/pokeapi#contributing). Pokémon and Pokémon character names are trademarks of Nintendo.

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

* Neither the name of PokéAPI nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

## Item and tutor locations

The per-game acquisition books in `resources/data/acquisition/` (and their working copies in
`docs/i18n/acquisition/`) record **facts** — where and how an item or tutor move is obtained and
what it costs — in PokeSix's own structured format (method, place, content, cost, condition).
The facts were researched from these sources, which we thank and credit; no text from them is
reproduced:

- [Serebii.net ItemDex](https://www.serebii.net/itemdex/)
- The Korean walkthrough blog "미오시티의 작은 도서관" (TM/HM and move tutor lists for Platinum and
  HeartGold/SoulSilver)
- In-game location names, cross-checked with the game text tables published by the
  [PKHeX](https://github.com/kwsch/PKHeX) project (names only; no PKHeX code is used)

## Software and fonts

- **Qt 6** — GNU LGPL v3, linked dynamically and unmodified. Source: <https://download.qt.io>.
- **GoogleTest** — BSD-3-Clause; downloaded at build time for the tests only.
- **Do Hyeon, Nanum Gothic, Nanum Gothic Coding, Silkscreen** — SIL Open Font License 1.1;
  the license texts are in `resources/fonts/OFL-*.txt`.
- **Microsoft Visual C++ runtime** (`msvcp140*.dll`, `vcruntime140*.dll` next to `PokeSix.exe`,
  Windows packages only) — redistributable files listed in Visual Studio's REDIST
  terms, shipped unmodified app-local as those terms allow.
- **WiX Toolset 3.14** (Microsoft Reciprocal License) builds the Windows `.msi`. The toolset itself
  is not part of the packages; its license applies to the tool, not to installers made with it.
