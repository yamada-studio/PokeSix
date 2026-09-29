# CHANGELOG — 디자인 v1 → v2 (디자인 요청 0001)

## 화면
- **추가** SCR-01 인트로: `Intro.dc.html`(1440) · `Intro1024.dc.html` · `Intro960.dc.html` · `IntroStates.dc.html`(상태 · 첫 실행 · 4탭 앱 막대)
- **삭제** v1 엔트리 홈(`Home.dc.html`) — 구현 대상 아님. 캔버스에는 "(v1)"로 참고용 보관
- **수정** 앱 막대: "홈" 탭 제거(탭 4개), 마크 버튼 → 인트로, 마크를 캡슐 스티커로 교체
  - 적용 파일: `Dex`, `Items`, `Squad`, `Settings`, `DexCompact`, `SquadCompact` (본문 변경 없음)

## 마크 · 아이콘
- **교체** 육각 "여섯 칸" → 캡슐. 시안 A 말랑 육각 · B 캡슐 · C 알 → **B 확정**
- `assets/icons/` 전부 새로 생성 (macOS icns · iconset, Windows ico · png, Linux hicolor 16–1024 + scalable, 파비콘 ico · png, SVG 원본 4종)
- 16px 수작업 픽셀 원본 교체

## 토큰 (`design/tokens.json`, `Tokens.h`) — 추가만, 삭제 · 값 변경 없음
- 색: `capsule.red` `capsule.cream #FFF8EC` `capsule.blush #F4A3A0` `capsule.gloss` · `plate.macTop #3E7FD0` `plate.macBottom #1D4E8F` `plate.linuxBase #B01F19` `plate.linuxFace #FFFDF8` · `intro.stripe #EFEADF` `intro.titleShadow #F2C12E` `menu.pressed #F2C12E`
- 크기: `introMenuWidth` `introMenuRow` `introMenuRowCompact` `introMenuRowMin` `introMark` `introWordmark` `appBarMark` `appBarTabs`
- 새 블록 `mark`: 캡슐 기하 · 크기별 규칙
- `pokesix.qss`: 변경 없음

## 문서
- `docs/01b_DESIGN_SHEET_V2.md` — §1 추가 토큰, §6 마크 교체
- `docs/02b_SCREEN_SPEC_V2.md` — SCR-00 수정, SCR-01 인트로
- `docs/03b_ARCHITECTURE_V2.md` — root 스택 구조, IntroPage, 첫 실행 흐름, 아이콘 연결
- 다이어그램: `architecture.png`, `screen-flow.png` 갱신
