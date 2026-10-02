#include "ui/home/intromenuitem.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"
#include "ui/widgets/svgicon.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QPainter>
#include <QPainterPath>

namespace {
// Intro.dc.html의 메뉴 줄: grid-template-columns: 22px 1fr auto; gap: 12px; padding: 0 12px
// 줄에는 선택되지 않았을 때도 투명한 2px 테두리가 있다(border: 2px solid transparent).
// 그래서 내용은 테두리 2 + padding 12 = 14에서 시작한다. 선택돼도 글자가 움직이지 않는 이유.
constexpr qreal kBorder = 2;          // 선택 테 border: 2px solid ink
constexpr qreal kPadX = kBorder + 12; // 테두리 + padding
constexpr qreal kCursorColumn = 22;
constexpr qreal kGap = 12;
constexpr qreal kTextX = kPadX + kCursorColumn + kGap; // 48: 이름 · 설명이 시작하는 x
constexpr qreal kRadius = 7;                           // border-radius: 7px
constexpr qreal kPressedOffset = 2;                    // 눌림: 2px 내려앉는다
constexpr qreal kLockedOpacity = 0.45; // 잠김: 불투명도 45% (02b SCR-01 메뉴 항목 상태)

// 카드 버튼(Entry · Primary)
constexpr int kNamePx = 20; // 이름: 도현 20
constexpr qreal kNameLine = 24;
constexpr int kDescPx = 12;       // 설명: 나눔고딕 12
constexpr qreal kNameDescGap = 2; // 이름과 설명 사이
constexpr int kBadge = 36;        // 아이콘 배지(먹색 둥근 네모 + 흰 그림)
constexpr int kBadgeRadius = 9;
constexpr int kBadgeIcon = 20;
constexpr int kCardPadX = 12;
constexpr int kCardGap = 12; // 배지 ↔ 글자

constexpr com::yamada::studio::PanelStyle kEntryCard {
        .outline = 2,
        .radius = 10,
        .shadow = 3,
        .fill = com::yamada::studio::tok::kWhite,
        .ink = com::yamada::studio::tok::kInk,
};

// CSS의 줄 상자(line box) 안에 글자를 넣을 때의 기준선 y.
// 브라우저는 줄 높이와 글꼴 높이(ascent + descent)의 차이를 위아래로 반씩 나눈다(half-leading).
qreal baselineIn(qreal lineTop, qreal lineHeight, const QFontMetricsF &metrics)
{
    return lineTop + (lineHeight - (metrics.ascent() + metrics.descent())) / 2.0 + metrics.ascent();
}
} // namespace

namespace com::yamada::studio {
IntroMenuItem::IntroMenuItem(Kind kind, QWidget *parent)
    : QAbstractButton(parent)
    , m_kind(kind)
{
    QAbstractButton::setFocusPolicy(Qt::NoFocus); // 키보드는 IntroMenu가 받는다
    QAbstractButton::setCursor(cursors::pointer());
}

void IntroMenuItem::setDescription(const QString &description)
{
    m_description = description;
    QAbstractButton::update();
}

void IntroMenuItem::setIconSvg(const QString &svg)
{
    m_iconSvg = svg;
    m_icon = {};
    QAbstractButton::update();
}

void IntroMenuItem::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    QAbstractButton::update(); // 다시 그려 달라고 "예약"만 한다. 실제 그리기는 이벤트 루프가
                               // paintEvent로.
}

void IntroMenuItem::changeEvent(QEvent *event)
{
    QAbstractButton::changeEvent(event);
    // 활성 상태가 바뀌면(event->type() == QEvent::EnabledChange) 커서를 바꾼다:
    //        활성 = 흰 장갑(cursors::pointer()), 비활성 = Qt::ArrowCursor(보통 화살표)
    if (event->type() == QEvent::EnabledChange)
        QAbstractButton::setCursor(QAbstractButton::isEnabled() ? cursors::pointer()
                                                                : QCursor(Qt::ArrowCursor));
}

void IntroMenuItem::enterEvent(QEnterEvent *event)
{
    QAbstractButton::enterEvent(
            event); // override 안에서 베이스 구현 호출 — 가상 함수의 정상적인 확장
    emit hovered();
}

void IntroMenuItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 눌림: 줄 전체가 2px 내려앉는다. 위젯 밖으로는 그릴 수 없으니 위쪽을 2px 줄인 상자를 쓴다.
    // 잠김(비활성)이면 이 줄 전체를 45% 불투명도로 그린다: painter.setOpacity(0.45)
    if (!QAbstractButton::isEnabled())
        painter.setOpacity(kLockedOpacity);

    const bool down = QAbstractButton::isDown();

    // 카드 버튼: 눌리면 2px 내려앉고 그림자가 1로 준다(그림자 속으로 들어가는 느낌)
    const qreal offset = down ? kPressedOffset : 0;
    const QRectF row(0, offset, width(), height() - offset);
    paintCard(painter, row);
}

void IntroMenuItem::paintCard(QPainter &painter, const QRectF &row)
{
    PanelStyle style = kEntryCard;
    if (m_kind == Kind::Primary)
        style.fill = tok::kRed; // 주 행동(스쿼드)은 빨강 카드 + 흰 글자
    if (QAbstractButton::isDown())
        style.shadow = 1;
    paintPanel(painter, row.toRect(), style);
    const QRectF face = row.adjusted(style.outline, style.outline, -style.outline,
                                     -style.outline - style.shadow);

    // 선택(키보드 ↑↓ · hover): 면 안쪽 노란 테 — 세대 카드와 같은 문법
    if (m_selected) {
        painter.setPen(QPen(QColor(tok::kYellow), 3));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(face.adjusted(2.5, 2.5, -2.5, -2.5), style.radius - 3,
                                style.radius - 3);
    }

    const bool primary = m_kind == Kind::Primary;
    const QColor title = primary ? QColor(tok::kWhite) : QColor(tok::kText1);
    QColor sub = primary ? QColor(tok::kWhite) : QColor(tok::kText2);
    if (primary)
        sub.setAlpha(215);

    // 왼쪽 아이콘 배지: 먹색 둥근 네모 + 흰 그림
    const QRectF badge(face.left() + kCardPadX, face.center().y() - kBadge / 2.0, kBadge, kBadge);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kInk));
    painter.drawRoundedRect(badge, kBadgeRadius, kBadgeRadius);
    if (m_icon.isNull() && !m_iconSvg.isEmpty())
        m_icon = svgicon::pixmap(QString(m_iconSvg).replace(QStringLiteral("currentColor"),
                                                            QColor(tok::kWhite).name()),
                                 kBadgeIcon, devicePixelRatio());
    if (!m_icon.isNull())
        painter.drawPixmap(badge.center() - QPointF(kBadgeIcon / 2.0, kBadgeIcon / 2.0), m_icon);

    // 이름 + 설명(잠겼으면 "데이터가 필요해요")
    const qreal textX = badge.right() + kCardGap;
    const QFont nameFont = theme::font(theme::kFamilyTitle, kNamePx);
    const QFont descFont = theme::font(theme::kFamilyBody, kDescPx);
    const QFontMetricsF nameMetrics(nameFont);
    const QFontMetricsF descMetrics(descFont);
    const qreal descLine = descMetrics.height();
    const qreal top = face.center().y() - (kNameLine + kNameDescGap + descLine) / 2.0;
    painter.setFont(nameFont);
    painter.setPen(title);
    painter.drawText(QPointF(textX, baselineIn(top, kNameLine, nameMetrics)),
                     QAbstractButton::text());
    painter.setFont(descFont);
    painter.setPen(sub);
    painter.drawText(
            QPointF(textX, baselineIn(top + kNameLine + kNameDescGap, descLine, descMetrics)),
            QAbstractButton::isEnabled() ? m_description : tr("데이터가 필요해요"));

    // 오른쪽 화살표 ›
    painter.setPen(QPen(title, 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    const QPointF tip(face.right() - kCardPadX - 4, face.center().y());
    QPainterPath chevron;
    chevron.moveTo(tip + QPointF(-5, -6));
    chevron.lineTo(tip + QPointF(1, 0));
    chevron.lineTo(tip + QPointF(-5, 6));
    painter.drawPath(chevron);
}

} // namespace com::yamada::studio
