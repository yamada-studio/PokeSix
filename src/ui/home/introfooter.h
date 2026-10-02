#pragma once

#include <QWidget>

namespace com::yamada::studio {
// 인트로 맨 아래 정보 줄:  v0.0.1 ········· 데이터: PokéAPI · 비공식 팬 도구
// 키 안내 묶음(↑↓ · ←→ · Enter · 1–3)은 뺐다(사용자 결정) — 단축키 자체는 그대로 동작한다.
// 그리기 · 동작 없이 QLabel 조합만으로 된다. 모양은 app.qss의 QLabel#intro… 규칙이 정한다.
class IntroFooter : public QWidget
{
    Q_OBJECT
public:
    explicit IntroFooter(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
