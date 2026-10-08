#pragma once

#include "ui/shell/page.h"

#include <QMainWindow>
#include <QSize>

class QStackedWidget;

namespace com::yamada::studio {
class AppBar;
class AppState;
class DataUpdater;
class DexPage;
class Repository;

// 메인 창 = [인트로 │ 본 화면(앱 막대 + 페이지)]. 화면 전환은 전부 여기서 한다.
// Repository · AppState · DataUpdater는 만들지 않고 받는다(생성자 주입). 만들고 잇는 곳은
// app 레이어의 Application이고, 셋 다 이 창보다 오래 산다.
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(Repository *repository, AppState *state, DataUpdater *dataUpdater,
               QWidget *parent = nullptr);

    // 본 화면으로 가서 page를 연다(탭 표시도 맞춘다). 인트로 메뉴 · 탭 · 단축키가 모두 여기로 온다.
    void open(Page page);
    // 인트로(처음 화면)로 돌아간다. 앱 막대의 마크 버튼.
    void showIntro();

private:
    enum Screen { IntroScreen = 0, MainScreen = 1 }; // m_screens의 페이지 순서

    QStackedWidget *m_screens = nullptr; // [인트로 │ 본 화면]
    QStackedWidget *m_pages = nullptr;   // 본 화면의 페이지. 순서 = Page
    AppBar *m_appBar = nullptr;
    AppState *m_state = nullptr;
    DexPage *m_dexPage = nullptr; // 도감 탭(목록 ↔ 상세) // object tree로 소유(this가 부모)
    Repository *m_repository = nullptr; // 소유하지 않는다(Application이 소유)

    // 처음 여는 크기: 도감 표(840) + 창 테두리 · 여백이 딱 들어가는 폭, 인트로(최소 높이 804)가 다
    // 들어가는 높이. 최소 크기는 따로 정하지 않는다 — 레이아웃이 계산한 minimumSizeHint가 최소다.
    // 1440×900: 도감 · 아이템의 세 칸(필터 | 목록 | 상세)과 홈 부채꼴이 들어가는 폭(ADR 0015)
    static constexpr QSize kDefaultSize {1440, 900};
};
} // namespace com::yamada::studio
