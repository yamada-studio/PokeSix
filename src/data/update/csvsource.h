#pragma once

#include <QtGlobal>

#include <array>
#include <string_view>

// 게임 데이터의 원천: PokéAPI 저장소의 CSV 원본을 "고정된 커밋"에서 받는다 (ADR 0011).
//   https://raw.githubusercontent.com/PokeAPI/pokeapi/<kCommit>/data/v2/csv/<name>.csv
//
// 파일마다 크기와 SHA-256을 적어 둔다. 받은 파일이 이 값과 다르면 버린다.
// GoogleTest를 URL + 해시로 고정한 것과 같은 원리다: 같은 앱 버전이면 누구나 똑같은 데이터를
// 갖는다.
//
// 커밋을 올리는 방법 (앱 버전을 올릴 때):
//   1) kCommit을 새 커밋으로 바꾼다
//   2) 목록의 파일을 그 커밋에서 받아 `stat -c %s` / `sha256sum` 값으로 크기와 해시를 갱신한다
//   3) CsvImporter 테스트로 CSV 열 구성이 바뀌지 않았는지 확인한다
namespace com::yamada::studio::csvsource {
// 2026-09-26 PokeAPI/pokeapi master
inline constexpr char kCommit[] = "168b1e89467054cda2e7df43ccebbb69b459497a";

struct CsvFile
{
    std::string_view name; // 확장자 없는 파일 이름 (예: "pokemon_moves")
    qint64 size;           // 바이트
    std::string_view sha256;
};

inline constexpr auto kFiles = std::to_array<CsvFile>({
        // ── 공통: 언어(이름 표기 ko/en/ja) · 세대 · 버전 그룹(습득 기술 · 도감이 어느 게임
        // 기준인지) · 버전과
        // 지방 이름(도감 선택 버튼)
        {"languages", 291, "fbb60019a6a461783d5671a995d5f590db61792a273e90faa0ed630d102a19b8"},
        {"generations", 194, "d39634465251f1334aa0389b5adc4b4f2754eaec61f305fceb9432a3d4c96748"},
        {"version_groups", 726, "28da8d89d8eb4966941f81a9e62b3990510ed4d76dd774158246551a8e7707a7"},
        {"versions", 907, "70083465865a6a69a9aad2be3fc3915078c6ff740f14a090b1367d8ec9cfc3cd"},
        {"version_names", 8943, "23e3e9062f98e1f83d475b9eeac57ddff4375b46c175948a65c4fe8d3e1d87b4"},
        {"region_names", 999, "9380d909d2179a7f3bbad09969a0c89f623be135a100686596b6284c10231b48"},
        // ── 타입과 상성. *_past = 옛 세대의 값(예: 1세대 상성, 6세대 전 강철 내성)
        {"types", 321, "37f039c8d722f47d51ba1c5c5ecf9b7007235b1a9a1af2827645c777b70307c8"},
        {"type_names", 2843, "685230c51074cf2f723debcf827a4df4c36ab0ec7e929c806ad65a3e40958705"},
        {"type_efficacy", 2883, "cdd7de4066680414d0a2433af805f139f425d6350cbf998ecab559a0fe8ff587"},
        {"type_efficacy_past", 118,
         "0d8cb1251c7f987c7978d77d6c1b9ecfc54e04474b7584b8ecf96fef498fb9d4"},
        // ── 도감: 종 · 이름 · 모습(폼) · 폼 등장 시기 · 타입 · 종족값(과거 포함) · 특성 · 지역
        // 도감 번호 · 진화
        {"stats", 202, "ee8c541b45b64f40f0eb2545c767294b5801ce8a1410e19d8c33a54e6f1c3b9c"},
        {"pokemon_species", 56884,
         "e66e2eeb25fd3836b0ebab6bf87bbf01960aa3c0555e2bac495fa8393c5e0c45"},
        {"pokemon_species_names", 404502,
         "820cde17074cdb1c2b0595c997fb8f998e773bd5da3bb525dec85703c86c5fd9"},
        {"pokemon", 47082, "16c81c33188b0eac403aa2f759fcbe9e42c611f722d263f5b5a6a5bff9f8ce6b"},
        {"pokemon_forms", 63721,
         "99bf8f7ad4dc1f2e291357a090cef6a575623ec3cbf9030d0e33656e6e608ae2"}, // 폼이 처음 나온 버전
                                                                              // 그룹 → 그 포켓몬이
                                                                              // 처음 나온 세대
        {"pokemon_types", 19058,
         "f1fc4bfd657a034ea3bf6972423b10276424aa068577b304a78a08996425ba05"},
        {"pokemon_types_past", 428,
         "02553c38e3871f99c7ed809b944066fea3f42ef6bccd4daba19274aca01fbff0"},
        {"pokemon_stats", 94392,
         "fa2c44263a3706468682fefc3e0b3c4f5487fe9febed7d4ebcd6939c34b16aa8"},
        {"pokemon_stats_past", 3046,
         "0a074d9581369c1df13d8d4f4015fd7215b896c5dfb58107f3b529f7156950d2"},
        {"abilities", 7074, "74c3588ad48e08e54ff01f432899a4028c3e91508bbd53201df85e1f6b9e9ef3"},
        {"ability_names", 65239,
         "8acb80c42210f86ae747dc3347b060d69cd2574a9dbfaf74774ae632ca6348da"},
        {"pokemon_abilities", 37224,
         "4a79ee53d386a8ad3a89657483ddfd31594e05a5d52400fe131e5e4dfd7ec04a"},
        {"pokemon_abilities_past", 6271,
         "cc77c6d85f8323f5f52047d45671140309d19ec0eb0e5e610b68971dc9c64e02"},
        {"ability_flavor_text", 1261989,
         "5ab2165547dce3897b1d88e654f0159511680b1d84f93fd4f265752b66904adb"},
        // ── 성격(3세대부터): 오르는 · 내리는 능력치와 이름
        {"natures", 586, "3536990f6adeaa1735551a6afe798649d22426e161c937da09d3ae9a5381309c"},
        {"nature_names", 3650, "a6bf8bedb9822d0e966a3036885e33e1306172b3a6566edc22d02ff2e7155031"},
        {"pokedexes", 723, "aa6570e15092f80750431a59a8712410cc114d74ffffb75b3334df64b33b3d07"},
        {"pokedex_version_groups", 274,
         "e842691103129f6ad30b8a4de3503edfa5086f55a117630cbab6fddcfa83e82c"},
        {"pokemon_dex_numbers", 80383,
         "b8fe3353eec2b0ba4fc37e07e94ffa9f33ce96a7f9856c643a18b3ad518cd393"},
        {"evolution_chains", 2643,
         "5159f0481eaa786f2b08e58356bcc6271e0982dceeb69746aaac4637d5a47125"},
        {"evolution_triggers", 304,
         "fb4b055fdb022eab3110147de669ae243c5497559257ad1f370c929ab6309a8c"},
        {"pokemon_evolution", 37836,
         "e78be51c8805551fffccd5551a556e752e4a76ff1b52482b53b409dd3882e1dd"},
        // ── 기술: 기술 · 이름 · 변경 이력(세대별 위력 · 명중 · 타입) · 물리/특수/변화 · 습득 방법
        // · 습득 기술(가장 큼, 약 64만 줄)
        {"moves", 42322, "8aafd37bf78f19471495c05b201545180f50f0a08a2a2a844d69f9837dd39ac9"},
        {"move_names", 202670, "99e23ee38ea53d1473474d463b87651deac3cd4928750f8186feae66da45c147"},
        {"move_changelog", 3475,
         "86c1511f6a0e29dcc5a885c5b8f125ffd502afbec9f69c7f2b85a3e0e8b001f8"},
        {"move_damage_classes", 44,
         "b7101ceca4dff152537a2fb5c439ca4030b05f86442c643812c0b7b6ccf16f1b"},
        {"pokemon_move_methods", 187,
         "78b75acfcf6dca4c82da9d4851357cbbd65e61b08154eddfc2ace0efd00e408e"},
        {"pokemon_moves", 10733699,
         "22a807cef26891eeac0d0c900bd363e66baf421f7ee795bc7bc3e718f23b939e"},
        // ── 아이템: 아이템 · 이름 · 분류 · 주머니 · 세대별 존재(game indices) · 게임
        // 설명문(한국어는
        // XY~소드실드. 변환할 때 한국어 줄만 남긴다)
        {"items", 59564, "f08cd6dc30b447cb91cbe9232c79052e8521f32f6105bb1489b5bfabf4ca3241"},
        {"item_names", 498386, "e286953dda52ceddb72c85079641f965bde6eab6fed2a576e292dce053a1af67"},
        {"item_categories", 922,
         "8bfcaaf1dfe45d33a8cc374a602f2424e1c37ad68bab222f090cd95c691660d7"},
        {"item_pockets", 87, "8ba8329ca4a72fca13df87cebe1b67ea389ed61ac856d2c9b8073e6bd077aff4"},
        {"item_game_indices", 75060,
         "ad7576a23dea4e136ea887f5f04b680abc4f72131812fa011bdddeb54760c653"},
        // ── 획득법(야생 출현): 출현 · 출현 칸(방법 · 확률) · 방법 · 장소 구역 · 장소 · 장소 이름
        {"encounters", 3172057, "93ba8853ac871719d230a6406c91f8418754fdc773f2f4b9f476007f743b4955"},
        {"encounter_slots", 57659,
         "597dc58df2b0a2c64cedce6d6a2a08feeecbd1cf860bdf926cd27f119417d87a"},
        {"encounter_methods", 1299,
         "99327ac41c463b8bcca18d04b56cad3d5532fec70dd7eb869b9ca650a007e291"},
        {"location_areas", 26734,
         "4706b96842166a44ca619da0add34c212088669270a478d9e84bc78e9f581840"},
        {"locations", 23389, "2af5d6a1151d5ae402f9da1d5513bc1c7437795e69b140787c7aab3f6ed7eaa3"},
        {"location_names", 116637,
         "4f901da13ab1fcd4cfeee99dd653652c2bd10afe21739f47a99ee906a99a004e"},
        {"machines", 32831, "f9df7faaf0a60896fa344abd2b639f83f79c2a24e1fd4bb3560ca7da76e9a6ad"},
        {"item_flavor_text", 6281294,
         "f85281c97e4a423fb6ae673a3d2cfab116ac2e7bb9fcdecfb43ac8b1e1fb8676"},
        {"move_flavor_text", 5392794,
         "43177df8d76dac477fc837aa19b2a50d74ee2c94e47fb827506768c58b360087"},
});

// 전부 합친 크기. 첫 실행 화면의 "예상 크기"와 진행 막대에 쓴다. (28.9 MB)
inline constexpr qint64 kTotalSize = [] {
    qint64 sum = 0;
    for (const CsvFile &file : kFiles)
        sum += file.size;
    return sum;
}();
} // namespace com::yamada::studio::csvsource
