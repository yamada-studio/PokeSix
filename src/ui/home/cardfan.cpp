#include "ui/home/cardfan.h"

#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"

#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QVariantAnimation>
#include <QtMath>

#include <algorithm>

namespace {
using namespace com::yamada::studio;

// 카드 한 장 (비율 10 : 14, 창 높이에 따라 늘어난다)
constexpr qreal kCardRatio = 10.0 / 14.0;
constexpr qreal kCardMinHeight = 150;
constexpr qreal kCardMaxHeight = 250;
constexpr int kLiftDistance = 30; // 들린 카드가 부채 바깥쪽으로 나오는 거리
constexpr int kTopPad = kLiftDistance + 14; // 들린 카드 위 여유
// 위젯 높이 → 카드 높이. 바깥 카드는 회전하면서 아래로 처지므로(반지름이 작을수록 더)
// 카드 높이 + 처짐 + 위 여유가 위젯 높이에 들어가야 한다.
constexpr qreal kHeightToCard = 1.45;
constexpr qreal kMaxHalfAngle = qDegreesToRadians(32.0); // 부채가 이보다 더 벌어지지 않는다
constexpr int kLiftRaiseMs = 150;                        // 들리는 건 빠르게
constexpr int kLiftDropMs = 240; // 내려오는 건 천천히 — 겹친 카드 사이를 지나도 덜 튄다
constexpr int kBandHeight = 40; // 윗띠(세대 번호)
constexpr int kSpreadMs = 620;  // 펼침
constexpr int kLiftMs = 140;    // 들림
constexpr int kSwingBackMs = 520; // 놓은 부채가 돌아오는 시간 (OutBack: 살짝 넘겼다 돌아온다)
constexpr qreal kDragThreshold = 6;      // 이보다 멀리 끌면 클릭이 아니라 흔들기
constexpr qreal kSwingPerPixel = 0.0011; // 끈 거리(px) → 부채 각도(라디안)
constexpr qreal kMaxSwing = 0.20;

constexpr PanelStyle kCardStyle {
        .outline = 2,
        .radius = 10,
        .shadow = 4,
        .fill = tok::kWhite,
        .ink = tok::kInk,
};
} // namespace

namespace com::yamada::studio {
CardFan::CardFan(AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
    , m_cards(homecards::all())
    , m_fronts(new SpriteCache(SpriteCache::Kind::PokemonFront, this))
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget::setMinimumSize(640, int(kCardMinHeight * kHeightToCard) + kTopPad);
    QWidget::setMouseTracking(true); // 버튼을 누르지 않아도 mouseMoveEvent(들림용 hover)를 받는다

    m_spreadAnimation = new QVariantAnimation(this);
    m_spreadAnimation->setDuration(kSpreadMs);
    m_spreadAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_spreadAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_spread = v.toReal();
        QWidget::update();
    });

    m_swingAnimation = new QVariantAnimation(this);
    m_swingAnimation->setDuration(kSwingBackMs);
    m_swingAnimation->setEasingCurve(QEasingCurve::OutBack);
    connect(m_swingAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_swing = v.toReal();
        QWidget::update();
    });

    for (size_t i = 0; i < m_liftAnimations.size(); ++i) {
        QVariantAnimation *lift = new QVariantAnimation(this);
        lift->setDuration(kLiftMs);
        lift->setEasingCurve(QEasingCurve::OutCubic);
        connect(lift, &QVariantAnimation::valueChanged, this, [this, i](const QVariant &v) {
            m_lift[i] = v.toReal();
            QWidget::update();
        });
        m_liftAnimations[i] = lift;
    }

    connect(m_fronts, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    // 다른 곳(앱 막대)에서 세대가 바뀌어도 고른 카드가 따라가게
    connect(m_state, &AppState::generationChanged, this, [this](int) {
        for (int i = 0; i < int(m_cards.size()); ++i)
            animateLift(i);
        QWidget::update();
    });
    connect(m_state, &AppState::languageChanged, this, [this] { QWidget::update(); });
}

void CardFan::selectNeighbor(int direction)
{
    for (int i = 0; i < int(m_cards.size()); ++i) {
        if (m_cards.at(i).generation != m_state->generation())
            continue;
        const int next = std::clamp(i + direction, 0, int(m_cards.size()) - 1);
        select(next);
        return;
    }
    if (!m_cards.isEmpty())
        select(0);
}

CardFan::Layout CardFan::fanLayout() const
{
    Layout fan;
    fan.cardHeight = std::clamp<qreal>((height() - kTopPad) / kHeightToCard, kCardMinHeight,
                                       kCardMaxHeight);
    fan.cardWidth = fan.cardHeight * kCardRatio;
    // 반지름이 작을수록 손에 쥔 패처럼 원심 쪽으로 모인다(겹침이 깊어지고 폭이 준다)
    fan.radius = fan.cardHeight * 2.4;
    // 부채의 가로 폭: 창 폭과, 카드 높이의 1.5배(홈에서 부채가 퍼지지 않게) 중 작은 쪽
    const qreal half = std::max<qreal>(
            std::min(width() / 2.0 - fan.cardWidth / 2 - 20, fan.cardHeight * 1.5), 60);
    const qreal reach = fan.radius - fan.cardHeight / 2;
    const qreal maxAngle = std::min(kMaxHalfAngle, std::asin(std::min(half / reach, 1.0)));
    fan.step = m_cards.size() > 1 ? 2 * maxAngle / (m_cards.size() - 1) : 0;
    fan.pivot = QPointF(width() / 2.0, fan.radius + kTopPad);
    return fan;
}

QTransform CardFan::cardTransform(const Layout &fan, int index, bool withLift) const
{
    const qreal middle = (m_cards.size() - 1) / 2.0;
    const qreal angle = (index - middle) * fan.step * m_spread + m_swing;
    QTransform transform;
    transform.translate(fan.pivot.x(), fan.pivot.y());
    transform.rotateRadians(angle);
    transform.translate(0, -(fan.radius + (withLift ? m_lift[size_t(index)] * kLiftDistance : 0)));
    return transform; // 카드 로컬 좌표: 윗변 가운데가 (0, 0), 카드는 (−w/2, 0, w, h)
}

QPixmap CardFan::mascotSprite(int index) const
{
    const homecards::Card &card = m_cards.at(index);
    const auto it = m_mascots.constFind(card.mascot);
    if (it != m_mascots.constEnd())
        return *it;
    // 모든 세대가 같은 기본(96×96) 그림을 쓴다 — 세대 그림은 해상도(1세대 40 ~ 9세대 256)와
    // 여백이 제각각이라 카드마다 크기가 들쭉날쭉했다.
    const QString key = QStringLiteral("default/%1").arg(card.mascot);
    const QString file = m_fronts->path(key);
    QImage image;
    if (file.isEmpty() || !image.load(file)) {
        m_fronts->request(key);
        return {};
    }
    // 투명 여백 잘라 내기: 알파가 있는 픽셀의 경계 상자
    QRect bounds;
    const QImage alpha = image.convertToFormat(QImage::Format_ARGB32);
    for (int y = 0; y < alpha.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(alpha.constScanLine(y));
        for (int x = 0; x < alpha.width(); ++x)
            if (qAlpha(line[x]) > 8)
                bounds = bounds.isNull() ? QRect(x, y, 1, 1) : bounds.united(QRect(x, y, 1, 1));
    }
    const QPixmap trimmed = QPixmap::fromImage(bounds.isNull() ? alpha : alpha.copy(bounds));
    m_mascots.insert(card.mascot, trimmed);
    return trimmed;
}

QList<int> CardFan::paintOrder() const
{
    QList<int> order;
    for (int i = 0; i < int(m_cards.size()); ++i)
        order.append(i);
    // 왼쪽 → 오른쪽(오른쪽 카드가 위로 겹친다), 들린 카드는 그 위로
    std::stable_sort(order.begin(), order.end(),
                     [this](int a, int b) { return m_lift[size_t(a)] < m_lift[size_t(b)]; });
    return order;
}

int CardFan::cardAt(const QPointF &pos, bool withLift) const
{
    const Layout fan = fanLayout();
    const QList<int> order = paintOrder();
    for (qsizetype i = order.size() - 1; i >= 0; --i) { // 맨 위에 그려진 카드부터
        const int index = order.at(i);
        const QPointF local = cardTransform(fan, index, withLift).inverted().map(pos);
        if (QRectF(-fan.cardWidth / 2, 0, fan.cardWidth, fan.cardHeight).contains(local))
            return index;
    }
    return -1;
}

int CardFan::hoverCardAt(const QPointF &pos) const
{
    // 들림(애니메이션 중 포함)을 기준으로 잡으면, 카드가 움직이는 동안 마우스 아래 카드가 바뀌어
    // hover가 이웃과 왔다 갔다 튄다. 그래서:
    //   1) 지금 hover 카드가 (들린 모습 그대로) 아직 마우스 아래면 그대로 둔다
    //   2) 아니면 모두 제자리에 있다고 치고(들림 무시) 판정한다 → 판정이 흔들리지 않는다
    if (m_hovered >= 0) {
        const Layout fan = fanLayout();
        const QPointF local = cardTransform(fan, m_hovered, true).inverted().map(pos);
        if (QRectF(-fan.cardWidth / 2, 0, fan.cardWidth, fan.cardHeight).contains(local))
            return m_hovered;
    }
    return cardAt(pos, false);
}

qreal CardFan::liftTarget(int index) const
{
    if (index == m_hovered)
        return 1.0;
    if (m_cards.at(index).generation == m_state->generation())
        return 0.5; // 고른 카드는 조금 들린 채로 둔다(너무 들면 이웃 윗띠를 가린다)
    return 0.0;
}

void CardFan::animateLift(int index)
{
    QVariantAnimation *animation = m_liftAnimations[size_t(index)];
    const qreal target = liftTarget(index);
    animation->stop();
    animation->setDuration(target > m_lift[size_t(index)] ? kLiftRaiseMs : kLiftDropMs);
    animation->setStartValue(m_lift[size_t(index)]);
    animation->setEndValue(target);
    animation->start();
}

void CardFan::setHovered(int index)
{
    if (index == m_hovered)
        return;
    const int before = m_hovered;
    m_hovered = index;
    if (before >= 0)
        animateLift(before);
    if (m_hovered >= 0)
        animateLift(m_hovered);
    QWidget::setCursor(m_hovered >= 0 ? cursors::pointer() : QCursor(Qt::ArrowCursor));
}

void CardFan::select(int index)
{
    m_state->setGeneration(m_cards.at(index).generation); // generationChanged → 들림 갱신
}

void CardFan::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 보일 때마다 가운데에 모인 패를 다시 펼친다 (인트로로 돌아올 때도)
    m_spreadAnimation->stop();
    m_spreadAnimation->setStartValue(0.0);
    m_spreadAnimation->setEndValue(1.0);
    m_spreadAnimation->start();
    for (int i = 0; i < int(m_cards.size()); ++i)
        animateLift(i);
}

void CardFan::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_pressed = cardAt(event->position(), true);
    m_pressPos = event->position();
    m_pressSwing = m_swing;
    m_dragging = false;
}

void CardFan::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed >= 0 || m_dragging) {
        const qreal dx = event->position().x() - m_pressPos.x();
        if (!m_dragging && std::abs(dx) > kDragThreshold) {
            m_dragging = true; // 여기서부터는 클릭이 아니라 부채 흔들기
            setHovered(-1);
        }
        if (m_dragging) {
            m_swingAnimation->stop();
            m_swing = std::clamp(m_pressSwing + dx * kSwingPerPixel, -kMaxSwing, kMaxSwing);
            QWidget::update();
        }
        return;
    }
    setHovered(hoverCardAt(event->position()));
}

void CardFan::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const int pressed = m_pressed;
    m_pressed = -1;
    if (m_dragging) {
        m_dragging = false;
        m_swingAnimation->stop(); // 놓으면 튕기며 제자리로
        m_swingAnimation->setStartValue(m_swing);
        m_swingAnimation->setEndValue(0.0);
        m_swingAnimation->start();
        setHovered(hoverCardAt(event->position()));
        return;
    }
    if (pressed >= 0 && pressed == cardAt(event->position(), true))
        select(pressed);
}

void CardFan::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    setHovered(-1);
}

void CardFan::paintCard(QPainter &painter, const Layout &fan, int index) const
{
    const homecards::Card &card = m_cards.at(index);
    const QRect box(int(-fan.cardWidth / 2), 0, int(fan.cardWidth), int(fan.cardHeight));
    paintPanel(painter, box, kCardStyle);

    // 그림자 · 먹선 안쪽 면. 이후 내용은 면의 둥근 모서리로 자른다(윗띠가 모서리를 덮지 않게).
    const QRect face = box.adjusted(kCardStyle.outline, kCardStyle.outline, -kCardStyle.outline,
                                    -kCardStyle.outline - kCardStyle.shadow);
    QPainterPath clip;
    clip.addRoundedRect(face, kCardStyle.radius - 2, kCardStyle.radius - 2);
    painter.save();
    painter.setClipPath(clip);

    // 윗띠: 세대 번호. 띠 아래 먹선 2px.
    const QRect band(face.left(), face.top(), face.width(), kBandHeight);
    painter.fillRect(band, card.accent);
    painter.fillRect(QRect(band.left(), band.bottom() + 1, band.width(), 2), QColor(tok::kInk));
    painter.setFont(theme::font(theme::kFamilyTitle, 20));
    painter.setPen(QColor(tok::kWhite));
    painter.drawText(band, Qt::AlignCenter, tr("%1세대").arg(card.generation));

    // 아래: 지방 이름 띠(고정 높이) — 그림은 이 띠를 침범하지 않는다
    const QRect label(face.left(), face.bottom() - 32, face.width(), 26);
    painter.setFont(theme::font(theme::kFamilyBody, 14, QFont::Bold));
    painter.setPen(QColor(tok::kText1));
    painter.drawText(label, Qt::AlignCenter, card.region.text(m_state->language()));

    // 가운데: 마스코트. 투명 여백을 잘라 내고 모두 같은 높이로 바닥선에 세운다 — 원본의 해상도 ·
    // 여백이 제각각이어도 카드끼리 크기 · 자리가 맞게. 아직 없으면 받기 시작한다 → ready가 update.
    const QRect spriteBox(face.left() + 10, band.bottom() + 10, face.width() - 20,
                          label.top() - band.bottom() - 16);
    const QPixmap sprite = mascotSprite(index);
    if (!sprite.isNull()) {
        qreal scale = spriteBox.height() / qreal(sprite.height());
        scale = std::min(scale, spriteBox.width() / qreal(sprite.width()));
        const QSizeF size = sprite.size() * scale;
        const QRectF target(spriteBox.center().x() - size.width() / 2,
                            spriteBox.bottom() - size.height(), size.width(), size.height());
        painter.drawPixmap(target, sprite, sprite.rect()); // SmoothPixmapTransform은 paintEvent에서
    }

    // 고른 카드: 노란 테 (선택 칸의 문법, 디자인 시트 §3)
    if (card.generation == m_state->generation()) {
        painter.setClipping(false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(tok::kYellow), 3));
        painter.drawRoundedRect(QRectF(face).adjusted(2.5, 2.5, -2.5, -2.5), kCardStyle.radius - 3,
                                kCardStyle.radius - 3);
    }
    painter.restore();
}

void CardFan::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_cards.isEmpty())
        return;
    const Layout fan = fanLayout();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true); // 회전한 카드의 그림 보간
    const QList<int> order = paintOrder();
    for (const int index : order) {
        painter.save();
        painter.setTransform(cardTransform(fan, index), true);
        paintCard(painter, fan, index);
        painter.restore();
    }
}
} // namespace com::yamada::studio
