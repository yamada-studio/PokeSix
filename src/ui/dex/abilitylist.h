#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
// 특성 목록 (도감 상세): 한 줄에 특성 하나 — [이름] [숨겨진 특성] 효과 문구(그 세대 게임 설명문).
// 1–2세대는 특성이 없었다고 알린다. 효과가 길면 말줄임하고 툴팁으로 전체를 보인다.
class AbilityList : public QWidget
{
    Q_OBJECT
public:
    explicit AbilityList(QWidget *parent = nullptr);

    void setAbilities(const QList<AbilityEntry> &abilities, int generation, Language language);
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    bool event(QEvent *event) override;

private:
    QList<AbilityEntry> m_abilities;
    int m_generation = 0;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
