#include "ui/widgets/generationbutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QtMath>

namespace {
// 크기별 수치. Large = Intro.dc.html의 세대 버튼, Compact = Dex.dc.html 등 앱 막대의 세대 버튼.
struct Metrics
{
    int height;   // 테두리 포함 높이
    int shadow;   // box-shadow: 0 Npx 0
    int paddingX; // padding: 0 Npx
    int gap;      // 글자 · ▾ 사이
    int labelPx;  // "4세대": 도현
    int chevron;  // ▾ 아이콘 크기 (viewBox 24)
};
constexpr Metrics kLarge {46, 3, 16, 10, 20, 16};
constexpr Metrics kCompact {36, 2, 12, 8, 16, 14};
constexpr int kBorder = 2;        // border: 2px solid ink
constexpr int kRadiusLarge = 8;   // border-radius: 인트로 8px
constexpr int kRadiusCompact = 6; // 〃 앱 막대 6px
constexpr int kPressedOffset = 2; // 눌림: 2px 내려앉고 그림자가 사라진다

const Metrics &metricsOf(com::yamada::studio::GenerationButton::Size size)
{
    return size == com::yamada::studio::GenerationButton::Size::Large ? kLarge : kCompact;
}

// 배지 바탕색(아주 옅은 틴트)은 섞어 놓으면 서로 비슷해 보인다 — 무지개가 읽히게 채도만 올린다
QColor vivid(const QColor &tint)
{
    const float saturation = qMin(1.0f, tint.hsvSaturationF() * 2.2f);
    return QColor::fromHsvF(qMax(0.0f, tint.hsvHueF()), saturation, tint.valueF());
}

QLinearGradient rainbowOf(const QList<QColor> &colors, const QPointF &from, const QPointF &to)
{
    QLinearGradient rainbow(from, to);
    for (qsizetype i = 0; i < colors.size(); ++i)
        rainbow.setColorAt(colors.size() == 1 ? 0 : qreal(i) / (colors.size() - 1),
                           vivid(colors.at(i)));
    return rainbow;
}
} // namespace

namespace com::yamada::studio {
GenerationButton::GenerationButton(Size size, QWidget *parent)
    : QAbstractButton(parent)
    , m_size(size)
    , m_font(theme::font(theme::kFamilyTitle, metricsOf(size).labelPx))
{
    QAbstractButton::setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    QAbstractButton::setCursor(cursors::pointer());
    connect(this, &QAbstractButton::clicked, this, &GenerationButton::showMenu);
}

void GenerationButton::setGeneration(int number)
{
    m_number = number;
    // 접근성: 화면 낭독기가 읽을 이름. 그림으로만 그린 글자는 스스로 알릴 수 없다.
    QAbstractButton::setAccessibleName(tr("세대 선택: %1").arg(label()));
    QAbstractButton::updateGeometry(); // 글자 폭이 바뀌면 sizeHint도 바뀐다 → 레이아웃에 다시 묻게
    QAbstractButton::update();
}

void GenerationButton::setGenerationRange(int first, int last)
{
    m_first = first;
    m_last = last;
}

QString GenerationButton::label() const
{
    return tr("%1세대").arg(m_number);
}

void GenerationButton::showMenu()
{
    // QMenu: 팝업 창(Qt::Popup). exec()는 고를 때까지 기다리는 동안 자기 이벤트 루프를 돌린다 —
    // 앱은 멈추지 않는다(다른 창의 그리기 · 타이머도 돈다). 메뉴 밖을 누르거나 Esc면 nullptr.
    // 모양은 app.qss의 QMenu 규칙(흰 바탕 · 먹선 2 · 도현 16).
    QMenu menu(this);
    menu.setMinimumWidth(width()); // 버튼보다 좁지 않게
    for (int n = m_first; n <= m_last; ++n) {
        QAction *action = menu.addAction(tr("%1세대").arg(n));
        action->setData(n);
        action->setCheckable(true);
        action->setChecked(n == m_number); // 지금 세대에 체크 표시
        // 줄 앞에 그 세대의 무지개 견본(버튼과 같은 색 띠)
        const QList<QColor> colors = dexstyle::generationColors(n);
        if (colors.isEmpty())
            continue;
        const qreal ratio = devicePixelRatioF();
        QPixmap swatch(qRound(26 * ratio), qRound(14 * ratio));
        swatch.setDevicePixelRatio(ratio);
        swatch.fill(Qt::transparent);
        QPainter painter(&swatch);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(QPen(QColor(tok::kInk), 1.5));
        painter.setBrush(rainbowOf(colors, QPointF(0, 0), QPointF(26, 0)));
        painter.drawRoundedRect(QRectF(1, 1, 24, 12), 4, 4);
        action->setIcon(QIcon(swatch));
    }
    // 버튼 바로 아래(그림자 아래)에, 버튼 왼쪽 끝에 맞춰 연다.
    const QPoint below
            = mapToGlobal(QPoint(0, metricsOf(m_size).height + metricsOf(m_size).shadow + 2));
    if (const QAction *chosen = menu.exec(below))
        emit generationSelected(chosen->data().toInt());
}

QSize GenerationButton::sizeHint() const
{
    const Metrics &m = metricsOf(m_size);
    const qreal width = 2 * kBorder + 2 * m.paddingX
                        + QFontMetricsF(m_font).horizontalAdvance(label()) + m.gap + m.chevron;
    return {qCeil(width), m.height + m.shadow};
}

void GenerationButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    const Metrics &m = metricsOf(m_size);
    const int radius = m_size == Size::Large ? kRadiusLarge : kRadiusCompact;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // isDown(): 마우스를 누르고 있는 동안 true (QAbstractButton이 관리).
    const bool down = QAbstractButton::isDown();
    const int offset = down ? kPressedOffset : 0;
    const QRectF box(0, offset, width(), m.height);

    // 1) 그림자 — 눌리면 버튼이 그림자 위로 내려앉은 것처럼 보이도록 그리지 않는다.
    if (!down) {
        QPainterPath shadow;
        shadow.addRoundedRect(box.translated(0, m.shadow), radius, radius);
        painter.fillPath(shadow, QColor(tok::kInk));
    }

    // 2) 본체: 그 세대 시리즈 색의 무지개 바탕(1세대 = 레드 → 그린 → 블루 → 옐로) + 먹선 2.
    // 색 목록이 없는 세대는 전처럼 흰 바탕(눌림은 노란 옅은 바탕). 펜은 선의 가운데를 따라
    // 그리므로 반 폭 안쪽으로.
    const qreal half = kBorder / 2.0;
    const QList<QColor> colors = dexstyle::generationColors(m_number);
    QBrush body(QColor(down ? tok::kYellowTint : tok::kWhite));
    if (!colors.isEmpty())
        body = rainbowOf(colors, box.topLeft(), box.topRight());
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(body);
    painter.drawRoundedRect(box.adjusted(half, half, -half, -half), radius - half, radius - half);

    // 3) "4세대" (도현)
    qreal x = kBorder + m.paddingX;
    const QString text = label();
    const qreal textW = QFontMetricsF(m_font).horizontalAdvance(text);
    painter.setFont(m_font);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRectF(x, box.top(), textW, box.height()), Qt::AlignVCenter | Qt::AlignLeft,
                     text);

    // 4) ▾ — SVG path "M6 9 l6 6 6 -6"(viewBox 24)를 줄여 그린다.
    x += textW + m.gap;
    const qreal scale = m.chevron / 24.0;
    const QPointF topLeft(x, box.center().y() - m.chevron / 2.0);
    QPainterPath chevron;
    chevron.moveTo(topLeft + QPointF(6, 9) * scale);
    chevron.lineTo(topLeft + QPointF(12, 15) * scale);
    chevron.lineTo(topLeft + QPointF(18, 9) * scale);
    QPen chevronPen(QColor(tok::kText1), 2.6 * scale);
    chevronPen.setCapStyle(Qt::FlatCap);    // SVG 기본값(stroke-linecap: butt)과 같게
    chevronPen.setJoinStyle(Qt::MiterJoin); // SVG 기본값(stroke-linejoin: miter)과 같게
    painter.setPen(chevronPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(chevron);
}
} // namespace com::yamada::studio
