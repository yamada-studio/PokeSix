#include "ui/dex/evolutionview.h"

#include "data/sprites/spritecache.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/dex/guidebook.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QToolTip>

namespace {
constexpr int kRowHeight = 30;
constexpr int kIndent = 18; // 깊이 하나
constexpr int kIconWidth = 36;
constexpr int kNameGap = 4;
constexpr int kConditionGap = 8;
} // namespace

namespace com::yamada::studio {
EvolutionView::EvolutionView(SpriteCache *icons, QWidget *parent)
    : QWidget(parent)
    , m_icons(icons)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QWidget::setMouseTracking(true); // 누를 수 있는 줄 위에서 손가락 커서
    connect(m_icons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
}

void EvolutionView::setEvolution(const QList<EvolutionStep> &steps, int currentSpeciesId,
                                 Language language)
{
    m_steps = steps;
    m_current = currentSpeciesId;
    m_language = language;
    QWidget::updateGeometry();
    QWidget::update();
}

QSize EvolutionView::sizeHint() const
{
    return {260, int(m_steps.size()) * kRowHeight};
}

QString EvolutionView::conditionText(const EvolutionCondition &c, Language language)
{
    QStringList parts;
    switch (c.trigger) {
    case 1: // 레벨업
        parts.append(c.minLevel > 0 ? tr("Lv %1").arg(c.minLevel) : tr("레벨업"));
        break;
    case 2: // 통신교환
        if (!c.heldItem.isEmpty())
            parts.append(tr("%1 지니고 통신교환").arg(c.heldItem.text(language)));
        else if (!c.tradeSpecies.isEmpty())
            parts.append(tr("%1과(와) 통신교환").arg(c.tradeSpecies.text(language)));
        else
            parts.append(tr("통신교환"));
        break;
    case 3: // 도구 사용
        parts.append(tr("%1 사용").arg(c.item.text(language)));
        break;
    case 4: // 빈자리(껍질몬)
        parts.append(tr("파티 빈자리 · 몬스터볼"));
        break;
    default:
        parts.append(tr("특별한 조건"));
        break;
    }
    if (c.trigger == 1 && !c.heldItem.isEmpty())
        parts.append(tr("%1 지님").arg(c.heldItem.text(language)));
    if (!c.location.isEmpty())
        parts.append(tr("%1에서").arg(guidebook::placeName(c.location, c.locationName, language)));
    if (c.minHappiness > 0 || c.minAffection > 0)
        parts.append(tr("친밀도"));
    if (c.minBeauty > 0)
        parts.append(tr("아름다움"));
    if (c.timeOfDay == QLatin1String("day"))
        parts.append(tr("낮"));
    else if (c.timeOfDay == QLatin1String("night"))
        parts.append(tr("밤"));
    else if (c.timeOfDay == QLatin1String("dusk"))
        parts.append(tr("황혼"));
    if (c.gender == 1)
        parts.append(tr("암컷"));
    else if (c.gender == 2)
        parts.append(tr("수컷"));
    if (!c.knownMove.isEmpty())
        parts.append(tr("%1 배운 상태").arg(c.knownMove.text(language)));
    if (const tok::TypeColor *type = typechip::find(c.knownMoveType))
        parts.append(tr("%1 타입 기술 배운 상태").arg(typechip::label(*type, language)));
    if (c.relativeStats == 1)
        parts.append(tr("공격 > 방어"));
    else if (c.relativeStats == 0)
        parts.append(tr("공격 = 방어"));
    else if (c.relativeStats == -1)
        parts.append(tr("공격 < 방어"));
    if (!c.partySpecies.isEmpty())
        parts.append(tr("파티에 %1").arg(c.partySpecies.text(language)));
    if (const tok::TypeColor *type = typechip::find(c.partyType))
        parts.append(tr("파티에 %1 타입").arg(typechip::label(*type, language)));
    if (c.needsRain)
        parts.append(tr("비 오는 날"));
    if (c.upsideDown)
        parts.append(tr("기기 뒤집기"));
    return parts.join(QStringLiteral(" · "));
}

QString EvolutionView::conditionsOf(const EvolutionStep &step) const
{
    QStringList alternatives; // 방법이 여럿이면 그중 하나로 진화한다
    for (const EvolutionCondition &c : step.conditions) {
        const QString text = conditionText(c, m_language);
        if (!alternatives.contains(text))
            alternatives.append(text);
    }
    return alternatives.join(QStringLiteral(" / "));
}

int EvolutionView::rowAt(int y) const
{
    const int row = y / kRowHeight;
    return row >= 0 && row < m_steps.size() ? row : -1;
}

void EvolutionView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(tok::kWhite));
    const QFont nameFont = theme::font(theme::kFamilyBody, 13, QFont::ExtraBold);
    const QFont conditionFont = theme::font(theme::kFamilyBody, 12);
    for (qsizetype i = 0; i < m_steps.size(); ++i) {
        const EvolutionStep &step = m_steps.at(i);
        const int top = int(i) * kRowHeight;
        const bool current = step.speciesId == m_current;
        if (current) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(tok::kYellowTint));
            painter.drawRoundedRect(QRectF(0, top + 1, width(), kRowHeight - 2), 4, 4);
        }
        qreal x = step.depth * kIndent;
        if (step.depth > 0) { // ↳ 꺾은 선
            painter.setPen(QPen(QColor(tok::kLineStrong), 1.5));
            const qreal cx = x - kIndent / 2.0;
            painter.drawLine(QPointF(cx, top + 4), QPointF(cx, top + kRowHeight / 2.0));
            painter.drawLine(QPointF(cx, top + kRowHeight / 2.0),
                             QPointF(x - 2, top + kRowHeight / 2.0));
        }
        DexRowDelegate::paintPokemonIcon(&painter, QRectF(x, top, kIconWidth, kRowHeight),
                                         step.pokemonId, m_icons);
        x += kIconWidth + kNameGap;
        const QString name = step.name.text(m_language);
        painter.setFont(nameFont);
        painter.setPen(QColor(tok::kText1));
        const qreal nameWidth = QFontMetricsF(nameFont).horizontalAdvance(name);
        painter.drawText(QRectF(x, top, nameWidth + 1, kRowHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, name);
        x += nameWidth + kConditionGap;
        const QString conditions = conditionsOf(step);
        if (!conditions.isEmpty() && x < width()) {
            painter.setFont(conditionFont);
            painter.setPen(QColor(tok::kText2));
            painter.drawText(QRectF(x, top, width() - x - 4, kRowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             QFontMetricsF(conditionFont)
                                     .elidedText(conditions, Qt::ElideRight, width() - x - 4));
        }
    }
}

void EvolutionView::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(int(event->position().y()));
    const bool clickable = row >= 0 && m_steps.at(row).speciesId != m_current;
    QWidget::setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void EvolutionView::mousePressEvent(QMouseEvent *event)
{
    const int row = rowAt(int(event->position().y()));
    if (event->button() == Qt::LeftButton && row >= 0 && m_steps.at(row).speciesId != m_current)
        emit pokemonClicked(m_steps.at(row).pokemonId);
}

bool EvolutionView::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        const auto *help = static_cast<QHelpEvent *>(event);
        const int row = rowAt(help->pos().y());
        QString tip = row >= 0 ? conditionsOf(m_steps.at(row)) : QString();
        if (tip.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(help->globalPos(),
                               tip.replace(QStringLiteral(" / "), QStringLiteral("\n")), this);
        return true;
    }
    return QWidget::event(event);
}
} // namespace com::yamada::studio
