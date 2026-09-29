#include "ui/home/intromenuitem.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

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

// Entry 줄
constexpr int kNamePx = 25;                // 이름: 도현 25, line-height 1.1
constexpr qreal kNameLine = kNamePx * 1.1; // 이름 줄 상자 높이 27.5
constexpr int kDescPx = 12;                // 설명: 나눔고딕 12
constexpr qreal kNameDescGap = 2;          // 이름과 설명 사이 gap: 2px
constexpr QSizeF kCursor {12, 16};         // ▶: border-left 12 · 위아래 8
constexpr qreal kChip = 26;                // 단축키 칸 26×26, 반경 5, 먹선 2
constexpr qreal kChipRadius = 5;
constexpr int kChipPx = 12; // 단축키 글자: Silkscreen 12

// Quit 줄
constexpr int kQuitPx = 19;            // "종료": 도현 19, text.2
constexpr int kEscPx = 11;             // "Esc": 나눔고딕코딩 11, text.3
constexpr QSizeF kQuitCursor {10, 14}; // ▶: border-left 10 · 위아래 7

// ▶ 삼각형: 폭 w · 높이 h, 22px 칸의 가운데.
void paintCursor(QPainter &painter, const QRectF &row, QSizeF size)
{
    const QPointF center(kPadX + kCursorColumn / 2.0, row.center().y());
    QPainterPath triangle;
    triangle.moveTo(center + QPointF(-size.width() / 2.0, -size.height() / 2.0));
    triangle.lineTo(center + QPointF(size.width() / 2.0, 0));
    triangle.lineTo(center + QPointF(-size.width() / 2.0, size.height() / 2.0));
    triangle.closeSubpath();
    painter.fillPath(triangle, QColor(com::yamada::studio::tok::kInk));
}

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
    QAbstractButton::setCursor(Qt::PointingHandCursor);
}

void IntroMenuItem::setDescription(const QString &description)
{
    m_description = description;
    QAbstractButton::update();
}

void IntroMenuItem::setShortcutText(const QString &text)
{
    m_shortcut = text;
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
    // TODO C 활성 상태가 바뀌면(event->type() == QEvent::EnabledChange) 커서를 바꾼다:
    //        활성 = Qt::PointingHandCursor(손가락), 비활성 = Qt::ArrowCursor(보통 화살표)
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
    // TODO A 잠김(비활성)이면 이 줄 전체를 45% 불투명도로: painter.setOpacity(kLockedOpacity)

    const bool down = QAbstractButton::isDown();
    const qreal offset = down ? kPressedOffset : 0;
    const QRectF row(0, offset, width(), height() - offset);

    // 선택(또는 눌림) 바탕: 노란 옅은 칸 + 먹선 2 · 반경 7. 눌림은 진한 노랑(menu.pressed).
    if (m_selected || down) {
        const qreal half = kBorder / 2.0;
        painter.setPen(QPen(QColor(tok::kInk), kBorder));
        painter.setBrush(QColor(down ? tok::kMenuPressed : tok::kYellowTint));
        painter.drawRoundedRect(row.adjusted(half, half, -half, -half), kRadius - half,
                                kRadius - half);
    }

    if (m_kind == Kind::Entry)
        paintEntry(painter, row);
    else
        paintQuit(painter, row);
}

void IntroMenuItem::paintEntry(QPainter &painter, const QRectF &row)
{
    if (m_selected)
        paintCursor(painter, row, kCursor);

    // 이름(27.5) + 간격(2) + 설명 한 줄을 묶어서 세로 가운데에 둔다.
    const QFont nameFont = theme::font(theme::kFamilyTitle, kNamePx);
    const QFont descFont = theme::font(theme::kFamilyBody, kDescPx);
    const QFontMetricsF nameMetrics(nameFont);
    const QFontMetricsF descMetrics(descFont);
    const qreal descLine
            = descMetrics.height(); // CSS line-height: normal에 가장 가까운 값(ascent + descent)
    const qreal top = row.center().y() - (kNameLine + kNameDescGap + descLine) / 2.0;

    painter.setFont(nameFont);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QPointF(kTextX, baselineIn(top, kNameLine, nameMetrics)),
                     QAbstractButton::text());

    painter.setFont(descFont);
    painter.setPen(QColor(tok::kText2));
    painter.drawText(
            QPointF(kTextX, baselineIn(top + kNameLine + kNameDescGap, descLine, descMetrics)),
            // TODO B 잠겼으면 설명 대신 tr("데이터가 필요해요")
            m_description); // ← 잠겼으면 tr("데이터가 필요해요")가 나오게 바꾼다

    // 오른쪽 단축키 칸: 흰 바탕(선택 시 노랑) + 먹선 2 + Silkscreen 숫자
    const qreal half = kBorder / 2.0;
    const QRectF chip(row.right() - kPadX - kChip, row.center().y() - kChip / 2.0, kChip, kChip);
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(QColor(m_selected ? tok::kYellow : tok::kWhite));
    painter.drawRoundedRect(chip.adjusted(half, half, -half, -half), kChipRadius - half,
                            kChipRadius - half);
    painter.setFont(theme::font(theme::kFamilyPixel, kChipPx));
    painter.setPen(QColor(tok::kText1));
    painter.drawText(chip, Qt::AlignCenter, m_shortcut);
}

void IntroMenuItem::paintQuit(QPainter &painter, const QRectF &row)
{
    if (m_selected)
        paintCursor(painter, row, kQuitCursor);

    const QRectF textArea(kTextX, row.top(), row.width() - kTextX - kPadX, row.height());

    painter.setFont(theme::font(theme::kFamilyTitle, kQuitPx));
    painter.setPen(QColor(tok::kText2));
    painter.drawText(textArea, Qt::AlignLeft | Qt::AlignVCenter, QAbstractButton::text());

    painter.setFont(theme::font(theme::kFamilyData, kEscPx));
    painter.setPen(QColor(tok::kText3));
    painter.drawText(textArea, Qt::AlignRight | Qt::AlignVCenter, m_shortcut);
}
} // namespace com::yamada::studio
