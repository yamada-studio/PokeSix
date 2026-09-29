#pragma once

#include <QStyledItemDelegate>

namespace com::yamada::studio {
class SpriteCache;

// 도감 목록의 한 칸을 디자인대로 그린다 (Dex.dc.html 목록 표, 01 §5-4 · 02 SCR-02).
// 모델/뷰에서 delegate는 "칸 하나를 어떻게 그릴지"를 맡는다. 모델은 값만 주고(QtCore), 색 · 글꼴 ·
// 칩은 여기(ui)서 정한다.
//   줄: 높이 40(디자인 34보다 조금 크게 — 아이콘이 들어가고, 도감은 스크롤이 길어도 괜찮다) · 짝수
//   줄 paper.alt · 선택 줄 yellow.tint + ▶ · 아래 line.soft 1px 번호: 나눔고딕코딩 13 굵게 text.3 /
//   아이콘 34×28 / 이름: 나눔고딕 14 ExtraBold / 타입: 칩 종족값: 나눔고딕코딩 13, 120+ blue.deep
//   굵게 · 100–119 굵게 · ~59 red.deep / 합계: 14 굵게 빨강
// (글자는 디자인보다 1px씩 크게. 사용자 결정 — design/README.md "의도한 차이")
class DexRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static constexpr int kRowHeight = 40;

    // sprites: 아이콘 파일 캐시(소유하지 않는다). 파일이 아직 없으면 받기를 부탁하고 빈 칸으로
    // 둔다.
    explicit DexRowDelegate(SpriteCache *sprites, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    void paintIcon(QPainter *painter, const QRectF &cell, int pokemonId) const;

    SpriteCache *m_sprites = nullptr;
};
} // namespace com::yamada::studio
