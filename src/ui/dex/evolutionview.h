#pragma once

#include "data/repository/repository.h"

#include <QWidget>

namespace com::yamada::studio {
class SpriteCache;

// 진화 트리 (도감 상세의 획득법 카드). 뿌리부터 한 줄에 한 종, 깊이만큼 들여 쓴다.
//   [아이콘] 랄토스
//     ↳ [아이콘] 킬리아      Lv 20
//         ↳ [아이콘] 가디안   Lv 30          ← 보고 있는 포켓몬은 노랑 바탕
//         ↳ [아이콘] 엘레이드 각성의돌 사용 · 수컷
// 조건 글자는 그 세대의 방법이다(Repository가 골라 둔다). 줄을 누르면 그 포켓몬의 상세로 간다.
class EvolutionView : public QWidget
{
    Q_OBJECT
public:
    explicit EvolutionView(SpriteCache *icons, QWidget *parent = nullptr);

    void setEvolution(const QList<EvolutionStep> &steps, int currentSpeciesId, Language language);
    QSize sizeHint() const override;

    // 조건 한 줄(화면 문구): "Lv 30" · "각성의돌 사용 · 수컷" · "친밀도 · 낮" …
    static QString conditionText(const EvolutionCondition &condition, Language language);

signals:
    void pokemonClicked(int pokemonId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    bool event(QEvent *event) override; // 조건 툴팁(잘린 글자 전체)

private:
    int rowAt(int y) const;
    QString conditionsOf(const EvolutionStep &step) const;

    SpriteCache *m_icons = nullptr;
    QList<EvolutionStep> m_steps;
    int m_current = 0;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
