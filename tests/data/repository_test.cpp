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
        const ItemRow *row
                = findItem(repository.itemsForGeneration(generation), QStringLiteral("tm01"));
        return std::pair(row->machineMove.ko, row->machineType);
    };
    EXPECT_EQ(tm01(1), std::pair(QStringLiteral("메가톤펀치"), QStringLiteral("normal")));
    EXPECT_EQ(tm01(4), std::pair(QStringLiteral("힘껏펀치"), QStringLiteral("fighting")));
    EXPECT_EQ(tm01(5), std::pair(QStringLiteral("손톱갈기"), QStringLiteral("dark")));
    EXPECT_EQ(tm01(7),
              std::pair(QStringLiteral("분발"), QStringLiteral("normal"))); // SM(LGPE 아님)

    // 기술머신이 아닌 아이템은 비어 있고, 가격은 그대로
    const ItemRow *potion = findItem(repository.itemsForGeneration(4), QStringLiteral("potion"));
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
