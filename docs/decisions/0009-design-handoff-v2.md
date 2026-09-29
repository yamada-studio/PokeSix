# 0009. 디자인 핸드오프 v2(인트로 · 4탭 · 캡슐 마크)를 채택한다

- 상태: Accepted
- 날짜: 2026-09-29

## 맥락

[요청서 0001](../../design/requests/0001-intro-and-mark.md)의 결과로 Claude Design의 v2 패키지가 도착했다([design/handoff-v2/](../../design/handoff-v2/PROMPT.md)).
인트로(타이틀) 화면 3크기와 상태, 4탭 앱 막대, 캡슐 마크와 전 플랫폼 아이콘이 들어 있다.
v1과 마찬가지로 저장소를 보지 않고 만들어졌고, PROMPT는 Claude가 직접 구현하는 것을 전제로 한다.
같은 날 디자인 자료를 `docs/` 밖의 최상위 `design/`으로 옮겼다(`handoff-v1/`, `handoff-v2/`, `requests/`).

## 결정

- **v2를 v1 위에 덮어 채택한다.** v2의 `docs/*b_*.md`가 v1의 SCR-00 · SCR-01 · 디자인 시트 §6을 대체하고, 나머지 v1 규격은 계속 유효하다
- [ADR 0005](0005-design-handoff-adoption.md)의 대체 규칙을 그대로 적용한다: C++20 · GoogleTest · Qt 없는 core · `com::yamada::studio` · 학습 루프
- **이름**: 페이지는 `HomePage`(`ui/home/`)를 유지한다([ADR 0008](0008-intro-screen-replaces-home.md)). v2 문서의 `IntroPage`는 이 리포의 `HomePage`다.
  부품은 핸드오프 이름을 그대로 쓴다: `IntroMenu`, `IntroFooter`, `IntroBackground`, `MarkWidget`, `WordmarkLabel`, `GenerationButton` + `GenerationMenu`, `FirstRunPanel`
- **캡슐 마크를 채택한다.** 포켓볼의 구별 요소(원형 몸통 · 몸통을 가로지르는 띠 · 가운데 둥근 버튼)가 없고, 01b §6-1이 그 조합을 금지 사항으로 명시한다.
  앱 아이콘 리소스(`resources/icons/app/`)를 v2로 교체했다. 파일 이름이 같아 빌드 설정은 바뀌지 않았다
- 메뉴는 5줄(도감 · 아이템 · SixSquad · 설정 · 종료)로 간다
- **`FirstRunPanel`(데이터 없음 · 받는 중 · 실패)은 Phase D4(PokéAPI 가져오기)에서 만든다.** `DataUpdater`가 있어야 의미가 있기 때문이다. Phase A의 인트로는 데이터가 있는 상태만 다룬다
- v2 PROMPT §6의 질문은 해당 단계 전에 정한다([roadmap 열린 질문](../roadmap.md#열린-질문-해당-단계-전에-결정))
  - 종료 확인 대화 상자를 띄울지(A7 전)
  - 버전 문자열: 이미 CMake `PROJECT_VERSION` → `POKESIX_VERSION` → `QApplication::applicationVersion()` 경로가 있다. 결정할 것 없음
  - PokéAPI 출처 표기 문구(A10 전)

## 결과

- 인트로 기준 이미지는 `design/handoff-v2/images/screens/30`–`34`다. 캡처 비교는 A3 이후 이 이미지로 한다
- 1024 · 960 인트로 배치(960에서 마크 + 워드마크 가로 배치)는 Phase F의 반응형에서 다룬다
- 핸드오프 문서 안의 옛 경로(`docs/design-handoff…`)는 스냅샷이라 고치지 않는다. 읽는 법은 [design/README.md](../../design/README.md)
