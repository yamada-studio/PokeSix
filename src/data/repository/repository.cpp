#include "data/repository/repository.h"

#include "core/rules/movereach.h"
#include "data/db/gamedatabase.h"
#include "data/logging/logging.h"
#include "data/text/namebook.h"

#include <QHash>
#include <QSet>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

#include <algorithm>
#include <functional>
#include <iterator>

namespace {
// 세대 g의 값 = 구간 [gen_from, gen_to]에 g가 들어가는 줄 (ADR 0011, schema.h):
//   gen_from <= :g AND (gen_to IS NULL OR gen_to >= :g)
// 같은 이름의 자리표시(:g)를 두 번 써도 SQLite는 같은 값으로 채운다.
constexpr int kSpecialStat = 9; // 1세대 전용 "특수"

// 질의 결과의 column · column+1 · column+2 열(ko · en · ja)을 하나로
com::yamada::studio::LocalizedText localized(const QSqlQuery &query, int column)
{
    return {query.value(column).toString(), query.value(column + 1).toString(),
            query.value(column + 2).toString()};
}

// 3세대까지는 기술마다가 아니라 타입마다 물리 · 특수가 정해져 있었다(4세대 DP에서 기술별로 나뉨).
// 세대 규칙을 if 대신 표로 둔다(architecture §5).
constexpr int kHiddenMachineOffset = 100; // machines.machine_number: 비전머신 n = 100 + n
constexpr int kLastTypeBasedDamageClassGeneration = 3;
constexpr int kStatusClass = 1;
constexpr int kPhysicalClass = 2;
constexpr int kSpecialClass = 3;
const QSet<QString> kPhysicalTypesBeforeSplit
        = {QStringLiteral("normal"), QStringLiteral("fighting"), QStringLiteral("flying"),
           QStringLiteral("poison"), QStringLiteral("ground"),   QStringLiteral("rock"),
           QStringLiteral("bug"),    QStringLiteral("ghost"),    QStringLiteral("steel")};
} // namespace

namespace com::yamada::studio {
Repository::Repository(const QString &dbPath)
    : m_path(dbPath)
    , m_connection(
              QStringLiteral("pokesix.repository.%1").arg(reinterpret_cast<quintptr>(this), 0, 16))
{
}

Repository::~Repository()
{
    if (QSqlDatabase::contains(m_connection)) {
        QSqlDatabase::database(m_connection, false).close(); // false: 닫으려고 다시 열지 않는다
        QSqlDatabase::removeDatabase(m_connection);
    }
}

bool Repository::open()
{
    if (QSqlDatabase::contains(m_connection) && QSqlDatabase::database(m_connection).isOpen())
        return true;
    if (!gamedatabase::isUsable(m_path)) {
        m_error = QStringLiteral("game database is missing or has another schema version");
        return false;
    }
    QSqlDatabase db = QSqlDatabase::contains(m_connection)
                              ? QSqlDatabase::database(m_connection, false)
                              : QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connection);
    db.setDatabaseName(m_path);
    db.setConnectOptions(
            QStringLiteral("QSQLITE_OPEN_READONLY")); // 앱은 게임 데이터를 고치지 않는다
    if (!db.open()) {
        m_error = db.lastError().text();
        qCWarning(lcData) << "cannot open game database" << m_path << m_error;
        return false;
    }
    return true;
}

void Repository::close()
{
    m_details.clear(); // 다시 열면 DB가 새로 만들어졌을 수 있다
    m_otherVersionSpecies.clear();
    if (QSqlDatabase::contains(m_connection))
        QSqlDatabase::database(m_connection, false).close();
}

QList<ItemEvolution> Repository::evolutionsWithItem(int itemId, int generation)
{
    QList<ItemEvolution> evolutions;
    if (!open())
        return evolutions;
    // 사용(item_id) · 지님(held_item_id) 모두. 규칙이 세대마다 여러 줄일 수 있어 DISTINCT.
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral("SELECT DISTINCT f.name_ko, f.name_en, f.name_ja, "
                                 "       t.name_ko, t.name_en, t.name_ja, e.held_item_id = :held "
                                 "FROM evolutions e "
                                 "JOIN species t ON t.id = e.evolved_species_id "
                                 "LEFT JOIN species f ON f.id = t.evolves_from "
                                 "WHERE (e.item_id = :item OR e.held_item_id = :held2) "
                                 "  AND e.generation <= :g AND t.intro_gen <= :g2 "
                                 "ORDER BY t.id"));
    query.bindValue(QStringLiteral(":item"), itemId);
    query.bindValue(QStringLiteral(":held"), itemId);
    query.bindValue(QStringLiteral(":held2"), itemId);
    query.bindValue(QStringLiteral(":g"), generation);
    query.bindValue(QStringLiteral(":g2"), generation);
    if (!query.exec()) {
        m_error = query.lastError().text();
        qCWarning(lcData) << "item evolution query failed:" << m_error;
        return evolutions;
    }
    while (query.next())
        evolutions.append({localized(query, 0), localized(query, 3), query.value(6).toBool()});
    return evolutions;
}

QList<SpeciesRow> Repository::speciesForGeneration(int generation)
{
    QList<SpeciesRow> rows;
    if (!open())
        return rows;

    // 그 세대까지 나온 종과 기본 모습. 전국도감이므로 도감 번호 = 종 번호.
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(
            QStringLiteral("SELECT s.id, p.id, s.name_ko, s.name_en, s.name_ja, s.id, "
                           "       s.is_legendary OR s.is_mythical, "
                           "       NOT EXISTS (SELECT 1 FROM species c "
                           "                   WHERE c.evolves_from = s.id AND c.intro_gen <= :g2) "
                           "FROM species s "
                           "JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1 "
                           "WHERE s.intro_gen <= :g ORDER BY s.id"));
    query.bindValue(QStringLiteral(":g"), generation);
    query.bindValue(QStringLiteral(":g2"), generation);
    if (!readSpecies(query, rows))
        return rows;
    fillTypesAndStats(rows, generation);
    return rows;
}

QList<SpeciesRow> Repository::speciesForDex(int pokedexId, int generation)
{
    QList<SpeciesRow> rows;
    if (!open())
        return rows;

    // 지방 도감: 도감 번호 순. 어떤 종이 들어가는지는 dex_numbers가 정한다(신오도감에 이상해씨는
    // 없다). 그 도감의 게임은 모두 generation 이하에 나왔으므로 intro_gen 조건은 필요 없다.
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(
            QStringLiteral("SELECT s.id, p.id, s.name_ko, s.name_en, s.name_ja, dn.number, "
                           "       s.is_legendary OR s.is_mythical, "
                           "       NOT EXISTS (SELECT 1 FROM species c "
                           "                   WHERE c.evolves_from = s.id AND c.intro_gen <= :g) "
                           "FROM dex_numbers dn "
                           "JOIN species s ON s.id = dn.species_id "
                           "JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1 "
                           "WHERE dn.pokedex_id = :dex ORDER BY dn.number"));
    query.bindValue(QStringLiteral(":dex"), pokedexId);
    query.bindValue(QStringLiteral(":g"), generation);
    if (!readSpecies(query, rows))
        return rows;
    fillTypesAndStats(rows, generation);
    return rows;
}

QList<DexInfo> Repository::dexesForGeneration(int generation)
{
    QList<DexInfo> dexes;
    if (!open())
        return dexes;

    // 줄마다 (도감, 버전) 하나. 게임이 나온 순서(sort_order)대로 읽으면서 도감별로 묶는다.
    // 본편 도감만(is_main_series), 지방이 있는 것만(전국 · conquest 제외 — 전국은 UI가 따로 단다).
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral(
            "SELECT d.id, d.identifier, r.name_ko, r.name_en, r.name_ja, v.identifier, v.name_ko, "
            "v.name_en, v.name_ja, vg.identifier "
            "FROM pokedexes d "
            "JOIN pokedex_version_groups pvg ON pvg.pokedex_id = d.id "
            "JOIN version_groups vg ON vg.id = pvg.version_group_id "
            "JOIN versions v ON v.version_group_id = vg.id "
            "JOIN regions r ON r.id = d.region_id "
            "WHERE vg.generation = :g AND d.is_main_series = 1 "
            "ORDER BY vg.sort_order, v.id"));
    query.bindValue(QStringLiteral(":g"), generation);
    if (!query.exec()) {
        m_error = query.lastError().text();
        qCWarning(lcData) << "pokedex query failed:" << m_error;
        return dexes;
    }
    QHash<int, qsizetype> indexOfDex; // 도감 id → dexes의 위치 (처음 나온 순서를 지킨다)
    while (query.next()) {
        const int id = query.value(0).toInt();
        auto it = indexOfDex.constFind(id);
        if (it == indexOfDex.constEnd()) {
            DexInfo dex;
            dex.pokedexId = id;
            dex.identifier = query.value(1).toString();
            dex.region = localized(query, 2);
            it = indexOfDex.insert(id, dexes.size());
            dexes.append(dex);
        }
        DexInfo &dex = dexes[*it];
        dex.versions.append(query.value(5).toString());
        dex.versionNames.append(localized(query, 6));
        namebook::fill(namebook::Kind::Version, dex.versions.last(), dex.versionNames.last());
        const QString group = query.value(9).toString();
        if (!dex.versionGroups.contains(group))
            dex.versionGroups.append(group);
    }
    return dexes;
}

int ItemRow::introGeneration() const
{
    for (int g = 1; g <= 16; ++g)
        if (existsIn(g))
            return g;
    return 0;
}

QList<ItemRow> Repository::itemsForGeneration(int generation, const QString &versionGroup)
{
    QList<ItemRow> rows;
    if (!open())
        return rows;
    QSqlQuery query(QSqlDatabase::database(m_connection));

    // 1) 아이템 + 분류. 어느 세대에도 없던 아이템(미사용 데이터 등)은 뺀다.
    if (!query.exec(QStringLiteral(
                "SELECT i.id, i.identifier, c.identifier, c.pocket, i.name_ko, i.name_en, "
                "i.name_ja, i.cost "
                "FROM items i JOIN item_categories c ON c.id = i.category_id "
                "WHERE EXISTS (SELECT 1 FROM item_generations g WHERE g.item_id = i.id) "
                "ORDER BY i.id"))) {
        m_error = query.lastError().text();
        qCWarning(lcData) << "item query failed:" << m_error;
        return rows;
    }
    QHash<int, qsizetype> rowOfItem;
    while (query.next()) {
        ItemRow row;
        row.id = query.value(0).toInt();
        row.identifier = query.value(1).toString();
        row.category = query.value(2).toString();
        row.pocket = query.value(3).toString();
        row.name = localized(query, 4);
        namebook::fill(namebook::Kind::Item, row.identifier, row.name); // 빈 한국어 칸 보충
        row.cost = query.value(7).toInt();
        rowOfItem.insert(row.id, rows.size());
        rows.append(row);
    }

    // 2) 세대별 존재 → 비트
    if (query.exec(QStringLiteral("SELECT item_id, generation FROM item_generations"))) {
        while (query.next()) {
            const auto it = rowOfItem.constFind(query.value(0).toInt());
            const int g = query.value(1).toInt();
            if (it != rowOfItem.constEnd() && g >= 1 && g <= 16)
                rows[*it].generations |= quint16(1u << (g - 1));
        }
    }

    QHash<int, int> moveOfItem; // 기술머신 item id → 이 세대(게임)에 담긴 move id
    // 3) 기술머신: 담긴 기술 + 그 기술의 이 세대 타입(구간 질의). 게임을 주면 그 게임의 기술머신
    //    표(machines — 버전 그룹 단위)를, 아니면 세대 표(item_machines — 그 세대 첫 게임)를 쓴다
    const QString machineSource
            = versionGroup.isEmpty()
                      ? QStringLiteral("(SELECT item_id, move_id FROM item_machines "
                                       " WHERE generation = :g)")
                      : QStringLiteral("(SELECT ma.item_id, ma.move_id FROM machines ma "
                                       " JOIN version_groups vg ON vg.id = ma.version_group_id "
                                       " WHERE vg.identifier = :vg)");
    query.prepare(
            QStringLiteral("SELECT im.item_id, m.name_ko, m.name_en, m.name_ja, t.identifier, "
                           "im.move_id, m.identifier FROM %1 im "
                           "JOIN moves m ON m.id = im.move_id "
                           "LEFT JOIN move_types mt ON mt.move_id = m.id AND mt.gen_from <= :g "
                           "  AND (mt.gen_to IS NULL OR mt.gen_to >= :g) "
                           "LEFT JOIN types t ON t.id = mt.type_id")
                    .arg(machineSource));
    query.bindValue(QStringLiteral(":g"), generation);
    if (!versionGroup.isEmpty())
        query.bindValue(QStringLiteral(":vg"), versionGroup);
    if (query.exec()) {
        while (query.next()) {
            const auto it = rowOfItem.constFind(query.value(0).toInt());
            if (it == rowOfItem.constEnd())
                continue;
            ItemRow &row = rows[*it];
            row.machineMove = localized(query, 1);
            namebook::fill(namebook::Kind::Move, query.value(6).toString(), row.machineMove);
            row.machineType = query.value(4).toString();
            moveOfItem.insert(row.id, query.value(5).toInt());
        }
    }

    // 4) 효과 문구: 언어마다 따로 고른다(한국어는 6세대부터, 영어는 3세대부터 있다).
    //    세대 오름차순으로 읽으면서, generation 이하면 계속 덮어쓰고(→ 가장 최근), 아직 아무것도
    //    없으면 처음 것을 쥔다(→ generation보다 뒤 세대 중 가장 이른 것).
    //    기술머신은 아이템 문구 대신 담긴 기술의 문구를 같은 규칙으로 고른다. 아이템 문구를 다른
    //    세대에서 빌려 오면 기술과 설명이 어긋나지만(4세대 기술머신01 = 힘껏펀치인데 6세대 한국어
    //    문구는 손톱갈기), 기술 문구는 세대가 달라도 같은 기술을 설명한다.
    struct Taken
    {
        std::array<int, 3> from {}; // 언어별로 문구를 가져온 세대(0 = 아직 없음)
    };
    const auto pick = [generation](LocalizedText &effect, Taken &t, const QSqlQuery &q) {
        const int g = q.value(1).toInt();
        QString *byLanguage[3] = {&effect.ko, &effect.en, &effect.ja};
        for (int language = 0; language < 3; ++language) {
            const QString text = q.value(2 + language).toString();
            if (text.isEmpty())
                continue;
            const int from = t.from[language];
            if (from == 0 || (g <= generation && from <= generation)) {
                *byLanguage[language] = text;
                t.from[language] = g;
            }
        }
    };
    QHash<int, Taken> taken; // item id →
    if (query.exec(QStringLiteral("SELECT item_id, generation, text_ko, text_en, text_ja "
                                  "FROM item_effects ORDER BY generation"))) {
        while (query.next()) {
            const int id = query.value(0).toInt();
            const auto it = rowOfItem.constFind(id);
            if (it == rowOfItem.constEnd() || moveOfItem.contains(id))
                continue;
            pick(rows[*it].effect, taken[id], query);
        }
    }
    for (ItemRow &row : rows) // 어느 세대에도 한국어 문구가 없는 아이템(9세대 신규 등)
        if (!moveOfItem.contains(row.id))
            namebook::fill(namebook::Kind::ItemEffect, row.identifier, row.effect);
    if (!moveOfItem.isEmpty()) {
        QHash<int, QList<qsizetype>> rowsOfMove; // move id → 그 기술이 담긴 기술머신 행들
        for (auto it = moveOfItem.cbegin(); it != moveOfItem.cend(); ++it)
            rowsOfMove[it.value()].append(rowOfItem.value(it.key()));
        QHash<int, Taken> takenOfMove;
        if (query.exec(QStringLiteral("SELECT move_id, generation, text_ko, text_en, text_ja "
                                      "FROM move_effects ORDER BY generation"))) {
            while (query.next()) {
                const int moveId = query.value(0).toInt();
                const auto it = rowsOfMove.constFind(moveId);
                if (it == rowsOfMove.constEnd())
                    continue;
                LocalizedText effect = rows[it->first()].effect;
                pick(effect, takenOfMove[moveId], query);
                for (const qsizetype row : *it)
                    rows[row].effect = effect;
            }
        }
    }
    // 게임을 골랐으면 그 게임의 기술머신 표에 없는 기술머신 · 비전머신은 뺀다(레츠고는 60개뿐인데
    // 세대 목록에는 썬문의 기술머신100까지 있다)
    if (!versionGroup.isEmpty())
        rows.removeIf([&moveOfItem](const ItemRow &row) {
            return row.pocket == QLatin1String("machines") && !moveOfItem.contains(row.id);
        });
    return rows;
}

QString Repository::representativeVersionGroup(int generation)
{
    // 세대마다 기술 · 기술머신 번호의 기준 게임. 그 세대를 가장 넓게 담는 셋째 판 · 확장판을 고른다
    // (4세대 = 플래티넘: DP에 없던 기술 가르침이 있고, 기술머신 획득처 사전도 Pt 기준이다).
    // 리메이크(FRLG · HGSS · ORAS · BDSP)는 그 세대의 대표로 쓰지 않는다.
    static constexpr const char *kGroups[] = {"yellow",
                                              "crystal",
                                              "emerald",
                                              "platinum",
                                              "black-2-white-2",
                                              "x-y",
                                              "ultra-sun-ultra-moon",
                                              "sword-shield",
                                              "scarlet-violet"};
    if (generation < 1 || generation > int(std::size(kGroups)))
        return {};
    return QString::fromLatin1(kGroups[generation - 1]);
}

TypeChart Repository::typeChart(int generation)
{
    TypeChart chart;
    if (!open())
        return chart;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral("SELECT identifier FROM types WHERE intro_gen <= :g ORDER BY id"));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec())
        while (query.next())
            chart.types.append(query.value(0).toString());
    query.prepare(
            QStringLiteral("SELECT a.identifier, d.identifier, c.multiplier FROM type_chart c "
                           "JOIN types a ON a.id = c.atk_type JOIN types d ON d.id = c.def_type "
                           "WHERE c.gen_from <= :g AND (c.gen_to IS NULL OR c.gen_to >= :g)"));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec())
        while (query.next())
            chart.multipliers.insert(query.value(0).toString() + QLatin1Char('/')
                                             + query.value(1).toString(),
                                     query.value(2).toDouble());
    return chart;
}

PokemonDetail Repository::pokemonDetail(int pokemonId, int generation, const QString &versionGroup)
{
    // 게임 데이터는 읽기 전용이라 한 번 읽은 상세는 그대로 쓴다(스쿼드 HG ↔ SS · 도감 상세 오가기)
    const QString key = QStringLiteral("%1/%2/%3").arg(pokemonId).arg(generation).arg(versionGroup);
    const auto cached = m_details.constFind(key);
    if (cached != m_details.constEnd())
        return *cached;
    PokemonDetail detail = readPokemonDetail(pokemonId, generation, versionGroup);
    if (detail.isValid()) {
        if (m_details.size() >= kDetailCacheSize)
            m_details.clear(); // 드물다 — 오래된 것만 골라 버릴 만큼 크지 않다
        m_details.insert(key, detail);
    }
    return detail;
}

PokemonDetail Repository::readPokemonDetail(int pokemonId, int generation,
                                            const QString &versionGroup)
{
    PokemonDetail detail;
    if (!open())
        return detail;
    QSqlQuery query(QSqlDatabase::database(m_connection));

    // 1) 이름 · 분류 · 키 · 몸무게
    query.prepare(QStringLiteral(
            "SELECT s.id, s.name_ko, s.name_en, s.name_ja, s.genus_ko, s.genus_en, s.genus_ja, "
            "p.height, p.weight FROM pokemon p JOIN species s ON s.id = p.species_id "
            "WHERE p.id = :p"));
    query.bindValue(QStringLiteral(":p"), pokemonId);
    if (!query.exec() || !query.next())
        return detail;
    detail.pokemonId = pokemonId;
    detail.speciesId = query.value(0).toInt();
    detail.name = localized(query, 1);
    detail.genus = localized(query, 4);
    detail.height = query.value(7).toInt();
    detail.weight = query.value(8).toInt();
    detail.generation = generation;

    // 2) 타입 · 종족값: 목록과 같은 함수로(1세대 특수 규칙도 같이)
    QList<SpeciesRow> one(1);
    one[0].pokemonId = pokemonId;
    fillTypesAndStats(one, generation);
    detail.types = one[0].types;
    detail.stats = one[0].stats;
    detail.total = one[0].total;

    // 3) 기술 기준 게임 묶음: 고른 도감의 게임(그 세대 것일 때만), 아니면 세대의 대표 게임
    int groupId = 0;
    for (const QString &candidate : {versionGroup, representativeVersionGroup(generation)}) {
        if (candidate.isEmpty())
            continue;
        query.prepare(QStringLiteral(
                "SELECT id FROM version_groups WHERE identifier = :vg AND generation = :g"));
        query.bindValue(QStringLiteral(":vg"), candidate);
        query.bindValue(QStringLiteral(":g"), generation);
        if (query.exec() && query.next()) {
            groupId = query.value(0).toInt();
            detail.versionGroup = candidate;
            break;
        }
    }
    query.prepare(QStringLiteral("SELECT identifier, name_ko, name_en, name_ja FROM versions WHERE "
                                 "version_group_id = :id ORDER BY id"));
    query.bindValue(QStringLiteral(":id"), groupId);
    if (query.exec()) {
        while (query.next()) {
            detail.groupVersions.append(query.value(0).toString());
            detail.groupGames.append(localized(query, 1));
            namebook::fill(namebook::Kind::Version, detail.groupVersions.last(),
                           detail.groupGames.last());
        }
    }

    // 4) 습득 기술: 레벨업(1) · 기술머신(4, 번호 · 아이템과 함께)
    query.prepare(QStringLiteral(
            "SELECT move_id, level FROM pokemon_moves WHERE pokemon_id = :p "
            "AND version_group_id = :vg AND method = 1 ORDER BY level, sort_order, move_id"));
    query.bindValue(QStringLiteral(":p"), pokemonId);
    query.bindValue(QStringLiteral(":vg"), groupId);
    if (query.exec()) {
        while (query.next()) {
            MoveEntry move;
            move.moveId = query.value(0).toInt();
            move.level = query.value(1).toInt();
            detail.levelMoves.append(move);
        }
    }
    query.prepare(QStringLiteral(
            "SELECT pm.move_id, m.machine_number, i.identifier FROM pokemon_moves pm "
            "JOIN machines m ON m.version_group_id = pm.version_group_id AND m.move_id = "
            "pm.move_id "
            "JOIN items i ON i.id = m.item_id "
            "WHERE pm.pokemon_id = :p AND pm.version_group_id = :vg AND pm.method = 4"));
    query.bindValue(QStringLiteral(":p"), pokemonId);
    query.bindValue(QStringLiteral(":vg"), groupId);
    if (query.exec()) {
        while (query.next()) {
            MoveEntry move;
            move.moveId = query.value(0).toInt();
            move.machineNumber = query.value(1).toInt();
            move.machineItem = query.value(2).toString();
            move.hiddenMachine = move.machineItem.startsWith(QLatin1String("hm"));
            if (move.hiddenMachine && move.machineNumber > kHiddenMachineOffset)
                move.machineNumber -= kHiddenMachineOffset; // PokéAPI는 비전머신을 101–108로 센다
            detail.machineMoves.append(move);
        }
    }
    // 기술머신 → 비전머신 순, 번호 순(기술레코드 등 다른 머신은 기술머신 뒤에 이름순으로 섞이지
    // 않게 아이템 이름까지)
    std::sort(detail.machineMoves.begin(), detail.machineMoves.end(),
              [](const MoveEntry &a, const MoveEntry &b) {
                  if (a.hiddenMachine != b.hiddenMachine)
                      return !a.hiddenMachine;
                  if (a.machineItem.left(2) != b.machineItem.left(2))
                      return a.machineItem < b.machineItem;
                  return a.machineNumber < b.machineNumber;
              });
    // 기술 가르침(3) · 알 기술(2)
    for (const auto &[method, list] :
         {std::pair(3, &detail.tutorMoves), std::pair(2, &detail.eggMoves)}) {
        query.prepare(
                QStringLiteral("SELECT DISTINCT move_id FROM pokemon_moves WHERE pokemon_id = :p "
                               "AND version_group_id = :vg AND method = :m ORDER BY move_id"));
        query.bindValue(QStringLiteral(":p"), pokemonId);
        query.bindValue(QStringLiteral(":vg"), groupId);
        query.bindValue(QStringLiteral(":m"), method);
        if (query.exec()) {
            while (query.next()) {
                MoveEntry move;
                move.moveId = query.value(0).toInt();
                list->append(move);
            }
        }
    }
    fillMoves(detail.levelMoves, generation);
    fillMoves(detail.machineMoves, generation);
    fillMoves(detail.tutorMoves, generation);
    fillMoves(detail.eggMoves, generation);
    fillEvolution(detail, groupId);
    fillAbilities(detail);

    // 5) 야생 출현: 그 세대의 모든 버전(DP · Pt · HGSS)
    query.prepare(QStringLiteral(
            "SELECT v.identifier, v.name_ko, v.name_en, v.name_ja, l.identifier, l.name_ko, "
            "l.name_en, l.name_ja, em.identifier, e.min_level, e.max_level, e.rarity "
            "FROM encounters e JOIN versions v ON v.id = e.version_id "
            "JOIN version_groups vg ON vg.id = v.version_group_id "
            "JOIN locations l ON l.id = e.location_id "
            "JOIN encounter_methods em ON em.id = e.method_id "
            "WHERE e.pokemon_id = :p AND vg.generation = :g "
            "ORDER BY v.id, l.id, em.id"));
    query.bindValue(QStringLiteral(":p"), pokemonId);
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            EncounterEntry e;
            e.version = query.value(0).toString();
            e.versionName = localized(query, 1);
            namebook::fill(namebook::Kind::Version, e.version, e.versionName);
            e.location = query.value(4).toString();
            e.locationName = localized(query, 5);
            e.method = query.value(8).toString();
            e.minLevel = query.value(9).toInt();
            e.maxLevel = query.value(10).toInt();
            e.rarity = query.value(11).toInt();
            detail.encounters.append(e);
        }
    }
    return detail;
}

void Repository::fillEvolution(PokemonDetail &detail, int versionGroupId)
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    const int generation = detail.generation;

    // 1) 같은 진화 사슬의 종(그 세대에 있는 것만)
    int chain = 0;
    query.prepare(QStringLiteral("SELECT evolution_chain FROM species WHERE id = :s"));
    query.bindValue(QStringLiteral(":s"), detail.speciesId);
    if (query.exec() && query.next())
        chain = query.value(0).toInt();
    QHash<int, EvolutionStep> steps;
    query.prepare(QStringLiteral(
            "SELECT s.id, s.evolves_from, s.name_ko, s.name_en, s.name_ja, p.id FROM species s "
            "JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1 "
            "WHERE s.evolution_chain = :c AND s.intro_gen <= :g ORDER BY s.id"));
    query.bindValue(QStringLiteral(":c"), chain);
    query.bindValue(QStringLiteral(":g"), generation);
    QList<int> order;
    if (query.exec()) {
        while (query.next()) {
            EvolutionStep step;
            step.speciesId = query.value(0).toInt();
            step.fromSpeciesId = query.value(1).toInt();
            step.name = localized(query, 2);
            step.pokemonId = query.value(5).toInt();
            steps.insert(step.speciesId, step);
            order.append(step.speciesId);
        }
    }
    // 진화 전 종이 이 세대에 없으면(1세대의 피카츄: 피츄는 2세대부터) 그 종이 뿌리다
    for (EvolutionStep &step : steps)
        if (!steps.contains(step.fromSpeciesId))
            step.fromSpeciesId = 0;

    // 2) 하트비늘: 이 게임에서 잡기 · 받기 · 진화로 얻는 길을 따라 자연히 익히는 기술과 견준다
    QList<int> ancestors; // 기본 모습 pokemon id(가까운 단계부터)
    QList<int> ancestorSpecies;
    for (int from = steps.value(detail.speciesId).fromSpeciesId; from != 0;
         from = steps.value(from).fromSpeciesId) {
        ancestors.append(steps.value(from).pokemonId);
        ancestorSpecies.append(from);
    }
    std::vector<movereach::Stage> path; // 뿌리 → 이 포켓몬
    for (qsizetype i = ancestors.size(); i >= 0; --i) {
        const int pokemonId = i > 0 ? ancestors.at(i - 1) : detail.pokemonId;
        const int speciesId = i > 0 ? ancestorSpecies.at(i - 1) : detail.speciesId;
        movereach::Stage stage;
        query.prepare(QStringLiteral(
                "SELECT move_id, level, sort_order FROM pokemon_moves WHERE pokemon_id = :p "
                "AND version_group_id = :vg AND method = 1"));
        query.bindValue(QStringLiteral(":p"), pokemonId);
        query.bindValue(QStringLiteral(":vg"), versionGroupId);
        if (query.exec())
            while (query.next())
                stage.learnset.push_back(
                        {query.value(0).toInt(), query.value(1).toInt(), query.value(2).toInt()});
        query.prepare(QStringLiteral("SELECT MIN(e.min_level) FROM encounters e "
                                     "JOIN versions v ON v.id = e.version_id "
                                     "WHERE e.pokemon_id = :p AND v.version_group_id = :vg"));
        query.bindValue(QStringLiteral(":p"), pokemonId);
        query.bindValue(QStringLiteral(":vg"), versionGroupId);
        if (query.exec() && query.next() && !query.value(0).isNull())
            stage.caughtLevel = query.value(0).toInt();
        if (!path.empty()) {
            // 그 세대 이하에서 가장 최근 세대의 진화 방법 중 가장 낮은 레벨(레벨 조건 없음 = 0)
            query.prepare(QStringLiteral(
                    "SELECT MIN(COALESCE(min_level, 0)) FROM evolutions "
                    "WHERE evolved_species_id = :s AND generation = (SELECT MAX(generation) "
                    "FROM evolutions WHERE evolved_species_id = :s2 AND generation <= :g)"));
            query.bindValue(QStringLiteral(":s"), speciesId);
            query.bindValue(QStringLiteral(":s2"), speciesId);
            query.bindValue(QStringLiteral(":g"), generation);
            if (query.exec() && query.next())
                stage.evolveLevel = query.value(0).toInt();
        }
        path.push_back(std::move(stage));
    }
    const movereach::Reach reach = movereach::reachableMoves(path);
    if (reach.obtainable) {
        detail.earliestLevel = reach.earliestLevel;
        for (MoveEntry &move : detail.levelMoves)
            move.needsReminder = !reach.moves.contains(move.moveId);
    } else if (!ancestors.isEmpty()) {
        // 이 게임의 출현 자료로는 얻을 수 없다(다른 게임에서 데려오기 등): 진화 전 단계가
        // 레벨업으로 배우지 않는 Lv 1 기술만
        QSet<int> inherited; // 진화 전 단계가 레벨업으로 배우는 기술
        for (const int pokemonId : std::as_const(ancestors)) {
            query.prepare(QStringLiteral("SELECT move_id FROM pokemon_moves WHERE pokemon_id = :p "
                                         "AND version_group_id = :vg AND method = 1"));
            query.bindValue(QStringLiteral(":p"), pokemonId);
            query.bindValue(QStringLiteral(":vg"), versionGroupId);
            if (query.exec())
                while (query.next())
                    inherited.insert(query.value(0).toInt());
        }
        QSet<int> laterLevels; // 이 포켓몬이 Lv 2 이상에서도 배우는 기술
        for (const MoveEntry &move : std::as_const(detail.levelMoves))
            if (move.level > 1)
                laterLevels.insert(move.moveId);
        for (MoveEntry &move : detail.levelMoves)
            move.needsReminder = move.level <= 1 && !inherited.contains(move.moveId)
                                 && !laterLevels.contains(move.moveId);
    }

    if (steps.size() < 2)
        return; // 진화하지 않는 포켓몬

    // 3) 진화 방법: 종마다 "그 세대 이하에서 방법이 있는 가장 최근 세대"의 줄만
    QStringList ids;
    for (const int id : std::as_const(order))
        ids.append(QString::number(id));
    QHash<int, int> latest; // 종 → 쓸 세대
    QList<QList<QVariant>> rows;
    // id 목록은 DB에서 읽은 정수라 SQL에 이어 붙여도 안전하다(사용자 입력이 아니다)
    if (query.exec(
                QStringLiteral(
                        "SELECT evolved_species_id, generation, trigger, item_id, min_level, "
                        "gender, "
                        "location_id, held_item_id, time_of_day, known_move_id, "
                        "known_move_type_id, "
                        "min_happiness, min_beauty, min_affection, relative_stats, "
                        "party_species_id, "
                        "party_type_id, trade_species_id, needs_rain, upside_down FROM evolutions "
                        "WHERE generation <= %1 AND evolved_species_id IN (%2)")
                        .arg(generation)
                        .arg(ids.join(QLatin1Char(','))))) {
        while (query.next()) {
            QList<QVariant> row;
            for (int c = 0; c < 20; ++c)
                row.append(query.value(c));
            const int species = row[0].toInt();
            latest[species] = std::max(latest.value(species), row[1].toInt());
            rows.append(row);
        }
    }
    // 이름 찾기(도구 · 기술 · 장소 · 종 · 타입). 줄이 몇 개뿐이라 그때그때 묻는다.
    auto nameOf = [&](const char *table, const QVariant &id) {
        if (id.isNull())
            return LocalizedText();
        QSqlQuery q(QSqlDatabase::database(m_connection));
        q.prepare(QStringLiteral("SELECT name_ko, name_en, name_ja FROM %1 WHERE id = :id")
                          .arg(QLatin1String(table)));
        q.bindValue(QStringLiteral(":id"), id);
        return q.exec() && q.next() ? localized(q, 0) : LocalizedText();
    };
    auto identifierOf = [&](const char *table, const QVariant &id) {
        if (id.isNull())
            return QString();
        QSqlQuery q(QSqlDatabase::database(m_connection));
        q.prepare(QStringLiteral("SELECT identifier FROM %1 WHERE id = :id")
                          .arg(QLatin1String(table)));
        q.bindValue(QStringLiteral(":id"), id);
        return q.exec() && q.next() ? q.value(0).toString() : QString();
    };
    for (const QList<QVariant> &row : std::as_const(rows)) {
        const int species = row[0].toInt();
        if (row[1].toInt() != latest.value(species) || !steps.contains(species))
            continue;
        EvolutionCondition c;
        c.trigger = row[2].toInt();
        c.item = nameOf("items", row[3]);
        c.itemIdentifier = identifierOf("items", row[3]);
        c.minLevel = row[4].toInt();
        c.gender = row[5].toInt();
        c.location = identifierOf("locations", row[6]);
        c.locationName = nameOf("locations", row[6]);
        c.heldItem = nameOf("items", row[7]);
        c.heldItemIdentifier = identifierOf("items", row[7]);
        c.timeOfDay = row[8].toString();
        c.knownMove = nameOf("moves", row[9]);
        c.knownMoveType = identifierOf("types", row[10]);
        c.minHappiness = row[11].toInt();
        c.minBeauty = row[12].toInt();
        c.minAffection = row[13].toInt();
        if (!row[14].isNull())
            c.relativeStats = row[14].toInt();
        c.partySpecies = nameOf("species", row[15]);
        c.partyType = identifierOf("types", row[16]);
        c.tradeSpecies = nameOf("species", row[17]);
        c.needsRain = row[18].toInt() != 0;
        c.upsideDown = row[19].toInt() != 0;
        steps[species].conditions.append(c);
    }

    // 4) 뿌리부터 깊이 우선(형제는 종 번호 순): 랄토스 → 킬리아 → 가디안 · 엘레이드
    QHash<int, QList<int>> children;
    QList<int> roots;
    for (const int id : std::as_const(order)) {
        const int from = steps.value(id).fromSpeciesId;
        (from == 0 ? roots : children[from]).append(id);
    }
    std::function<void(int, int)> visit = [&](int id, int depth) {
        EvolutionStep step = steps.value(id);
        step.depth = depth;
        detail.evolution.append(step);
        for (const int child : children.value(id))
            visit(child, depth + 1);
    };
    for (const int root : std::as_const(roots))
        visit(root, 0);
}

void Repository::fillAbilities(PokemonDetail &detail)
{
    QSqlQuery query(QSqlDatabase::database(m_connection));
    const int generation = detail.generation;
    query.prepare(
            QStringLiteral("SELECT pa.slot, pa.is_hidden, a.id, a.name_ko, a.name_en, a.name_ja, "
                           "a.identifier "
                           "FROM pokemon_abilities pa JOIN abilities a ON a.id = pa.ability_id "
                           "WHERE pa.pokemon_id = :p AND pa.gen_from <= :g AND (pa.gen_to IS NULL "
                           "OR pa.gen_to >= :g) "
                           "ORDER BY pa.slot"));
    query.bindValue(QStringLiteral(":p"), detail.pokemonId);
    query.bindValue(QStringLiteral(":g"), generation);
    QHash<int, qsizetype> indexOf; // 특성 id → 위치
    if (query.exec()) {
        while (query.next()) {
            AbilityEntry ability;
            ability.slot = query.value(0).toInt();
            ability.hidden = query.value(1).toInt() != 0;
            ability.abilityId = query.value(2).toInt();
            ability.name = localized(query, 3);
            ability.identifier = query.value(6).toString();
            namebook::fill(namebook::Kind::Ability, ability.identifier, ability.name);
            indexOf.insert(ability.abilityId, detail.abilities.size());
            detail.abilities.append(ability);
        }
    }
    if (detail.abilities.isEmpty())
        return;

    // 효과 문구: 아이템과 같은 규칙 — 언어마다 generation 이하에서 가장 최근, 없으면 가장 이른 세대
    QStringList ids;
    for (const AbilityEntry &ability : std::as_const(detail.abilities))
        ids.append(QString::number(ability.abilityId));
    QHash<int, std::array<int, 3>> takenFrom;
    // id 목록은 DB에서 읽은 정수라 SQL에 이어 붙여도 안전하다
    if (query.exec(QStringLiteral("SELECT ability_id, generation, text_ko, text_en, text_ja FROM "
                                  "ability_effects WHERE ability_id IN (%1) ORDER BY generation")
                           .arg(ids.join(QLatin1Char(','))))) {
        while (query.next()) {
            const int id = query.value(0).toInt();
            const int g = query.value(1).toInt();
            LocalizedText &effect = detail.abilities[indexOf.value(id)].effect;
            QString *byLanguage[3] = {&effect.ko, &effect.en, &effect.ja};
            std::array<int, 3> &from = takenFrom[id];
            for (int language = 0; language < 3; ++language) {
                const QString text = query.value(2 + language).toString();
                if (text.isEmpty())
                    continue;
                if (from[language] == 0 || (g <= generation && from[language] <= generation)) {
                    *byLanguage[language] = text;
                    from[language] = g;
                }
            }
        }
    }
    // 같은 특성이 두 칸에 있으면(1 · 2번 칸이 같은 포켓몬) 한 번만
    QList<AbilityEntry> unique;
    QSet<int> seen;
    for (const AbilityEntry &ability : std::as_const(detail.abilities))
        if (!seen.contains(ability.abilityId) || ability.hidden) {
            seen.insert(ability.abilityId);
            unique.append(ability);
        }
    detail.abilities = unique;
    for (AbilityEntry &ability : detail.abilities) // 한국어 설명이 없는 특성(9세대 신규 등)
        namebook::fill(namebook::Kind::AbilityEffect, ability.identifier, ability.effect);
}

QList<Nature> Repository::natures()
{
    QList<Nature> natures;
    if (!open())
        return natures;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (query.exec(QStringLiteral("SELECT id, name_ko, name_en, name_ja, increased_stat, "
                                  "decreased_stat FROM natures ORDER BY id"))) {
        while (query.next()) {
            Nature nature;
            nature.id = query.value(0).toInt();
            nature.name = localized(query, 1);
            nature.increasedStat = query.value(4).toInt();
            nature.decreasedStat = query.value(5).toInt();
            natures.append(nature);
        }
    }
    return natures;
}

QHash<int, int> Repository::evolvesFrom()
{
    QHash<int, int> parents;
    if (!open())
        return parents;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    if (query.exec(QStringLiteral(
                "SELECT id, evolves_from FROM species WHERE evolves_from IS NOT NULL")))
        while (query.next())
            parents.insert(query.value(0).toInt(), query.value(1).toInt());
    return parents;
}

QList<GameInfo> Repository::gamesForGeneration(int generation)
{
    QList<GameInfo> games;
    if (!open())
        return games;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(
            QStringLiteral("SELECT vg.identifier, v.identifier, v.name_ko, v.name_en, v.name_ja "
                           "FROM version_groups vg "
                           "JOIN versions v ON v.version_group_id = vg.id WHERE vg.generation = :g "
                           "AND EXISTS (SELECT 1 FROM pokedex_version_groups pvg WHERE "
                           "pvg.version_group_id = vg.id) "
                           "ORDER BY vg.sort_order, v.id"));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            const QString group = query.value(0).toString();
            if (games.isEmpty() || games.last().versionGroup != group)
                games.append(GameInfo {group, {}, {}});
            games.last().versions.append(query.value(1).toString());
            games.last().versionNames.append(localized(query, 2));
            namebook::fill(namebook::Kind::Version, games.last().versions.last(),
                           games.last().versionNames.last());
        }
    }
    return games;
}

QString Repository::resolveVersion(int generation, const QString &version)
{
    const QList<GameInfo> games = gamesForGeneration(generation);
    if (games.isEmpty())
        return {};
    for (const GameInfo &game : games) {
        if (game.versions.contains(version))
            return version;
        if (game.versionGroup == version)
            return game.versions.value(0);
    }
    const QString representative = representativeVersionGroup(generation);
    for (const GameInfo &game : games)
        if (game.versionGroup == representative)
            return game.versions.value(0);
    return games.first().versions.value(0);
}

QSet<int> Repository::otherVersionSpecies(const QString &version)
{
    const auto cached = m_otherVersionSpecies.constFind(version);
    if (cached != m_otherVersionSpecies.constEnd())
        return *cached;
    QSet<int> species;
    if (!open())
        return species;
    QSqlQuery query(QSqlDatabase::database(m_connection));
    // 버전마다 출현 자료가 있는 진화 사슬
    auto chainsOf = [&](const QString &where) {
        QSet<int> chains;
        query.prepare(QStringLiteral("SELECT DISTINCT s.evolution_chain FROM encounters e "
                                     "JOIN versions v ON v.id = e.version_id "
                                     "JOIN pokemon p ON p.id = e.pokemon_id "
                                     "JOIN species s ON s.id = p.species_id WHERE %1")
                              .arg(where));
        query.bindValue(QStringLiteral(":v"), version);
        if (where.contains(QLatin1String(":v2")))
            query.bindValue(QStringLiteral(":v2"), version);
        if (query.exec())
            while (query.next())
                chains.insert(query.value(0).toInt());
        return chains;
    };
    const QSet<int> mine = chainsOf(QStringLiteral("v.identifier = :v"));
    const QSet<int> siblings = chainsOf(
            QStringLiteral("v.identifier <> :v AND v.version_group_id = "
                           "(SELECT version_group_id FROM versions WHERE identifier = :v2)"));
    const QSet<int> elsewhere = QSet<int>(siblings).subtract(mine);
    if (!elsewhere.isEmpty()) {
        QStringList ids;
        for (const int chain : elsewhere)
            ids.append(QString::number(chain));
        // 사슬 id는 DB에서 읽은 정수라 SQL에 이어 붙여도 안전하다
        if (query.exec(QStringLiteral("SELECT id FROM species WHERE evolution_chain IN (%1)")
                               .arg(ids.join(QLatin1Char(',')))))
            while (query.next())
                species.insert(query.value(0).toInt());
    }
    m_otherVersionSpecies.insert(version, species);
    return species;
}

QList<MoveEntry> Repository::moves(const QList<int> &ids, int generation)
{
    QList<MoveEntry> moves;
    if (ids.isEmpty() || !open())
        return moves;
    for (const int id : ids) {
        MoveEntry move;
        move.moveId = id;
        moves.append(move);
    }
    fillMoves(moves, generation);
    // fillMoves는 없는 id도 그대로 둔다(이름이 빈 채로) → 뺀다
    moves.removeIf([](const MoveEntry &move) { return move.name.isEmpty(); });
    return moves;
}

void Repository::fillMoves(QList<MoveEntry> &moves, int generation)
{
    if (moves.isEmpty())
        return;
    QSqlQuery query(QSqlDatabase::database(m_connection));

    // 필요한 기술만 읽는다(표 전체를 훑으면 상세 한 번에 수천 줄 — 스쿼드 게임을 바꿀 때 버벅였다).
    // id는 MoveEntry의 정수라 SQL에 이어 붙여도 안전하다
    QSet<int> wanted;
    for (const MoveEntry &move : moves)
        wanted.insert(move.moveId);
    QStringList idList;
    for (const int id : std::as_const(wanted))
        idList.append(QString::number(id));
    const QString ids = idList.join(QLatin1Char(','));

    // 지금 값 + 그 세대 타입(구간)
    QHash<int, MoveEntry> info;
    query.prepare(
            QStringLiteral("SELECT m.id, m.name_ko, m.name_en, m.name_ja, t.identifier, m.power, "
                           "m.pp, m.accuracy, "
                           "m.damage_class, m.identifier, mm.target, mm.ailment, mm.healing "
                           "FROM moves m LEFT JOIN move_meta mm ON mm.move_id = m.id "
                           "LEFT JOIN move_types mt ON mt.move_id = m.id AND mt.gen_from <= :g "
                           "  AND (mt.gen_to IS NULL OR mt.gen_to >= :g) "
                           "LEFT JOIN types t ON t.id = mt.type_id WHERE m.id IN (%1)")
                    .arg(ids));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            const int id = query.value(0).toInt();
            if (!wanted.contains(id))
                continue;
            MoveEntry m;
            m.name = localized(query, 1);
            m.type = query.value(4).toString();
            m.power = query.value(5).toInt();
            m.pp = query.value(6).toInt();
            m.accuracy = query.value(7).toInt();
            m.damageClass = query.value(8).toInt();
            m.identifier = query.value(9).toString();
            m.target = query.value(10).toInt();
            m.ailment = query.value(11).toInt();
            m.healing = query.value(12).toInt();
            info.insert(id, m);
        }
    }

    // 옛 값: until_gen이 generation 이상인 줄 중 가장 이른 것이 그 세대의 값이다(항목마다 따로).
    struct Past
    {
        int until = 0;
        QVariant power, pp, accuracy;
    };
    QHash<int, QList<Past>> past;
    if (query.exec(QStringLiteral("SELECT move_id, until_gen, power, pp, accuracy FROM "
                                  "move_changelog WHERE move_id IN (%1) ORDER BY until_gen")
                           .arg(ids))) {
        while (query.next())
            past[query.value(0).toInt()].append(
                    {query.value(1).toInt(), query.value(2), query.value(3), query.value(4)});
    }

    // 능력치 변화 · 그 세대 설명문(언어마다: generation 이하 중 가장 최근, 없으면 이후 중 가장 이른
    // 것)
    if (query.exec(QStringLiteral("SELECT move_id, stat_id, change FROM move_stat_changes "
                                  "WHERE move_id IN (%1) ORDER BY move_id, stat_id")
                           .arg(ids))) {
        while (query.next()) {
            const auto it = info.find(query.value(0).toInt());
            if (it != info.end())
                it->statChanges.append({query.value(1).toInt(), query.value(2).toInt()});
        }
    }
    QHash<int, std::array<int, 3>> effectFrom; // move id → 언어별로 문구를 가져온 세대(0 = 없음)
    if (query.exec(QStringLiteral("SELECT move_id, generation, text_ko, text_en, text_ja FROM "
                                  "move_effects WHERE move_id IN (%1) ORDER BY generation")
                           .arg(ids))) {
        while (query.next()) {
            const auto it = info.find(query.value(0).toInt());
            if (it == info.end())
                continue;
            const int g = query.value(1).toInt();
            std::array<int, 3> &from = effectFrom[it.key()];
            QString *byLanguage[3] = {&it->effect.ko, &it->effect.en, &it->effect.ja};
            for (int language = 0; language < 3; ++language) {
                const QString text = query.value(2 + language).toString();
                if (text.isEmpty())
                    continue;
                if (from[language] == 0 || (g <= generation && from[language] <= generation)) {
                    *byLanguage[language] = text;
                    from[language] = g;
                }
            }
        }
    }

    for (MoveEntry &move : moves) {
        const MoveEntry base = info.value(move.moveId);
        move.name = base.name;
        move.type = base.type;
        move.power = base.power;
        move.pp = base.pp;
        move.accuracy = base.accuracy;
        move.damageClass = base.damageClass;
        move.ownDamageClass = base.damageClass;
        move.identifier = base.identifier;
        move.target = base.target;
        move.ailment = base.ailment;
        move.healing = base.healing;
        move.statChanges = base.statChanges;
        move.effect = base.effect;
        bool powerSet = false, ppSet = false, accuracySet = false;
        for (const Past &p : past.value(move.moveId)) {
            if (p.until < generation)
                continue;
            if (!powerSet && !p.power.isNull())
                move.power = p.power.toInt(), powerSet = true;
            if (!ppSet && !p.pp.isNull())
                move.pp = p.pp.toInt(), ppSet = true;
            if (!accuracySet && !p.accuracy.isNull())
                move.accuracy = p.accuracy.toInt(), accuracySet = true;
        }
        // 3세대까지는 물리 · 특수를 타입이 정했다(변화 기술은 그대로)
        if (generation <= kLastTypeBasedDamageClassGeneration && move.damageClass != kStatusClass)
            move.damageClass = kPhysicalTypesBeforeSplit.contains(move.type) ? kPhysicalClass
                                                                             : kSpecialClass;
        // PokéAPI에 한국어가 없는 이름 · 설명(다크 기술 · 9세대 신규 등)은 사전으로 채운다
        namebook::fill(namebook::Kind::Move, move.identifier, move.name);
        namebook::fill(namebook::Kind::MoveEffect, move.identifier, move.effect);
    }
}

bool Repository::readSpecies(QSqlQuery &query, QList<SpeciesRow> &rows)
{
    // 열 순서: 종 id · 기본 모습 pokemon id · 이름 ko/en/ja · 도감 번호 · 전설 · 최종 진화
    if (!query.exec()) {
        m_error = query.lastError().text();
        qCWarning(lcData) << "species query failed:" << m_error;
        return false;
    }
    while (query.next()) {
        SpeciesRow row;
        row.speciesId = query.value(0).toInt();
        row.pokemonId = query.value(1).toInt();
        row.name = localized(query, 2);
        row.dexNumber = query.value(5).toInt();
        row.legendary = query.value(6).toBool();
        row.finalEvolution = query.value(7).toBool();
        rows.append(row);
    }
    return true;
}

void Repository::fillTypesAndStats(QList<SpeciesRow> &rows, int generation)
{
    QHash<int, qsizetype> rowOfPokemon; // pokemon id → rows의 위치
    for (qsizetype i = 0; i < rows.size(); ++i)
        rowOfPokemon.insert(rows[i].pokemonId, i);
    QSqlQuery query(QSqlDatabase::database(m_connection));
    // 몇 마리뿐이면(상세 · 스쿼드) 그 포켓몬만 읽는다(인덱스). 목록이면 표 전체를 한 번에
    QString only;
    if (rows.size() <= 64) {
        QStringList ids;
        for (const SpeciesRow &row : std::as_const(rows))
            ids.append(QString::number(row.pokemonId));
        only = QStringLiteral(" AND pokemon_id IN (%1)").arg(ids.join(QLatin1Char(',')));
    }

    // 1) 그 세대의 타입(슬롯 순). 표 전체를 한 번에 읽고 나눠 담는다 — 종마다 질의하면 수백 번이
    // 된다.
    query.prepare(
            QStringLiteral("SELECT pt.pokemon_id, t.identifier FROM pokemon_types pt "
                           "JOIN types t ON t.id = pt.type_id "
                           "WHERE pt.gen_from <= :g AND (pt.gen_to IS NULL OR pt.gen_to >= :g)%1 "
                           "ORDER BY pt.pokemon_id, pt.slot")
                    .arg(only));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            const auto it = rowOfPokemon.constFind(query.value(0).toInt());
            if (it != rowOfPokemon.constEnd())
                rows[*it].types.append(query.value(1).toString());
        }
    }

    // 2) 그 세대의 종족값
    query.prepare(QStringLiteral("SELECT pokemon_id, stat_id, value FROM pokemon_stats "
                                 "WHERE gen_from <= :g AND (gen_to IS NULL OR gen_to >= :g)%1")
                          .arg(only));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            const auto it = rowOfPokemon.constFind(query.value(0).toInt());
            if (it == rowOfPokemon.constEnd())
                continue;
            SpeciesRow &row = rows[*it];
            const int stat = query.value(1).toInt();
            const int value = query.value(2).toInt();
            if (stat >= 1 && stat <= 6) {
                row.stats[stat - 1] = value;
                row.total += value;
            } else if (stat == kSpecialStat) { // 1세대: 특공 · 특방 두 칸에 같은 값, 합계에는 한 번
                row.stats[3] = value;
                row.stats[4] = value;
                row.total += value;
            }
        }
    }
}
} // namespace com::yamada::studio
