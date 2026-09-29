# A2 — 레이아웃: 두 단계 화면과 인트로 뼈대

> 학습 루프 ① 가이드. 코드는 직접 작성하고, 끝나면 "A2 진단해줘"라고 요청한다.
> 브랜치: `feat/a2-home-layout` (Claude가 만들어 둠)
>
> **방향 전환(2026-09-29)**: 홈은 앱 막대 없는 전체 화면 인트로가 되었다([ADR 0008](../decisions/0008-intro-screen-replaces-home.md)).
> 클래스 이름은 `HomePage`를 그대로 쓴다. 이 리포에서 "홈"은 인트로 화면이고, 핸드오프 SCR-01(대시보드)과는 다르다.
> 처음 버전의 이 가이드(세대 카드 · 최근 스쿼드 · 바로 가기)는 폐기했다. 체크포인트 1(AppBar + 페이지 스택)은 그대로 살아 있다.

## 목표

1. `MainWindow`를 **두 단계 스택**으로 바꾼다: 인트로 │ 본 화면(앱 막대 + 페이지)
2. 인트로를 이루는 위젯을 **최종 클래스 이름 그대로** 만들고, 점선 상자와 클래스 이름만으로 화면 가운데에 세운다

A4(토큰) · A5(`paintEvent`) · A6(메뉴 동작) · A7(세대 버튼) · A8(앱 막대)은 새 파일을 거의 만들지 않고, 여기서 만든 상자의 안을 채운다.
구조 원칙은 [ADR 0007](../decisions/0007-ui-component-structure.md)(그리기 · 동작 · 겉모양 분리, 위젯 rect = 상자 + 그림자)을 따른다.

**시각 기준**: 새 디자인은 Claude Design에 요청 중이다([요청서 0001](../design-requests/0001-intro-and-mark.md)).
그때까지는 아래 §3의 **임시 와이어프레임 수치**를 쓴다. 디자인이 도착하면 수치 블록만 바꾼다. 이번 단계에서 배우는 건 수치가 아니라 **배치하는 방법**이다.

---

## 1. 만들 것

| 파일 (`src/ui/…`) | 클래스 | 베이스 | 이번 단계에서 할 일 |
|---|---|---|---|
| `shell/mainwindow` | `MainWindow` (수정) | `QMainWindow` | central = `m_screens`(QStackedWidget) [인트로 │ 본 화면] |
| `shell/appbar` | `AppBar` (있음) | `QWidget` | 그대로. 본 화면의 맨 위 |
| `home/homepage` | `HomePage` (있음) | `QWidget` | **이번 단계의 본체.** 인트로(홈) 전체 레이아웃 |
| `home/homemenu` | `HomeMenu` | `QWidget` | 메뉴 창의 **내용**: 메뉴 항목 4개를 세로로. 키보드 이동은 A6 |
| `home/menubutton` | `MenuButton` | `QAbstractButton` | 메뉴 항목 하나. ▶ 커서 · 선택은 A6 |
| `widgets/panelframe` | `PanelFrame` | `QWidget` | 창의 **겉모양**. 내용 위젯 하나를 받아 안에 끼운다. 그림은 A5 |
| `widgets/logo` | `Logo` | `QWidget` | 마크 + 워드마크. 인트로(크게)와 앱 막대(작게, A8)에서 쓴다 |
| `widgets/generationbutton` | `GenerationButton` | `QAbstractButton` | "GEN 4 신오 ▾". 인트로와 앱 막대에서 쓴다. 메뉴는 A7 |
| `widgets/layoutguide` | 자유 함수 (있음) | — | 임시 점선 + 클래스 이름. A5에서 지운다 |

**폴더를 가른 기준**(ADR 0007 6항): `Logo`와 `GenerationButton`은 인트로와 앱 막대 **두 곳**에서 쓰니 `widgets/`에 둔다.
`MenuButton`은 지금은 홈에서만 쓰니 `home/`에 둔다. 나중에 설정 · 아이템의 분류 목록(노란 칸 + ▶)에서도 쓰게 되면 그때 `widgets/`로 올린다.

## 2. 알아야 할 개념

### 2-1. 두 단계 스택

```
MainWindow
 └ m_screens (QStackedWidget)            ← setCentralWidget
    ├ [0] HomePage                      전체 화면
    └ [1] 본 화면 (QWidget, 클래스 없음)
         └ QVBoxLayout (margins 0, spacing 0)
            ├ AppBar                     고정 60
            └ m_pages (QStackedWidget)   도감 · 아이템 · 스쿼드 · 설정 (지금은 빈 QWidget 하나)
```
- **바깥 스택**(`m_screens`)은 "인트로냐 본 화면이냐", **안쪽 스택**(`m_pages`)은 "본 화면에서 어느 페이지냐"를 고른다. 두 선택은 서로 독립이다
- "본 화면"은 그리기도 동작도 없고 한 곳에서만 쓰는 **조합**이라 클래스로 만들지 않는다. `MainWindow` 생성자에서 `QWidget` + 레이아웃으로 조립한다(체크포인트 2에서 배운 기준)
- 전환(메뉴 → 본 화면, 마크 → 인트로)은 시그널/슬롯이 필요하니 A6에서 붙인다. 지금은 `setCurrentIndex()`를 손으로 바꿔 확인한다

### 2-2. 가운데 정렬과 `sizeHint()`

인트로는 모든 요소가 **가로 가운데**, 묶음 전체가 **세로 가운데**다.
- 세로 가운데: 묶음 **위와 아래에 같은 stretch**를 둔다. 남는 공간이 둘로 똑같이 나뉜다
- 가로 가운데: `addWidget(w, 0, Qt::AlignHCenter)`

여기서 Qt의 중요한 규칙이 하나 나온다.
> **정렬(alignment)을 주면, 레이아웃은 그 위젯을 칸에 꽉 채우지 않고 `sizeHint()` 크기로 둔다.**

그런데 레이아웃도 없고 `sizeHint()`를 재정의하지도 않은 `QWidget`의 `sizeHint()`는 **무효값(-1 × -1)**이다. 그래서 정렬을 주는 순간 상자가 사라질 수 있다(실험 3).
이번 단계에서는 `setFixedSize()`로 크기를 못 박는다. 제대로 된 방법은 위젯이 자기 크기를 아는 것, 즉 `sizeHint()` 재정의다. 로고 · 버튼이 실제로 그림을 그리게 되는 A5 · A7에서 그렇게 바꾼다.

반대로 **레이아웃이 있는 위젯**(`PanelFrame`, `HomeMenu`)은 레이아웃이 자식들의 크기를 합쳐 `sizeHint()`를 계산해 준다. 그래서 메뉴 창은 높이를 따로 정하지 않아도 된다.

### 2-3. 레이아웃과 object tree (체크포인트 1 복습)

- `new QVBoxLayout(w)`는 `w`에 레이아웃을 설치한다
- `layout->addWidget(child)`는 child의 부모를 **레이아웃이 설치된 위젯**으로 바꾼다. `stack->addWidget(page)`는 page의 부모를 stack으로 바꾼다
- 레이아웃은 위젯을 소유하지 않는다. `addLayout(inner)`로 넣은 안쪽 레이아웃은 바깥 레이아웃이 소유한다

### 2-4. `QAbstractButton`은 추상 클래스다

`paintEvent()`가 순수 가상이라 재정의하지 않으면 인스턴스를 만들 수 없다. 대신 클릭 · 키보드 · 포커스 · `clicked()` · 텍스트(`setText`) · 접근성을 준다.
`MenuButton`과 `GenerationButton`이 이 클래스를 고른 이유다. 메뉴 문구는 지금 `setText(tr("도감 대백과"))`처럼 넣어 두면 A5에서 그릴 때 `text()`로 꺼내 쓴다.

### 2-5. 겉모양(`PanelFrame`)과 내용(`HomeMenu`)

- `PanelFrame`은 내용 위젯 하나를 `setBody(QWidget *)` 같은 메서드로 받는다
- 겉모양의 두께(테두리 + 머리 + 그림자)는 `PanelFrame` **자기 레이아웃의 contentsMargins**로 확보한다. 내용은 그 안쪽 사각형만 받는다
- 메뉴 창은 머리가 없는 변형이다: 좌 · 위 · 우 테두리 2, 아래 테두리 2 + 그림자 4
- `HomeMenu`는 자기가 창 안에 있다는 걸 모른다. A6에서 이 내용을 그대로 두고 키보드 이동만 넣는다

### 2-6. 수치를 어디에 둘까

각 `.cpp` 맨 위의 익명 namespace에 `constexpr`로 모은다. `QMargins`, `QSize`도 `constexpr`가 된다.
```cpp
namespace {
constexpr QSize kLogoSize{560, 160};
constexpr QMargins kPageMargins{40, 40, 40, 24};
} // namespace
```
A4에서 이 블록을 `ui/theme`로 옮기고, 새 디자인이 오면 값만 바꾼다.

### 2-7. 그림자는 위젯 rect 안에 (ADR 0007 4항)

Qt 위젯은 자기 rect 밖에 그릴 수 없다. 그래서 아래로 떨어지는 그림자는 **위젯 높이에 포함**하고, 그 아래 간격에서 그만큼 뺀다.
- 버튼: 디자인 시트 §4 그림자 3 → `GenerationButton` 높이 40 + 3
- 창: 그림자 4 → `PanelFrame` 아래 margin에 +4

## 3. 임시 와이어프레임 (1440×900)

```
┌──────────────────────────────── HomePage (0,0 1440×900) ────────────────────────────────┐
│  margins 좌 40 · 위 40 · 우 40 · 아래 24                                                    │
│                                      (stretch 1)                                           │
│                            ┌──────── Logo 560×160 ────────┐                                │
│                            └──────────────────────────────┘                                │
│                                        간격 24                                             │
│                              ┌── GenerationButton 220×43 ──┐   (버튼 40 + 그림자 3)          │
│                              └─────────────────────────────┘                               │
│                                   간격 28 − 3 = 25                                          │
│                          ┌─────── PanelFrame 폭 400 ────────┐  margins 2 · 2 · 2 · 2+4      │
│                          │ ┌──── HomeMenu ──────────────┐  │  margins 8 · spacing 4        │
│                          │ │ MenuButton  높이 52          │  │  도감 대백과                   │
│                          │ │ MenuButton                   │  │  아이템 대백과                 │
│                          │ │ MenuButton                   │  │  SixSquad                     │
│                          │ │ MenuButton                   │  │  설정                         │
│                          │ └──────────────────────────────┘  │                               │
│                          └───────────────────────────────────┘                              │
│                                      (stretch 1)                                           │
│  v0.0.1                                                               데이터: PokéAPI       │
└────────────────────────────────────────────────────────────────────────────────────────────┘
```
- 아래 정보 줄은 `QHBoxLayout`: 버전 라벨 · stretch · 출처 라벨. 버전은 `QApplication::applicationVersion()`에서 읽는다. 클래스는 만들지 않는다
- 문구는 모두 `tr()`로 감싼다

**확인할 좌표(창 좌표)**
| 상자 | x | 폭 | 높이 |
|---|---|---|---|
| `Logo` | 440 | 560 | 160 |
| `GenerationButton` | 610 | 220 | 43 |
| `PanelFrame` | 520 | 400 | 244 |
| `MenuButton` ×4 | 530 | 380 | 52 |

`PanelFrame` 높이 244와 `MenuButton` x · 폭은 직접 계산해 보고, 왜 이 값이 되는지 설명할 수 있어야 한다.
세로 위치는 위아래 stretch가 똑같이 나눈다. `Logo`의 위 여백과 `PanelFrame`의 아래 여백이 같은지(정보 줄 높이 차이만큼 제외) 확인한다.

## 4. 작업 순서

**체크포인트 A — 두 단계 스택**
1. `MainWindow`: 2-1의 트리로 바꾼다. `m_pages`에는 임시로 빈 `QWidget` 하나를 넣는다
   - 확인: 창 전체가 `…::HomePage` 점선 상자다. `m_screens->setCurrentIndex(1)`로 잠깐 바꾸면 위에 `AppBar`(60)가 보인다. 확인이 끝나면 0으로 되돌린다

**체크포인트 B — 인트로 뼈대**
2. `widgets/`: `PanelFrame`(`setBody`), `Logo`, `GenerationButton`
3. `home/`: `MenuButton`, `HomeMenu`(메뉴 항목 4개 + 문구)
4. `HomePage`: 바깥 레이아웃, 위아래 stretch, 가운데 정렬, 정보 줄
5. `src/ui/CMakeLists.txt`에 새 파일 추가. `scripts/linux/build.sh --clean --format` 경고 0 · 포맷 통과

## 5. 실험 (진단 때 결과를 알려 줄 것)

1. **좌표 찍기**: 창 좌표로 각 상자의 위치와 크기를 로그로 찍는다
   - `findChildren<QWidget *>()`로 자식을 순회하고, `className()`과 `QRect(w->mapTo(window, QPoint(0, 0)), w->size())`를 `qCDebug(lcUi)`로 찍는다. `geometry()`가 아니라 `mapTo()`인 이유는?
   - **생성자 안**에서 찍을 때와 **`show()` 뒤**에서 찍을 때 값이 다른가? 왜 그런가?
   - 이 값으로 §3의 좌표 표를 확인한다(`run.sh --log`)
2. **stretch 한쪽만**: 위나 아래 stretch 하나를 빼면 묶음이 어디로 가는가? 둘 다 빼면?
3. **정렬 vs 고정 크기**: `Logo`의 `setFixedSize()`를 빼고 `Qt::AlignHCenter`는 그대로 두면 어떻게 되는가? 정렬도 같이 빼면? 2-2와 연결해 설명한다
4. **보이지 않는 페이지**: 인트로가 보이는 동안 `m_pages` 안의 위젯 좌표는 어떻게 찍히는가? `QStackedWidget`이 보이지 않는 페이지를 어떻게 다루는지 문서에서 찾아본다

## 6. 완료 조건

- [ ] `m_screens`[인트로 │ 본 화면], 본 화면 = `AppBar` + `m_pages`
- [ ] 1440×900에서 §3 좌표 표와 같다(±1px), 묶음이 세로 가운데에 있다
- [ ] 창 크기를 바꿔도 묶음이 가운데를 유지하고, 정보 줄은 아래 모서리에 붙어 있다
- [ ] 클래스 · 파일이 1절 표의 폴더에 있고, `widgets/`가 `home/`을 include하지 않는다
- [ ] 수치가 각 `.cpp` 위쪽의 `constexpr` 블록에 모여 있다
- [ ] 빌드 경고 0, `clang-format --dry-run --Werror` 통과, `ctest` 통과
- [ ] 실험 1~4 결과를 설명할 수 있다

## 7. 막히면 먼저 볼 것

| 증상 | 의심할 곳 |
|---|---|
| `cannot declare variable … to be of abstract type` | `QAbstractButton` 자식에 `paintEvent()` 재정의가 없다(2-4) |
| 상자가 안 보인다 | 그 클래스의 `paintEvent()`에서 가이드를 부르는가, 레이아웃에 넣었는가, 정렬을 줬는데 크기가 없는가(2-2) |
| 상자가 왼쪽에 붙는다 | `addWidget`의 정렬 인자 |
| 묶음이 위에 붙거나 퍼진다 | stretch 위치와 개수 |
| 새 파일에서 `undefined reference to vtable` / `paintEvent` | 헤더를 `qt_add_library` 목록에 넣었는가, 선언한 함수를 정의했는가 |
| 좌표가 전부 0 | 레이아웃이 아직 계산되기 전이다(실험 1) |

## 8. 읽을 문서

- [Layout Management](https://doc.qt.io/qt-6/layout.html) — "Adding Widgets to a Layout"의 크기 협상 규칙
- [QBoxLayout](https://doc.qt.io/qt-6/qboxlayout.html) — `addStretch`, `addSpacing`, `addWidget`의 alignment 인자
- [QWidget::sizeHint](https://doc.qt.io/qt-6/qwidget.html#sizeHint-prop) — "The default implementation … returns an invalid size if there is no layout"
- [QSizePolicy](https://doc.qt.io/qt-6/qsizepolicy.html)
- [QStackedWidget](https://doc.qt.io/qt-6/qstackedwidget.html)
- [QAbstractButton](https://doc.qt.io/qt-6/qabstractbutton.html) — "Subclassing" 절

## 부록 — 디자인이 도착하면: CSS 수치를 Qt로 읽는 법

`design/source/*.dc.html`은 텍스트로 열어 인라인 스타일을 읽는다.

| CSS | Qt |
|---|---|
| `display:flex; flex-direction:column` / 가로 | `QVBoxLayout` / `QHBoxLayout` |
| `display:grid; grid-template-columns: repeat(n, 1fr)` | `QGridLayout` + 열마다 같은 stretch |
| `gap` | `setSpacing()` (모든 칸 같을 때) / `setSpacing(0)` + `addSpacing()` (칸마다 다를 때) |
| `padding` | 레이아웃의 `setContentsMargins()` |
| `flex-grow:1`, `margin-left:auto`, `margin:auto` | stretch factor, `addStretch()`, 양쪽 stretch 또는 alignment |
| `align-items` / `justify-content: center` | `addWidget(w, 0, Qt::AlignBottom / AlignHCenter …)` |
| `width` / `height` (+ `box-sizing: border-box`) | `setFixedWidth` / `setFixedHeight` 또는 `sizeHint()` — 테두리 포함 |
| `box-shadow: 0 Npx 0` | 위젯 rect 안에 그린다 → 높이 +N, 아래 간격 −N |
| `outline` (선택 테 · 포커스 링) | 상자 바깥. 위젯 스스로 그리지 않고 자리만 비워 둔다(A6, 열린 질문 7) |
