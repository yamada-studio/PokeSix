#include "data/repository/repository.h"

#include "data/logging/logging.h"

#include <QHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariant>

namespace {
// 세대 g의 값 = 구간 [gen_from, gen_to]에 g가 들어가는 줄 (ADR 0011, schema.h):
//   gen_from <= :g AND (gen_to IS NULL OR gen_to >= :g)
// 같은 이름의 자리표시(:g)를 두 번 써도 SQLite는 같은 값으로 채운다.
constexpr int kSpecialStat = 9; // 1세대 전용 "특수"
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

QList<SpeciesRow> Repository::speciesForGeneration(int generation)
{
    QList<SpeciesRow> rows;
    if (!open())
        return rows;

    // 그 세대까지 나온 종과 기본 모습. 전국도감이므로 도감 번호 = 종 번호.
    QSqlQuery query(QSqlDatabase::database(m_connection));
    query.prepare(QStringLiteral(
            "SELECT s.id, p.id, s.name_ko, s.name_en, s.name_ja, s.id FROM species s "
            "JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1 "
            "WHERE s.intro_gen <= :g ORDER BY s.id"));
    query.bindValue(QStringLiteral(":g"), generation);
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
    query.prepare(QStringLiteral(
            "SELECT s.id, p.id, s.name_ko, s.name_en, s.name_ja, dn.number FROM dex_numbers dn "
            "JOIN species s ON s.id = dn.species_id "
            "JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1 "
            "WHERE dn.pokedex_id = :dex ORDER BY dn.number"));
    query.bindValue(QStringLiteral(":dex"), pokedexId);
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
            "SELECT d.id, d.identifier, r.name_ko, v.identifier, v.name_en, v.name_ko "
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
            dex.regionKo = query.value(2).toString();
            it = indexOfDex.insert(id, dexes.size());
            dexes.append(dex);
        }
        DexInfo &dex = dexes[*it];
        dex.versions.append(query.value(3).toString());
        dex.versionsEn.append(query.value(4).toString());
        dex.versionsKo.append(query.value(5).toString());
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

QList<ItemRow> Repository::itemsForGeneration(int generation)
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
        row.nameKo = query.value(4).toString();
        row.nameEn = query.value(5).toString();
        row.nameJa = query.value(6).toString();
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

    // 3) 기술머신: 이 세대에 담긴 기술 + 그 기술의 이 세대 타입(구간 질의)
    query.prepare(QStringLiteral(
            "SELECT im.item_id, m.name_ko, m.name_en, t.identifier FROM item_machines im "
            "JOIN moves m ON m.id = im.move_id "
            "LEFT JOIN move_types mt ON mt.move_id = m.id AND mt.gen_from <= :g "
            "  AND (mt.gen_to IS NULL OR mt.gen_to >= :g) "
            "LEFT JOIN types t ON t.id = mt.type_id "
            "WHERE im.generation = :g"));
    query.bindValue(QStringLiteral(":g"), generation);
    if (query.exec()) {
        while (query.next()) {
            const auto it = rowOfItem.constFind(query.value(0).toInt());
            if (it == rowOfItem.constEnd())
                continue;
            ItemRow &row = rows[*it];
            row.machineMoveKo = query.value(1).toString();
            row.machineMoveEn = query.value(2).toString();
            row.machineType = query.value(3).toString();
        }
    }

    // 4) 효과 문구: 세대 오름차순으로 읽으면서, generation 이하면 계속 덮어쓰고(→ 가장 최근),
    //    아직 아무것도 없으면 처음 것을 쥔다(→ generation보다 뒤 세대 중 가장 이른 것).
    QHash<int, int> takenFrom; // item id → 문구를 가져온 세대
    if (query.exec(QStringLiteral(
                "SELECT item_id, generation, text_ko FROM item_effects ORDER BY generation"))) {
        while (query.next()) {
            const int id = query.value(0).toInt();
            const auto it = rowOfItem.constFind(id);
            if (it == rowOfItem.constEnd())
                continue;
            const int g = query.value(1).toInt();
            const int taken = takenFrom.value(id, 0);
            if (taken == 0 || (g <= generation && taken <= generation)) {
                rows[*it].effect = query.value(2).toString();
                takenFrom.insert(id, g);
            }
        }
    }
    // 기술머신의 설명문은 담긴 기술의 설명이다. 다른 세대 문구를 빌려 오면 기술과 설명이 어긋난다
    // (4세대 기술머신01 = 힘껏펀치인데 6세대 문구는 손톱갈기) → 그 세대 문구가 아니면 비운다.
    for (ItemRow &row : rows)
        if (!row.machineMoveKo.isEmpty() && takenFrom.value(row.id) != generation)
            row.effect.clear();
    return rows;
}

bool Repository::readSpecies(QSqlQuery &query, QList<SpeciesRow> &rows)
{
    // 열 순서: 종 id · 기본 모습 pokemon id · 이름 ko/en/ja · 도감 번호
    if (!query.exec()) {
        m_error = query.lastError().text();
        qCWarning(lcData) << "species query failed:" << m_error;
        return false;
    }
    while (query.next()) {
        SpeciesRow row;
        row.speciesId = query.value(0).toInt();
        row.pokemonId = query.value(1).toInt();
        row.nameKo = query.value(2).toString();
        row.nameEn = query.value(3).toString();
        row.nameJa = query.value(4).toString();
        row.dexNumber = query.value(5).toInt();
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

    // 1) 그 세대의 타입(슬롯 순). 표 전체를 한 번에 읽고 나눠 담는다 — 종마다 질의하면 수백 번이
    // 된다.
    query.prepare(
            QStringLiteral("SELECT pt.pokemon_id, t.identifier FROM pokemon_types pt "
                           "JOIN types t ON t.id = pt.type_id "
                           "WHERE pt.gen_from <= :g AND (pt.gen_to IS NULL OR pt.gen_to >= :g) "
                           "ORDER BY pt.pokemon_id, pt.slot"));
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
                                 "WHERE gen_from <= :g AND (gen_to IS NULL OR gen_to >= :g)"));
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
