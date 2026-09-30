#pragma once

#include <QAbstractButton>
#include <QFont>

namespace com::yamada::studio {
// 세대 버튼 "4세대 ▾". 인트로와 앱 막대(A9)에서 같이 쓴다. 누르면 세대 메뉴(1세대 … 7세대)가
// 버튼 아래에 열리고, 고르면 generationSelected(n)를 보낸다(로드맵 A8).
//
// 디자인(02b SCR-01 #4)은 "[GEN 4] 신오"였지만, 지방은 도감 백과의 도감 버튼(신오 · 성도 …)이
// 나타내므로 세대만 보인다(사용자 결정 — design/README.md "의도한 차이").
//
// 버튼은 "무엇을 골랐다"만 알린다. 실제 세대를 바꾸는 건 AppState이고, 바뀐 값은
// setGeneration()으로 다시 받아 그린다 — 버튼이 상태를 따로 들고 있지 않는다(값의 주인은 하나).
class GenerationButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Size {
        Large,   // 인트로: 높이 46 · 도현 20
        Compact, // 앱 막대: 높이 36 · 도현 16
    };

    explicit GenerationButton(Size size = Size::Large, QWidget *parent = nullptr);

    void setGeneration(int number);               // 보이는 세대("4세대")
    void setGenerationRange(int first, int last); // 메뉴에 나올 세대(AppState의 범위)

    QSize sizeHint() const override;

signals:
    void generationSelected(int number); // 메뉴에서 고를 때만(지금 세대를 다시 골라도 보낸다)

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString label() const;
    void showMenu();

    Size m_size;
    int m_number = 1;
    int m_first = 1;
    int m_last = 1;
    QFont m_font;
};
} // namespace com::yamada::studio
