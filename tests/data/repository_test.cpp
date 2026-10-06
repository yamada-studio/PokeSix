#include "data/models/itemfilterproxy.h"
#include "data/models/itemtablemodel.h"
#include "data/models/speciesfilterproxy.h"
#include "data/models/speciestablemodel.h"
#include "data/repository/repository.h"
#include "data/state/appstate.h"
#include "data/state/squadsession.h"
#include "data/store/squadstore.h"
#include "data/update/csvimporter.h"

#include <QFile>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
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
    const QList<SpeciesRow> gen1
            = repository.speciesForGeneration(1); // 포인터가 가리킬 목록을 살려 둔다
    const SpeciesRow *bulbasaur = find(gen1, 1);
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

TEST_F(RepositoryTest, FlagsLegendaryAndFinalEvolution)
{
    Repository repository(s_dbPath);
    const QList<SpeciesRow> gen4 = repository.speciesForGeneration(4);
    // 삐삐는 픽시가 있으니 최종 진화가 아니고, 픽시 · 한카리아스는 최종 진화다
    EXPECT_FALSE(find(gen4, 35)->finalEvolution);
    EXPECT_TRUE(find(gen4, 36)->finalEvolution);
    EXPECT_TRUE(find(gen4, 445)->finalEvolution);
    EXPECT_FALSE(find(gen4, 445)->legendary); // 시드에 전설 · 환상은 없다
}

TEST_F(RepositoryTest, ListsEvolutionsWithAnItem)
{
    Repository repository(s_dbPath);
    // 시드: 픽시는 달의돌(81)로 진화(1세대 규칙)
    const QList<ItemEvolution> stone = repository.evolutionsWithItem(81, 4);
    ASSERT_EQ(stone.size(), 1);
    EXPECT_EQ(stone.first().from.en, QStringLiteral("Clefairy"));
    EXPECT_EQ(stone.first().to.en, QStringLiteral("Clefable"));
    EXPECT_FALSE(stone.first().held);
    EXPECT_TRUE(repository.evolutionsWithItem(999, 4).isEmpty()); // 진화와 무관한 아이템
}

TEST_F(RepositoryTest, ProxyFiltersByTypeTotalAndFlags)
{
    // 프록시 조건은 DB가 필요 없다 — 행을 손으로 만들어 조건마다 확인한다
    SpeciesRow dragonite;
    dragonite.speciesId = 149;
    dragonite.dexNumber = 149;
    dragonite.name.ko = QStringLiteral("망나뇽");
    dragonite.types = {QStringLiteral("dragon"), QStringLiteral("flying")};
    dragonite.total = 600;
    SpeciesRow dratini;
    dratini.speciesId = 147;
    dratini.dexNumber = 147;
    dratini.name.ko = QStringLiteral("미뇽");
    dratini.types = {QStringLiteral("dragon")};
    dratini.total = 300;
    dratini.finalEvolution = false;
    SpeciesRow dialga;
    dialga.speciesId = 483;
    dialga.dexNumber = 483;
    dialga.name.ko = QStringLiteral("디아루가");
    dialga.types = {QStringLiteral("steel"), QStringLiteral("dragon")};
    dialga.total = 680;
    dialga.legendary = true;
    SpeciesRow pikachu;
    pikachu.speciesId = 25;
    pikachu.dexNumber = 25;
    pikachu.name.ko = QStringLiteral("피카츄");
    pikachu.types = {QStringLiteral("electric")};
    pikachu.total = 320;
    pikachu.finalEvolution = false;

    SpeciesTableModel model;
    model.setRows({dragonite, dratini, dialga, pikachu});
    SpeciesFilterProxy proxy;
    proxy.setSourceModel(&model);

    proxy.setTypes({QStringLiteral("dragon")}); // 하나라도 가지면 통과
    EXPECT_EQ(proxy.rowCount(), 3);
    proxy.setExcludeLegendary(true); // 디아루가가 빠진다
    EXPECT_EQ(proxy.rowCount(), 2);
    proxy.setFinalEvolutionOnly(true); // 미뇽이 빠진다
    EXPECT_EQ(proxy.rowCount(), 1);
    EXPECT_EQ(proxy.index(0, SpeciesTableModel::NumberColumn).data().toInt(), 149);

    proxy.clearFilters(); // 전부 돌아온다
    EXPECT_EQ(proxy.rowCount(), 4);
    proxy.setTotalRange(310, 650); // 미뇽(300) · 디아루가(680)가 빠진다
    EXPECT_EQ(proxy.rowCount(), 2);
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
    EXPECT_EQ(dexes[0].region.ko, QStringLiteral("신오"));
    EXPECT_EQ(dexes[2].region.ko, QStringLiteral("성도"));
    EXPECT_EQ(dexes[0].identifier, QStringLiteral("original-sinnoh"));
    EXPECT_EQ(dexes[0].versions,
              (QStringList {QStringLiteral("diamond"), QStringLiteral("pearl")}));
    EXPECT_EQ(dexes[0].versionNames.at(0).en, QStringLiteral("Diamond"));
    EXPECT_EQ(dexes[0].versionNames.at(1).ko, QStringLiteral("펄기아"));
    EXPECT_EQ(dexes[2].versionNames.at(0).ko, QStringLiteral("하트골드"));
    EXPECT_EQ(dexes[0].region.en, QStringLiteral("Sinnoh"));

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
    // 시드 아이템 7개 + 플래티넘 기술머신 · 비전머신(도감 상세의 기술머신 번호에 쓴다)
    EXPECT_EQ(items.size(), 106);

    const ItemRow *dawn = findItem(items, QStringLiteral("dawn-stone"));
    ASSERT_NE(dawn, nullptr);
    EXPECT_EQ(dawn->name.ko, QStringLiteral("각성의돌"));
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

TEST_F(RepositoryTest, MachineContentsFollowTheGame)
{
    Repository repository(s_dbPath);
    // 같은 7세대라도 기술머신01은 썬문 = 분발, 레츠고 = 박치기 (PokéAPI machines는 게임 단위)
    auto tm01 = [&](const QString &versionGroup) {
        return findItem(repository.itemsForGeneration(7, versionGroup), QStringLiteral("tm01"))
                ->machineMove.ko;
    };
    EXPECT_EQ(tm01(QStringLiteral("sun-moon")), QStringLiteral("분발"));
    EXPECT_EQ(tm01(QStringLiteral("lets-go-pikachu-lets-go-eevee")), QStringLiteral("박치기"));
    // 그 게임의 기술머신 표에 없는 기술머신은 게임을 고르면 빠진다(픽스처: 기술머신02는 레츠고에
    // 없다)
    const QList<ItemRow> letsGo
            = repository.itemsForGeneration(7, QStringLiteral("lets-go-pikachu-lets-go-eevee"));
    for (const ItemRow &row : letsGo)
        if (row.pocket == QStringLiteral("machines"))
            EXPECT_FALSE(row.machineMove.isEmpty()) << qPrintable(row.identifier);
    // 게임을 주지 않으면 세대 기준(그 세대 첫 게임 = 썬문)
    EXPECT_EQ(findItem(repository.itemsForGeneration(7), QStringLiteral("tm01"))->machineMove.ko,
              QStringLiteral("분발"));
}

TEST_F(RepositoryTest, ItemEffectFollowsTheGeneration)
{
    Repository repository(s_dbPath);
    // 기술머신01은 게임마다 담긴 기술이 다르다. 문구는 아이템 설명문이 아니라 담긴 기술의
    // 설명문이다 (기술 설명문은 세대가 달라도 같은 기술을 설명하니 가까운 세대에서 빌려도 된다).
    auto tm01 = [&](int generation) {
        return findItem(repository.itemsForGeneration(generation), QStringLiteral("tm01"))
                ->effect.ko;
    };
    EXPECT_EQ(tm01(6), // XY 손톱갈기 (줄바꿈 → 띄어쓰기)
              QStringLiteral("손톱을 갈아 날카롭게 한다. 자신의 공격과 명중률을 올린다."));
    // 4세대 기술머신01 = 힘껏펀치. 한국어 기술 설명문은 XY부터라 6세대 문구를 빌린다
    EXPECT_EQ(tm01(4),
              QStringLiteral(
                      "정신력을 높여 펀치를 날린다. 기술을 쓰기 전에 공격을 받으면 실패한다."));
    EXPECT_FALSE(tm01(9).isEmpty());
    // 다른 아이템은 가장 가까운 세대의 문구를 빌린다(각성의돌: 4세대 → 6세대 문구)
    EXPECT_EQ(findItem(repository.itemsForGeneration(4), QStringLiteral("dawn-stone"))->effect.ko,
              QStringLiteral("어느 특정 포켓몬을 진화시키는 이상한 돌. 눈동자처럼 아름답다."));
}

TEST_F(RepositoryTest, ItemProxyFiltersByCategoryGenerationAndText)
{
    Repository repository(s_dbPath);
    ItemTableModel model;
    model.setRows(repository.itemsForGeneration(4), 4);
    ItemFilterProxy proxy;
    proxy.setSourceModel(&model);
    EXPECT_EQ(proxy.rowCount(), 106);

    proxy.setCategoryFilter(
            [](const QString &category, const QString &) { return category == "evolution"; });
    EXPECT_EQ(proxy.rowCount(), 3); // 불꽃의돌 · 각성의돌 · 얼음의돌

    proxy.setOnlyInGeneration(true);
    EXPECT_EQ(proxy.rowCount(), 2); // 얼음의돌은 4세대에 없다

    // 기술머신을 뺀 나머지에서 효과 문구로 찾는다: 상처약 · 먹다남은음식
    proxy.setCategoryFilter(
            [](const QString &, const QString &pocket) { return pocket != "machines"; });
    proxy.setOnlyInGeneration(false);
    proxy.setSearchText(QStringLiteral("회복"));
    EXPECT_EQ(proxy.rowCount(), 2);

    proxy.setSearchText(QString());
    proxy.sort(ItemTableModel::NameColumn, Qt::AscendingOrder); // 가나다순
    EXPECT_EQ(proxy.index(0, ItemTableModel::NameColumn).data().toString(),
              QStringLiteral("각성의돌"));
}

TEST_F(RepositoryTest, MachineHoldsTheMoveOfTheGeneration)
{
    Repository repository(s_dbPath);
    // 기술머신01에 담긴 기술은 세대마다 다르다(그 세대 첫 게임 기준). 타입은 아이콘 CD 색 · 타입
    // 칩.
    auto tm01 = [&](int generation) {
        const QList<ItemRow> items = repository.itemsForGeneration(generation);
        const ItemRow *row = findItem(items, QStringLiteral("tm01"));
        return std::pair(row->machineMove.ko, row->machineType);
    };
    EXPECT_EQ(tm01(1), std::pair(QStringLiteral("메가톤펀치"), QStringLiteral("normal")));
    EXPECT_EQ(tm01(4), std::pair(QStringLiteral("힘껏펀치"), QStringLiteral("fighting")));
    EXPECT_EQ(tm01(5), std::pair(QStringLiteral("손톱갈기"), QStringLiteral("dark")));
    EXPECT_EQ(tm01(7),
              std::pair(QStringLiteral("분발"), QStringLiteral("normal"))); // SM(LGPE 아님)

    // 기술머신이 아닌 아이템은 비어 있고, 가격은 그대로
    const QList<ItemRow> items = repository.itemsForGeneration(4);
    const ItemRow *potion = findItem(items, QStringLiteral("potion"));
    EXPECT_TRUE(potion->machineMove.isEmpty());
    EXPECT_EQ(potion->cost, 200);
}

TEST_F(RepositoryTest, EffectsAreKeptPerLanguage)
{
    Repository repository(s_dbPath);
    // 4세대 기술머신01(힘껏펀치): 언어마다 따로 고른다 — 영어는 DP 문구, 한국어는 XY 문구
    const QList<ItemRow> items
            = repository.itemsForGeneration(4); // 포인터가 가리킬 목록을 살려 둔다
    const ItemRow *tm01 = findItem(items, QStringLiteral("tm01"));
    EXPECT_TRUE(tm01->effect.ko.startsWith(QStringLiteral("정신력을")));
    EXPECT_FALSE(tm01->effect.en.isEmpty());
    EXPECT_EQ(tm01->effect.text(Language::Korean), tm01->effect.ko);
    EXPECT_EQ(tm01->machineMove.en, QStringLiteral("Focus Punch"));
}

TEST(LocalizedText, FallsBackInTheLanguageOrder)
{
    const LocalizedText onlyEnglish {QString(), QStringLiteral("Orange Mail"), QString()};
    EXPECT_EQ(onlyEnglish.text(Language::Korean), QStringLiteral("Orange Mail"));
    const LocalizedText noEnglish {QStringLiteral("마스터볼"), QString(),
                                   QStringLiteral("マスターボール")};
    EXPECT_EQ(noEnglish.text(Language::English),
              QStringLiteral("마스터볼")); // 영어 → 한국어 → 일본어
    EXPECT_EQ(noEnglish.text(Language::Japanese), QStringLiteral("マスターボール"));
    EXPECT_EQ(languageFromCode(QStringLiteral("ja")), Language::Japanese);
    EXPECT_EQ(languageFromCode(QStringLiteral("xx")), Language::Korean);
}

TEST_F(RepositoryTest, DetailFollowsTheRepresentativeGame)
{
    Repository repository(s_dbPath);
    // 4세대 대표 게임 = 플래티넘. 레벨업 · 기술머신은 Pt 습득 기술, 획득법은 DP · Pt · HGSS 전부.
    EXPECT_EQ(Repository::representativeVersionGroup(4), QStringLiteral("platinum"));
    const PokemonDetail bulbasaur = repository.pokemonDetail(1, 4);
    ASSERT_TRUE(bulbasaur.isValid());
    EXPECT_EQ(bulbasaur.genus.ko, QStringLiteral("씨앗포켓몬"));
    EXPECT_EQ(bulbasaur.total, 318);
    ASSERT_EQ(bulbasaur.levelMoves.size(), 14); // 포딕 4세대와 같다
    EXPECT_EQ(bulbasaur.levelMoves.first().level, 1);
    EXPECT_EQ(bulbasaur.levelMoves.first().name.ko, QStringLiteral("몸통박치기"));
    EXPECT_EQ(bulbasaur.levelMoves.first().power, 35); // 4세대 위력(지금은 40) — move_changelog
    EXPECT_EQ(bulbasaur.machineMoves.size(), 28);      // 기술머신 25 + 비전머신 3
    EXPECT_EQ(bulbasaur.machineMoves.first().machineNumber, 6); // TM06 맹독
    EXPECT_TRUE(bulbasaur.machineMoves.last().hiddenMachine);
    EXPECT_EQ(bulbasaur.machineMoves.last().machineNumber, 6); // HM06 바위깨기(101–108 → 1–8)

    // 획득법: HGSS 태초마을에서 받는다(선물)
    ASSERT_FALSE(bulbasaur.encounters.isEmpty());
    EXPECT_EQ(bulbasaur.encounters.first().location, QStringLiteral("pallet-town"));
    EXPECT_EQ(bulbasaur.encounters.first().method, QStringLiteral("gift"));

    // 1세대: 물리 · 특수는 타입이 정한다(덩굴채찍 = 풀 → 특수)
    for (const MoveEntry &move : repository.pokemonDetail(1, 4).levelMoves)
        if (move.name.en == QLatin1String("Vine Whip")) {
            EXPECT_EQ(move.damageClass, 2); // 4세대부터는 물리
        }
}

TEST_F(RepositoryTest, TypeChartFollowsTheGeneration)
{
    Repository repository(s_dbPath);
    EXPECT_EQ(repository.typeChart(4).at(QStringLiteral("ghost"), QStringLiteral("steel")), 0.5);
    EXPECT_EQ(repository.typeChart(6).at(QStringLiteral("ghost"), QStringLiteral("steel")), 1.0);
    EXPECT_EQ(repository.typeChart(1).types.size(), 15); // 악 · 강철 · 페어리 없음
}

TEST_F(RepositoryTest, EvolutionTreeAndReminderMoves)
{
    Repository repository(s_dbPath);
    // 삐삐 → 픽시(달의돌 사용). 시드에 없는 삐(173)는 빠지고, 삐삐가 뿌리가 된다.
    const PokemonDetail clefable = repository.pokemonDetail(36, 4);
    ASSERT_EQ(clefable.evolution.size(), 2);
    EXPECT_EQ(clefable.evolution[0].speciesId, 35);
    EXPECT_EQ(clefable.evolution[0].depth, 0);
    EXPECT_EQ(clefable.evolution[1].speciesId, 36);
    EXPECT_EQ(clefable.evolution[1].depth, 1);
    ASSERT_FALSE(clefable.evolution[1].conditions.isEmpty());
    EXPECT_EQ(clefable.evolution[1].conditions.first().trigger, 3); // 도구 사용

    // 픽시의 Lv 1 기술(노래하기 · 연속뺨치기 …)은 삐삐가 레벨업으로 배운다 → 하트비늘 아님
    for (const MoveEntry &move : clefable.levelMoves) {
        EXPECT_FALSE(move.needsReminder) << move.name.en.toStdString();
    }
    // 기본 단계의 Lv 1 기술(이상해씨 몸통박치기)도 아님. 진화하지 않는 시드(한카리아스만 있는
    // 사슬)는 트리 없음
    for (const MoveEntry &move : repository.pokemonDetail(1, 4).levelMoves) {
        EXPECT_FALSE(move.needsReminder);
    }
    EXPECT_TRUE(repository.pokemonDetail(445, 4).evolution.isEmpty());
}

TEST_F(RepositoryTest, ReminderMovesFollowTheObtainLevel)
{
    Repository repository(s_dbPath);
    // 시드의 DP 삐삐는 Lv 16부터 나온다. 그때 가진 기술은 Lv 16 이하의 마지막 4개(노래하기 ·
    // 연속뺨치기 · 웅크리기 · 날따름)라 Lv 1 막치기(1) · 울음소리(45), Lv 4 앵콜(227)은
    // 하트비늘이다.
    const PokemonDetail clefairy = repository.pokemonDetail(35, 4, QStringLiteral("diamond-pearl"));
    EXPECT_EQ(clefairy.earliestLevel, 16);
    for (const MoveEntry &move : clefairy.levelMoves) {
        const bool forgotten = move.moveId == 1 || move.moveId == 45 || move.moveId == 227;
        EXPECT_EQ(move.needsReminder, forgotten) << move.moveId;
    }
    // 픽시(달의돌 — 레벨 조건 없음)는 삐삐를 얻은 레벨에 바로 진화할 수 있다. Lv 1 기술 넷은 모두
    // 삐삐가 기본으로 갖고 있거나 그 뒤에 배운다.
    const PokemonDetail clefable = repository.pokemonDetail(36, 4, QStringLiteral("diamond-pearl"));
    EXPECT_EQ(clefable.earliestLevel, 16);
    ASSERT_FALSE(clefable.levelMoves.isEmpty());
    for (const MoveEntry &move : clefable.levelMoves)
        EXPECT_FALSE(move.needsReminder) << move.moveId;
}

TEST_F(RepositoryTest, DetailUsesTheChosenGame)
{
    Repository repository(s_dbPath);
    // 신오(DP) 도감에서 열면 디아루가 · 펄기아 기준
    const PokemonDetail dp = repository.pokemonDetail(1, 4, QStringLiteral("diamond-pearl"));
    EXPECT_EQ(dp.versionGroup, QStringLiteral("diamond-pearl"));
    EXPECT_EQ(dp.groupVersions, (QStringList {QStringLiteral("diamond"), QStringLiteral("pearl")}));
    EXPECT_FALSE(dp.levelMoves.isEmpty());
    // 다른 세대 게임을 주면(세대를 바꿨다) 그 세대의 대표 게임으로 대신한다
    EXPECT_EQ(repository.pokemonDetail(1, 4, QStringLiteral("sword-shield")).versionGroup,
              QStringLiteral("platinum"));
    // 도감 정보에도 게임 묶음이 실린다(신오 DP = diamond-pearl)
    EXPECT_EQ(repository.dexesForGeneration(4).first().versionGroups,
              QStringList {QStringLiteral("diamond-pearl")});
}

TEST_F(RepositoryTest, AbilitiesFollowTheGeneration)
{
    Repository repository(s_dbPath);
    auto names = [&](int pokemonId, int generation) {
        QStringList list;
        for (const AbilityEntry &a : repository.pokemonDetail(pokemonId, generation).abilities)
            list.append((a.hidden ? QStringLiteral("*") : QString()) + a.name.en);
        return list;
    };
    EXPECT_TRUE(names(36, 2).isEmpty()); // 특성은 3세대부터
    // 픽시: 3세대는 헤롱헤롱바디 하나, 4세대부터 매직가드, 5세대부터 숨겨진 특성(천진)
    EXPECT_EQ(names(36, 3), QStringList {QStringLiteral("Cute Charm")});
    EXPECT_EQ(names(36, 4),
              (QStringList {QStringLiteral("Cute Charm"), QStringLiteral("Magic Guard")}));
    EXPECT_EQ(names(36, 5).size(), 3);
    EXPECT_TRUE(names(36, 5).last().startsWith(QLatin1Char('*')));
    // 효과 문구: 한국어는 6세대부터라 3세대에서는 6세대 문구를 빌린다
    EXPECT_FALSE(repository.pokemonDetail(36, 3).abilities.first().effect.ko.isEmpty());
}

TEST_F(RepositoryTest, NaturesRaiseOneStatAndLowerAnother)
{
    Repository repository(s_dbPath);
    const QList<Nature> natures = repository.natures();
    ASSERT_EQ(natures.size(), 25);
    int neutral = 0;
    for (const Nature &n : natures)
        neutral += n.isNeutral() ? 1 : 0;
    EXPECT_EQ(neutral, 5); // 노력 · 온순 · 수줍음 · 변덕 · 성실
    for (const Nature &n : natures) {
        if (n.name.en == QLatin1String("Timid")) { // 겁쟁이: 스피드 ▲ · 공격 ▼
            EXPECT_EQ(n.increasedStat, 6);
            EXPECT_EQ(n.decreasedStat, 2);
            EXPECT_EQ(n.name.ko, QStringLiteral("겁쟁이"));
        }
    }
}

TEST(SquadStore, SavesAndLoadsSquadsPerGame)
{
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("squads.json"));
    Squad squad;
    squad.name = QStringLiteral("신오 정주행");
    squad.members[0].pokemonId = 445;
    squad.members[0].moves = {89, 200, 444, 14};
    squad.members[0].memo = QStringLiteral("에이스");
    squad.members[0].natureId = 4;
    {
        SquadStore store(path);
        store.setSquad(4, QStringLiteral("platinum"), squad);
        store.setCurrentGame(4, QStringLiteral("platinum"));
        EXPECT_TRUE(store.hasPendingSave()); // 바로 쓰지 않는다(디바운스)
        EXPECT_TRUE(store.flush());
    }
    SquadStore reloaded(path);
    EXPECT_EQ(reloaded.squad(4, QStringLiteral("platinum")), squad);
    EXPECT_EQ(reloaded.squad(4, QStringLiteral("heartgold-soulsilver")).filled(),
              0); // 게임마다 따로
    EXPECT_EQ(reloaded.squad(3, QStringLiteral("emerald")).filled(), 0);
    EXPECT_EQ(reloaded.currentGame(4), QStringLiteral("platinum"));
}

TEST(SquadStore, MovesVersion1SquadsToTheirGame)
{
    // version 1: 세대마다 스쿼드 하나. versionGroup이 있으면 그 게임, 없으면 세대의 대표 게임으로
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("squads.json"));
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(R"({"version": 1, "squads": {
        "3": {"name": "", "versionGroup": "firered-leafgreen", "members": [{"pokemon": 6}]},
        "4": {"name": "신오", "versionGroup": "", "members": [{"pokemon": 445}]}}})");
    file.close();
    SquadStore store(path);
    EXPECT_EQ(store.squad(3, QStringLiteral("firered-leafgreen")).members[0].pokemonId, 6);
    EXPECT_EQ(store.currentGame(3), QStringLiteral("firered-leafgreen"));
    EXPECT_EQ(store.squad(4, QStringLiteral("platinum")).name, QStringLiteral("신오"));
    EXPECT_EQ(store.currentGame(4), QStringLiteral("platinum"));
}

TEST_F(RepositoryTest, SquadSessionResolvesAndAnalyzesTheGeneration)
{
    // AppState는 QSettings에 세대를 쓴다 → 사용자 설정을 건드리지 않게 임시 폴더로
    QTemporaryDir settings;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settings.path());
    QTemporaryDir dir;
    Repository repository(s_dbPath);
    SquadStore store(dir.filePath(QStringLiteral("squads.json")));
    AppState state;
    state.setGeneration(4);
    SquadSession session(&repository, &store, &state);

    // 4세대 게임: DP · Pt · HGSS (외전은 도감이 없어 빠진다)
    QStringList games;
    for (const GameInfo &game : session.games())
        games.append(game.versionGroup);
    EXPECT_EQ(games, (QStringList {QStringLiteral("diamond-pearl"), QStringLiteral("platinum"),
                                   QStringLiteral("heartgold-soulsilver")}));
    EXPECT_EQ(session.versionGroup(), QStringLiteral("platinum")); // 고르지 않으면 대표 게임

    session.setPokemon(0, 445); // 한카리아스
    EXPECT_EQ(session.detail(0).types,
              (QStringList {QStringLiteral("dragon"), QStringLiteral("ground")}));
    // 지진(기술머신) · 역린(가르침) · 스톤에지(기술머신) · 칼춤(기술머신)
    for (int i = 0; i < 4; ++i)
        session.setMove(0, i, std::array {89, 200, 444, 14}[std::size_t(i)]);
    ASSERT_TRUE(session.slotMoves(0)[1].has_value());
    EXPECT_TRUE(session.slotMoves(0)[1]->learnable);
    EXPECT_EQ(session.slotMoves(0)[1]->move.name.en, QStringLiteral("Outrage"));

    // 배우는 방법을 한 줄로: 지진 = TM26, 역린 = 가르침
    bool earthquake = false, outrage = false;
    for (const SquadSession::LearnableMove &m : session.learnableMoves(0)) {
        earthquake = earthquake || (m.move.moveId == 89 && m.machine == QStringLiteral("TM26"));
        outrage = outrage || (m.move.moveId == 200 && m.tutor);
    }
    EXPECT_TRUE(earthquake);
    EXPECT_TRUE(outrage);

    // 분석: ×4 얼음 · 물리 3 · 변화 1
    const SquadAnalysis &a = session.analysis();
    EXPECT_EQ(a.filled, 1);
    EXPECT_DOUBLE_EQ(a.received[0][std::size_t(Type::Ice)], 4);
    EXPECT_EQ(a.split.physical, 3);
    EXPECT_EQ(a.split.status, 1);
    ASSERT_FALSE(a.problems.empty());
    EXPECT_EQ(a.problems.front().kind, ProblemKind::Quad);

    // 특성은 첫 칸(모래숨기)으로 미리 골라 둔다
    ASSERT_NE(session.ability(0), nullptr);
    EXPECT_EQ(session.ability(0)->name.en, QStringLiteral("Sand Veil"));

    // 끌어서 옮기기: 0번을 2번 자리로 → 1 · 2번이 한 칸씩 당겨진다
    session.setPokemon(1, 36);
    session.setPokemon(2, 37);
    session.moveSlot(0, 2);
    EXPECT_EQ(session.squad().members[0].pokemonId, 36);
    EXPECT_EQ(session.squad().members[1].pokemonId, 37);
    EXPECT_EQ(session.squad().members[2].pokemonId, 445);
    EXPECT_EQ(session.detail(2).speciesId, 445); // 풀어 둔 값도 같이 옮겨진다
    EXPECT_EQ(store.squad(4, QStringLiteral("platinum")).members[2].pokemonId, 445); // 저장소에도
    session.moveSlot(2, 0);                                                          // 되돌리기
    EXPECT_EQ(session.squad().members[0].pokemonId, 445);
    session.clearSlot(1);
    session.clearSlot(2);

    // 게임(버전)을 바꾸면 그 버전의 스쿼드(비어 있음), 돌아오면 다시. 앱 상태도 같이 바뀐다
    session.setVersion(QStringLiteral("heartgold"));
    EXPECT_EQ(session.version(), QStringLiteral("heartgold"));
    EXPECT_EQ(session.versionGroup(), QStringLiteral("heartgold-soulsilver"));
    EXPECT_EQ(state.game(), QStringLiteral("heartgold"));
    EXPECT_EQ(session.squad().filled(), 0);
    session.setPokemon(0, 37); // HG 스쿼드에 식스테일 — SS 스쿼드와는 따로다
    session.setVersion(QStringLiteral("soulsilver"));
    EXPECT_EQ(session.squad().filled(), 0);
    state.setGame(QStringLiteral("heartgold")); // 다른 화면이 앱 상태를 바꿔도 따라간다
    EXPECT_EQ(session.squad().members[0].pokemonId, 37);
    session.setVersion(QStringLiteral("platinum"));
    EXPECT_EQ(session.squad().members[0].pokemonId, 445);
    EXPECT_EQ(store.currentGame(4), QStringLiteral("platinum"));

    // 세대를 바꾸면 그 세대 스쿼드(비어 있음), 돌아오면 다시
    state.setGeneration(3);
    EXPECT_EQ(session.squad().filled(), 0);
    state.setGeneration(4);
    EXPECT_EQ(session.squad().members[0].pokemonId, 445);
}

TEST_F(RepositoryTest, SquadsSplitFromAGroupSquadPerVersion)
{
    QTemporaryDir settings;
    QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, settings.path());
    QTemporaryDir dir;
    Repository repository(s_dbPath);
    SquadStore store(dir.filePath(QStringLiteral("squads.json")));
    // 버전으로 나누기 전: HGSS 묶음 하나에 저장한 스쿼드와 마지막 게임
    Squad old;
    old.members[0].pokemonId = 35;
    store.setSquad(4, QStringLiteral("heartgold-soulsilver"), old);
    store.setCurrentGame(4, QStringLiteral("heartgold-soulsilver"));
    AppState state;
    state.setGeneration(4);
    SquadSession session(&repository, &store, &state);
    // 앱이 게임을 고른 적이 없으면 저장소의 마지막 게임(묶음) → 그 첫 버전. 앱 상태에도 써 둔다
    EXPECT_EQ(session.version(), QStringLiteral("heartgold"));
    EXPECT_EQ(state.game(), QStringLiteral("heartgold"));
    EXPECT_EQ(session.squad().members[0].pokemonId, 35); // 묶음 스쿼드에서 시작
    session.setPokemon(1, 36);                           // 고치면 HG 스쿼드로 갈라진다
    EXPECT_TRUE(store.hasSquad(4, QStringLiteral("heartgold")));
    session.setVersion(QStringLiteral("soulsilver"));
    EXPECT_EQ(session.squad().members[0].pokemonId, 35); // SS도 묶음 스쿼드에서 시작
    EXPECT_EQ(session.squad().members[1].pokemonId, 0);
}

TEST_F(RepositoryTest, MachineLearnersFollowTheGame)
{
    Repository repository(s_dbPath);
    auto speciesOf = [&](const char *item, const char *group) {
        QList<int> ids;
        for (const SpeciesRow &row :
             repository.machineLearners(QLatin1String(item), QLatin1String(group)))
            ids.append(row.speciesId);
        return ids;
    };
    // 시드의 Pt 기술머신26(지진)은 한카리아스만, 기술머신06(맹독)은 다섯 종 모두(종 번호 순)
    EXPECT_EQ(speciesOf("tm26", "platinum"), (QList<int> {445}));
    EXPECT_EQ(speciesOf("tm06", "platinum"), (QList<int> {1, 35, 36, 37, 445}));
    EXPECT_TRUE(speciesOf("tm26", "black-white").isEmpty());    // 시드에 없는 게임
    EXPECT_TRUE(speciesOf("fire-stone", "platinum").isEmpty()); // 기술머신이 아니다
    // 진화 아이템: 진화 전 · 후의 아이콘(기본 모습)
    const QList<ItemEvolution> moon = repository.evolutionsWithItem(81, 4); // 달의돌
    ASSERT_FALSE(moon.isEmpty());
    EXPECT_EQ(moon.first().speciesId, 36);
    EXPECT_EQ(moon.first().fromPokemonId, 35);
    EXPECT_EQ(moon.first().toPokemonId, 36);
}

TEST_F(RepositoryTest, VersionsResolveAndKnowTheirExclusives)
{
    Repository repository(s_dbPath);
    EXPECT_EQ(repository.resolveVersion(4, QStringLiteral("soulsilver")),
              QStringLiteral("soulsilver"));
    EXPECT_EQ(repository.resolveVersion(4, QStringLiteral("heartgold-soulsilver")),
              QStringLiteral("heartgold")); // 옛 저장값(묶음)
    EXPECT_EQ(repository.resolveVersion(4, {}), QStringLiteral("platinum")); // 대표 게임
    EXPECT_EQ(repository.resolveVersion(4, QStringLiteral("black")), QStringLiteral("platinum"));
    // 시드의 식스테일(37)은 SS에만 나온다 → HG에서 본 다른 버전 한정
    EXPECT_EQ(repository.otherVersionSpecies(QStringLiteral("heartgold")), (QSet<int> {37}));
    EXPECT_TRUE(repository.otherVersionSpecies(QStringLiteral("soulsilver")).isEmpty());
    EXPECT_TRUE(repository.otherVersionSpecies(QStringLiteral("platinum")).isEmpty());
}

TEST_F(RepositoryTest, TypeChartChangesAcrossGenerations)
{
    Repository repository(s_dbPath);
    auto at = [&](int generation, const char *attack, const char *defense) {
        return repository.typeChart(generation).at(QLatin1String(attack), QLatin1String(defense));
    };
    // 04 §4 T2-6 · T2-7: 1세대 표 → 2세대 표
    EXPECT_EQ(at(1, "ghost", "psychic"), 0.0); // 1세대 버그: 고스트 → 에스퍼 무효
    EXPECT_EQ(at(2, "ghost", "psychic"), 2.0);
    EXPECT_EQ(at(1, "bug", "poison"), 2.0);
    EXPECT_EQ(at(2, "bug", "poison"), 0.5);
    EXPECT_EQ(at(1, "poison", "bug"), 2.0);
    EXPECT_EQ(at(2, "poison", "bug"), 1.0);
    EXPECT_EQ(at(1, "ice", "fire"), 1.0);
    EXPECT_EQ(at(2, "ice", "fire"), 0.5);
    // T2-5: 강철의 고스트 · 악 반감은 5세대까지
    EXPECT_EQ(at(5, "dark", "steel"), 0.5);
    EXPECT_EQ(at(6, "dark", "steel"), 1.0);
    // 페어리는 6세대(XY)부터: 타입 수 15(1세대) → 17(2–5세대) → 18
    EXPECT_EQ(repository.typeChart(2).types.size(), 17);
    EXPECT_EQ(repository.typeChart(5).types.size(), 17);
    EXPECT_EQ(repository.typeChart(6).types.size(), 18);
    EXPECT_EQ(at(6, "dragon", "fairy"), 0.0);
}

TEST_F(RepositoryTest, DamageClassFollowsTheTypeUntilGeneration3)
{
    Repository repository(s_dbPath);
    // 불꽃펀치(불꽃 · 물리) · 역린(드래곤 · 물리) · 화염방사(불꽃 · 특수)
    auto classes = [&](int generation) {
        QList<int> result;
        for (const MoveEntry &m : repository.moves({7, 200, 53}, generation))
            result.append(m.damageClass);
        return result;
    };
    // 3세대까지: 불꽃 · 드래곤은 특수 타입 → 셋 다 특수
    EXPECT_EQ(classes(3), (QList<int> {3, 3, 3}));
    // 4세대(DP)부터 기술마다
    EXPECT_EQ(classes(4), (QList<int> {2, 2, 3}));
    // 기술 자체의 분류는 세대와 상관없이 남아 있다("4세대 이후 규칙이었다면"에 쓴다)
    EXPECT_EQ(repository.moves({7}, 3).first().ownDamageClass, 2);
}

TEST_F(RepositoryTest, MovesCarryTheirEffectSkeleton)
{
    Repository repository(s_dbPath);
    const QList<MoveEntry> moves = repository.moves({14, 86, 174, 74}, 4);
    ASSERT_EQ(moves.size(), 4);
    // 칼춤: 자신(대상 7) 공격 +2
    EXPECT_EQ(moves[0].identifier, QStringLiteral("swords-dance"));
    EXPECT_EQ(moves[0].target, 7);
    EXPECT_EQ(moves[0].statChanges, (QList<std::pair<int, int>> {{2, 2}}));
    // 전기자석파: 상대 마비
    EXPECT_EQ(moves[1].ailment, 1);
    EXPECT_TRUE(moves[1].statChanges.isEmpty());
    // 저주: PokéAPI에는 고유 효과(분류 13)라 능력치 변화가 없다 — 고스트 / 그 밖 두 경우를 UI
    // 사전(move-effects.json)이 정한다
    EXPECT_TRUE(moves[2].statChanges.isEmpty());
    // 성장: DB는 지금 값(공격 · 특공 +1). 4세대까지 특공만이었던 차이는 UI 사전(move-effects.json)
    EXPECT_EQ(moves[3].statChanges, (QList<std::pair<int, int>> {{2, 1}, {4, 1}}));
    // 그 세대 설명문: 한국어는 XY부터라 4세대는 6세대 문구를 빌린다
    EXPECT_FALSE(moves[0].effect.ko.isEmpty());
}

TEST_F(RepositoryTest, RefusesADatabaseWithAnotherSchemaVersion)
{
    // 스키마가 바뀐 뒤 데이터를 다시 받기 전의 옛 DB: 열지 않는다(옛 표를 읽어 빈 이름 · 잘못된
    // 분류를 보여 주지 않게). 새 DB로 바뀐 뒤 close() → 다음 조회는 새 파일을 연다
    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("old.sqlite"));
    ASSERT_TRUE(QFile::copy(s_dbPath, path));
    QFile::setPermissions(path, QFile::ReadOwner | QFile::WriteOwner);
    {
        QSqlDatabase db
                = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), QStringLiteral("old"));
        db.setDatabaseName(path);
        ASSERT_TRUE(db.open());
        QSqlQuery(db).exec(
                QStringLiteral("UPDATE meta SET value = '1' WHERE key = 'schema_version'"));
        db.close();
    }
    QSqlDatabase::removeDatabase(QStringLiteral("old"));
    Repository repository(path);
    EXPECT_TRUE(repository.speciesForGeneration(4).isEmpty());
    EXPECT_FALSE(repository.errorString().isEmpty());

    ASSERT_TRUE(QFile::remove(path));
    ASSERT_TRUE(QFile::copy(s_dbPath, path)); // 데이터 받기가 새 DB로 바꿔 놓았다
    repository.close();
    EXPECT_EQ(repository.speciesForGeneration(4).size(), 5);
}

TEST_F(RepositoryTest, KnowsWhatEachSpeciesEvolvesFrom)
{
    Repository repository(s_dbPath);
    const QHash<int, int> parents = repository.evolvesFrom();
    EXPECT_EQ(parents.value(36), 35);  // 픽시 ← 삐삐
    EXPECT_FALSE(parents.contains(1)); // 이상해씨는 진화 전이 없다
}
