# 05. 스쿼드 — SquadPage

`m_pages`의 3번(`Page::Squad`). 앱에서 가장 큰 화면(`squadpage.cpp` 약 1700줄)이다.
파일: `src/ui/squad/*`, 상태는 `src/data/state/squadsession.*`

## 1. 상태와 화면의 관계

```
SquadPage ★ (화면)                        SquadSession (data 레이어, QObject)
   │  편집: setPokemon · setMove · moveSlot …  ───────▶  m_squad 바꾸고 commit()
   │                                                         │ 저장소에 넘김(디바운스 저장)
   │  ◀────────────── changed() ───────────────────────────┘
   └▶ refresh(): 카드 6장 · 분석 패널을 세션에서 다시 읽어 그린다
```

화면은 상태를 **들고 있지 않는다**. 버튼은 세션의 함수를 부르기만 하고, 그림은 `changed()`를 받을 때마다 세션에서 다시 읽는다.
ROS 2로 치면 세션이 상태를 가진 노드, 화면은 그 토픽을 구독해서 그리는 뷰어다. 되돌리기(undo)도 세션 안에서 끝난다.

SquadPage가 **직접 만드는 비위젯 객체**(부모 `this`):

| 멤버 | 클래스 | 역할 |
|---|---|---|
| `m_store` | `SquadStore` | `squads.json` 디바운스 저장 |
| `m_session` | `SquadSession` | 편집 창구 · 되돌리기 · 분석 |
| `m_pokemonIcons` · `m_itemIcons` | `SpriteCache` | 아이콘 받기(카드 · 선택 창이 포인터로 빌려 씀) |

주입(소유 X): `Repository*` · `AppState*`.

## 2. 배치 그림 — 넓은 배치 (페이지 폭 ≥ 1240)

```
SquadPage ★   [V] 여백 (20,16,20,12) · 간격 14
┌ m_topBar : QWidget  [H] 간격 12 (오른쪽 여백 = 스크롤바 폭 + 4, 아래 설명) ──────────────────────────────────────┐
│[m_name QLineEdit#squadName][m_rule #squadRulePill "4세대 규칙"][m_game GameSelector ★][m_pips SquadPips ★ ■■■■■□]   │
│[m_count #squadCount "5 / 6"][m_shuttleButton 비전셔틀][↶][↷][⟲] ~stretch~ [불러오기][내보내기][이미지 ▾]·4·[✓ 자동 저장됨]│
└─────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
┌ m_scroll : QScrollArea#squadScroll  stretch 1 (세로만) ─────────────────────────────────────────────────────────────┐
│ content#squadContent   m_columns : QBoxLayout(LeftToRight) 여백 (0,0,4,4) · 간격 16                                │
│ ┌ m_cardArea : QWidget  stretch 1 · 최소 폭 612 · 위 정렬 ┐ ┌ m_analysisColumn : QWidget  stretch 1 ───────────────┐│
│ │ [G] m_grid  2열 × 3행 · 간격 6                          │ │ [V] 여백 (0,3,0,3) ← 카드의 링 자리만큼 맞춤          ││
│ │ ┌ SlotCard ★ 0 ┐ ┌ SlotCard ★ 1 ┐                     │ │ ┌ m_analysis : PanelFrame ★ (파랑 머리 38) ─────────┐ ││
│ │ └──────────────┘ └──────────────┘                     │ │ │▓ 스쿼드 분석 · 4세대 상성표 · 17타입 ▓ [m_problemPill]│ ││
│ │ ┌ SlotCard ★ 2 ┐ ┌ SlotCard ★ 3 ┐                     │ │ │ m_analysisScroll : QScrollArea (안에서 스크롤)    │ ││
│ │ └──────────────┘ └──────────────┘                     │ │ │  frameBody [V] (14,10,14,12)                      │ ││
│ │ ┌ SlotCard ★ 4 ┐ ┌ SlotCard ★ 5 ┐                     │ │ │  m_emptyAnalysis 또는 m_analysisBody (하나만 보임)│ ││
│ │ └──────────────┘ └──────────────┘                     │ │ │   "방어 상성 히트맵"  ~~~  설명                   │ ││
│ │ 카드 높이 = (viewport − 4 − 2×6) / 3, 최소 234         │ │ │   m_heatmap : HeatmapView ★ (높이 300)           │ ││
│ └─────────────────────────────────────────────────────────┘ │ │   "문제 항목" 13개 ~~~ [크게 보기 ↗]               │ ││
│                                                             │ │   m_problemScroll → m_problems : ProblemList ★    │ ││
│                                                             │ │   "물리 · 특수 분포" ~~~ "기술 x / y"              │ ││
│                                                             │ │   m_split : SplitBar ★ (높이 58)                  │ ││
│                                                             │ │   m_resourceBox: "리소스 투자" ~~~ 합계 / 본문     │ ││
│                                                             │ └───────────────────────────────────────────────────┘ ││
│                                                             └───────────────────────────────────────────────────────┘│
└────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

### 위쪽 막대 (m_topBar) 항목 순서

| # | 멤버 | 클래스 | objectName · 비고 |
|---|---|---|---|
| 0 | `m_name` | QLineEdit | `squadName`, 최대 24자, 글자 폭에 맞춰 고정 폭을 다시 정함 |
| 1 | `m_rule` | QLabel | `squadRulePill` "N세대 규칙" + 규칙 툴팁 |
| 2 | `m_game` | GameSelector ★ | 버전 조각 칩. 게임이 하나뿐인 세대면 숨김 |
| 3 | `m_pips` | SquadPips ★ | 96×24, 슬롯마다 첫 타입 색 네모 (squadpage.cpp 안) |
| 4 | `m_count` | QLabel | `squadCount` "n / 6" |
| 5 | `m_shuttleButton` | QPushButton | `squadShuttleButton`, 아이콘을 글자 뒤로(RightToLeft). 비전머신 있는 세대만 보임 |
| 6–8 | `m_undoButton` · `m_redoButton` · `m_clearButton` | QToolButton | `squadIconButton`, SVG 아이콘 + 꺼짐용 그림 따로 |
| 9 | `addStretch()` | | |
| 10–12 | 불러오기 · 내보내기 · 이미지 ▾ | QPushButton | `squadToolButton`. 이미지 버튼엔 `QMenu`(클립보드로 복사 · PNG로 저장)를 미리 달아 둔다 |
| 13 | `addSpacing(4)` | | |
| 14 | `m_saveStatus` | QLabel | `squadSaveStatus` + 동적 속성 `state`("" · "pending" · "error") |

**오른쪽 끝 맞추기**: 위쪽 막대는 스크롤 영역 밖, 분석 패널은 안에 있어서 스크롤바 폭만큼 끝이 어긋난다. 그래서 `m_scroll->viewport()`에 **이벤트 필터**를 걸고, Resize 때마다 막대 레이아웃의 오른쪽 여백을 `스크롤 전체 폭 − viewport 폭 + 4`로 맞춘다. 같은 필터가 카드 높이도 다시 맞춘다(`fitCardHeights`).

## 3. 좁은 배치 (페이지 폭 < 1240)

```
SquadPage
├ m_topBar (같음)
└ m_scroll  ← 페이지 전체가 세로로 스크롤
   └ content   m_columns : QBoxLayout(TopToBottom) 간격 16
      ├ m_cardArea  [G] 3열 × 2행, stretch 0
      │   [카드 0][카드 1][카드 2]
      │   [카드 3][카드 4][카드 5]      카드 높이 = (viewport − 4 − 6) / 2, 최소 234 → 두 줄이 화면을 채움
      └ m_analysisColumn  stretch 1, 위 정렬
          └ m_analysis — 안쪽 스크롤 끔, 최소 높이 = 내용 전체 높이 → 스크롤하면 카드 아래에 이어서 보임
```

`resizeEvent`가 `width() >= kWideWidth(1240)`가 **바뀔 때만** `placeCards(wide)`를 부른다. 바뀌는 것:

| | 넓은 배치 | 좁은 배치 |
|---|---|---|
| `m_columns` 방향 | `LeftToRight` | `TopToBottom` — **`QBoxLayout` 하나의 방향만 바꿔서** 가로 ↔ 세로 |
| 격자 | 2열 × 3행 | 3열 × 2행 (`addWidget(card, slot/3, slot%3)`로 다시 배치) |
| 카드 영역 : 분석 stretch | 1 : 1 | 0 : 1 |
| 카드 영역 최소 폭 | 612 | 0 |
| 분석 패널 안쪽 스크롤 | 필요할 때 | 끔(페이지 스크롤에 맡김) |
| 문제 목록 높이 | 최소 2줄(98) ~ 내용 | 최대 4줄(202)로 고정 |

`QBoxLayout`은 `QVBoxLayout` · `QHBoxLayout`의 부모 클래스다. 방향을 실행 중에 바꿀 수 있어서 반응형 배치에 그대로 쓴다.

## 4. SlotCard ★ — 카드 한 장

`QWidget` ⟨paint⟩. **자식 위젯은 메모 칸 하나**(`m_memo : QLineEdit`, `role="memo"`)뿐이고 나머지는 전부 그린다.
메모 칸도 레이아웃 없이 `resizeEvent`에서 `setGeometry(areas().memo)`로 손으로 놓는다.

```
┌ 위젯 전체 (kRing = 3 여백: 선택 노랑 · 경고 빨강 링이 그려질 자리) ─────────────────────────┐
│ ┌ g.card  paintPanel(먹선 2 · 반경 8 · 그림자 3) ─────────────────────────────────────────┐ │
│ │┌ g.header 34px, 첫 타입 색 ─────────────────────────────────────────────────────────────┐│ │
│ ││ [▶] 01 [아이콘 36] 이름(도현 19, 말줄임) ……… (!) [⇄ 바꾸기][🗑 빼기][+ 자세히]          ││ │
│ │└────────────────────────────────────────────────────────────────────────────────────────┘│ │
│ │  g.types    [타입][타입]                                         높이 20                  │ │
│ │  g.memo     [ m_memo : QLineEdit — 유일한 자식 위젯 ]            높이 26                  │ │
│ │  g.traits   [특성 …… ▾] [성격 …… ▾] [🎒 물건 …… ▾]               높이 24, 세대에 있는 것만 │ │
│ │  g.moves    [■ 기술0 … Lv.N 물] [■ 기술1 … TM13 특]              높이 28                  │ │
│ │             [■ 기술2 …  ♥  변] [ + 기술 추가 (점선) ]                                      │ │
│ │  g.weak     약점 [얼×4][드][…]                                    높이 20                  │ │
│ └──────────────────────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────────────────────┘
빈 슬롯: 점선 상자 · "+" 원 · "NN 빈 슬롯" · 빨강 [+ 개체 추가] 버튼 · 추천 문구
```

- 크기: Expanding/Fixed, 기본 높이 258 · 최소 폭 262. 실제 높이는 `SquadPage::fitCardHeights()`가 `setCardHeight()`로 정한다(최소 234). 높이가 줄면 행 사이 간격이 비율대로 줄어든다
- `areas()` 하나가 모든 영역의 사각형을 계산한다. 그리기(`paintEvent`) · 클릭 판정(`hitAt`) · 툴팁(`event`)이 **같은 계산**을 쓰므로 어긋나지 않는다
- **카드는 아무것도 열지 않는다.** 눌린 곳을 시그널로 알리기만 한다:

| 시그널 | SquadPage가 하는 일 |
|---|---|
| `selectRequested` | 카드 선택(히트맵 행과 같이 강조) |
| `addRequested` · `replaceRequested` | `pickPokemon` — 포켓몬 선택 창 |
| `removeRequested` | 세션에서 비우기 |
| `detailRequested` | `showMemberDetail` — 도감 상세 모달 |
| `moveRequested(slot, i)` | `pickMove` — 기술 선택 창 |
| `abilityRequested` · `natureRequested` · `itemRequested`(전역 위치) | 특성 메뉴 · 성격 팝업 · 물건 선택 창 |
| `dragStarted` · `dragMoved` · `dragFinished` | 카드 끌어서 순서 바꾸기(아래) |

### 카드 끌기 — QDrag 없이 손으로

```
머리 띠를 누르고 startDragDistance 이상 움직이면  dragStarted
  SquadPage::onDragStarted   격자 끄기(m_grid->setEnabled(false)) · 카드 raise() · QGraphicsDropShadowEffect 붙이기
dragMoved
  onDragMoved               카드를 move()로 따라오게, 가장 가까운 칸 찾기,
                            다른 카드들은 QPropertyAnimation("pos", 180ms)으로 밀려남, 위아래 끝이면 자동 스크롤
dragFinished
  onDragFinished            제자리로 140ms 미끄러지기 → 끝나면 finishDrag
  finishDrag                그림자 효과 제거 · m_session->moveSlot(from, to) · 격자 다시 켜기
```

카드 위젯은 끝까지 "n번 자리"에 묶여 있다. 순서를 바꾸는 건 세션의 데이터이고, 바뀐 데이터를 카드들이 다시 그린다.

## 5. 분석 패널의 손그림 위젯

| 위젯 | 크기 | 그리는 것 · 상호작용 |
|---|---|---|
| `HeatmapView ★` | Expanding/Fixed, 높이 300 | 머리(18타입) · 6행(배율 칸) · 요약 3행 · 문제 열 빨강 테 · 범례. 이름 칸 클릭 → `slotClicked`. 문제 강조 깜빡임은 값 멤버 `QTimer m_flash` |
| `ProblemList ★` | Expanding/Fixed, 줄당 46 + 6 | 빨강 줄 · 타입 상자 · [공격]/[방어] 꼬리표. hover → `rowHovered`(카드에 빨강 링 · 히트맵 열 강조), 클릭 → `rowClicked`(히트맵으로 스크롤 + 깜빡임) |
| `SplitBar ★` | Expanding/Fixed, 높이 58 | 물리 · 특수 · 변화 · 빈칸(사선) 누적 막대 + 범례 |

빈 분석(`m_emptyAnalysis`)과 내용(`m_analysisBody`)은 `QStackedWidget`이 아니라 **둘 중 하나를 숨기는** 방식이다 — 스택은 큰 장에 높이를 맞춰서, 빈 상태에서도 패널이 길어지기 때문이다(코드 주석).

## 6. 창 · 메뉴 · 팝업 (모두 필요할 때 만든다)

| 여는 곳 | 객체 | 방식 |
|---|---|---|
| [+ 개체 추가] · ⇄ | `ListPicker`(스택 변수) + 머리에 `DexFilterBar ★` | `exec()` 모달 |
| 기술 칸 | `ListPicker`(줄 높이 36) | `exec()` |
| 물건 칸 | `ListPicker`(줄 높이 40) | `exec()` |
| 특성 칸 | `QMenu`(스택 변수, 체크 액션) | `menu.exec(위치)` |
| 성격 칸 | `NaturePicker ★`(`new`, Qt::Popup, 닫히면 스스로 지움) | `show()` — 모달 아님 |
| + 자세히 | `QDialog` 안에 `DexDetailPage ★` | `exec()`, 화면의 94%까지 |
| 크게 보기 ↗ | `QDialog` 안에 `ProblemList ★` 하나 더 | `exec()`, 820×640. 열려 있는 동안 분석이 바뀌면 같이 갱신 |
| 비전셔틀 | `ShuttleDialog ★` | `exec()`. 안에서 포켓몬 고르기 → `ListPicker`가 하나 더 뜬다 |
| 불러오기 · 내보내기 · PNG 저장 | `QFileDialog::get…FileName`(정적) | 모달 |
| 오류 · 덮어쓰기 확인 | `QMessageBox::warning` · `question`(정적) | 모달 |

**스택 변수 + `exec()`** 패턴: `ListPicker picker(…, this); if (picker.exec() == QDialog::Accepted) …` — `exec()`가 자기만의 이벤트 루프를 돌며 창이 닫힐 때까지 기다리고, 함수가 끝나면 스택에서 저절로 사라진다. 부모를 `this`로 주는 이유는 창 위치(부모 위 가운데)와 모달 범위 때문이다.

### ListPicker ★ — 공용 선택 창 (QDialog, 600×620)

```
ListPicker  [V] 여백 (16,14,16,14) · 간격 10
├ m_headingRow [H]: m_heading QLabel#dexSectionLabel  ~stretch~  [머리 위젯 — 포켓몬이면 DexFilterBar ★]
├ m_search : SearchField ★  (↑↓ Enter를 이벤트 필터로 목록에 넘김)
├ m_header : HeaderStrip ★  (칸 제목 띠, showHeader() 때만 보임)
├ m_view : QListView#squadPickerList  stretch 1
│    모델: m_model QStringListModel(검색용 문자열만) → m_proxy QSortFilterProxyModel(대소문자 무시)
│    delegate: PickerDelegate ★ (줄 바탕만 칠하고 내용은 호출한 쪽의 painter 함수)
│    hover: RowHover ★
└ [H]: none ShadowButton("기술 비우기" 등, 필요할 때만) ~stretch~ close ShadowButton "닫기"
```
모델에는 **검색할 글자만** 넣고, 줄을 그리는 건 호출한 쪽이 넘긴 `std::function` painter가 한다. 그래서 포켓몬 · 기술 · 물건이 같은 창을 쓴다.

### ShuttleDialog ★ — 비전셔틀 (QDialog, 최소 폭 640)

```
ShuttleDialog  [V] m_layout 여백 (20,16,20,16) · 간격 10
└ m_body : QWidget   ← 세션이 바뀔 때마다 통째로 지우고 다시 만든다(rebuild)
   ├ head [H] 간격 12: [pick QPushButton#shuttlePickButton (아이콘 36 + 이름)] [Chips ★] [비우기] ~stretch~ [기술 n / 4]
   ├ table : QFrame#shuttleTable  [G] 여백 (16,12,16,12), 4번 열 stretch
   │    머리: "비전머신"(0–2열) │ "채용"(3–4열)
   │    HM마다: 번호 · 이름 · Chips(타입) · [☐ 셔틀 QCheckBox] 또는 "배울 수 없어요" · 누가 드는지(아이콘 + 이름 / 상태 글자)
   └ note QLabel#squadNote
(확인 버튼 없음 — 창 닫기 · Esc)
```
세션 `changed`와 아이콘 `ready`를 **`Qt::QueuedConnection`**으로 받는다. 체크박스를 누른 그 시그널 처리 도중에 그 체크박스를 지우면 안 되므로, 다시 만드는 일을 이벤트 루프 다음 차례로 미룬다.

## 7. 연결 요약

| 시그널 | 받는 곳 |
|---|---|
| `m_session::changed` · `AppState::languageChanged` | `refresh()` → 카드 6장 · `refreshAnalysis()` · `refreshResources()` |
| `m_store::saveScheduled` / `saved(ok)` | 저장 상태 글자 + `state` 속성 + `style()->polish()` |
| `m_name::editingFinished` | `m_session->setName` |
| `m_game::versionSelected` | `m_session->setVersion` |
| undo · redo · clear | `SquadSession::undo` · `redo` · `clearAll` |
| `m_heatmap::slotClicked` | `selectSlot` |
| `m_problems::rowHovered` · `rowClicked` | 카드 경고 링 · 히트맵 강조 / 스크롤 + 깜빡임 |
| `DataUpdater::finished`(MainWindow에서) | `reloadData()` |

## 8. 객체 트리

```
squad : SquadPage                                       [S] m_pages
├ QVBoxLayout
├ m_store : SquadStore · m_session : SquadSession · m_pokemonIcons · m_itemIcons : SpriteCache   (this)
├ m_topBar : QWidget                                    [L]  + QHBoxLayout
│  ├ m_name : QLineEdit · m_rule : QLabel · m_count : QLabel · m_saveStatus : QLabel
│  ├ m_game : GameSelector  └ QButtonGroup, * VersionChip × n
│  ├ m_pips : SquadPips
│  ├ m_shuttleButton : QPushButton · 불러오기 · 내보내기 : QPushButton
│  ├ m_undoButton · m_redoButton · m_clearButton : QToolButton
│  └ image : QPushButton  └ imageMenu : QMenu           (image)
├ m_scroll : QScrollArea#squadScroll                    [L]
│  └ viewport  (SquadPage가 이벤트 필터)
│     └ content#squadContent                            [SA]  + m_columns : QBoxLayout
│        ├ m_cardArea : QWidget                         [L]   + m_grid : QGridLayout
│        │  └ SlotCard × 6                              [L]
│        │     ├ m_memo : QLineEdit                     (this — 레이아웃 없이 손으로 배치)
│        │     └ * QGraphicsDropShadowEffect            (끄는 동안만)
│        └ m_analysisColumn : QWidget                   [L]
│           └ m_analysis : PanelFrame                   [L]
│              ├ m_problemPill : QLabel#squadProblemPill [H]
│              └ m_analysisScroll : QScrollArea         [L] (setBody)
│                 └ viewport └ frameBody : QWidget      [SA]
│                    ├ m_emptyAnalysis : QLabel#squadEmpty
│                    └ m_analysisBody : QWidget
│                       ├ QLabel × n (제목 · 설명 · 개수), expand : QPushButton#squadLinkButton
│                       ├ m_heatmap : HeatmapView
│                       ├ m_problemScroll : QScrollArea └ viewport └ m_problems : ProblemList [SA]
│                       ├ m_split : SplitBar
│                       └ m_resourceBox : QWidget └ QLabel × 3
├ * QPropertyAnimation × 최대 6 (m_slides)              (this) 카드 미끄러지기, 처음 쓸 때 만들어 재사용
└ * 창 · 메뉴 · 팝업 (§6) — 대부분 스택 변수라 exec()가 끝나면 사라진다
```
