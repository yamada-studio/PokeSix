#pragma once

#include "data/text/localizedtext.h"
#include "ui/map/townmapbook.h"

#include <QHash>
#include <QWidget>

namespace com::yamada::studio {
class SpriteCache;

// 타운맵 뷰어(T1). 그림은 원작(인게임 타운맵 — 첫 사용 때 받아 캐시), 조작은 우리 것:
// 호버 = 장소 이름 말풍선, 클릭 = 선택(locationSelected), 휠 = 확대 · 축소, 끌기 = 이동.
// 도트가 뭉개지지 않게 정수 배율 니어리스트로만 그린다. 이미지가 아직 없으면(받는 중 ·
// 오프라인) 노드만으로 그리는 자작 스키매틱으로 대신한다.
class MapView : public QWidget
{
    Q_OBJECT
public:
    explicit MapView(QWidget *parent = nullptr);

    // layerLabels: 레이어(이어 붙인 지도)마다 모서리에 쓸 지방 이름("성도" · "관동").
    // 레이어가 하나뿐이면 그리지 않는다
    void setRegion(const QString &region, const QHash<QString, LocalizedText> &names,
                   Language language, const QStringList &layerLabels = {});
    void select(const QString &location); // 빈 문자열 = 선택 해제
    QString selected() const { return m_selected; }

signals:
    void locationSelected(const QString &location); // 빈 문자열 = 빈 곳을 눌러 해제

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int fitScale() const;  // 창에 꼭 맞는 정수 배율(최소 1)
    int scale() const;     // 실제 배율 = fit + m_zoom (1 이상)
    QPoint origin() const; // 지도(크롭) 왼쪽 위의 위젯 좌표(가운데 정렬 + 팬)
    QPoint clampPan(QPoint pan) const;
    const townmapbook::Node *nodeAt(const QPoint &widgetPos) const;
    QRect nodeRect(const townmapbook::Node &node) const; // 위젯 좌표
    void ensurePixmap(); // 배율이 바뀌면 니어리스트 확대본을 다시 만든다

    const townmapbook::RegionMap *m_map = nullptr;
    QHash<QString, LocalizedText> m_names;
    Language m_language = Language::Korean;
    SpriteCache *m_cache = nullptr;

    QPixmap m_scaled; // 지금 배율의 확대본(크롭 반영)
    int m_scaledFor = 0;
    int m_zoom = 0; // fitScale()에 더하는 단계(휠)
    QPoint m_pan;
    QPoint m_dragStart;
    QPoint m_panStart;
    bool m_dragging = false;
    QString m_hover;
    QString m_selected;
    QStringList m_layerLabels;
};
} // namespace com::yamada::studio
