#pragma once

#include "data/text/localizedtext.h"
#include "ui/theme/tokens.h"

#include <QRectF>
#include <QString>

class QPainter;
class QPixmap;

// 스쿼드 화면의 카드 · 히트맵 · 문제 목록이 같이 쓰는 그리기 조각.
namespace com::yamada::studio::squadpaint {
// 타입 약칭: 한국어는 한 글자(tokens.h의 abbr), 다른 언어는 칩 이름 앞 글자
QString typeAbbr(const tok::TypeColor &type, Language language);
// 받는 배율 글자: 4 · 2 · ½ · ¼ · 0 (1은 빈 글자)
QString multiplierText(double multiplier);
// 기술 분류 약칭: 물 · 특 · 변
QString damageClassAbbr(int damageClass);

// 타입 색 네모(약칭 글자). 히트맵 머리 · 문제 목록 · 약점 줄
void paintTypeBox(QPainter &painter, const QRectF &rect, const tok::TypeColor &type,
                  const QString &text, int fontSize = 11);
// 분류 배지: 물리 주황 · 특수 파랑 · 변화 회색
void paintDamageClass(QPainter &painter, const QRectF &rect, int damageClass);
// 사선 무늬(세대에 없는 타입 칸 · 빈 기술 칸)
void paintHatch(QPainter &painter, const QRectF &rect);

// 박스 아이콘 파일을 투명 여백 없이 height 높이로 — 그대로 쓰면 작은 포켓몬이 캔버스 구석에
// 조그맣게 나온다(비전셔틀 버튼 · 창). 파일을 못 읽으면 빈 QPixmap
QPixmap trimmedIcon(const QString &path, int height);
} // namespace com::yamada::studio::squadpaint
