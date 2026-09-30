#pragma once

#include "data/text/localizedtext.h"

#include <QStyledItemDelegate>

namespace com::yamada::studio {
class SpriteCache;

// 아이템 대백과 목록의 한 칸을 그린다 (Items.dc.html 목록 표, 02 SCR-03, E3).
//
// - 줄: 높이 40 · 홀수 줄 paper.alt · 선택 줄 yellow.tint + ▶ · 아래 line.soft 1px
// - 아이콘: 지금 세대 모양의 아이템 그림(iconKey). 30×30 도트를 늘리지 않는다. 없으면 원 자리 표시
// - 이름: 나눔고딕 14 ExtraBold
// - 효과: 나눔고딕 13 text.2, 길면 말줄임. 기술머신은 [타입 칩] 기술 이름 + 설명
// - 가격: 나눔고딕코딩 13, 오른쪽 정렬("1,200원"). 팔지 않으면 "—"
//
// 줄 높이 · 글자는 도감과 맞춰 디자인(34 · 12–13)보다 조금 크다(design/README.md "의도한 차이").
class ItemRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static constexpr int kRowHeight = 40;

    // 아이콘 파일 key(SpriteCache::Kind::Item). 세대에 따라 그림 모양이 다르다:
    //   1–5세대 = "gen5/<이름>"(BW 그림. 1–4세대 가방 그림은 PokéAPI에 없어 가장 가까운 것을 쓴다),
    //   6세대 이후 = "<이름>". 기술머신 · 비전머신은 담긴 기술의 타입 CD("tm-fire", "hm-water").
    static QString iconKey(const QString &identifier, const QString &machineType, int generation);

    // sprites: 아이템 아이콘 파일 캐시(소유하지 않는다).
    explicit ItemRowDelegate(SpriteCache *sprites, QObject *parent = nullptr);

    void setGeneration(int generation) { m_generation = generation; } // 아이콘 모양의 기준
    void setLanguage(Language language) { m_language = language; }    // 타입 칩 글자

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintIcon(QPainter *painter, const QRectF &cell, const QModelIndex &index) const;
    void paintEffect(QPainter *painter, const QRectF &content, const QModelIndex &index) const;

    SpriteCache *m_sprites = nullptr;
    int m_generation = 1;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
