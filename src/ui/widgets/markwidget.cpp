#include "ui/widgets/markwidget.h"

#include <QPainter>
#include <QSvgRenderer>

namespace com::yamada::studio {
MarkWidget::MarkWidget(QWidget *parent)
    : QWidget(parent)
    // SVG는 한 번만 읽어 둔다. 부모를 this로 주면 object tree가 수명을 관리한다(delete 불필요).
    , m_renderer(new QSvgRenderer(QStringLiteral(":/icons/mark/pokesix-mark.svg"), this))
{
    // 레이아웃이 늘리거나 줄이지 않고 sizeHint 크기 그대로 두게 한다.
    QWidget::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void MarkWidget::setMarkSize(int size)
{
    m_size = size;
    QWidget::updateGeometry(); // "내 sizeHint가 바뀌었다"고 레이아웃에 알린다
    QWidget::update();
}

QSize MarkWidget::sizeHint() const
{
    return {m_size, m_size};
}

void MarkWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    m_renderer->render(&painter, QRectF(rect())); // 벡터라서 어느 크기로 그려도 깨지지 않는다
}
} // namespace com::yamada::studio
