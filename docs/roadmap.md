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
- 디자인 기준: [design/](../design/README.md). 수치가 이미지와 다르면 `design/source/*.dc.html`이 정답이다

상태: ⬜ 대기 · 🟨 진행 중 · ✅ 완료

---

## Phase 0 — 프로젝트 세팅 ✅ → v0.0.1

빌드 시스템, 프리셋, OS별 스크립트, 코드 품질 도구, core 골격, 테스트 파이프라인, 문서,
디자인 핸드오프 반영, 글꼴 · 앱 아이콘 가져오기.

## Phase A — 인트로 화면으로 배우는 Qt 기초 → v0.1.0

**전체 화면 인트로(마크 · 워드마크 · 현재 세대 버튼 · ▶ 메뉴)와, 메뉴로 들어가는 본 화면의 틀(4탭 앱 막대 + 페이지)을 완성하면서**
Qt의 기초(창, 레이아웃, 리소스, 테마, 커스텀 페인트, 시그널/슬롯)를 순서대로 익힌다.
근거: [ADR 0008](decisions/0008-intro-screen-replaces-home.md)(인트로로 전환) · [ADR 0009](decisions/0009-design-handoff-v2.md)(v2 채택).
기준 이미지: [`30_intro_1440.png`](../design/handoff-v2/images/screens/30_intro_1440.png) · `31`(세대 메뉴) · `34`(상태 · 앱 막대).
사양: [02b §SCR-00 · SCR-01](../design/handoff-v2/docs/02b_SCREEN_SPEC_V2.md), 디자인 시트 v1 §1–§5 + [01b](../design/handoff-v2/docs/01b_DESIGN_SHEET_V2.md). 원본 수치: `design/handoff-v2/design/source/Intro.dc.html`.

| | 단계 | 배우는 것 | 눈에 보이는 결과 |
|---|---|---|---|
| ✅ | **A1** MainWindow와 디버깅 | VS Code 프리셋 빌드·디버그, `Q_OBJECT`와 moc, `QMainWindow`, object tree, `QLoggingCategory` | 1440×900 창, 최소 960×640, 로그, 생성자 breakpoint |
| ✅ | **A2** 레이아웃 — 두 단계 화면과 인트로 뼈대 | `QVBoxLayout` / `QHBoxLayout`, margin · spacing · stretch, `addSpacing`, 가운데 정렬과 `sizeHint`, `QStackedWidget` 두 단계, CSS 수치 → Qt 환산 | `m_screens`[인트로 │ 본 화면(AppBar + `m_pages`)]. 인트로의 마크 · 워드마크 · 부제 · 세대 블록 · 메뉴 창(5줄) · 아래 정보 줄이 **최종 클래스 이름 그대로의 빈 상자로** `30`과 같은 x 좌표에 선다 |
| 🟨 | **A3** 캡처 도구와 앱 초기화 | `QCommandLineParser`, `QWidget::grab()`, offscreen 렌더링, 첫 `pokesix_app` 타깃, 로그 형식, `setWindowIcon` | `PokeSix --screenshot intro 1440x900 out.png` → 이후 단계를 `30`과 나란히 비교. 창 제목 표시줄에 캡슐 아이콘 |
| ✅ | **A4** 글꼴 · 토큰 · QSS | qrc, `QFontDatabase`, `Tokens.h`(v2) 이식(`ui/theme`) + 의미 역할 층, QSS `@token` 치환, `QPalette`, 폰트 메트릭으로 `sizeHint()` | 종이색 바탕, Silkscreen 워드마크 · 도현 부제와 메뉴 이름 · 나눔고딕 설명과 정보 줄 |
| ✅ | **A5** 첫 `paintEvent` — 바탕 · 창 · 글자 그림자 | `QPainter`, `QPainterPath`, 안티에일리어싱, `QPixmap` 타일, 블러 없는 오프셋 그림자, 그리기 함수(`paintPanel`)와 위젯의 분리 | 사선 무늬 + 위아래 빨강 띠(`IntroBackground`), 먹선 3 · 이중 테 · 그림자 6의 메뉴 창, 노란 그림자의 워드마크 |
| ✅ | **A6** 캡슐 마크를 코드로 | `QTransform`(회전 −35°), 경로 합성 · 클리핑, 크기별 규칙(01b §6-3), `sizeHint()` | `MarkWidget` 136이 `41_mark_final.png`의 128+ 규칙대로 그려진다(광택 · 눈빛 · 입 · 볼) |
| 🟨 | **A7** 메뉴 동작 — 시그널/슬롯 | `QAbstractButton`, hover가 선택을 옮김, 키보드(↑↓ 순환 없음 · Enter · 1–4 · Esc), 포커스 이유(`Qt::TabFocusReason`), ▶ 커서와 노란 선택 칸, 눌림 2px, **시그널/슬롯**, 상자 바깥 포커스 링 | 메뉴로 본 화면에 들어가고(임시 되돌아가기 버튼으로) 인트로로 돌아온다. 종료 줄 · Esc 두 번 → 종료 |
| ⬜ | **A8** 세대 버튼 · 세대 메뉴와 AppState | `Qt::Popup` 창, `Q_PROPERTY`, 첫 `pokesix_data` 타깃, core의 세대 표, `QSettings`(`gen`) | "GEN 4 신오 ▾" → 폭 300 · 9줄 팝업(▶ · ✓), 고른 세대가 `AppState`에 들어가고 다음 실행에도 남는다 |
| ✅ | **A9** 앱 막대와 탭 | 폴더 탭 커스텀 페인트, 마크 버튼(스티커 변형 36, hover 점선 테), `QShortcut`(Ctrl+1…4), `enum class Page` + `MainWindow::open(Page)` | 빨강 앱 막대: 캡슐 마크 + POKESIX + 탭 4 + 세대 버튼(같은 `AppState`) · 검색 자리. 메뉴 선택 → 그 탭이 활성인 본 화면, 마크 → 인트로 |
| ✅ | **A9b** 창 크기를 도감 표에 맞추기 ([가이드](guides/a9b-window-size.md)) | `sizeHint` / `minimumSizeHint` / `QSizePolicy`, 레이아웃의 크기 협상, `resizeEvent`, `QMainWindow` 최소 크기 | 920×840으로 열리고, 좁히면 검색창만 280 → 160으로 줄어든다 |
| ⬜ | **A10** 마무리 | 캡처 차이 목록, 키보드만으로 왕복 확인 | `30` · `31`과의 차이 목록이 비어 있거나 설명 가능 → **v0.1.0** |

- **A2 후반 · A4 · A5 · A6과 A3 · A7의 일부는 사용자 요청으로 Claude가 구현했다**([ADR 0010](decisions/0010-intro-implemented-by-claude.md)).
  학습 루프상으로는 해당 코드(`src/ui/theme`, `src/ui/widgets`, `src/ui/home`)를 읽고 설명할 수 있으면 그 단계를 이해한 것으로 본다
  - A3 남은 것: `--screenshot`, `pokesix_app` 타깃, 로그 형식. 끝난 것: 창 아이콘, 설치 시 Qt RPATH
  - A6: 마크는 SVG(`QSvgRenderer`)로 그렸다. `QTransform`으로 직접 그려 보는 것은 선택 과제
  - A7 남은 것: 포커스 링(열린 질문 7). 끝난 것: 선택 · 키보드 · hover · 눌림 · 종료 · 메뉴 → 본 화면 전환(A9와 함께)
  - A9는 사용자 요청으로 Claude가 구현했다: 마크 버튼(→ 인트로), 폴더 탭 4, 앱 막대용 세대 버튼, 검색 칸, `MainWindow::open(Page)`, Ctrl+1…4 · Ctrl+K
- 캡슐 앱 아이콘 리소스는 v2로 교체했다([ADR 0009](decisions/0009-design-handoff-v2.md))
- `FirstRunPanel`(데이터 없음 · 받는 중 · 실패, `34` ③)은 D4에서 만든다. Phase A의 인트로는 데이터가 있는 상태만 다룬다
- 1024 · 960 인트로 배치(`32`, `33`)는 Phase F에서 다룬다
- v1 홈의 세대 카드 · 최근 스쿼드 · 바로 가기는 빠졌다. 최근 스쿼드 목록은 E2에서 다룬다

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
테스트 값은 [04_RULES_AND_TESTS.md](../design/handoff-v1/docs/04_RULES_AND_TESTS.md)의 표를 그대로 쓴다.

| | 단계 | 내용 | 통과할 테스트 |
|---|---|---|---|
| ⬜ | **C1** | 세대 표현 + `TypeChart::forGeneration` — **"규칙을 데이터로" 설계 과제** | T2-5, T2-6, T2-7 |
| ⬜ | **C2** | `GenerationRules`: 존재 타입, 물리/특수 판정, 설정 토글 반영 | 04 §2 규칙 표 |
| ⬜ | **C3** | 값 타입: `Species`, `Move`, `Squad` / 복합 타입 방어 배율 | T2-1, T2-2, T2-4 |
| ⬜ | **C4** | `SquadAnalyzer` — 약점·내성 집계, 문제 판정, 커버리지, 물리/특수 분포 | **T1 전체** |

## Phase D — 데이터 (PROMPT 4-4, 4-7 일부) → v0.4.0

| | 단계 | 내용 | 주제 |
|---|---|---|---|
| ✅ | **D1** | SQLite 스키마 + 개발 · 테스트용 시드 CSV(목업의 4세대 19종 · 기술 · 아이템, `tests/fixtures/`) + `CsvImporter`(CSV → SQLite) | QtSql, 트랜잭션, 세대 구간 쿼리, CSV 파싱 |
| ⬜ | **D1b** | 기술 · 이름 · 변경 이력 · 습득 기술(63만 줄) · 아이템 변환 + `Repository` 조회 창구 | 대량 INSERT 성능, 버전 그룹 → 세대, 조회 API 설계 |
| ✅ | **D2** | `Repository`(종 목록 조회) + `SpeciesTableModel` + `SpeciesFilterProxy` + 도감 목록 화면(`DexPage`, 행 delegate · 타입 칩) | **모델/뷰**, role, `QSortFilterProxyModel`, delegate |
| 🟨 | **D2b** | ([가이드](guides/d2b-regional-dex.md)) 전국도감과 세대별 지역 도감(관동 · 성도 · 호연 · 신오 …) 분리: `pokedexes` · `pokemon_dex_numbers` · `pokedex_version_groups` CSV 변환, 도감 목록에 도감 선택(전국 / 그 세대 지역 도감)과 지역 번호 칸 | 스키마 확장, 버전 그룹 → 도감 매핑, 모델 칸 · 정렬 키 바꾸기 |
| ⬜ | **D3** | `SquadSession` + `SquadStore`(JSON, 디바운스 저장) → 홈의 최근 스쿼드를 실제 데이터로 | `QJsonDocument`, `QSaveFile`, 디바운스 `QTimer` |
| ✅ | **D4** | 데이터 받기: PokéAPI CSV 원본을 **고정 커밋**에서 다운로드, 파일별 **SHA-256 검증**, 파일 단위 이어받기 ([ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md)) | `QNetworkAccessManager`, 비동기 흐름, `QCryptographicHash`, `QSaveFile` |
| ✅ | **D5** | `DataUpdater`: 받기 → worker 스레드에서 `CsvImporter`로 변환 → 임시 DB 교체, 인트로 `FirstRunPanel`(받기 전 · 받는 중 · 실패, v2 `34` ③)과 메뉴 잠금 | `moveToThread`, 스레드별 `QSqlDatabase` 연결, 진행 시그널, 상태 전환 |

- **D1의 CP1–CP3(CsvReader · 스키마 · 타입 · 상성 · 종 · 포켓몬 · 종족값)도 사용자 요청으로 Claude가 구현했다.** CP4(`pokesix-import-csv`)는 사용자가 구현했다. 기술 · 습득 기술 · 아이템과 `Repository`는 D1b로 나눴다
- **D5**: CP1(`DataUpdater` · worker 스레드)은 사용자가, CP2–CP4(메뉴 잠금 · `FirstRunPanel` · 연결)는 사용자 요청으로 Claude가 구현했다
- **D2는 사용자 요청("이어서해봐")으로 Claude가 구현했다**: 세대 4 고정(A8에서 `AppState`로), 필터 · 상세 창은 E1. 이후 아이콘 칸(실행 중에 사용자 캐시로 받기) · 번호 오름차순 · 고정 칸 폭을 더했다
- **D4는 사용자 요청("섞어서")으로 Claude가 구현했다**: `CsvDownloader`, 고정 목록 `csvsource.h`, 개발 도구 `pokesix-fetch-csv`.
  D1(스키마 · CSV 파서 · `CsvImporter`)과 D5의 worker 스레드는 사용자가 가이드를 따라 구현한다
- Phase D를 Phase A · C보다 먼저 시작했다. D1은 core의 타입(`core/types`)만 있으면 되므로 순서에 막히지 않는다

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

1. ~~**PokéAPI 가져오기 방식** (D4 전)~~ → 첫 실행에 CSV 원본(고정 커밋 + SHA-256)을 받아 로컬에서 SQLite로 변환([ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md))
2. **스프라이트** (E1 전): 끝까지 `SpritePlaceholder`로 둘지, 런타임에 받아 개인 캐시에 둘지
3. **앱 이름 "PokeSix"** (공개 배포 전): "Poké" 접두어의 상표 문제를 검토할 필요가 있는지
4. **패키징 방식** (Phase G): OS별 배포 형식
5. ~~**Tokens 네이밍** (A4)~~ → 핸드오프 이름(`tok::kRed`) 유지로 결정([ADR 0010](decisions/0010-intro-implemented-by-claude.md))
6. ~~**글꼴 12MB를 qrc에 넣을지** (A4)~~ → 실행 파일 qrc에 넣고 `qt_add_big_resources`로 빌드 속도 유지([ADR 0010](decisions/0010-intro-implemented-by-claude.md))
7. **상자 바깥 장식을 그리는 방식** (A7): 부모 위젯이 그리기 vs `QFocusFrame`처럼 대상 위를 덮는 형제 위젯. 선택 테와 포커스 링에 같이 쓴다
8. **모달 · 드로어 방식** (E2 전): 창 안 오버레이(dim + `PanelFrame`, 추천 기울기) vs frameless `QDialog`. 근거는 [ADR 0007](decisions/0007-ui-component-structure.md) 9항
9. **종료 확인** (A7 전): 종료 줄 Enter · 두 번째 Esc에서 확인 대화 상자를 띄울지(v2 PROMPT §6-1)
10. **PokéAPI 출처 표기 문구** (A10 전): 인트로 "데이터: PokéAPI · 비공식 팬 도구"가 약관에 맞는지(v2 PROMPT §6-3)

## 장기 목표 (Phase A–G 이후)

지금 구조는 이 목표들을 막지 않게 유지한다. 설계 방향은 [architecture.md §9](architecture.md#9-확장-여지).

| 목표 | 필요한 것 | 지금 지켜 둘 것 |
|---|---|---|
| **배틀 시뮬레이터** | core의 세대별 배틀 규칙 엔진, 기술 · 특성 · 도구 효과 구현, 결정적 난수 | 계산은 core에(Qt 없음), 세대는 항상 인자로, id는 PokéAPI id |
| **에뮬레이터 오버레이** | 게임 내부 번호 → PokéAPI id 표(`*_game_indices`), 에뮬레이터 상태 읽기, 별도 창 | core 분석 · data state를 UI와 분리해 두기 |

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
