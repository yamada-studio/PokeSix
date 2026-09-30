#pragma once

#include <QHeaderView>

namespace com::yamada::studio {
// 도감 목록의 머리 칸(th). 기본 QHeaderView 대신 직접 그린다. 이유는 두 가지다:
//   1) 글자 자리: 데이터 칸과 **같은 함수**(DexRowDelegate::contentRect · alignment)로 계산해서,
//   머리
//      글자와 데이터가 px 단위로 같은 선에 선다. (기본 머리는 QSS padding · 스타일 여백으로 따로
//      계산)
//   2) 정렬 화살표: 기본 머리는 화살표 자리를 글자 칸에서 빼서, 오른쪽 정렬 칸을 정렬하면 글자가
//      왼쪽으로 밀린다. 여기서는 화살표를 글자의 **정렬 반대쪽**에 붙인다 — 왼쪽 정렬이면 글자 뒤,
//      오른쪽 정렬이면 글자 앞("▼합계"). 그래서 글자의 기준선(왼쪽 끝 · 오른쪽 끝)은 움직이지
//      않는다.
// 모양(01 §5-4 표 머리): 흰 바탕 · 나눔고딕 11 ExtraBold text.2 · 아래 먹선 2 · 정렬 중인 칸은
// 빨강.
class DexHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit DexHeaderView(QWidget *parent = nullptr);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
};
} // namespace com::yamada::studio
