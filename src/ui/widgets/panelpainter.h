#pragma once

#include "ui/theme/tokens.h"

#include <QMargins>
#include <QRgb>

class QPainter;
class QRect;

namespace com::yamada::studio {
// 창("패널")의 모양. 기본값은 v1 표준 패널(디자인 시트 §4): 먹선 2 · 반경 8 · 단단한 그림자 4.
// 색을 QColor가 아닌 QRgb로 들고 있어서 constexpr로 만들 수 있다(HomePage의 kMenuWindow 참고).
struct PanelStyle
{
    int outline = 2;         // 바깥 먹선 두께
    int radius = 8;          // 바깥 모서리 반경
    int shadow = 4;          // 바로 아래로 떨어지는 먹색 그림자(블러 없음)
    QRgb fill = tok::kWhite; // 창 바탕
    QRgb ink = tok::kInk;    // 먹선 · 그림자 색

    // 선택: 먹선 안쪽의 두 번째 선(인트로 메뉴 창의 "이중 테").
    int innerInset = 0; // 먹선과 안쪽 선 사이 간격
    int innerLine = 0;  // 안쪽 선 두께. 0이면 그리지 않는다
    int innerRadius = 0;
    QRgb innerColor = tok::kLine;

    // 선택: 컬러 머리(디자인 시트 §4 "머리 높이 38 · 머리 아래 ink 2px"). 0이면 머리 없음.
    // 머리 색은 역할로 고른다(01 §5-2): 빨강 = 주 콘텐츠, 파랑 = 필터 · 분석 · 정보, 초록 = 분류 …
    int header = 0;
    QRgb headerColor = tok::kRed;
    int headerLine = 2; // 머리 아래 먹선
};

// 겉모양이 사방에서 차지하는 두께 → 겉모양 위젯이 자기 레이아웃의 contentsMargins로 쓴다.
// 아래쪽에는 그림자가 더해진다. 그림자는 위젯 rect "안"에 그리기 때문이다(ADR 0007 4항).
QMargins chromeMargins(const PanelStyle &style);

// rect(그림자 포함) 안에 그림자 → 바탕 → 먹선 → 안쪽 선 순서로 그린다.
// 창 모양을 그리는 곳은 여기 하나뿐이다(ADR 0007 1항). PanelFrame, 앞으로의 카드 · 모달이 모두 이
// 함수를 부른다.
void paintPanel(QPainter &painter, const QRect &rect, const PanelStyle &style);
} // namespace com::yamada::studio
