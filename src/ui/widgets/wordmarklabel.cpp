#include "ui/widgets/wordmarklabel.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QtMath>

namespace {
constexpr int kTextPx
        = com::yamada::studio::tok::kSizeIntroWordmark; // 80 — CSS font-size, line-height 1
constexpr int kShadow = 5;                              // text-shadow: 5px 5px 0
constexpr qreal kLetterSpacing = kTextPx * 0.02;        // letter-spacing: 0.02em
} // namespace

namespace com::yamada::studio {
WordmarkLabel::WordmarkLabel(QWidget *parent)
    : QWidget(parent)
    , m_font(theme::font(theme::kFamilyPixel, kTextPx, QFont::Bold))
{
    m_font.setLetterSpacing(QFont::AbsoluteSpacing, kLetterSpacing);
    QWidget::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

QSize WordmarkLabel::sizeHint() const
{
    // 가로: 글자 폭 양옆에 그림자 폭만큼 여유를 둔다. 그림자는 오른쪽에만 생기지만
    //       양쪽에 두어야 가운데 정렬했을 때 "글자"가 정확히 가운데에 온다(CSS와 같게).
    // 세로: CSS 줄 상자(80) + 아래 그림자. 위젯은 자기 rect 밖에 그릴 수 없다(ADR 0007).
    const QFontMetricsF metrics(m_font);
    const int textWidth = qCeil(metrics.horizontalAdvance(QStringLiteral("POKESIX")));
    return {textWidth + 2 * kShadow, kTextPx + kShadow};
}

void WordmarkLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::TextAntialiasing, true);
    painter.setFont(m_font);

    // CSS line-height: 1 → 80px 줄 상자 안에서 글꼴의 (ascent + descent)를 세로 가운데에 둔다
    // (브라우저의 half-leading 규칙). 그 위치의 기준선(baseline)에 글자를 쓴다.
    const QFontMetricsF metrics(m_font);
    const qreal baseline
            = (kTextPx - (metrics.ascent() + metrics.descent())) / 2.0 + metrics.ascent();
    const QPointF origin(kShadow, baseline);

    const QString poke = QStringLiteral("POKE");
    const QString six = QStringLiteral("SIX");
    const QPointF sixOffset(metrics.horizontalAdvance(poke), 0);

    // 1) 그림자: 같은 글자를 (5, 5) 옮겨 노랑으로 먼저 그린다.
    painter.setPen(QColor(tok::kIntroTitleShadow));
    painter.drawText(origin + QPointF(kShadow, kShadow), poke + six);

    // 2) 본 글자: POKE는 본문 글자색(text.1), SIX는 빨강.
    painter.setPen(QColor(tok::kText1));
    painter.drawText(origin, poke);
    painter.setPen(QColor(tok::kRed));
    painter.drawText(origin + sixOffset, six);
}
} // namespace com::yamada::studio
