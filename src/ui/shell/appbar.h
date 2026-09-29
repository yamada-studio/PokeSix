#pragma once

#include "ui/shell/page.h"

#include <QWidget>

namespace com::yamada::studio {
class AppTabBar;
class GenerationButton;
class MarkButton;
class SearchField;

// 본 화면 맨 위의 빨강 앱 막대 (디자인 v2 02b SCR-00): 높이 60 · 아래 먹선 3.
//   [마크 + POKESIX] [도감 · 아이템 · 스쿼드 · 설정 탭]            [GEN 4 신오 ▾] [검색 ⌕ … Ctrl K]
// 막대는 무엇이 눌렸는지만 알린다(homeRequested · pageSelected). 화면을 바꾸는 일은 MainWindow가
// 한다.
class AppBar : public QWidget
{
    Q_OBJECT
public:
    explicit AppBar(QWidget *parent = nullptr);

    void setCurrentPage(Page page); // 활성 탭 표시만 바꾼다
    SearchField *searchField() const { return m_search; }
    GenerationButton *generationButton() const { return m_generation; }

signals:
    void homeRequested();                              // 마크 버튼 → 인트로
    void pageSelected(com::yamada::studio::Page page); // 탭

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    MarkButton *m_mark = nullptr;
    AppTabBar *m_tabs = nullptr;
    GenerationButton *m_generation = nullptr;
    SearchField *m_search = nullptr;
};
} // namespace com::yamada::studio
