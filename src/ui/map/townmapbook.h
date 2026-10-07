#pragma once

#include <QHash>
#include <QList>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>

// 타운맵 사전: resources/data/townmap/<지방>.json을 처음 부를 때 한 번 읽어 둔다(guidebook과
// 같은 방식). 노드 = 장소(location identifier)의 지도 픽셀 영역. 지도 이미지는 커밋하지 않는다 —
// imageUrl에서 첫 사용 때 받아 사용자 캐시에 둔다(SpriteCache::Kind::TownMap).
namespace com::yamada::studio::townmapbook {
struct Node
{
    QString location; // PokéAPI location identifier("goldenrod-city")
    QString region; // 비어 있으면 파일의 지방. 경계 노드만 따로("tohjo-falls" → kanto)
    QRect rect; // 원본 이미지 좌표(크롭 전)
};

struct RegionMap
{
    QString region; // "johto"
    QString imageUrl;
    QSize imageSize;
    QRect crop; // 보여 줄 영역. isNull이면 전체
    QList<Node> nodes;
    bool isValid() const { return !nodes.isEmpty(); }
    QRect view() const { return crop.isNull() ? QRect(QPoint(0, 0), imageSize) : crop; }
};

// 그 지방의 지도. 파일이 없으면 isValid() == false인 빈 맵
const RegionMap &regionMap(const QString &region);
QStringList regions(); // 지도가 준비된 지방들
} // namespace com::yamada::studio::townmapbook
