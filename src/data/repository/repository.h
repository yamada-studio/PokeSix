#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <array>

class QSqlQuery;

namespace com::yamada::studio {
// 도감 목록 한 줄: 한 종의 기본 모습(pokemon.is_default = 1)을 "그 세대 기준"으로 본 값.
struct SpeciesRow
{
    int speciesId = 0; // 전국도감 번호
    int dexNumber = 0; // 보고 있는 도감의 번호(전국이면 speciesId와 같다)
    int pokemonId = 0; // 기본 모습의 pokemon id
    QString nameKo, nameEn, nameJa;
    QStringList types; // 타입 identifier(슬롯 순): "dragon", "ground" — UI가 색 · 이름을 붙인다
    std::array<int, 6> stats {}; // HP · 공격 · 방어 · 특공 · 특방 · 스피드
    int total = 0;               // 합계
};

// 지방 도감 하나(도감 선택 버튼 하나). 한 도감을 여러 게임이 쓰면 버전 이름이 모인다.
struct DexInfo
{
    int pokedexId = 0; // PokéAPI pokedexes.id (신오 DP = 5, 신오 Pt = 6, 성도 HGSS = 7)
    QString regionKo; // "신오" — 도감의 한국어 이름은 PokéAPI에 없어서 지방 이름을 쓴다
    QStringList versionsEn; // {"Diamond", "Pearl"} — 약칭(D · P)은 UI가 붙인다
    QStringList versionsKo; // {"디아루가", "펄기아"}
};

// 게임 데이터 DB(pokesix.sqlite)의 조회 창구. 로컬 DB만 읽는다(네트워크 없음, ADR 0011).
// UI 스레드에서 짧은 질의만 한다(architecture §6). QSqlDatabase 연결은 이 객체를 만든 스레드에서만
// 쓴다. 모든 질의는 세대를 인자로 받는다 — 세대를 전역에서 몰래 읽지 않는다(architecture §9).
class Repository
{
public:
    explicit Repository(const QString &dbPath);
    ~Repository();
    Repository(const Repository &) = delete; // 연결 이름을 하나씩 가지므로 복사하지 않는다
    Repository &operator=(const Repository &) = delete;

    bool open(); // 이미 열려 있으면 true. 실패하면 errorString()
    QString errorString() const { return m_error; }

    // 세대 generation 게임들의 지방 도감(본편, 게임이 나온 순서). 전국도감은 넣지 않는다.
    // 4세대 → 신오(DP) · 신오(Pt) · 성도(HGSS). 목록을 코드에 적지 않고 도감 ↔ 게임 ↔ 세대 연결로
    // 찾는다.
    QList<DexInfo> dexesForGeneration(int generation);
    // 지방 도감 pokedexId의 종(도감 번호 순). 타입 · 종족값은 generation 기준.
    QList<SpeciesRow> speciesForDex(int pokedexId, int generation);

    // 세대 generation까지 나온 종 전부(번호 순). 타입 · 종족값은 그 세대 기준.
    // 1세대는 "특수"(stat 9) 하나였으므로 특공 · 특방 칸에 같은 값을 넣고, 합계에는 한 번만 더한다.
    QList<SpeciesRow> speciesForGeneration(int generation);

private:
    QString m_path;
    QString m_connection;
    QString m_error;

    // 실행 전인 query(열: 종 · pokemon · 이름 ko/en/ja · 도감 번호)를 실행해 rows에 담는다.
    bool readSpecies(QSqlQuery &query, QList<SpeciesRow> &rows);
    // rows의 기본 모습에 세대 generation의 타입 · 종족값을 채운다(전국 · 지방 목록 공통).
    void fillTypesAndStats(QList<SpeciesRow> &rows, int generation);
};
} // namespace com::yamada::studio
