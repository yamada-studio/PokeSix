#include "ui/dex/statbars.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QPainter>

namespace {
constexpr int kRowHeight = 24;
constexpr int kLabelWidth = 56;
constexpr int kValueWidth = 36;
constexpr int kGap = 8;
constexpr int kBarHeight = 12;
constexpr int kMaxStat = 255; // 종족값의 최댓값(해피너스 HP)

QRgb colorOf(int value)
{
    using namespace com::yamada::studio;
    if (value < 60)
        return tok::kStatLow;
    if (value < 90)
        return tok::kStatMid;
    if (value < 120)
        return tok::kStatGood;
    return tok::kStatHigh;
}
} // namespace

namespace com::yamada::studio {
StatBars::StatBars(QWidget *parent)
    : QWidget(parent)
    , m_labelFont(theme::font(theme::kFamilyBody, 12, QFont::ExtraBold))
    , m_valueFont(theme::font(theme::kFamilyData, 13, QFont::Bold))
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void StatBars::setStats(const std::array<int, 6> &stats, int total)
{
    m_stats = stats;
    m_total = total;
    QWidget::update();
}

QSize StatBars::sizeHint() const
{
    return {kLabelWidth + kValueWidth + 2 * kGap + 160, 7 * kRowHeight};
}

void StatBars::paintEvent(QPaintEvent *)
{
    const QString labels[]
            = {tr("HP"), tr("공격"), tr("방어"), tr("특공"), tr("특방"), tr("스피드")};
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int barLeft = kLabelWidth + kValueWidth + 2 * kGap;
    const int barWidth = std::max(0, width() - barLeft);
    for (int i = 0; i < 6; ++i) {
        const QRect row(0, i * kRowHeight, width(), kRowHeight);
        painter.setFont(m_labelFont);
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(row.left(), row.top(), kLabelWidth, row.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, labels[i]);
        painter.setFont(m_valueFont);
        painter.setPen(QColor(tok::kText1));
        painter.drawText(QRect(kLabelWidth + kGap, row.top(), kValueWidth, row.height()),
                         Qt::AlignRight | Qt::AlignVCenter, QString::number(m_stats[i]));
        // 막대: 바탕(paper.alt + 먹선) 위에 값만큼 색
        const QRectF track(barLeft, row.center().y() - kBarHeight / 2.0, barWidth, kBarHeight);
        painter.setPen(QPen(QColor(tok::kInk), 1.5));
        painter.setBrush(QColor(tok::kPaperAlt));
        painter.drawRoundedRect(track.adjusted(0.75, 0.75, -0.75, -0.75), 3, 3);
        const qreal filled = (track.width() - 3) * std::min(m_stats[i], kMaxStat) / kMaxStat;
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(colorOf(m_stats[i])));
        painter.drawRoundedRect(
                QRectF(track.left() + 1.5, track.top() + 1.5, filled, track.height() - 3), 2, 2);
    }
    // 합계
    const QRect row(0, 6 * kRowHeight, width(), kRowHeight);
    painter.setFont(m_labelFont);
    painter.setPen(QColor(tok::kText2));
    painter.drawText(QRect(0, row.top(), kLabelWidth, row.height()),
                     Qt::AlignLeft | Qt::AlignVCenter, tr("합계"));
    painter.setFont(theme::font(theme::kFamilyData, 14, QFont::Bold));
    painter.setPen(QColor(tok::kRed));
    painter.drawText(QRect(kLabelWidth + kGap, row.top(), kValueWidth, row.height()),
                     Qt::AlignRight | Qt::AlignVCenter, QString::number(m_total));
}
} // namespace com::yamada::studio
