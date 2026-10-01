#pragma once

#include "data/text/localizedtext.h"

#include <QSet>
#include <QTimer>
#include <QWidget>

namespace com::yamada::studio {
class SquadSession;

// 방어 상성 히트맵(02 SCR-04 ②): 행 = 슬롯 6(빈 슬롯은 회색), 열 = 공격 타입 18(세대에 없는 타입은
// 사선). 아래 요약 3행(약점 수 · 내성/무효 수 · 공격 커버)과 범례 한 줄.
class HeatmapView : public QWidget
{
    Q_OBJECT
public:
    explicit HeatmapView(SquadSession *session, QWidget *parent = nullptr);

    void refresh(Language language);
    void setSelectedSlot(int slot);                  // −1 = 없음
    void setProblemTypes(const QSet<QString> &keys); // 빨강 테 열(타입 key)
    void setHotType(const QString &key); // 문제 행에 마우스 → 그 열을 진하게
    void flashType(const QString &key);  // 문제를 눌렀다 → 그 열을 두 번 깜빡

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void slotClicked(int slot);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    int labelWidth() const;
    qreal cellWidth() const;
    int rowAt(const QPoint &pos) const; // 슬롯 행(0–5), 없으면 −1

    SquadSession *m_session = nullptr;
    Language m_language = Language::Korean;
    int m_selected = -1;
    QSet<QString> m_problemTypes;
    QString m_hotType;
    QString m_flashType;
    int m_flashStep = 0;
    QTimer m_flash;
};
} // namespace com::yamada::studio
