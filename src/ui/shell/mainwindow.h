#pragma once

#include "ui/shell/page.h"

#include <QMainWindow>

#include <memory>

class QStackedWidget;

namespace com::yamada::studio {
class AppBar;
class Repository;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override; // Repository가 여기서는 불완전 타입이라 소멸자를 .cpp에 둔다

    // 본 화면으로 가서 page를 연다(탭 표시도 맞춘다). 인트로 메뉴 · 탭 · 단축키가 모두 여기로 온다.
    void open(Page page);
    // 인트로(처음 화면)로 돌아간다. 앱 막대의 마크 버튼.
    void showIntro();

private:
    enum Screen { IntroScreen = 0, MainScreen = 1 }; // m_screens의 페이지 순서

    QStackedWidget *m_screens = nullptr; // [인트로 │ 본 화면]
    QStackedWidget *m_pages = nullptr;   // 본 화면의 페이지. 순서 = Page
    AppBar *m_appBar = nullptr;
    // 게임 데이터 DB 조회 창구. QObject가 아니라서 object tree 대신 unique_ptr로 소유한다.
    // 화면들(DexPage …)은 포인터만 받아 쓴다(생성자 주입).
    std::unique_ptr<Repository> m_repository;
};
} // namespace com::yamada::studio
