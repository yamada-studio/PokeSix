#pragma once

#include "data/text/localizedtext.h"

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
    LocalizedText name;
    QStringList types; // 타입 identifier(슬롯 순): "dragon", "ground" — UI가 색 · 이름을 붙인다
    std::array<int, 6> stats {}; // HP · 공격 · 방어 · 특공 · 특방 · 스피드
    int total = 0;               // 합계
};

// 지방 도감 하나(도감 선택 버튼 하나). 한 도감을 여러 게임이 쓰면 버전 이름이 모인다.
struct DexInfo
{
    int pokedexId = 0; // PokéAPI pokedexes.id (신오 DP = 5, 신오 Pt = 6, 성도 HGSS = 7)
    QString identifier; // "original-sinnoh" — UI 도감 표시 규칙(숨김 · 이름 바꾸기)의 키
    LocalizedText region; // "신오" — 도감 이름은 PokéAPI에 없어서 지방 이름을 쓴다
    QStringList versions; // {"diamond", "pearl"} — UI 버전 색 · 약칭 표(dexstyle.json)의 키
    QList<LocalizedText>
            versionNames; // {디아루가 · Diamond · ダイヤモンド, …} (versions와 같은 순서)
};

// 아이템 대백과 한 줄. 세대별 존재는 비트로: bit (g − 1) = g세대에 있다.
struct ItemRow
{
    int id = 0;
    QString identifier; // "fire-stone" — 아이콘 파일 이름(SpriteCache::Kind::Item)
    QString category;   // PokéAPI item_categories.identifier("evolution", "healing" …)
    QString pocket;     // 가방 주머니("misc", "medicine", "machines" …)
    LocalizedText name;
    quint16 generations = 0;
    LocalizedText effect; // 조회한 세대의 효과 문구(언어마다 가까운 세대 것. 없으면 빈 칸)
    int cost = 0; // 상점 가격(PokéAPI는 최신 게임 기준 하나뿐이다). 0 = 팔지 않음
    // 기술머신 · 비전머신 · 기술레코드면 조회한 세대에 담긴 기술(아니면 비어 있다)
    LocalizedText machineMove;
    QString machineType; // 그 기술의 그 세대 타입 identifier("fighting"). ??? 타입이면 비어 있다

    bool existsIn(int generation) const { return generations & (1u << (generation - 1)); }
    int introGeneration() const; // 처음 나온 세대(없으면 0)
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

    // 어느 세대에든 있었던 아이템 전부(id 순). 세대별 존재(generations)는 전부 담고, 효과 문구는
    // generation 기준이다: 그 세대 이하에서 가장 최근 문구, 없으면(1–5세대) 가장 이른 문구(6세대).
    // 그 세대에 없는 아이템도 넣는다 — 화면이 흐리게 보여 줄지 뺄지 정한다(프록시).
    QList<ItemRow> itemsForGeneration(int generation);

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
