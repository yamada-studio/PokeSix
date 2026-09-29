#pragma once

#include <QAbstractButton>

namespace com::yamada::studio {
// 인트로 메뉴의 한 줄 (디자인 02b SCR-01 #5a · #5c, 상태는 34 ①).
//
//   [▶] 도감 백과                  [1]     ← Kind::Entry  높이 58
//       종족값 · 타입 상성 · 습득 기술
//   [▶] 종료                        Esc    ← Kind::Quit   높이 42
//
// "선택"은 이 줄이 스스로 정하지 않는다. 마우스 hover와 키보드 이동을 한곳에서 다루도록
// IntroMenu가 setSelected()로 알려 준다. 이 줄은 hover를 hovered() 시그널로 알리기만 한다.
// 포커스는 IntroMenu 하나만 받는다(NoFocus) — Tab이 줄마다 멈추지 않게.
class IntroMenuItem : public QAbstractButton
{
    Q_OBJECT
public:
    enum class Kind { Entry, Quit };

    explicit IntroMenuItem(Kind kind = Kind::Entry, QWidget *parent = nullptr);

    void setDescription(const QString &description); // 이름 아래 한 줄 (Entry만)
    void setShortcutText(const QString &text);       // 오른쪽 단축키 칸: "1" … "4", "Esc"

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

signals:
    void hovered();

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEnterEvent *event) override;

private:
    void paintEntry(QPainter &painter, const QRectF &row);
    void paintQuit(QPainter &painter, const QRectF &row);

    Kind m_kind;
    QString m_description;
    QString m_shortcut;
    bool m_selected = false;
};
} // namespace com::yamada::studio
