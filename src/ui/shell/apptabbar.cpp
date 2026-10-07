#include "ui/shell/apptabbar.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/svgicon.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QColor>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <iterator>

namespace {
using namespace com::yamada::studio;

constexpr int kBarHeight = tok::kSizeTabActive;        // 45: 활성 탭 높이 = 탭 막대 높이
constexpr int kInactiveHeight = tok::kSizeTabInactive; // 38
constexpr int kBaseLine = 3; // 앱 막대 아래 먹선 두께(비활성 탭은 그 위에 선다)
constexpr int kActiveBorder = 3;
constexpr int kRadius = 8; // 위쪽 모서리만 둥글다
constexpr int kPaddingActive = 18;
constexpr int kPaddingInactive = 16;
constexpr int kIconSize = 16;
constexpr int kIconGap = 7;
constexpr int kTabGap = 4;

// Dex.dc.html 앱 막대의 탭 아이콘(24 격자, 선 2.2). currentColor 자리는 탭 글자색으로 바꾼다.
struct TabSpec
{
    Page page;
    const char *svg;
};
const TabSpec kTabs[] = {
        {Page::Dex,
         R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2"><rect x="4" y="4" width="16" height="16" rx="2"/><path d="M4 9h16M9 9v11"/></svg>)"},
        {Page::Items,
         R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"><path d="M5 8h14l-1.2 12H6.2z"/><path d="M9 8V6.5a3 3 0 0 1 6 0V8"/></svg>)"},
        {Page::Map,
         R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"><path d="M9 4 4 6v14l5-2 6 2 5-2V4l-5 2z"/><path d="M9 4v14M15 6v14"/></svg>)"},
        {Page::Squad,
         R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linejoin="round"><path d="M12 3l7.8 4.5v9L12 21l-7.8-4.5v-9z"/><path d="M12 3v18M4.2 7.5l15.6 9M19.8 7.5l-15.6 9"/></svg>)"},
        {Page::Settings,
         R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round"><circle cx="12" cy="12" r="3.2"/><path d="M12 2.8v2.6M12 18.6v2.6M2.8 12h2.6M18.6 12h2.6M5.5 5.5l1.8 1.8M16.7 16.7l1.8 1.8M5.5 18.5l1.8-1.8M16.7 7.3l1.8-1.8"/></svg>)"},
};

// 탭 하나. 시그널을 새로 만들지 않으므로 Q_OBJECT가 필요 없다(QAbstractButton의 것을 쓴다).
class TabButton : public QAbstractButton
{
public:
    TabButton(const QString &label, const char *svg, qreal dpr)
        : m_font(theme::font(theme::kFamilyTitle, 18))
    {
        QAbstractButton::setText(label);
        QAbstractButton::setCheckable(true);
        QAbstractButton::setCursor(cursors::pointer());
        QAbstractButton::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        const QString source = QString::fromLatin1(svg);
        m_iconOn = svgicon::pixmap(
                QString(source).replace(QStringLiteral("currentColor"), QColor(tok::kRed).name()),
                kIconSize, dpr);
        m_iconOff = svgicon::pixmap(
                QString(source).replace(QStringLiteral("currentColor"), QColor(tok::kWhite).name()),
                kIconSize, dpr);
        // 켜지면 안쪽 여백과 테두리가 달라져 폭이 바뀐다 → 레이아웃에 다시 묻게 한다
        connect(this, &QAbstractButton::toggled, this,
                [this] { QAbstractButton::updateGeometry(); });
    }

    QSize sizeHint() const override
    {
        const QFontMetricsF metrics(m_font);
        const int padding
                = QAbstractButton::isChecked() ? kActiveBorder + kPaddingActive : kPaddingInactive;
        return {qCeil(2 * padding + kIconSize + kIconGap
                      + metrics.horizontalAdvance(QAbstractButton::text())),
                kBarHeight};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const bool active = QAbstractButton::isChecked();

        // 위쪽만 둥근 상자 경로. 비활성은 먹선(3) 위에 서도록 아래를 3 비운다.
        const QRectF box = active ? QRectF(0, 0, width(), kBarHeight)
                                  : QRectF(0, kBarHeight - kBaseLine - kInactiveHeight, width(),
                                           kInactiveHeight);
        if (active) {
            // 테두리는 위 · 왼쪽 · 오른쪽만. 선 폭의 반만큼 안쪽 경로를 그린다(아래는 열려 있다).
            const qreal half = kActiveBorder / 2.0;
            const QRectF r = box.adjusted(half, half, -half, 0);
            QPainterPath fill;
            fill.moveTo(box.bottomLeft());
            fill.lineTo(box.left(), box.top() + kRadius);
            fill.quadTo(box.topLeft(), QPointF(box.left() + kRadius, box.top()));
            fill.lineTo(box.right() - kRadius, box.top());
            fill.quadTo(box.topRight(), QPointF(box.right(), box.top() + kRadius));
            fill.lineTo(box.bottomRight());
            fill.closeSubpath();
            painter.fillPath(fill, QColor(tok::kPaper)); // 아래 먹선까지 덮는다 → 본문과 이어진다

            QPainterPath border;
            border.moveTo(r.bottomLeft());
            border.lineTo(r.left(), r.top() + kRadius - half);
            border.quadTo(r.topLeft(), QPointF(r.left() + kRadius - half, r.top()));
            border.lineTo(r.right() - kRadius + half, r.top());
            border.quadTo(r.topRight(), QPointF(r.right(), r.top() + kRadius - half));
            border.lineTo(r.bottomRight());
            painter.strokePath(border, QPen(QColor(tok::kInk), kActiveBorder));
        } else {
            QPainterPath fill;
            fill.moveTo(box.bottomLeft());
            fill.lineTo(box.left(), box.top() + kRadius);
            fill.quadTo(box.topLeft(), QPointF(box.left() + kRadius, box.top()));
            fill.lineTo(box.right() - kRadius, box.top());
            fill.quadTo(box.topRight(), QPointF(box.right(), box.top() + kRadius));
            fill.lineTo(box.bottomRight());
            fill.closeSubpath();
            painter.fillPath(fill, QColor(tok::kRedDeep));
        }

        // 아이콘 + 글자 묶음을 가운데에
        const QFontMetricsF metrics(m_font);
        const qreal content
                = kIconSize + kIconGap + metrics.horizontalAdvance(QAbstractButton::text());
        qreal x = box.center().x() - content / 2.0;
        painter.drawPixmap(QPointF(x, box.center().y() - kIconSize / 2.0),
                           active ? m_iconOn : m_iconOff);
        x += kIconSize + kIconGap;
        painter.setFont(m_font);
        painter.setPen(QColor(active ? tok::kRed : tok::kWhite));
        painter.drawText(QRectF(x, box.top(), box.right() - x, box.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, QAbstractButton::text());
    }

private:
    QFont m_font;
    QPixmap m_iconOn;
    QPixmap m_iconOff;
};
} // namespace

namespace com::yamada::studio {
AppTabBar::AppTabBar(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
{
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kTabGap);
    m_group->setExclusive(true); // 하나를 켜면 나머지는 저절로 꺼진다

    // 탭 이름은 tr()로 직접 감싼다 — 번역 도구(lupdate)가 이 클래스의 문구로 찾을 수 있게
    const QString labels[] = {tr("도감"), tr("아이템"), tr("타운맵"), tr("스쿼드"),
                              tr("설정")}; // kTabs와 같은 순서
    for (int i = 0; i < static_cast<int>(std::size(kTabs)); ++i) {
        const TabSpec &tab = kTabs[i];
        TabButton *button = new TabButton(labels[i], tab.svg, devicePixelRatioF());
        m_group->addButton(button, static_cast<int>(tab.page)); // 버튼의 id = Page 값
        layout->addWidget(button, 0, Qt::AlignBottom);
    }
    QWidget::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    // idClicked: 사용자가 누를 때만 나온다(setChecked로 바꿀 때는 나오지 않는다) → 되먹임 고리가
    // 생기지 않는다
    connect(m_group, &QButtonGroup::idClicked, this,
            [this](int id) { emit pageSelected(static_cast<Page>(id)); });
}

void AppTabBar::setCurrentPage(Page page)
{
    if (QAbstractButton *button = m_group->button(static_cast<int>(page)))
        button->setChecked(true);
}
} // namespace com::yamada::studio
