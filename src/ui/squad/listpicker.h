#pragma once

#include <QDialog>

#include <functional>

class QListView;
class QSortFilterProxyModel;
class QStringListModel;

namespace com::yamada::studio {
class RowHover;
class SearchField;

// 검색 + 목록에서 하나를 고르는 창(포켓몬 · 기술 · 지닌 물건이 같이 쓴다).
//
// 줄은 문자열 하나(검색 대상)로만 모델에 들어간다. 그리는 건 넘겨받은 painter 함수가 한다 —
// 원본 줄 번호(row)를 받아 호출한 쪽이 가진 목록에서 꺼내 그린다. 그래서 선택지마다 모델 ·
// delegate 클래스를 따로 만들지 않아도 된다.
//   ListPicker picker(tr("기술 고르기"), searchTexts, [&](QPainter &p, const QRect &r, int row,
//                     bool hot) { … rows.at(row) … }, 34, this);
//   if (picker.exec() == QDialog::Accepted) use(picker.chosenRow());   // −1 = "없음"을 골랐다
class ListPicker : public QDialog
{
    Q_OBJECT
public:
    using Painter = std::function<void(QPainter &painter, const QRect &rect, int row, bool hot)>;

    ListPicker(const QString &title, const QStringList &searchTexts, Painter painter, int rowHeight,
               QWidget *parent = nullptr);

    // 아래쪽 "없음" 버튼(기술 비우기 · 물건 없음). 누르면 Accepted + chosenRow() = −1
    void setNoneText(const QString &text);
    void setCurrentRow(int row); // 처음에 고른 줄(지금 값)
    // 목록 위 칸 제목 띠. painter가 row = −1로 불린다(줄과 같은 rect 계산으로 제목을 맞춘다)
    void showHeader(int height = 24);
    int chosenRow() const { return m_chosen; }
    QListView *view() const { return m_view; } // 그림이 받아지면 다시 그리기용

protected:
    bool eventFilter(QObject *watched, QEvent *event) override; // 검색 칸의 ↑↓ · Enter

private:
    void choose(const QModelIndex &proxyIndex);

    SearchField *m_search = nullptr;
    QListView *m_view = nullptr;
    QStringListModel *m_model = nullptr;
    QSortFilterProxyModel *m_proxy = nullptr;
    RowHover *m_hover = nullptr;
    QWidget *m_noneButton = nullptr;
    QWidget *m_header = nullptr;
    Painter m_painter;
    int m_chosen = -1;
};
} // namespace com::yamada::studio
