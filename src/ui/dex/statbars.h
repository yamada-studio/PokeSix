#pragma once

#include <QFont>
#include <QWidget>

#include <array>

namespace com::yamada::studio {
// 종족값 막대 여섯 줄 + 합계 (도감 상세, 디자인 01 §5 StatBar의 간단판).
//   [HP   45 ████░░░░░░]  막대 길이 = 값 / 255, 색 = 구간(~59 stat.low · ~89 stat.mid · ~119
//   stat.good · 120~ stat.high). 1세대는 특공 · 특방 칸에 같은 "특수" 값이 들어온다(Repository).
class StatBars : public QWidget
{
    Q_OBJECT
public:
    explicit StatBars(QWidget *parent = nullptr);

    void setStats(const std::array<int, 6> &stats, int total);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::array<int, 6> m_stats {};
    int m_total = 0;
    QFont m_labelFont;
    QFont m_valueFont;
};
} // namespace com::yamada::studio
