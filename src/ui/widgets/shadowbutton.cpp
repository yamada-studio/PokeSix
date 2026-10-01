#include "ui/widgets/shadowbutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
constexpr int kShadow = 3;        // box-shadow: 0 3px 0 ink
constexpr int kBorder = 2;        // border: 2px solid ink
constexpr int kRadius = 6;        // border-radius: 6px
constexpr int kPaddingX = 16;     // 글자 양옆 여백
constexpr int kIconSize = 15;     // 아이콘 15px
constexpr int kIconGap = 6;       // 아이콘과 글자 사이 gap: 6px
constexpr int kPressedOffset = 2; // 눌림: 2px 내려앉는다
constexpr qreal kDisabledOpacity = 0.45;
} // namespace

namespace com::yamada::studio {
ShadowButton::ShadowButton(Variant variant, QWidget *parent)
    : QAbstractButton(parent)
    , m_variant(variant)
    , m_font(theme::font(theme::kFamilyTitle, variant == Variant::Primary ? 18 : 17))
{
    QAbstractButton::setCursor(cursors::pointer());
    // 가로는 들어갈 칸만큼 늘어나도 되고(패널에서는 폭 전체), 세로는 버튼 높이 + 그림자로 고정.
    QAbstractButton::setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    QAbstractButton::setIconSize(QSize(kIconSize, kIconSize));
}

int ShadowButton::boxHeight() const
{
    return m_variant == Variant::Primary ? 40 : 36;
}

QSize ShadowButton::sizeHint() const
{
    const QFontMetricsF metrics(m_font);
    qreal width = 2 * (kBorder + kPaddingX) + metrics.horizontalAdvance(QAbstractButton::text());
    if (!QAbstractButton::icon().isNull())
        width += kIconSize + kIconGap;
    return {qCeil(width), boxHeight() + kShadow};
}

void ShadowButton::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        QAbstractButton::animateClick(); // 잠깐 눌린 모양을 보여 주고 clicked()를 보낸다
        return;
    }
    QAbstractButton::keyPressEvent(event);
}

void ShadowButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (!QAbstractButton::isEnabled())
        painter.setOpacity(kDisabledOpacity);

    const bool down = QAbstractButton::isDown();
    const bool primary = m_variant == Variant::Primary;
    const QRectF box(0, down ? kPressedOffset : 0, width(), boxHeight());

    // 1) 그림자 — 눌리면 그리지 않는다(버튼이 그림자 위로 내려앉은 모양)
    if (!down) {
        QPainterPath shadow;
        shadow.addRoundedRect(box.translated(0, kShadow), kRadius, kRadius);
        painter.fillPath(shadow, QColor(tok::kInk));
    }

    // 2) 본체: 채움 + 먹선. 펜은 선 가운데를 따라가므로 반 폭 안쪽으로 들인다.
    const qreal half = kBorder / 2.0;
    QColor fill = primary ? QColor(tok::kRed) : QColor(tok::kWhite);
    if (down)
        fill = primary ? QColor(tok::kRedDeep) : QColor(tok::kPaper); // 눌림은 한 톤 어둡게
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(fill);
    painter.drawRoundedRect(box.adjusted(half, half, -half, -half), kRadius - half, kRadius - half);

    // 3) 아이콘 + 글자를 한 묶음으로 가운데에
    const QFontMetricsF metrics(m_font);
    const QString label = QAbstractButton::text();
    const bool hasIcon = !QAbstractButton::icon().isNull();
    const qreal contentWidth
            = metrics.horizontalAdvance(label) + (hasIcon ? kIconSize + kIconGap : 0);
    qreal x = box.center().x() - contentWidth / 2.0;
    if (hasIcon) {
        const QRectF iconRect(x, box.center().y() - kIconSize / 2.0, kIconSize, kIconSize);
        QAbstractButton::icon().paint(&painter, iconRect.toAlignedRect());
        x += kIconSize + kIconGap;
    }
    painter.setFont(m_font);
    painter.setPen(primary ? QColor(tok::kWhite) : QColor(tok::kText1));
    painter.drawText(QRectF(x, box.top(), box.right() - x, box.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, label);
}
} // namespace com::yamada::studio
