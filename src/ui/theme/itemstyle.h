#pragma once

#include "data/models/itemfilterproxy.h"

#include <QColor>
#include <QList>
#include <QString>

// 아이템 대백과의 분류 묶음(회복 · 볼 · 기술머신 · 진화 · 배틀 · 나무열매 · 기타)과 숨길 분류.
//
// 값은 설정 파일 resources/theme/itemstyle.json(qrc ":/theme/itemstyle.json")에 있다. PokéAPI
// 분류는 55가지라서, 화면의 묶음으로 모으는 규칙을 코드 대신 파일에 둔다(dexstyle.h · ADR 0012와
// 같은 방식). 처음 부를 때 한 번 읽어 기억해 둔다.
//
// 쓰는 법 (ItemsPage):
//   for (const itemstyle::Group &g : itemstyle::groups()) … 분류 창에 한 줄씩(g.label, g.color)
//   proxy->setCategoryFilter(itemstyle::filterFor(g.key));  // 그 묶음 + 숨김 규칙으로 거르기
namespace com::yamada::studio::itemstyle {
inline constexpr char kAll[] = "all"; // 첫 묶음 "전체"의 key

struct Group
{
    QString key;   // "evolution"
    QString label; // "진화"
    QColor color;  // 분류 창의 색 견본
};

// 분류 창에 나올 묶음, 위에서부터. 첫째는 "전체"(kAll).
const QList<Group> &groups();

// 아이템 하나가 드는 묶음의 key. 숨길 분류면 빈 문자열.
QString groupOf(const QString &category, const QString &pocket);

// 프록시에 넘길 필터: groupKey 묶음만 통과(kAll이면 숨길 분류만 빼고 전부).
ItemFilterProxy::CategoryFilter filterFor(const QString &groupKey);
} // namespace com::yamada::studio::itemstyle
