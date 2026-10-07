#include "ui/map/mapview.h"

#include "data/sprites/spritecache.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QWheelEvent>

#include <algorithm>

namespace com::yamada::studio {
MapView::MapView(QWidget *parent)
    : QWidget(parent)
    , m_cache(new SpriteCache(SpriteCache::Kind::TownMap, this))
{
    QWidget::setMouseTracking(true);
    QWidget::setMinimumSize(320, 280);
    QWidget::setCursor(Qt::PointingHandCursor);
    connect(m_cache, &SpriteCache::ready, this, [this] {
        m_scaledFor = 0; // 받아졌다 — 다음 그리기에서 확대본을 만든다
        QWidget::update();
    });
}

void MapView::setRegion(const QString &region, const QHash<QString, LocalizedText> &names,
                        Language language)
{
    const townmapbook::RegionMap &map = townmapbook::regionMap(region);
    m_map = map.isValid() ? &map : nullptr;
    m_names = names;
    m_language = language;
    m_zoom = 0;
    m_pan = {};
    m_hover.clear();
    m_selected.clear();
    m_scaledFor = 0;
    QWidget::update();
}

int MapView::fitScale() const
{
    // 세로를 꽉 채우는 배율이 기본이다 — 성도 · 관동 합본처럼 긴 지도는 가로가 넘치고,
    // 넘친 만큼 끌어서(드래그) 본다. 세로가 맞으면 빈 공간이 크게 남지 않는다
    if (!m_map)
        return 1;
    return std::max(1, (height() - 24) / m_map->canvas.height());
}

int MapView::scale() const
{
    return std::max(1, fitScale() + m_zoom);
}

QPoint MapView::origin() const
{
    if (!m_map)
        return {};
    const QSize content = m_map->canvas * scale();
    // 내용이 창보다 작으면 가운데, 크면 팬(클램프)
    const int x
            = content.width() <= width() ? (width() - content.width()) / 2 : clampPan(m_pan).x();
    const int y = content.height() <= height() ? (height() - content.height()) / 2
                                               : clampPan(m_pan).y();
    return {x, y};
}

QPoint MapView::clampPan(QPoint pan) const
{
    const QSize content = m_map->canvas * scale();
    pan.setX(std::clamp(pan.x(), std::min(0, width() - content.width()), 0));
    pan.setY(std::clamp(pan.y(), std::min(0, height() - content.height()), 0));
    return pan;
}

QRect MapView::nodeRect(const townmapbook::Node &node) const
{
    const int s = scale();
    return QRect(origin() + QPoint(node.rect.x() * s, node.rect.y() * s), node.rect.size() * s);
}

const townmapbook::Node *MapView::nodeAt(const QPoint &widgetPos) const
{
    if (!m_map)
        return nullptr;
    const townmapbook::Node *best = nullptr;
    for (const townmapbook::Node &node : m_map->nodes)
        if (nodeRect(node).contains(widgetPos))
            // 작은 노드(마을)가 큰 노드(도로 띠) 위에 겹쳐 있다 — 작은 쪽을 고른다
            if (!best
                || node.rect.width() * node.rect.height()
                           < best->rect.width() * best->rect.height())
                best = &node;
    return best;
}

void MapView::ensurePixmap()
{
    const int s = scale();
    if (!m_map || m_scaledFor == s)
        return;
    // 레이어 그림이 다 받아졌을 때만 합성한다(합본은 성도 + 관동 두 장)
    QList<QPixmap> sources;
    for (const townmapbook::Layer &layer : m_map->layers) {
        const QString file = m_cache->path(layer.source);
        if (file.isEmpty()) {
            m_cache->request(layer.source);
            m_scaled = QPixmap();
            m_scaledFor = 0;
            return;
        }
        sources.append(QPixmap(file));
    }
    QPixmap canvas(m_map->canvas);
    canvas.fill(Qt::transparent);
    {
        QPainter painter(&canvas);
        for (qsizetype i = 0; i < m_map->layers.size(); ++i)
            painter.drawPixmap(m_map->layers.at(i).offset,
                               sources.at(i).copy(m_map->layers.at(i).crop));
    }
    m_scaled = canvas.scaled(canvas.size() * s, Qt::IgnoreAspectRatio,
                             Qt::FastTransformation); // 도트 유지(니어리스트)
    m_scaledFor = s;
}

void MapView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.fillRect(rect(), QColor(tok::kPaperAlt));
    if (!m_map) {
        painter.setFont(theme::font(theme::kFamilyBody, 14, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(rect(), Qt::AlignCenter, tr("이 지방의 지도는 아직 준비 중이에요"));
        return;
    }
    ensurePixmap();
    const QPoint topLeft = origin();
    if (!m_scaled.isNull()) {
        painter.drawPixmap(topLeft, m_scaled);
    } else {
        // 아직 그림이 없다(받는 중 · 오프라인) — 노드만으로 그리는 스키매틱 폴백
        const int s = scale();
        const QSize view = m_map->canvas;
        painter.fillRect(QRect(topLeft, view * s), QColor(tok::kPaper));
        painter.setPen(QPen(QColor(tok::kPaperStripe), 1));
        for (int x = 0; x <= view.width(); x += 10)
            painter.drawLine(topLeft.x() + x * s, topLeft.y(), topLeft.x() + x * s,
                             topLeft.y() + view.height() * s);
        for (int y = 0; y <= view.height(); y += 10)
            painter.drawLine(topLeft.x(), topLeft.y() + y * s, topLeft.x() + view.width() * s,
                             topLeft.y() + y * s);
        for (const townmapbook::Node &node : m_map->nodes) {
            painter.setPen(QPen(QColor(tok::kInk), 1.5));
            painter.setBrush(QColor(tok::kBlueCell));
            painter.drawRect(nodeRect(node));
        }
        painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(0, height() - 26, width(), 22), Qt::AlignCenter,
                         tr("지도 그림을 받는 중이에요…"));
    }

    painter.setRenderHint(QPainter::Antialiasing);
    // 선택: 노랑 테(스쿼드 카드의 선택 테와 같은 말)
    auto outline = [&](const QString &location, QRgb color, qreal width) {
        QRect first;
        for (const townmapbook::Node &node : m_map->nodes)
            if (node.location == location) { // 같은 장소의 조각(합본 경계)을 전부 두른다
                painter.setPen(QPen(QColor(color), width));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(QRectF(nodeRect(node)).adjusted(-2, -2, 2, 2), 3, 3);
                if (first.isNull())
                    first = nodeRect(node);
            }
        return first;
    };
    if (!m_selected.isEmpty())
        outline(m_selected, tok::kYellow, 3);
    if (!m_hover.isEmpty() && m_hover != m_selected) {
        const QRect r = outline(m_hover, tok::kInk, 2);
        // 말풍선: 노드 위(공간이 없으면 아래)에 장소 이름
        const QString name = m_names.value(m_hover).text(m_language);
        if (!name.isEmpty() && !r.isNull()) {
            painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::ExtraBold));
            const QSizeF text = QFontMetricsF(painter.font()).size(0, name);
            QRectF bubble(0, 0, text.width() + 16, text.height() + 8);
            bubble.moveCenter(QPointF(r.center().x(), 0));
            bubble.moveBottom(r.top() - 6);
            if (bubble.top() < 4)
                bubble.moveTop(r.bottom() + 6);
            bubble.moveLeft(std::clamp(bubble.left(), 4.0, width() - bubble.width() - 4));
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(tok::kInk));
            painter.drawRoundedRect(bubble, 5, 5);
            painter.setPen(QColor(tok::kWhite));
            painter.drawText(bubble, Qt::AlignCenter, name);
        }
    }
}

void MapView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        m_pan = clampPan(m_panStart + (event->pos() - m_dragStart));
        QWidget::update();
        return;
    }
    const townmapbook::Node *node = nodeAt(event->pos());
    const QString hover = node ? node->location : QString();
    if (hover != m_hover) {
        m_hover = hover;
        QWidget::setCursor(node ? Qt::PointingHandCursor : Qt::OpenHandCursor);
        QWidget::update();
    }
}

void MapView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    m_dragStart = event->pos();
    m_panStart = m_pan;
    m_dragging = false;
    // 끌기 판정은 mouseMove에서 — 즉시 누르면 클릭으로 본다
    if (!nodeAt(event->pos()))
        m_dragging = true; // 빈 곳은 바로 끌기 시작
}

void MapView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const bool moved = (event->pos() - m_dragStart).manhattanLength() > 4;
    m_dragging = false;
    if (moved)
        return;
    const townmapbook::Node *node = nodeAt(event->pos());
    const QString location = node ? node->location : QString();
    if (location == m_selected)
        return;
    m_selected = location;
    QWidget::update();
    emit locationSelected(location);
}

void MapView::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    if (!m_hover.isEmpty()) {
        m_hover.clear();
        QWidget::update();
    }
}

void MapView::wheelEvent(QWheelEvent *event)
{
    if (!m_map)
        return;
    const int step = event->angleDelta().y() > 0 ? 1 : -1;
    const int next = std::clamp(scale() + step, 1, 12);
    if (next == scale())
        return;
    // 커서 아래 지점이 그대로 있도록 팬을 보정한다
    const QPointF pos = event->position();
    const QPointF before = (pos - origin()) / scale();
    m_zoom = next - fitScale();
    m_pan = clampPan((pos - before * next).toPoint());
    QWidget::update();
}

void MapView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_pan = m_map ? clampPan(m_pan) : QPoint();
}

void MapView::select(const QString &location)
{
    if (m_selected == location)
        return;
    m_selected = location;
    QWidget::update();
}
} // namespace com::yamada::studio
