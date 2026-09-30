#pragma once

#include "ui/shell/page.h"

#include <QWidget>

class QLabel;
class QSpacerItem;

namespace com::yamada::studio {
class DataUpdater;
class FirstRunPanel;
class GenerationButton;
class IntroMenu;
class MarkWidget;

// 인트로(타이틀) 화면. 디자인 v2의 IntroPage에 해당한다(ADR 0008 · 0009).
// 앱 막대 없이 창 전체를 쓰고, 위에서부터 마크 → 워드마크 → 부제 → 세대 블록 → 메뉴 창,
// 맨 아래에 정보 줄을 둔다. 바탕(사선 무늬 + 위아래 빨강 띠)은 이 위젯이 직접 그린다.
//
// 메뉴에서 무엇을 골랐는지는 시그널로만 알린다. 화면을 실제로 바꾸는 일은 MainWindow가 한다
// (HomePage는 자기가 어디에 들어 있는지 모른다).
//
// 첫 실행(쓸 수 있는 게임 DB가 없음)이면 메뉴 창 위에 FirstRunPanel을 끼우고, 데이터가 필요한
// 메뉴를 잠근다. 받기 · 변환은 밖에서 넘겨받은 DataUpdater가 한다(생성자 주입 — 만들고 잇는 일은
// MainWindow가 한다). 데이터가 준비되면 패널을 없애고 잠금을 푼다.
class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(DataUpdater *updater, QWidget *parent = nullptr);

    GenerationButton *generationButton() const { return m_generationButton; }

signals:
    void openRequested(com::yamada::studio::Page page); // 메뉴 1–4
    void quitRequested();                               // 메뉴 "종료"

protected:
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void onDataReady();
    void setCompact(
            bool compact); // 첫 실행 패널이 끼어들면 세로가 모자라다 → 마크를 줄이고 부제를 숨긴다

    DataUpdater *m_updater = nullptr; // 소유하지 않는다(MainWindow가 소유)
    IntroMenu *m_menu = nullptr;
    GenerationButton *m_generationButton = nullptr;
    MarkWidget *m_mark = nullptr;
    QLabel *m_subtitle = nullptr;
    QSpacerItem *m_subtitleGap = nullptr; // 워드마크 ↔ 부제 간격 (compact에서 0)
    QSpacerItem *m_generationGap = nullptr; // 부제 ↔ 세대 블록 간격 (compact에서 줄인다)
    QWidget *m_firstRunBlock = nullptr; // 패널 + 그 아래 간격. 데이터가 준비되면 통째로 지운다
    FirstRunPanel *m_firstRun = nullptr;
};
} // namespace com::yamada::studio
