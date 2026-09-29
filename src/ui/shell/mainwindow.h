#pragma once

#include "ui/shell/page.h"

#include <QMainWindow>

class QStackedWidget;

namespace com::yamada::studio {
class AppBar;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

    // 본 화면으로 가서 page를 연다(탭 표시도 맞춘다). 인트로 메뉴 · 탭 · 단축키가 모두 여기로 온다.
    void open(Page page);
    // 인트로(처음 화면)로 돌아간다. 앱 막대의 마크 버튼.
    void showIntro();

private:
    enum Screen { IntroScreen = 0, MainScreen = 1 }; // m_screens의 페이지 순서

    QStackedWidget *m_screens = nullptr; // [인트로 │ 본 화면]
    QStackedWidget *m_pages = nullptr;   // 본 화면의 페이지. 순서 = Page
    AppBar *m_appBar = nullptr;
};
} // namespace com::yamada::studio
