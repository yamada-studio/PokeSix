#pragma once

#include <algorithm>
#include <optional>
#include <utility>
#include <vector>

// PokéAPI의 "옛 값" 표(*_past)를 우리 스키마의 세대 구간(gen_from / gen_to)으로 바꾸는 순수 함수.
// Qt를 쓰지 않는다 → SQL 없이 따로 테스트한다(tests/data/genranges_test.cpp).
//
// PokéAPI의 규칙: *_past의 한 줄 (g, v)는 "g세대까지는 v였다"는 뜻이다. 현재 값은 별도 표에 있다.
//
//   예) 삐삐(35)의 타입       past: (5, 노말)   current: 페어리   firstGen: 1
//       → [1–5 노말] [6– 페어리]
//   예) 고스트 → 에스퍼 상성   past: (1, ×0)    current: ×2      firstGen: 1
//       → [1–1 ×0] [2– ×2]
//   예) 이상해씨의 "특수"(1세대 전용 능력치)  past: (1, 65)  current: 없음
//       → [1–1 65]   (2세대부터는 구간이 없다 = 그 능력치가 없다)
//
// 세대 g의 값을 찾는 질의: gen_from <= g AND (gen_to IS NULL OR gen_to >= g)
namespace com::yamada::studio::genranges {
template<typename Value>
struct GenRange
{
    int from;              // 이 세대부터 (포함)
    std::optional<int> to; // 이 세대까지 (포함). nullopt = 지금까지 계속
    Value value;

    bool operator==(const GenRange &) const = default;
};

// firstGen — 이 대상이 존재하기 시작한 세대. 그 전의 past 줄은 무시한다.
//   포켓몬이면 처음 나온 세대, 상성이면 두 타입 중 늦게 나온 쪽의 세대.
//   1세대가 아니라 "처음 나온 세대"부터 시작해야 "그 세대에는 없었다"를 구분할 수 있다.
// past — (그 세대까지, 값) 목록. 순서는 상관없다.
// current — 지금 값. 없으면(nullopt) 마지막 past 이후로는 구간을 만들지 않는다.
// 이웃한 두 구간의 값이 같으면 하나로 합친다.
template<typename Value>
std::vector<GenRange<Value>> toGenRanges(int firstGen, std::vector<std::pair<int, Value>> past,
                                         std::optional<Value> current)
{
    std::sort(past.begin(), past.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    std::vector<GenRange<Value>> ranges;
    int start = firstGen;
    for (auto &[lastGen, value] : past) {
        if (lastGen < start)
            continue; // 존재하기 전의 옛 값(데이터상 드묾)
        if (!ranges.empty() && ranges.back().value == value)
            ranges.back().to = lastGen; // 앞 구간과 값이 같다 → 늘린다
        else
            ranges.push_back({start, lastGen, std::move(value)});
        start = lastGen + 1;
    }

    if (current) {
        if (!ranges.empty() && ranges.back().value == *current)
            ranges.back().to = std::nullopt;
        else
            ranges.push_back({start, std::nullopt, std::move(*current)});
    }
    return ranges;
}
} // namespace com::yamada::studio::genranges
