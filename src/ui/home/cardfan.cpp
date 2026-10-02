#include "ui/home/cardfan.h"

#include "data/sprites/spritecache.h"
#include "data/sprites/spritekeys.h"
#include "data/state/appstate.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"
#include "ui/widgets/spritefit.h"

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
constexpr qreal kCardMinHeight = 140;
constexpr qreal kCardMaxHeight = 210;
constexpr int kLiftDistance = 26; // 들린 카드가 부채 바깥쪽으로 나오는 거리
constexpr int kTopPad = kLiftDistance + 14; // 들린 카드 위 여유
// 위젯 높이 → 카드 높이. 바깥 카드는 회전하면서 아래로 처지므로(≈ 0.55 × 카드 높이)
// 카드 높이 + 처짐 + 위 여유가 위젯 높이에 들어가야 한다.
constexpr qreal kHeightToCard = 1.55;
constexpr qreal kMaxHalfAngle = qDegreesToRadians(28.0); // 부채가 이보다 더 벌어지지 않는다
constexpr int kBandHeight = 40;                          // 윗띠(세대 번호)
constexpr int kSpreadMs = 620;                           // 펼침
constexpr int kLiftMs = 140;                             // 들림
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
    // 반지름이 클수록 부채가 평평해지고 이웃 카드 사이가 벌어진다(카드가 서로 덜 가린다)
    fan.radius = fan.cardHeight * 4.7;
    // 부채가 창 폭을 넘지 않게: 바깥 카드 가운데의 가로 거리 ≈ sin(θmax) × (R − h/2)
    const qreal half = std::max<qreal>(width() / 2.0 - fan.cardWidth / 2 - 20, 60);
    const qreal reach = fan.radius - fan.cardHeight / 2;
    const qreal maxAngle = std::min(kMaxHalfAngle, std::asin(std::min(half / reach, 1.0)));
    fan.step = m_cards.size() > 1 ? 2 * maxAngle / (m_cards.size() - 1) : 0;
    fan.pivot = QPointF(width() / 2.0, fan.radius + kTopPad);
    return fan;
}

QTransform CardFan::cardTransform(const Layout &fan, int index) const
{
    const qreal middle = (m_cards.size() - 1) / 2.0;
    const qreal angle = (index - middle) * fan.step * m_spread + m_swing;
    QTransform transform;
    transform.translate(fan.pivot.x(), fan.pivot.y());
    transform.rotateRadians(angle);
    transform.translate(0, -(fan.radius + m_lift[size_t(index)] * kLiftDistance));
    return transform; // 카드 로컬 좌표: 윗변 가운데가 (0, 0), 카드는 (−w/2, 0, w, h)
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

int CardFan::cardAt(const QPointF &pos) const
{
    const Layout fan = fanLayout();
    const QList<int> order = paintOrder();
    for (qsizetype i = order.size() - 1; i >= 0; --i) { // 맨 위에 그려진 카드부터
        const int index = order.at(i);
        const QPointF local = cardTransform(fan, index).inverted().map(pos);
        if (QRectF(-fan.cardWidth / 2, 0, fan.cardWidth, fan.cardHeight).contains(local))
            return index;
    }
    return -1;
}

qreal CardFan::liftTarget(int index) const
{
    if (index == m_hovered)
        return 1.0;
    if (m_cards.at(index).generation == m_state->generation())
        return 0.62; // 고른 카드는 조금 들린 채로 둔다
    return 0.0;
}

void CardFan::animateLift(int index)
{
    QVariantAnimation *animation = m_liftAnimations[size_t(index)];
    const qreal target = liftTarget(index);
    animation->stop();
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
    m_pressed = cardAt(event->position());
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
    setHovered(cardAt(event->position()));
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
        setHovered(cardAt(event->position()));
        return;
    }
    if (pressed >= 0 && pressed == cardAt(event->position()))
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

    // 가운데: 마스코트(그 세대의 정면 스프라이트). 아직 없으면 받기 시작한다 → ready가 update.
    const QRect spriteBox(face.left() + 8, band.bottom() + 8, face.width() - 16,
                          face.bottom() - band.bottom() - 40);
    const QString key = spritekeys::front(card.generation, card.mascot);
    const QString file = m_fronts->path(key);
    QPixmap sprite;
    if (file.isEmpty() || !sprite.load(file))
        m_fronts->request(key);
    else
        spritefit::draw(painter, spriteBox, sprite);

    // 아래: 지방 이름
    painter.setFont(theme::font(theme::kFamilyBody, 14, QFont::Bold));
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRect(face.left(), face.bottom() - 30, face.width(), 24), Qt::AlignCenter,
                     card.region.text(m_state->language()));

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
