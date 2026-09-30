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
// [이름] 포켓몬 · 타입 이름은 ko / en / ja 열로 둔다(PokéAPI languages: 3 = ko, 9 = en, 11 = ja).
//   UI 문구(tr())와는 별개의 경로다(architecture.md §4).
namespace com::yamada::studio::schema {
inline constexpr int kVersion = 2; // 스키마를 바꾸면 올린다. meta 표에 기록된다

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
};
} // namespace com::yamada::studio::schema
