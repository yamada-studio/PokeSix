#include "ui/widgets/generationbutton.h"

#include "ui/theme/cursors.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <functional>

namespace {
using namespace com::yamada::studio;

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

// 단색 띠를 나란히: 그라데이션으로 섞으면 무슨 색인지 안 보인다(사용자 결정). 같은 색을 띠의
// 양 끝에 두 번 찍으면 QLinearGradient가 섞지 않고 딱 끊어 그린다
QLinearGradient stripesOf(const QList<QColor> &colors, const QPointF &from, const QPointF &to)
{
    QLinearGradient stripes(from, to);
    const qreal step = 1.0 / colors.size();
    for (qsizetype i = 0; i < colors.size(); ++i) {
        const QColor color = vivid(colors.at(i));
        stripes.setColorAt(i * step, color);
        stripes.setColorAt(qMin(1.0, (i + 1) * step - 0.0001), color);
    }
    return stripes;
}

// 세대 드롭다운: QMenu 대신 직접 그린다 — 줄마다 그 세대의 색 띠를 배경으로 깔고(아이콘 견본
// 대신), 폭을 버튼과 똑같이 맞추기 위해서다. Qt::Popup이라 바깥을 누르면 저절로 닫힌다.
// 시그널이 필요 없도록 고르면 부를 콜백을 받는다(Q_OBJECT 없이).
class GenerationMenu : public QWidget
{
public:
    GenerationMenu(int first, int last, int current, int width, std::function<void(int)> onPick,
                   QWidget *parent)
        : QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)
        , m_first(first)
        , m_current(current)
        , m_onPick(std::move(onPick))
        , m_font(theme::font(theme::kFamilyTitle, 16))
    {
        QWidget::setAttribute(Qt::WA_DeleteOnClose);
        QWidget::setMouseTracking(true);
        m_rows = last - first + 1;
        m_hover = current - first;
        QWidget::setFixedSize(width, kBorderWidth * 2 + m_rows * kRowHeight);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        // 틀: 흰 바탕 + 먹선 2 (app.qss의 QMenu와 같은 말투)
        const qreal half = kBorderWidth / 2.0;
        painter.setPen(QPen(QColor(tok::kInk), kBorderWidth));
        painter.setBrush(QColor(tok::kWhite));
        painter.drawRoundedRect(QRectF(rect()).adjusted(half, half, -half, -half), kRadius - half,
                                kRadius - half);
        QPainterPath inner;
        inner.addRoundedRect(
                QRectF(rect()).adjusted(kBorderWidth, kBorderWidth, -kBorderWidth, -kBorderWidth),
                kRadius - kBorderWidth, kRadius - kBorderWidth);
        painter.setClipPath(inner);
        for (int i = 0; i < m_rows; ++i) {
            const QRectF row(kBorderWidth, kBorderWidth + i * kRowHeight,
                             width() - 2 * kBorderWidth, kRowHeight);
            // 줄 배경 = 그 세대의 단색 띠(버튼과 같은 배열)
            const QList<QColor> colors = dexstyle::generationColors(m_first + i);
            if (colors.isEmpty())
                painter.fillRect(row, QColor(tok::kWhite));
            else
                painter.fillRect(row, stripesOf(colors, row.topLeft(), row.topRight()));
            if (i == m_hover) // 올린 줄: 먹색을 옅게 덮는다
                painter.fillRect(row, QColor(0, 0, 0, 26));
            painter.setPen(QColor(tok::kText1));
            painter.setFont(m_font);
            painter.drawText(row.adjusted(12, 0, -10, 0), Qt::AlignLeft | Qt::AlignVCenter,
                             GenerationButton::tr("%1세대").arg(m_first + i));
            if (m_first + i == m_current) // 지금 세대 표시
                painter.drawText(row.adjusted(12, 0, -10, 0), Qt::AlignRight | Qt::AlignVCenter,
                                 QStringLiteral("✓"));
            if (i < m_rows - 1) // 줄 구분: 마지막만 빼고 아래 먹선 1px
                painter.fillRect(QRectF(row.left(), row.bottom() - 1, row.width(), 1),
                                 QColor(tok::kInk));
        }
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        const int row = rowAt(event->position().y());
        if (row != m_hover) {
            m_hover = row;
            QWidget::update();
        }
    }

    void mouseReleaseEvent(QMouseEvent *event) override
    {
        const int row = rowAt(event->position().y());
        if (row >= 0 && rect().contains(event->position().toPoint()))
            pick(row);
    }

    void keyPressEvent(QKeyEvent *event) override
    {
        switch (event->key()) {
        case Qt::Key_Up:
            m_hover = (m_hover + m_rows - 1) % m_rows;
            QWidget::update();
            break;
        case Qt::Key_Down:
            m_hover = (m_hover + 1) % m_rows;
            QWidget::update();
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (m_hover >= 0)
                pick(m_hover);
            break;
        case Qt::Key_Escape:
            QWidget::close();
            break;
        default:
            QWidget::keyPressEvent(event);
        }
    }

private:
    static constexpr int kBorderWidth = 2;
    static constexpr int kRadius = 6;
    static constexpr int kRowHeight = 34;

    int rowAt(qreal y) const
    {
        const int row = int((y - kBorderWidth) / kRowHeight);
        return row < 0 || row >= m_rows ? -1 : row;
    }

    void pick(int row)
    {
        const int number = m_first + row;
        QWidget::close();
        m_onPick(number);
    }

    int m_first = 1;
    int m_rows = 0;
    int m_current = 1;
    int m_hover = -1;
    std::function<void(int)> m_onPick;
    QFont m_font;
};
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
    // 버튼과 같은 폭의 색 띠 메뉴(위 GenerationMenu). 버튼 바로 아래(그림자 아래), 왼쪽 맞춤.
    GenerationMenu *menu = new GenerationMenu(
            m_first, m_last, m_number, width(),
            [this](int number) { emit generationSelected(number); }, this);
    menu->move(mapToGlobal(QPoint(0, metricsOf(m_size).height + metricsOf(m_size).shadow + 2)));
    menu->show();
    menu->setFocus();
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

    // 2) 본체: 그 세대 시리즈 색의 단색 띠 바탕(1세대 = [레드|그린|블루|옐로]) + 먹선 2.
    // 색 목록이 없는 세대는 전처럼 흰 바탕(눌림은 노란 옅은 바탕). 펜은 선의 가운데를 따라
    // 그리므로 반 폭 안쪽으로.
    const qreal half = kBorder / 2.0;
    const QList<QColor> colors = dexstyle::generationColors(m_number);
    QBrush body(QColor(down ? tok::kYellowTint : tok::kWhite));
    if (!colors.isEmpty())
        body = stripesOf(colors, box.topLeft(), box.topRight());
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(body);
    painter.drawRoundedRect(box.adjusted(half, half, -half, -half), radius - half, radius - half);

    // 3) ▾ — 글자 앞에(사용자 결정: [▾ 4세대]). SVG path "M6 9 l6 6 6 -6"(viewBox 24)를 줄여
    // 그린다.
    qreal x = kBorder + m.paddingX;
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

    // 4) "4세대" (도현)
    x += m.chevron + m.gap;
    const QString text = label();
    const qreal textW = QFontMetricsF(m_font).horizontalAdvance(text);
    painter.setFont(m_font);
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRectF(x, box.top(), textW, box.height()), Qt::AlignVCenter | Qt::AlignLeft,
                     text);
}
} // namespace com::yamada::studio
