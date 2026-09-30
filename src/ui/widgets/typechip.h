#pragma once

#include "data/text/localizedtext.h"
#include "ui/theme/tokens.h"

#include <QFont>
#include <QRectF>
#include <QString>

class QPainter;

// 타입 칩 (디자인 시트 §2 · §5-3): 솔리드 채움 + 먹선 테두리, 글자색은 대비 4.5:1이 되는
// 쪽(tok::kTypes). 표 안의 SM 크기: 높이 20 · 좌우 여백 7 · 반경 4 · 먹선 1.5 · 나눔고딕 11
// ExtraBold. 위젯이 아니라 그리기 함수다(ADR 0007): 표의 delegate도, 나중의 TypeChip 위젯도 이
// 함수를 부른다.
namespace com::yamada::studio::typechip {
// PokéAPI identifier("dragon")로 색 · 이름을 찾는다. 모르는 타입이면 nullptr.
const tok::TypeColor *find(const QString &identifier);

QFont font(); // 칩 글자 글꼴
// 칩 글자: 한국어는 tokens.h, 영어 · 일본어는 typechip.cpp의 표(게임 표기)
QString label(const tok::TypeColor &type, Language language = Language::Korean);
qreal width(const tok::TypeColor &type, Language language = Language::Korean); // 높이는 kHeight
inline constexpr qreal kHeight = 20;
inline constexpr qreal kGap = 3; // 칩 사이

// rect.topLeft()에 칩 하나를 그린다. 그린 폭을 돌려준다(다음 칩의 x를 정하는 데 쓴다).
qreal paint(QPainter &painter, const QPointF &topLeft, const tok::TypeColor &type,
            Language language = Language::Korean);
} // namespace com::yamada::studio::typechip
