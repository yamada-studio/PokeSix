#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QLabel;

namespace com::yamada::studio {
class ShadowButton;
class SpriteCache;
class StatRadar;

// 도감 목록 오른쪽의 미리 보기 창 내용 (02 SCR-02 오른쪽 창, E1).
// 목록에서 줄을 고르면(한 번 클릭 · ↑↓) 요약을 보여 준다: 그림 · 번호 · 이름 · 타입 ·
// 종족값 레이더 · 약점(받을 때 ×4 · ×2). [자세히 보기](또는 더블클릭 · Enter)가 전체 화면 상세다.
// 약점 계산은 DexPage가 한다(상성표를 들고 있는 쪽) — 여기는 받은 것을 그리기만.
class DexPreview : public QWidget
{
    Q_OBJECT
public:
    explicit DexPreview(QWidget *parent = nullptr);

    void setSpecies(const SpeciesRow &row, int generation, const QStringList &quadWeak,
                    const QStringList &doubleWeak, Language language);
    void clear(); // 빈 상태("목록에서 포켓몬을 고르면 …")로

signals:
    void detailRequested(); // [자세히 보기]

private:
    class Head;
    class WeakRows;

    QLabel *m_empty = nullptr;
    Head *m_head = nullptr;
    QLabel *m_statsCaption = nullptr;
    StatRadar *m_radar = nullptr;
    QLabel *m_weakCaption = nullptr;
    WeakRows *m_weak = nullptr;
    ShadowButton *m_detail = nullptr;
    SpriteCache *m_fronts = nullptr;
};
} // namespace com::yamada::studio
