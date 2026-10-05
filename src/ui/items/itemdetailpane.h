#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QLabel;

namespace com::yamada::studio {
class SpriteCache;

// 아이템 대백과 오른쪽의 상세 창 내용 (02 SCR-03 오른쪽 창, E3).
// 목록에서 줄을 고르면(클릭 · ↑↓): 아이콘 · 이름 · 가격, 세대별 존재(1–9 칸, 지금 세대 테두리),
// 효과 전문, 기술머신이면 담긴 기술과 획득처(사전에 있는 게임만), 진화 아이템이면 진화 대상.
// 조회(진화 대상 · 획득처)는 ItemsPage가 한다 — 여기는 받은 것을 그리기만.
class ItemDetailPane : public QWidget
{
    Q_OBJECT
public:
    explicit ItemDetailPane(SpriteCache *sprites, QWidget *parent = nullptr);

    void setItem(const ItemRow &item, int generation, Language language,
                 const QList<ItemEvolution> &evolutions, const QStringList &sources);
    void clear();

private:
    class Head;
    class GenerationCells;

    QLabel *m_empty = nullptr;
    Head *m_head = nullptr;
    QLabel *m_generationCaption = nullptr;
    GenerationCells *m_generations = nullptr;
    QLabel *m_effectCaption = nullptr;
    QLabel *m_effect = nullptr;
    QLabel *m_extraCaption = nullptr; // "진화" (진화 아이템만)
    QLabel *m_extra = nullptr;
    QLabel *m_sourceCaption = nullptr; // "입수처" — 고른 게임의 입수 사전(없으면 안내 문구)
    QLabel *m_sources = nullptr;
};
} // namespace com::yamada::studio
