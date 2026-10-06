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
        QString version; // 이 조각의 버전 identifier(같은 약칭이 여럿이면 첫 버전)
    };

    // suffix: 약칭이 겹치는 칩을 가르는 뒷말(흰 바탕 · 작은 글자). 비어 있으면 없음
    VersionChip(const QList<Part> &parts, const QString &suffix, QWidget *parent = nullptr);

    // 버전 identifier · 이름 → 조각(같은 약칭은 한 번: 관동도감의 일본판 레드 = 레드 = R)
    static QList<Part> partsFor(const QStringList &versions, const QList<LocalizedText> &names);
    static QString keyOf(const QList<Part> &parts); // 약칭을 이어 붙인 글자("HGSS") — 겹침 판정용

    QSize sizeHint() const override;

    // 버전 고르기(게임 칩): 켠 칩 안에서 이 조각만 버전 색, 나머지 조각은 흐리게. −1 = 모두 같게
    void setSelectedPart(int part);
    int selectedPart() const { return m_selectedPart; }
    int partAt(const QPointF &pos) const; // 그 자리의 조각(약칭 바깥 = −1)
    int pressedPart() const { return m_pressedPart; } // 마지막으로 마우스로 누른 조각(키보드 = −1)
    int partCount() const { return int(m_parts.size()); }
    const Part &part(int index) const { return m_parts.at(index); }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override; // 흐린 조각 위 → 살짝 칠해 누를 수 있음을
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QList<qreal> boundaries() const; // 조각마다 오른쪽 끝 x(마지막은 색칠한 자리 끝)
    bool dimmedPart(qsizetype index) const // 켠 칩에서 고르지 않은 버전 조각
    {
        return isChecked() && m_selectedPart >= 0 && m_parts.size() > 1 && index != m_selectedPart;
    }

    QList<Part> m_parts;
    QString m_suffix;
    int m_selectedPart = -1;
    int m_pressedPart = -1;
    int m_hoverPart = -1;
};
} // namespace com::yamada::studio
