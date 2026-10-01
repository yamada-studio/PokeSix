#pragma once

#include "data/repository/repository.h"

#include <QList>
#include <QRgb>
#include <QString>

class QFont;
class QPainter;
class QRectF;

// 변화 기술의 효과 한 줄: "공격 ▲2" · "상대 스피드 ▼1" · "상대 마비" · "HP ½ 회복".
//
// 뼈대는 DB(PokéAPI move_meta · move_meta_stat_changes — 지금 게임 값)이고, 세대 차이 · 예외는
// resources/data/move-effects.json이 덮는다(성장은 4세대까지 특공만 · 저주는 고스트면 다른 효과 …).
// 1세대는 특공 · 특방 대신 "특수" 하나(GenerationFeatures::specialSplit). 뼈대로 그릴 것이 없는
// 기술(리플렉터 · 날씨 · 대타출동 …)은 그 세대의 게임 설명문을 쓴다.
namespace com::yamada::studio::moveeffect {
struct Segment
{
    QString text;
    QRgb color;
};
using Parts = QList<Segment>;

// userTypes: 쓰는 포켓몬의 타입(저주처럼 쓰는 쪽 타입에 따라 효과가 다른 기술)
Parts describe(const MoveEntry &move, int generation, const QStringList &userTypes,
               Language language);
QString plainText(const Parts &parts);
// rect 안에 왼쪽부터 이어 그린다. 넘치면 말줄임(…). 잘렸으면 true
bool paint(QPainter &painter, const QRectF &rect, const Parts &parts, const QFont &font);
} // namespace com::yamada::studio::moveeffect
