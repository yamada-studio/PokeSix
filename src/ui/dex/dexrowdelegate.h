#pragma once

#include "data/text/localizedtext.h"

#include <QStyledItemDelegate>

namespace com::yamada::studio {
class SpriteCache;

// 도감 목록의 한 칸을 디자인대로 그린다 (Dex.dc.html 목록 표, 01 §5-4 · 02 SCR-02).
// 모델/뷰에서 delegate는 "칸 하나를 어떻게 그릴지"를 맡는다. 모델은 값만 주고(QtCore), 색 · 글꼴 ·
// 칩은 여기(ui)서 정한다.
//
// - 줄: 높이 40 · 짝수 줄 paper.alt · 선택 줄 yellow.tint + ▶ · 아래 line.soft 1px
// - 번호: 나눔고딕코딩 13 굵게 text.3
// - 아이콘 34×28 / 이름: 나눔고딕 14 ExtraBold / 타입: 칩
// - 종족값: 나눔고딕코딩 13 (120+ blue.deep 굵게 · 100–119 굵게 · ~59 red.deep)
// - 합계: 나눔고딕코딩 14 굵게 빨강
//
// 줄 높이와 글자는 디자인보다 조금 크게 했다(사용자 결정 — design/README.md "의도한 차이").
//
// 칸 안에서 글자가 서는 자리(contentRect · alignment)는 머리 칸(DexHeaderView)도 같은 함수로
// 계산한다 → 머리 글자와 데이터가 px 단위로 같은 선에 선다.
class DexRowDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    static constexpr int kRowHeight = 40;

    // 타입 칸에 칩 두 개가 딱 들어가는 폭: 18타입 중 가장 넓은 칩 둘 + 칩 사이 간격 + 칸 좌우 여백.
    // 포켓몬은 타입이 최대 2개이므로 어느 조합이든 잘리지 않는다. 글꼴 폭으로 계산하니 px를 적지
    // 않는다.
    static int typesColumnWidth(Language language = Language::Korean);

    // 칸 정렬 규칙: 글자 칸(번호 · 이름 · 타입)은 왼쪽, 숫자 칸(종족값 · 합계)은 오른쪽.
    // 숫자를 오른쪽에 두는 건 자릿수가 세로로 맞아야(45 / 100) 비교하기 쉬워서다(디자인 01 §3).
    static Qt::Alignment alignment(int column);
    // 칸 rect에서 글자가 들어갈 안쪽 rect: 좌우 여백 4, 마지막 칸(합계)은 오른쪽 끝 여유 kEndGap
    // 더.
    static QRectF contentRect(const QRectF &cell, int column);
    // 표 오른쪽 끝(스크롤바 앞) 여유. 합계 숫자가 스크롤바에 붙지 않게.
    static constexpr int kEndGap = 8;

    // sprites: 아이콘 파일 캐시(소유하지 않는다). 파일이 아직 없으면 받기를 부탁하고 빈 칸으로
    // 둔다.
    explicit DexRowDelegate(SpriteCache *sprites, QObject *parent = nullptr);

    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setLanguage(Language language) { m_language = language; } // 타입 칩 글자

    // 포켓몬 아이콘(박스 아이콘, 투명 여백을 잘라 cell 가운데 34×28에). 파일이 없으면 받기를
    // 부탁하고 아무것도 그리지 않는다. 진화 트리(EvolutionView)도 같이 쓴다.
    static void paintPokemonIcon(QPainter *painter, const QRectF &cell, int pokemonId,
                                 SpriteCache *sprites);

private:
    SpriteCache *m_sprites = nullptr;
    Language m_language = Language::Korean;
};
} // namespace com::yamada::studio
