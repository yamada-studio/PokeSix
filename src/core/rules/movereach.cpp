#include "core/rules/movereach.h"

#include <algorithm>
#include <deque>

namespace com::yamada::studio::movereach {
namespace {
constexpr std::size_t kMoveSlots = 4;
}

std::vector<int> defaultMoveset(std::span<const LevelMove> learnset, int level)
{
    std::vector<LevelMove> sorted(learnset.begin(), learnset.end());
    std::ranges::stable_sort(sorted, [](const LevelMove &a, const LevelMove &b) {
        return a.level != b.level ? a.level < b.level : a.order < b.order;
    });
    std::deque<int> known;
    for (const LevelMove &entry : sorted) {
        if (entry.level < 1 || entry.level > level)
            continue;
        if (std::ranges::find(known, entry.move) != known.end())
            continue;
        known.push_back(entry.move);
        if (known.size() > kMoveSlots)
            known.pop_front();
    }
    return {known.begin(), known.end()};
}

Reach reachableMoves(std::span<const Stage> path)
{
    Reach reach;
    for (const Stage &stage : path) {
        Reach next;
        if (reach.obtainable) {
            // 진화: 앞 단계의 기술을 그대로 갖고, 진화한 레벨부터 이 단계의 기술을 배운다
            const int evolved = std::max(stage.evolveLevel, reach.earliestLevel);
            next.obtainable = true;
            next.earliestLevel = evolved;
            next.moves = reach.moves;
            for (const LevelMove &entry : stage.learnset)
                if (entry.level == 0 || entry.level >= evolved)
                    next.moves.insert(entry.move);
        }
        if (stage.caughtLevel) {
            const int caught = *stage.caughtLevel;
            next.earliestLevel = next.obtainable ? std::min(next.earliestLevel, caught) : caught;
            next.obtainable = true;
            for (const int move : defaultMoveset(stage.learnset, caught))
                next.moves.insert(move);
            for (const LevelMove &entry : stage.learnset)
                if (entry.level > caught)
                    next.moves.insert(entry.move);
        }
        reach = std::move(next);
    }
    return reach;
}
} // namespace com::yamada::studio::movereach
