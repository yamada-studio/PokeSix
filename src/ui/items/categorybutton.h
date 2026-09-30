#pragma once

#include "ui/theme/itemstyle.h"

#include <QAbstractButton>
#include <QFont>

namespace com::yamada::studio {
// 아이템 대백과 분류 창의 한 줄: [▶] [색 견본] 회복 (Items.dc.html의 분류 버튼, E3).
//   높이 44 · 좌우 여백 10 · 칸 사이 10 · 반경 6 · 도현 18
//   선택됨 = 바탕 yellow.tint + 먹선 2 + ▶ / hover = 바탕 paper.alt / 키보드 포커스 = 파랑 테
// 체크 가능한 버튼이라 선택 · 클릭 · 키보드는 QAbstractButton이 한다. 묶음 이름 · 색은
// itemstyle.json. 새 시그널은 없지만 Q_OBJECT를 둔다 — qobject_cast · findChildren<CategoryButton
// *>가 메타 정보(moc)로 타입을 가려내기 때문이다.
class CategoryButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit CategoryButton(const itemstyle::Group &group, QWidget *parent = nullptr);

    const QString &groupKey() const { return m_group.key; }
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    itemstyle::Group m_group;
    QFont m_font;
};
} // namespace com::yamada::studio
