#pragma once

#include <QHeaderView>

namespace com::yamada::studio {
// 아이템 목록의 머리 칸. DexHeaderView와 같은 모양: 흰 바탕 · 나눔고딕 11 ExtraBold text.2 ·
// 아래 먹선 2 · 정렬 중인 칸은 빨강 + 화살표. 글자 칸은 왼쪽, 가격 칸은 오른쪽 정렬(데이터와 같게).
// 화살표는 글자의 정렬 반대쪽에 붙여 글자 끝선이 데이터와 어긋나지 않게 한다.
class ItemHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit ItemHeaderView(QWidget *parent = nullptr);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;
};
} // namespace com::yamada::studio
