#pragma once

#include "core/analysis/squadanalyzer.h"

#include <QWidget>

namespace com::yamada::studio {
// 물리 · 특수 분포(02 SCR-04 ③): 누적 막대(물리 주황 · 특수 파랑 · 변화 회색 · 빈칸 사선) + 범례.
// 분류는 그 세대 규칙으로 이미 판정된 값이다. 기준 칸 수 = 채운 포켓몬 × 4.
class SplitBar : public QWidget
{
    Q_OBJECT
public:
    explicit SplitBar(QWidget *parent = nullptr);

    void setSplit(const MoveSplit &split);

    QSize sizeHint() const override { return {400, 58}; }
    QSize minimumSizeHint() const override { return {200, 58}; }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    MoveSplit m_split;
};
} // namespace com::yamada::studio
