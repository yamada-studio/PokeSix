#pragma once

#include <QWidget>

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

    QLineEdit *lineEdit() const { return m_edit; }

protected:
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched,
                     QEvent *event) override; // 입력의 포커스 변화를 엿보고 다시 그린다

private:
    QLineEdit *m_edit = nullptr;
};
} // namespace com::yamada::studio
