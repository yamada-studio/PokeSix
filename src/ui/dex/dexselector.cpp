#include "ui/dex/dexselector.h"

#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <utility>

namespace {
using namespace com::yamada::studio;

constexpr int kSpacing = 6; // 버튼 사이
constexpr int kHeight = 26; // 빨강 띠 38 안에 위아래 6씩
constexpr int kBorder = 2;  // 먹선
constexpr int kRadius = 6;
constexpr int kPaddingX = 9;
constexpr int kLabelToBadge = 6; // 지방 이름 ↔ 배지
constexpr int kBadgeHeight = 16;
constexpr int kBadgeRadius = 3;
constexpr int kBadgePaddingX = 5; // 배지 칸 하나의 좌우 여백
constexpr qreal kBadgeBorder = 1.5;

// 도감 버튼 하나: "신오 [D|P]". 지방 이름(도현 15) + 버전 배지. 배지는 버전마다 칸을 나눠
// 칸마다 그 버전의 바탕 · 글자색(dexstyle.json)으로 칠한다 — 레퍼런스의 반반 배지.
// 켜짐(checked) = 노랑, hover = 연노랑, 키보드 포커스 = 파랑 테두리. 시그널이 없어서 Q_OBJECT는
// 필요 없다(클릭 · 체크 · 포커스는 QAbstractButton이 한다).
class DexButton : public QAbstractButton
{
public:
    DexButton(const QString &label, QList<dexstyle::VersionStyle> badges)
        : m_badges(std::move(badges))
        , m_labelFont(theme::font(theme::kFamilyTitle, 15))
        , m_badgeFont(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold))
    {
        QAbstractButton::setText(label);
        QAbstractButton::setCheckable(true);
        QAbstractButton::setCursor(Qt::PointingHandCursor);
        QAbstractButton::setFocusPolicy(
                Qt::TabFocus); // 마우스로 눌러도 검색 칸의 포커스를 뺏지 않는다
        QAbstractButton::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        QAbstractButton::setAttribute(
                Qt::WA_Hover); // hover 모양을 그리려고 enter/leave 때 다시 그린다
    }

    QSize sizeHint() const override
    {
        qreal width
                = 2 * (kBorder + kPaddingX) + QFontMetricsF(m_labelFont).horizontalAdvance(text());
        if (!m_badges.isEmpty())
            width += kLabelToBadge + badgesWidth();
        return {qCeil(width), kHeight};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 1) 본체
        QColor fill(tok::kWhite);
        if (isChecked())
            fill = QColor(tok::kYellow);
        else if (underMouse())
            fill = QColor(tok::kYellowTint);
        const QColor ink(hasFocus() ? tok::kBlue : tok::kInk);
        const qreal half = kBorder / 2.0;
        painter.setPen(QPen(ink, kBorder));
        painter.setBrush(fill);
        painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), kRadius - half,
                                kRadius - half);

        // 2) 지방 이름
        qreal x = kBorder + kPaddingX;
        const qreal labelWidth = QFontMetricsF(m_labelFont).horizontalAdvance(text());
        painter.setFont(m_labelFont);
        painter.setPen(QColor(tok::kText1));
        painter.drawText(QRectF(x, 0, labelWidth, height()), Qt::AlignLeft | Qt::AlignVCenter,
                         text());
        if (m_badges.isEmpty())
            return;

        // 3) 배지: 둥근 사각형 하나를 버전 수만큼 세로로 나눠 칸마다 칠하고, 칸 사이에 먹선 1
        x += labelWidth + kLabelToBadge;
        const QRectF badge(x, (height() - kBadgeHeight) / 2.0, badgesWidth(), kBadgeHeight);
        QPainterPath outline;
        outline.addRoundedRect(badge, kBadgeRadius, kBadgeRadius);
        painter.save();
        painter.setClipPath(outline); // 칸의 네모 모서리가 둥근 테 밖으로 나가지 않게
        const QFontMetricsF metrics(m_badgeFont);
        painter.setFont(m_badgeFont);
        qreal cellX = badge.left();
        for (qsizetype i = 0; i < m_badges.size(); ++i) {
            const dexstyle::VersionStyle &style = m_badges.at(i);
            const qreal cellWidth = metrics.horizontalAdvance(style.shortName) + 2 * kBadgePaddingX;
            const QRectF cell(cellX, badge.top(), cellWidth, badge.height());
            painter.fillRect(cell, style.background);
            painter.setPen(style.text);
            painter.drawText(cell, Qt::AlignCenter, style.shortName);
            if (i > 0)
                painter.fillRect(QRectF(cellX - 0.5, badge.top(), 1, badge.height()),
                                 QColor(tok::kInk));
            cellX += cellWidth;
        }
        painter.restore();
        painter.setPen(QPen(QColor(tok::kInk), kBadgeBorder));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(outline);
    }

private:
    qreal badgesWidth() const
    {
        const QFontMetricsF metrics(m_badgeFont);
        qreal width = 0;
        for (const dexstyle::VersionStyle &style : m_badges)
            width += metrics.horizontalAdvance(style.shortName) + 2 * kBadgePaddingX;
        return width;
    }

    QList<dexstyle::VersionStyle> m_badges;
    QFont m_labelFont;
    QFont m_badgeFont;
};
} // namespace

namespace com::yamada::studio {
DexSelector::DexSelector(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0); // 스타일 기본 여백 없이(A2)
    m_layout->setSpacing(kSpacing);
    m_group->setExclusive(true); // 하나를 켜면 나머지는 저절로 꺼진다
    // idClicked: 사용자가 누를 때만 나온다. setDexes()가 setChecked로 전국을 켤 때는 나오지 않는다.
    connect(m_group, &QButtonGroup::idClicked, this, &DexSelector::dexSelected);
    setDexes({});
}

void DexSelector::setDexes(const QList<DexInfo> &dexes)
{
    m_dexes = dexes;
    rebuild(kNational); // 세대가 바뀌면 전국부터
}

void DexSelector::setLanguage(Language language)
{
    if (language == m_language)
        return;
    m_language = language;
    // 버튼 글자만 바뀐다 — 고른 도감은 그대로 둔다(목록도 그대로라 dexSelected는 보내지 않는다)
    rebuild(m_group->checkedId() < 0 ? kNational : m_group->checkedId());
}

void DexSelector::rebuild(int checkedId)
{
    // 1) 기존 버튼 지우기. delete하면 레이아웃과 버튼 그룹에서도 저절로 빠지지만(소멸자가 알린다),
    //    그룹에서 먼저 빼 두면 순서에 기대지 않아도 된다.
    const QList<QAbstractButton *> old = m_group->buttons();
    for (QAbstractButton *button : old) {
        m_group->removeButton(button);
        delete button;
    }

    // 2) [전국] + 지방 도감. 보이는 규칙(숨김 · 이름 · 배지 색)은 dexstyle.json이 정한다.
    addButton(new DexButton(tr("전국"), {}), tr("그 세대까지 나온 포켓몬 전부"), kNational);
    for (const DexInfo &dex : std::as_const(m_dexes)) {
        const dexstyle::DexStyle rule = dexstyle::dex(dex.identifier);
        if (rule.hidden)
            continue;
        QList<dexstyle::VersionStyle> badges;
        QStringList shown; // 같은 약칭은 한 번만(관동도감: 일본판 레드 = 레드 = R)
        for (qsizetype i = 0; i < dex.versions.size(); ++i) {
            const dexstyle::VersionStyle style = dexstyle::version(
                    dex.versions.at(i), dex.versionNames.value(i).text(Language::English));
            if (shown.contains(style.shortName))
                continue;
            shown.append(style.shortName);
            badges.append(style);
        }
        // 이름: dexstyle.json의 이름 바꾸기(칼로스 셋 · DLC 도감) > DB의 지방 이름. 둘 다 언어별.
        const QString label
                = rule.label.isEmpty() ? dex.region.text(m_language) : rule.label.text(m_language);
        // 툴팁: 버전 이름(그 언어가 없으면 대체 순서 — 9세대 DLC는 PokéAPI에 한국어 이름이 아직
        // 없다)
        QStringList names;
        for (const LocalizedText &name : dex.versionNames)
            names.append(name.text(m_language));
        addButton(new DexButton(label, badges),
                  tr("%1 — %2").arg(label, names.join(QStringLiteral(" · "))), dex.pokedexId);
    }

    // 3) 고른 도감을 켠다(없어졌으면 전국). 4) sizeHint가 바뀌었다 → 부모(PanelFrame)가 자리를 다시
    // 잡는다.
    QAbstractButton *checked = m_group->button(checkedId);
    (checked ? checked : m_group->button(kNational))->setChecked(true);
    QWidget::updateGeometry();
}

void DexSelector::addButton(QAbstractButton *button, const QString &toolTip, int id)
{
    button->setToolTip(toolTip);
    m_layout->addWidget(button);
    m_group->addButton(button, id);
}
} // namespace com::yamada::studio
