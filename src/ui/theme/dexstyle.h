#pragma once

#include "data/text/localizedtext.h"

#include <QColor>
#include <QList>
#include <QString>

// 도감 선택 버튼의 표시 규칙: 버전 배지(약칭 · 색)와 도감 이름 바꾸기 · 숨김.
//
// 값은 코드가 아니라 설정 파일 resources/theme/dexstyle.json(qrc ":/theme/dexstyle.json")에 있다.
// 버전이 40개 가까이 되고 세대를 늘릴 때마다 더해야 해서, 색 토큰(tokens.h)처럼 코드에 두면 고칠
// 때마다 다시 컴파일해야 한다. 처음 부를 때 한 번 읽어 기억해 둔다.
//
// data 계층(Repository)은 PokéAPI의 identifier만 넘기고, 어떻게 보일지는 여기(ui/theme)가 정한다.
namespace com::yamada::studio::dexstyle {
struct VersionStyle
{
    QString shortName; // "D"
    QColor background; // 배지 바탕
    QColor text;       // 배지 글자
};

// identifier("diamond")의 배지. 표에 없으면 약칭 = fallbackName, 흰 바탕 · 먹 글자.
VersionStyle version(const QString &identifier, const QString &fallbackName);

struct DexStyle
{
    LocalizedText label; // 비어 있으면 지방 이름을 쓴다. JSON에서는 {ko, en, ja} 또는 글자 하나(ko)
    bool hidden = false; // 버튼을 만들지 않는다(알로라 섬 도감 등)
};

// identifier("kalos-central")의 표시 규칙. 표에 없으면 기본값.
DexStyle dex(const QString &identifier);

// 게임 묶음(version_groups)의 짧은 이름(DLC: 외딴섬 · 설원 …). 없으면 빈 칸
LocalizedText groupLabel(const QString &versionGroup);

// 세대 버튼의 무지개 바탕: 그 세대 시리즈의 배지 바탕색(발매 순). 모르는 세대면 빈 목록
QList<QColor> generationColors(int generation);
} // namespace com::yamada::studio::dexstyle
