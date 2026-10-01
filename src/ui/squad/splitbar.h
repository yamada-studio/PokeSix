#pragma once

#include "core/analysis/squadanalyzer.h"

#include <QWidget>

namespace com::yamada::studio {
// 물리 · 특수 분포(02 SCR-04 ③): 누적 막대(물리 주황 · 특수 파랑 · 변화 회색 · 빈칸 사선) + 범례 +
// 반대 규칙이었다면의 값 한 줄. 기준 칸 수 = 채운 포켓몬 × 4.
class SplitBar : public QWidget
{
    Q_OBJECT
public:
    explicit SplitBar(QWidget *parent = nullptr);

    // otherRuleText: "1–3세대 규칙(타입 기준)이었다면" 같은 앞말(값은 이 위젯이 붙인다)
    void setSplit(const MoveSplit &split, const MoveSplit &other, const QString &otherRuleText);

    QSize sizeHint() const override { return {400, 58}; }
    QSize minimumSizeHint() const override { return {200, 58}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    MoveSplit m_split;
    MoveSplit m_other;
    QString m_otherRuleText;
};
} // namespace com::yamada::studio
