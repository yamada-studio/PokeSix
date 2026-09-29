#pragma once

#include <QWidget>

#include <vector>

namespace com::yamada::studio {
class IntroMenuItem;

// 인트로 메뉴 창의 "내용": 메뉴 줄 4개 + 점선 구분선 + 종료 줄 (디자인 02b SCR-01 #5).
// 자기가 PanelFrame 안에 들어 있다는 사실은 모른다(ADR 0007: 겉모양과 내용의 분리).
//
// 선택(▶ + 노란 칸)은 이 클래스가 인덱스 하나로 관리한다.
//   - 마우스: 줄에 올라가면(hover) 선택이 그 줄로 옮겨 간다
//   - 키보드: ↑↓ 이동(처음 · 끝에서 멈춤, 순환 없음), Enter 실행, 1–4 바로 실행,
//             Esc = 종료 줄로 이동, 종료 줄에서 한 번 더 Esc = 실행
// 실행은 activated(index) 시그널로 밖에 알린다. 무엇을 할지는 HomePage가 정한다.
class IntroMenu : public QWidget
{
    Q_OBJECT
public:
    static constexpr int kQuitIndex = 4; // 0–3 = 도감 · 아이템 · SixSquad · 설정, 4 = 종료

    explicit IntroMenu(QWidget *parent = nullptr);

    int currentIndex() const { return m_current; }
    void setCurrentIndex(int index); // 잠긴(비활성) 줄은 고를 수 없다 — 부르면 무시한다

    // 데이터가 없으면 도감 · 아이템 · SixSquad를 잠근다(설정 · 종료는 데이터 없이도 쓸 수 있다).
    // 선택이 잠긴 줄에 있었으면 쓸 수 있는 첫 줄로 옮긴다. 잠금이 풀리면 선택은 도감으로 간다.
    void setDataLocked(bool locked);

signals:
    // 사용자가 줄을 실행했다(클릭, Enter, 1–4, 종료 줄에서 Esc). index는 kQuitIndex 포함.
    void activated(int index);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    // from에서 direction(+1 아래 / −1 위) 쪽으로 가며 처음 만나는 쓸 수 있는 줄. 없으면 from.
    int nextEnabled(int from, int direction) const;

    std::vector<IntroMenuItem *> m_items; // 소유는 object tree(부모 = this)가 한다. 여기는 참조만.
    int m_current = 0;
};
} // namespace com::yamada::studio
