# 01. 상속 계층 — 어떤 Qt 클래스를 받아서 무엇을 만들었나

[README](README.md)의 **① 상속 트리**다. `src/ui` · `src/data/models`의 모든 클래스(70개 — 다른 클래스 안에 숨은 중첩 클래스 5개 포함)를 Qt 기본 클래스 아래에 매달았다.

## 한눈에 보는 결론

1. **우리 클래스끼리는 상속하지 않는다.** 70개 전부가 Qt 클래스를 **바로** 상속한다(가장 깊어도 "Qt → 우리" 한 단).
   재사용은 상속이 아니라 **합성**으로 한다 — 예: `PanelFrame`(겉 테두리) 안에 내용 위젯을 `setBody()`로 끼운다([ADR 0007](../decisions/0007-ui-component-structure.md))
2. **버튼은 `QPushButton`이 아니라 `QAbstractButton`을 상속한다.** 클릭 · 체크 · 키보드 · `QButtonGroup` 같은 "버튼의 동작"은 Qt가 주고,
   모양은 `paintEvent`로 전부 직접 그린다. `QPushButton`은 이미 플랫폼 모양을 그리는 클래스라 덮어쓸 게 많아진다
3. **`QWidget`을 바로 상속한 것은 두 부류다.**
   - **조합형** — 자식 위젯(QLabel · 버튼 · 다른 ★)을 레이아웃에 담는다. 페이지들(`DexPage` · `SquadPage` …)이 여기 속한다
   - **직접 그리는 형** ⟨paint⟩ — 자식 없이 `paintEvent` 하나로 그린다. 히트맵 · 레이더 · 지도 · 슬롯 카드처럼 모양이 복잡하거나
     줄 수가 고정된 것. "자식 위젯 수십 개 + 레이아웃"보다 가볍고, 수치를 디자인 그대로 맞추기 쉽다
4. **`Q_OBJECT`는 시그널 · 슬롯 · `Q_PROPERTY`를 새로 만들 때만 붙인다.** 예: `TabButton`은 `QAbstractButton`의 `clicked`만 쓰므로 없다(moc 불필요)

## 상속 트리

```
QObject                                              ← 모든 것의 뿌리: 객체 트리 · 시그널/슬롯 · 이름
│
├─ QWidget                                           ← 화면에 보이는 모든 것
│  │
│  ├─ QMainWindow
│  │  └─ MainWindow ★                    shell/      메인 창. 인트로 ↔ 본 화면 전환, 단축키
│  │
│  ├─ QDialog                                        ← 따로 뜨는 창(모달)
│  │  ├─ ListPicker ★                    squad/      검색 + 목록에서 하나 고르기(포켓몬 · 기술 · 물건 공용)
│  │  └─ ShuttleDialog ★                 squad/      비전셔틀(7번째 멤버) 창
│  │
│  ├─ QAbstractButton                                ← 버튼의 "동작"만 있는 추상 클래스. 모양은 우리가 그린다
│  │  ├─ ShadowButton ★        ⟨paint⟩  widgets/    단단한 그림자 버튼(Primary 빨강 · Secondary 흰색)
│  │  ├─ DropdownButton ★      ⟨paint⟩  widgets/    작은 "글자 ▾" 버튼(성격 · 특성 · 게임 고르기)
│  │  ├─ GenerationButton ★    ⟨paint⟩  widgets/    "▾ 4세대" — 세대 색 줄무늬 버튼(앱 막대. Large 크기는 정의만 있고 안 쓴다)
│  │  ├─ VersionChip ★         ⟨paint⟩  widgets/    버전 칩 [DP] [Pt] [HGSS] — 칩 안을 버전 색으로 나눠 칠함(Q_OBJECT 없음)
│  │  ├─ MarkButton ★          ⟨paint⟩  shell/      앱 막대 왼쪽 "캡슐 마크 + POKESIX" → 인트로로
│  │  ├─ TabButton ★           ⟨paint⟩  shell/      앱 막대의 폴더 탭 하나 (apptabbar.cpp 안, Q_OBJECT 없음)
│  │  ├─ IntroMenuItem ★       ⟨paint⟩  home/       인트로 메뉴의 카드 버튼 하나
│  │  ├─ DexButton ★           ⟨paint⟩  dex/        도감 선택 버튼 "신오 [D|P]" (dexselector.cpp 안)
│  │  ├─ TypeToggle ★          ⟨paint⟩  dex/        필터의 타입 칩 토글 (dexfilterpanel.cpp 안)
│  │  └─ CategoryButton ★      ⟨paint⟩  items/      아이템 분류 한 줄 "▶ ■ 회복"
│  │
│  ├─ QFrame ─ QAbstractScrollArea ─ QAbstractItemView
│  │                                 └─ QHeaderView                     ← 표의 머리 칸
│  │                                    ├─ DexHeaderView ★   dex/       도감 표 머리 (paintSection으로 직접 그림)
│  │                                    └─ ItemHeaderView ★  items/     아이템 표 머리
│  │
│  └─ (QWidget 바로 상속)
│     │  ── 조합형: 자식 위젯 + 레이아웃 ──────────────────────────────
│     ├─ AppTabBar ★                     shell/      탭 4개를 담는 줄
│     ├─ IntroFooter ★                   home/       인트로 맨 아래 정보 줄(QLabel 조합)
│     ├─ DexPage ★                       dex/        도감 목록 화면 [필터 | 표 | 미리 보기]
│     ├─ DexDetailPage ★                 dex/        도감 상세 화면
│     ├─ DexPreview ★                    dex/        도감 미리 보기 창 내용
│     ├─ DexFilterPanel ★                dex/        도감 필터 창 내용
│     ├─ DexSelector ★                   dex/        도감 선택 버튼 줄
│     ├─ GameSelector ★                  dex/        게임 칩 줄(도감 · 스쿼드 · 타운맵 공용)
│     ├─ DexFilterBar ★                  squad/      포켓몬 선택 창의 도감 칩 줄
│     ├─ SquadPage ★                     squad/      스쿼드 편집 화면
│     ├─ ItemsPage ★                     items/      아이템 백과 화면
│     ├─ ItemDetailPane ★                items/      아이템 상세 창 내용
│     ├─ TownMapPage ★                   map/        타운맵 백과 화면
│     │
│     │  ── 직접 그리는 형 ⟨paint⟩ (자식 있는 것은 따로 표시) ─────────
│     ├─ PanelFrame ★                    widgets/    창의 겉모양(먹선 · 그림자 · 머리 띠). 내용은 setBody()로 — 자식 1개
│     ├─ AppBar ★                        shell/      빨강 앱 막대 바탕 + 자식(마크 버튼 · 탭 줄 · 세대 버튼)
│     ├─ HomePage ★                      home/       인트로 바탕(사선 무늬 · 빨강 띠) + 자식(마크 · 배럴 · 메뉴 …)
│     ├─ IntroMenu ★                     home/       메뉴 카드 버튼들 + 포커스 링(부모가 그림, ADR 0016)
│     ├─ FirstRunPanel ★                 home/       첫 실행 패널(받기 전 · 받는 중 · 실패) + 자식 버튼
│     ├─ CardBarrel ★                    home/       세대 카드 9장 원통
│     ├─ MarkWidget ★                    widgets/    캡슐 마크(SVG)
│     ├─ WordmarkLabel ★                 widgets/    "POKESIX" 워드마크(두 색 + 그림자)
│     ├─ GenerationMenu ★                widgets/    세대 드롭다운 팝업(generationbutton.cpp 안, Q_OBJECT 없음)
│     ├─ SearchField ★                   widgets/    검색 칸(돋보기 · 입력 · 단축키 표시) + 자식 QLineEdit
│     ├─ RangeSlider ★                   widgets/    손잡이 둘 범위 슬라이더
│     ├─ SegmentProgress ★               widgets/    10칸 진행 막대
│     ├─ StatRadar ★                     dex/        종족값 육각 레이더
│     ├─ MoveList ★                      dex/        기술 표(레벨업 · 기술머신)
│     ├─ AbilityList ★                   dex/        특성 목록
│     ├─ EncounterList ★                 dex/        야생 출현 목록
│     ├─ EvolutionView ★                 dex/        진화 트리
│     ├─ MatchupView ★                   dex/        타입 상성표
│     ├─ NaturePicker ★                  dex/        성격 5×5 팝업
│     ├─ ProfileCard ★                   dex/        상세 왼쪽 위 카드(dexdetailpage.cpp 안)
│     ├─ SlotCard ★                      squad/      스쿼드 슬롯 카드 하나
│     ├─ HeatmapView ★                   squad/      방어 상성 히트맵 6 × 18
│     ├─ ProblemList ★                   squad/      문제 목록
│     ├─ SplitBar ★                      squad/      물리 · 특수 분포 막대
│     ├─ SquadPips ★                     squad/      슬롯 핍 6개(squadpage.cpp 안)
│     ├─ Chips ★                         squad/      타입 칩 몇 개(shuttledialog.cpp 안)
│     ├─ HeaderStrip ★                   squad/      선택 창 목록의 칸 제목 띠(listpicker.cpp 안)
│     ├─ PokemonIconGrid ★               items/      포켓몬 아이콘 격자(heightForWidth, Q_OBJECT 없음)
│     ├─ DexPreview::Head ★              dex/        미리 보기 머리(그림 · 이름 · 타입) — 중첩 클래스
│     ├─ DexPreview::WeakRows ★          dex/        미리 보기 약점 줄 — 중첩 클래스
│     ├─ ItemDetailPane::Head ★          items/      아이템 상세 머리 — 중첩 클래스
│     ├─ ItemDetailPane::GenerationCells ★ items/    세대별 존재 1–9 칸 — 중첩 클래스
│     ├─ ItemDetailPane::EvolutionRows ★ items/      진화 아이템 줄 — 중첩 클래스
│     └─ MapView ★                       map/        타운맵 뷰어(줌 · 끌기 · 호버 · 선택)
│
├─ QAbstractItemDelegate ─ QStyledItemDelegate                          ← 표의 "칸 하나를 어떻게 그릴지"
│                          ├─ DexRowDelegate ★    dex/                  도감 표의 칸
│                          ├─ ItemRowDelegate ★   items/                아이템 표의 칸
│                          └─ PickerDelegate ★    squad/                선택 창 줄 바탕(listpicker.cpp 안)
│
├─ QAbstractItemModel                                                   ← 데이터를 표 모양으로 내보내는 쪽 (data 레이어)
│  ├─ QAbstractTableModel
│  │  ├─ SpeciesTableModel ★      data/models/   도감 표의 행 · 열
│  │  └─ ItemTableModel ★         data/models/   아이템 표의 행 · 열
│  └─ QAbstractProxyModel ─ QSortFilterProxyModel
│                           ├─ SpeciesFilterProxy ★  data/models/   도감 필터 · 정렬
│                           └─ ItemFilterProxy ★     data/models/   아이템 필터 · 정렬
│
└─ QObject 바로 상속 (화면에 안 보이는 것)
   └─ RowHover ★                         widgets/    표의 "마우스 올라간 줄" 기억 + 장갑 커서 애니메이션(이벤트 필터)
      (data 레이어: AppState · SquadSession · SquadStore · SpriteCache · DataUpdater · CsvDownloader · ImportWorker —
       UI가 아니므로 이 문서 범위 밖. UI는 이들을 생성자로 받아 쓴다)
```

- 들여 쓴 Qt 중간 클래스(`QFrame ─ QAbstractScrollArea ─ …`)는 Qt 쪽 상속 사슬이다. 우리 클래스가 매달린 곳만 펼쳤다
- `.cpp 안`이라고 적은 클래스는 그 파일의 익명 namespace에 숨은 **내부 부품**이다 — 다른 파일에서 쓸 수 없다
- `A::B` 꼴(중첩 클래스)은 헤더에 이름만 선언(`class Head;`)하고 정의는 `.cpp`에 둔다 — 바깥 클래스만 쓰는 부품을 감추는 방법
- 표의 머리(`QHeaderView`)는 `paintEvent`가 아니라 `paintSection`, delegate는 `paint`를 재정의해서 그린다. 그리는 "자리"만 다르고 생각은 같다

## Qt 기본 위젯을 그대로 쓰는 곳 (상속 없이 `new`)

상속하지 않고 Qt 클래스를 그대로 만들어 쓰는 경우다(2026-10-08, `new Q…` 개수).

| 클래스 | 개수 | 주로 어디 |
|---|---|---|
| `QLabel` | 64 | 거의 모든 페이지의 제목 · 값 · 안내 글자 |
| `QVBoxLayout` · `QHBoxLayout` · `QGridLayout` · `QBoxLayout` | 36 · 32 · 3 · 1 | 모든 조합형 위젯 |
| `QWidget`(그냥 빈 위젯) | 31 | 레이아웃을 담을 **상자** — 여러 위젯을 한 덩어리로 묶어 스택 · 스크롤에 넣을 때 |
| `QScrollArea` | 8 | 도감 상세 · 스쿼드 · 상세 창 — 내용이 창보다 길 때 |
| `QStackedWidget` | 4 | 메인 창의 화면 · 페이지 전환, 장소 패널 탭 등 "한 번에 한 장" |
| `QButtonGroup` | 7 | 탭 · 칩 · 분류 버튼을 "하나만 켜짐"으로 묶기(위젯 아님, QObject) |
| `QPushButton` · `QToolButton` · `QCheckBox` | 7 · 1 · 3 | 기본 모양으로 충분한 버튼(모양은 QSS로) |
| `QLineEdit` | 3 | 검색 입력 · 스쿼드 이름 |
| `QTableView` · `QListView` | 2 · 1 | 도감 · 아이템 표, 선택 창 목록(모델/뷰) |
| `QMenu` | 1 | 스쿼드의 불러오기/내보내기 · 이미지 메뉴 |
| `QShortcut` | 3 | 메인 창의 단축키 |
| `QTimer` · `QVariantAnimation` · `QPropertyAnimation` | 2 · 1 · 1 | 디바운스 · 배럴 회전 · 장갑 애니메이션 |
| `QGraphicsDropShadowEffect` · `QFrame` · `QSvgRenderer` · `QSpacerItem` | 1씩 | 그림자 효과 · 구분선 · SVG 마크 · 빈칸 |
| `QStringListModel` · `QSortFilterProxyModel` | 1 · 1 | 선택 창(ListPicker)의 검색 목록 |

## 클래스를 고를 때의 규칙 (이 리포에서 굳어진 것)

| 만들려는 것 | 상속 | 예 |
|---|---|---|
| 누르는 것(버튼 · 칩 · 탭) | `QAbstractButton` + `paintEvent` | `ShadowButton`, `VersionChip` |
| 모양이 복잡하거나 줄 수가 정해진 표시 | `QWidget` + `paintEvent` | `HeatmapView`, `MoveList` |
| 여러 부품을 묶은 화면 · 창 내용 | `QWidget` + 레이아웃 | `DexPage`, `ItemDetailPane` |
| 줄이 많고 정렬 · 필터가 필요한 표 | `QTableView`(그대로) + 모델 · 프록시 · delegate · header | 도감 · 아이템 목록([09-model-view.md](09-model-view.md)) |
| 따로 뜨는 선택 창 | `QDialog` | `ListPicker` |
| 바깥을 누르면 닫히는 팝업 | `QWidget` + `Qt::Popup` 창 플래그 | `GenerationMenu`, `NaturePicker` |
| 화면 없는 도우미 | `QObject`(+ 이벤트 필터) | `RowHover` |
