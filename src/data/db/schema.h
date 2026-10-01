#pragma once

#include <array>

// 게임 데이터 DB(pokesix.sqlite)의 스키마. CsvImporter가 빈 DB에 이 순서대로 실행한다.
// SQLite의 QSqlQuery::exec()는 문장을 하나씩만 실행하므로 문장마다 따로 둔다.
//
// [세대 구간] 세대에 따라 달라지는 값은 gen_from / gen_to 구간으로 저장한다(ADR 0011, 설계서 03
// §5).
//   세대 g의 값: gen_from <= g AND (gen_to IS NULL OR gen_to >= g)
//   세대마다 표를 따로 두지 않는 이유: 바뀌는 값이 아주 적다(상성 6줄, 타입 36마리, 종족값 235건).
//   9세대분을 복사하면 같은 값이 9번 들어가고, 고칠 곳도 9군데가 된다.
//   습득 기술(다음 체크포인트)은 원래 게임 버전(version group)별로 나뉘어 있어서 자연히 세대별이다.
//
// [종과 포켓몬] species = 도감의 한 종(이상해씨). pokemon = 실제 모습 하나(기본형 + 메가진화 ·
// 리전폼 등,
//   id 10001~). 타입 · 종족값은 pokemon에 붙는다. 기본형은 is_default = 1.
//   pokemon.intro_gen은 그 모습이 처음 나온 세대다(알로라 식스테일 = 7, 식스테일 = 1).
//
// [도감] 세대 → 게임 묶음(version_groups) → 도감(pokedex_version_groups, 다대다) → 도감 번호
//   (dex_numbers). "그 세대의 지방 도감" = 그 세대 게임 묶음에 연결된 도감이라서, 세대별 도감
//   목록을 따로 적지 않는다(4세대: 신오 DP · 신오 Pt · 성도 HGSS). 도감의 한국어 이름은 PokéAPI에
//   없어서 지방 이름(regions)과 버전 이름(versions)으로 나타낸다. 전국도감은 표로 조회하지
//   않는다(species).
//
// [아이템] 세대별 존재는 item_generations(PokéAPI item_game_indices: 그 세대 게임에 번호가 있으면
// 존재).
//   효과 문구는 item_effects에 세대마다 한 줄 — 그 세대 첫 게임(버전 그룹 순서가 가장 이른 것)의
//   설명문을 언어마다(ko · en · ja) 고른다. 그 세대에 없는 언어는 조회할 때 가까운 세대 문구로
//   대신한다(한국어는 6세대부터라 1–5세대는 6세대 문구). 분류(category)는 PokéAPI의 55가지 그대로
//   두고, 화면의 묶음(회복 · 기술머신 …)은 UI가 정한다. 기술머신은 게임마다 담긴 기술이 다르다 →
//   item_machines에 세대마다 한 줄(그 세대 첫 게임 기준). 기술의 타입도 세대에 따라
//   바뀐다(애교부리기: 5세대까지 노말, 6세대부터 페어리) → move_types 구간.
//
// [이름] 포켓몬 · 타입 이름은 ko / en / ja 열로 둔다(PokéAPI languages: 3 = ko, 9 = en, 11 = ja).
//   UI 문구(tr())와는 별개의 경로다(architecture.md §4).
namespace com::yamada::studio::schema {
inline constexpr int kVersion = 10; // 스키마를 바꾸면 올린다. meta 표에 기록된다

inline constexpr std::array kStatements = {
        // 이 DB를 만든 원천과 스키마 버전 (key = 'schema_version', 'source_commit', 'imported_at')
        R"(CREATE TABLE meta (key TEXT PRIMARY KEY, value TEXT NOT NULL))",

        R"(CREATE TABLE generations (id INTEGER PRIMARY KEY, identifier TEXT NOT NULL))",

        // intro_gen: 처음 나온 세대(강철 · 악 = 2, 페어리 = 6)
        R"(CREATE TABLE types (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, intro_gen INTEGER NOT NULL,
        name_ko TEXT, name_en TEXT, name_ja TEXT))",

        // 공격 타입 → 방어 타입의 배율(0, 0.5, 1, 2). 두 타입이 모두 존재하는 세대부터만 줄이 있다.
        R"(CREATE TABLE type_chart (
        atk_type INTEGER NOT NULL REFERENCES types(id), def_type INTEGER NOT NULL REFERENCES types(id),
        multiplier REAL NOT NULL, gen_from INTEGER NOT NULL, gen_to INTEGER))",
        R"(CREATE INDEX type_chart_pair ON type_chart (atk_type, def_type))",

        R"(CREATE TABLE stats (id INTEGER PRIMARY KEY, identifier TEXT NOT NULL))",

        R"(CREATE TABLE species (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, intro_gen INTEGER NOT NULL,
        evolves_from INTEGER REFERENCES species(id), evolution_chain INTEGER,
        is_baby INTEGER NOT NULL, is_legendary INTEGER NOT NULL, is_mythical INTEGER NOT NULL,
        sort_order INTEGER,
        name_ko TEXT, name_en TEXT, name_ja TEXT, genus_ko TEXT, genus_en TEXT, genus_ja TEXT))",

        R"(CREATE TABLE pokemon (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL,
        species_id INTEGER NOT NULL REFERENCES species(id), is_default INTEGER NOT NULL,
        intro_gen INTEGER NOT NULL, height INTEGER, weight INTEGER, sort_order INTEGER))",
        R"(CREATE INDEX pokemon_species ON pokemon (species_id))",

        R"(CREATE TABLE pokemon_types (
        pokemon_id INTEGER NOT NULL REFERENCES pokemon(id), slot INTEGER NOT NULL,
        type_id INTEGER NOT NULL REFERENCES types(id), gen_from INTEGER NOT NULL, gen_to INTEGER))",
        R"(CREATE INDEX pokemon_types_pokemon ON pokemon_types (pokemon_id))",

        R"(CREATE TABLE pokemon_stats (
        pokemon_id INTEGER NOT NULL REFERENCES pokemon(id), stat_id INTEGER NOT NULL REFERENCES stats(id),
        value INTEGER NOT NULL, gen_from INTEGER NOT NULL, gen_to INTEGER))",
        R"(CREATE INDEX pokemon_stats_pokemon ON pokemon_stats (pokemon_id))",

        R"(CREATE TABLE regions (id INTEGER PRIMARY KEY, name_ko TEXT, name_en TEXT, name_ja TEXT))",

        // sort_order: 게임이 나온 순서(version_groups.order). 버튼 순서에 쓴다
        R"(CREATE TABLE version_groups (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, generation INTEGER NOT NULL,
        sort_order INTEGER NOT NULL))",

        R"(CREATE TABLE versions (
        id INTEGER PRIMARY KEY, version_group_id INTEGER NOT NULL REFERENCES version_groups(id),
        identifier TEXT NOT NULL, name_ko TEXT, name_en TEXT, name_ja TEXT))",

        R"(CREATE TABLE pokedexes (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, region_id INTEGER REFERENCES regions(id),
        is_main_series INTEGER NOT NULL))",

        R"(CREATE TABLE pokedex_version_groups (
        pokedex_id INTEGER NOT NULL REFERENCES pokedexes(id),
        version_group_id INTEGER NOT NULL REFERENCES version_groups(id)))",

        // 도감별 번호. 목록은 (pokedex_id, number) 순서로 읽는다
        R"(CREATE TABLE dex_numbers (
        pokedex_id INTEGER NOT NULL REFERENCES pokedexes(id),
        species_id INTEGER NOT NULL REFERENCES species(id), number INTEGER NOT NULL))",
        R"(CREATE INDEX dex_numbers_dex ON dex_numbers (pokedex_id, number))",

        // pocket: 가방 주머니 identifier("medicine", "machines" …)를 그대로 둔다(주머니 표는
        // 이름뿐이라)
        R"(CREATE TABLE item_categories (id INTEGER PRIMARY KEY, identifier TEXT NOT NULL,
        pocket TEXT NOT NULL))",
        R"(CREATE TABLE items (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL,
        category_id INTEGER NOT NULL REFERENCES item_categories(id), cost INTEGER,
        name_ko TEXT, name_en TEXT, name_ja TEXT))",
        R"(CREATE TABLE item_generations (
        item_id INTEGER NOT NULL REFERENCES items(id), generation INTEGER NOT NULL,
        PRIMARY KEY (item_id, generation)))",
        // 지금(최신 게임) 값. 옛 세대 값은 move_changelog에서 고른다(Repository). damage_class: 1
        // 변화 ·
        // 2 물리 · 3 특수 — 3세대까지는 기술이 아니라 타입이 물리/특수를 정했다(core 규칙이 아니라
        // UI 표).
        R"(CREATE TABLE moves (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, intro_gen INTEGER NOT NULL,
        name_ko TEXT, name_en TEXT, name_ja TEXT, type_id INTEGER, power INTEGER, pp INTEGER,
        accuracy INTEGER, damage_class INTEGER))",
        // 옛 값: until_gen 세대까지는 이 값이었다(NULL 칸 = 그 항목은 이때 바뀌지 않았다)
        R"(CREATE TABLE move_changelog (
        move_id INTEGER NOT NULL REFERENCES moves(id), until_gen INTEGER NOT NULL,
        type_id INTEGER, power INTEGER, pp INTEGER, accuracy INTEGER))",
        R"(CREATE INDEX move_changelog_move ON move_changelog (move_id))",
        // 습득 기술(게임마다). method: 1 레벨업 · 2 교배 · 3 NPC(가르침) · 4 기술머신
        R"(CREATE TABLE pokemon_moves (
        pokemon_id INTEGER NOT NULL, version_group_id INTEGER NOT NULL, move_id INTEGER NOT NULL,
        method INTEGER NOT NULL, level INTEGER NOT NULL))",
        R"(CREATE INDEX pokemon_moves_pokemon ON pokemon_moves (pokemon_id, version_group_id))",
        // 기술머신(게임마다): 번호 · 아이템 · 담긴 기술
        R"(CREATE TABLE machines (
        version_group_id INTEGER NOT NULL, machine_number INTEGER NOT NULL,
        item_id INTEGER NOT NULL, move_id INTEGER NOT NULL))",
        R"(CREATE INDEX machines_group ON machines (version_group_id))",
        // 장소 · 야생 출현(버전 · 장소 · 방법마다 한 줄로 묶었다: 레벨 범위 · 출현 칸 확률 합)
        R"(CREATE TABLE locations (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, region_id INTEGER,
        name_ko TEXT, name_en TEXT, name_ja TEXT))",
        R"(CREATE TABLE encounter_methods (id INTEGER PRIMARY KEY, identifier TEXT NOT NULL))",
        // 진화 방법: 종(evolved_species)이 진화 전 종(species.evolves_from)에서 어떻게 진화하는지.
        // generation = 이 방법이 적용되기 시작한 게임의 세대(리피아: 4 이끼바위 → 8 리프의돌).
        // trigger: 1 레벨업 · 2 통신교환 · 3 도구 사용 · 4 빈자리(껍질) · 그 밖 = 특별한 조건
        R"(CREATE TABLE evolutions (
        evolved_species_id INTEGER NOT NULL REFERENCES species(id), generation INTEGER NOT NULL,
        trigger INTEGER NOT NULL, item_id INTEGER, min_level INTEGER, gender INTEGER,
        location_id INTEGER, held_item_id INTEGER, time_of_day TEXT, known_move_id INTEGER,
        known_move_type_id INTEGER, min_happiness INTEGER, min_beauty INTEGER, min_affection INTEGER,
        relative_stats INTEGER, party_species_id INTEGER, party_type_id INTEGER,
        trade_species_id INTEGER, needs_rain INTEGER, upside_down INTEGER))",
        R"(CREATE INDEX evolutions_species ON evolutions (evolved_species_id))",
        // 특성(3세대부터). pokemon_abilities는 칸(1 · 2 = 일반, 3 = 숨겨진 특성)마다 세대 구간 —
        // 숨겨진 특성은 5세대부터, 팬텀의 1번 칸은 6세대까지 부유. 설명문은 아이템처럼 세대 ·
        // 언어마다.
        R"(CREATE TABLE abilities (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, intro_gen INTEGER NOT NULL,
        name_ko TEXT, name_en TEXT, name_ja TEXT))",
        R"(CREATE TABLE pokemon_abilities (
        pokemon_id INTEGER NOT NULL, slot INTEGER NOT NULL, ability_id INTEGER NOT NULL,
        is_hidden INTEGER NOT NULL, gen_from INTEGER NOT NULL, gen_to INTEGER))",
        R"(CREATE INDEX pokemon_abilities_pokemon ON pokemon_abilities (pokemon_id))",
        R"(CREATE TABLE ability_effects (
        ability_id INTEGER NOT NULL, generation INTEGER NOT NULL,
        text_ko TEXT, text_en TEXT, text_ja TEXT, PRIMARY KEY (ability_id, generation)))",
        // 성격: 오르는 · 내리는 능력치(stats.id 2–6). 둘이 같으면 무보정(노력 · 수줍음 …)
        R"(CREATE TABLE natures (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, increased_stat INTEGER NOT NULL,
        decreased_stat INTEGER NOT NULL, name_ko TEXT, name_en TEXT, name_ja TEXT))",
        R"(CREATE TABLE encounters (
        pokemon_id INTEGER NOT NULL, version_id INTEGER NOT NULL, location_id INTEGER NOT NULL,
        method_id INTEGER NOT NULL, min_level INTEGER NOT NULL, max_level INTEGER NOT NULL,
        rarity INTEGER NOT NULL))",
        R"(CREATE INDEX encounters_pokemon ON encounters (pokemon_id))",
        // 기술 타입의 세대 구간(move_changelog의 옛 타입 → genranges). ??? 타입(저주 2–4세대)은
        // 빠진다
        R"(CREATE TABLE move_types (
        move_id INTEGER NOT NULL REFERENCES moves(id), type_id INTEGER NOT NULL REFERENCES types(id),
        gen_from INTEGER NOT NULL, gen_to INTEGER))",
        R"(CREATE INDEX move_types_move ON move_types (move_id))",
        R"(CREATE TABLE item_machines (
        item_id INTEGER NOT NULL REFERENCES items(id), generation INTEGER NOT NULL,
        move_id INTEGER NOT NULL REFERENCES moves(id), PRIMARY KEY (item_id, generation)))",
        // 언어마다 따로 고른 문구(한국어는 6세대부터, 영어는 3세대부터 있다). 없는 언어는 NULL
        R"(CREATE TABLE item_effects (
        item_id INTEGER NOT NULL REFERENCES items(id), generation INTEGER NOT NULL,
        text_ko TEXT, text_en TEXT, text_ja TEXT, PRIMARY KEY (item_id, generation)))",
        // 기술 효과의 뼈대(PokéAPI move_meta · moves.target_id): 대상 · 분류 · 상태이상 ·
        // 회복량(%).
        // 능력치 변화는 move_stat_changes에 기술마다 여러 줄. 둘 다 지금(최신 게임) 값이다 — 옛
        // 세대 차이(성장 · 실뿜기 …)는 UI의 move-effects.json이 덮는다
        R"(CREATE TABLE move_meta (
        move_id INTEGER PRIMARY KEY REFERENCES moves(id), target INTEGER NOT NULL,
        category INTEGER NOT NULL, ailment INTEGER NOT NULL, healing INTEGER NOT NULL))",
        R"(CREATE TABLE move_stat_changes (
        move_id INTEGER NOT NULL REFERENCES moves(id), stat_id INTEGER NOT NULL,
        change INTEGER NOT NULL))",
        // 기술 설명문(item_effects와 같은 꼴). 기술머신 설명에 쓴다 — 아이템 설명문의 한국어는
        // 6세대부터라 앞 세대 기술머신은 담긴 기술의 설명문을 가까운 세대에서 빌려 온다
        R"(CREATE TABLE move_effects (
        move_id INTEGER NOT NULL REFERENCES moves(id), generation INTEGER NOT NULL,
        text_ko TEXT, text_en TEXT, text_ja TEXT, PRIMARY KEY (move_id, generation)))",
};
} // namespace com::yamada::studio::schema
