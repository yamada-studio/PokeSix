#pragma once

#include <QFont>
#include <QString>

class QApplication;

// 디자인 시트를 코드로 옮긴 곳(ADR 0007 7항). 색 값은 tokens.h, 여기는 "적용하는 방법"이다.
namespace com::yamada::studio::theme {
// 앱 전체에 디자인 스타일을 입힌다.
//   1) resources/fonts의 번들 글꼴을 등록한다(설치하지 않아도 앱 안에서 쓸 수 있게)
//   2) :/styles/app.qss를 읽어 @이름 자리를 tokens.h의 색으로 바꾸고 setStyleSheet 한다
// QApplication을 만든 직후, 위젯을 만들기 전에 한 번 부른다(main.cpp).
void apply(QApplication &app);

// QSS 문자열 안의 @이름(예: @red.deep)을 #RRGGBB로 바꾼다. 모르는 이름은 그대로 두고 로그를 남긴다.
QString substituteTokens(const QString &styleSheet);

// 등록된 글꼴 이름(fc-scan으로 확인한 실제 family 이름).
inline constexpr char kFamilyTitle[] = "Do Hyeon";         // 도현: 제목 · 메뉴 · 버튼
inline constexpr char kFamilyBody[] = "NanumGothic";       // 나눔고딕: 본문
inline constexpr char kFamilyData[] = "NanumGothicCoding"; // 나눔고딕코딩: 숫자 · 키
inline constexpr char kFamilyPixel[] = "Silkscreen"; // Silkscreen(도트): 워드마크 · GEN 뱃지

// paintEvent에서 쓸 글꼴. 디자인 수치가 전부 px라서 pointSize가 아닌 pixelSize로 맞춘다
// (pointSize는 화면 DPI에 따라 실제 픽셀 크기가 달라진다).
QFont font(const char *family, int pixelSize, QFont::Weight weight = QFont::Normal);
} // namespace com::yamada::studio::theme
