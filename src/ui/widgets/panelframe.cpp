#include "ui/widgets/panelframe.h"

#include <QPainter>
#include <QVBoxLayout>

namespace com::yamada::studio {
PanelFrame::PanelFrame(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    // 겉모양이 차지하는 두께만큼 안쪽으로 들여서 내용을 놓는다 — 내용은 안쪽 사각형만 받는다.
    m_layout->setContentsMargins(chromeMargins(m_style));
    m_layout->setSpacing(0);
}

void PanelFrame::setBody(QWidget *body)
{
    m_layout->addWidget(body); // body의 부모가 this로 바뀐다(object tree가 소유)
}

void PanelFrame::setPanelStyle(const PanelStyle &style)
{
    m_style = style;
    m_layout->setContentsMargins(chromeMargins(m_style));
    QWidget::update(); // 다시 그리기 예약. 실제 그리기는 이벤트 루프가 paintEvent를 불러서 한다
}

void PanelFrame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    paintPanel(painter, rect(), m_style);
}
} // namespace com::yamada::studio
