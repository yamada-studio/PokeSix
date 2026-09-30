#include "ui/widgets/panelframe.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QVBoxLayout>

namespace {
constexpr int kHeaderPaddingRight = 8; // 머리 띠 오른쪽 안쪽 여백 (Dex.dc.html: padding 0 8 0 14)
} // namespace

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

void PanelFrame::setHeaderWidget(QWidget *widget)
{
    // 레이아웃에 넣지 않으니 부모를 직접 정한다 → object tree에 들어가고(소멸도 함께), 이 위젯 위에
    // 그려진다. 부모가 이미 보이는 중이면 새 자식은 숨은 채로 생기므로 show()도 부른다.
    widget->setParent(this);
    m_headerWidget = widget;
    widget->show();
    placeHeaderWidget();
}

void PanelFrame::placeHeaderWidget()
{
    if (!m_headerWidget) // 머리 위젯이 없는 창(첫 실행 패널 등)도 resizeEvent를 탄다
        return;
    const QSize size = m_headerWidget->sizeHint();
    const int left = width() - m_style.outline - kHeaderPaddingRight - size.width();
    const int top = m_style.outline + (m_style.header - size.height()) / 2;
    m_headerWidget->setGeometry(left, top, size.width(), size.height());
}

bool PanelFrame::event(QEvent *event)
{
    // 머리 위젯의 sizeHint가 바뀌면(버튼 수가 달라짐) 그 위젯이 updateGeometry()를 부르고, Qt는
    // 부모인 여기로 LayoutRequest를 보낸다. 레이아웃이 관리하지 않는 자식이라 직접 자리를 다시
    // 잡는다.
    if (event->type() == QEvent::LayoutRequest)
        placeHeaderWidget();
    return QWidget::event(event);
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

void PanelFrame::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    placeHeaderWidget();
}
} // namespace com::yamada::studio
