#pragma once

#include <QStyledItemDelegate>

namespace com::yamada::studio {
// 도감 목록의 한 칸을 디자인대로 그린다 (Dex.dc.html 목록 표, 01 §5-4 · 02 SCR-02).
// 모델/뷰에서 delegate는 "칸 하나를 어떻게 그릴지"를 맡는다. 모델은 값만 주고(QtCore), 색 · 글꼴 ·
// 칩은 여기(ui)서 정한다.
//   줄: 높이 34 · 짝수 줄 paper.alt · 선택 줄 yellow.tint + ▶ · 아래 line.soft 1px
//   번호: 나눔고딕코딩 12 굵게 text.3 / 이름: 나눔고딕 13 ExtraBold / 타입: 칩
//   종족값: 나눔고딕코딩 12, 120+ blue.deep 굵게 · 100–119 굵게 · ~59 red.deep / 합계: 13 굵게 빨강
class DexRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    using QStyledItemDelegate::QStyledItemDelegate; // 생성자는 베이스 것을 그대로

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
} // namespace com::yamada::studio
