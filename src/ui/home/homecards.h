#pragma once

#include "data/text/localizedtext.h"

#include <QColor>
#include <QList>

// 홈(인트로) 세대 카드의 내용: 시리즈별 스타팅 층 · 윗띠 색 · 지방 이름들.
// 값은 설정 파일 resources/theme/homecards.json(qrc ":/theme/homecards.json")에 있다 —
// 마스코트를 바꾸거나 세대가 늘 때 코드를 다시 컴파일하지 않아도 되게(dexstyle.json과 같은 방식).
namespace com::yamada::studio::homecards {
struct Card
{
    int generation = 0;
    // 시리즈별 포켓몬 층(전국 번호). 첫 층 = 그 세대 첫 시리즈(맨 앞 · 바닥), 다음 층은 나온
    // 순서대로 한 층씩 뒤 · 위로(4세대: 신오 DP·Pt 스타팅 → 성도 HGSS 스타팅). 3마리면 2번째가
    // 가운데
    QList<QList<int>> layers;
    int rangeFrom = 0; // 그 세대 전국도감 구간(No.387–493)
    int rangeTo = 0;
    QColor accent;                // 윗띠 · 배경 패턴 색. 파일에 없으면 흰색
    QList<LocalizedText> regions; // 그 세대 시리즈들의 지방(신오 · 성도 · 관동)
};

// 세대 순서대로. 파일이 없거나 깨졌으면 빈 목록(홈이 카드 없이 뜬다 — 앱은 돈다).
const QList<Card> &all();
} // namespace com::yamada::studio::homecards
