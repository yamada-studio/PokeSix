# 테스트용 시드 CSV

PokéAPI 저장소([PokeAPI/pokeapi](https://github.com/PokeAPI/pokeapi), BSD-3-Clause)의 `data/v2/csv`에서
커밋 `168b1e89467054cda2e7df43ccebbb69b459497a`의 파일을 **머리줄 + 필요한 줄만** 잘라 둔 것이다.
`CsvImporter` 테스트가 인터넷 없이 돌도록 하기 위함이다([ADR 0011](../../../docs/decisions/0011-data-from-pinned-pokeapi-csv.md)).

- 통째로 둔 작은 표: `generations` · `version_groups` · `versions` · `pokedexes` · `pokedex_version_groups` · `types` · `type_efficacy` · `type_efficacy_past` · `stats`
- 이름 표(`*_names`, `version_names` · `region_names` 포함)는 ko(3) · en(9) · ja(11) 줄만
- 도감 번호(`pokemon_dex_numbers`)는 아래 시드 5종의 줄만
- 기술머신(`machines`)은 플래티넘 전체 + 시드 아이템의 줄, 그 기술머신 아이템(`items` · `item_names` · `item_game_indices`)
- 습득 기술(`pokemon_moves`)은 시드 포켓몬의 DP · Pt 줄, 기술(`moves` · `move_names` · `move_changelog`)은 거기 나오는 기술만
- 야생 출현(`encounters`)은 시드 포켓몬의 4세대 줄, 그 칸 · 구역 · 장소(`encounter_slots` · `location_areas` · `locations` · `location_names`), `encounter_methods`는 통째로
- 아이템(`items` · `item_names` · `item_game_indices` · `item_flavor_text`)은 시드 아이템 7개의 줄만(설명문은 ko · en). `item_categories` · `item_pockets`는 통째로
  - 마스터볼(1) · 상처약(17) · 불꽃의돌(82) · 각성의돌(109, 4세대~) · 먹다남은음식(211, 2세대~) · 기술머신01(305, 세대마다 문구가 다르다) · 얼음의돌(885, 7세대~)
- 포켓몬은 세대 규칙을 확인하기 좋은 것만:
  - 이상해씨(1): 1세대 전용 능력치 "특수"(stat 9)
  - 삐삐(35) · 픽시(36): 5세대까지 노말, 6세대부터 페어리
  - 식스테일(37)과 알로라 식스테일(10103): 같은 종인데 폼이 7세대에 처음 나왔다
  - 한카리아스(445): 디자인 목업의 기준 포켓몬(4세대)

줄을 고치지 않는다. 실제 파일에 있는 줄을 그대로 옮긴다.
