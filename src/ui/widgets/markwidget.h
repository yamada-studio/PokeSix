#pragma once

#include <QWidget>

class QSvgRenderer;

namespace com::yamada::studio {
// 캡슐 마크(디자인 01b §6). 핸드오프의 SVG 원본을 그대로 그린다(QSvgRenderer).
// 128px 이상은 전부 있는 기본 마크(광택 · 눈빛 · 입 · 볼 터치)를 쓴다.
// 작은 크기(32 이하 · 앱 막대 36의 스티커 변형)는 요소를 빼는 규칙이 달라서 A9에서 변형을 추가한다.
//
// 비율(80×50) · 기울기(−35°) · 색은 바꾸지 않는다. 포켓볼 도안(원 + 가로 띠 + 가운데 버튼)과
// 구별되게 하려는 디자인 결정이다(ADR 0009).
class MarkWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MarkWidget(QWidget *parent = nullptr);

    // 한 변의 길이(px). 인트로 136, 앱 막대 36(A9). 크기가 바뀌면 레이아웃에 다시 묻게 한다.
    void setMarkSize(int size);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QSvgRenderer *m_renderer = nullptr;
    int m_size = 136;
};
} // namespace com::yamada::studio
