#pragma once

#include "data/text/localizedtext.h"

#include <QList>
#include <QString>
#include <QWidget>

namespace com::yamada::studio {
// 문제 목록(02 SCR-04 ①) — 분석 창의 맨 위. 문구는 SquadPage가 만들어 넘긴다.
//   [땅] 땅 — 약점 3, 받아낼 포켓몬 1                       [방어]
//        초염몽 ×2 · 엠페르트 ×2 · 렌트라 ×2 / 받아냄: 토게키스 ×0
class ProblemList : public QWidget
{
    Q_OBJECT
public:
    struct Row
    {
        QString typeKey; // 타입 칩 색("ground")
        QString title;
        QString detail;
        bool offense = false; // 태그: 방어 / 공격
    };

    explicit ProblemList(QWidget *parent = nullptr);

    void setRows(const QList<Row> &rows, const QString &emptyText, Language language);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override { return {200, sizeHint().height()}; }

signals:
    void rowHovered(int row); // −1 = 벗어남
    void rowClicked(int row);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int rowAt(const QPoint &pos) const;

    QList<Row> m_rows;
    QString m_emptyText;
    int m_hover = -1;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
