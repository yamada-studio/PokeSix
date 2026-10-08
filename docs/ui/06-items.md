# 06. 아이템 백과 — ItemsPage

`m_pages`의 1번(`Page::Items`). 도감 목록과 같은 "세 창" 구조다 — [분류 | 표 | 상세].
파일: `src/ui/items/*`, 모델은 `src/data/models/item*`

## 1. 배치 그림

```
ItemsPage ★   [H] 여백 (20,16,20,20) · 간격 16
┌ m_categoryPanel : PanelFrame ★ ┐ ┌ m_listPanel : PanelFrame ★  stretch 1 ──────────────────────┐ ┌ m_detailPanel : PanelFrame ★ ┐
│▓ 분류 (초록 머리 38) ▓▓▓▓▓▓▓▓▓ │ │▓ 아이템 대백과 · 진화 · 39개 ▓▓▓ [m_games : DexSelector ★] ▓│ │▓ 상세 (파랑 머리) ▓▓▓▓▓▓▓▓▓▓ │
│ body [V] (10,10,10,0) · 간격 4 │ │ body [V] (12,8,12,0) · 간격 8                                 │ │ detailBody [V] 여백 0        │
│ ┌ CategoryButton ★ h44 ──────┐ │ │ ┌ toolbar [H] ─────────────────────────────────────────────┐  │ │ ┌ detailScroll : QScrollArea ┐│
│ │ ▶ ■ 전체                    │ │ │ │[m_search : SearchField ★ "이름 · 효과 검색"]  ~stretch~  │  │ │ │ #itemDetailScroll (세로만) ││
│ ├ CategoryButton ────────────┤ │ │ └───────────────────────────────────────────────────────────┘  │ │ │ └ detailContent            ││
│ │   ■ 회복                    │ │ │ ┌ m_table : QTableView#itemsTable  stretch 1 ─────────────┐  │ │ │   #itemDetailContent       ││
│ ├ … itemstyle.json 순서 ──────┤ │ │ │ m_header : ItemHeaderView ★ (높이 30)                    │  │ │ │   [V] (12,10,12,12)        ││
│ │                             │ │ │ │ ▶24 │ 아이콘40 │ 이름132 │ 효과(stretch) │ 가격84       │█ │ │ │   └ m_detail :             ││
│ ~stretch~                     │ │ │ │ 줄 40, ItemRowDelegate ★가 그림                          │█ │ │ │     ItemDetailPane ★       ││
│                                │ │ │ └──────────────────────────────────────────────────────────┘  │ │ └────────────────────────────┘│
└──── 고정 폭 220 ──────────────┘ └────────────────────────────────────────────────────────────────┘ └──── 고정 폭 292 ────────────┘
```

- 세 창 모두 `PanelFrame` + `setBody()`. 가운데 창의 게임 칩 `m_games : DexSelector`는 **머리 위젯**(레이아웃 밖)
- 상세 창은 `PanelFrame` → 몸통 → `QScrollArea` → 내용 위젯 → `ItemDetailPane` 순으로 감싼다. 상세가 길어도 창 높이는 그대로이고 안에서 스크롤된다
- 분류 버튼은 `QButtonGroup m_groups`(exclusive)로 묶여 하나만 켜진다

## 2. 표: 모델 → 프록시 → 뷰

```
Repository::itemsForGeneration ──▶ m_model : ItemTableModel ★ ──▶ m_proxy : ItemFilterProxy ★ ──▶ m_table : QTableView#itemsTable
                                                                    필터 순서:                         ├ m_header : ItemHeaderView ★
                                                                    ① 그 세대에 있는가                 ├ m_delegate : ItemRowDelegate ★
                                                                    ② 그 게임에서 얻을 수 있는가       └ RowHover ★
                                                                    ③ 분류(카테고리 · 주머니)
                                                                    ④ 검색어
```

| 설정 | 값 |
|---|---|
| 칸 | ▶ · 아이콘 · 이름 · 효과 · 가격 — 효과만 `Stretch`, 나머지 `Fixed` |
| 정렬 | 켬, 기본 이름 오름차순(가격 칸은 숫자 · 오른쪽 정렬) |
| 스크롤 | 가로 끔 · 세로 항상 |

### 손그림 부품

| 부품 | 그리는 곳 | 내용 |
|---|---|---|
| `ItemHeaderView ★` | `paintSection` | 흰 바탕 · 아래 먹선 2 · 정렬 중인 칸은 빨강 + 삼각형 |
| `ItemRowDelegate ★` | `paint` | 줄 바탕(선택 노랑 · hover · 홀수 줄) · ▶ · 아이콘(없으면 받기 요청 + 원) · 이름 · [타입 칩] 기술 · 효과 · 가격 "%1원" |
| `CategoryButton ★` | `paintEvent` | ▶(켜졌을 때) · 색 견본(itemstyle.json) · 이름(도현 18), 높이 44 |

## 3. 상세 창 내용 — ItemDetailPane

```
m_detail : ItemDetailPane ★   [V] 여백 0 · 간격 8
├ m_empty : QLabel#previewEmpty  stretch 1          ← 아무것도 안 골랐을 때만 (clear())
├ m_head : Head  ⟨paint⟩ 높이 148                    72×72 아이콘 상자 · 이름 · [타입 칩] 기술 · 가격/비매품
├ QLabel#filterCaption "세대별 존재"
├ m_generations : GenerationCells  ⟨paint⟩ 높이 32   1–9세대 칸, 있으면 초록, 지금 세대는 노란 테
├ QLabel#filterCaption "효과" · m_effect : QLabel#itemEffect (줄바꿈)
├ QLabel#filterCaption "진화" · m_extra : EvolutionRows ⟨paint⟩    ← 진화에 쓰이는 아이템일 때만
├ QLabel#filterCaption "입수처" · m_sources : QLabel#itemEffect
├ QLabel#filterCaption · m_learners : PokemonIconGrid ★          ← 기술머신일 때만 (heightForWidth)
└ ~stretch~
```

- `Head` · `GenerationCells` · `EvolutionRows`는 `itemdetailpane.cpp` 안의 중첩 클래스(QWidget, Q_OBJECT 없음)
- 입수처를 배울 수 있는 포켓몬 격자보다 **위에** 둔다 — 격자가 길어질 수 있어서
- `PokemonIconGrid ★`: 폭에 맞춰 열 수가 접히는 아이콘 격자. `hasHeightForWidth() = true` + `heightForWidth(w)`로 "이 폭이면 이 높이"를 레이아웃에 알린다. 이름은 툴팁
- 이 창은 **그리기만** 한다. DB 질의는 `ItemsPage::showDetail()`이 하고 결과를 `setItem(...)`으로 넘긴다

## 4. 연결

| 시그널 | 받는 곳 |
|---|---|
| `CategoryButton::clicked`(각각) | `selectGroup(key)` → 프록시 분류 필터 · 맨 위로 · 제목 |
| `m_games::dexSelected(id)` | 게임 묶음 바꾸기 → `load()` |
| `m_search::searchTextChanged` | 150ms 단발 타이머 → 프록시 검색어 · 제목 |
| `m_table::clicked` · `currentRowChanged` | `showDetail(index)` |
| `m_sprites::ready` | 표 viewport 다시 그리기 |
| `AppState::generationChanged` · `gameChanged` · `languageChanged` | 다시 읽기 · 언어 적용 |

**늦게 읽기**: `ItemsPage`는 생성자에서 데이터를 읽지 않는다. 처음 `showEvent` 때 `load()`한다(세대가 바뀌어도 보일 때만 즉시, 아니면 다음 표시 때).
앱을 켤 때 다섯 페이지가 모두 만들어지므로, 보이지 않는 페이지의 DB 질의를 미루는 것이다.

## 5. 객체 트리

```
ItemsPage                                              [S] m_pages
├ QHBoxLayout
├ m_model : ItemTableModel · m_proxy : ItemFilterProxy · m_sprites : SpriteCache(Item)
│ m_groups : QButtonGroup · m_searchDelay : QTimer      (this)
├ m_categoryPanel : PanelFrame                          [L]
│  └ body : QWidget                                     [L] (setBody)
│     └ CategoryButton × n                              [L]
├ m_listPanel : PanelFrame                              [L]
│  ├ m_games : DexSelector                              [H]  └ QButtonGroup, * DexButton × n
│  └ body : QWidget                                     [L]
│     ├ m_search : SearchField                          [L]
│     └ m_table : QTableView#itemsTable                 [L]
│        ├ m_header : ItemHeaderView · m_delegate : ItemRowDelegate · RowHover   (m_table)
│        └ (Qt 내부)
└ m_detailPanel : PanelFrame                            [L]
   └ detailBody : QWidget                               [L]
      └ detailScroll : QScrollArea#itemDetailScroll     [L]
         └ viewport └ detailContent#itemDetailContent   [SA]
            └ m_detail : ItemDetailPane                 [L]
               ├ m_pokemonIcons : SpriteCache(PokemonIcon)   (this)
               └ m_empty, m_head(Head), QLabel × 5, m_generations(GenerationCells), m_effect, m_extra(EvolutionRows),
                 m_sources, m_learners : PokemonIconGrid     [L]
```
