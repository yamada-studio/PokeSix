# design/

Claude Design에서 받은 시각 디자인 패키지와, 우리가 보낸 디자인 요청서를 둔다.
코드가 아니라 **기준 자료**다. 핸드오프 폴더는 받은 그대로의 스냅샷이며 수정하지 않는다.

| 폴더 | 내용 | 상태 |
|---|---|---|
| [handoff-v1/](handoff-v1/PROMPT.md) | 첫 핸드오프: 기초 · 컴포넌트 · 도감 · 아이템 · 스쿼드 · 설정, 세대 규칙과 테스트 값, 토큰 · QSS | 유효. 단, 홈(SCR-01) · 마크(§6) · 앱 막대 탭은 v2가 대체 |
| [handoff-v2/](handoff-v2/PROMPT.md) | 요청 0001의 결과: 인트로(타이틀) 화면, 4탭 앱 막대, 캡슐 마크 · 전 플랫폼 아이콘 | 유효. `docs/*b_*.md`가 v1의 해당 절을 덮어쓴다 |
| [requests/](requests/) | 우리가 Claude Design에 보낸 요청서 | 0001 → handoff-v2 |

## 읽는 법

- 수치의 정답은 각 핸드오프의 `design/source/*.dc.html`이다. 브라우저가 아니라 **텍스트로** 읽는다(목록 데이터는 파일 끝 `renderVals()`)
- 기준 이미지는 `images/screens/`에 있다
- 핸드오프의 기술 전제(C++17 · QtTest · `ps::tok` · core의 QtCore)는 이 리포의 결정으로 대체된다.
  대응표: [docs/architecture.md §3](../docs/architecture.md#3-디자인-설계서와의-대응)
- 핸드오프 문서 안의 `docs/design-handoff/`, `docs/design-handoff-v2/` 경로는 옮기기 전의 위치다.
  각각 `design/handoff-v1/`, `design/handoff-v2/`로 읽는다
- v2의 `Home.dc.html`, `Identity.dc.html`은 v1 유물(참고용)이다

## 새 디자인이 필요할 때

1. `requests/NNNN-<slug>.md`에 요청서를 쓴다(무엇을 · 왜 · 지킬 선 · 받을 형식)
2. 결과 패키지를 `handoff-vN/`으로 받는다
3. 채택 결정은 `docs/decisions/`에 ADR로 남긴다
