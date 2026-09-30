#pragma once

#include <QStyledItemDelegate>

namespace com::yamada::studio {
class SpriteCache;

// 아이템 대백과 목록의 한 칸을 디자인대로 그린다 (Items.dc.html 목록 표, 02 SCR-03, E3).
//
// - 줄: 높이 40 · 홀수 줄 paper.alt · 선택 줄 yellow.tint + ▶ · 아래 line.soft 1px
// - 아이콘: 아이템 스프라이트(30×30 도트, 늘리지 않는다). 없으면(기술머신 등) 원 자리 표시
// - 이름: 나눔고딕 14 ExtraBold / 효과: 나눔고딕 13 text.2, 길면 말줄임
// - 세대 칸: 18×16 칸 9개(사이 2) — 있음 = green + 먹선, 없음 = 점선, 지금 세대 = 노란 테 2px
//   (목록에는 지금 세대에 있는 아이템만 나오므로 지금 세대 칸은 늘 초록이다)
//
// 줄 높이 · 글자는 도감과 맞춰 디자인(34 · 12–13)보다 조금 크다(design/README.md "의도한 차이").
class ItemRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static constexpr int kRowHeight = 40;
    static constexpr int kGenerationCount = 9;
    // 세대 칸 열의 폭: 18 × 9 + 2 × 8 + 좌우 여백 4씩
    static int generationsColumnWidth();
    // 세대 칸 열(cell) 안에서 g세대 칸의 사각형. 머리 칸(ItemHeaderView)도 이걸로 숫자를 맞춘다.
    static QRectF generationRect(const QRectF &cell, int generation);

    // sprites: 아이템 아이콘 파일 캐시(소유하지 않는다).
    explicit ItemRowDelegate(SpriteCache *sprites, QObject *parent = nullptr);

    void setGeneration(int generation) { m_generation = generation; } // 노란 테의 기준

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintIcon(QPainter *painter, const QRectF &cell, const QString &identifier) const;
    void paintGenerations(QPainter *painter, const QRectF &cell, int bits) const;

    SpriteCache *m_sprites = nullptr;
    int m_generation = 1;
};
} // namespace com::yamada::studio
