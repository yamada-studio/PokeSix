#pragma once

#include <QAbstractButton>
#include <QFont>

namespace com::yamada::studio {
// 세대 버튼 "[GEN 4] 신오 ▾" (디자인 02b SCR-01 #4). 인트로와 앱 막대(A9)에서 같이 쓴다.
//
// QAbstractButton을 상속하므로 클릭 · 키보드(Space) · 포커스 · clicked() 시그널은
// Qt가 처리하고, 이 클래스는 모양(paintEvent)과 크기(sizeHint)만 책임진다.
// 누르면 열리는 세대 메뉴 팝업과 AppState 연결은 A8에서 붙인다.
class GenerationButton : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Size {
        Large,   // 인트로: 높이 46 · 지역명 도현 20
        Compact, // 앱 막대: 높이 36 · 지역명 도현 16
    };

    explicit GenerationButton(Size size = Size::Large, QWidget *parent = nullptr);

    // 표시할 세대. number는 뱃지("GEN 4"), region은 지역명("신오").
    void setGeneration(int number, const QString &region);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QString badgeText() const;

    Size m_size;
    int m_number = 1;
    QString m_region;
    QFont m_badgeFont;
    QFont m_regionFont;
};
} // namespace com::yamada::studio
