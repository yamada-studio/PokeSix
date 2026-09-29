# 아키텍처

PokeSix의 레이어 구조, 의존 방향, 각 디렉토리의 책임을 정리한다.
디자인 핸드오프의 설계서([design/handoff-v1/docs/03_ARCHITECTURE.md](../design/handoff-v1/docs/03_ARCHITECTURE.md))는
"어떤 역할이 필요한가"의 기준으로 쓰고, "그 역할이 어디에 놓이는가"는 이 문서가 정한다.

## 1. 레이어와 의존 방향

```
┌────────────────────────────────────────────────────────────┐
│ app      composition root: 인자 해석, 초기화, 객체 생성·주입      │  pokesix_app
└───────────────────────┬────────────────────────────────────┘
                        ▼
┌────────────────────────────────────────────────────────────┐
│ ui       Qt Widgets: shell · theme · widgets · 화면(page)     │  pokesix_ui
└───────────────────────┬────────────────────────────────────┘      Qt6::Widgets
                        ▼
┌────────────────────────────────────────────────────────────┐
│ data     api · db · repository · store · state · models      │  pokesix_data
└───────────────────────┬────────────────────────────────────┘      Qt6::Core/Sql/Network
                        ▼
┌────────────────────────────────────────────────────────────┐
│ core     types · rules · analysis · domain   (순수 C++20)     │  pokesix_core
└────────────────────────────────────────────────────────────┘      Qt 없음
```

- 화살표 방향으로만 의존한다. 아래 레이어는 위 레이어를 모른다.
- **core는 Qt에 링크하지 않는다.** core에서 Qt 헤더를 include하면 컴파일 에러가 난다(의도된 동작).
- **data는 QtWidgets·QtGui를 쓰지 않는다.** `state/`와 `models/`는 나중에 QML에서도 그대로 쓸 수 있어야 한다.
- 레이어 사이의 타입 변환(`std::string` ↔ `QString`, `std::vector` ↔ `QList`)은 **data 레이어가** 한다.

ROS 2에 빗대면 core는 노드와 무관한 알고리즘 라이브러리, data는 드라이버와 매니저 노드,
ui는 rviz 플러그인, app은 launch 파일이 하는 일(구성하고 연결하기)에 해당한다.

## 2. 디렉토리별 책임

### core — `src/core/` (순수 C++20)

| 디렉토리 | 책임 | 예 |
|---|---|---|
| `types/` | 포켓몬 타입 enum, 상성표 | `Type`, `TypeChart` |
| `rules/` | 세대별 규칙 (존재하는 타입, 물리/특수 판정, 1세대 전용 상성 등) | `GenerationRules` |
| `domain/` | 값 타입 (엔티티) | `Species`, `Move`, `Item`, `Generation`, `Squad` |
| `analysis/` | 팀 분석 (순수 함수) | `SquadAnalyzer`, `AnalysisResult` |

### data — `src/data/` (Qt Core / Sql / Network)

| 디렉토리 | 책임 | 예 |
|---|---|---|
| `update/` | 데이터 받기와 변환: 고정 커밋의 PokéAPI CSV 다운로드 · SHA-256 검증 · 이어받기, CSV → SQLite | `DataUpdater`, `CsvDownloader`, `CsvImporter` |
| `db/` | SQLite 연결, 스키마 생성 | `Database`, `schema.sql` |
| `repository/` | 조회 창구. 로컬 DB만 읽는다(네트워크 없음) | `Repository` |
| `store/` | 사용자 데이터 저장 | `SquadStore` (`squads.json`) |
| `state/` | 앱 상태 QObject. 시그널로 변경 통지 | `AppState`, `SquadSession`, `Settings` |
| `models/` | Qt item model과 필터 프록시 | `SpeciesTableModel`, `SpeciesFilterProxy` |

### ui — `src/ui/` (Qt Widgets)

| 디렉토리 | 책임 | 예 |
|---|---|---|
| `shell/` | 화면 틀: 인트로 │ 본 화면(앱 막대 + 페이지)의 두 단계 스택 | `MainWindow`, `AppBar`, `AppTabBar`, `ResponsiveController` |
| `theme/` | 디자인 토큰, 글꼴, QSS 적용 | `Tokens`, `TypeColors`, `Theme`, `FontLoader` |
| `widgets/` | 재사용 컴포넌트 | `PanelFrame`, `TypeChip`, `StatBar`, `HeatmapWidget` … |
| `home/` `dex/` `items/` `squad/` `settings/` | 화면(page)과 그 화면 전용 위젯. `home`은 전체 화면 인트로 | `HomePage`, `DexPage`, `DetailDrawer` … |

컴포넌트 규칙([ADR 0007](decisions/0007-ui-component-structure.md)):
- `widgets/`에는 디자인 시트 §5에 있거나 두 화면 이상에서 쓰는 것만 둔다. widgets는 화면 폴더를 include하지 않는다
- 창 그림(테두리 · 그림자 · 머리)은 `paintPanel()` 한 곳에서 그린다. 베이스 클래스는 동작으로 고른다
  (클릭 · 선택 → `QAbstractButton`, 컨테이너 → `QWidget`)
- 내용 위젯은 자기를 감싼 겉모양(`PanelFrame` · 드로어 · 모달)을 모른다. 겉모양 두께는 `contentsMargins`로 확보한다
- 위젯 rect = 상자 + 아래 그림자. 상자 바깥 장식(선택 테 · 포커스 링)은 위젯 스스로 그리지 않는다

### app — `src/app/`

composition root. 명령행 인자(`--gallery`, `--screenshot`), 로깅 초기화, 글꼴 로드와 테마 적용,
`AppState`·`Repository` 생성, `MainWindow` 생성과 주입을 맡는다.
**객체를 생성하고 연결하는 코드는 여기에만 둔다.** 다른 레이어는 필요한 의존성을 생성자로 받는다(주입).

## 3. 디자인 설계서와의 대응

설계서는 [design/handoff-v1/docs/03_ARCHITECTURE.md](../design/handoff-v1/docs/03_ARCHITECTURE.md)와 그 위를 덮는 [design/handoff-v2/docs/03b_ARCHITECTURE_V2.md](../design/handoff-v2/docs/03b_ARCHITECTURE_V2.md)다.

| 설계서(03) | 이 리포 | 비고 |
|---|---|---|
| `domain/` | `core/types`, `core/rules`, `core/domain` | |
| `analysis/` | `core/analysis` | 설계서는 `QList`를 허용하지만, 여기서는 **Qt 없이** `std::vector` 사용 |
| `data/` (Repository, SquadStore, DataUpdater) | `data/repository`, `data/store`, `data/api`, `data/db` | |
| `app/` (AppState, SquadSession, Settings) | `data/state` | ui가 의존해야 하므로 ui 아래 레이어에 둔다 |
| `models/` | `data/models` | |
| `theme/` | `ui/theme` | QtGui(`QRgb`, `QFont`, `QPalette`)가 필요하므로 ui 레이어 |
| `ui/MainWindow`, `AppBar` … | `ui/shell` | |
| `ui/widgets` | `ui/widgets` | |
| `ui/pages` | `ui/home`, `ui/dex`, `ui/items`, `ui/squad`, `ui/settings` | 화면별 하위 위젯이 많아서 폴더로 나눔. `home`은 대시보드가 아니라 인트로([ADR 0008](decisions/0008-intro-screen-replaces-home.md)) |
| v2 `IntroPage` | `ui/home/HomePage` | 이름만 다르다([ADR 0009](decisions/0009-design-handoff-v2.md)). 부품 `IntroMenu` · `IntroFooter` · `IntroBackground`는 `ui/home/` |
| v2 `MarkWidget` · `WordmarkLabel` · `GenerationButton` · `GenerationMenu` | `ui/widgets` | 인트로와 앱 막대 두 곳에서 쓴다 |
| v2 `FirstRunPanel` | `ui/home` | Phase D4(`DataUpdater`)와 함께 만든다 |
| v2 `Shell` | `MainWindow`가 조립하는 `QWidget` | 그리기 · 동작이 없는 조합이라 클래스로 만들지 않는다 |
| CMake 타깃 `pokesix_core` 하나 | `pokesix_core` / `pokesix_data` / `pokesix_ui` / `pokesix_app` | 레이어 규칙을 링크 단계에서 강제 |
| QtTest | GoogleTest | core는 Qt가 없으므로 GoogleTest. 04 문서의 표를 그대로 테스트 데이터로 쓴다 |
| C++17 | C++20 | |
| 데이터 원본 미정, 번들 `pokesix.db` | 첫 실행에 PokéAPI CSV(고정 커밋) → 로컬 `pokesix.sqlite` | 설계서의 스키마(`gen_from`/`gen_to`)를 채택. `DataUpdater`는 설계서 이름 그대로 ([ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md)) |

## 4. 데이터 흐름

```
첫 실행 · "지금 갱신" 때만:
  GitHub PokeAPI/pokeapi@<고정 커밋>/data/v2/csv ──HTTP(비동기)──▶ update (SHA-256 검증, 이어받기)
                                                                   │ worker 스레드: CsvImporter
                                                                   ▼
평소:                                                        db (pokesix.sqlite)
                                                                   ▲
                                                        repository ┘  ← 로컬 DB만 읽는다
                                                            │
                                              models / state (QObject, 시그널)
                                                            │
                                                           ui  (view가 model을 표시, 사용자 입력 → state 변경)
```
- **데이터 원천**: PokéAPI API 서버가 아니라 PokéAPI 저장소의 CSV 원본을 고정 커밋에서 받는다([ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md)).
  개발 · 테스트는 `tests/fixtures/`의 작은 시드 CSV를 같은 `CsvImporter`로 변환해 쓴다

- **세대 전환**: `AppState::generationChanged` → 모든 화면이 다시 계산한다. 세대는 앱 전역 상태의 중심이다.
- **스쿼드 편집**: `SquadSession::changed` → `SquadAnalyzer::analyze()`(core, 동기) → 분석 패널 갱신,
  `SquadStore`에 디바운스(800ms) 저장.
- **이름 표기**: UI 문구는 `tr()`(한국어 소스)로, 포켓몬·기술·아이템 이름은 PokéAPI CSV의
  다국어 이름(`*_names.csv`의 ko / en / ja)을 DB에 저장하고 설정(`nameLang`)에 따라 고른다. 두 경로는 별개다.

## 5. 세대별 규칙은 데이터로

세대 차이는 `if (gen >= 6)`식 분기를 코드 곳곳에 두지 않고, **테이블 또는 전략 객체**로 표현한다.
규칙 목록과 반드시 통과해야 할 값은 [design/handoff-v1/docs/04_RULES_AND_TESTS.md](../design/handoff-v1/docs/04_RULES_AND_TESTS.md)에 있다.

- DB: 세대에 따라 달라지는 값(타입, 종족값, 상성)은 `gen_from`/`gen_to` 구간으로 저장한다.
  세대 g의 값은 `gen_from <= g AND (gen_to IS NULL OR gen_to >= g)`로 조회한다.
- core: `GenerationRules`가 "이 세대에 존재하는 타입", "물리/특수 판정 방식", "상성표"를 제공한다.
  구체적인 표현 방식(테이블, 전략, 둘의 조합)은 Phase C의 설계 과제다.

## 6. 스레딩

- **UI 스레드(메인 스레드)를 막지 않는다.** Qt의 이벤트 루프(`QApplication::exec()`)는
  ROS 2의 `spin()`과 같다. 핸들러 하나가 오래 걸리면 화면 전체가 멈춘다.
- 네트워크: `QNetworkAccessManager`는 **원래 비동기**다. 별도 스레드 없이 메인 스레드에서 쓴다.
  응답은 `QNetworkReply::finished` 시그널로 받는다.
- 분석: 6×18 계산이라 UI 스레드에서 동기로 호출해도 된다.
- DB: 처음에는 메인 스레드에서 짧은 쿼리만 쓴다. 대량 가져오기(import)처럼 오래 걸리는 작업은
  worker `QObject` + `moveToThread()` 패턴으로 옮긴다.
  **`QSqlDatabase` 연결은 만든 스레드에서만 쓴다.** 스레드마다 별도 연결 이름으로 연다.

## 7. 에러 처리

- 예외를 **슬롯, 이벤트 핸들러, 이벤트 루프 밖으로 던지지 않는다.** Qt는 그런 경우를 지원하지 않는다.
- core: 실패할 수 있는 연산은 반환값(`std::optional<T>`, 필요하면 간단한 결과 타입)으로 알린다.
  불변식 위반(프로그래밍 오류)은 `assert`.
- data: 동기 API는 `bool`/`std::optional`과 로그. 비동기 API는 성공/실패 시그널
  (`finished(result)` / `failed(error)`)로 알린다. `QSqlQuery::exec()`의 반환값은 항상 확인한다.
- ui: 에러를 사용자에게 보여 주기만 한다. 복구 로직은 두지 않는다.

## 8. 런타임 경로

`QApplication::setOrganizationName("YamadaStudio")`, `setApplicationName("PokeSix")` 기준.

| 용도 | 위치 (`QStandardPaths`) | Linux 예 |
|---|---|---|
| 설정 | `QSettings` (IniFormat) | `~/.config/YamadaStudio/PokeSix.ini` |
| 게임 데이터 DB | `AppDataLocation/pokesix.sqlite` | `~/.local/share/YamadaStudio/PokeSix/` |
| 받은 CSV 원본 (이어받기용) | `CacheLocation/pokeapi-csv/<커밋>/` | `~/.cache/YamadaStudio/PokeSix/` |
| 스쿼드 | `AppDataLocation/squads.json` | 위와 같음 |
| 스프라이트 캐시 (도입 시) | `CacheLocation/sprites/` | `~/.cache/YamadaStudio/PokeSix/` |

## 9. 확장 여지

- **QML**: `data/state`와 `data/models`는 QtCore만 쓰므로 QML에 그대로 노출할 수 있다.
  QML 화면을 붙일 때는 `ui_qml/` 같은 새 레이어를 `ui`와 나란히 추가한다.
- **에뮬레이터 오버레이**: 별도 창 또는 별도 실행 파일. core의 분석과 data의 state를 재사용하고,
  오버레이 전용 UI만 새로 만든다. 지금은 구조상 막히는 곳이 없게만 유지한다.
