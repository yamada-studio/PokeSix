#pragma once

#include "data/text/localizedtext.h"

#include <QColor>
#include <QList>

// 홈(인트로) 세대 카드의 내용: 마스코트 포켓몬 · 윗띠 색 · 지방 이름.
// 값은 설정 파일 resources/theme/homecards.json(qrc ":/theme/homecards.json")에 있다 —
// 마스코트를 바꾸거나 세대가 늘 때 코드를 다시 컴파일하지 않아도 되게(dexstyle.json과 같은 방식).
namespace com::yamada::studio::homecards {
struct Card
{
    int generation = 0;
    QList<int> pokemon; // 카드의 포켓몬들(전국 번호). 2번째가 가운데 큰 자리 — 그 세대 스타팅 셋
    int rangeFrom = 0;  // 그 세대 전국도감 구간(No.387–493)
    int rangeTo = 0;
    QColor accent; // 윗띠 · 배경 패턴 색. 파일에 없으면 흰색
    LocalizedText region;
};

// 세대 순서대로. 파일이 없거나 깨졌으면 빈 목록(홈이 카드 없이 뜬다 — 앱은 돈다).
const QList<Card> &all();
} // namespace com::yamada::studio::homecards
