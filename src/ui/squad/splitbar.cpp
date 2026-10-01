#include "ui/squad/splitbar.h"

#include "ui/squad/squadpaint.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kBarHeight = 22;
} // namespace

namespace com::yamada::studio {
SplitBar::SplitBar(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void SplitBar::setSplit(const MoveSplit &split, const MoveSplit &other,
                        const QString &otherRuleText)
{
    m_split = split;
    m_other = other;
    m_otherRuleText = otherRuleText;
    QWidget::update();
}

void SplitBar::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int total = m_split.physical + m_split.special + m_split.status + m_split.empty;
    const QRectF bar(1, 1, width() - 2, kBarHeight);

    // 막대: 칸 수 비율로 나눈다(테 안쪽을 잘라 모서리를 둥글게)
    QPainterPath clip;
    clip.addRoundedRect(bar, 4, 4);
    painter.save();
    painter.setClipPath(clip);
    painter.fillRect(bar, QColor(tok::kWhite));
    struct Segment
    {
        int count;
        QRgb fill; // 0 = 사선(빈칸)
    };
    const Segment segments[] = {{m_split.physical, tok::kCatPhysical},
                                {m_split.special, tok::kCatSpecial},
                                {m_split.status, tok::kCatStatus},
                                {m_split.empty, 0}};
    qreal x = bar.left();
    for (const Segment &s : segments) {
        if (total == 0 || s.count == 0)
            continue;
        const qreal w = bar.width() * s.count / total;
        const QRectF part(x, bar.top(), w, bar.height());
        if (s.fill == 0)
            squadpaint::paintHatch(painter, part);
        else
            painter.fillRect(part, QColor(s.fill));
        painter.setPen(QPen(QColor(tok::kInk), 1.5));
        if (x > bar.left())
            painter.drawLine(QPointF(x, bar.top()), QPointF(x, bar.bottom()));
        x += w;
    }
    painter.restore();
    painter.setPen(QPen(QColor(tok::kInk), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(bar, 4, 4);

    // 범례(왼쪽) · 반대 규칙(오른쪽)
    const int top = kBarHeight + 10;
    const QFont font = theme::font(theme::kFamilyBody, 12, QFont::Bold);
    const QFont number = theme::font(theme::kFamilyData, 12, QFont::Bold);
    struct Legend
    {
        QRgb fill;
        QString text;
        int count;
    };
    const Legend legend[] = {{tok::kCatPhysical, tr("물리"), m_split.physical},
                             {tok::kCatSpecial, tr("특수"), m_split.special},
                             {tok::kCatStatus, tr("변화"), m_split.status},
                             {0, tr("빈칸"), m_split.empty}};
    int lx = 1;
    for (const Legend &l : legend) {
        const QRectF swatch(lx, top + 4, 12, 12);
        if (l.fill == 0) {
            painter.setPen(QPen(QColor(tok::kInk), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
        } else {
            painter.setPen(QPen(QColor(tok::kInk), 1));
            painter.setBrush(QColor(l.fill));
        }
        painter.drawRect(swatch);
        painter.setFont(font);
        painter.setPen(QColor(tok::kText2));
        const int tw = int(QFontMetricsF(font).horizontalAdvance(l.text));
        painter.drawText(QRect(lx + 17, top, tw + 2, 20), Qt::AlignLeft | Qt::AlignVCenter, l.text);
        painter.setFont(number);
        painter.setPen(QColor(tok::kText1));
        const QString n = QString::number(l.count);
        const int nw = int(QFontMetricsF(number).horizontalAdvance(n));
        painter.drawText(QRect(lx + 21 + tw, top, nw + 2, 20), Qt::AlignLeft | Qt::AlignVCenter, n);
        lx += 21 + tw + nw + 16;
    }
    if (!m_otherRuleText.isEmpty()) {
        const QString value = tr("물리 %1 · 특수 %2").arg(m_other.physical).arg(m_other.special);
        painter.setFont(number);
        painter.setPen(QColor(tok::kText1));
        const int vw = int(QFontMetricsF(number).horizontalAdvance(value));
        painter.drawText(QRect(width() - vw - 2, top, vw + 2, 20),
                         Qt::AlignRight | Qt::AlignVCenter, value);
        painter.setFont(theme::font(theme::kFamilyBody, 12));
        painter.setPen(QColor(tok::kText3));
        const int room = width() - vw - 8 - lx;
        if (room > 60)
            painter.drawText(
                    QRect(lx, top, room, 20), Qt::AlignRight | Qt::AlignVCenter,
                    QFontMetricsF(painter.font()).elidedText(m_otherRuleText, Qt::ElideLeft, room));
    }
}
} // namespace com::yamada::studio
