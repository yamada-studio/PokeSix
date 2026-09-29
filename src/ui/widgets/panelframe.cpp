#include "ui/widgets/panelframe.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
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

void PanelFrame::setTitle(const QString &title, const QString &detail)
{
    m_title = title;
    m_detail = detail;
    QWidget::update();
}

void PanelFrame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    paintPanel(painter, rect(), m_style);

    if (m_style.header <= 0 || m_title.isEmpty())
        return;
    // 머리 글자: 왼쪽 여백 14, 머리 띠의 세로 가운데 (Dex.dc.html: padding 0 8 0 14, gap 8)
    const QRectF band(m_style.outline + 14, m_style.outline, width() - 2 * m_style.outline - 22,
                      m_style.header);
    const QFont titleFont = theme::font(theme::kFamilyTitle, 20);
    const QFont detailFont = theme::font(theme::kFamilyData, 13, QFont::Bold);
    painter.setPen(QColor(tok::kWhite));
    painter.setFont(titleFont);
    painter.drawText(band, Qt::AlignLeft | Qt::AlignVCenter, m_title);
    if (!m_detail.isEmpty()) {
        const qreal x = band.left() + QFontMetricsF(titleFont).horizontalAdvance(m_title) + 8;
        painter.setFont(detailFont);
        painter.drawText(QRectF(x, band.top(), band.right() - x, band.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, m_detail);
    }
}
} // namespace com::yamada::studio
