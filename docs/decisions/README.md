# 아키텍처 결정 기록 (ADR)

되돌리기 어렵거나, 나중에 "왜 이렇게 했지?"라고 물을 만한 결정을 한 파일에 하나씩 기록한다.

- 새 결정은 [0000-template.md](0000-template.md)를 복사해 다음 번호로 만든다
- 결정을 바꿀 때는 기존 문서를 고치지 않는다. 새 ADR을 쓰고, 기존 문서의 상태를 `Superseded by 00NN`으로 바꾼다
- 상태: `Proposed` → `Accepted` / `Rejected` / `Superseded`

| 번호 | 제목 | 상태 |
|---|---|---|
| [0001](0001-qt-widgets-over-qml.md) | UI는 Qt Widgets로 만든다 | Accepted |
| [0002](0002-qt-free-core.md) | core를 Qt 없는 정적 라이브러리로 분리한다 | Accepted |
| [0003](0003-qt-6.8-minimum-and-aqtinstall.md) | 최소 Qt 6.8, aqtinstall 권장 | Accepted |
| [0004](0004-googletest-via-fetchcontent.md) | GoogleTest를 FetchContent + 해시로 고정한다 | Accepted |
| [0005](0005-design-handoff-adoption.md) | 디자인 핸드오프를 기존 구조 위에 맞춰 반영한다 | Accepted |
| [0006](0006-semver-and-trunk-based-branches.md) | SemVer(Phase = MINOR)와 main 하나 + 짧은 작업 브랜치 | Accepted |
| [0007](0007-ui-component-structure.md) | UI 컴포넌트는 "그리기 · 동작 · 겉모양"을 나눠 모듈화한다 | Accepted |
| [0008](0008-intro-screen-replaces-home.md) | 홈 화면을 전체 화면 인트로(타이틀)로 바꾼다 | Accepted |
| [0009](0009-design-handoff-v2.md) | 디자인 핸드오프 v2(인트로 · 4탭 · 캡슐 마크)를 채택한다 | Accepted |
| [0010](0010-intro-implemented-by-claude.md) | 인트로 화면을 Claude가 구현하고, 그 과정의 기술 결정을 확정한다 | Accepted |
| [0011](0011-data-from-pinned-pokeapi-csv.md) | 게임 데이터는 첫 실행에 PokéAPI CSV 원본(고정 커밋)을 받아 로컬 SQLite로 변환한다 | Accepted |
| [0012](0012-dex-presentation-in-json.md) | 도감 선택 버튼의 표시 규칙(버전 색 · 약칭 · 도감 숨김)은 JSON 설정 파일로 둔다 | Accepted |
| [0013](0013-localization.md) | 표기 언어 — 게임 데이터는 DB 3열 + 대체 순서, 화면 문구는 Qt Linguist | Accepted |
| [0014](0014-guidebook-dictionaries.md) | 도감 상세의 기준 게임(세대 대표)과 공략 사전(기술머신 획득처 · 장소 이름) | Accepted |
| [0015](0015-home-card-fan.md) | 홈(인트로)의 세대 선택은 카드 부채꼴로 | Accepted |
| [0016](0016-outside-decorations-drawn-by-parent.md) | 상자 바깥 장식(포커스 링 · 선택 테)은 부모 위젯이 자기 여백에 그린다 | Accepted |
| [0017](0017-build-identity-string.md) | 빌드 식별 문자열: git describe를 빌드마다 헤더로 생성, Debug는 C++에서 표시 | Accepted |
