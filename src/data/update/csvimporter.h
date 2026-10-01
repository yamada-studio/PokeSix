#pragma once

#include <QHash>
#include <QString>
#include <QStringList>

#include <functional>

class QSqlDatabase;

namespace com::yamada::studio {
// 받아 둔 PokéAPI CSV 폴더를 게임 데이터 DB(schema.h)로 바꾼다 (ADR 0011, 로드맵 D1).
//
//   CsvImporter importer;
//   if (!importer.run(csvDir, dbPath))
//       qWarning() << importer.errorString();
//
// - 동기 함수다. 실제 데이터 전체는 수 초가 걸리므로 앱에서는 worker 스레드에서 부른다(D5).
//   그래서 DB 연결을 이 객체 안에서 만들고 닫는다. QSqlDatabase 연결은 만든 스레드에서만 쓸 수
//   있다.
// - DB는 "dbPath.importing" 임시 파일에 만든 뒤, 전부 성공했을 때만 dbPath로 바꿔 넣는다.
//   도중에 실패해도 기존 DB는 그대로 남고, 반쪽짜리 DB가 생기지 않는다.
// - 표 전체를 트랜잭션 하나로 넣는다. SQLite는 트랜잭션 없이 INSERT마다 디스크에 확정하므로
//   수천 배 느려진다(가이드 D1 실험 1).
class CsvImporter
{
public:
    bool run(const QString &csvDir, const QString &dbPath);
    QString errorString() const { return m_error; }

private:
    bool createSchema(QSqlDatabase &db);
    bool importGenerations(QSqlDatabase &db);
    bool importTypes(QSqlDatabase &db);
    bool importTypeChart(QSqlDatabase &db);
    bool importStats(QSqlDatabase &db);
    bool importSpecies(QSqlDatabase &db);
    bool
    importPokemon(QSqlDatabase &db); // 폼이 처음 나온 세대(pokemon_forms + version_groups)도 여기서
    bool importPokemonTypes(QSqlDatabase &db);
    bool importPokemonStats(QSqlDatabase &db);
    // 도감 선택(D2b): 지방 이름 · 게임 묶음 · 버전 이름 · 도감 목록과 도감별 번호
    bool importRegions(QSqlDatabase &db);
    bool importVersionGroups(QSqlDatabase &db);
    bool importVersions(QSqlDatabase &db);
    bool importPokedexes(QSqlDatabase &db); // pokedexes + pokedex_version_groups + dex_numbers
    // 아이템(E3): 분류 · 아이템 + 이름 · 세대별 존재 · 세대별 한국어 효과 문구
    bool importItems(QSqlDatabase &db);
    bool importMoves(QSqlDatabase &db); // 기술 + 이름 + 세대별 타입(기술머신 아이콘 · 타입 칩)
    bool importMachines(QSqlDatabase &db); // 기술머신 → 세대별로 담긴 기술 + 게임마다의 번호 표
    bool importPokemonMoves(QSqlDatabase &db); // 습득 기술(레벨업 · 교배 · NPC · 기술머신)
    bool importEncounters(QSqlDatabase &db); // 장소 · 방법 · 야생 출현(묶어서)
    bool importEvolutions(QSqlDatabase &db); // 진화 방법(세대마다)
    bool importItemEffects(QSqlDatabase &db);
    bool writeMeta(QSqlDatabase &db);

    // CSV 파일 하나를 열어, 레코드마다 columns 순서대로 값을 뽑아 row(values)를 부른다.
    // 필요한 열이 없거나 파일을 못 열거나 row가 false를 돌려주면 실패.
    bool forEachRecord(const QString &file, const QStringList &columns,
                       const std::function<bool(const QStringList &values)> &row);
    bool fail(const QString &message);

    int typeIntro(int typeId) const;       // 없으면 1
    int pokemonIntro(int pokemonId) const; // 없으면 1

    QString m_csvDir;
    QString m_error;

    // 앞 단계에서 읽어 두고 뒤 단계의 구간 시작 세대(firstGen)로 쓴다.
    QHash<int, int> m_typeIntroGen;    // type_id → 처음 나온 세대
    QHash<int, int> m_speciesIntroGen; // species_id → 처음 나온 세대
    QHash<int, int> m_pokemonIntroGen; // pokemon_id → 처음 나온 세대(폼 기준)
};
} // namespace com::yamada::studio
