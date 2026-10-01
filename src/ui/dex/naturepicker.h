#pragma once

#include "data/repository/repository.h"

#include <QAbstractButton>
#include <QWidget>

namespace com::yamada::studio {
// 성격표 팝업 (도감 상세의 종족값 카드 머리 [성격 ▾]): 고전적인 5×5 표.
//   세로(줄) = 오르는 능력치(▲ +10%), 가로(칸) = 내리는 능력치(▼ −10%), 대각선 = 무보정(노력 ·
//   수줍음 …)
//            ▼공격  ▼방어  ▼특공  ▼특방  ▼스피드
//   ▲공격   노력    외로움  고집   개구쟁이 용감
//   …
// 칸을 누르면 natureChosen(id), 맨 아래 [성격 없음]은 natureChosen(0). Qt::Popup 창이라 바깥을
// 누르면 닫힌다.
class NaturePicker : public QWidget
{
    Q_OBJECT
public:
    NaturePicker(const QList<Nature> &natures, int currentId, Language language,
                 QWidget *parent = nullptr);

    QSize sizeHint() const override;

signals:
    void natureChosen(int natureId); // 0 = 성격 없음

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int cellAt(const QPoint &pos) const; // 칸 번호(줄 * 5 + 칸), 성격 없음 = 25, 없으면 −1
    int natureAt(int row, int column) const;

    QList<Nature> m_natures;
    int m_current = 0;
    int m_hover = -1;
    Language m_language = Language::Korean;
};

// 종족값 카드 머리의 작은 버튼 "성격 ▾" / "조심 ▾"
class NatureButton : public QAbstractButton
{
    Q_OBJECT
public:
    explicit NatureButton(QWidget *parent = nullptr);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};
} // namespace com::yamada::studio
