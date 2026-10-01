#include "ui/dex/matchupview.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QPainter>

namespace {
constexpr int kSectionWidth = 76; // "받을 때" · "줄 때"
constexpr int kLabelWidth = 40;   // ×4
constexpr int kRowGap = 6;
constexpr int kChipLineGap = 4;
constexpr double kDefenseOrder[] = {4, 2, 1, 0.5, 0.25, 0};
constexpr double kOffenseOrder[] = {2, 1, 0.5, 0};

QString multiplierText(double m)
{
    if (m == 0.5)
        return QStringLiteral("×½");
    if (m == 0.25)
        return QStringLiteral("×¼");
    return QStringLiteral("×%1").arg(m);
}
} // namespace

namespace com::yamada::studio {
MatchupView::MatchupView(QWidget *parent)
    : QWidget(parent)
{
    QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    policy.setHeightForWidth(true);
    QWidget::setSizePolicy(policy);
}

void MatchupView::setMatchups(const QStringList &types, const TypeChart &chart, Language language)
{
    m_language = language;
    m_rows.clear();
    // 받는 피해: 상대 공격 타입 a마다 내 타입들에 대한 배율의 곱
    for (const double order : kDefenseOrder) {
        Row row {true, order, {}};
        for (const QString &attack : chart.types) {
            double m = 1;
            for (const QString &mine : types)
                m *= chart.at(attack, mine);
            if (m == order)
                row.types.append(attack);
        }
        if (!row.types.isEmpty())
            m_rows.append(row);
    }
    // 주는 피해: 상대 방어 타입 d마다 내 타입들(자속) 중 가장 센 배율
    for (const double order : kOffenseOrder) {
        Row row {false, order, {}};
        for (const QString &defense : chart.types) {
            double best = 0;
            for (const QString &mine : types)
                best = std::max(best, chart.at(mine, defense));
            if (best == order)
                row.types.append(defense);
        }
        if (!row.types.isEmpty())
            m_rows.append(row);
    }
    QWidget::updateGeometry();
    QWidget::update();
}

int MatchupView::layout(int width, QPainter *painter) const
{
    const QFont sectionFont = theme::font(theme::kFamilyTitle, 15);
    const QFont labelFont = theme::font(theme::kFamilyData, 13, QFont::Bold);
    const int chipsLeft = kSectionWidth + kLabelWidth;
    int y = 0;
    bool previousDefense = true;
    for (qsizetype i = 0; i < m_rows.size(); ++i) {
        const Row &row = m_rows.at(i);
        if (i > 0 && row.defense != previousDefense)
            y += kRowGap * 2; // 받는 피해 ↔ 주는 피해 사이를 조금 더 띄운다
        const int top = y;
        // 칩을 줄 폭에 맞춰 흘려 놓는다
        qreal x = chipsLeft;
        int lineTop = y;
        for (const QString &identifier : row.types) {
            const tok::TypeColor *type = typechip::find(identifier);
            if (!type)
                continue;
            const qreal w = typechip::width(*type, m_language);
            if (x + w > width && x > chipsLeft) {
                x = chipsLeft;
                lineTop += int(typechip::kHeight) + kChipLineGap;
            }
            if (painter)
                typechip::paint(*painter, QPointF(x, lineTop), *type, m_language);
            x += w + typechip::kGap;
        }
        const int bottom = lineTop + int(typechip::kHeight);
        if (painter) {
            const bool firstOfSection = i == 0 || row.defense != previousDefense;
            if (firstOfSection) {
                painter->setFont(sectionFont);
                painter->setPen(QColor(tok::kText1));
                painter->drawText(QRect(0, top, kSectionWidth, int(typechip::kHeight)),
                                  Qt::AlignLeft | Qt::AlignVCenter,
                                  row.defense ? tr("받을 때") : tr("줄 때"));
            }
            painter->setFont(labelFont);
            const bool strong = row.defense ? row.multiplier > 1 : row.multiplier > 1;
            const bool weak = row.multiplier < 1;
            painter->setPen(QColor(strong ? tok::kRed : weak ? tok::kBlueDeep : tok::kText3));
            painter->drawText(QRect(kSectionWidth, top, kLabelWidth, int(typechip::kHeight)),
                              Qt::AlignLeft | Qt::AlignVCenter, multiplierText(row.multiplier));
        }
        y = bottom + kRowGap;
        previousDefense = row.defense;
    }
    return std::max(0, y - kRowGap);
}

int MatchupView::heightForWidth(int width) const
{
    return layout(width, nullptr);
}

QSize MatchupView::sizeHint() const
{
    const int w = std::max(width(), 400);
    return {w, heightForWidth(w)};
}

void MatchupView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    layout(width(), &painter);
}
} // namespace com::yamada::studio
