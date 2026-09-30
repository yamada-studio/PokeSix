#include "data/update/csvimporter.h"

#include "data/db/schema.h"
#include "data/logging/logging.h"
#include "data/update/csvreader.h"
#include "data/update/csvsource.h"
#include "data/update/genranges.h"

#include <QDateTime>
#include <QFile>
#include <QMap>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <array>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace {
using com::yamada::studio::genranges::toGenRanges;

// PokéAPI languages.csv의 id → 우리 스키마의 이름 열 (0 = ko, 1 = en, 2 = ja)
int nameColumnOf(int languageId)
{
    switch (languageId) {
    case 3:
        return 0; // ko
    case 9:
        return 1; // en
    case 11:
        return 2; // ja (한자 · 가나 표기)
    default:
        return -1; // 다른 언어는 저장하지 않는다
    }
}
using Names = std::array<QString, 3>; // ko, en, ja

// 세대에 따라 존재가 달라지는 능력치 — 세대 규칙을 코드 곳곳의 if 대신 표로 둔다(architecture §5).
//   1세대의 "특수"(stat 9)는 2세대부터 특수공격(4) · 특수방어(5)로 나뉘었다.
//   9에는 past 줄만 있으므로 toGenRanges가 1세대 구간만 만든다. 4 · 5는 2세대부터 시작하게 한다.
struct StatFirstGen
{
    int statId;
    int firstGen;
};
constexpr StatFirstGen kStatFirstGen[] = {{4, 2}, {5, 2}};

// PokéAPI의 id가 10000 이상인 타입(unknown, shadow)은 배틀 타입이 아니다. 저장하지 않는다.
constexpr int kFirstNonBattleTypeId = 10000;

// CSV 값 → SQL 값. 빈 칸(null QString)은 SQL NULL.
QVariant intOrNull(const QString &text)
{
    return text.isEmpty() ? QVariant() : QVariant(text.toInt());
}
QVariant textOrNull(const QString &text)
{
    return text.isNull() ? QVariant() : QVariant(text);
}
QVariant genOrNull(const std::optional<int> &gen)
{
    return gen ? QVariant(*gen) : QVariant();
}
} // namespace

namespace com::yamada::studio {
bool CsvImporter::fail(const QString &message)
{
    if (m_error.isEmpty()) // 처음 난 오류가 원인이다. 뒤따르는 오류로 덮어쓰지 않는다
        m_error = message;
    qCWarning(lcData) << "import failed:" << message;
    return false;
}

bool CsvImporter::run(const QString &csvDir, const QString &dbPath)
{
    m_csvDir = csvDir;
    m_error.clear();
    m_typeIntroGen.clear();
    m_speciesIntroGen.clear();
    m_pokemonIntroGen.clear();

    const QString tempPath = dbPath + QStringLiteral(".importing");
    QFile::remove(tempPath); // 지난번에 실패하고 남은 임시 파일

    // 연결 이름은 객체마다 다르게 — 같은 이름으로 두 번 addDatabase하면 앞의 연결을 덮어쓴다.
    const QString connection
            = QStringLiteral("pokesix.import.%1").arg(reinterpret_cast<quintptr>(this), 0, 16);
    bool ok = false;
    {
        // 이 중괄호가 끝나기 전에 연결을 쓰는 모든 객체(QSqlDatabase, QSqlQuery)가 사라져야
        // 아래 removeDatabase가 "still in use" 경고 없이 연결을 정리할 수 있다.
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(tempPath);
        if (!db.open()) {
            fail(QStringLiteral("cannot open %1: %2").arg(tempPath, db.lastError().text()));
        } else {
            {
                // 임시 파일이고 실패하면 통째로 버리므로, 저널(되돌리기 기록)과 디스크 동기화를
                // 끈다 → 빠르다.
                QSqlQuery pragma(db);
                pragma.exec(QStringLiteral("PRAGMA journal_mode = OFF"));
                pragma.exec(QStringLiteral("PRAGMA synchronous = OFF"));
            }
            ok = db.transaction() || fail(QStringLiteral("cannot begin a transaction"));
            ok = ok && createSchema(db) && importGenerations(db) && importTypes(db)
                 && importTypeChart(db) && importStats(db) && importSpecies(db) && importPokemon(db)
                 && importPokemonTypes(db) && importPokemonStats(db) && importRegions(db)
                 && importVersionGroups(db) && importVersions(db) && importPokedexes(db)
                 && writeMeta(db);
            if (ok)
                ok = db.commit()
                     || fail(QStringLiteral("commit failed: %1").arg(db.lastError().text()));
            else
                db.rollback();
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connection);

    if (!ok) {
        QFile::remove(tempPath);
        return false;
    }
    // 완성된 DB로 교체. QFile::rename은 대상이 있으면 실패하므로 먼저 지운다.
    QFile::remove(dbPath);
    if (!QFile::rename(tempPath, dbPath))
        return fail(QStringLiteral("cannot move %1 to %2").arg(tempPath, dbPath));
    qCInfo(lcData) << "imported PokéAPI CSV" << m_csvDir << "into" << dbPath;
    return true;
}

bool CsvImporter::forEachRecord(const QString &name, const QStringList &columns,
                                const std::function<bool(const QStringList &values)> &row)
{
    QFile file(m_csvDir + QLatin1Char('/') + name + QStringLiteral(".csv"));
    if (!file.open(QIODevice::ReadOnly))
        return fail(QStringLiteral("cannot open %1").arg(file.fileName()));

    CsvReader reader(&file);
    if (!reader.readHeader())
        return fail(QStringLiteral("%1: no header").arg(name));

    // 열은 번호가 아니라 이름으로 찾는다. PokéAPI가 열을 추가 · 재배치해도 깨지지 않는다.
    std::vector<int> indexes;
    for (const QString &column : columns) {
        const int index = reader.column(column);
        if (index < 0)
            return fail(QStringLiteral("%1: missing column '%2'").arg(name, column));
        indexes.push_back(index);
    }

    QStringList fields;
    QStringList values;
    while (reader.readRecord(fields)) {
        values.clear();
        for (const int index : indexes)
            values.append(fields.value(index)); // 짧은 줄이면 null QString → NULL
        if (!row(values))
            return false;
    }
    if (reader.hasError())
        return fail(QStringLiteral("%1: %2").arg(name, reader.errorString()));
    return true;
}

namespace {
// 준비(prepare)는 한 번, 실행(exec)은 줄마다. 값은 바인딩한다 — SQL에 문자열을 이어 붙이지 않는다.
class Insert
{
public:
    Insert(QSqlDatabase &db, const QString &sql)
        : m_query(db)
    {
        m_ok = m_query.prepare(sql);
    }
    bool isValid() const { return m_ok; }
    bool exec(const QVariantList &values)
    {
        for (qsizetype i = 0; i < values.size(); ++i)
            m_query.bindValue(static_cast<int>(i), values.at(i));
        return m_query.exec();
    }
    QString error() const { return m_query.lastError().text(); }

private:
    QSqlQuery m_query;
    bool m_ok = false;
};
} // namespace

bool CsvImporter::createSchema(QSqlDatabase &db)
{
    QSqlQuery query(db);
    for (const char *statement : schema::kStatements) {
        if (!query.exec(QString::fromUtf8(statement)))
            return fail(QStringLiteral("schema: %1").arg(query.lastError().text()));
    }
    return true;
}

bool CsvImporter::importGenerations(QSqlDatabase &db)
{
    Insert insert(db, QStringLiteral("INSERT INTO generations (id, identifier) VALUES (?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("generations"),
                         {QStringLiteral("id"), QStringLiteral("identifier")},
                         [&](const QStringList &v) {
                             return insert.exec({v[0].toInt(), v[1]}) || fail(insert.error());
                         });
}

bool CsvImporter::importTypes(QSqlDatabase &db)
{
    QHash<int, Names> names;
    const bool namesOk
            = forEachRecord(QStringLiteral("type_names"),
                            {QStringLiteral("type_id"), QStringLiteral("local_language_id"),
                             QStringLiteral("name")},
                            [&](const QStringList &v) {
                                const int column = nameColumnOf(v[1].toInt());
                                if (column >= 0)
                                    names[v[0].toInt()][column] = v[2];
                                return true;
                            });
    if (!namesOk)
        return false;

    Insert insert(
            db, QStringLiteral(
                        "INSERT INTO types (id, identifier, intro_gen, name_ko, name_en, name_ja) "
                        "VALUES (?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(
            QStringLiteral("types"),
            {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("generation_id")},
            [&](const QStringList &v) {
                const int id = v[0].toInt();
                if (id >= kFirstNonBattleTypeId)
                    return true;
                const int intro = v[2].toInt();
                m_typeIntroGen.insert(id, intro);
                const Names &n = names.value(id);
                return insert.exec({id, v[1], intro, textOrNull(n[0]), textOrNull(n[1]),
                                    textOrNull(n[2])})
                       || fail(insert.error());
            });
}

int CsvImporter::typeIntro(int typeId) const
{
    return m_typeIntroGen.value(typeId, 1);
}

bool CsvImporter::importTypeChart(QSqlDatabase &db)
{
    using Pair = std::pair<int, int>;                      // (공격 타입, 방어 타입)
    std::map<Pair, int> current;                           // 지금의 배율(%)
    std::map<Pair, std::vector<std::pair<int, int>>> past; // (그 세대까지, 배율%)

    if (!forEachRecord(QStringLiteral("type_efficacy"),
                       {QStringLiteral("damage_type_id"), QStringLiteral("target_type_id"),
                        QStringLiteral("damage_factor")},
                       [&](const QStringList &v) {
                           current[{v[0].toInt(), v[1].toInt()}] = v[2].toInt();
                           return true;
                       }))
        return false;
    if (!forEachRecord(QStringLiteral("type_efficacy_past"),
                       {QStringLiteral("damage_type_id"), QStringLiteral("target_type_id"),
                        QStringLiteral("damage_factor"), QStringLiteral("generation_id")},
                       [&](const QStringList &v) {
                           past[{v[0].toInt(), v[1].toInt()}].push_back(
                                   {v[3].toInt(), v[2].toInt()});
                           return true;
                       }))
        return false;

    Insert insert(
            db, QStringLiteral(
                        "INSERT INTO type_chart (atk_type, def_type, multiplier, gen_from, gen_to) "
                        "VALUES (?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (const auto &[pair, factor] : current) {
        // 두 타입이 모두 존재하는 세대부터: 1세대에는 강철 · 악이 없으므로 그 줄은 2세대부터
        // 시작한다.
        const int firstGen = std::max(typeIntro(pair.first), typeIntro(pair.second));
        const auto pastIt = past.find(pair);
        const auto ranges = toGenRanges<int>(
                firstGen,
                pastIt != past.end() ? pastIt->second : std::vector<std::pair<int, int>> {},
                factor);
        for (const auto &range : ranges) {
            if (!insert.exec({pair.first, pair.second, range.value / 100.0, range.from,
                              genOrNull(range.to)}))
                return fail(insert.error());
        }
    }
    return true;
}

bool CsvImporter::importStats(QSqlDatabase &db)
{
    Insert insert(db, QStringLiteral("INSERT INTO stats (id, identifier) VALUES (?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("stats"),
                         {QStringLiteral("id"), QStringLiteral("identifier")},
                         [&](const QStringList &v) {
                             return insert.exec({v[0].toInt(), v[1]}) || fail(insert.error());
                         });
}

bool CsvImporter::importSpecies(QSqlDatabase &db)
{
    QHash<int, Names> names;
    QHash<int, Names> genera;
    if (!forEachRecord(QStringLiteral("pokemon_species_names"),
                       {QStringLiteral("pokemon_species_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name"), QStringLiteral("genus")},
                       [&](const QStringList &v) {
                           const int column = nameColumnOf(v[1].toInt());
                           if (column >= 0) {
                               names[v[0].toInt()][column] = v[2];
                               genera[v[0].toInt()][column] = v[3];
                           }
                           return true;
                       }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO species (id, identifier, intro_gen, "
                                     "evolves_from, evolution_chain, "
                                     "is_baby, is_legendary, is_mythical, sort_order, "
                                     "name_ko, name_en, name_ja, genus_ko, genus_en, genus_ja) "
                                     "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(
            QStringLiteral("pokemon_species"),
            {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("generation_id"),
             QStringLiteral("evolves_from_species_id"), QStringLiteral("evolution_chain_id"),
             QStringLiteral("is_baby"), QStringLiteral("is_legendary"),
             QStringLiteral("is_mythical"), QStringLiteral("order")},
            [&](const QStringList &v) {
                const int id = v[0].toInt();
                const int intro = v[2].toInt();
                m_speciesIntroGen.insert(id, intro);
                const Names &n = names.value(id);
                const Names &g = genera.value(id);
                return insert.exec({id, v[1], intro, intOrNull(v[3]), intOrNull(v[4]), v[5].toInt(),
                                    v[6].toInt(), v[7].toInt(), intOrNull(v[8]), textOrNull(n[0]),
                                    textOrNull(n[1]), textOrNull(n[2]), textOrNull(g[0]),
                                    textOrNull(g[1]), textOrNull(g[2])})
                       || fail(insert.error());
            });
}

bool CsvImporter::importPokemon(QSqlDatabase &db)
{
    // 폼이 처음 나온 세대: pokemon_forms.introduced_in_version_group_id →
    // version_groups.generation_id. 메가진화 · 리전폼(id 10001~)은 종보다 늦게 나왔다(알로라
    // 식스테일 = 7세대, 식스테일 = 1세대).
    QHash<int, int> versionGroupGen;
    if (!forEachRecord(QStringLiteral("version_groups"),
                       {QStringLiteral("id"), QStringLiteral("generation_id")},
                       [&](const QStringList &v) {
                           versionGroupGen.insert(v[0].toInt(), v[1].toInt());
                           return true;
                       }))
        return false;
    QHash<int, int> formIntroGen; // pokemon_id → 그 포켓몬의 폼 중 가장 이른 세대
    if (!forEachRecord(
                QStringLiteral("pokemon_forms"),
                {QStringLiteral("pokemon_id"), QStringLiteral("introduced_in_version_group_id")},
                [&](const QStringList &v) {
                    if (v[1].isEmpty())
                        return true;
                    const int pokemonId = v[0].toInt();
                    const int gen = versionGroupGen.value(v[1].toInt(), 0);
                    if (gen > 0
                        && (!formIntroGen.contains(pokemonId) || gen < formIntroGen[pokemonId]))
                        formIntroGen[pokemonId] = gen;
                    return true;
                }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO pokemon (id, identifier, species_id, is_default, "
                                     "intro_gen, height, "
                                     "weight, sort_order) VALUES (?, ?, ?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(
            QStringLiteral("pokemon"),
            {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("species_id"),
             QStringLiteral("is_default"), QStringLiteral("height"), QStringLiteral("weight"),
             QStringLiteral("order")},
            [&](const QStringList &v) {
                const int id = v[0].toInt();
                const int speciesId = v[2].toInt();
                // 폼 정보가 없으면 종이 처음 나온 세대. 둘 다 있으면 늦은 쪽(폼이 종보다 먼저일
                // 수는 없다).
                const int speciesGen = m_speciesIntroGen.value(speciesId, 1);
                const int intro = std::max(speciesGen, formIntroGen.value(id, speciesGen));
                m_pokemonIntroGen.insert(id, intro);
                return insert.exec({id, v[1], speciesId, v[3].toInt(), intro, intOrNull(v[4]),
                                    intOrNull(v[5]), intOrNull(v[6])})
                       || fail(insert.error());
            });
}

int CsvImporter::pokemonIntro(int pokemonId) const
{
    return m_pokemonIntroGen.value(pokemonId, 1);
}

bool CsvImporter::importPokemonTypes(QSqlDatabase &db)
{
    // 값은 "슬롯 1 · 2의 묶음"이다: [(1, 노말)] 또는 [(1, 드래곤), (2, 땅)]. 묶음 전체를 비교해
    // 구간을 나눈다.
    using Slots = std::vector<std::pair<int, int>>; // (slot, type_id), slot 순으로 정렬
    std::map<int, Slots> current;
    std::map<int, std::map<int, Slots>> pastByGen; // pokemon → (그 세대까지 → 묶음)

    if (!forEachRecord(
                QStringLiteral("pokemon_types"),
                {QStringLiteral("pokemon_id"), QStringLiteral("type_id"), QStringLiteral("slot")},
                [&](const QStringList &v) {
                    current[v[0].toInt()].push_back({v[2].toInt(), v[1].toInt()});
                    return true;
                }))
        return false;
    if (!forEachRecord(QStringLiteral("pokemon_types_past"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("generation_id"),
                        QStringLiteral("type_id"), QStringLiteral("slot")},
                       [&](const QStringList &v) {
                           pastByGen[v[0].toInt()][v[1].toInt()].push_back(
                                   {v[3].toInt(), v[2].toInt()});
                           return true;
                       }))
        return false;

    Insert insert(db,
                  QStringLiteral(
                          "INSERT INTO pokemon_types (pokemon_id, slot, type_id, gen_from, gen_to) "
                          "VALUES (?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (auto &[pokemonId, typeSlots] : current) {
        // 주의: 변수 이름을 slots로 지으면 안 된다. Qt가 `public slots:`용 빈 매크로로 정의해 둔
        // 이름이다.
        std::sort(typeSlots.begin(), typeSlots.end());
        std::vector<std::pair<int, Slots>> past;
        if (const auto it = pastByGen.find(pokemonId); it != pastByGen.end()) {
            for (auto [gen, pastSlots] : it->second) {
                std::sort(pastSlots.begin(), pastSlots.end());
                past.push_back({gen, pastSlots});
            }
        }
        for (const auto &range : toGenRanges<Slots>(pokemonIntro(pokemonId), past, typeSlots)) {
            for (const auto &[slot, typeId] : range.value) {
                if (!insert.exec({pokemonId, slot, typeId, range.from, genOrNull(range.to)}))
                    return fail(insert.error());
            }
        }
    }
    return true;
}

bool CsvImporter::importPokemonStats(QSqlDatabase &db)
{
    using Key = std::pair<int, int>; // (pokemon, stat)
    std::map<Key, int> current;
    std::map<Key, std::vector<std::pair<int, int>>> past; // (그 세대까지, 종족값)

    if (!forEachRecord(QStringLiteral("pokemon_stats"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("stat_id"),
                        QStringLiteral("base_stat")},
                       [&](const QStringList &v) {
                           current[{v[0].toInt(), v[1].toInt()}] = v[2].toInt();
                           return true;
                       }))
        return false;
    if (!forEachRecord(QStringLiteral("pokemon_stats_past"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("generation_id"),
                        QStringLiteral("stat_id"), QStringLiteral("base_stat")},
                       [&](const QStringList &v) {
                           past[{v[0].toInt(), v[2].toInt()}].push_back(
                                   {v[1].toInt(), v[3].toInt()});
                           return true;
                       }))
        return false;

    // 현재 값이 없고 옛 값만 있는 능력치(1세대의 "특수")도 넣어야 하므로 두 표의 키를 합친다.
    std::map<Key, bool> keys;
    for (const auto &entry : current)
        keys[entry.first] = true;
    for (const auto &entry : past)
        keys[entry.first] = true;

    Insert insert(
            db, QStringLiteral(
                        "INSERT INTO pokemon_stats (pokemon_id, stat_id, value, gen_from, gen_to) "
                        "VALUES (?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (const auto &[key, unused] : keys) {
        Q_UNUSED(unused);
        const auto [pokemonId, statId] = key;
        int firstGen = pokemonIntro(pokemonId);
        for (const StatFirstGen &rule : kStatFirstGen) {
            if (rule.statId == statId)
                firstGen = std::max(firstGen, rule.firstGen);
        }
        const auto pastIt = past.find(key);
        const auto currentIt = current.find(key);
        const auto ranges = toGenRanges<int>(
                firstGen,
                pastIt != past.end() ? pastIt->second : std::vector<std::pair<int, int>> {},
                currentIt != current.end() ? std::optional<int>(currentIt->second) : std::nullopt);
        for (const auto &range : ranges) {
            if (!insert.exec({pokemonId, statId, range.value, range.from, genOrNull(range.to)}))
                return fail(insert.error());
        }
    }
    return true;
}

// ── 도감 · 게임 (D2b) ─────────────────────────────────────────────────────────────────────
// 세대 → 게임 묶음(version_groups) → 도감(pokedex_version_groups) → 도감 번호(dex_numbers).
// 버튼에 쓰는 이름은 지방(regions) · 버전(versions) 이름이다. 도감 자체의 한국어 이름은 PokéAPI에
// 없다.

bool CsvImporter::importRegions(QSqlDatabase &db)
{
    // region_names만 받는다(regions.csv에는 identifier뿐이라 쓸 곳이 없다). 이름이 하나라도 있는
    // 지방 id가 곧 지방 목록이다.
    QMap<int, Names> names; // id 순서대로 넣으려고 QMap
    if (!forEachRecord(QStringLiteral("region_names"),
                       {QStringLiteral("region_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           const int column = nameColumnOf(v[1].toInt());
                           if (column >= 0)
                               names[v[0].toInt()][column] = v[2];
                           return true;
                       }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO regions (id, name_ko, name_en, name_ja) "
                                     "VALUES (?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        const Names &n = it.value();
        if (!insert.exec({it.key(), textOrNull(n[0]), textOrNull(n[1]), textOrNull(n[2])}))
            return fail(insert.error());
    }
    return true;
}

bool CsvImporter::importVersionGroups(QSqlDatabase &db)
{
    // CSV의 order 열은 SQL 예약어라서 sort_order로 저장한다.
    Insert insert(db, QStringLiteral("INSERT INTO version_groups (id, identifier, generation, "
                                     "sort_order) VALUES (?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("version_groups"),
                         {QStringLiteral("id"), QStringLiteral("identifier"),
                          QStringLiteral("generation_id"), QStringLiteral("order")},
                         [&](const QStringList &v) {
                             return insert.exec({v[0].toInt(), v[1], v[2].toInt(), v[3].toInt()})
                                    || fail(insert.error());
                         });
}

bool CsvImporter::importVersions(QSqlDatabase &db)
{
    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("version_names"),
                       {QStringLiteral("version_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           const int column = nameColumnOf(v[1].toInt());
                           if (column >= 0)
                               names[v[0].toInt()][column] = v[2];
                           return true;
                       }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO versions (id, version_group_id, identifier, "
                                     "name_ko, name_en, name_ja) VALUES (?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("versions"),
                         {QStringLiteral("id"), QStringLiteral("version_group_id"),
                          QStringLiteral("identifier")},
                         [&](const QStringList &v) {
                             const int id = v[0].toInt();
                             const Names &n = names.value(id);
                             return insert.exec({id, v[1].toInt(), v[2], textOrNull(n[0]),
                                                 textOrNull(n[1]), textOrNull(n[2])})
                                    || fail(insert.error());
                         });
}

bool CsvImporter::importPokedexes(QSqlDatabase &db)
{
    // 1) 도감 목록. 전국 · conquest처럼 지방이 없는 도감은 region_id가 빈 칸 → NULL
    Insert dexes(db, QStringLiteral("INSERT INTO pokedexes (id, identifier, region_id, "
                                    "is_main_series) VALUES (?, ?, ?, ?)"));
    if (!dexes.isValid())
        return fail(dexes.error());
    if (!forEachRecord(QStringLiteral("pokedexes"),
                       {QStringLiteral("id"), QStringLiteral("identifier"),
                        QStringLiteral("region_id"), QStringLiteral("is_main_series")},
                       [&](const QStringList &v) {
                           return dexes.exec({v[0].toInt(), v[1], intOrNull(v[2]), v[3].toInt()})
                                  || fail(dexes.error());
                       }))
        return false;

    // 2) 도감 ↔ 게임 묶음 (다대다: 관동도감 = 레드 · 블루 · 피카츄 …, 신오도감 = DP · BDSP)
    Insert links(db, QStringLiteral("INSERT INTO pokedex_version_groups (pokedex_id, "
                                    "version_group_id) VALUES (?, ?)"));
    if (!links.isValid())
        return fail(links.error());
    if (!forEachRecord(QStringLiteral("pokedex_version_groups"),
                       {QStringLiteral("pokedex_id"), QStringLiteral("version_group_id")},
                       [&](const QStringList &v) {
                           return links.exec({v[0].toInt(), v[1].toInt()}) || fail(links.error());
                       }))
        return false;

    // 3) 도감별 번호 (8천 줄 — run()의 트랜잭션 안이라 한 번에 커밋된다)
    Insert numbers(db, QStringLiteral("INSERT INTO dex_numbers (pokedex_id, species_id, number) "
                                      "VALUES (?, ?, ?)"));
    if (!numbers.isValid())
        return fail(numbers.error());
    return forEachRecord(QStringLiteral("pokemon_dex_numbers"),
                         {QStringLiteral("pokedex_id"), QStringLiteral("species_id"),
                          QStringLiteral("pokedex_number")},
                         [&](const QStringList &v) {
                             return numbers.exec({v[0].toInt(), v[1].toInt(), v[2].toInt()})
                                    || fail(numbers.error());
                         });
}

bool CsvImporter::writeMeta(QSqlDatabase &db)
{
    Insert insert(db, QStringLiteral("INSERT INTO meta (key, value) VALUES (?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    const std::pair<QString, QString> rows[] = {
            {QStringLiteral("schema_version"), QString::number(schema::kVersion)},
            {QStringLiteral("source_commit"), QLatin1StringView(csvsource::kCommit)},
            {QStringLiteral("imported_at"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
    };
    for (const auto &[key, value] : rows) {
        if (!insert.exec({key, value}))
            return fail(insert.error());
    }
    return true;
}
} // namespace com::yamada::studio
