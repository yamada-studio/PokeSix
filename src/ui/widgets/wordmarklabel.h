#pragma once

#include <QFont>
#include <QWidget>

namespace com::yamada::studio {
// 워드마크 "POKESIX". Silkscreen 700, "SIX"만 빨강, 노란 글자 그림자(오프셋, 블러 없음).
// 인트로 기준 글자 80px · 그림자 5px 5px (디자인 02b SCR-01 #2).
//
// QLabel 대신 직접 그리는 이유: 한 줄 안에서 글자 색이 두 가지이고, 같은 글자를
// 오프셋해서 먼저 그리는 그림자가 필요하다. 둘 다 QSS로는 표현할 수 없다.
class WordmarkLabel : public QWidget
{
    Q_OBJECT
public:
    explicit WordmarkLabel(QWidget *parent = nullptr);

    // 글자 폭 + 그림자 여백. 레이아웃이 이 크기를 보고 배치한다(고정 크기를 박지 않는다).
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QFont m_font;
};
} // namespace com::yamada::studio
