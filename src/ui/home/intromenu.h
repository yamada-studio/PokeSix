#pragma once

#include <QWidget>

#include <vector>

namespace com::yamada::studio {
class IntroMenuItem;

// 인트로 메뉴: 카드 버튼 3개(스쿼드는 빨강 주 행동) (ADR 0015, 사진 레퍼런스).
// 버튼마다 자기 카드(먹선 · 그림자 · 배지 아이콘)를 그린다 — 바깥 PanelFrame은 더 쓰지 않는다.
// 종료 줄과 키 안내는 뺐다(사용자 결정) — 종료는 창 닫기 버튼으로.
//
// 선택(▶ + 노란 칸)은 이 클래스가 인덱스 하나로 관리한다.
//   - 마우스: 줄에 올라가면(hover) 선택이 그 줄로 옮겨 간다
//   - 키보드: ↑↓ 이동(처음 · 끝에서 멈춤, 순환 없음), Enter 실행, 1–3 바로 실행
// 실행은 activated(index) 시그널로 밖에 알린다. 무엇을 할지는 HomePage가 정한다.
//
// 포커스 링(ADR 0016): Tab · Shift+Tab으로 메뉴에 포커스가 들어오면 선택된 카드 **바깥**에 파란
// 링(3px, 간격 2px)을 그린다. 카드(IntroMenuItem)는 자기 상자 밖을 그리지 않으므로(ADR 0007 §5)
// 부모인 이 위젯이 paintEvent에서 그린다. 그 자리는 레이아웃 여백(kFocusMargin)으로 비워 둔다.
// 마우스 클릭 · 프로그램(setFocus(OtherFocusReason))으로 들어온 포커스에는 링을 그리지 않는다 —
// 선택 칸(노란 테)이 이미 보이고, 키보드 사용자에게만 "지금 키가 여기로 간다"를 알리면 된다.
class IntroMenu : public QWidget
{
    Q_OBJECT
public:
    explicit IntroMenu(QWidget *parent = nullptr);

    // 카드 둘레에 비워 두는 여백 = 링 3 + 간격 2. HomePage가 메뉴 폭을 정할 때 더한다.
    static constexpr int kFocusMargin = 5;

    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index); // 잠긴(비활성) 줄은 고를 수 없다 — 부르면 무시한다

    // 데이터가 없으면 도감 · 아이템 · 스쿼드를 잠근다(설정 · 종료는 데이터 없이도 쓸 수 있다).
    // 선택이 잠긴 줄에 있었으면 쓸 수 있는 첫 줄로 옮긴다. 잠금이 풀리면 선택은 도감으로 간다.
    void setDataLocked(bool locked);

signals:
    // 사용자가 줄을 실행했다(클릭, Enter, 1–3).
    void activated(int index);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;  // Tab으로 들어왔는지(reason)를 기억한다
    void focusOutEvent(QFocusEvent *event) override; // 링을 지운다
    void paintEvent(QPaintEvent *event) override;    // 선택된 카드 바깥의 포커스 링

private:
    // from에서 direction(+1 아래 / −1 위) 쪽으로 가며 처음 만나는 쓸 수 있는 줄. 없으면 from.
    int nextEnabled(int from, int direction) const;

    std::vector<IntroMenuItem *> m_items; // 소유는 object tree(부모 = this)가 한다. 여기는 참조만.
    int m_current = 0;
    bool m_keyboardFocus = false; // 포커스가 Tab · Shift+Tab으로 들어왔다 → 링을 그린다
};
} // namespace com::yamada::studio
