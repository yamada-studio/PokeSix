#include "data/models/itemfilterproxy.h"
#include "data/models/itemtablemodel.h"
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

TEST_F(RepositoryTest, ListsTheRegionalDexesOfAGeneration)
{
    Repository repository(s_dbPath);
    const QList<DexInfo> dexes = repository.dexesForGeneration(4);
    ASSERT_EQ(dexes.size(), 3); // 신오(DP) · 신오(Pt) · 성도(HGSS) — 게임이 나온 순서
    EXPECT_EQ(dexes[0].pokedexId, 5);
    EXPECT_EQ(dexes[1].pokedexId, 6);
    EXPECT_EQ(dexes[2].pokedexId, 7);
    EXPECT_EQ(dexes[0].regionKo, QStringLiteral("신오"));
    EXPECT_EQ(dexes[2].regionKo, QStringLiteral("성도"));
    EXPECT_EQ(dexes[0].identifier, QStringLiteral("original-sinnoh"));
    EXPECT_EQ(dexes[0].versions,
              (QStringList {QStringLiteral("diamond"), QStringLiteral("pearl")}));
    EXPECT_EQ(dexes[0].versionsEn,
              (QStringList {QStringLiteral("Diamond"), QStringLiteral("Pearl")}));
    EXPECT_EQ(dexes[2].versionsKo,
              (QStringList {QStringLiteral("하트골드"), QStringLiteral("소울실버")}));

    // 같은 신오도감(5)이 8세대(BDSP)에도 나온다 — 도감 목록은 게임 ↔ 세대 연결로 정해진다
    bool sinnohInGen8 = false;
    for (const DexInfo &dex : repository.dexesForGeneration(8))
        sinnohInGen8 = sinnohInGen8 || dex.pokedexId == 5;
    EXPECT_TRUE(sinnohInGen8);
}

TEST_F(RepositoryTest, RegionalDexUsesItsOwnNumbers)
{
    Repository repository(s_dbPath);
    auto numbers = [](const QList<SpeciesRow> &rows) {
        QList<int> result;
        for (const SpeciesRow &row : rows)
            result.append(row.dexNumber);
        return result;
    };

    // 신오(DP): 시드 5종 중 삐삐 100 · 픽시 101 · 한카리아스 111만 있다
    const QList<SpeciesRow> sinnoh = repository.speciesForDex(5, 4);
    EXPECT_EQ(numbers(sinnoh), (QList<int> {100, 101, 111}));
    const SpeciesRow *garchomp = find(sinnoh, 445);
    ASSERT_NE(garchomp, nullptr);
    EXPECT_EQ(garchomp->types, (QStringList {QStringLiteral("dragon"), QStringLiteral("ground")}));
    EXPECT_EQ(garchomp->total, 600); // 타입 · 종족값도 채워졌다

    // 성도(HGSS): 삐삐 41 · 픽시 42 · 식스테일 127 · 이상해씨 231
    EXPECT_EQ(numbers(repository.speciesForDex(7, 4)), (QList<int> {41, 42, 127, 231}));

    // 전국 목록에서는 도감 번호 = 종 번호
    EXPECT_EQ(find(repository.speciesForGeneration(4), 445)->dexNumber, 445);
}

TEST_F(RepositoryTest, ProxyFindsRegionalAndNationalNumbers)
{
    Repository repository(s_dbPath);
    SpeciesTableModel model;
    model.setRows(repository.speciesForDex(5, 4));
    SpeciesFilterProxy proxy;
    proxy.setSourceModel(&model);

    EXPECT_EQ(proxy.index(0, SpeciesTableModel::NumberColumn).data().toInt(), 100); // 지방 번호
    proxy.setSearchText(QStringLiteral("111"));
    EXPECT_EQ(proxy.rowCount(), 1);
    proxy.setSearchText(QStringLiteral("445"));
    EXPECT_EQ(proxy.rowCount(), 1);
}

// ── 아이템 (E3). 시드: 마스터볼 · 상처약 · 불꽃의돌 · 각성의돌(4세대~) · 얼음의돌(7세대~) ·
// 기술머신01 · 먹다남은음식(2세대~)
namespace {
const ItemRow *findItem(const QList<ItemRow> &rows, const QString &identifier)
{
    for (const ItemRow &row : rows)
        if (row.identifier == identifier)
            return &row;
    return nullptr;
}
} // namespace

TEST_F(RepositoryTest, ItemsKnowWhichGenerationsHaveThem)
{
    Repository repository(s_dbPath);
    const QList<ItemRow> items = repository.itemsForGeneration(4);
    EXPECT_EQ(items.size(), 7);

    const ItemRow *dawn = findItem(items, QStringLiteral("dawn-stone"));
    ASSERT_NE(dawn, nullptr);
    EXPECT_EQ(dawn->nameKo, QStringLiteral("각성의돌"));
    EXPECT_EQ(dawn->category, QStringLiteral("evolution"));
    EXPECT_FALSE(dawn->existsIn(3));
    EXPECT_TRUE(dawn->existsIn(4));
    EXPECT_TRUE(dawn->existsIn(9));
    EXPECT_EQ(dawn->introGeneration(), 4);

    const ItemRow *ice = findItem(items, QStringLiteral("ice-stone"));
    ASSERT_NE(ice, nullptr);
    EXPECT_FALSE(ice->existsIn(4)); // 4세대 목록에도 들어 있지만(흐리게 보인다) 4세대에는 없다
    EXPECT_EQ(ice->introGeneration(), 7);
    EXPECT_EQ(findItem(items, QStringLiteral("leftovers"))->introGeneration(), 2);
}

TEST_F(RepositoryTest, ItemEffectFollowsTheGeneration)
{
    Repository repository(s_dbPath);
    // 기술머신01은 게임마다 담긴 기술이 달라 문구가 다르다. 세대마다 그 세대 첫 게임의 문구.
    auto tm01 = [&](int generation) {
        return findItem(repository.itemsForGeneration(generation), QStringLiteral("tm01"))->effect;
    };
    const QString xy
            = QStringLiteral("손톱을 갈아 날카롭게 만든다. 자신의 공격과 명중률을 올린다.");
    EXPECT_EQ(tm01(6), xy); // XY (줄바꿈 → 띄어쓰기)
    EXPECT_EQ(tm01(7),
              QStringLiteral("스스로 분발해서 공격과 특수공격을 올린다.")); // SM (LGPE 아님)
    EXPECT_EQ(tm01(4), xy); // 1–5세대는 한국어 문구가 없어서 가장 이른 문구(6세대)
    EXPECT_EQ(tm01(9),
              QStringLiteral("굉장한 힘을 담은 킥으로 상대를 걷어차서 공격한다.")); // 8세대 문구
}

TEST_F(RepositoryTest, ItemProxyFiltersByCategoryGenerationAndText)
{
    Repository repository(s_dbPath);
    ItemTableModel model;
    model.setRows(repository.itemsForGeneration(4), 4);
    ItemFilterProxy proxy;
    proxy.setSourceModel(&model);
    EXPECT_EQ(proxy.rowCount(), 7);

    proxy.setCategoryFilter(
            [](const QString &category, const QString &) { return category == "evolution"; });
    EXPECT_EQ(proxy.rowCount(), 3); // 불꽃의돌 · 각성의돌 · 얼음의돌

    proxy.setOnlyInGeneration(true);
    EXPECT_EQ(proxy.rowCount(), 2); // 얼음의돌은 4세대에 없다

    proxy.setCategoryFilter(nullptr);
    proxy.setOnlyInGeneration(false);
    proxy.setSearchText(QStringLiteral("회복")); // 효과 문구로도 찾는다: 상처약 · 먹다남은음식
    EXPECT_EQ(proxy.rowCount(), 2);

    proxy.setSearchText(QString());
    proxy.sort(ItemTableModel::NameColumn, Qt::AscendingOrder); // 가나다순
    EXPECT_EQ(proxy.index(0, ItemTableModel::NameColumn).data().toString(),
              QStringLiteral("각성의돌"));
}
