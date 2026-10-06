#pragma once

#include "data/text/localizedtext.h"

#include <QHash>
#include <QList>
#include <QSet>
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
    bool legendary = false;      // 전설 · 환상 (필터 "전설 · 환상 제외")
    // 그 세대 기준 최종 진화인가(그 세대까지 나온 진화형이 없다). 강철톤이 없는 1세대의 롱스톤은
    // 최종 진화다 — 세대 차이는 질의가 계산한다.
    bool finalEvolution = true;
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
    // 이 도감을 쓰는 게임 묶음(그 세대, 나온 순서). 마지막 것이 상세 화면의 기술 기준이 된다
    // (관동도감 1세대: 레드·그린(일) · 블루(일) · 레드·블루 · 피카츄 → 피카츄)
    QStringList versionGroups;
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

// 상세 화면의 기술 한 줄(레벨업 · 기술머신). 값은 조회한 세대 기준(위력 · 명중 · PP · 타입 · 분류).
struct MoveEntry
{
    int moveId = 0;
    LocalizedText name;
    QString type; // 타입 identifier. ??? 타입(저주 2–4세대)이면 비어 있다
    int damageClass
            = 0; // 1 변화 · 2 물리 · 3 특수 — 3세대까지는 타입이 정한다(Repository가 바꿔 둔다)
    int power = 0;    // 0 = 없음(변화 기술 · 위력이 바뀌는 기술)
    int accuracy = 0; // 0 = 반드시 맞음
    int pp = 0;
    int level = 0;              // 레벨업: 배우는 레벨(1 = 처음부터)
    int machineNumber = 0;      // 기술머신: 번호
    bool hiddenMachine = false; // 비전머신
    QString machineItem; // 기술머신 아이템 identifier("tm01") — UI의 획득처 사전 키
    // 하트비늘(기술 떠올리기)이 있어야 배우는 기술. 이 게임에서 얻는 레벨보다 낮고 얻을 때 기본으로
    // 갖고 있지도 않은 기술(HGSS 화석 프테라 Lv 20의 Lv 1 삼색엄니), 진화 전 단계가 익히지 않는
    // 진화형의 Lv 1 기술(가디안의 치유소원). 이 게임에서 얻을 수 없으면 뒤의 것만 본다.
    bool needsReminder = false;
    // 효과 뼈대(지금 게임 값 — 옛 세대 차이는 UI의 move-effects.json이 덮는다)
    QString identifier; // "swords-dance" — UI 효과 사전의 키
    int target = 0;     // PokéAPI move_targets.id: 7 자신, 10 고른 상대 …
    int ailment = 0; // move_meta_ailments.id: 1 마비 · 2 잠듦 · 3 얼음 · 4 화상 · 5 독 · 6 혼란 …
    int healing = 0;                        // HP 회복 %(음수 = 소모)
    QList<std::pair<int, int>> statChanges; // (stat id 2–8, 단계 ±n)
    LocalizedText effect; // 그 세대의 게임 설명문(언어마다 가까운 세대 것)
};

// 야생 출현 한 줄: 버전 · 장소 · 방법마다 레벨 범위와 출현 칸 확률의 합.
struct EncounterEntry
{
    QString version;            // "platinum"
    LocalizedText versionName;  // 기라티나 · Platinum · プラチナ
    QString location;           // "sinnoh-route-201" — UI 장소 사전의 키
    LocalizedText locationName; // PokéAPI 이름(신오 · 성도 · 관동은 한국어가 없다)
    QString method;             // "walk", "surf", "old-rod" …
    int minLevel = 0;
    int maxLevel = 0;
    int rarity = 0; // 출현 칸 확률(%)의 합 — 시간대 · 층 칸이 묶여 있어서 100을 넘을 수 있다
};

// 진화 조건 하나(그 세대에 적용되는 방법). 비어 있는 칸은 조건이 아니다.
struct EvolutionCondition
{
    static constexpr int kNoRelativeStats = 99;
    int trigger = 0; // 1 레벨업 · 2 통신교환 · 3 도구 사용 · 4 빈자리 · 그 밖 = 특별
    int minLevel = 0;
    LocalizedText item;         // 쓰는 도구(진화의 돌 등)
    QString itemIdentifier;     // "water-stone" — 입수 사전(guidebook) 키
    LocalizedText heldItem;     // 지닌 도구(통신교환 · 레벨업)
    QString heldItemIdentifier; // "kings-rock"
    int gender = 0;             // 1 암컷 · 2 수컷
    QString location; // 그곳에서 레벨업(이끼바위 · 얼음바위 · 천관산) — UI 장소 사전 키
    LocalizedText locationName;
    QString timeOfDay;       // "day" · "night" · "dusk"
    LocalizedText knownMove; // 그 기술을 배운 채로
    QString knownMoveType;   // 그 타입 기술을 배운 채로(님피아: 페어리)
    int minHappiness = 0;
    int minBeauty = 0;
    int minAffection = 0;
    int relativeStats = kNoRelativeStats; // 배루키: 1 공격 > 방어 · 0 같음 · −1 공격 < 방어
    LocalizedText partySpecies; // 그 포켓몬이 파티에 있을 때(만타인: 총어)
    QString partyType;          // 그 타입 포켓몬이 파티에 있을 때(판짱: 악)
    LocalizedText tradeSpecies; // 그 포켓몬과 교환할 때(쪼마리 · 딱정곤)
    bool needsRain = false;
    bool upsideDown = false;
};

// 진화 트리의 한 마디. 뿌리(기본 단계)부터 깊이 우선 순서로 늘어놓는다.
struct EvolutionStep
{
    int speciesId = 0;
    int pokemonId = 0;     // 기본 모습(아이콘 · 누르면 그 상세로)
    int fromSpeciesId = 0; // 진화 전 종(뿌리는 0)
    int depth = 0;         // 뿌리 0
    LocalizedText name;
    QList<EvolutionCondition> conditions; // 진화 전 종에서 이 종이 되는 방법(여럿이면 그중 하나)
};

// 특성 하나(상세 화면). 효과 문구는 그 세대 · 언어의 게임 설명문(없으면 가까운 세대).
struct AbilityEntry
{
    int abilityId = 0;
    QString identifier;  // "rock-head" — 이름 · 설명 보충 사전(namebook)의 키
    int slot = 0;        // 1 · 2 = 일반, 3 = 숨겨진 특성
    bool hidden = false; // 숨겨진 특성(5세대부터)
    LocalizedText name;
    LocalizedText effect;
};

// 성격 하나. 능력치는 stats.id(2 공격 · 3 방어 · 4 특공 · 5 특방 · 6 스피드). 같으면 무보정.
struct Nature
{
    int id = 0;
    LocalizedText name;
    int increasedStat = 0; // +10%
    int decreasedStat = 0; // −10%

    bool isNeutral() const { return increasedStat == decreasedStat; }
};

// 포켓몬 상세(도감 상세 화면). 세대 g 기준. 기술은 그 세대 대표 게임(versionGroup) 기준이다.
struct PokemonDetail
{
    int speciesId = 0;
    int pokemonId = 0;
    LocalizedText name;
    LocalizedText genus; // 씨앗포켓몬
    QStringList types;
    std::array<int, 6> stats {};
    int total = 0;
    int height = 0; // 10 cm 단위(PokéAPI)
    int weight = 0; // 100 g 단위
    int generation = 0;
    QString versionGroup;            // 기술 기준 게임 묶음 identifier("platinum")
    QList<LocalizedText> groupGames; // 그 묶음의 버전 이름들(기라티나)
    QStringList groupVersions; // 그 묶음의 버전 identifier("platinum") — UI가 약칭(Pt)을 붙인다
    QList<EncounterEntry> encounters; // 그 세대 모든 버전(DP · Pt · HGSS)
    QList<MoveEntry> levelMoves;      // 레벨 순(같은 레벨은 게임이 익히는 순서)
    QList<MoveEntry> machineMoves;    // 기술머신 번호 순, 비전머신은 뒤에
    QList<MoveEntry> tutorMoves; // 기술 가르침(이름은 fillMoves가 채운다. 순서 = move id)
    QList<MoveEntry> eggMoves; // 알 기술(진화 전 단계에만 있는 경우가 많다)
    QList<EvolutionStep> evolution; // 그 세대에 있는 종만. 진화하지 않는 포켓몬은 비어 있다
    // 이 게임에서 이 포켓몬을 가장 일찍 가질 수 있는 레벨(잡기 · 받기 · 진화). 0 = 이 게임의 출현
    // 자료로는 얻을 수 없다
    int earliestLevel = 0;
    QList<AbilityEntry> abilities; // 그 세대의 특성(칸 순). 1–2세대는 비어 있다(특성이 없었다)

    bool isValid() const { return pokemonId > 0; }
};

// 세대 g의 타입 상성표: (공격 타입, 방어 타입) → 배율. 표에 없는 쌍은 1배.
// 세대의 게임 묶음 하나(4세대: DP · Pt · HGSS). 스쿼드의 "기술 기준 게임" 선택지
// 아이템으로 하는 진화 한 줄 (아이템 상세 창의 "진화"): 가디 → 윈디(불꽃의돌 사용),
// 분이 → 딱구리(지니고 통신교환)
struct ItemEvolution
{
    LocalizedText from;
    LocalizedText to;
    bool held = false;     // true = 지니고(통신교환 등), false = 사용
    int speciesId = 0;     // 진화한 종(to)
    int fromPokemonId = 0; // 진화 전 · 후의 기본 모습(아이콘). 진화 전이 없으면 0
    int toPokemonId = 0;
    // 같은 묶음 중 이 진화를 할 수 있는 버전(진화 사슬이 버전 한정일 때만 — 비면 모든 버전).
    // Repository가 아니라 화면이 게임 묶음을 보고 채운다
    QStringList onlyVersions;
};

struct GameInfo
{
    QString versionGroup;              // "platinum"
    QStringList versions;              // {"platinum"} — UI 약칭(dexstyle.json)의 키
    QList<LocalizedText> versionNames; // versions와 같은 순서
};

// 버전("soulsilver")이 든 게임 묶음 identifier. 없으면 빈 칸
inline QString versionGroupOf(const QList<GameInfo> &games, const QString &version)
{
    for (const GameInfo &game : games)
        if (game.versions.contains(version))
            return game.versionGroup;
    return {};
}

struct TypeChart
{
    QStringList types;                  // 그 세대에 있는 타입(id 순)
    QHash<QString, double> multipliers; // key = "공격/방어"

    double at(const QString &attack, const QString &defense) const
    {
        return multipliers.value(attack + QLatin1Char('/') + defense, 1.0);
    }
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

    // 이미 열려 있으면 true. 파일이 없거나 스키마 버전이 이 앱과 다르면(옛 DB) 열지 않는다 —
    // 첫 실행 · 스키마가 바뀐 뒤의 데이터 받기가 끝나기 전에 옛 표를 읽지 않게. 실패하면
    // errorString()
    bool open();
    // 연결을 닫는다. DB 파일이 새것으로 바뀌었을 때(데이터 받기 끝) 부른다 — 열린 연결은 바꿔치기
    // 전의 옛 파일을 계속 보므로, 닫아야 다음 open()이 새 파일을 연다
    void close();
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
    // versionGroup을 주면 기술머신에 담긴 기술을 그 게임 것으로(같은 7세대라도 레츠고 ≠ 썬문,
    // 8세대 소드실드 ≠ BDSP). 비우면 세대 기준(그 세대 첫 게임)
    QList<ItemRow> itemsForGeneration(int generation, const QString &versionGroup = {});

    // 포켓몬 하나의 상세(기본 모습 pokemonId, 세대 generation). 없으면 isValid() == false.
    // versionGroup: 기술 · 기술머신의 기준 게임 묶음("heartgold-soulsilver"). 비었거나 그 세대
    // 게임이 아니면 세대의 대표 게임(representativeVersionGroup).
    PokemonDetail pokemonDetail(int pokemonId, int generation, const QString &versionGroup = {});
    // 세대 generation의 타입 상성표(그 세대에 있는 타입끼리)
    TypeChart typeChart(int generation);
    // 세대마다 기술 기준으로 쓰는 게임 묶음("platinum"). 셋째 판 · 확장판이 그 세대를 가장 넓게
    // 담는다.
    static QString representativeVersionGroup(int generation);
    // 성격 25가지(id 순). 성격은 3세대부터 — 1–2세대 화면은 쓰지 않는다
    QList<Nature> natures();
    // 종 → 진화 전 종(species.evolves_from). 진화 전이 없는 종은 빠진다. 스쿼드의 "진화 전 · 후
    // 모습이 이미 있어요" 경고에 쓴다
    QHash<int, int> evolvesFrom();
    // 그 세대의 본편 게임 묶음(도감이 연결된 것만 — 콜로세움 · XD 같은 외전은 뺀다). 출시 순
    QList<GameInfo> gamesForGeneration(int generation);
    // 그 세대에서 쓸 버전: version이 그 세대의 버전이면 그대로, 게임 묶음 identifier면(옛 저장값)
    // 그 묶음의 첫 버전, 아니면 대표 게임 묶음(representativeVersionGroup)의 첫 버전
    QString resolveVersion(int generation, const QString &version);
    // 같은 묶음의 다른 버전에서만 얻을 수 있는 종(HG에서 본 SS 한정 나옹 · 식스테일 …). 진화 사슬
    // 단위로 센다 — 사슬의 한 종이라도 이 버전에서 잡히면(진화 · 교배로) 사슬 전체를 얻을 수 있다.
    // 두 버전 모두 출현 자료가 없는 종(선물 · 이벤트)은 넣지 않는다.
    QSet<int> otherVersionSpecies(const QString &version);
    // 기술 id들 → 그 세대 값(타입 · 위력 · 분류 …). 없는 id는 빠진다. 순서는 ids 그대로
    QList<MoveEntry> moves(const QList<int> &ids, int generation);

    // 세대 generation까지 나온 종 전부(번호 순). 타입 · 종족값은 그 세대 기준.
    // 1세대는 "특수"(stat 9) 하나였으므로 특공 · 특방 칸에 같은 값을 넣고, 합계에는 한 번만 더한다.
    QList<SpeciesRow> speciesForGeneration(int generation);
    // 이 아이템으로 진화하는 포켓몬(그 세대까지의 규칙 · 그 세대까지 나온 진화형만)
    QList<ItemEvolution> evolutionsWithItem(int itemId, int generation);
    // 그 게임 묶음에서 이 기술머신 · 비전머신(아이템 identifier "tm26")으로 기술을 배울 수 있는 종
    // (기본 모습, 종 번호 순). 기술머신이 아니거나 그 게임에 없으면 빈 목록
    QList<SpeciesRow> machineLearners(const QString &item, const QString &versionGroup);
    // 그 게임 묶음에서 만나는 종(진화 사슬 단위): 그 게임의 지방 도감(HGSS = 성도 도감 256)에 있는
    // 종 + 야생(풀숲 · 파도타기 · 낚시 …)에서 잡는 종(관동 도로 · 사파리존처럼 도감 밖). 고정 배치
    // · 배회 · 선물 · 교환은 도감 밖이면 넣지 않는다(HGSS 신오의 유적 기라티나 같은 특별 이벤트).
    // 비면 모른다 (거르지 않는다)
    QSet<int> gameSpecies(const QString &versionGroup);

private:
    QString m_path;
    QString m_connection;
    QString m_error;
    QHash<QString, QSet<int>> m_otherVersionSpecies; // 버전 → otherVersionSpecies 결과
    QHash<QString, QSet<int>> m_gameSpecies;         // 게임 묶음 → gameSpecies 결과
    static constexpr qsizetype kDetailCacheSize = 256;
    QHash<QString, PokemonDetail> m_details; // "포켓몬/세대/게임 묶음" → pokemonDetail 결과

    // 실행 전인 query(열: 종 · pokemon · 이름 ko/en/ja · 도감 번호)를 실행해 rows에 담는다.
    bool readSpecies(QSqlQuery &query, QList<SpeciesRow> &rows);
    // rows의 기본 모습에 세대 generation의 타입 · 종족값을 채운다(전국 · 지방 목록 공통).
    void fillTypesAndStats(QList<SpeciesRow> &rows, int generation);
    // moves의 moveId로 이름 · 타입 · 분류 · 위력 · 명중 · PP를 세대 generation 기준으로 채운다
    void fillMoves(QList<MoveEntry> &moves, int generation);
    // 진화 트리(그 세대 기준)와 레벨업 기술의 하트비늘 표시(진화 전 단계의 레벨업 기술과 견준다)
    void fillEvolution(PokemonDetail &detail, int versionGroupId);
    PokemonDetail readPokemonDetail(int pokemonId, int generation, const QString &versionGroup);
    // 그 세대의 특성(칸 · 숨겨진 특성)과 효과 문구
    void fillAbilities(PokemonDetail &detail);
};
} // namespace com::yamada::studio
