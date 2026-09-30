#pragma once

#include <QWidget>

class QLabel;
class QLineEdit;

namespace com::yamada::studio {
// 검색 칸: 돋보기 · 입력 · 단축키 표시("Ctrl K") (디자인 01 §5-7, 앱 막대 검색 폭 280 · 높이 36).
// 먹선 2 · 반경 6 · 흰 바탕. 입력에 포커스가 있으면 테두리가 파랑(포커스 표시).
// 검색 결과 팝업(포켓몬 · 기술 · 아이템)은 E단계에서 붙인다. 지금은 입력만 받는다.
class SearchField : public QWidget
{
    Q_OBJECT
public:
    explicit SearchField(const QString &placeholder, const QString &shortcutText = QString(),
                         QWidget *parent = nullptr);

    // 크기: 선호 280 · 최소 160 · 높이 36. 레이아웃이 이 두 hint를 보고 창 폭에 따라 160–280 사이로
    // 나눠 준다(앱 막대). 최대 폭도 280으로 걸어 둔다 — 어느 레이아웃에 넣어도 그 이상 늘지 않게.
    static constexpr int kPreferredWidth = 280;
    static constexpr int kMinimumWidth = 160;
    static constexpr int kHeight = 36;

    QLineEdit *lineEdit() const { return m_edit; }
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched,
                     QEvent *event) override; // 입력의 포커스 변화를 엿보고 다시 그린다
    void resizeEvent(QResizeEvent *event) override; // 좁으면 단축키 배지를 숨긴다

private:
    QLineEdit *m_edit = nullptr;
    QLabel *m_shortcut = nullptr; // "Ctrl K" 배지. 단축키 문구가 없으면 nullptr
    // 이보다 좁으면 배지를 숨긴다: 160 폭에 배지(약 40 + 간격 8)까지 넣으면 입력 칸이 60px 남짓만
    // 남는다
    static constexpr int kShortcutMinWidth = 220;
};
} // namespace com::yamada::studio
