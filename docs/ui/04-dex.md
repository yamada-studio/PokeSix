# 04. 도감 — DexPage (목록) · DexDetailPage (상세)

`m_pages`의 0번(`Page::Dex`). 한 페이지 안에 **목록 줄과 상세 화면을 `QStackedWidget`으로 겹쳐** 두고 바꿔 보인다.
파일: `src/ui/dex/*`, 모델은 `src/data/models/species*`

## 1. 페이지 안의 두 장

```
DexPage ★    [V] 여백 (20,16,20,20) — 창이 넓으면 resizeEvent가 좌우 여백을 키워 가운데 1660px로 모은다
└ [S] m_views : QStackedWidget
   ├ 0  m_listRow : QWidget       목록 줄 [필터 | 표 | 미리 보기]
   └ 1  m_detail : DexDetailPage ★ 전체 화면 상세
```

| 전환 | 길 |
|---|---|
| 목록 → 상세 | 표에서 더블클릭 · Enter(`QTableView::activated`), 미리 보기의 [자세히 보기] → `openDetail(index)` → `m_detail->showPokemon(id)` + `setCurrentWidget(m_detail)` |
| 상세 → 목록 | [← 목록] 버튼 · Esc · 그 세대에 없는 포켓몬 → `DexDetailPage::backRequested` → `showList()` |
| 도감 탭을 또 누름 | `MainWindow::open`이 `m_dexPage->showList()` |

## 2. 목록 줄 (m_listRow)

```
m_listRow : QWidget   [H] 여백 0 · 간격 12
┌ filterFrame : PanelFrame ★ ┐ ┌ m_panel : PanelFrame ★  stretch 1 ─────────────────────┐ ┌ previewFrame : PanelFrame ★ ┐
│▓ 필터 (파랑 머리 38) ▓▓▓▓▓ │ │▓ 도감 백과  N마리 ▓▓▓▓▓▓▓▓ [m_selector : DexSelector ★] ▓│ │▓ 미리 보기 (초록 머리) ▓▓▓▓▓│
│ filterBody [V](12,10,12,12)│ │ body [V] (12,8,12,0) · 간격 8                            │ │ previewBody [V](12,10,12,12)│
│  m_filter :                │ │ ┌ toolbar [H] ───────────────────────────────────────┐   │ │  m_preview : DexPreview ★   │
│   DexFilterPanel ★         │ │ │[m_search : SearchField ★] ~stretch~ [m_games :     │   │ │                             │
│                            │ │ │ ≤280×36                     GameSelector ★ 칩]     │   │ │                             │
│                            │ │ └─────────────────────────────────────────────────────┘   │ │                             │
│                            │ │ ┌ m_table : QTableView#dexTable  stretch 1 ──────────┐   │ │                             │
│                            │ │ │ DexHeaderView ★ (높이 30)                           │   │ │                             │
│                            │ │ │ 줄 높이 40, 칸마다 DexRowDelegate ★가 그림          │█  │ │                             │
│                            │ │ └─────────────────────────────────────────────────────┘   │ │                             │
└──── 고정 폭 238 ──────────┘ └──────────────────────────────────────────────────────────┘ └──── 고정 폭 292 ────────────┘
```

- 세 창 모두 `PanelFrame`(겉모양)에 내용 위젯을 `setBody()`로 끼운 것이다. 머리 띠 높이 38 → 몸통은 PanelFrame 여백 (2, 42, 2, 5) 안쪽
- `m_selector : DexSelector`는 **레이아웃 밖**이다. `PanelFrame::setHeaderWidget()`이 `setParent(frame)` 후 머리 띠 오른쪽 끝에 손으로 놓는다(크기가 바뀌면 다시 놓음) → [08-widgets.md](08-widgets.md)
- 가운데 창 제목 "도감 백과 · N마리"는 필터 결과 수가 바뀔 때마다 `updateTitle()`이 다시 쓴다

### 표: 모델 → 프록시 → 뷰

```
Repository ──speciesForGeneration──▶ m_model : SpeciesTableModel ★ ──▶ m_proxy : SpeciesFilterProxy ★ ──▶ m_table : QTableView
                                      (값만, QtCore)                     (필터 · 정렬)                      ├ 머리: DexHeaderView ★
                                                                                                             ├ 칸: DexRowDelegate ★ (모든 칸)
                                                                                                             └ hover: RowHover ★ (이벤트 필터)
```

| 설정 | 값 |
|---|---|
| 선택 | 줄 단위 · 하나만 · 편집 없음 |
| 모양 | 격자 없음 · 줄바꿈 없음 · 테두리 없음, 세로 머리 숨김(줄 높이 40) |
| 스크롤 | 가로 끔 · 세로 **항상**(폭 계산이 흔들리지 않게) |
| 정렬 | 켬, 기본 번호 오름차순 |
| 칸 폭 | 고정: ▶ 24 · 번호 52 · 아이콘 44 · 타입(언어별 계산). 나머지는 `layoutColumns()`가 비율로 나눈다(이름 3 : 능력치 1씩 : 합계 1 + 8) — 표 viewport의 Resize를 **이벤트 필터**로 받아 다시 계산 |

자세한 모델/뷰 설명은 [09-model-view.md](09-model-view.md).

### 필터 창 내용 — DexFilterPanel ★

```
[V] 여백 0 · 간격 8
QLabel#filterCaption "타입"
[G] m_typeGrid  3열, 가로 4 · 세로 5     ← TypeToggle ★ (타입 칩 버튼)이 setTypes() 때 생긴다(세대가 바뀌면 지우고 다시)
·6·
QLabel#filterCaption "종족값 합계"
m_total : RangeSlider ★ (150–800, 10씩)
m_totalLabel : QLabel#filterValue
·6·
m_legendary : QCheckBox "전설 · 환상 제외"
m_finalOnly : QCheckBox "최종 진화만"
~stretch~
m_reset : ShadowButton ★(Secondary) "필터 초기화"
```
무엇이든 바뀌면 시그널 `changed()` 하나 → `DexPage::applyFilters()`가 프록시에 넘긴다.

### 미리 보기 창 내용 — DexPreview ★

```
[V] 여백 0 · 간격 8
m_empty : QLabel#previewEmpty  stretch 1      ← 아무것도 안 골랐을 때만 보임(나머지는 숨김)
m_head : DexPreview::Head  ⟨paint⟩ 높이 218    그림 132 · 번호 · 이름 · 타입 칩
QLabel#filterCaption "종족값"
m_radar : StatRadar ★  가운데 정렬
QLabel#filterCaption "약점 (받을 때)"
m_weak : DexPreview::WeakRows  ⟨paint⟩         ×4 · ×2 타입 칩 줄 (폭에 따라 높이 다시 계산)
~stretch~
m_detail : ShadowButton ★(Primary) "자세히 보기"   → detailRequested()
```
`Head` · `WeakRows`는 `dexpreview.cpp` 안의 중첩 클래스(QWidget, Q_OBJECT 없음)다.

### 목록의 연결

| 시그널 | 받는 곳 |
|---|---|
| `m_selector::dexSelected(id)` | `showDex` — 전국 / 지방 도감 바꾸기 |
| `m_games::versionSelected(v)` | `AppState::setGame(v)` |
| `m_search::searchTextChanged` | `m_searchDelay`(150ms 단발 타이머) 시작 → `timeout` → 프록시 검색어 · 제목 |
| `m_filter::changed` | `applyFilters` |
| `m_table::clicked` · `selectionModel::currentRowChanged` | `showPreview` |
| `m_table::activated` · `m_preview::detailRequested` | `openDetail` |
| `m_sprites::ready` | `m_table->viewport()->update()` — 아이콘이 받아지면 다시 그리기 |
| `AppState::generationChanged` · `gameChanged` · `languageChanged` | 다시 읽기(보일 때 즉시, 아니면 다음 `showEvent`) |

검색은 **디바운스**한다 — 글자를 칠 때마다 필터하지 않고 150ms 멈추면 한 번(ROS 2의 throttle과 같은 목적).

## 3. 상세 화면 — DexDetailPage

```
DexDetailPage ★   [V] 여백 0 · 간격 10
┌ top [H] ────────────────────────────────────────────────────────────────────────────────┐
│ [← 목록] back : ShadowButton(Secondary)  ~stretch~  QLabel#dexDetailBasis "기준 게임" ·6· [m_games : GameSelector] │
└──────────────────────────────────────────────────────────────────────────────────────────┘
┌ m_scroll : QScrollArea#dexDetailScroll  stretch 1 (세로만 스크롤) ─────────────────────────┐
│ content : QWidget#dexDetailContent   [V] 여백 (0,0,4,8) · 간격 16                          │
│ ┌ row [H] 간격 10 — 세 장 모두 높이 340 · 최소 폭 236 · stretch 1:1:1 ─────────────────────┐│
│ │┌ m_profile ─────────┐ ┌ m_statsPanel : PanelFrame(파랑) ┐ ┌ encounters : PanelFrame(초록) ┐││
│ ││ ProfileCard ★      │ │▓ 종족값 · 합계 N ▓ [성격 ▾]    ▓│ │▓ 획득법 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│││
│ ││ ⟨paint⟩ 그림 176 · │ │  m_natureButton : DropdownButton│ │ encounterScroll : QScrollArea │││
│ ││ No. · 이름 · 다른  │ │  (머리 위젯)                    │ │  └ acquisition#dexAcquisition │││
│ ││ 언어 이름 · 분류 · │ │  m_stats : StatRadar ★          │ │     진화 / EvolutionView ★    │││
│ ││ 타입 · 키 · 몸무게 │ │                                 │ │     야생 출현 / EncounterList ★│││
│ │└────────────────────┘ └─────────────────────────────────┘ └───────────────────────────────┘││
│ └────────────────────────────────────────────────────────────────────────────────────────────┘│
│ ┌ abilities : PanelFrame(진파랑) ▓ 특성 ▓           → m_abilities : AbilityList ★              ┐│
│ ┌ matchups  : PanelFrame(빨강)   ▓ 타입 상성 ▓      → m_matchups  : MatchupView ★ (heightForWidth)┐│
│ ┌ level     : PanelFrame(먹색)   ▓ 레벨업으로 익히는 기술 ▓ → m_levelMoves : MoveList ★(LevelUp) ┐│
│ ┌ m_machinePanel (진파랑)        ▓ 기술머신 · 비전머신 N개 ▓ → m_machineMoves : MoveList ★     ┐│
│ ┌ m_tutorPanel (초록, 비면 숨김) ▓ NPC 가르침 기술 ▓ → m_tutorMoves                            ┐│
│ ┌ m_eggPanel   (초록, 비면 숨김) ▓ 알 기술 ▓ → m_eggMoves                                      ┐│
│ ~stretch~                                                                                   │
└──────────────────────────────────────────────────────────────────────────────────────────────┘
* NaturePicker ★ — [성격 ▾]를 누를 때 만드는 Qt::Popup (394×210), 닫히면 스스로 지운다
```

- 모든 섹션은 `section(색, 내용 위젯, 여백)` 도우미 하나로 만든다: `PanelFrame`(머리 34) + 몸통 `QWidget` + `[V]`. 내용이 `Expanding`이면 stretch 1, 아니면 stretch 0 + `addStretch()`
- **목록을 손으로 그리는 이유**: `MoveList` · `AbilityList` · `EncounterList`는 줄 수가 수십 개로 고정이고 스크롤은 상세 화면 전체가 하므로, 모델/뷰 대신 위젯 하나가 `paintEvent`로 그린다(툴팁은 `event()`의 `QEvent::ToolTip`)
- 같은 `DexDetailPage`가 **스쿼드 화면의 모달 창 안에도 하나 더** 만들어진다(`setFollowsAppGame(false)`) → [05-squad.md](05-squad.md)

### 상세 화면의 손그림 위젯

| 위젯 | 크기 정책 | 그리는 것 |
|---|---|---|
| `ProfileCard`(dexdetailpage.cpp 안) | 고정 높이 340 | 패널 · 그림 상자 · 이름들 · 타입 · 키 · 몸무게 |
| `StatRadar` | Expanding, 260×260 | 육각 고리 · 축 · 빨강 도형 · 값, 성격 ▲▼ |
| `EvolutionView` | Expanding/Fixed | 들여 쓴 진화 트리, 보고 있는 종은 노랑. 클릭 → `pokemonClicked(id)` |
| `EncounterList` | Expanding/Fixed | 버전 배지 · 장소 · 방법 · 레벨 · 확률 (폭 < 330이면 확률 칸 생략) |
| `AbilityList` | Expanding/Fixed | 특성 이름 · 숨겨진 특성 꼬리표 · 효과(말줄임 + 툴팁) |
| `MatchupView` | Expanding/Preferred, **heightForWidth** | 배율별 타입 칩 줄 — 폭에 따라 줄바꿈해서 높이가 바뀐다 |
| `MoveList` ×4 | Expanding/Fixed | 28px 머리 + 30px 줄, 모드별 칸(LevelUp · Machine · Plain) |

### 상세의 연결

| 시그널 | 받는 곳 |
|---|---|
| back 클릭 · Esc(`QShortcut`, 이 위젯 안에서만) | `backRequested()` |
| `m_games::versionSelected` | 앱 게임 따라가기면 `AppState::setGame`, 아니면(스쿼드 모달) 자기 `m_version`만 |
| `m_natureButton::clicked` | `showNaturePicker()` → `NaturePicker::natureChosen` → 레이더에 성격 반영 |
| `m_evolution::pokemonClicked(id)` | `showPokemon(id)` — 진화체로 이동 |
| `AppState::generationChanged` · `gameChanged` · `languageChanged` | `reload` · `applyLanguage` |

## 4. 객체 트리

`[L]` 레이아웃 · `[S]` 스택 · `[SA]` QScrollArea::setWidget(부모 = viewport) · `[H]` PanelFrame::setHeaderWidget · `(this)` 명시 · `*` 나중에/다시 만들어짐

```
m_dexPage : DexPage                                [S] m_pages
├ QVBoxLayout
├ m_model : SpeciesTableModel · m_proxy : SpeciesFilterProxy · m_sprites : SpriteCache · m_searchDelay : QTimer   (this)
└ m_views : QStackedWidget                         [L]
   ├ m_listRow : QWidget                           [S]  + QHBoxLayout
   │  ├ filterFrame : PanelFrame                   [L]
   │  │  └ filterBody : QWidget                    [L]
   │  │     └ m_filter : DexFilterPanel            [L]  + QVBoxLayout, QGridLayout
   │  │        ├ QLabel × 2, m_total : RangeSlider, m_totalLabel, m_legendary · m_finalOnly : QCheckBox, m_reset : ShadowButton
   │  │        └ * TypeToggle × 18까지             (setTypes 때 다시)
   │  ├ m_panel : PanelFrame                       [L]
   │  │  ├ m_selector : DexSelector                [H]  + QHBoxLayout, QButtonGroup
   │  │  │  └ * DexButton × n                      (rebuild 때 다시)
   │  │  └ body : QWidget                          [L]  + bodyLayout, toolbar
   │  │     ├ m_search : SearchField               [L]  └ QLabel(돋보기), QLineEdit#searchFieldInput
   │  │     ├ m_games : GameSelector               [L]  └ QButtonGroup, * VersionChip × n
   │  │     └ m_table : QTableView#dexTable        [L]
   │  │        ├ DexHeaderView                     (m_table)
   │  │        ├ m_delegate : DexRowDelegate       (m_table)
   │  │        ├ RowHover                          (m_table)
   │  │        └ (Qt 내부: viewport · 스크롤바 · 세로 머리 · 선택 모델)
   │  └ previewFrame : PanelFrame                  [L]
   │     └ previewBody : QWidget                   [L]
   │        └ m_preview : DexPreview               [L]
   │           ├ m_fronts : SpriteCache            (this)
   │           └ m_empty, m_head(Head), QLabel × 2, m_radar : StatRadar, m_weak(WeakRows), m_detail : ShadowButton   [L]
   └ m_detail : DexDetailPage                      [S]
      ├ m_fronts · m_icons · m_pokemonIcons : SpriteCache, QShortcut(Esc)    (this)
      ├ back : ShadowButton, QLabel#dexDetailBasis, m_games : GameSelector   [L]
      ├ * NaturePicker (Qt::Popup)                 (this, 누를 때마다)
      └ m_scroll : QScrollArea#dexDetailScroll     [L]
         └ viewport └ content#dexDetailContent     [SA]
            ├ m_profile : ProfileCard
            ├ m_statsPanel : PanelFrame ── m_natureButton : DropdownButton [H], body └ m_stats : StatRadar
            ├ encounters : PanelFrame ── body └ encounterScroll : QScrollArea
            │                                    └ viewport └ acquisition#dexAcquisition [SA]
            │                                       └ m_evolutionLabel, m_evolution : EvolutionView, wildLabel, m_encounters : EncounterList
            ├ abilities ── body └ m_abilities : AbilityList
            ├ matchups  ── body └ m_matchups : MatchupView
            ├ level · m_machinePanel · m_tutorPanel · m_eggPanel ── body └ MoveList × 4
            (row · section 몸통 레이아웃은 모두 content / 각 PanelFrame 아래로 접힌다)

주입(소유 X): Repository* · AppState* — Application이 소유
```
