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
constexpr int kMethodWidth = 80;
constexpr int kLevelWidth = 64;
constexpr int kRateWidth = 44;
constexpr int kGap = 8;
// 이보다 좁으면(카드 셋이 같은 폭이라 기본 창 폭에서는 좁다) 확률 칸을 빼고 방법 · 레벨 칸을 줄인다
constexpr int kRoomForRate = 330;
constexpr int kNarrowMethodWidth = 66;
constexpr int kNarrowLevelWidth = 56;
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
    // 빈 상태 문구는 두세 줄로 접힐 수 있어서 넉넉히
    return {320, m_encounters.isEmpty() ? 3 * kRowHeight : int(m_encounters.size()) * kRowHeight};
}

void EncounterList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    // 카드 안 스크롤 영역에 들어 있다 — 스크롤 viewport의 회색 바탕이 비치지 않게 흰 바탕부터
    painter.fillRect(rect(), QColor(tok::kWhite));
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
        // 좁으면(기본 창 폭) 확률 칸을 빼고 방법 · 레벨 칸을 줄여 장소 이름에 먼저 자리를 준다
        const bool showRate = width() >= kRoomForRate;
        const int rateSpace = showRate ? kRateWidth + kGap : 0;
        const int methodWidth = showRate ? kMethodWidth : kNarrowMethodWidth;
        const int levelWidth = showRate ? kLevelWidth : kNarrowLevelWidth;
        const int placeWidth
                = std::max(48, width() - x - methodWidth - levelWidth - rateSpace - 3 * kGap);
        painter.setFont(font);
        painter.setPen(QColor(tok::kText1));
        const QString place = guidebook::placeName(e.location, e.locationName, m_language);
        painter.drawText(QRect(x, top, placeWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(font).elidedText(place, Qt::ElideRight, placeWidth));
        x += placeWidth + kGap;
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(x, top, methodWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(font).elidedText(guidebook::methodName(e.method, m_language),
                                                        Qt::ElideRight, methodWidth));
        x += methodWidth + kGap;
        painter.setFont(dataFont);
        painter.setPen(QColor(tok::kText1));
        const QString level = e.minLevel == e.maxLevel
                                      ? QStringLiteral("Lv %1").arg(e.minLevel)
                                      : QStringLiteral("Lv %1–%2").arg(e.minLevel).arg(e.maxLevel);
        painter.drawText(QRect(x, top, levelWidth, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                         level);
        x += levelWidth + kGap;
        if (!showRate)
            continue;
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(x, top, kRateWidth, kRowHeight), Qt::AlignRight | Qt::AlignVCenter,
                         QStringLiteral("%1%").arg(std::min(e.rarity, 100)));
    }
}
} // namespace com::yamada::studio
