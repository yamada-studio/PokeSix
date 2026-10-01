#include "ui/dex/encounterlist.h"

#include "ui/dex/guidebook.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QPainter>

namespace {
constexpr int kRowHeight = 26;
constexpr int kBadgeWidth = 34;
constexpr int kMethodWidth = 110;
constexpr int kLevelWidth = 70;
constexpr int kRateWidth = 44;
constexpr int kGap = 8;
} // namespace

namespace com::yamada::studio {
EncounterList::EncounterList(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

void EncounterList::setEncounters(const QList<EncounterEntry> &encounters, Language language)
{
    m_encounters = encounters;
    m_language = language;
    QWidget::updateGeometry();
    QWidget::update();
}

QSize EncounterList::sizeHint() const
{
    return {320, int(std::max<qsizetype>(m_encounters.size(), 1)) * kRowHeight};
}

void EncounterList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QFont font = theme::font(theme::kFamilyBody, 12, QFont::Bold);
    const QFont dataFont = theme::font(theme::kFamilyData, 12, QFont::Bold);
    if (m_encounters.isEmpty()) {
        painter.setFont(theme::font(theme::kFamilyBody, 13));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(rect(), Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                         tr("이 세대에는 야생에서 만날 수 없어요. 진화 · 교배 · 교환으로 얻어요."));
        return;
    }
    for (qsizetype i = 0; i < m_encounters.size(); ++i) {
        const EncounterEntry &e = m_encounters.at(i);
        const int top = int(i) * kRowHeight;
        if (i % 2 == 1)
            painter.fillRect(QRect(0, top, width(), kRowHeight), QColor(tok::kPaperAlt));

        // 버전 배지(도감 선택 버튼과 같은 색 · 약칭)
        const dexstyle::VersionStyle style
                = dexstyle::version(e.version, e.versionName.text(Language::English));
        const QRectF badge(2, top + 4, kBadgeWidth, kRowHeight - 8);
        painter.setPen(QPen(QColor(tok::kInk), 1.5));
        painter.setBrush(style.background);
        painter.drawRoundedRect(badge.adjusted(0.75, 0.75, -0.75, -0.75), 3, 3);
        painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold));
        painter.setPen(style.text);
        painter.drawText(badge, Qt::AlignCenter, style.shortName);

        int x = kBadgeWidth + kGap + 2;
        const int placeWidth
                = std::max(60, width() - x - kMethodWidth - kLevelWidth - kRateWidth - 3 * kGap);
        painter.setFont(font);
        painter.setPen(QColor(tok::kText1));
        const QString place = guidebook::placeName(e.location, e.locationName, m_language);
        painter.drawText(QRect(x, top, placeWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(font).elidedText(place, Qt::ElideRight, placeWidth));
        x += placeWidth + kGap;
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(x, top, kMethodWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         guidebook::methodName(e.method, m_language));
        x += kMethodWidth + kGap;
        painter.setFont(dataFont);
        painter.setPen(QColor(tok::kText1));
        const QString level = e.minLevel == e.maxLevel
                                      ? QStringLiteral("Lv %1").arg(e.minLevel)
                                      : QStringLiteral("Lv %1–%2").arg(e.minLevel).arg(e.maxLevel);
        painter.drawText(QRect(x, top, kLevelWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         level);
        x += kLevelWidth + kGap;
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(x, top, kRateWidth, kRowHeight), Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1%").arg(std::min(e.rarity, 100)));
    }
}
} // namespace com::yamada::studio
