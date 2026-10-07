#pragma once

#include "data/text/localizedtext.h"

#include <QHash>
#include <QList>
#include <QPoint>
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
    QString region;   // 장소가 속한 지방("johto") — 이름 찾기용
    QRect rect;       // 캔버스 좌표
};

// 지도 그림 한 장(합본은 여러 장을 이어 붙인다 — 성도 + 관동)
struct Layer
{
    QString source; // 받을 그림의 key("johto" → pokearth/johto.png)
    QString url;
    QSize size;    // 원본 크기
    QRect crop;    // 원본에서 쓸 영역
    QPoint offset; // 캔버스에서의 자리
};

struct RegionMap
{
    QString region; // "johto" · "johto-kanto"(합본)
    QSize canvas;   // 전체 캔버스 크기
    QList<Layer> layers;
    QList<Node> nodes; // 캔버스 좌표
    bool isValid() const { return !nodes.isEmpty(); }
};

// 그 지방의 지도. 파일이 없으면 isValid() == false인 빈 맵
const RegionMap &regionMap(const QString &region);
QStringList regions(); // 지도가 준비된 지방들

// [랜드마크] 탭: 장소의 시설 · 이벤트(townmap/landmarks/<게임 묶음>.json). 없으면 빈 목록
struct Landmark
{
    LocalizedText name;
    LocalizedText detail; // 비어도 된다
};
QList<Landmark> landmarks(const QString &versionGroup, const QString &location);
} // namespace com::yamada::studio::townmapbook
