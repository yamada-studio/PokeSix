#pragma once

#include <QWidget>

// 끌어다 놓기 안내 덮개 (H2-CP8). 가이드: docs/guides/h2-squad-sav-import.md
//
// 파일을 끌고 스쿼드 화면 위로 들어오면 페이지 전체를 덮어서 "여기에 놓으면 된다"는 걸 보여 준다:
//
//   ┌───────────────────────────────────────────────┐
//   │░░░░░░░░░░░ 종이색 반투명 막(뒤가 흐리게 비침) ░░│
//   │░░ ┌ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┐ ░░░░░│
//   │░░ │      여기에 놓으면 불러와요            │ ░░░░│   ← 도현 28, 먹색
//   │░░ │  스쿼드 파일(.pks) · 게임 세이브(.sav)  │ ░░░░│   ← 본문 14, text.2
//   │░░ └ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ┘ ░░░░░│   ← 파랑 점선 3px, 반경 12, 안쪽 여백 16
//   └───────────────────────────────────────────────┘
//
// 그리기만 한다 — 언제 보이고 숨을지는 SquadPage(dragEnter · dragLeave · drop)가 정한다.
// 마우스 · 끌어다 놓기 이벤트를 가로채지 않는다(아래로 통과): 덮개가 이벤트를 먹으면 SquadPage의
// dragLeaveEvent가 불려 덮개가 깜빡인다.
namespace com::yamada::studio {
class DropOverlay : public QWidget
{
    Q_OBJECT // 시그널은 없지만 tr()의 번역 문맥(클래스 이름)에 필요하다
            public : explicit DropOverlay(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
} // namespace com::yamada::studio
