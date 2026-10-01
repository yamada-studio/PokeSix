#pragma once

#include <QAbstractButton>

namespace com::yamada::studio {
// 작은 "글자 ▾" 버튼(먹선 1.5 · 반경 5 · 흰 바탕, 마우스가 오르면 노랑 연한 바탕). 누르면 메뉴 ·
// 팝업을 여는 자리에 쓴다(도감 상세의 성격, 스쿼드의 게임 · 특성 · 성격).
class DropdownButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit DropdownButton(const QString &text = QString(), QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};
} // namespace com::yamada::studio
