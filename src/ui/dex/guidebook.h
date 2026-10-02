#pragma once

#include "data/text/localizedtext.h"

#include <QString>
#include <QStringList>

// 공략 정보 사전: PokéAPI에 없는 글자를 앱에 들어 있는 사전(resources/data/*.json)에서 찾는다.
//   machine-locations.json  기술머신 · 비전머신 획득처(게임 묶음마다)
//   place-names.json        장소의 한국어 이름(신오 · 성도 · 관동은 PokéAPI에 없다)
//   encounter-methods.json  야생 출현 방법 이름(풀숲 · 낚싯대 …)
// dexstyle · itemstyle과 같은 방식이다: 처음 부를 때 한 번 읽어 둔다. 사전에 없으면 대신 보일
// 글자를 돌려준다 — 화면이 비지 않는다.
namespace com::yamada::studio::guidebook {
// 기술머신 획득처 한 줄씩. 사전에 없으면 빈 목록. versionGroup = "platinum", machineItem = "tm01"
QStringList machinePlaces(const QString &versionGroup, const QString &machineItem);

// NPC 가르침 기술의 비용("48BP" · "빨강조각 8개" …). 사전에 없으면 빈 문자열.
// PokéAPI에는 이 데이터가 없어서 사전(tutor-costs.json)으로만 안다.
QString tutorCost(const QString &versionGroup, const QString &move, Language language);

// 장소 이름: 사전(한국어) → 도로 · 수로 번호 규칙("201번 도로") → PokéAPI 이름(대체 순서)
QString placeName(const QString &identifier, const LocalizedText &pokeapiName, Language language);

// 출현 방법 이름. 사전에 없으면 identifier를 사람이 읽기 좋게("rock-smash" → "rock smash")
QString methodName(const QString &identifier, Language language);
} // namespace com::yamada::studio::guidebook
