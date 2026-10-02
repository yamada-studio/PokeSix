#pragma once

#include <QWidget>

namespace com::yamada::studio {
// 두 손잡이 범위 슬라이더 (도감 필터의 "종족값 합계", 디자인 02 SCR-02 필터 창).
// Qt에는 손잡이 둘짜리가 없어서 직접 그린다: 트랙(line 색) 위에 고른 구간을 노랑으로 칠하고,
// 양 끝에 네모 손잡이(흰 바탕 · 먹선 · 그림자 1)를 둔다. 값은 step 단위로 끊는다.
class RangeSlider : public QWidget
{
    Q_OBJECT
public:
    explicit RangeSlider(int minimum, int maximum, int step, QWidget *parent = nullptr);

    int lowerValue() const { return m_lower; }
    int upperValue() const { return m_upper; }
    void setValues(int lower, int upper); // 끊고(step) 겹치지 않게 맞춘 뒤 valuesChanged

    QSize sizeHint() const override;

signals:
    void valuesChanged(int lower, int upper); // 사용자 드래그 · setValues 양쪽 다

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    enum class Handle { None, Lower, Upper };

    QRectF handleRect(int value) const;
    int valueAt(qreal x) const; // 트랙 위 x → step으로 끊은 값
    qreal xOf(int value) const;

    int m_minimum;
    int m_maximum;
    int m_step;
    int m_lower;
    int m_upper;
    Handle m_dragging = Handle::None;
};
} // namespace com::yamada::studio
