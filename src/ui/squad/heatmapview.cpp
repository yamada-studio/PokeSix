#include "ui/squad/heatmapview.h"

#include "core/types/typekey.h"
#include "data/state/squadsession.h"
#include "ui/squad/squadpaint.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QMouseEvent>
#include <QPainter>

namespace {
using namespace com::yamada::studio;

constexpr int kHeaderHeight = 26;
constexpr int kRowHeight = 26;
constexpr int kSummaryGap = 12;
constexpr int kSummaryHeight = 24;
constexpr int kLegendHeight = 26;
constexpr int kLabelWidth = 110;
constexpr qreal kMinimumCell = 24;
constexpr int kFlashSteps = 4; // 켬 · 끔 · 켬 · 끔 = 두 번 깜빡
constexpr int kFlashMs = 180;

double receivedOf(const SquadAnalysis &analysis, int slot, const tok::TypeColor &type)
{
    const std::optional<Type> t = typeFromKey(type.key);
    return t ? analysis.received[std::size_t(slot)][std::size_t(*t)] : 1.0;
}

std::size_t indexOf(const tok::TypeColor &type)
{
    const std::optional<Type> t = typeFromKey(type.key);
    return t ? std::size_t(*t) : 0;
}

// 받는 배율 칸(heat.* 토큰)
void paintCell(QPainter &painter, const QRectF &cell, double multiplier, bool selectedRow)
{
    QRgb fill = selectedRow ? tok::kYellowRowSel : tok::kWhite;
    QRgb text = tok::kText1;
    if (multiplier >= 4.0)
        fill = tok::kHeatX4, text = tok::kWhite;
    else if (multiplier >= 2.0)
        fill = tok::kHeatX2, text = tok::kHeatX2Text;
    else if (multiplier <= 0.0)
        fill = tok::kHeatZero, text = tok::kWhite;
    else if (multiplier <= 0.25)
        fill = tok::kHeatQuarter, text = tok::kWhite;
    else if (multiplier <= 0.5)
        fill = tok::kHeatHalf, text = tok::kHeatHalfText;
    painter.setPen(QPen(QColor(tok::kCellBorder), 1));
    painter.setBrush(QColor(fill));
    painter.drawRect(cell);
    painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
    painter.setPen(QColor(text));
    painter.drawText(cell, Qt::AlignCenter, squadpaint::multiplierText(multiplier));
}
} // namespace

namespace com::yamada::studio {
HeatmapView::HeatmapView(SquadSession *session, QWidget *parent)
    : QWidget(parent)
    , m_session(session)
{
    QWidget::setMouseTracking(true);
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_flash.setInterval(kFlashMs);
    connect(&m_flash, &QTimer::timeout, this, [this] {
        if (++m_flashStep >= kFlashSteps) {
            m_flash.stop();
            m_flashType.clear();
        }
        QWidget::update();
    });
}

QSize HeatmapView::sizeHint() const
{
    const int height
            = kHeaderHeight + 6 * kRowHeight + kSummaryGap + 3 * kSummaryHeight + 8 + kLegendHeight;
    return {kLabelWidth + int(18 * 36), height};
}

QSize HeatmapView::minimumSizeHint() const
{
    return {kLabelWidth + int(18 * kMinimumCell), sizeHint().height()};
}

void HeatmapView::refresh(Language language)
{
    m_language = language;
    QWidget::update();
}

void HeatmapView::setSelectedSlot(int slot)
{
    m_selected = slot;
    QWidget::update();
}

void HeatmapView::setProblemTypes(const QSet<QString> &keys)
{
    m_problemTypes = keys;
    QWidget::update();
}

void HeatmapView::setHotType(const QString &key)
{
    m_hotType = key;
    QWidget::update();
}

void HeatmapView::flashType(const QString &key)
{
    m_flashType = key;
    m_flashStep = 0;
    m_flash.start();
    QWidget::update();
}

int HeatmapView::labelWidth() const
{
    return kLabelWidth;
}

qreal HeatmapView::cellWidth() const
{
    return std::max(kMinimumCell, (width() - labelWidth() - 2) / 18.0);
}

int HeatmapView::rowAt(const QPoint &pos) const
{
    const int y = pos.y() - kHeaderHeight;
    if (pos.x() >= labelWidth() || y < 0 || y >= 6 * kRowHeight)
        return -1;
    return y / kRowHeight;
}

void HeatmapView::mouseMoveEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position().toPoint());
    if (row >= 0 && m_session->detail(row).isValid())
        QWidget::setCursor(cursors::pointer());
    else
        QWidget::unsetCursor();
}

void HeatmapView::mousePressEvent(QMouseEvent *event)
{
    const int row = rowAt(event->position().toPoint());
    if (row >= 0 && m_session->detail(row).isValid())
        emit slotClicked(row);
    else
        QWidget::mousePressEvent(event);
}

void HeatmapView::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false); // 칸 경계는 픽셀에 맞춘다
    const SquadAnalysis &analysis = m_session->analysis();
    const QStringList &existing = m_session->chart().types;
    const qreal cell = cellWidth();
    const int left = labelWidth();
    auto columnX = [&](int column) {
        return left + column * cell;
    };

    // 머리: 타입 약칭 칩
    painter.setRenderHint(QPainter::Antialiasing, true);
    for (int c = 0; c < 18; ++c) {
        const tok::TypeColor &type = tok::kTypes[std::size_t(c)];
        const QRectF box(columnX(c) + 2, 2, cell - 4, kHeaderHeight - 5);
        if (existing.contains(QLatin1String(type.key))) {
            squadpaint::paintTypeBox(painter, box, type, squadpaint::typeAbbr(type, m_language));
        } else { // 세대에 없는 타입: 흐린 글자 + 점선
            painter.setPen(QPen(QColor(tok::kLineStrong), 1, Qt::DashLine));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(box, 3, 3);
            painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::Bold));
            painter.setPen(QColor(tok::kTextDisabled));
            painter.drawText(box, Qt::AlignCenter, squadpaint::typeAbbr(type, m_language));
        }
    }
    painter.setRenderHint(QPainter::Antialiasing, false);

    // 슬롯 행
    for (int slot = 0; slot < 6; ++slot) {
        const int top = kHeaderHeight + slot * kRowHeight;
        const PokemonDetail &detail = m_session->detail(slot);
        const bool filled = detail.isValid();
        const bool selected = slot == m_selected && filled;
        if (selected)
            painter.fillRect(QRect(0, top + 1, left - 4, kRowHeight - 2), QColor(tok::kYellowSoft));
        painter.setFont(theme::font(theme::kFamilyData, 11, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(6, top, 22, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1").arg(slot + 1, 2, 10, QLatin1Char('0')));
        const QFont nameFont = theme::font(theme::kFamilyBody, 12, QFont::ExtraBold);
        painter.setFont(filled ? nameFont : theme::font(theme::kFamilyBody, 12));
        painter.setPen(QColor(filled ? tok::kText1 : tok::kTextDisabled));
        const QString name = filled ? detail.name.text(m_language) : tr("빈 슬롯");
        painter.drawText(QRect(30, top, left - 36, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(painter.font()).elidedText(name, Qt::ElideRight, left - 36));
        for (int c = 0; c < 18; ++c) {
            const tok::TypeColor &type = tok::kTypes[std::size_t(c)];
            const QRectF box(columnX(c) + 1, top + 1, cell - 2, kRowHeight - 2);
            if (!existing.contains(QLatin1String(type.key)))
                squadpaint::paintHatch(painter, box);
            else if (!filled)
                painter.fillRect(box, QColor(tok::kPaperAlt));
            else
                paintCell(painter, box, receivedOf(analysis, slot, type), selected);
        }
    }

    // 요약 3행
    const int summaryTop = kHeaderHeight + 6 * kRowHeight + kSummaryGap;
    const QString labels[3] = {tr("약점 수"), tr("내성 · 무효"), tr("공격 커버")};
    for (int r = 0; r < 3; ++r) {
        const int top = summaryTop + r * kSummaryHeight;
        painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::ExtraBold));
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(6, top, left - 10, kSummaryHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         labels[r]);
        for (int c = 0; c < 18; ++c) {
            const tok::TypeColor &type = tok::kTypes[std::size_t(c)];
            const QRectF box(columnX(c) + 2, top + 2, cell - 4, kSummaryHeight - 4);
            if (!existing.contains(QLatin1String(type.key))) {
                painter.setFont(theme::font(theme::kFamilyData, 11));
                painter.setPen(QColor(tok::kTextDisabled));
                painter.drawText(box, Qt::AlignCenter, QStringLiteral("—"));
                continue;
            }
            const std::size_t i = indexOf(type);
            painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
            if (r == 0) {
                const int weak = analysis.weak[i];
                if (weak >= 2)
                    painter.fillRect(box, QColor(weak >= 3 ? tok::kHeatX4 : tok::kHeatX2));
                painter.setPen(QColor(weak >= 3   ? tok::kWhite
                                      : weak == 2 ? tok::kHeatX2Text
                                                  : tok::kText3));
                painter.drawText(box, Qt::AlignCenter, QString::number(weak));
            } else if (r == 1) {
                const int resist = analysis.resist[i];
                painter.setPen(QColor(resist > 0 ? tok::kBlueDeep : tok::kTextDisabled));
                painter.drawText(box, Qt::AlignCenter, QString::number(resist));
            } else if (!analysis.hasAttackingMove) {
                painter.setPen(QColor(tok::kTextDisabled));
                painter.drawText(box, Qt::AlignCenter, QStringLiteral("—"));
            } else if (analysis.covered[i]) {
                painter.fillRect(box, QColor(tok::kGreenTint));
                painter.setPen(QColor(tok::kGreen));
                painter.drawText(box, Qt::AlignCenter, QStringLiteral("✓"));
            } else {
                painter.fillRect(box, QColor(tok::kRed));
                painter.setPen(QColor(tok::kWhite));
                painter.drawText(box, Qt::AlignCenter, QStringLiteral("✕"));
            }
        }
    }

    // 문제 열: 빨강 테(머리부터 공격 커버까지). 마우스가 올라간 문제 · 깜빡임은 두껍게
    painter.setRenderHint(QPainter::Antialiasing, true);
    const int bottom = summaryTop + 3 * kSummaryHeight;
    for (int c = 0; c < 18; ++c) {
        const QString key = QLatin1String(tok::kTypes[std::size_t(c)].key);
        const bool flashOn = key == m_flashType && m_flashStep % 2 == 0;
        const bool hot = key == m_hotType;
        if (!m_problemTypes.contains(key) && !flashOn && !hot)
            continue;
        if (flashOn) {
            QColor glow(tok::kYellow);
            glow.setAlpha(90);
            painter.fillRect(QRectF(columnX(c), 0, cell, bottom), glow);
        }
        painter.setPen(QPen(QColor(tok::kRed), hot || flashOn ? 3 : 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(columnX(c), 0.5, cell, bottom), 3, 3);
    }

    // 범례
    const int legendTop = bottom + 8;
    struct Legend
    {
        QRgb fill;
        QString text;
    };
    const Legend legend[] = {{tok::kHeatX4, QStringLiteral("×4")},
                             {tok::kHeatX2, QStringLiteral("×2")},
                             {tok::kHeatHalf, QStringLiteral("×½")},
                             {tok::kHeatQuarter, QStringLiteral("×¼")},
                             {tok::kHeatZero, tr("무효")}};
    const QFont legendFont = theme::font(theme::kFamilyBody, 11, QFont::Bold);
    painter.setFont(legendFont);
    int x = 6;
    for (const Legend &l : legend) {
        painter.setPen(QPen(QColor(tok::kInk), 1));
        painter.setBrush(QColor(l.fill));
        painter.drawRect(QRectF(x, legendTop + 7, 12, 12));
        painter.setPen(QColor(tok::kText2));
        const int w = int(QFontMetricsF(legendFont).horizontalAdvance(l.text));
        painter.drawText(QRect(x + 16, legendTop, w + 4, kLegendHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, l.text);
        x += 16 + w + 14;
    }
    painter.setPen(QPen(QColor(tok::kRed), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRectF(x, legendTop + 7, 12, 12));
    painter.setPen(QColor(tok::kText2));
    painter.drawText(QRect(x + 16, legendTop, 80, kLegendHeight), Qt::AlignLeft | Qt::AlignVCenter,
                     tr("문제 열"));
    painter.setFont(theme::font(theme::kFamilyBody, 11));
    painter.setPen(QColor(tok::kText3));
    painter.drawText(QRect(x + 90, legendTop, width() - x - 92, kLegendHeight),
                     Qt::AlignRight | Qt::AlignVCenter, tr("이름을 누르면 행이 노랗게 강조돼요"));
}
} // namespace com::yamada::studio
