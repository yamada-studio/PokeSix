#include "ui/widgets/segmentprogress.h"

#include "ui/theme/tokens.h"

#include <QPainter>

namespace {
constexpr int kSegments = 10;
constexpr int kBorder = 2;
constexpr int kRadius = 4;
constexpr int kPadding = 3;
constexpr int kGap = 3;
constexpr int kCellHeight = 12;
constexpr qreal kCellRadius = 1;
} // namespace

namespace com::yamada::studio {
SegmentProgress::SegmentProgress(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SegmentProgress::setValue(int percent)
{
    percent = qBound(0, percent, 100);
    if (percent == m_percent)
        return;
    m_percent = percent;
    QWidget::update();
}

QSize SegmentProgress::sizeHint() const
{
    return {200, 2 * (kBorder + kPadding) + kCellHeight};
}

void SegmentProgress::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal half = kBorder / 2.0;
    painter.setPen(QPen(QColor(com::yamada::studio::tok::kInk), kBorder));
    painter.setBrush(QColor(tok::kWhite));
    painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), kRadius - half,
                            kRadius - half);

    const QRectF inner = QRectF(rect()).adjusted(kBorder + kPadding, kBorder + kPadding,
                                                 -(kBorder + kPadding), -(kBorder + kPadding));
    const qreal cellWidth = (inner.width() - (kSegments - 1) * kGap) / kSegments;
    const int filled = m_percent / 10; // 60% → 6칸
    painter.setPen(Qt::NoPen);
    for (int i = 0; i < kSegments; ++i) {
        painter.setBrush(QColor(i < filled ? tok::kStatGood : tok::kLineSoft));
        painter.drawRoundedRect(
                QRectF(inner.left() + i * (cellWidth + kGap), inner.top(), cellWidth, kCellHeight),
                kCellRadius, kCellRadius);
    }
}
} // namespace com::yamada::studio
