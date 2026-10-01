#pragma once

#include <QCursor>

// 앱의 마우스 커서: 흰 장갑(resources/icons/cursor/*.svg, 직접 그린 그림).
//   pointer() — 누를 수 있는 곳(버튼 · 탭 · 목록 줄). Qt::PointingHandCursor 대신 쓴다
//   grab()    — 움켜쥔 장갑. 목록 줄에 올라갈 때 "꾹꾹" 애니메이션(RowHover)의 한 장
// 두 그림은 같은 핫스폿(검지 끝)을 써서, 바꿔 끼워도 누르는 자리가 그대로다.
namespace com::yamada::studio::cursors {
const QCursor &pointer();
const QCursor &grab();
} // namespace com::yamada::studio::cursors
