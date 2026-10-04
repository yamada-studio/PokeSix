#include "ui/squad/problemlist.h"

#include "ui/squad/squadpaint.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>

namespace {
constexpr int kRowHeight = 46;
constexpr int kGap = 6;
constexpr int kEmptyHeight = 40;
} // namespace

namespace com::yamada::studio {
ProblemList::ProblemList(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setMouseTracking(true);
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void ProblemList::setRows(const QList<Row> &rows, const QString &emptyText, Language language)
{
    m_language = language;
    m_rows = rows;
    m_emptyText = emptyText;
    m_hover = -1;
    QWidget::updateGeometry();
    QWidget::update();
}

int ProblemList::heightForRows(int rows)
{
    return rows * (kRowHeight + kGap) - kGap;
}

QSize ProblemList::sizeHint() const
{
    if (m_rows.isEmpty())
        return {400, kEmptyHeight};
    return {400, heightForRows(int(m_rows.size()))};
}

int ProblemList::rowAt(const QPoint &pos) const
{
    const int row = pos.y() / (kRowHeight + kGap);
    if (row < 0 || row >= m_rows.size() || pos.y() % (kRowHeight + kGap) >= kRowHeight)
        return -1;
    return row;
}

void ProblemList::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position().toPoint());
    if (row == m_hover)
        return;
    m_hover = row;
    if (row >= 0)
        QWidget::setCursor(cursors::pointer());
    else
        QWidget::unsetCursor();
    QWidget::update();
    emit rowHovered(row);
}

void ProblemList::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_hover < 0)
        return;
    m_hover = -1;
    QWidget::update();
    emit rowHovered(-1);
}

void ProblemList::mousePressEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position().toPoint());
    if (row >= 0)
        emit rowClicked(row);
    else
        QWidget::mousePressEvent(event);
}

void ProblemList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_rows.isEmpty()) { // 문제 없음: 초록 띠
        const QRectF box = QRectF(rect()).adjusted(1, 1, -1, -1);
        painter.setPen(QPen(QColor(tok::kGreen), 1.5));
        painter.setBrush(QColor(tok::kGreenTint));
        painter.drawRoundedRect(box, 6, 6);
        painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
        painter.setPen(QColor(tok::kGreen));
        painter.drawText(box.adjusted(14, 0, -14, 0), Qt::AlignLeft | Qt::AlignVCenter,
                         m_emptyText);
        return;
    }
    const QFont titleFont = theme::font(theme::kFamilyBody, 13, QFont::ExtraBold);
    const QFont detailFont = theme::font(theme::kFamilyBody, 11);
    const QFont tagFont = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    for (qsizetype i = 0; i < m_rows.size(); ++i) {
        const Row &row = m_rows.at(i);
        const QRectF box(1, i * (kRowHeight + kGap) + 1, width() - 2, kRowHeight - 2);
        painter.setPen(QPen(QColor(tok::kRed), i == m_hover ? 2.5 : 1.5));
        painter.setBrush(QColor(tok::kRedTint));
        painter.drawRoundedRect(box, 6, 6);
        if (const tok::TypeColor *type = typechip::find(row.typeKey))
            squadpaint::paintTypeBox(painter,
                                     QRectF(box.left() + 10, box.center().y() - 13, 26, 26), *type,
                                     squadpaint::typeAbbr(*type, m_language), 11);
        // 태그(오른쪽)
        const QString tag = row.offense ? tr("공격") : tr("방어");
        const qreal tagWidth = QFontMetricsF(tagFont).horizontalAdvance(tag) + 16;
        const QRectF tagBox(box.right() - 10 - tagWidth, box.center().y() - 10, tagWidth, 20);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(tok::kRed));
        painter.drawRoundedRect(tagBox, 4, 4);
        painter.setFont(tagFont);
        painter.setPen(QColor(tok::kWhite));
        painter.drawText(tagBox, Qt::AlignCenter, tag);
        // 제목 · 설명
        const qreal left = box.left() + 46;
        const int textWidth = int(tagBox.left() - 8 - left);
        painter.setFont(titleFont);
        painter.setPen(QColor(tok::kRedText));
        painter.drawText(QRectF(left, box.top() + 4, textWidth, 20),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(titleFont).elidedText(row.title, Qt::ElideRight, textWidth));
        painter.setFont(detailFont);
        painter.setPen(QColor(tok::kText2));
        painter.drawText(
                QRectF(left, box.top() + 23, textWidth, 18), Qt::AlignLeft | Qt::AlignVCenter,
                QFontMetricsF(detailFont).elidedText(row.detail, Qt::ElideRight, textWidth));
    }
}
} // namespace com::yamada::studio
