// H2 — 게임 세이브 → 스쿼드. ctest -R SaveImport
// 합성 세이브(tests/core/syntheticsave.h)를 임시 파일로 쓰고, 픽스처 CSV로 만든 DB의 번호표로
// 바꾼다. 픽스처: 한카리아스 445 · 삐삐 35, 성격 개구쟁이 = 게임 번호 4 · id 17, 4세대 아이템 234 =
// item 211
#include "data/repository/repository.h"
#include "data/store/saveimport.h"
#include "data/update/csvimporter.h"
#include "syntheticsave.h"

#include <QFile>
#include <QTemporaryDir>

#include <gtest/gtest.h>

using namespace com::yamada::studio;
using synth::MemberSpec;
using synth::SlotSpec;

namespace {
class SaveImportTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        s_dir = new QTemporaryDir;
        s_dbPath = s_dir->filePath(QStringLiteral("pokesix.sqlite"));
        CsvImporter importer;
        s_ok = importer.run(QStringLiteral(POKESIX_FIXTURES_DIR "/pokeapi-csv"), s_dbPath);
    }
    static void TearDownTestSuite()
    {
        delete s_dir;
        s_dir = nullptr;
    }
    void SetUp() override { ASSERT_TRUE(s_ok); }

    // 합성 세이브를 파일로 쓰고 경로를 돌려준다
    static QString writeSave(const std::vector<std::uint8_t> &bytes, const QString &name)
    {
        const QString path = s_dir->filePath(name);
        QFile file(path);
        EXPECT_TRUE(file.open(QIODevice::WriteOnly));
        file.write(reinterpret_cast<const char *>(bytes.data()), qint64(bytes.size()));
        return path;
    }

    static MemberSpec member(std::uint16_t species, std::uint8_t origin)
    {
        MemberSpec spec;
        spec.species = species;
        spec.originGame = origin;
        spec.heldItem = 0;
        return spec;
    }

    static inline QTemporaryDir *s_dir = nullptr;
    static inline QString s_dbPath;
    static inline bool s_ok = false;
};
} // namespace

TEST(SaveImport, RecognisesSaveFilesByExtension) // H2-CP4-1
{
    EXPECT_TRUE(saveimport::isSaveFile(QStringLiteral("/a/포켓몬스터 소울실버(K).sav")));
    EXPECT_TRUE(saveimport::isSaveFile(QStringLiteral("PLATINUM.SAV")));
    EXPECT_TRUE(saveimport::isSaveFile(QStringLiteral("pt.dsv")));
    EXPECT_FALSE(saveimport::isSaveFile(QStringLiteral("squad.pks")));
    EXPECT_FALSE(saveimport::isSaveFile(QStringLiteral("save")));
}

TEST(SaveImport, NamesTheOriginGame) // H2-CP4-2
{
    EXPECT_EQ(saveimport::versionOfOriginGame(7), QStringLiteral("heartgold"));
    EXPECT_EQ(saveimport::versionOfOriginGame(8), QStringLiteral("soulsilver"));
    EXPECT_EQ(saveimport::versionOfOriginGame(10), QStringLiteral("diamond"));
    EXPECT_EQ(saveimport::versionOfOriginGame(11), QStringLiteral("pearl"));
    EXPECT_EQ(saveimport::versionOfOriginGame(12), QStringLiteral("platinum"));
    EXPECT_TRUE(saveimport::versionOfOriginGame(99).isEmpty());
}

TEST_F(SaveImportTest, TurnsAPartyIntoASquad) // H2-CP4-3 … 7
{
    MemberSpec garchomp = member(445, 12);
    garchomp.pid = 0x2313;   // % 25 = 4 → 개구쟁이 → natures.id 17
    garchomp.heldItem = 234; // 4세대 게임 번호 → item 211
    garchomp.ability = 8;    // 모래숨기 — 특성은 번호 그대로
    garchomp.moves = {89, 337, 0, 0};
    const MemberSpec clefairy = member(35, 12);
    const auto bytes = synth::buildSave(synth::seriesOf("platinum"),
                                        SlotSpec {.party = {garchomp, clefairy}});

    Repository repository(s_dbPath);
    QString error;
    const auto loaded
            = saveimport::load(writeSave(bytes, QStringLiteral("pt.sav")), repository, &error);
    ASSERT_TRUE(loaded.has_value()) << qPrintable(error);
    EXPECT_EQ(loaded->generation, 4);
    EXPECT_EQ(loaded->game, QStringLiteral("platinum"));

    const SquadMember &first = loaded->squad.members[0];
    EXPECT_EQ(first.pokemonId, 445);
    EXPECT_EQ(first.itemId, 211);
    EXPECT_EQ(first.natureId, 17);
    EXPECT_EQ(first.abilityId, 8);
    EXPECT_EQ(first.moves, (std::array<int, 4> {89, 337, 0, 0}));
    EXPECT_EQ(loaded->squad.members[1].pokemonId, 35);
    EXPECT_EQ(loaded->squad.members[1].itemId, 0); // 물건 없음은 0 그대로
    EXPECT_EQ(loaded->squad.filled(), 2);
}

TEST_F(SaveImportTest, LeavesEggsOutOfTheSquad) // H2-CP4-6
{
    MemberSpec egg = member(35, 12);
    egg.iv32 = 1u << 30;
    const auto bytes = synth::buildSave(synth::seriesOf("platinum"),
                                        SlotSpec {.party = {member(445, 12), egg}});
    Repository repository(s_dbPath);
    const auto loaded = saveimport::load(writeSave(bytes, QStringLiteral("egg.sav")), repository);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->squad.members[0].pokemonId, 445);
    EXPECT_TRUE(loaded->squad.members[1].isEmpty());
}

TEST_F(SaveImportTest, PicksTheVersionMostOfThePartyCameFrom) // H2-CP4-5
{
    const auto bytes
            = synth::buildSave(synth::seriesOf("heartgold-soulsilver"),
                               SlotSpec {.party = {member(445, 8), member(35, 8), member(36, 7)}});
    Repository repository(s_dbPath);
    const auto loaded = saveimport::load(writeSave(bytes, QStringLiteral("ss.sav")), repository);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->game, QStringLiteral("soulsilver"));
}

TEST_F(SaveImportTest, FallsBackToTheFirstVersionOfTheGame) // H2-CP4-5
{
    // 파티가 전부 다른 게임(다이아)에서 데려온 포켓몬이면 그 묶음의 첫 버전
    const auto bytes = synth::buildSave(synth::seriesOf("heartgold-soulsilver"),
                                        SlotSpec {.party = {member(445, 10)}});
    Repository repository(s_dbPath);
    const auto loaded
            = saveimport::load(writeSave(bytes, QStringLiteral("traded.sav")), repository);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->game, QStringLiteral("heartgold"));
}

TEST_F(SaveImportTest, RejectsWhatIsNotASave) // H2-CP4-3 · 4
{
    Repository repository(s_dbPath);
    QString error;
    EXPECT_FALSE(
            saveimport::load(s_dir->filePath(QStringLiteral("missing.sav")), repository, &error)
                    .has_value());
    EXPECT_FALSE(error.isEmpty());

    error.clear();
    const QString junk = writeSave(std::vector<std::uint8_t>(1000, 0), QStringLiteral("junk.sav"));
    EXPECT_FALSE(saveimport::load(junk, repository, &error).has_value());
    EXPECT_FALSE(error.isEmpty());
}

// ── H6: 5세대 세이브 → 스쿼드 (③ 구성) ─────────────────────────
// 픽스처: 성격 촐랑 = 게임 번호 9 · id 18, 5세대 아이템 234 = item 211, 버전 black · white ·
// black-2 · white-2

TEST_F(SaveImportTest, TurnsABlackWhitePartyIntoASquad) // H6-CP5
{
    MemberSpec garchomp = member(445, 21); // 블랙 출신
    garchomp.natureByte = 9;               // 5세대는 성격이 0x41에 따로 있다(촐랑)
    garchomp.heldItem = 234;
    garchomp.moves = {89, 337, 0, 0};
    const auto bytes = synth::buildTableSave(synth::seriesOf("black-white"),
                                             synth::TableSaveSpec {.party = {garchomp}});

    Repository repository(s_dbPath);
    QString error;
    const auto loaded
            = saveimport::load(writeSave(bytes, QStringLiteral("bw.sav")), repository, &error);
    ASSERT_TRUE(loaded.has_value()) << qPrintable(error);
    EXPECT_EQ(loaded->generation, 5); // 상수 4가 아니라 세이브가 알려 준 세대
    EXPECT_EQ(loaded->game, QStringLiteral("black"));
    const SquadMember &first = loaded->squad.members[0];
    EXPECT_EQ(first.pokemonId, 445);
    EXPECT_EQ(first.natureId, 18); // 촐랑
    EXPECT_EQ(first.itemId, 211);  // 5세대 번호표로 변환
    EXPECT_EQ(first.moves, (std::array<int, 4> {89, 337, 0, 0}));
}

TEST_F(SaveImportTest, PicksBlack2OrWhite2ByOrigin) // H6-CP5-3
{
    const auto bytes = synth::buildTableSave(
            synth::seriesOf("black-2-white-2"),
            synth::TableSaveSpec {.party = {member(445, 22), member(35, 22), member(36, 23)}});
    Repository repository(s_dbPath);
    const auto loaded = saveimport::load(writeSave(bytes, QStringLiteral("w2.sav")), repository);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->generation, 5);
    EXPECT_EQ(loaded->game, QStringLiteral("white-2"));
}

TEST(SaveImport, NamesTheFifthGenerationOriginGames) // H6-CP5-3
{
    EXPECT_EQ(saveimport::versionOfOriginGame(20), QStringLiteral("white"));
    EXPECT_EQ(saveimport::versionOfOriginGame(21), QStringLiteral("black"));
    EXPECT_EQ(saveimport::versionOfOriginGame(22), QStringLiteral("white-2"));
    EXPECT_EQ(saveimport::versionOfOriginGame(23), QStringLiteral("black-2"));
}
