#include "ui/dex/naturepicker.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr int kHeaderWidth = 58;
constexpr int kCellWidth = 64;
constexpr int kHeaderHeight = 24;
constexpr int kCellHeight = 28;
constexpr int kFooterHeight = 30;
constexpr int kPadding = 8;
constexpr int kStats = 5; // 공격 · 방어 · 특공 · 특방 · 스피드(stats.id 2–6)
constexpr int kNoneCell = 25;
} // namespace

namespace com::yamada::studio {
NaturePicker::NaturePicker(const QList<Nature> &natures, int currentId, Language language,
                           QWidget *parent)
    : QWidget(parent, Qt::Popup) // 팝업 창: 바깥을 누르거나 Esc면 닫힌다
    , m_natures(natures)
    , m_current(currentId)
    , m_language(language)
{
    QWidget::setAttribute(Qt::WA_DeleteOnClose); // 닫히면 스스로 지운다(한 번 쓰고 버리는 창)
    QWidget::setMouseTracking(true);
    QWidget::setCursor(cursors::pointer());
    QWidget::resize(sizeHint());
}

QSize NaturePicker::sizeHint() const
{
    return {2 * kPadding + kHeaderWidth + kStats * kCellWidth,
            2 * kPadding + kHeaderHeight + kStats * kCellHeight + kFooterHeight};
}

int NaturePicker::natureAt(int row, int column) const
{
    for (const Nature &n : m_natures)
        if (n.increasedStat == row + 2 && n.decreasedStat == column + 2)
            return n.id;
    return 0;
}

int NaturePicker::cellAt(const QPoint &pos) const
{
    const int x = pos.x() - kPadding - kHeaderWidth;
    const int y = pos.y() - kPadding - kHeaderHeight;
    if (x >= 0 && y >= 0 && x < kStats * kCellWidth && y < kStats * kCellHeight)
        return (y / kCellHeight) * kStats + x / kCellWidth;
    if (y >= kStats * kCellHeight && y < kStats * kCellHeight + kFooterHeight)
        return kNoneCell;
    return -1;
}

void NaturePicker::paintEvent(QPaintEvent *)
{
    const QString stats[kStats] = {tr("공격"), tr("방어"), tr("특공"), tr("특방"), tr("스피드")};
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 창: 흰 바탕 · 먹선 2 · 반경 6
    painter.setPen(QPen(QColor(tok::kInk), 2));
    painter.setBrush(QColor(tok::kWhite));
    painter.drawRoundedRect(QRectF(rect()).adjusted(1, 1, -1, -1), 6, 6);

    const QFont headerFont = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    const QFont cellFont = theme::font(theme::kFamilyBody, 12, QFont::Bold);
    const int left = kPadding + kHeaderWidth;
    const int top = kPadding + kHeaderHeight;
    painter.setFont(headerFont);
    for (int i = 0; i < kStats; ++i) { // 머리: ▼ 내리는(칸) · ▲ 오르는(줄)
        painter.setPen(QColor(tok::kBlueDeep));
        painter.drawText(QRect(left + i * kCellWidth, kPadding, kCellWidth, kHeaderHeight),
                         Qt::AlignCenter, QStringLiteral("▼") + stats[i]);
        painter.setPen(QColor(tok::kRed));
        painter.drawText(QRect(kPadding, top + i * kCellHeight, kHeaderWidth - 4, kCellHeight),
                         Qt::AlignRight | Qt::AlignVCenter, QStringLiteral("▲") + stats[i]);
    }
    for (int row = 0; row < kStats; ++row) {
        for (int column = 0; column < kStats; ++column) {
            const int id = natureAt(row, column);
            const QRectF cell(left + column * kCellWidth + 2, top + row * kCellHeight + 2,
                              kCellWidth - 4, kCellHeight - 4);
            const bool current = id != 0 && id == m_current;
            const bool hovered = m_hover == row * kStats + column;
            if (current || hovered) {
                painter.setPen(current ? QPen(QColor(tok::kInk), 1.5) : Qt::NoPen);
                painter.setBrush(QColor(current ? tok::kYellow : tok::kYellowTint));
                painter.drawRoundedRect(cell, 4, 4);
            }
            QString name;
            for (const Nature &n : m_natures)
                if (n.id == id)
                    name = n.name.text(m_language);
            painter.setFont(cellFont);
            painter.setPen(
                    QColor(row == column ? tok::kTextDisabled : tok::kText1)); // 대각선 = 무보정
            painter.drawText(cell, Qt::AlignCenter, name);
        }
    }
    // [성격 없음]
    const QRectF none(left + 2, top + kStats * kCellHeight + 4, kStats * kCellWidth - 4,
                      kFooterHeight - 6);
    if (m_hover == kNoneCell) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(tok::kPaperAlt));
        painter.drawRoundedRect(none, 4, 4);
    }
    painter.setFont(cellFont);
    painter.setPen(QColor(tok::kText2));
    painter.drawText(none, Qt::AlignCenter, tr("성격 없음"));
}

void NaturePicker::mouseMoveEvent(QMouseEvent *event)
{
    const int cell = cellAt(event->position().toPoint());
    if (cell != m_hover) {
        m_hover = cell;
        QWidget::update();
    }
}

void NaturePicker::mousePressEvent(QMouseEvent *event)
{
    const int cell = cellAt(event->position().toPoint());
    if (cell < 0) {
        QWidget::mousePressEvent(event); // 팝업 바깥 → Qt가 닫는다
        return;
    }
    emit natureChosen(cell == kNoneCell ? 0 : natureAt(cell / kStats, cell % kStats));
    QWidget::close();
}

} // namespace com::yamada::studio
