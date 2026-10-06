#pragma once

#include "data/text/localizedtext.h"

#include <QSet>
#include <QString>
#include <QStringList>

// 공략 정보 사전: PokéAPI에 없는 글자를 앱에 들어 있는 사전(resources/data/*.json)에서 찾는다.
//   acquisition/<게임>.json 게임(버전 그룹)마다 아이템 · 기술머신 입수 방법과 NPC 가르침 비용
//   place-names.json        장소의 한국어 이름(신오 · 성도 · 관동은 PokéAPI에 없다)
//   encounter-methods.json  야생 출현 방법 이름(풀숲 · 낚싯대 …)
// dexstyle · itemstyle과 같은 방식이다: 처음 부를 때 한 번 읽어 둔다. 사전에 없으면 대신 보일
// 글자를 돌려준다 — 화면이 비지 않는다.
namespace com::yamada::studio::guidebook {
// 아이템(기술머신 포함) 입수 방법 한 줄씩: "필드 · 무쇠게이트 (지하 1층)", "교환 · 배틀프런티어
// 48BP". 그 게임의 사전에 없으면 빈 목록. versionGroup = "platinum", item = "tm01" · "fire-stone".
// version("white-2")을 주면 다른 버전 한정 입수처는 뺀다(모두 빠지면 "다른 버전 한정" 한 줄).
// 비우면 묶음 전체: 버전 한정 입수처 앞에 그 버전 약칭을 붙인다("W2 — 13번 도로")
QStringList itemSources(const QString &versionGroup, const QString &item, Language language,
                        const QString &version = {});

// 이 게임의 입수 사전에 아이템 목록이 있는가 — 있으면 아이템 백과가 그 목록으로 게임별 존재를
// 거른다(없으면 세대 기준 그대로). itemsIn = 그 목록(아이템 identifier)
bool hasItemBook(const QString &versionGroup);
QSet<QString> itemsIn(const QString &versionGroup);

// NPC 가르침 기술의 비용(컨텐츠 + 재화: "배틀프런티어 48BP"). 사전에 없으면 빈 문자열.
// PokéAPI에는 이 데이터가 없어서 입수 사전의 tutors로만 안다.
QString tutorCost(const QString &versionGroup, const QString &move, Language language);

// 아이템(기술머신) 하나의 공급 사정 — 스쿼드 분석의 리소스 집계용(core resourceledger의 입력).
// 한 번만 얻는 입수처(필드 · 숨겨진 · 받기 · 보상 · 통신 교환)는 copies로 세고, 반복 입수처(상점 ·
// 교환 · 경품)는 repeatable로 둔다. 픽업 특성 같은 랜덤 입수(other)는 세지 않는다.
struct ItemSupply
{
    bool known = false; // 사전에 셀 수 있는 입수처가 있다
    int copies = 0;
    bool repeatable = false;
    QList<std::pair<int, QString>> repeatCost; // 반복 입수 한 번의 비용(비용이 적힌 첫 입수처)
    // 모든 입수처가 엔딩 후에야 가는 지방(성도 게임의 관동 — 입수처의 region으로 1차 근사)
    bool postGameOnly = false;
};
ItemSupply itemSupply(const QString &versionGroup, const QString &item, const QString &version);

// NPC 가르침 한 번의 비용 (양, 단위) 목록 — 비용이 적힌 첫 입수처. 사전에 없으면 빈 목록
QList<std::pair<int, QString>> tutorCostAmounts(const QString &versionGroup, const QString &move,
                                                const QString &version);

// 비용 한 덩이의 화면 글자: (48, "bp") → "48BP", (2, "red-shard") → "빨강조각 2개"
QString costLabel(int amount, const QString &unit);

// 장소 이름: 사전(한국어) → 도로 · 수로 번호 규칙("201번 도로") → PokéAPI 이름(대체 순서)
QString placeName(const QString &identifier, const LocalizedText &pokeapiName, Language language);

// 출현 방법 이름. 사전에 없으면 identifier를 사람이 읽기 좋게("rock-smash" → "rock smash")
QString methodName(const QString &identifier, Language language);
} // namespace com::yamada::studio::guidebook
