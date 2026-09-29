#pragma once

#include "ui/shell/page.h"

#include <QWidget>

class QButtonGroup;

namespace com::yamada::studio {
// 앱 막대의 폴더 탭 4개: 도감 · 아이템 · 스쿼드 · 설정 (디자인 v2 02b SCR-00, 01 §5-1).
//   비활성: 높이 38 · red.deep 바탕 · 흰 글자, 막대 아래 먹선 위에 선다
//   활성:   높이 45 · paper 바탕 · 빨강 글자 · 먹선 3(아래 없음) — 막대 아래 먹선을 덮어 본문과
//   이어진 폴더처럼 보인다
// 한 번에 하나만 켜진다(QButtonGroup exclusive). 사용자가 탭을 누르면 pageSelected()를 보낸다.
class AppTabBar : public QWidget
{
    Q_OBJECT
public:
    explicit AppTabBar(QWidget *parent = nullptr);

    // 코드에서 탭을 바꾼다(메뉴 · 단축키로 페이지가 바뀌었을 때). pageSelected()는 보내지 않는다.
    void setCurrentPage(Page page);

signals:
    void pageSelected(com::yamada::studio::Page page);

private:
    QButtonGroup *m_group = nullptr;
};
} // namespace com::yamada::studio
