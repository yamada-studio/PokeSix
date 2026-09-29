#pragma once

#include <QWidget>

namespace com::yamada::studio {
// 10칸 세그먼트 진행 막대 (디자인 시트 §5-8 "진행 막대는 10칸 세그먼트가 하나씩 초록으로 찬다").
// 먹선 2 · 반경 4 · 안쪽 여백 3 · 칸 사이 3 · 칸 높이 12. 찬 칸 = stat.good, 빈 칸 = line.soft.
// 연속 애니메이션 없이 칸 단위로만 바뀐다(디자인 원칙: 블러 · 연속 애니메이션 금지).
class SegmentProgress : public QWidget
{
    Q_OBJECT
public:
    explicit SegmentProgress(QWidget *parent = nullptr);

    void setValue(int percent); // 0–100. 10%마다 한 칸
    int value() const { return m_percent; }

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_percent = 0;
};
} // namespace com::yamada::studio
