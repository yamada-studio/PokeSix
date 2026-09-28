# 0005. 디자인 핸드오프를 기존 구조 위에 맞춰 반영한다

- 상태: Accepted
- 날짜: 2026-09-28

## 맥락

Claude Design으로 만든 디자인 핸드오프([docs/design-handoff/](../design-handoff/PROMPT.md))가 도착했다.
화면 캡처, 디자인 시트, 화면 정의서, 설계서, 규칙 테스트 값, 토큰·QSS 시작 파일, 앱 아이콘이 들어 있다.
핸드오프는 저장소를 보지 않고 작성되었다. 그래서 일부 전제가 이미 확정한 결정과 다르다(C++17, QtTest,
core의 QtCore 허용, `ps::tok` namespace, 데이터 원본 미정). 또 PROMPT.md는 **Claude가 직접 구현하는 것**을 전제로 한다.

## 결정

- 핸드오프 스스로도 "기존 구조가 있으면 존중하라"고 적고 있다. 그래서 **기존 레이어 구조를 유지하고 역할을 대응시킨다**
  (대응표는 [architecture.md §3](../architecture.md#3-디자인-설계서와의-대응))
- 기존 결정을 우선한다: C++20(0003 맥락), GoogleTest(0004), Qt 없는 core(0002), `com::yamada::studio`, PokéAPI 캐시
- 핸드오프에서 그대로 채택하는 것: 시각 디자인 전부, 컴포넌트 이름, DB 스키마(`gen_from`/`gen_to`),
  설정 키, 04 문서의 테스트 값, 폭 구간, `--screenshot`·`--gallery` 검증 방식
- **진행 방식은 CLAUDE.md의 학습 루프를 따른다.** PROMPT.md의 작업 순서(4-1~4-7)는
  [roadmap.md](../roadmap.md)의 단계로 다시 쪼갰다
- UI 문구의 소스 언어는 한국어(디자인 문구 그대로)
- 핸드오프 폴더는 원본 스냅샷으로 **수정하지 않고** 보존한다. 필요한 에셋(아이콘)은 `resources/`로 복사했다

## 결과

- 이미지와 수치가 다르면 `design/source/*.dc.html`이 정답이다
- 핸드오프 PROMPT §7의 질문 중 데이터 원본(PokéAPI)과 최소 Qt(6.8)는 이미 결정되었다.
  나머지(스프라이트, 앱 이름 상표, 패키징)는 roadmap.md의 "열린 질문"으로 옮겼다
