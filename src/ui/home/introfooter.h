#pragma once

#include <QWidget>

namespace com::yamada::studio {
// 인트로 맨 아래 정보 줄 (디자인 02b SCR-01 #6):
//   v0.0.1            [↑↓] 이동  [Enter] 선택  [1–4] 바로 가기            데이터: PokéAPI · 비공식
//   팬 도구
// 그리기 · 동작 없이 QLabel 조합만으로 된다. 모양은 app.qss의 QLabel#intro… 규칙이 정한다.
// 클래스로 둔 이유: 좁은 창(1024)에서 "1–4 바로 가기"를 숨기는 반응형 동작이 Phase F에서 붙는다.
class IntroFooter : public QWidget
{
    Q_OBJECT
public:
    explicit IntroFooter(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
