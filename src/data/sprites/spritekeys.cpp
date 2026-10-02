#include "data/sprites/spritekeys.h"

namespace com::yamada::studio::spritekeys {
QString front(int generation, int pokemonId)
{
    static constexpr const char *kFolders[] = {"generation-i/yellow",
                                               "generation-ii/crystal",
                                               "generation-iii/emerald",
                                               "generation-iv/platinum",
                                               "generation-v/black-white",
                                               "generation-vi/x-y",
                                               nullptr,
                                               nullptr,
                                               "generation-ix/scarlet-violet"};
    const char *folder = generation >= 1 && generation <= 9 ? kFolders[generation - 1] : nullptr;
    return folder ? QStringLiteral("%1/%2").arg(QLatin1String(folder)).arg(pokemonId)
                  : QStringLiteral("default/%1").arg(pokemonId); // 폴더 없음 → 2번째 출처(기본)
}
} // namespace com::yamada::studio::spritekeys
