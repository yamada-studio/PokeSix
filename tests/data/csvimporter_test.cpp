#include "data/db/gamedatabase.h"
#include "data/update/csvimporter.h"
#include "data/update/csvsource.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QVariant>

#include <gtest/gtest.h>

using com::yamada::studio::CsvImporter;

namespace {
// 시드 CSV(tests/fixtures/pokeapi-csv)를 임시 폴더의 DB로 변환하고, 그 DB에 질의한다.
class CsvImporterTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        s_dir = new QTemporaryDir;
        s_dbPath = s_dir->filePath(QStringLiteral("pokesix.sqlite"));
        CsvImporter importer;
        s_imported = importer.run(QStringLiteral(POKESIX_FIXTURES_DIR "/pokeapi-csv"), s_dbPath);
        s_error = importer.errorString();
    }
    static void TearDownTestSuite()
    {
        delete s_dir;
        s_dir = nullptr;
    }

    void SetUp() override
    {
        ASSERT_TRUE(s_imported) << s_error.toStdString();
        m_connection = QStringLiteral("test.%1").arg(
                ::testing::UnitTest::GetInstance()->current_test_info()->name());
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
        db.setDatabaseName(s_dbPath);
        ASSERT_TRUE(db.open());
    }
    void TearDown() override
    {
        QSqlDatabase::database(m_connection).close();
        QSqlDatabase::removeDatabase(m_connection);
    }

    // 결과가 한 줄 · 한 칸인 질의. 값이 없으면 invalid QVariant.
    QVariant scalar(const QString &sql, const QVariantList &values = {})
    {
        QSqlQuery query(QSqlDatabase::database(m_connection));
        query.prepare(sql);
        for (int i = 0; i < values.size(); ++i)
            query.bindValue(i, values.at(i));
        if (!query.exec() || !query.next())
            return {};
        return query.value(0);
    }

    // 세대 gen의 상성 배율
    QVariant multiplier(int atk, int def, int gen)
    {
        return scalar(
                QStringLiteral(
                        "SELECT multiplier FROM type_chart WHERE atk_type = ? AND def_type = ? "
                        "AND gen_from <= ? AND (gen_to IS NULL OR gen_to >= ?)"),
                {atk, def, gen, gen});
    }
    // 세대 gen의 타입(슬롯 순, 쉼표로 이어서)
    QString types(int pokemon, int gen)
    {
        QSqlQuery query(QSqlDatabase::database(m_connection));
        query.prepare(QStringLiteral(
                "SELECT t.identifier FROM pokemon_types pt JOIN types t ON t.id = pt.type_id "
                "WHERE pt.pokemon_id = ? AND pt.gen_from <= ? "
                "AND (pt.gen_to IS NULL OR pt.gen_to >= ?) ORDER BY pt.slot"));
        query.bindValue(0, pokemon);
        query.bindValue(1, gen);
        query.bindValue(2, gen);
        query.exec();
        QStringList names;
        while (query.next())
            names << query.value(0).toString();
        return names.join(QLatin1Char(','));
    }
    QVariant stat(int pokemon, int statId, int gen)
    {
        return scalar(
                QStringLiteral(
                        "SELECT value FROM pokemon_stats WHERE pokemon_id = ? AND stat_id = ? "
                        "AND gen_from <= ? AND (gen_to IS NULL OR gen_to >= ?)"),
                {pokemon, statId, gen, gen});
    }

    static inline QTemporaryDir *s_dir = nullptr;
    static inline QString s_dbPath;
    static inline bool s_imported = false;
    static inline QString s_error;
    QString m_connection;
};

enum Type { Normal = 1, Ghost = 8, Steel = 9, Psychic = 14, Dark = 17 };
enum Stat { SpecialAttack = 4, Special = 9 };
} // namespace

TEST_F(CsvImporterTest, GhostVersusPsychicWasZeroOnlyInGen1)
{
    EXPECT_DOUBLE_EQ(multiplier(Ghost, Psychic, 1).toDouble(), 0.0);
    EXPECT_DOUBLE_EQ(multiplier(Ghost, Psychic, 2).toDouble(), 2.0);
    EXPECT_DOUBLE_EQ(multiplier(Ghost, Psychic, 9).toDouble(), 2.0);
}

TEST_F(CsvImporterTest, SteelResistedGhostAndDarkUntilGen5)
{
    EXPECT_DOUBLE_EQ(multiplier(Ghost, Steel, 5).toDouble(), 0.5);
    EXPECT_DOUBLE_EQ(multiplier(Ghost, Steel, 6).toDouble(), 1.0);
    EXPECT_DOUBLE_EQ(multiplier(Dark, Steel, 5).toDouble(), 0.5);
    EXPECT_DOUBLE_EQ(multiplier(Dark, Steel, 6).toDouble(), 1.0);
}

TEST_F(CsvImporterTest, NoChartRowsBeforeATypeExists)
{
    EXPECT_FALSE(multiplier(Ghost, Steel, 1).isValid()); // 1세대에는 강철이 없다
    EXPECT_TRUE(multiplier(Ghost, Steel, 2).isValid());
}

TEST_F(CsvImporterTest, ClefairyWasNormalUntilGen5)
{
    EXPECT_EQ(types(35, 5), QStringLiteral("normal"));
    EXPECT_EQ(types(35, 6), QStringLiteral("fairy"));
    EXPECT_EQ(types(445, 4), QStringLiteral("dragon,ground")); // 한카리아스: 슬롯 순서 유지
}

TEST_F(CsvImporterTest, AlolanFormsStartInGen7)
{
    EXPECT_EQ(scalar(QStringLiteral("SELECT intro_gen FROM pokemon WHERE id = 37")).toInt(), 1);
    EXPECT_EQ(scalar(QStringLiteral("SELECT intro_gen FROM pokemon WHERE id = 10103")).toInt(), 7);
    EXPECT_EQ(types(10103, 6), QString()); // 6세대에는 없던 모습
    EXPECT_EQ(types(10103, 7), QStringLiteral("ice"));
}

TEST_F(CsvImporterTest, SpecialStatExistsOnlyInGen1)
{
    EXPECT_EQ(stat(1, Special, 1).toInt(), 65);
    EXPECT_FALSE(stat(1, Special, 2).isValid());
    EXPECT_FALSE(stat(1, SpecialAttack, 1).isValid()); // 특수공격은 2세대부터
    EXPECT_EQ(stat(1, SpecialAttack, 2).toInt(), 65);
}

TEST_F(CsvImporterTest, StoresKoreanEnglishJapaneseNames)
{
    EXPECT_EQ(scalar(QStringLiteral("SELECT name_ko FROM species WHERE id = 35")).toString(),
              QStringLiteral("삐삐"));
    EXPECT_EQ(scalar(QStringLiteral("SELECT name_en FROM species WHERE id = 35")).toString(),
              QStringLiteral("Clefairy"));
    EXPECT_EQ(scalar(QStringLiteral("SELECT name_ko FROM types WHERE id = 18")).toString(),
              QStringLiteral("페어리"));
}

TEST_F(CsvImporterTest, EmptyCsvFieldBecomesSqlNull)
{
    // 이상해씨는 진화 전이 없다 → evolves_from은 빈 칸 → NULL (빈 문자열이 아니다)
    EXPECT_EQ(scalar(QStringLiteral(
                             "SELECT COUNT(*) FROM species WHERE id = 1 AND evolves_from IS NULL"))
                      .toInt(),
              1);
    EXPECT_EQ(scalar(QStringLiteral("SELECT evolves_from FROM species WHERE id = 36")).toInt(), 35);
}

TEST_F(CsvImporterTest, RecordsTheSourceCommit)
{
    EXPECT_EQ(
            scalar(QStringLiteral("SELECT value FROM meta WHERE key = 'source_commit'")).toString(),
            QLatin1StringView(com::yamada::studio::csvsource::kCommit));
}

TEST(CsvImporterFailure, MissingFolderFailsAndLeavesNoDatabase)
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("pokesix.sqlite"));
    CsvImporter importer;
    EXPECT_FALSE(importer.run(dir.filePath(QStringLiteral("no-such-folder")), dbPath));
    EXPECT_FALSE(importer.errorString().isEmpty());
    EXPECT_FALSE(QFile::exists(dbPath));
    EXPECT_FALSE(QFile::exists(dbPath + QStringLiteral(".importing")));
}

// ── 바꿔 넣지 못한 임시 DB(gamedatabase::pendingPath) ────────────────────────────────────────
// Windows에서 다른 PokeSix 창이 옛 DB를 열어 두면 변환이 끝나도 파일을 바꿔 넣지 못한다. 그때 남긴
// 임시 DB를 다음 실행(Application) · 다시 시도(CsvImporter::run)가 넣어 준다.
namespace {
using namespace com::yamada::studio;

QString fixturesDir()
{
    return QStringLiteral(POKESIX_FIXTURES_DIR "/pokeapi-csv");
}
} // namespace

TEST(PendingImport, IsAdoptedWhenItMatchesThisBuild)
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("pokesix.sqlite"));
    CsvImporter importer;
    ASSERT_TRUE(importer.run(fixturesDir(), dbPath)) << importer.errorString().toStdString();
    // 변환은 끝났는데 자리에 못 넣은 상황을 흉내 낸다: 완성본을 .importing으로 옮기고 옛 파일은
    // 비운다
    const QString pending = gamedatabase::pendingPath(dbPath);
    ASSERT_TRUE(QFile::rename(dbPath, pending));
    {
        QFile old(dbPath);
        ASSERT_TRUE(old.open(QIODevice::WriteOnly));
        old.write("not a database");
    }
    EXPECT_FALSE(gamedatabase::isUsable(dbPath));
    EXPECT_TRUE(gamedatabase::matchesThisBuild(pending));

    EXPECT_TRUE(gamedatabase::adoptPendingImport(dbPath));
    EXPECT_FALSE(QFile::exists(pending));
    EXPECT_TRUE(gamedatabase::isUsable(dbPath));
    EXPECT_FALSE(gamedatabase::adoptPendingImport(dbPath)); // 두 번째는 넣을 것이 없다
}

TEST(PendingImport, StaleOrHalfWrittenTempIsDiscarded)
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("pokesix.sqlite"));
    const QString pending = gamedatabase::pendingPath(dbPath);
    {
        QFile half(pending); // 변환 도중 죽어서 meta 표가 없는 파일
        ASSERT_TRUE(half.open(QIODevice::WriteOnly));
        half.write("half");
    }
    EXPECT_FALSE(gamedatabase::matchesThisBuild(pending));
    EXPECT_FALSE(gamedatabase::adoptPendingImport(dbPath));
    EXPECT_FALSE(QFile::exists(pending)); // 지웠다
    EXPECT_FALSE(QFile::exists(dbPath));  // 옛 파일을 건드리지 않았다
}

TEST(PendingImport, RunAdoptsInsteadOfReimporting)
{
    QTemporaryDir dir;
    const QString dbPath = dir.filePath(QStringLiteral("pokesix.sqlite"));
    CsvImporter first;
    ASSERT_TRUE(first.run(fixturesDir(), dbPath)) << first.errorString().toStdString();
    const QString pending = gamedatabase::pendingPath(dbPath);
    ASSERT_TRUE(QFile::rename(dbPath, pending));

    // 다시 시도: CSV 폴더가 엉터리여도 완성된 임시 DB가 있으면 그것을 넣고 성공한다
    CsvImporter retry;
    EXPECT_TRUE(retry.run(dir.filePath(QStringLiteral("no-such-csv")), dbPath))
            << retry.errorString().toStdString();
    EXPECT_FALSE(retry.pendingSwap());
    EXPECT_TRUE(gamedatabase::isUsable(dbPath));
    EXPECT_FALSE(QFile::exists(pending));
}
