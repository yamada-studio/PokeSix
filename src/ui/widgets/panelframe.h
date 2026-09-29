#pragma once

#include "ui/widgets/panelpainter.h"

#include <QWidget>

class QVBoxLayout;

namespace com::yamada::studio {
// 창의 "겉모양"(먹선 · 그림자 · 안쪽 선). 내용 위젯은 setBody()로 받아 안쪽에 끼운다.
// 내용 위젯은 자기가 창 안에 있다는 걸 모른다(ADR 0007 3항). 같은 내용을 나중에 드로어나 모달에도
// 넣을 수 있다.
class PanelFrame : public QWidget
{
    Q_OBJECT
public:
    explicit PanelFrame(QWidget *parent = nullptr);

    // 한 번만 부른다. body의 소유권은 PanelFrame이 가져간다(레이아웃에 넣으면 부모가 this로
    // 바뀐다).
    void setBody(QWidget *body);

    // 모양을 바꾼다. 레이아웃 margins도 겉모양 두께(chromeMargins)에 맞춰 함께 바뀐다.
    void setPanelStyle(const PanelStyle &style);
    const PanelStyle &panelStyle() const { return m_style; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVBoxLayout *m_layout = nullptr;
    PanelStyle m_style;
};
} // namespace com::yamada::studio
