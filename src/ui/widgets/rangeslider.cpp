#include "ui/widgets/rangeslider.h"

#include "ui/theme/cursors.h"
#include "ui/theme/tokens.h"

#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr int kHeight = 24;
constexpr qreal kTrack = 4;                 // 트랙 두께
constexpr qreal kHandle = 14;               // 손잡이 한 변
constexpr qreal kSidePad = kHandle / 2 + 1; // 손잡이가 위젯 밖으로 나가지 않게
} // namespace

namespace com::yamada::studio {
RangeSlider::RangeSlider(int minimum, int maximum, int step, QWidget *parent)
    : QWidget(parent)
    , m_minimum(minimum)
    , m_maximum(maximum)
    , m_step(std::max(1, step))
    , m_lower(minimum)
    , m_upper(maximum)
{
    QWidget::setCursor(cursors::pointer());
}

QSize RangeSlider::sizeHint() const
{
    return {160, kHeight};
}

void RangeSlider::setValues(int lower, int upper)
{
    const auto snap = [this](int value) {
        const int snapped = m_minimum + (value - m_minimum + m_step / 2) / m_step * m_step;
        return std::clamp(snapped, m_minimum, m_maximum);
    };
    lower = snap(lower);
    upper = snap(upper);
    if (lower > upper)
        std::swap(lower, upper);
    if (lower == m_lower && upper == m_upper)
        return;
    m_lower = lower;
    m_upper = upper;
    QWidget::update();
    emit valuesChanged(m_lower, m_upper);
}

qreal RangeSlider::xOf(int value) const
{
    const qreal span = width() - 2 * kSidePad;
    return kSidePad + span * (value - m_minimum) / qreal(m_maximum - m_minimum);
}

int RangeSlider::valueAt(qreal x) const
{
    const qreal span = width() - 2 * kSidePad;
    const qreal ratio = std::clamp((x - kSidePad) / span, 0.0, 1.0);
    const int raw = m_minimum + qRound(ratio * (m_maximum - m_minimum));
    return m_minimum + (raw - m_minimum + m_step / 2) / m_step * m_step;
}

QRectF RangeSlider::handleRect(int value) const
{
    return {xOf(value) - kHandle / 2, (height() - kHandle) / 2.0, kHandle, kHandle};
}

void RangeSlider::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const qreal x = event->position().x();
    // 가까운 손잡이를 잡는다. 같은 자리면(둘이 겹침) 움직일 방향 쪽 손잡이.
    const qreal toLower = std::abs(x - xOf(m_lower));
    const qreal toUpper = std::abs(x - xOf(m_upper));
    if (toLower < toUpper)
        m_dragging = Handle::Lower;
    else if (toUpper < toLower)
        m_dragging = Handle::Upper;
    else
        m_dragging = x < xOf(m_lower) ? Handle::Lower : Handle::Upper;
    mouseMoveEvent(event);
}

void RangeSlider::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging == Handle::None)
        return;
    const int value = valueAt(event->position().x());
    if (m_dragging == Handle::Lower)
        setValues(std::min(value, m_upper), m_upper);
    else
        setValues(m_lower, std::max(value, m_lower));
}

void RangeSlider::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    m_dragging = Handle::None;
}

void RangeSlider::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const qreal mid = height() / 2.0;
    // 트랙과 고른 구간
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kLine));
    painter.drawRoundedRect(QRectF(kSidePad, mid - kTrack / 2, width() - 2 * kSidePad, kTrack), 2,
                            2);
    painter.setBrush(QColor(tok::kYellow));
    painter.drawRoundedRect(
            QRectF(xOf(m_lower), mid - kTrack / 2, xOf(m_upper) - xOf(m_lower), kTrack), 2, 2);

    // 손잡이: 흰 바탕 · 먹선 1.5 · 아래 그림자 1
    for (const int value : {m_lower, m_upper}) {
        const QRectF handle = handleRect(value);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(tok::kInk));
        painter.drawRoundedRect(handle.translated(0, 1.5), 3, 3);
        painter.setPen(QPen(QColor(tok::kInk), 1.5));
        painter.setBrush(QColor(tok::kWhite));
        painter.drawRoundedRect(handle, 3, 3);
    }
}
} // namespace com::yamada::studio
