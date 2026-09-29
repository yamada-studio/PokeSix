#pragma once

#include <QAbstractButton>
#include <QFont>
#include <QPixmap>

namespace com::yamada::studio {
// 앱 막대 왼쪽의 "캡슐 스티커 마크 36 + POKESIX 워드마크 20" 버튼 (디자인 v2 02b SCR-00).
// 누르면 인트로(처음 화면)로 돌아간다. 툴팁 "처음 화면으로".
// hover · 포커스 때 연노랑 2px 점선 테(간격 2)를 두른다 — 그 테가 들어갈 자리까지 위젯 크기에
// 넣는다.
class MarkButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit MarkButton(QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override; // hover 테를 그리려고 다시 그리기를 부탁한다
    void leaveEvent(QEvent *event) override;
    // 점선 테는 키보드(Tab)로 들어왔을 때만 보인다. 마우스 클릭 · 화면 전환으로 포커스가 온 경우는
    // 제외
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    QPixmap m_mark;
    bool m_keyboardFocus = false;
    QFont m_font;
};
} // namespace com::yamada::studio
