# 08. 공용 컴포넌트 카탈로그

여러 페이지가 같이 쓰는 부품이다. 대부분 `src/ui/widgets/`에 있고, 몇 개는 처음 쓴 페이지 폴더에 있다(표의 "위치").

## 한눈에

| 부품 | 상속 | 위치 | 자식 | 쓰는 곳 |
|---|---|---|---|---|
| `PanelFrame` | QWidget | widgets | 몸통 1 + 머리 위젯 | 도감 · 아이템 · 타운맵 · 스쿼드의 거의 모든 창 |
| `paintPanel` · `PanelStyle` · `chromeMargins` | 함수 · 구조체 | widgets/panelpainter | — | PanelFrame · SlotCard · 상세 카드 · 인트로 카드 · 첫 실행 패널 |
| `ShadowButton` | QAbstractButton | widgets | 없음 ⟨paint⟩ | 미리 보기 · 상세 · 필터 · 선택 창 · 첫 실행 |
| `GenerationButton` (+ `GenerationMenu`) | QAbstractButton (+ QWidget 팝업) | widgets | 없음 ⟨paint⟩ | 앱 막대 |
| `VersionChip` | QAbstractButton | widgets | 없음 ⟨paint⟩ | `GameSelector` · `DexFilterBar` 안 |
| `GameSelector` | QWidget | dex | VersionChip × n | 도감 목록 · 도감 상세 · 스쿼드 · 타운맵 |
| `DexSelector` (+ `DexButton`) | QWidget | dex | DexButton × n | 도감 · 아이템 창 머리 |
| `DropdownButton` | QAbstractButton | widgets | 없음 ⟨paint⟩ | 도감 상세 [성격 ▾] |
| `SearchField` | QWidget | widgets | QLabel + QLineEdit | 도감 · 아이템 · 선택 창 |
| `RangeSlider` | QWidget | widgets | 없음 ⟨paint⟩ | 도감 필터 |
| `SegmentProgress` | QWidget | widgets | 없음 ⟨paint⟩ | 첫 실행 패널 |
| `RowHover` | **QObject** | widgets | — | 도감 · 아이템 표, 선택 창 목록 |
| `MarkWidget` · `WordmarkLabel` | QWidget | widgets | (QSvgRenderer) ⟨paint⟩ | 인트로 |
| `typechip`(네임스페이스) | 그리기 함수 | widgets | — | 14개 파일 |
| `spritefit` · `svgicon` | 함수 | widgets | — | 그림 맞추기 · 인라인 SVG → 아이콘 |

## PanelFrame — "창의 겉모양"

```
┌──[먹선 2 · 반경 8]──────────────────────────────────────────────┐
│▓ 제목(도현 20 흰색) 부가 정보(코딩체 13)     [머리 위젯 ──────]▓│  ← 머리 띠(style.header px, style.headerColor)
│══════════════════════════════════════════════════════════════════│  ← 머리 선 2
│  [V] m_layout  여백 = chromeMargins(style) · 간격 0              │
│      [ 몸통 위젯 (setBody) ]                                      │
└──────────────────────────────────────────────────────────────────┘
  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ 단단한 그림자(style.shadow px, 블러 없음)
```

| API | 하는 일 |
|---|---|
| `setBody(QWidget*)` | `m_layout->addWidget(body)` — 몸통의 부모가 PanelFrame이 된다 |
| `setHeaderWidget(QWidget*)` | `setParent(this)` + `show()` — **레이아웃에 넣지 않고** 머리 띠 오른쪽 끝(먹선 + 8 안쪽, 세로 가운데)에 `sizeHint` 크기로 손으로 놓는다. `resizeEvent`와 `QEvent::LayoutRequest`(머리 위젯 크기가 바뀜)에서 다시 놓는다 |
| `setTitle(제목, 부가)` | 머리 띠 글자(그리기) |
| `setPanelStyle(PanelStyle)` | 색 · 머리 높이 · 그림자 — 여백(`chromeMargins`)도 다시 계산 |

**여백 공식** `chromeMargins(style)`: 옆 = 먹선(+ 안쪽 테), 위 = 옆 + 머리 + 머리 선 2, 아래 = 옆 + 그림자.
도감 목록 창(머리 38 · 그림자 3)이면 **(2, 42, 2, 5)**, 도감 상세 섹션(머리 34)이면 (2, 38, 2, 5).
→ 몸통 위젯은 자기가 창 안에 있다는 걸 모른다. 같은 몸통을 나중에 모달 · 드로어에 넣을 수 있다([ADR 0007](../decisions/0007-ui-component-structure.md)).

**왜 머리 위젯을 레이아웃 밖에 두나**: 머리 띠는 레이아웃 여백(위 42) 안쪽이 아니라 **여백 자리 자체**에 그려진다. 레이아웃은 여백 안쪽만 배치하므로, 그 자리에 위젯을 놓으려면 손으로 `setGeometry` 해야 한다.

## 버튼 계열 — 모두 QAbstractButton + paintEvent

```
ShadowButton                    GenerationButton(Compact)          VersionChip                 DropdownButton
 ╭────────────────╮             ╭───────────╮                     ╭──┬──┬────────╮            ╭────────╮
 │ [ic] 데이터 받기 │ 40/36      │ ▾ 4세대   │ 36 (세대 색 줄무늬)  │HG│SS│ 외딴섬 │ 26          │ 성격 ▾ │ 24
 ╰────────────────╯             ╰───────────╯                     ╰──┴──┴────────╯            ╰────────╯
  ▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀▀ 그림자 3       ▀▀▀▀▀▀▀▀▀▀▀▀▀ 그림자 2             ▀▀▀▀▀▀▀▀▀▀▀▀▀▀ (켜졌을 때만)
  눌리면 2px 내려가고 그림자 사라짐
```

| 버튼 | 특징 |
|---|---|
| `ShadowButton` | Primary(빨강 · 흰 글자 · 40) / Secondary(흰색 · 먹색 · 36). 먹선 2 · 반경 6. Enter도 클릭(`animateClick`). 크기 정책 Preferred × **Fixed** |
| `GenerationButton` | "▾ N세대" — 바탕은 그 세대 게임들의 버전 색을 **단색 띠로** 나열(`QLinearGradient`에 같은 색을 두 번 찍어 경계를 딱 끊는다). Large · Compact 두 크기(지금은 Compact만 쓴다). 누르면 `GenerationMenu`를 띄운다 |
| `GenerationMenu` | `QWidget(parent, Qt::Popup | Qt::FramelessWindowHint)` + `WA_DeleteOnClose` — 바깥을 누르면 닫히고 스스로 지워진다. 폭 = 버튼 폭, 줄 34 · 줄마다 세대 색 띠 · 현재 세대에 ✓ · 마지막 줄 빼고 1px 먹선. **Q_OBJECT 없이** 고르면 부를 `std::function` 콜백을 받는다. ↑↓ Enter Esc |
| `VersionChip` | 칩 하나를 버전 수만큼 "조각"으로 나눠 각 버전 색(dexstyle.json)으로 칠한다. 조각마다 누를 수 있다(`partAt` · `pressedPart`). 켜지면 먹선 2 + 그림자, 꺼지면 흐리게. **Q_OBJECT 없음** |
| `DropdownButton` | 작은 "글자 ▾", 먹선 1.5 · 반경 5, hover 노랑 |

`QPushButton`을 쓰지 않는 이유는 [01-class-hierarchy.md](01-class-hierarchy.md) — 그리고 앱 전체 QSS의 `QWidget { font … }` 규칙이 `setFont()`를 덮어써서, 직접 그리는 버튼은 글꼴을 멤버(`m_font`)로 따로 들고 `painter.setFont(m_font)`로 쓴다.

## 칩 줄 — GameSelector · DexSelector · DexFilterBar

```
GameSelector                     [DP] [Pt] [HG|SS]        [H] 간격 5 + QButtonGroup(exclusive) of VersionChip
DexSelector (도감 · 아이템 머리)  [전국][신오 D|P][성도 HG|SS]  [H] 간격 6 + QButtonGroup of DexButton(26 높이)
DexFilterBar (포켓몬 선택 창)     [전국][DP][Pt][HGSS]      [H] 간격 5 + QButtonGroup of VersionChip
```

- 버튼은 **세대 · 게임이 바뀔 때마다 지우고 다시 만든다**(`setGames` · `setDexes` · `rebuild`). 같은 목록이 다시 오면 다시 만들지 않고 체크만 바꾼다(`GameSelector`)
- 지울 때 `deleteLater()` — 지금 처리 중인 클릭 시그널의 주인일 수 있어서, 이벤트 루프의 다음 차례에 지운다
- `QButtonGroup::idClicked`는 사용자 클릭에만 나온다 → `versionSelected(QString)` · `dexSelected(int)`. 코드가 체크를 바꿀 때는 시그널이 나오지 않는다
- `GameSelector::setSplitVersions(true)`면 버전마다 고를 수 있다(도감 · 스쿼드), `false`면 묶음 칩(타운맵)

## SearchField — 조합형

```
╭──────────────────────────────────────╮
│ 🔍  이름 · 번호로 찾기        [Ctrl K] │  높이 36, 먹선 2 (입력에 포커스면 파랑), 반경 6
╰──────────────────────────────────────╯
[H] 여백 (12,0,12,0) · 간격 8:  QLabel(돋보기 16) · m_edit : QLineEdit#searchFieldInput (stretch 1) · [m_shortcut]
```
- 상자는 `paintEvent`가 그리고, 입력 칸은 QSS로 테두리 · 배경을 없앤 진짜 `QLineEdit`이다(조합형 + 그리기형의 혼합)
- `m_edit`에 **이벤트 필터**: 포커스가 들고 날 때 다시 그려 테두리 색을 바꾸고, 한글 입력기의 **조합 중 글자**(preedit)를 잡아 `searchTextChanged`로 낸다 — 그래서 "피카"를 치는 도중에도 목록이 걸러진다
- 폭 280(최소 160, 최대 280), 220보다 좁으면 단축키 표시를 숨긴다. 지금은 아무도 단축키 글자를 넘기지 않아 표시가 안 쓰인다

## RowHover — 화면 없는 도우미 (QObject + 이벤트 필터)

```
RowHover(view)      부모 = 표 뷰. 생성자에서 view->viewport()->installEventFilter(this)
  MouseMove  → 그 줄 번호 기억, viewport 다시 그리기, 손가락 커서
  누름        → 장갑 "쥐기" 커서 애니메이션 (110ms × 4프레임, QTimer)
  Leave      → 줄 −1
delegate가 hover->row()를 읽어 줄 전체를 hover 색으로 칠한다
```
`QTableView`는 hover 상태를 **칸 하나**에만 준다. 줄 전체를 칠하려면 마우스 줄을 따로 기억해야 해서 만든 부품이다.
상속이 아니라 **이벤트 필터로 남의 위젯에 기능을 붙이는** Qt 패턴의 예다(ROS 2의 데코레이터 노드처럼, 원래 객체를 고치지 않는다).

## 그 밖의 손그림 위젯

| 위젯 | 크기 | 그리는 것 |
|---|---|---|
| `RangeSlider` | 160×24 | 4px 트랙 · 고른 구간 노랑 · 14px 네모 손잡이 둘(흰 바탕 · 먹선 · 그림자). 마우스로 가까운 손잡이를 잡는다. 시그널 `valuesChanged(lo, hi)` |
| `SegmentProgress` | 200×22, Expanding × Fixed | 10칸, 찬 칸 초록 · 빈 칸 연회색. `setValue(%)` |
| `MarkWidget` | n × n | `QSvgRenderer`로 SVG를 `rect()`에 |
| `WordmarkLabel` | 글자 폭 + 10 × 85 | "POKE" + "SIX"(빨강) + 노란 오프셋 그림자 |

## 그리기 도우미 (위젯 아님)

| 이름 | 하는 일 |
|---|---|
| `typechip::paint(painter, 위치, 타입, 언어)` | 타입 칩(높이 20 · 반경 4 · 먹선 1.5) 하나를 그리고 그린 폭을 돌려준다. 위젯이 아니라 함수라서, 칩 수십 개를 위젯 없이 한 `paintEvent` 안에서 그린다(ADR 0007). 위젯이 필요하면 감싼다(`TypeToggle`, `Chips`) |
| `spritefit::draw(painter, 상자, 그림, 배율)` | 정수 배율 니어리스트로 가운데, 넘치면 부드럽게 줄여 맞춤 |
| `svgicon::pixmap/icon(svg, 크기, dpr)` | 코드에 적은 인라인 SVG 문자열 → 고해상도 대응 아이콘 |
| `paintLayoutGuide(widget)` | 디버그용 점선 상자 + 클래스 이름 — 지금 부르는 곳 없음 |

## 아이콘이 위젯에 닿는 길 — SpriteCache (data 레이어)

```
위젯/delegate의 paint:
  file = cache->path(key)        디스크에 있으면 경로, 없으면 ""
  if (file.isEmpty()) { cache->request(key); return; }   ← 아무것도 안 그리고 받기만 요청
  QPixmapCache에서 꺼내거나 읽어서 그림
받기가 끝나면:
  SpriteCache::ready(key) ──connect──▶ update() / viewport()->update()   → 다시 그려지며 이번엔 그림이 있다
```
`SpriteCache`는 data 레이어라 `QPixmap`(QtGui)을 모르고 **파일 경로만** 다룬다. 그림으로 바꾸는 건 ui의 몫이다.
종류: `PokemonIcon` · `PokemonFront` · `Item` · `TownMap`. 페이지(또는 위젯)마다 자기 캐시를 자식으로 만들고, 하위 부품에 포인터로 빌려준다.
