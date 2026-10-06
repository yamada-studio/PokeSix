#include "ui/home/cardbarrel.h"

#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
using namespace com::yamada::studio;

// 카드 한 장 (비율 10 : 14, 창 높이에 따라 늘어난다)
constexpr qreal kCardRatio = 10.0 / 14.0;
constexpr qreal kCardMinHeight = 200;
constexpr qreal kCardMaxHeight = 320;
constexpr int kTopPad = 10; // 카드 위아래 여유(배럴은 카드가 기울지 않아 처짐이 없다)
constexpr int kBandHeight = 40; // 윗띠(세대 번호)
constexpr qreal kDepthScale = 0.20; // 뒤로 간 카드가 작아지는 몫 (정면 1 ↔ 모서리 1 − 0.20)
constexpr qreal kMinFacing = 0.06; // 가로 압축(cos)의 바닥 — 0이 되면 역변환이 깨진다
constexpr int kTurnMs = 420;       // 한 칸 돌리기
constexpr int kEnterMs = 760;      // 등장(반 바퀴쯤 돌면서 자리를 잡는다)
constexpr qreal kEnterOffset = 2.6; // 등장을 시작하는 각도(라디안)
constexpr qreal kDragThreshold = 6; // 이보다 멀리 끌면 클릭이 아니라 돌리기

constexpr PanelStyle kCardStyle {
        .outline = 2,
        .radius = 10,
        .shadow = 4,
        .fill = tok::kWhite,
        .ink = tok::kInk,
};

// Scale2x(EPX): 도트 그림을 2배로 키우면서 계단을 모서리 방향으로 채우는 고전 업스케일러.
// 보간(bilinear)과 달리 색을 섞지 않아 또렷하다. 두 번 적용(4배)한 뒤 목표 크기로 부드럽게
// 줄이면, 원본을 바로 2~3배 늘인 것보다 훨씬 깨끗하다.
QImage scale2x(const QImage &source)
{
    QImage result(source.size() * 2, QImage::Format_ARGB32);
    const int w = source.width();
    const int h = source.height();
    for (int y = 0; y < h; ++y) {
        const QRgb *row = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        const QRgb *above
                = reinterpret_cast<const QRgb *>(source.constScanLine(std::max(y - 1, 0)));
        const QRgb *below
                = reinterpret_cast<const QRgb *>(source.constScanLine(std::min(y + 1, h - 1)));
        QRgb *out0 = reinterpret_cast<QRgb *>(result.scanLine(y * 2));
        QRgb *out1 = reinterpret_cast<QRgb *>(result.scanLine(y * 2 + 1));
        for (int x = 0; x < w; ++x) {
            const QRgb p = row[x];
            const QRgb a = above[x];                    // 위
            const QRgb b = row[std::min(x + 1, w - 1)]; // 오른쪽
            const QRgb c = row[std::max(x - 1, 0)];     // 왼쪽
            const QRgb d = below[x];                    // 아래
            out0[x * 2] = (c == a && c != d && a != b) ? a : p;
            out0[x * 2 + 1] = (a == b && a != c && b != d) ? b : p;
            out1[x * 2] = (d == c && d != b && c != a) ? c : p;
            out1[x * 2 + 1] = (b == d && b != a && d != c) ? d : p;
        }
    }
    return result;
}

// 각도를 (−π, π]로 접는다 — "가장 가까운 카드" · 최단 방향 회전에 쓴다
qreal wrapAngle(qreal angle)
{
    while (angle > M_PI)
        angle -= 2 * M_PI;
    while (angle <= -M_PI)
        angle += 2 * M_PI;
    return angle;
}
} // namespace

namespace com::yamada::studio {
CardBarrel::CardBarrel(AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_state(state)
    , m_cards(homecards::all())
    , m_fronts(new SpriteCache(SpriteCache::Kind::PokemonFront, this))
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QWidget::setMinimumSize(640, int(kCardMinHeight) + 2 * kTopPad);
    QWidget::setMouseTracking(true);

    m_turnAnimation = new QVariantAnimation(this);
    m_turnAnimation->setDuration(kTurnMs);
    m_turnAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_turnAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        m_angle = v.toReal();
        QWidget::update();
    });

    connect(m_turnAnimation, &QAbstractAnimation::finished, this, &CardBarrel::commitGeneration);
    connect(m_fronts, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    // 다른 곳(앱 막대)에서 세대가 바뀌어도 그 카드가 정면으로 돌아오게
    connect(m_state, &AppState::generationChanged, this, [this](int generation) {
        for (int i = 0; i < int(m_cards.size()); ++i)
            if (m_cards.at(i).generation == generation && i != frontIndex())
                rotateTo(i);
        QWidget::update();
    });
    connect(m_state, &AppState::languageChanged, this, [this] { QWidget::update(); });
}

CardBarrel::Layout CardBarrel::barrelLayout() const
{
    Layout barrel;
    barrel.cardHeight = std::clamp<qreal>(height() - 2 * kTopPad, kCardMinHeight, kCardMaxHeight);
    barrel.cardWidth = barrel.cardHeight * kCardRatio;
    barrel.step = m_cards.isEmpty() ? 0 : 2 * M_PI / m_cards.size();
    // 반지름 = 옆 카드가 퍼지는 가로 거리. 창 폭 절반(카드 반 장 + 여유를 빼고)과
    // 카드 폭의 1.7배(홈에서 너무 퍼지지 않게) 중 작은 쪽.
    barrel.radius = std::max<qreal>(
            std::min(width() / 2.0 - barrel.cardWidth / 2 - 24, barrel.cardWidth * 1.7), 80);
    return barrel;
}

qreal CardBarrel::angleOf(const Layout &barrel, int index) const
{
    return wrapAngle(index * barrel.step + m_angle);
}

QTransform CardBarrel::cardTransform(const Layout &barrel, int index) const
{
    const qreal angle = angleOf(barrel, index);
    const qreal facing = std::cos(angle);
    // 깊이(cos 1 → 정면): 뒤로 갈수록 조금 작아진다. 가로는 곡면을 따라 cos만큼 더 눌린다.
    const qreal depth = 1 - kDepthScale * (1 - std::max(facing, 0.0));
    const qreal squeeze = std::max(std::abs(facing), kMinFacing);
    QTransform transform;
    transform.translate(width() / 2.0 + std::sin(angle) * barrel.radius, height() / 2.0);
    transform.scale(squeeze * depth, depth);
    transform.translate(0, -barrel.cardHeight / 2);
    return transform; // 카드 로컬 좌표: 윗변 가운데가 (0, 0), 카드는 (−w/2, 0, w, h)
}

QList<int> CardBarrel::paintOrder() const
{
    const Layout barrel = barrelLayout();
    QList<int> order;
    for (int i = 0; i < int(m_cards.size()); ++i)
        if (std::cos(angleOf(barrel, i)) > 0.02) // 모서리(90°)를 넘어간 카드는 보이지 않는다
            order.append(i);
    // 뒤(깊은 쪽)부터 그린다 → 정면이 맨 위
    std::sort(order.begin(), order.end(), [&](int a, int b) {
        return std::cos(angleOf(barrel, a)) < std::cos(angleOf(barrel, b));
    });
    return order;
}

int CardBarrel::cardAt(const QPointF &pos) const
{
    const Layout barrel = barrelLayout();
    const QList<int> order = paintOrder();
    for (qsizetype i = order.size() - 1; i >= 0; --i) { // 맨 위에 그려진 카드부터
        const int index = order.at(i);
        const QPointF local = cardTransform(barrel, index).inverted().map(pos);
        if (QRectF(-barrel.cardWidth / 2, 0, barrel.cardWidth, barrel.cardHeight).contains(local))
            return index;
    }
    return -1;
}

int CardBarrel::frontIndex() const
{
    const Layout barrel = barrelLayout();
    int front = 0;
    qreal best = -2;
    for (int i = 0; i < int(m_cards.size()); ++i) {
        const qreal facing = std::cos(angleOf(barrel, i));
        if (facing > best) {
            best = facing;
            front = i;
        }
    }
    return front;
}

void CardBarrel::rotateTo(int index)
{
    const Layout barrel = barrelLayout();
    // index 카드가 정면(각 0)에 오는 회전각으로, 최단 방향으로 돈다
    const qreal target = m_angle - angleOf(barrel, index);
    m_turnAnimation->stop();
    m_turnAnimation->setDuration(kTurnMs);
    m_turnAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_turnAnimation->setStartValue(m_angle);
    m_turnAnimation->setEndValue(target);
    m_turnAnimation->start();
    const int generation = m_cards.at(index).generation;
    m_pendingGeneration = generation == m_state->generation() ? 0 : generation;
    QWidget::update();
}

int CardBarrel::selectedGeneration() const
{
    return m_pendingGeneration != 0 ? m_pendingGeneration : m_state->generation();
}

void CardBarrel::commitGeneration()
{
    if (m_pendingGeneration == 0)
        return;
    const int generation = std::exchange(m_pendingGeneration, 0);
    m_state->setGeneration(generation);
}

void CardBarrel::snapToNearest()
{
    rotateTo(frontIndex());
}

void CardBarrel::selectNeighbor(int direction)
{
    // 정면의 오른쪽 이웃은 index + 1 (각이 +step) — → 는 다음 세대로
    const int count = int(m_cards.size());
    if (count == 0)
        return;
    rotateTo(((frontIndex() + direction) % count + count) % count);
}

void CardBarrel::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 보일 때마다 배럴이 반 바퀴쯤 돌면서 자리를 잡는다. 고른 세대가 정면에 오게.
    int selected = 0;
    for (int i = 0; i < int(m_cards.size()); ++i)
        if (m_cards.at(i).generation == m_state->generation())
            selected = i;
    const Layout barrel = barrelLayout();
    const qreal target = -selected * barrel.step;
    m_turnAnimation->stop();
    m_turnAnimation->setDuration(kEnterMs);
    m_turnAnimation->setEasingCurve(QEasingCurve::OutCubic);
    m_turnAnimation->setStartValue(target + kEnterOffset);
    m_turnAnimation->setEndValue(target);
    m_turnAnimation->start();
}

void CardBarrel::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    commitGeneration(); // 메뉴를 눌러 떠나면 다음 화면이 고른 세대로 열리게
}

void CardBarrel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_pressed = true;
    m_dragging = false;
    m_pressPos = event->position();
    m_pressAngle = m_angle;
}

void CardBarrel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_pressed) {
        const qreal dx = event->position().x() - m_pressPos.x();
        if (!m_dragging && std::abs(dx) > kDragThreshold)
            m_dragging = true; // 여기서부터는 클릭이 아니라 돌리기
        if (m_dragging) {
            m_turnAnimation->stop();
            m_angle = m_pressAngle + dx / barrelLayout().radius; // 끈 거리만큼 원통이 돈다
            QWidget::update();
        }
        return;
    }
    QWidget::setCursor(cardAt(event->position()) >= 0 ? cursors::pointer()
                                                      : QCursor(Qt::ArrowCursor));
}

void CardBarrel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton || !m_pressed)
        return;
    m_pressed = false;
    if (m_dragging) {
        m_dragging = false;
        snapToNearest(); // 놓으면 가장 가까운 카드가 정면에 걸리며 선택된다
        return;
    }
    const int clicked = cardAt(event->position());
    if (clicked >= 0 && clicked != frontIndex())
        rotateTo(clicked); // 옆 카드를 누르면 그 카드가 정면으로
}

void CardBarrel::wheelEvent(QWheelEvent *event)
{
    const int delta = event->angleDelta().y();
    if (delta == 0)
        return;
    selectNeighbor(delta < 0 ? 1 : -1);
    event->accept();
}

QPixmap CardBarrel::pokemonSprite(int pokemonId) const
{
    const auto it = m_mascots.constFind(pokemonId);
    if (it != m_mascots.constEnd())
        return *it;
    // 모든 세대가 같은 기본(96×96) 그림을 쓴다 — 세대 그림은 해상도 · 여백이 제각각이라
    // 카드마다 크기가 들쭉날쭉했다.
    const QString key = QStringLiteral("default/%1").arg(pokemonId);
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
    const QPixmap trimmed
            = QPixmap::fromImage(scale2x(scale2x(bounds.isNull() ? alpha : alpha.copy(bounds))));
    m_mascots.insert(pokemonId, trimmed);
    return trimmed;
}

void CardBarrel::paintCard(QPainter &painter, const Layout &barrel, int index) const
{
    const homecards::Card &card = m_cards.at(index);
    const QRect box(int(-barrel.cardWidth / 2), 0, int(barrel.cardWidth), int(barrel.cardHeight));
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
    const QString title = tr("%1세대").arg(card.generation);
    painter.drawText(band, Qt::AlignCenter, title);
    // 고른 카드(= 정면): 띠 글자 앞에 ▶ (메뉴의 선택 커서와 같은 문법)
    if (card.generation == selectedGeneration()) {
        const qreal textLeft
                = band.center().x() - QFontMetricsF(painter.font()).horizontalAdvance(title) / 2;
        QPainterPath cursor;
        cursor.moveTo(textLeft - 16, band.center().y() - 6);
        cursor.lineTo(textLeft - 6, band.center().y());
        cursor.lineTo(textLeft - 16, band.center().y() + 6);
        cursor.closeSubpath();
        painter.fillPath(cursor, QColor(tok::kWhite));
    }

    // 아래: 지방 이름 + 전국도감 구간 — 그림은 이 띠를 침범하지 않는다
    const QRect label(face.left(), face.bottom() - 46, face.width(), 24);
    painter.setFont(theme::font(theme::kFamilyBody, 14, QFont::Bold));
    painter.setPen(QColor(tok::kText1));
    QStringList regions;
    for (const LocalizedText &region : card.regions)
        regions.append(region.text(m_state->language()));
    painter.drawText(label, Qt::AlignCenter,
                     QFontMetricsF(painter.font())
                             .elidedText(regions.join(QStringLiteral(" · ")), Qt::ElideRight,
                                         label.width() - 12));
    if (card.rangeFrom > 0) {
        painter.setFont(theme::font(theme::kFamilyData, 11, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(face.left(), label.bottom(), face.width(), 16), Qt::AlignCenter,
                         QStringLiteral("No.%1–%2").arg(card.rangeFrom).arg(card.rangeTo));
    }

    // 무대: 액센트를 옅게 푼 바탕 + 사선 무늬 + 흐린 세대 숫자(인트로 배경과 같은 문법)
    const QRect stage(face.left() + 8, band.bottom() + 10, face.width() - 16,
                      label.top() - band.bottom() - 16);
    const auto mix = [&card](qreal keep) { // 액센트 ↔ 흰색
        const QColor a = card.accent;
        return QColor(int(a.red() * keep + 255 * (1 - keep)),
                      int(a.green() * keep + 255 * (1 - keep)),
                      int(a.blue() * keep + 255 * (1 - keep)));
    };
    QPainterPath stageClip;
    stageClip.addRoundedRect(stage, 8, 8);
    painter.save();
    painter.setClipPath(stageClip, Qt::IntersectClip);
    painter.fillPath(stageClip, mix(0.10));
    painter.setPen(QPen(mix(0.16), 7));
    for (int x = stage.left() - stage.height(); x < stage.right(); x += 18) // 사선 무늬
        painter.drawLine(QPointF(x, stage.bottom()), QPointF(x + stage.height(), stage.top()));
    painter.setFont(theme::font(theme::kFamilyTitle, int(stage.height() * 0.72)));
    painter.setPen(mix(0.26));
    painter.drawText(stage.adjusted(0, -6, 0, -6), Qt::AlignCenter,
                     QString::number(card.generation));

    // 시리즈별 스타팅 층: 첫 층(그 세대 첫 시리즈)이 맨 앞 바닥, 다음 시리즈는 한 층씩 작게 뒤 ·
    // 위로 쌓는다(겹쳐도 된다). 그리는 순서는 뒤 층부터. 층 안에서는 2번째가 가운데 앞(크게),
    // 양옆이 뒤
    struct Spot
    {
        qreal x;      // 가운데 기준 가로 위치(스테이지 폭 비율)
        qreal height; // 그림 높이(스테이지 높이 비율)
    };
    const Spot one[] = {{0, 0.52}};
    const Spot two[] = {{-0.17, 0.46}, {0.17, 0.46}};
    const Spot three[] = {{-0.30, 0.38}, {0, 0.52}, {0.30, 0.38}};
    const int layerCount = int(card.layers.size());
    // 층이 많을수록 앞 층을 조금 줄여 뒤 층이 머리 위로 보이게
    const qreal frontScale = layerCount <= 1 ? 1.0 : layerCount == 2 ? 0.86 : 0.76;
    const qreal lift = stage.height() * (layerCount == 2 ? 0.30 : 0.24); // 층 사이 높이
    for (int layer = layerCount - 1; layer >= 0; --layer) {
        const QList<int> &ids = card.layers.at(layer);
        const int count = std::min<int>(int(ids.size()), 3);
        const Spot *spots = count == 3 ? three : count == 2 ? two : one;
        const qreal layerScale = frontScale * std::pow(0.74, layer);
        const qreal ground = stage.bottom() - 12 - layer * lift;
        // 그리는 순서: 양옆 먼저, 가운데를 맨 위에
        const int order3[] = {0, 2, 1};
        for (int k = 0; k < count; ++k) {
            const int slot = count == 3 ? order3[k] : k;
            const QPixmap sprite = pokemonSprite(ids.at(slot));
            const Spot &spot = spots[slot];
            const qreal cx = stage.center().x() + spot.x * stage.width();
            // 뒤 층의 가운데는 앞 층의 큰 가운데에 가려지기 쉽다 → 반 층 더 올린다
            const qreal feet = ground - (layer > 0 && count == 3 && slot == 1 ? lift * 0.45 : 0);
            // 발밑 그림자(눌린 타원) — 서 있는 느낌. 뒤 층일수록 옅고 작게
            QColor shade(QColor(tok::kInk));
            shade.setAlphaF(layer == 0 ? 0.10 : 0.07);
            const qreal shadowW = stage.width() * (spot.height > 0.5 ? 0.30 : 0.22) * layerScale;
            painter.setPen(Qt::NoPen);
            painter.setBrush(shade);
            painter.drawEllipse(QPointF(cx, feet + 3), shadowW / 2, 6 * layerScale);
            if (sprite.isNull())
                continue;
            // 뒤 층은 셋을 같은 크기로(가운데를 키우면 앞 층을 더 가린다)
            const qreal height = layer > 0 ? std::min(spot.height, 0.42) : spot.height;
            qreal scale = stage.height() * height * layerScale / sprite.height();
            scale = std::min(scale, stage.width() * 0.34 * layerScale / sprite.width());
            const QSizeF size = sprite.size() * scale;
            painter.drawPixmap(QRectF(cx - size.width() / 2, feet - size.height(), size.width(),
                                      size.height()),
                               sprite, sprite.rect());
        }
    }
    painter.restore();
    painter.setPen(QPen(QColor(tok::kLine), 1.5)); // 무대 테
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(stage).adjusted(0.75, 0.75, -0.75, -0.75), 8, 8);

    // 고른 카드: 노란 테 (선택 칸의 문법, 디자인 시트 §3)
    if (card.generation == selectedGeneration()) {
        painter.setClipping(false);
        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(QColor(tok::kYellow), 3));
        painter.drawRoundedRect(QRectF(face).adjusted(2.5, 2.5, -2.5, -2.5), kCardStyle.radius - 3,
                                kCardStyle.radius - 3);
    }
    painter.restore();
}

void CardBarrel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    if (m_cards.isEmpty())
        return;
    const Layout barrel = barrelLayout();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const QList<int> order = paintOrder();
    for (const int index : order) {
        painter.save();
        // 모서리에 가까울수록 흐려지며 사라진다 — 뒤집혀 보이는 순간이 없게
        painter.setOpacity(std::clamp(std::cos(angleOf(barrel, index)) * 3.0, 0.0, 1.0));
        painter.setTransform(cardTransform(barrel, index), true);
        paintCard(painter, barrel, index);
        painter.restore();
    }
}
} // namespace com::yamada::studio
