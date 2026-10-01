#pragma once

#include "data/text/localizedtext.h"

#include <QAbstractButton>
#include <QColor>
#include <QList>
#include <QStringList>

namespace com::yamada::studio {
// 게임 버전 칩: [DP] [Pt] [HGSS]. 칩 하나 안에서 바탕을 버전 수만큼 나눠 각 버전 색으로 칠하고,
// 약칭도 버전마다 그 색으로 쓴다(dexstyle.json). 켠 칩은 먹선 · 그림자로 떠 보이고, 꺼진 칩은
// 흐리다. 스쿼드의 포켓몬 선택 창(도감 칩) · 도감 상세(기준 게임 칩)가 같이 쓴다.
class VersionChip : public QAbstractButton
{
public:
    struct Part
    {
        QString text;
        QColor background;
        QColor color;
    };

    // suffix: 약칭이 겹치는 칩을 가르는 뒷말(흰 바탕 · 작은 글자). 비어 있으면 없음
    VersionChip(const QList<Part> &parts, const QString &suffix, QWidget *parent = nullptr);

    // 버전 identifier · 이름 → 조각(같은 약칭은 한 번: 관동도감의 일본판 레드 = 레드 = R)
    static QList<Part> partsFor(const QStringList &versions, const QList<LocalizedText> &names);
    static QString keyOf(const QList<Part> &parts); // 약칭을 이어 붙인 글자("HGSS") — 겹침 판정용

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<Part> m_parts;
    QString m_suffix;
};
} // namespace com::yamada::studio
