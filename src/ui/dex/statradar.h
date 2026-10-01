#pragma once

#include <QWidget>

#include <array>

namespace com::yamada::studio {
// 종족값 레이더 차트 (도감 상세): 여섯 능력치를 육각형 축에 찍고 이은 도형.
//
//            HP
//     특공 ╱    ╲ 공격       축 순서 = 게임 요약 화면(위에서부터 시계방향):
//         │  ◆  │            HP · 공격 · 방어 · 스피드 · 특방 · 특공
//     특방 ╲    ╱ 방어
//          스피드
//
// 눈금은 50 · 100 · 150 · 200 육각형 넷. 최대치를 200으로 고정해 포켓몬끼리 모양을 견줄 수 있게
// 하고, 200을 넘는 값(단단지 방어 230 등)은 끝에 붙인다. 축 이름 옆에 값을 적는다.
class StatRadar : public QWidget
{
    Q_OBJECT
public:
    explicit StatRadar(QWidget *parent = nullptr);

    // stats: HP · 공격 · 방어 · 특공 · 특방 · 스피드(Repository 순서). 1세대는 특공 = 특방 = 특수.
    void setStats(const std::array<int, 6> &stats);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::array<int, 6> m_stats {};
};
} // namespace com::yamada::studio
