#pragma once

#include <QObject>
#include <QTimer>

class QAbstractItemView;

namespace com::yamada::studio {
// 목록(QTableView 등)의 "마우스가 올라간 줄"을 기억하고(줄 위에서는 가리키기 장갑), 줄을 누르면
// 장갑 커서로 꾹꾹 쥐는 애니메이션을 한다. 도감 · 아이템 목록 · 스쿼드 선택 창이 같이 쓴다.
//
// 표 뷰는 칸 하나에만 hover 상태(State_MouseOver)를 주고 줄 전체는 모른다 → 뷰의 viewport에서
// 마우스 움직임을 엿보고(이벤트 필터) 줄 번호를 들고 있다가, delegate가 row()로 물어 줄 전체를
// 칠한다. ROS 2로 치면 마우스 위치를 구독해 "지금 강조할 줄"을 다시 내보내는 작은 노드.
class RowHover : public QObject
{
    Q_OBJECT
public:
    explicit RowHover(QAbstractItemView *view);

    int row() const { return m_row; } // 없으면 −1

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setRow(int row);
    void nextFrame();

    QAbstractItemView *m_view = nullptr;
    int m_row = -1;
    QTimer m_animation;
    int m_frame = 0;
};
} // namespace com::yamada::studio
