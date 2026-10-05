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
// 48BP". 그 게임의 사전에 없으면 빈 목록. versionGroup = "platinum", item = "tm01" · "fire-stone"
QStringList itemSources(const QString &versionGroup, const QString &item, Language language);

// 이 게임의 입수 사전에 아이템 목록이 있는가 — 있으면 아이템 백과가 그 목록으로 게임별 존재를
// 거른다(없으면 세대 기준 그대로). itemsIn = 그 목록(아이템 identifier)
bool hasItemBook(const QString &versionGroup);
QSet<QString> itemsIn(const QString &versionGroup);

// NPC 가르침 기술의 비용(컨텐츠 + 재화: "배틀프런티어 48BP"). 사전에 없으면 빈 문자열.
// PokéAPI에는 이 데이터가 없어서 입수 사전의 tutors로만 안다.
QString tutorCost(const QString &versionGroup, const QString &move, Language language);

// 장소 이름: 사전(한국어) → 도로 · 수로 번호 규칙("201번 도로") → PokéAPI 이름(대체 순서)
QString placeName(const QString &identifier, const LocalizedText &pokeapiName, Language language);

// 출현 방법 이름. 사전에 없으면 identifier를 사람이 읽기 좋게("rock-smash" → "rock smash")
QString methodName(const QString &identifier, Language language);
} // namespace com::yamada::studio::guidebook
