#include "data/store/squadfile.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

namespace com::yamada::studio::squadfile {
namespace {
TEST(SquadFile, RoundTripsASquad)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    const QString path = dir.filePath(QStringLiteral("squad.json"));
    Portable portable;
    portable.generation = 4;
    portable.game = QStringLiteral("heartgold");
    portable.squad.name = QStringLiteral("하트골드 스쿼드");
    portable.squad.members[0].pokemonId = 130;
    portable.squad.members[0].moves = {89, 57, 0, 0};
    portable.squad.members[0].memo = QStringLiteral("선봉");
    portable.squad.shuttle.pokemonId = 162;
    ASSERT_TRUE(save(path, portable));

    QString error;
    const std::optional<Portable> loaded = load(path, &error);
    ASSERT_TRUE(loaded.has_value()) << qPrintable(error);
    EXPECT_EQ(loaded->generation, 4);
    EXPECT_EQ(loaded->game, QStringLiteral("heartgold"));
    EXPECT_EQ(loaded->squad, portable.squad); // 멤버 · 메모 · 기술 · 셔틀까지 그대로
    EXPECT_EQ(fileName(*loaded), QStringLiteral("pokesix-squad-4-heartgold.json"));
}

TEST(SquadFile, RejectsAForeignOrBrokenFile)
{
    QTemporaryDir dir;
    ASSERT_TRUE(dir.isValid());
    QString error;
    // 없는 파일
    EXPECT_FALSE(load(dir.filePath(QStringLiteral("missing.json")), &error).has_value());
    EXPECT_FALSE(error.isEmpty());
    // PokeSix 스쿼드 파일이 아닌 JSON
    const QString foreign = dir.filePath(QStringLiteral("foreign.json"));
    QFile file(foreign);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write("{\"hello\": 1}");
    file.close();
    error.clear();
    EXPECT_FALSE(load(foreign, &error).has_value());
    EXPECT_FALSE(error.isEmpty());
    // 깨진 JSON
    const QString broken = dir.filePath(QStringLiteral("broken.json"));
    QFile bad(broken);
    ASSERT_TRUE(bad.open(QIODevice::WriteOnly));
    bad.write("{not json");
    bad.close();
    EXPECT_FALSE(load(broken, &error).has_value());
}
} // namespace
} // namespace com::yamada::studio::squadfile
