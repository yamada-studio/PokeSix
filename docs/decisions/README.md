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
