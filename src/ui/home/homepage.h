#pragma once

#include "ui/shell/page.h"

#include <QWidget>

namespace com::yamada::studio {
class IntroMenu;

// 인트로(타이틀) 화면. 디자인 v2의 IntroPage에 해당한다(ADR 0008 · 0009).
// 앱 막대 없이 창 전체를 쓰고, 위에서부터 마크 → 워드마크 → 부제 → 세대 블록 → 메뉴 창,
// 맨 아래에 정보 줄을 둔다. 바탕(사선 무늬 + 위아래 빨강 띠)은 이 위젯이 직접 그린다.
//
// 메뉴에서 무엇을 골랐는지는 시그널로만 알린다. 화면을 실제로 바꾸는 일은 MainWindow가 한다
// (HomePage는 자기가 어디에 들어 있는지 모른다).
class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(QWidget *parent = nullptr);

signals:
    void openRequested(com::yamada::studio::Page page); // 메뉴 1–4
    void quitRequested();                               // 메뉴 "종료"

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    IntroMenu *m_menu = nullptr;
};
} // namespace com::yamada::studio
