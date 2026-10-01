#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
// 타입 상성표 (도감 상세): 받는 피해(방어)와 주는 피해(공격, 자속 타입 기준)를 배율별 줄로.
//   받을 때 ×4 [불꽃] [비행] …    ← 이 포켓몬의 타입 조합이 맞을 때의 배율(타입 둘이면 곱)
//   줄 때   ×2 [물] [땅] …        ← 이 포켓몬의 타입(자속)으로 칠 때 가장 센 배율
// 그 세대의 상성표(TypeChart)로 계산한다 — 4세대 고스트 → 강철 ½, 6세대부터 1배.
// 칩이 줄 폭을 넘으면 다음 줄로 넘긴다(heightForWidth).
class MatchupView : public QWidget
{
    Q_OBJECT
public:
    explicit MatchupView(QWidget *parent = nullptr);

    void setMatchups(const QStringList &types, const TypeChart &chart, Language language);

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Row
    {
        bool defense = true; // 받는 피해 / 주는 피해
        double multiplier = 1;
        QStringList types; // 그 배율인 상대 타입
    };
    int layout(int width, QPainter *painter) const; // painter가 있으면 그리고, 높이를 돌려준다

    QList<Row> m_rows;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
