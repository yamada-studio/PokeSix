#pragma once

namespace com::yamada::studio {
// 본 화면(앱 막대 아래)의 페이지. 인트로 메뉴의 순서(1–4)와 같다.
// 인트로가 "이 페이지를 열어 달라"고 요청할 때와, 앱 막대 탭(A9)이 쓴다.
enum class Page {
    Dex,
    Items,
    Squad,
    Settings,
};
} // namespace com::yamada::studio
