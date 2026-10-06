#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
// 획득법(야생 출현) 목록 (도감 상세): 그 세대 모든 버전을 버전 배지로 구분해 한 줄씩.
//   [Pt] 201번 도로     풀숲 · 동굴   Lv 2–4   25%
// 장소 · 방법 이름은 공략 사전(guidebook)으로 옮긴다. 출현이 없으면(스타팅 · 진화로만 얻는 포켓몬)
// 그렇다고 한 줄로 알린다.
class EncounterList : public QWidget
{
    Q_OBJECT
public:
    explicit EncounterList(QWidget *parent = nullptr);

    // evolvedForm: 진화 전 단계가 있는 포켓몬 — 야생에 없을 때 안내가 달라진다.
    // elsewhere: 이 버전에는 없고 같은 묶음의 다른 버전에서 만나는 경우 그 버전 이름("하트골드")
    void setEncounters(const QList<EncounterEntry> &encounters, Language language,
                       bool evolvedForm = false, const QStringList &elsewhere = {});
    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QList<EncounterEntry> m_encounters;
    Language m_language = Language::Korean;
    bool m_evolvedForm = false;
    QStringList m_elsewhere;
};
} // namespace com::yamada::studio
