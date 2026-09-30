#pragma once

#include <QHeaderView>

namespace com::yamada::studio {
// 아이템 목록의 머리 칸. 세대 칸 열에는 "세대" 글자 대신 1–9 숫자를 세대 칸 위치에 맞춰 그리고,
// 지금 세대 숫자는 노란 바탕으로 한다(Items.dc.html genHead). 나머지는 DexHeaderView와 같은 모양:
// 흰 바탕 · 나눔고딕 11 ExtraBold text.2 · 아래 먹선 2 · 정렬 중인 칸은 빨강 + 화살표.
class ItemHeaderView : public QHeaderView
{
    Q_OBJECT
public:
    explicit ItemHeaderView(QWidget *parent = nullptr);

    void setGeneration(int generation);

protected:
    void paintSection(QPainter *painter, const QRect &rect, int logicalIndex) const override;

private:
    int m_generation = 1;
};
} // namespace com::yamada::studio
