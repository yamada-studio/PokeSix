#include "core/analysis/resourceledger.h"

#include <algorithm>

namespace com::yamada::studio::resourceledger {
Ledger tally(std::span<const TeachPlan> plans, const std::map<std::string, MachineSupply> &supplies)
{
    Ledger ledger;
    std::map<std::string, int> totals; // 단위 → 합계
    std::map<std::string, MachineUse> machines;

    for (const TeachPlan &plan : plans) {
        switch (plan.means) {
        case Means::Reminder:
            ++ledger.heartScales;
            break;
        case Means::Machine: {
            MachineUse &use = machines[plan.machine];
            use.machine = plan.machine;
            if (std::find(use.members.begin(), use.members.end(), plan.slot) == use.members.end())
                use.members.push_back(plan.slot);
            break;
        }
        case Means::Tutor:
            for (const Cost &cost : plan.cost)
                totals[cost.unit] += cost.amount;
            break;
        case Means::LevelUp:
        case Means::Other:
            break;
        }
    }

    for (auto &[machine, use] : machines) {
        if (const auto it = supplies.find(machine); it != supplies.end())
            use.supply = it->second;
        const int uses = int(use.members.size());
        const int missing = uses - use.supply.copies;
        if (use.supply.known && missing > 0) {
            if (use.supply.repeatable) {
                use.bought = missing;
                for (const Cost &cost : use.supply.repeatCost)
                    totals[cost.unit] += cost.amount * missing;
            } else {
                use.shortfall = true;
            }
        }
        ledger.machines.push_back(std::move(use));
    }

    for (const auto &[unit, amount] : totals)
        ledger.totals.push_back({amount, unit});
    return ledger;
}
} // namespace com::yamada::studio::resourceledger
