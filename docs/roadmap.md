# 구현 로드맵

## 진행 방식 — 학습 루프

```
 ┌─▶ ① 가이드 (Claude)   목표 · 개념 · 읽을 문서 · 완료 조건  →  docs/guides/<step>.md
 │   ② 구현 (사용자)      공부하며 직접 코드 작성 (브랜치는 Claude가 미리 만들어 둔다)
 │   ③ 진단 (Claude)      diff · 빌드 로그 · 실행 결과 · 캡처 비교 → 문제와 원인
 └── ④ 다음 단계          완료 조건을 만족하면 Claude가 커밋 · merge · 체크 표시
```

- 한 단계는 **작고, 끝나면 눈에 보이는 결과**가 있는 크기로 나눈다
- 가이드는 그 단계를 시작할 때 쓴다. 미리 전부 쓰지 않는다
- **Git 작업은 Claude가 전담한다**(브랜치 · 커밋 · merge · 태그 · CHANGELOG). 규칙: [versioning-and-git.md](versioning-and-git.md). Phase가 끝나면 MINOR 릴리스
- 진단을 요청할 때: "A2 진단해줘"라고 하면 된다. 막혔으면 에러 메시지 전체를 붙인다
- 디자인 기준: [design-handoff/](design-handoff/). 수치가 이미지와 다르면 `design/source/*.dc.html`이 정답이다

상태: ⬜ 대기 · 🟨 진행 중 · ✅ 완료

---

## Phase 0 — 프로젝트 세팅 ✅ → v0.0.1

빌드 시스템, 프리셋, OS별 스크립트, 코드 품질 도구, core 골격, 테스트 파이프라인, 문서,
디자인 핸드오프 반영, 글꼴 · 앱 아이콘 가져오기.

## Phase A — 홈 화면으로 배우는 Qt 기초 → v0.1.0

**홈 화면 하나를 기준 이미지 [`10_home_1440.png`](design-handoff/images/screens/10_home_1440.png)와
같게 완성하면서** Qt의 기초(창, 레이아웃, 리소스, 테마, 커스텀 페인트, 시그널/슬롯)를 순서대로 익힌다.
사양: [02_SCREEN_SPEC.md](design-handoff/docs/02_SCREEN_SPEC.md) SCR-00(앱 막대) · SCR-01(홈),
[01_DESIGN_SHEET.md](design-handoff/docs/01_DESIGN_SHEET.md) §1–§5. 원본 수치: `design/source/Home.dc.html`.

| | 단계 | 배우는 것 | 눈에 보이는 결과 |
|---|---|---|---|
| ✅ | **A1** MainWindow와 디버깅 | VS Code 프리셋 빌드·디버그, `Q_OBJECT`와 moc, `QMainWindow`, object tree, `QLoggingCategory` | 홈을 올릴 창: 1440×900, 최소 960×640, 로그, 생성자 breakpoint |
| ⬜ | **A2** 레이아웃 — 홈의 뼈대 | `QVBoxLayout` / `QHBoxLayout` / `QGridLayout`, margin · spacing · stretch, size policy, `QStackedWidget` | 앱 막대 자리(60) + 홈 페이지: 제목 줄, 세대 카드 9칸, 하단(최근 스쿼드 │ 바로 가기 3)이 **빈 상자로** 캡처와 같은 비율로 배치 |
| ⬜ | **A3** 캡처 도구 | `QCommandLineParser`, `QWidget::grab()`, offscreen 렌더링, 첫 `pokesix_app` 타깃 | `PokeSix --screenshot home 1440x900 out.png` → 이후 모든 단계를 기준 이미지와 나란히 비교 |
| ⬜ | **A4** 글꼴 · 토큰 · QSS | qrc, `QFontDatabase`, `Tokens.h` 이식(`ui/theme`), QSS `@token` 치환, `QPalette` | 종이색 바탕, 도현 36 제목, 나눔고딕 설명문 |
| ⬜ | **A5** PanelFrame — 첫 `paintEvent` | `QPainter`, `QPainterPath`, 안티에일리어싱, 블러 없는 오프셋 그림자 | 빨강 머리 "최근 스쿼드" 창, 바로 가기 카드 3개의 테두리와 그림자 |
| ⬜ | **A6** 세대 카드와 AppState | 커스텀 위젯, 마우스 이벤트 · hover, **시그널/슬롯**, `Q_PROPERTY`, 첫 `pokesix_data` 타깃, core의 세대 표 | 세대 카드 9장(머리 색 순환, Silkscreen "GEN n"). 클릭하면 노란 테와 "▶ 진행 중"이 따라 옮겨 간다 |
| ⬜ | **A7** 앱 막대와 탭 | SVG 렌더링(`QSvgRenderer`), 폴더 탭 커스텀 페인트, `QShortcut`(Ctrl+1…5) | 빨강 앱 막대: 마크 + POKESIX 워드마크 + 탭 5 + 세대 버튼 · 검색 자리. 탭으로 페이지 전환(홈 외 4개는 빈 페이지) |
| ⬜ | **A8** 최근 스쿼드 · 바로 가기 · 마무리 | 행 위젯 조합, 임시 데이터, 빈 상태, 캡처 차이 목록으로 마감 | 홈 화면 완성. 기준 이미지와 차이 목록이 비어 있거나 설명 가능 → **v0.1.0** |

- A8의 최근 스쿼드는 **임시 데이터**로 채운다. 실제 저장소(`SquadStore`)는 Phase D에서 연결한다
- 반응형(1100 미만 5열 × 2행)은 Phase F에서 다룬다

## Phase B — 컴포넌트와 갤러리 (PROMPT 4-2) → v0.2.0

`--gallery` 인자로 컴포넌트 갤러리 창을 띄우고 `02_components.png`와 비교한다.
A에서 만든 `PanelFrame`, 버튼 스타일도 갤러리에 올린다.

| | 단계 | 컴포넌트 | 주제 |
|---|---|---|---|
| ⬜ | **B1** | 갤러리 창 + `TypeChip`, `SpritePlaceholder` | 크기 변형(MD/SM/XS), `sizeHint()`, 점선 테두리 |
| ⬜ | **B2** | `ShadowButton`, `FilterChip`, `SearchField` | 기존 위젯 상속 + 페인트 보강, 포커스 링 |
| ⬜ | **B3** | `ToggleSwitch`, `SegmentedControl` | 체크 가능한 커스텀 위젯, 키보드 조작, 접근성 역할 |
| ⬜ | **B4** | `StatBar` | 값 → 색 구간, 고정폭 숫자 정렬 |
| ⬜ | **B5** | `EmptyState`, `SkeletonRows`, `SegmentProgress` | `QTimer` 계단식 애니메이션 |
| ⬜ | **B6** | `RangeSlider` | 마우스 이벤트, 손잡이 두 개 |

`HeatmapWidget`, `SquadSlotCard`는 core 분석 결과가 필요하므로 Phase E의 스쿼드 화면에서 만든다.

## Phase C — core 도메인과 분석 (PROMPT 4-3) → v0.3.0

순수 C++20 TDD. Qt 학습과 번갈아 진행해도 된다(머리 식히기용).
테스트 값은 [04_RULES_AND_TESTS.md](design-handoff/docs/04_RULES_AND_TESTS.md)의 표를 그대로 쓴다.

| | 단계 | 내용 | 통과할 테스트 |
|---|---|---|---|
| ⬜ | **C1** | 세대 표현 + `TypeChart::forGeneration` — **"규칙을 데이터로" 설계 과제** | T2-5, T2-6, T2-7 |
| ⬜ | **C2** | `GenerationRules`: 존재 타입, 물리/특수 판정, 설정 토글 반영 | 04 §2 규칙 표 |
| ⬜ | **C3** | 값 타입: `Species`, `Move`, `Squad` / 복합 타입 방어 배율 | T2-1, T2-2, T2-4 |
| ⬜ | **C4** | `SquadAnalyzer` — 약점·내성 집계, 문제 판정, 커버리지, 물리/특수 분포 | **T1 전체** |

## Phase D — 데이터 (PROMPT 4-4, 4-7 일부) → v0.4.0

| | 단계 | 내용 | 주제 |
|---|---|---|---|
| ⬜ | **D1** | SQLite 스키마 + 시드 데이터(목업의 4세대 19종·기술·아이템) + `Repository` 조회 | QtSql, 트랜잭션, 세대 구간 쿼리 |
| ⬜ | **D2** | `SpeciesTableModel` + `SpeciesFilterProxy` | **모델/뷰**, role, `QSortFilterProxyModel` |
| ⬜ | **D3** | `SquadSession` + `SquadStore`(JSON, 디바운스 저장) → 홈의 최근 스쿼드를 실제 데이터로 | `QJsonDocument`, `QSaveFile`, 디바운스 `QTimer` |
| ⬜ | **D4** | PokéAPI 가져오기 → 캐시 DB | `QNetworkAccessManager`, 비동기 흐름, worker 스레드 — **방식 결정 필요(아래 열린 질문 1)** |

## Phase E — 나머지 화면 (PROMPT 4-5) → v0.5.0

각 화면은 홈과 같은 순서로 쌓는다: **바탕·여백 → 패널 배치 → 컴포넌트 연결 → 상태(빈·로딩·선택) → 캡처 비교**.

| | 단계 | 화면 | 기준 이미지 | 필요한 선행 단계 |
|---|---|---|---|---|
| ⬜ | **E1** | 도감 (목록 · 그리드 · 상세) | `11_`, `12_dex_1440_*.png` | D2, B4 |
| ⬜ | **E2** | SixSquad (슬롯 카드 · 히트맵 · 분포) | `14_squad_1440.png` | C4, D3 |
| ⬜ | **E3** | 아이템 | `13_items_1440.png` | D1 |
| ⬜ | **E4** | 설정 | `15_settings_1440.png` | B3, F2 |

## Phase F — 반응형 · 설정 · 다크 테마 (PROMPT 4-6, 4-7) → v0.6.0

| | 단계 | 내용 |
|---|---|---|
| ⬜ | **F1** | `ResponsiveController` — 폭 구간(1360/1100/1024/960), 히스테리시스, 드로어. 기준: `20_dex_1024_drawer.png`, `21_squad_1024.png` |
| ⬜ | **F2** | `Settings`(QSettings 래퍼) — [conventions.md §8](conventions.md#8-설정-qsettings) 키, 즉시 적용 |
| ⬜ | **F3** | 다크 테마 — 토큰 교체만으로 전환되는지 검증 |

## Phase G — 배포 준비 → v1.0.0

CI(GitHub Actions, 3개 OS), 패키징(dmg / zip / AppImage), sanitizer 프리셋, i18n 영어 번역.

---

## 열린 질문 (해당 단계 전에 결정)

1. **PokéAPI 가져오기 방식** (D4 전)
   REST API로 1025종·기술·습득 기술을 받으면 첫 실행 때 요청이 수만 건이 되어 fair use 취지("요청 빈도 자제")에 어긋난다.
   후보는 세 가지다.
   - GraphQL 엔드포인트로 몇 번의 대량 쿼리만 보내기
   - PokéAPI가 공개한 CSV 데이터 덤프로 한 번에 가져오기
   - 필요한 세대만 지연(lazy) 로딩하기
2. **스프라이트** (E1 전): 끝까지 `SpritePlaceholder`로 둘지, 런타임에 받아 개인 캐시에 둘지
3. **앱 이름 "PokeSix"** (공개 배포 전): "Poké" 접두어의 상표 문제를 검토할 필요가 있는지
4. **패키징 방식** (Phase G): OS별 배포 형식
5. **Tokens 네이밍** (A4): 핸드오프의 `kRed`를 유지할지, 컨벤션(`camelCase`)으로 바꿀지
6. **글꼴 12MB를 qrc에 넣을지** (A4): 실행 파일에 포함(배포 간단, 빌드 느림) vs 실행 파일 옆 폴더(빌드 빠름, 경로 관리)

## 나중에 붙일 것 (백로그)

| 항목 | 시점 | 이유 |
|---|---|---|
| GitHub Actions CI (3 OS) | Phase A 이후 가능한 빨리 | macOS·Windows는 로컬에서 검증할 수 없다. `jurplel/install-qt-action`으로 6.8.3 고정 |
| ASan/UBSan 프리셋 | Phase C 전후 | core 로직 메모리 버그 조기 발견 |
| pre-commit clang-format 훅 | 원할 때 | 포맷 누락 방지 (선택) |
| `qt_add_translations` + 영어 `.ts` | Phase G | 소스 언어는 한국어 |
| 배포 스크립트 (`qt_generate_deploy_app_script`) | Phase G | windeployqt / macdeployqt |

## 이 프로젝트에는 불필요

| 항목 | 이유 |
|---|---|
| vcpkg / Conan | 외부 의존성이 Qt(aqtinstall로 고정)와 GoogleTest(FetchContent + 해시)뿐이다 |
| Git LFS | [conventions.md §11](conventions.md#11-git--버전) |
| 크래시 리포팅 · 텔레메트리 | 개인용 도구 |
| HiDPI 수동 설정 | Qt 6 기본 동작으로 충분하다 |
