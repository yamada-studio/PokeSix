#pragma once

#include <QAbstractButton>
#include <QFont>

namespace com::yamada::studio {
// 단단한 그림자가 있는 버튼 (디자인 시트 §4 · §5-7의 ShadowButton).
//   Primary   = 빨강 채움 · 흰 글자 · 높이 40 (확정: "데이터 받기", "다시 시도")
//   Secondary = 흰 바탕 · 먹색 글자 · 높이 36 (보조: "취소")
// 공통: 먹선 2 · 반경 6 · 아래 그림자 3(블러 없음) · 도현. 눌리면 2px 내려앉고 그림자가 사라진다.
//
// QSS가 아니라 paintEvent로 그리는 이유: QSS는 블러 없는 오프셋 그림자를 그릴 수 없다(ADR 0007).
// 아이콘은 QAbstractButton::setIcon()으로 준다(글자 왼쪽, 15px).
class ShadowButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Variant { Primary, Secondary };

    explicit ShadowButton(Variant variant = Variant::Primary, QWidget *parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    // QAbstractButton은 Space로만 눌린다. 버튼 관례대로 Enter도 누르게 한다.
    void keyPressEvent(QKeyEvent *event) override;

private:
    int boxHeight() const; // 그림자를 뺀 버튼 높이 (Primary 40, Secondary 36)

    Variant m_variant;
    // 글꼴을 QWidget::font()가 아니라 따로 들고 있는 이유: app.qss의 QWidget { font-… } 규칙이
    // setFont()로 준 글꼴을 덮어쓴다. 직접 그리는 위젯은 자기 글꼴로 그린다(WordmarkLabel과 같다).
    QFont m_font;
};
} // namespace com::yamada::studio
