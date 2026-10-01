#include "ui/dex/abilitylist.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QHelpEvent>
#include <QPainter>
#include <QToolTip>

namespace {
constexpr int kRowHeight = 28;
constexpr int kNameWidth = 120;
constexpr int kGap = 10;
constexpr int kFirstAbilityGeneration = 3;
} // namespace

namespace com::yamada::studio {
AbilityList::AbilityList(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void AbilityList::setAbilities(const QList<AbilityEntry> &abilities, int generation,
                               Language language)
{
    m_abilities = abilities;
    m_generation = generation;
    m_language = language;
    QWidget::updateGeometry();
    QWidget::update();
}

QSize AbilityList::sizeHint() const
{
    return {400, int(std::max<qsizetype>(m_abilities.size(), 1)) * kRowHeight};
}

void AbilityList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_abilities.isEmpty()) {
        painter.setFont(theme::font(theme::kFamilyBody, 13));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(rect(), Qt::AlignLeft | Qt::AlignVCenter,
                         m_generation < kFirstAbilityGeneration ? tr("특성은 3세대부터 있어요.")
                                                                : tr("특성 정보가 없어요."));
        return;
    }
    const QFont nameFont = theme::font(theme::kFamilyBody, 13, QFont::ExtraBold);
    const QFont tagFont = theme::font(theme::kFamilyBody, 10, QFont::ExtraBold);
    const QFont effectFont = theme::font(theme::kFamilyBody, 12);
    for (qsizetype i = 0; i < m_abilities.size(); ++i) {
        const AbilityEntry &ability = m_abilities.at(i);
        const int top = int(i) * kRowHeight;
        if (i % 2 == 1)
            painter.fillRect(QRect(0, top, width(), kRowHeight), QColor(tok::kPaperAlt));
        qreal x = 6;
        painter.setFont(nameFont);
        painter.setPen(QColor(tok::kText1));
        const QString name = ability.name.text(m_language);
        painter.drawText(QRectF(x, top, kNameWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         name);
        x += std::min<qreal>(kNameWidth, QFontMetricsF(nameFont).horizontalAdvance(name)) + 6;
        if (ability.hidden) { // "숨겨진 특성" 꼬리표(점선 테)
            const QString tag = tr("숨겨진 특성");
            const qreal w = QFontMetricsF(tagFont).horizontalAdvance(tag) + 10;
            const QRectF box(x, top + (kRowHeight - 16) / 2.0, w, 16);
            QPen dashed(QColor(tok::kBlue), 1.2);
            dashed.setStyle(Qt::DashLine);
            painter.setPen(dashed);
            painter.setBrush(QColor(tok::kBlueTint));
            painter.drawRoundedRect(box, 3, 3);
            painter.setFont(tagFont);
            painter.setPen(QColor(tok::kBlueDeep));
            painter.drawText(box, Qt::AlignCenter, tag);
        }
        const qreal effectLeft = 6 + kNameWidth + 70 + kGap; // 이름 · 꼬리표 자리 뒤로 맞춘다
        painter.setFont(effectFont);
        painter.setPen(QColor(tok::kText2));
        const QRectF effect(effectLeft, top, width() - effectLeft - 6, kRowHeight);
        painter.drawText(effect, Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(effectFont)
                                 .elidedText(ability.effect.text(m_language), Qt::ElideRight,
                                             effect.width()));
    }
}

bool AbilityList::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        const auto *help = static_cast<QHelpEvent *>(event);
        const int row = help->pos().y() / kRowHeight;
        if (row >= 0 && row < m_abilities.size())
            QToolTip::showText(help->globalPos(), m_abilities.at(row).effect.text(m_language),
                               this);
        else
            QToolTip::hideText();
        return true;
    }
    return QWidget::event(event);
}
} // namespace com::yamada::studio
