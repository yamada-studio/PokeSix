#include "data/models/speciesfilterproxy.h"
#include "data/models/speciestablemodel.h"
#include "data/repository/repository.h"
#include "data/update/csvimporter.h"

#include <QTemporaryDir>

#include <gtest/gtest.h>

using namespace com::yamada::studio;

namespace {
// 시드 CSV(이상해씨 · 삐삐 · 픽시 · 식스테일 · 한카리아스)로 DB를 한 번 만들어 모든 테스트가 같이
// 쓴다.
class RepositoryTest : public ::testing::Test
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

    static const SpeciesRow *find(const QList<SpeciesRow> &rows, int speciesId)
    {
        for (const SpeciesRow &row : rows)
            if (row.speciesId == speciesId)
                return &row;
        return nullptr;
    }

    static inline QTemporaryDir *s_dir = nullptr;
    static inline QString s_dbPath;
    static inline bool s_ok = false;
};
} // namespace

TEST_F(RepositoryTest, ListsSpeciesUpToTheGeneration)
{
    Repository repository(s_dbPath);
    EXPECT_EQ(repository.speciesForGeneration(4).size(), 5); // 시드 5종 모두 4세대까지 나왔다
    EXPECT_EQ(repository.speciesForGeneration(3).size(), 4); // 한카리아스(4세대)는 빠진다
}

TEST_F(RepositoryTest, TypesAndStatsFollowTheGeneration)
{
    Repository repository(s_dbPath);
    const auto gen5 = repository.speciesForGeneration(5);
    const auto gen6 = repository.speciesForGeneration(6);
    EXPECT_EQ(find(gen5, 35)->types,
              QStringList {QStringLiteral("normal")}); // 삐삐: 5세대까지 노말
    EXPECT_EQ(find(gen6, 35)->types, QStringList {QStringLiteral("fairy")}); // 6세대부터 페어리
    EXPECT_EQ(find(gen6, 445)->types,
              (QStringList {QStringLiteral("dragon"), QStringLiteral("ground")}));
    EXPECT_EQ(find(gen6, 445)->total, 600); // 한카리아스 108 · 130 · 95 · 80 · 85 · 102
}

TEST_F(RepositoryTest, Gen1SpecialFillsBothSpecialColumnsButCountsOnce)
{
    Repository repository(s_dbPath);
    const SpeciesRow *bulbasaur = find(repository.speciesForGeneration(1), 1);
    ASSERT_NE(bulbasaur, nullptr);
    EXPECT_EQ(bulbasaur->stats[3], 65);
    EXPECT_EQ(bulbasaur->stats[4], 65);
    EXPECT_EQ(bulbasaur->total, 45 + 49 + 49 + 65 + 45); // HP 공격 방어 특수 스피드
}

TEST_F(RepositoryTest, ProxySearchesNamesAndNumbers)
{
    Repository repository(s_dbPath);
    SpeciesTableModel model;
    model.setRows(repository.speciesForGeneration(9));
    SpeciesFilterProxy proxy;
    proxy.setSourceModel(&model);

    proxy.setSearchText(QStringLiteral("한카"));
    ASSERT_EQ(proxy.rowCount(), 1);
    EXPECT_EQ(proxy.index(0, SpeciesTableModel::NumberColumn).data().toInt(), 445);

    proxy.setSearchText(QStringLiteral("clef")); // 영어 이름으로도: Clefairy, Clefable
    EXPECT_EQ(proxy.rowCount(), 2);

    proxy.setSearchText(QString());
    // 시드 5종. 알로라 식스테일은 기본 모습이 아니라서 목록에 따로 나오지 않는다
    EXPECT_EQ(proxy.rowCount(), 5);
}

TEST_F(RepositoryTest, ProxySortsNumbersAsNumbers)
{
    Repository repository(s_dbPath);
    SpeciesTableModel model;
    model.setRows(repository.speciesForGeneration(9));
    SpeciesFilterProxy proxy;
    proxy.setSourceModel(&model);
    proxy.sort(SpeciesTableModel::TotalColumn, Qt::DescendingOrder);
    EXPECT_EQ(proxy.index(0, SpeciesTableModel::NumberColumn).data().toInt(),
              445); // 합계 600이 맨 위
}
