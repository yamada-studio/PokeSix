#include "data/update/csvimporter.h"

#include "data/db/schema.h"
#include "data/logging/logging.h"
#include "data/update/csvreader.h"
#include "data/update/csvsource.h"
#include "data/update/genranges.h"
#include "data/update/namesupplement.h"

#include <QDateTime>
#include <QFile>
#include <QMap>
#include <QPair>
#include <QSet>
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
namespace namesupplement = com::yamada::studio::namesupplement;

// PokéAPI languages.csv의 id → 우리 스키마의 이름 열 (0 = ko, 1 = en, 2 = ja)

constexpr int kJapaneseKana = 1; // ja-Hrkt: 가나만 쓴 표기. 한자 표기(11)가 없을 때만 쓴다

int nameColumnOf(int languageId)
{
    switch (languageId) {
    case 3:
        return 0; // ko
    case 9:
        return 1; // en
    case 11:
    case kJapaneseKana:
        return 2; // ja — 한자 · 가나 표기(11). 옛 아이템 일부는 가나 표기(1)만 있다
    default:
        return -1; // 다른 언어는 저장하지 않는다
    }
}
using Names = std::array<QString, 3>; // ko, en, ja

// PokéAPI에 없는 아이템 한국어 이름(namesupplement.h). 없으면 null QString → NULL.
QString supplementNameKo(const QString &identifier)
{
    for (const auto &entry : namesupplement::kItemNamesKo)
        if (identifier == QLatin1StringView(entry.identifier.data(), entry.identifier.size()))
            return QString::fromUtf8(entry.nameKo.data(), qsizetype(entry.nameKo.size()));
    return {};
}

constexpr int kJapaneseColumn = 2;

// 일본어 옛 표기는 숫자 · 영문을 전각으로 쓴다("ひでんマシン０１", "１ごうしつのカギ"). 한 줄
// 목록에서는 글자 사이가 벌어져 보이므로 NFKC 정규화로 반각으로 바꾼다(가나 · 한자는 그대로).
QString toHalfWidth(const QString &text)
{
    return text.normalized(QString::NormalizationForm_KC);
}

// 이름 한 줄을 알맞은 열에 넣는다. 가나 표기(1)는 한자 표기(11)를 덮지 않는다(줄 순서와 상관없이).
void setName(Names &names, int languageId, const QString &text)
{
    const int column = nameColumnOf(languageId);
    if (column < 0 || (languageId == kJapaneseKana && !names[column].isEmpty()))
        return;
    names[column] = column == kJapaneseColumn ? toHalfWidth(text) : text;
}

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
constexpr int kFirstAbilityGeneration = 3; // 특성은 3세대(RS)부터

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
                 && importItems(db) && importItemEffects(db) && importMoves(db)
                 && importMoveEffects(db) && importMoveMeta(db) && importMachines(db)
                 && importPokemonMoves(db) && importEncounters(db) && importEvolutions(db)
                 && importAbilities(db) && importNatures(db) && writeMeta(db);
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
                                setName(names[v[0].toInt()], v[1].toInt(), v[2]);
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
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           setName(genera[v[0].toInt()], v[1].toInt(), v[3]);
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
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
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
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
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

// ── 아이템 (E3) ────────────────────────────────────────────────────────────────────────────

bool CsvImporter::importItems(QSqlDatabase &db)
{
    // 1) 분류 + 주머니 이름(주머니 표는 id → identifier뿐이라 분류 줄에 합쳐 둔다)
    QHash<int, QString> pockets;
    if (!forEachRecord(QStringLiteral("item_pockets"),
                       {QStringLiteral("id"), QStringLiteral("identifier")},
                       [&](const QStringList &v) {
                           pockets.insert(v[0].toInt(), v[1]);
                           return true;
                       }))
        return false;
    Insert categories(db, QStringLiteral("INSERT INTO item_categories (id, identifier, pocket) "
                                         "VALUES (?, ?, ?)"));
    if (!categories.isValid())
        return fail(categories.error());
    if (!forEachRecord(
                QStringLiteral("item_categories"),
                {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("pocket_id")},
                [&](const QStringList &v) {
                    return categories.exec({v[0].toInt(), v[1], pockets.value(v[2].toInt())})
                           || fail(categories.error());
                }))
        return false;

    // 2) 아이템 + 이름(ko/en/ja)
    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("item_names"),
                       {QStringLiteral("item_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           return true;
                       }))
        return false;
    Insert items(db,
                 QStringLiteral("INSERT INTO items (id, identifier, category_id, cost, name_ko, "
                                "name_en, name_ja) VALUES (?, ?, ?, ?, ?, ?, ?)"));
    if (!items.isValid())
        return fail(items.error());
    if (!forEachRecord(QStringLiteral("items"),
                       {QStringLiteral("id"), QStringLiteral("identifier"),
                        QStringLiteral("category_id"), QStringLiteral("cost")},
                       [&](const QStringList &v) {
                           const int id = v[0].toInt();
                           Names n = names.value(id);
                           if (n[0].isEmpty()) // PokéAPI에 한국어 이름이 없으면 보충 표에서
                               n[0] = supplementNameKo(v[1]);
                           return items.exec({id, v[1], v[2].toInt(), intOrNull(v[3]),
                                              textOrNull(n[0]), textOrNull(n[1]), textOrNull(n[2])})
                                  || fail(items.error());
                       }))
        return false;

    // 3) 세대별 존재: 그 세대 게임의 아이템 번호(game index)가 있으면 그 세대에 있다
    Insert generations(db, QStringLiteral("INSERT OR IGNORE INTO item_generations (item_id, "
                                          "generation) VALUES (?, ?)"));
    if (!generations.isValid())
        return fail(generations.error());
    return forEachRecord(QStringLiteral("item_game_indices"),
                         {QStringLiteral("item_id"), QStringLiteral("generation_id")},
                         [&](const QStringList &v) {
                             return generations.exec({v[0].toInt(), v[1].toInt()})
                                    || fail(generations.error());
                         });
}

bool CsvImporter::importMoves(QSqlDatabase &db)
{
    // 버전 그룹 → 세대(옛 타입이 "어느 세대까지"였는지 계산에 쓴다)
    QHash<int, int> versionGroupGen;
    if (!forEachRecord(QStringLiteral("version_groups"),
                       {QStringLiteral("id"), QStringLiteral("generation_id")},
                       [&](const QStringList &v) {
                           versionGroupGen.insert(v[0].toInt(), v[1].toInt());
                           return true;
                       }))
        return false;

    // 옛 타입: move_changelog의 (기술, V, 타입) = "버전 그룹 V 전까지는 이 타입" → (gen(V) − 1,
    // 타입)
    // 위력 · PP · 명중의 옛 값은 그대로 move_changelog 표에 옮긴다(조회할 때 세대에 맞춰 고른다).
    QHash<int, std::vector<std::pair<int, int>>> pastTypes;
    Insert changelog(db, QStringLiteral("INSERT INTO move_changelog (move_id, until_gen, type_id, "
                                        "power, pp, accuracy) VALUES (?, ?, ?, ?, ?, ?)"));
    if (!changelog.isValid())
        return fail(changelog.error());
    if (!forEachRecord(QStringLiteral("move_changelog"),
                       {QStringLiteral("move_id"), QStringLiteral("changed_in_version_group_id"),
                        QStringLiteral("type_id"), QStringLiteral("power"), QStringLiteral("pp"),
                        QStringLiteral("accuracy")},
                       [&](const QStringList &v) {
                           const int until = versionGroupGen.value(v[1].toInt()) - 1;
                           if (!v[2].isEmpty())
                               pastTypes[v[0].toInt()].push_back({until, v[2].toInt()});
                           return changelog.exec({v[0].toInt(), until, intOrNull(v[2]),
                                                  intOrNull(v[3]), intOrNull(v[4]),
                                                  intOrNull(v[5])})
                                  || fail(changelog.error());
                       }))
        return false;

    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("move_names"),
                       {QStringLiteral("move_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           return true;
                       }))
        return false;

    Insert moves(db,
                 QStringLiteral("INSERT INTO moves (id, identifier, intro_gen, name_ko, name_en, "
                                "name_ja, type_id, power, pp, accuracy, damage_class) "
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    Insert types(db, QStringLiteral("INSERT INTO move_types (move_id, type_id, gen_from, gen_to) "
                                    "VALUES (?, ?, ?, ?)"));
    if (!moves.isValid() || !types.isValid())
        return fail(moves.isValid() ? types.error() : moves.error());
    return forEachRecord(
            QStringLiteral("moves"),
            {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("generation_id"),
             QStringLiteral("type_id"), QStringLiteral("power"), QStringLiteral("pp"),
             QStringLiteral("accuracy"), QStringLiteral("damage_class_id")},
            [&](const QStringList &v) {
                const int id = v[0].toInt();
                const int intro = v[2].toInt();
                const Names &n = names.value(id);
                if (!moves.exec({id, v[1], intro, textOrNull(n[0]), textOrNull(n[1]),
                                 textOrNull(n[2]), intOrNull(v[3]), intOrNull(v[4]),
                                 intOrNull(v[5]), intOrNull(v[6]), intOrNull(v[7])}))
                    return fail(moves.error());
                for (const auto &range :
                     genranges::toGenRanges<int>(intro, pastTypes.value(id), v[3].toInt())) {
                    if (range.value >= kFirstNonBattleTypeId)
                        continue; // ??? 타입(저주의 옛 타입)은 types 표에 없다
                    if (!types.exec({id, range.value, range.from, genOrNull(range.to)}))
                        return fail(types.error());
                }
                return true;
            });
}

bool CsvImporter::importMachines(QSqlDatabase &db)
{
    // 기술머신 번호 · 담긴 기술은 게임마다 다르다. 세대마다 그 세대 첫 게임(버전 그룹 순서가 가장
    // 이른 것)의 기술을 쓴다 — 효과 문구와 같은 규칙.
    struct GroupInfo
    {
        int generation = 0;
        int order = 0;
    };
    QHash<int, GroupInfo> groups;
    if (!forEachRecord(
                QStringLiteral("version_groups"),
                {QStringLiteral("id"), QStringLiteral("generation_id"), QStringLiteral("order")},
                [&](const QStringList &v) {
                    groups.insert(v[0].toInt(), {v[1].toInt(), v[2].toInt()});
                    return true;
                }))
        return false;
    struct Chosen
    {
        int order = 0;
        int move = 0;
    };
    QHash<QPair<int, int>, Chosen> chosen; // (item, 세대) → 기술
    // 게임마다의 기술머신 번호 표(상세 화면의 "기술머신으로 익히는 기술")는 그대로 옮긴다
    Insert machines(db, QStringLiteral("INSERT INTO machines (version_group_id, machine_number, "
                                       "item_id, move_id) VALUES (?, ?, ?, ?)"));
    if (!machines.isValid())
        return fail(machines.error());
    if (!forEachRecord(QStringLiteral("machines"),
                       {QStringLiteral("item_id"), QStringLiteral("version_group_id"),
                        QStringLiteral("move_id"), QStringLiteral("machine_number")},
                       [&](const QStringList &v) {
                           if (!machines.exec(
                                       {v[1].toInt(), v[3].toInt(), v[0].toInt(), v[2].toInt()}))
                               return fail(machines.error());
                           const GroupInfo group = groups.value(v[1].toInt());
                           const QPair<int, int> key(v[0].toInt(), group.generation);
                           const auto it = chosen.constFind(key);
                           if (it == chosen.constEnd() || group.order < it->order)
                               chosen.insert(key, {group.order, v[2].toInt()});
                           return true;
                       }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO item_machines (item_id, generation, move_id) "
                                     "VALUES (?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (auto it = chosen.cbegin(); it != chosen.cend(); ++it)
        if (!insert.exec({it.key().first, it.key().second, it->move}))
            return fail(insert.error());
    return true;
}

bool CsvImporter::importPokemonMoves(QSqlDatabase &db)
{
    // 습득 기술 63만 줄 중 레벨업 · 교배 · NPC · 기술머신(1–4)만. 나머지(스타디움 · 특별 이벤트
    // 등)는 뺀다.
    Insert insert(db, QStringLiteral("INSERT INTO pokemon_moves (pokemon_id, version_group_id, "
                                     "move_id, method, level) VALUES (?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(
            QStringLiteral("pokemon_moves"),
            {QStringLiteral("pokemon_id"), QStringLiteral("version_group_id"),
             QStringLiteral("move_id"), QStringLiteral("pokemon_move_method_id"),
             QStringLiteral("level")},
            [&](const QStringList &v) {
                const int method = v[3].toInt();
                if (method < 1 || method > 4)
                    return true;
                return insert.exec({v[0].toInt(), v[1].toInt(), v[2].toInt(), method, v[4].toInt()})
                       || fail(insert.error());
            });
}

bool CsvImporter::importEncounters(QSqlDatabase &db)
{
    // 1) 장소 + 이름(한국어는 신오 · 성도 · 관동에 없다 — UI가 장소 사전으로 채운다)
    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("location_names"),
                       {QStringLiteral("location_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           return true;
                       }))
        return false;
    Insert locations(db, QStringLiteral("INSERT INTO locations (id, identifier, region_id, "
                                        "name_ko, name_en, name_ja) VALUES (?, ?, ?, ?, ?, ?)"));
    if (!locations.isValid())
        return fail(locations.error());
    if (!forEachRecord(
                QStringLiteral("locations"),
                {QStringLiteral("id"), QStringLiteral("identifier"), QStringLiteral("region_id")},
                [&](const QStringList &v) {
                    const Names &n = names.value(v[0].toInt());
                    return locations.exec({v[0].toInt(), v[1], intOrNull(v[2]), textOrNull(n[0]),
                                           textOrNull(n[1]), textOrNull(n[2])})
                           || fail(locations.error());
                }))
        return false;

    // 2) 방법 표 · 구역 → 장소 · 출현 칸 → (방법, 확률)
    Insert methods(db,
                   QStringLiteral("INSERT INTO encounter_methods (id, identifier) VALUES (?, ?)"));
    if (!methods.isValid())
        return fail(methods.error());
    if (!forEachRecord(QStringLiteral("encounter_methods"),
                       {QStringLiteral("id"), QStringLiteral("identifier")},
                       [&](const QStringList &v) {
                           return methods.exec({v[0].toInt(), v[1]}) || fail(methods.error());
                       }))
        return false;
    QHash<int, int> locationOfArea;
    if (!forEachRecord(QStringLiteral("location_areas"),
                       {QStringLiteral("id"), QStringLiteral("location_id")},
                       [&](const QStringList &v) {
                           locationOfArea.insert(v[0].toInt(), v[1].toInt());
                           return true;
                       }))
        return false;
    struct Slot
    {
        int method = 0;
        int rarity = 0;
    };
    QHash<int, Slot> encounterSlots;
    if (!forEachRecord(QStringLiteral("encounter_slots"),
                       {QStringLiteral("id"), QStringLiteral("encounter_method_id"),
                        QStringLiteral("rarity")},
                       [&](const QStringList &v) {
                           encounterSlots.insert(v[0].toInt(), {v[1].toInt(), v[2].toInt()});
                           return true;
                       }))
        return false;

    // 3) 출현: (포켓몬, 버전, 장소, 방법)마다 레벨 범위 · 확률 합으로 묶는다. 같은 장소의 층 ·
    // 시간대 ·
    //    계절 칸이 여러 줄로 나뉘어 있어서(1만 줄 넘게), 화면에 쓰기 좋은 단위로 줄인다.
    struct Summary
    {
        int minLevel = 1000;
        int maxLevel = 0;
        int rarity = 0;
    };
    QMap<std::array<int, 4>, Summary> summary;
    if (!forEachRecord(QStringLiteral("encounters"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("version_id"),
                        QStringLiteral("location_area_id"), QStringLiteral("encounter_slot_id"),
                        QStringLiteral("min_level"), QStringLiteral("max_level")},
                       [&](const QStringList &v) {
                           const Slot slot = encounterSlots.value(v[3].toInt());
                           const std::array<int, 4> key {v[0].toInt(), v[1].toInt(),
                                                         locationOfArea.value(v[2].toInt()),
                                                         slot.method};
                           Summary &s = summary[key];
                           s.minLevel = std::min(s.minLevel, v[4].toInt());
                           s.maxLevel = std::max(s.maxLevel, v[5].toInt());
                           s.rarity += slot.rarity;
                           return true;
                       }))
        return false;
    Insert encounters(db, QStringLiteral("INSERT INTO encounters (pokemon_id, version_id, "
                                         "location_id, method_id, min_level, max_level, rarity) "
                                         "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    if (!encounters.isValid())
        return fail(encounters.error());
    for (auto it = summary.cbegin(); it != summary.cend(); ++it) {
        const auto &k = it.key();
        if (!encounters.exec({k[0], k[1], k[2], k[3], it->minLevel, it->maxLevel, it->rarity}))
            return fail(encounters.error());
    }
    return true;
}

bool CsvImporter::importEvolutions(QSqlDatabase &db)
{
    QHash<int, int> versionGroupGen;
    if (!forEachRecord(QStringLiteral("version_groups"),
                       {QStringLiteral("id"), QStringLiteral("generation_id")},
                       [&](const QStringList &v) {
                           versionGroupGen.insert(v[0].toInt(), v[1].toInt());
                           return true;
                       }))
        return false;
    Insert insert(
            db, QStringLiteral(
                        "INSERT INTO evolutions (evolved_species_id, generation, trigger, item_id, "
                        "min_level, gender, location_id, held_item_id, time_of_day, known_move_id, "
                        "known_move_type_id, min_happiness, min_beauty, min_affection, "
                        "relative_stats, party_species_id, party_type_id, trade_species_id, "
                        "needs_rain, upside_down) "
                        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("pokemon_evolution"),
                         {QStringLiteral("evolved_species_id"),
                          QStringLiteral("version_group_id"),
                          QStringLiteral("evolution_trigger_id"),
                          QStringLiteral("trigger_item_id"),
                          QStringLiteral("minimum_level"),
                          QStringLiteral("gender_id"),
                          QStringLiteral("location_id"),
                          QStringLiteral("held_item_id"),
                          QStringLiteral("time_of_day"),
                          QStringLiteral("known_move_id"),
                          QStringLiteral("known_move_type_id"),
                          QStringLiteral("minimum_happiness"),
                          QStringLiteral("minimum_beauty"),
                          QStringLiteral("minimum_affection"),
                          QStringLiteral("relative_physical_stats"),
                          QStringLiteral("party_species_id"),
                          QStringLiteral("party_type_id"),
                          QStringLiteral("trade_species_id"),
                          QStringLiteral("needs_overworld_rain"),
                          QStringLiteral("turn_upside_down")},
                         [&](const QStringList &v) {
                             // version_group_id가 비어 있는 줄은 그 방법이 처음부터(1세대) 있던
                             // 것으로 본다
                             const int generation
                                     = v[1].isEmpty() ? 1 : versionGroupGen.value(v[1].toInt(), 1);
                             return insert.exec({v[0].toInt(),
                                                 generation,
                                                 v[2].toInt(),
                                                 intOrNull(v[3]),
                                                 intOrNull(v[4]),
                                                 intOrNull(v[5]),
                                                 intOrNull(v[6]),
                                                 intOrNull(v[7]),
                                                 textOrNull(v[8].isEmpty() ? QString() : v[8]),
                                                 intOrNull(v[9]),
                                                 intOrNull(v[10]),
                                                 intOrNull(v[11]),
                                                 intOrNull(v[12]),
                                                 intOrNull(v[13]),
                                                 intOrNull(v[14]),
                                                 intOrNull(v[15]),
                                                 intOrNull(v[16]),
                                                 intOrNull(v[17]),
                                                 v[18].toInt(),
                                                 v[19].toInt()})
                                    || fail(insert.error());
                         });
}

bool CsvImporter::importFlavorTexts(QSqlDatabase &db, const QString &csv, const QString &idColumn,
                                    const QString &table, const QString &idField)
{
    // 게임 설명문(*_flavor_text) → 세대 · 언어마다 한 줄. 아이템 · 특성 · 기술이 같이 쓴다.
    // 버전 그룹 → (세대, 순서). 세대마다 "그 세대 첫 게임"의 문구를 고르는 데 쓴다.
    // (7세대: SM · USUM · LGPE 중 SM. 기술머신은 게임마다 담긴 기술이 달라 문구도 다르다)
    struct GroupInfo
    {
        int generation = 0;
        int order = 0;
    };
    QHash<int, GroupInfo> groups;
    if (!forEachRecord(
                QStringLiteral("version_groups"),
                {QStringLiteral("id"), QStringLiteral("generation_id"), QStringLiteral("order")},
                [&](const QStringList &v) {
                    groups.insert(v[0].toInt(), {v[1].toInt(), v[2].toInt()});
                    return true;
                }))
        return false;

    // (대상, 세대) → 언어마다 가장 이른 게임의 문구. 설명문 중 ko · en · ja 줄만 본다.
    struct Chosen
    {
        std::array<int, 3> order {}; // 0 = 아직 없음
        std::array<QString, 3> text; // ko, en, ja
    };
    QHash<QPair<int, int>, Chosen> chosen;
    if (!forEachRecord(csv,
                       {idColumn, QStringLiteral("version_group_id"), QStringLiteral("language_id"),
                        QStringLiteral("flavor_text")},
                       [&](const QStringList &v) {
                           const int language = v[2].toInt();
                           const int column = nameColumnOf(language);
                           if (column < 0)
                               return true;
                           const GroupInfo group = groups.value(v[1].toInt());
                           Chosen &slot = chosen[QPair<int, int>(v[0].toInt(), group.generation)];
                           // 더 이른 게임이면 바꾼다. 같은 게임의 가나 표기(1)는 한자 표기(11)를
                           // 덮지 않는다
                           const bool earlier
                                   = slot.order[column] == 0 || group.order < slot.order[column];
                           const bool sameGameKana
                                   = group.order == slot.order[column] && language == kJapaneseKana;
                           if (earlier && !sameGameKana) {
                               slot.order[column] = group.order;
                               slot.text[column] = v[3];
                           }
                           return true;
                       }))
        return false;

    Insert insert(db, QStringLiteral("INSERT INTO %1 (%2, generation, text_ko, text_en, text_ja) "
                                     "VALUES (?, ?, ?, ?, ?)")
                              .arg(table, idField));
    if (!insert.isValid())
        return fail(insert.error());
    for (auto it = chosen.cbegin(); it != chosen.cend(); ++it) {
        // 게임 화면의 줄바꿈(\n) · 쪽 넘김(\f)을 한 줄로. 한국어 · 영어는 띄어쓰기로 잇고, 일본어는
        // 띄어쓰기를 쓰지 않는 글이라 그냥 붙인다.
        const auto oneLine = [](QString text, bool spaced) {
            if (!spaced)
                text = toHalfWidth(text.remove(QLatin1Char('\n')).remove(QLatin1Char('\f')));
            return textOrNull(text.isNull() ? text : text.simplified());
        };
        if (!insert.exec({it.key().first, it.key().second, oneLine(it->text[0], true),
                          oneLine(it->text[1], true), oneLine(it->text[2], false)}))
            return fail(insert.error());
    }
    return true;
}

bool CsvImporter::importItemEffects(QSqlDatabase &db)
{
    return importFlavorTexts(db, QStringLiteral("item_flavor_text"), QStringLiteral("item_id"),
                             QStringLiteral("item_effects"), QStringLiteral("item_id"));
}

bool CsvImporter::importMoveEffects(QSqlDatabase &db)
{
    return importFlavorTexts(db, QStringLiteral("move_flavor_text"), QStringLiteral("move_id"),
                             QStringLiteral("move_effects"), QStringLiteral("move_id"));
}

bool CsvImporter::importMoveMeta(QSqlDatabase &db)
{
    // 1) 대상(moves.target_id): 7 = 자신, 10 = 고른 상대 … 능력치 변화가 누구 것인지 가른다
    QHash<int, int> targets;
    if (!forEachRecord(QStringLiteral("moves"), {QStringLiteral("id"), QStringLiteral("target_id")},
                       [&](const QStringList &v) {
                           targets.insert(v[0].toInt(), v[1].toInt());
                           return true;
                       }))
        return false;
    // 2) move_meta: 분류 · 상태이상 · 회복량(%, 음수 = 소모). 표에 없는 기술(Z기술 등)은 대상만
    Insert meta(db, QStringLiteral("INSERT INTO move_meta (move_id, target, category, ailment, "
                                   "healing) VALUES (?, ?, ?, ?, ?)"));
    if (!meta.isValid())
        return fail(meta.error());
    QSet<int> seen;
    if (!forEachRecord(QStringLiteral("move_meta"),
                       {QStringLiteral("move_id"), QStringLiteral("meta_category_id"),
                        QStringLiteral("meta_ailment_id"), QStringLiteral("healing")},
                       [&](const QStringList &v) {
                           const int id = v[0].toInt();
                           seen.insert(id);
                           return meta.exec({id, targets.value(id), v[1].toInt(), v[2].toInt(),
                                             v[3].toInt()})
                                  || fail(meta.error());
                       }))
        return false;
    for (auto it = targets.cbegin(); it != targets.cend(); ++it)
        if (!seen.contains(it.key()) && !meta.exec({it.key(), it.value(), 0, 0, 0}))
            return fail(meta.error());
    // 3) 능력치 변화(stat_id: 2 공격 … 6 스피드 · 7 명중률 · 8 회피율)
    Insert stats(
            db,
            QStringLiteral(
                    "INSERT INTO move_stat_changes (move_id, stat_id, change) VALUES (?, ?, ?)"));
    if (!stats.isValid())
        return fail(stats.error());
    return forEachRecord(
            QStringLiteral("move_meta_stat_changes"),
            {QStringLiteral("move_id"), QStringLiteral("stat_id"), QStringLiteral("change")},
            [&](const QStringList &v) {
                return stats.exec({v[0].toInt(), v[1].toInt(), v[2].toInt()})
                       || fail(stats.error());
            });
}

bool CsvImporter::importAbilities(QSqlDatabase &db)
{
    // 1) 특성 + 이름
    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("ability_names"),
                       {QStringLiteral("ability_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           return true;
                       }))
        return false;
    Insert abilities(db,
                     QStringLiteral("INSERT INTO abilities (id, identifier, intro_gen, name_ko, "
                                    "name_en, name_ja) VALUES (?, ?, ?, ?, ?, ?)"));
    if (!abilities.isValid())
        return fail(abilities.error());
    if (!forEachRecord(QStringLiteral("abilities"),
                       {QStringLiteral("id"), QStringLiteral("identifier"),
                        QStringLiteral("generation_id"), QStringLiteral("is_main_series")},
                       [&](const QStringList &v) {
                           if (v[3].toInt() != 1)
                               return true; // 본편이 아닌 특성(포켓몬 콜로세움 등)은 뺀다
                           const Names &n = names.value(v[0].toInt());
                           return abilities.exec({v[0].toInt(), v[1], v[2].toInt(),
                                                  textOrNull(n[0]), textOrNull(n[1]),
                                                  textOrNull(n[2])})
                                  || fail(abilities.error());
                       }))
        return false;

    // 2) 포켓몬 특성: 칸마다 (지금 값 + 옛 값) → 세대 구간. 옛 값의 빈 칸 = 그 세대까지 그 칸이
    // 없었다
    //    (숨겨진 특성 3번 칸은 대개 4세대까지 비어 있다). 특성은 3세대부터라 구간도 3세대부터.
    struct Slot
    {
        std::optional<int> current;            // 지금 특성(없으면 nullopt)
        std::vector<std::pair<int, int>> past; // (그 세대까지, 특성 — 0 = 칸 없음)
        bool hidden = false;
    };
    QMap<QPair<int, int>, Slot> abilitySlots; // (pokemon, 칸)
    if (!forEachRecord(QStringLiteral("pokemon_abilities"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("ability_id"),
                        QStringLiteral("is_hidden"), QStringLiteral("slot")},
                       [&](const QStringList &v) {
                           Slot &slot = abilitySlots[QPair<int, int>(v[0].toInt(), v[3].toInt())];
                           slot.current = v[1].toInt();
                           slot.hidden = v[2].toInt() != 0;
                           return true;
                       }))
        return false;
    if (!forEachRecord(QStringLiteral("pokemon_abilities_past"),
                       {QStringLiteral("pokemon_id"), QStringLiteral("generation_id"),
                        QStringLiteral("ability_id"), QStringLiteral("is_hidden"),
                        QStringLiteral("slot")},
                       [&](const QStringList &v) {
                           Slot &slot = abilitySlots[QPair<int, int>(v[0].toInt(), v[4].toInt())];
                           slot.past.push_back({v[1].toInt(), v[2].isEmpty() ? 0 : v[2].toInt()});
                           slot.hidden = slot.hidden || v[3].toInt() != 0;
                           return true;
                       }))
        return false;
    Insert insert(db, QStringLiteral("INSERT INTO pokemon_abilities (pokemon_id, slot, ability_id, "
                                     "is_hidden, gen_from, gen_to) VALUES (?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    for (auto it = abilitySlots.cbegin(); it != abilitySlots.cend(); ++it) {
        const int pokemonId = it.key().first;
        const int firstGen = std::max(kFirstAbilityGeneration, pokemonIntro(pokemonId));
        for (const auto &range : toGenRanges<int>(firstGen, it->past, it->current)) {
            if (range.value == 0)
                continue; // 이 구간에는 그 칸이 없었다
            if (!insert.exec({pokemonId, it.key().second, range.value, it->hidden ? 1 : 0,
                              range.from, genOrNull(range.to)}))
                return fail(insert.error());
        }
    }

    // 3) 설명문(게임 문구, 세대 · 언어마다)
    return importFlavorTexts(db, QStringLiteral("ability_flavor_text"),
                             QStringLiteral("ability_id"), QStringLiteral("ability_effects"),
                             QStringLiteral("ability_id"));
}

bool CsvImporter::importNatures(QSqlDatabase &db)
{
    QHash<int, Names> names;
    if (!forEachRecord(QStringLiteral("nature_names"),
                       {QStringLiteral("nature_id"), QStringLiteral("local_language_id"),
                        QStringLiteral("name")},
                       [&](const QStringList &v) {
                           setName(names[v[0].toInt()], v[1].toInt(), v[2]);
                           return true;
                       }))
        return false;
    Insert insert(db, QStringLiteral("INSERT INTO natures (id, identifier, increased_stat, "
                                     "decreased_stat, name_ko, name_en, name_ja) "
                                     "VALUES (?, ?, ?, ?, ?, ?, ?)"));
    if (!insert.isValid())
        return fail(insert.error());
    return forEachRecord(QStringLiteral("natures"),
                         {QStringLiteral("id"), QStringLiteral("identifier"),
                          QStringLiteral("increased_stat_id"), QStringLiteral("decreased_stat_id")},
                         [&](const QStringList &v) {
                             const Names &n = names.value(v[0].toInt());
                             return insert.exec({v[0].toInt(), v[1], v[2].toInt(), v[3].toInt(),
                                                 textOrNull(n[0]), textOrNull(n[1]),
                                                 textOrNull(n[2])})
                                    || fail(insert.error());
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
