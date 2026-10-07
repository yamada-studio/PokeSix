#include "data/store/squadstore.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace com::yamada::studio {
namespace {
TEST(SquadStore, RoundTripsTheShuttle)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("squads.json"));
    {
        SquadStore store(path);
        Squad squad;
        squad.name = QStringLiteral("셔틀 확인");
        squad.members[0].pokemonId = 130;
        squad.shuttle.pokemonId = 143;
        squad.shuttle.moves = {57, 70, 0, 0}; // 파도타기 · 괴력
        store.setSquad(4, QStringLiteral("soulsilver"), squad);
        ASSERT_TRUE(store.flush());
    }
    SquadStore store(path);
    const Squad loaded = store.squad(4, QStringLiteral("soulsilver"));
    EXPECT_EQ(loaded.shuttle.pokemonId, 143);
    EXPECT_EQ(loaded.shuttle.moves, (std::array<int, 4> {57, 70, 0, 0}));
    EXPECT_EQ(loaded.members[0].pokemonId, 130);
    EXPECT_EQ(loaded.filled(), 1); // 비전셔틀은 본편 수에 세지 않는다
}

TEST(SquadStore, ReadsAFileSavedBeforeTheShuttle)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("squads.json"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(R"({"version": 2, "squads": {"4": {"current": "soulsilver", "games": {
        "soulsilver": {"name": "", "members": [{"pokemon": 130, "memo": "",
        "moves": [89, 0, 0, 0], "ability": 0, "nature": 0, "item": 0}]}}}}})");
    file.close();
    SquadStore store(path);
    const Squad loaded = store.squad(4, QStringLiteral("soulsilver"));
    EXPECT_EQ(loaded.members[0].pokemonId, 130); // 옛 파일 그대로 읽힌다
    EXPECT_TRUE(loaded.shuttle.isEmpty());       // 셔틀 키가 없으면 빈 자리
}
} // namespace
} // namespace com::yamada::studio
