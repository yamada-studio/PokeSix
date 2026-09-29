# A2 — 레이아웃: 두 단계 화면과 인트로 뼈대

> 학습 루프 ① 가이드. 코드는 직접 작성하고, 끝나면 "A2 진단해줘"라고 요청한다.
> 브랜치: `feat/a2-home-layout` (Claude가 만들어 둠)
>
> 홈은 앱 막대 없는 전체 화면 인트로다([ADR 0008](../decisions/0008-intro-screen-replaces-home.md)).
> 시각 기준은 디자인 v2다([ADR 0009](../decisions/0009-design-handoff-v2.md), [`30_intro_1440.png`](../../design/handoff-v2/images/screens/30_intro_1440.png),
> 원본 수치 `design/handoff-v2/design/source/Intro.dc.html`). v2 문서의 `IntroPage`가 이 리포의 `HomePage`다.
> 체크포인트 1(AppBar + 페이지 스택)은 그대로 살아 있다.
>
> **완료(2026-09-29)**: 체크포인트 A · B1 · B2는 사용자가, 이후(B3 정보 줄과 실제 모양 · 동작)는 사용자 요청으로
> Claude가 구현했다([ADR 0010](../decisions/0010-intro-implemented-by-claude.md)). 이 가이드는 읽기 자료로 남긴다.
> §3의 "임시 고정 크기"는 이제 각 위젯의 `sizeHint()`로 바뀌었다.

## 목표

1. `MainWindow`를 **두 단계 스택**으로 바꾼다: 인트로 │ 본 화면(앱 막대 + 페이지)
2. 인트로를 이루는 위젯을 **최종 클래스 이름 그대로** 만들고, 점선 상자와 클래스 이름만으로 `30_intro_1440.png`와 같은 자리에 세운다

A4(글꼴 · 토큰) · A5(바탕 · 창 그리기) · A6(마크) · A7(메뉴 동작) · A8(세대 메뉴) · A9(앱 막대)은 새 파일을 거의 만들지 않고, 여기서 만든 상자의 안을 채운다.
구조 원칙은 [ADR 0007](../decisions/0007-ui-component-structure.md)(그리기 · 동작 · 겉모양 분리, 위젯 rect = 상자 + 그림자)을 따른다.

---

## 1. 만들 것

| 파일 (`src/ui/…`) | 클래스 | 베이스 | 이번 단계에서 할 일 |
|---|---|---|---|
| `shell/mainwindow` | `MainWindow` (수정) | `QMainWindow` | central = `m_screens`(QStackedWidget) [인트로 │ 본 화면] |
| `shell/appbar` | `AppBar` (있음) | `QWidget` | 그대로. 본 화면의 맨 위 |
| `home/homepage` | `HomePage` (있음, v2의 `IntroPage`) | `QWidget` | **이번 단계의 본체.** 인트로 전체 레이아웃 |
| `home/intromenu` | `IntroMenu` | `QWidget` | 메뉴 창의 **내용**: 메뉴 줄 4 + 구분 자리 + 종료 줄. 선택 · 키보드는 A7 |
| `home/intromenuitem` | `IntroMenuItem` | `QAbstractButton` | 메뉴 줄 하나(이름 · 설명 · 단축키 칸). ▶ 커서 · 눌림은 A7 |
| `home/introfooter` | `IntroFooter` | `QWidget` | 아래 정보 줄: 버전 · 키 안내 · 출처 |
| `widgets/panelframe` | `PanelFrame` | `QWidget` | 창의 **겉모양**. 내용 위젯 하나를 받아 안에 끼운다. 그림은 A5 |
| `widgets/markwidget` | `MarkWidget` | `QWidget` | 캡슐 마크. 인트로 136, 앱 막대 36(A9). 그림은 A6 |
| `widgets/wordmarklabel` | `WordmarkLabel` | `QWidget` | "POKESIX" + 노란 글자 그림자. 인트로 80, 앱 막대 20(A9). 그림은 A5 |
| `widgets/generationbutton` | `GenerationButton` | `QAbstractButton` | "GEN 4 신오 ▾". 인트로와 앱 막대에서 쓴다. 팝업은 A8 |
| `widgets/layoutguide` | 자유 함수 (있음) | — | 임시 점선 + 클래스 이름. A5에서 지운다 |

- 부품 이름은 v2 설계서([03b §2](../../design/handoff-v2/docs/03b_ARCHITECTURE_V2.md))의 이름을 그대로 쓴다. 메뉴 줄 하나는 설계서에 이름이 없어서 `IntroMenuItem`으로 정했다
- **폴더를 가른 기준**(ADR 0007 6항): 마크 · 워드마크 · 세대 버튼은 인트로와 앱 막대 **두 곳**에서 쓰니 `widgets/`, 메뉴와 정보 줄은 인트로에서만 쓰니 `home/`
- `IntroBackground`(사선 무늬 + 위아래 빨강 띠)와 `FirstRunPanel`은 이번 단계에 없다. 각각 A5, D4에서 만든다
- 부제("세대별 정주행을 위한 도감 · 파티 도우미")와 세대 라벨("지금 여행 중인 세대")은 그리기도 동작도 없으니 `QLabel`로 둔다

## 2. 알아야 할 개념

### 2-1. 두 단계 스택

```
MainWindow
 └ m_screens (QStackedWidget)            ← setCentralWidget
    ├ [0] HomePage                       전체 화면 인트로
    └ [1] 본 화면 (QWidget, 클래스 없음)   v2 설계서의 Shell
         └ QVBoxLayout (margins 0, spacing 0)
            ├ AppBar                     고정 60
            └ m_pages (QStackedWidget)   도감 · 아이템 · 스쿼드 · 설정 (지금은 빈 QWidget 하나)
```
- **바깥 스택**(`m_screens`)은 "인트로냐 본 화면이냐", **안쪽 스택**(`m_pages`)은 "본 화면에서 어느 페이지냐"를 고른다
- "본 화면"은 그리기도 동작도 없고 한 곳에서만 쓰는 **조합**이라 클래스로 만들지 않는다. `MainWindow` 생성자에서 조립한다
- 전환(메뉴 → 본 화면, 마크 → 인트로)은 A7 · A9에서 시그널/슬롯으로 붙인다. 지금은 `setCurrentIndex()`를 손으로 바꿔 확인한다

### 2-2. 가운데 정렬과 `sizeHint()`

인트로의 요소는 모두 **가로 가운데**에 놓인다: `addWidget(w, 0, Qt::AlignHCenter)`.
세로는 가운데가 아니라 **위 여백 44에서 시작해 차례로 쌓이고**, 아래 정보 줄만 바닥에 붙는다. 그래서 stretch는 메뉴 창과 정보 줄 **사이에 하나**만 둔다.

여기서 Qt의 중요한 규칙이 나온다.
> **정렬(alignment)을 주면, 레이아웃은 그 위젯을 칸에 꽉 채우지 않고 `sizeHint()` 크기로 둔다.**

레이아웃도 없고 `sizeHint()`를 재정의하지도 않은 `QWidget`의 `sizeHint()`는 **무효값**이다. 그래서 정렬을 주는 순간 상자가 사라질 수 있다(실험 3).
이번 단계에서는 `setFixedSize()`로 크기를 못 박는다. 제대로 된 방법은 위젯이 자기 크기를 아는 것, 즉 `sizeHint()` 재정의다.
워드마크 · 세대 버튼은 글자 폭으로 크기가 정해지므로 A4(폰트 메트릭)에서, 마크는 A6에서 그렇게 바꾼다.

반대로 **레이아웃이 있는 위젯**(`PanelFrame`, `IntroMenu`)은 레이아웃이 자식들의 크기를 합쳐 `sizeHint()`를 계산한다. 그래서 메뉴 창은 높이를 따로 정하지 않는다.

### 2-3. `addSpacing()`과 레이아웃 간격

칸마다 간격이 다르면 `setSpacing(0)` + 칸마다 `addSpacing(n)`이 명확하다(`HomePage`).
간격이 대부분 같고 한 군데만 다르면 `setSpacing(n)` + 그 자리에 `addSpacing(m)`을 쓴다(`IntroMenu`). 이때 알아 둘 동작이 있다.
> `addSpacing(m)`이 들어간 자리의 간격은 `m + spacing` **한 번**이다. 양쪽에 두 번 붙지 않는다.

Claude가 확인한 값: spacing 2, 높이 58 위젯 → `addSpacing(10)` → 다음 위젯의 y = 58 + 10 + 2 = 70.
CSS의 `gap`은 구분선 요소 **양쪽에** 붙으므로, 구분선 자리를 옮길 때는 이 차이를 환산해야 한다(§3).

### 2-4. 겉모양(`PanelFrame`)과 내용(`IntroMenu`)

- `PanelFrame`은 내용 위젯 하나를 `setBody(QWidget *)` 같은 메서드로 받는다
- 겉모양의 두께는 `PanelFrame` **자기 레이아웃의 contentsMargins**로 확보한다. 내용은 그 안쪽 사각형만 받는다
- 메뉴 창의 겉모양 = 바깥 먹선 3 + 안쪽 여백 8 + 안쪽 이중 테 2 (+ 아래 그림자 6). 이중 테 안쪽의 여백 6부터는 내용(`IntroMenu`)의 몫이다
- `IntroMenu`는 자기가 창 안에 있다는 걸 모른다

### 2-5. 그림자는 위젯 rect 안에 (ADR 0007 4항)

Qt 위젯은 자기 rect 밖에 그릴 수 없다. 그래서 오프셋 그림자는 **위젯 크기에 포함**하고, 그 뒤 간격에서 그만큼 뺀다.
- 워드마크: 글자 그림자 `5px 5px` → 높이 80 + 5 (폭도 +5)
- 세대 버튼: 그림자 3 → 높이 46 + 3
- 메뉴 창: 그림자 6 → `PanelFrame` 아래 margin +6

### 2-6. 레이아웃과 object tree (체크포인트 1 복습)

- `new QVBoxLayout(w)`는 `w`에 레이아웃을 설치한다. `setLayout()`을 또 부를 필요가 없다
- `layout->addWidget(child)`는 child의 부모를 레이아웃이 설치된 위젯으로, `stack->addWidget(page)`는 page의 부모를 stack으로 바꾼다
- `addLayout(inner)`로 넣은 안쪽 레이아웃은 바깥 레이아웃이 소유한다

### 2-7. `QAbstractButton`

`paintEvent()`가 순수 가상이라 재정의하지 않으면 인스턴스를 만들 수 없다. 대신 클릭 · 눌림 상태 · `clicked()` · 텍스트(`setText`) · 접근성을 준다.
메뉴 이름은 지금 `setText(tr("도감 대백과"))`처럼 넣어 두면 A5에서 그릴 때 `text()`로 꺼내 쓴다. 설명 문구와 단축키 숫자는 A5에서 멤버로 추가한다.

### 2-8. 수치를 어디에 둘까

각 `.cpp` 맨 위의 익명 namespace에 `constexpr`로 모은다. `QMargins`, `QSize`도 `constexpr`가 된다.
```cpp
namespace {
constexpr QSize kMarkSize{136, 136};
constexpr QMargins kPageMargins{32, 44, 32, 26};
} // namespace
```
A4에서 v2 `Tokens.h`(`kSizeIntroMark = 136`, `kSizeIntroMenuWidth = 520`, `kSizeIntroMenuRow = 58` …)를 `ui/theme`로 옮기면, 이 블록은 토큰 참조로 바뀐다.

## 3. 수치 (1440×900, `Intro.dc.html`)

### `HomePage` — `QVBoxLayout`, margins 좌 32 · 위 44 · 우 32 · 아래 26, spacing 0

| 순서 | 항목 | CSS 원본 | Qt |
|---|---|---|---|
| 1 | `MarkWidget` | `svg 136×136`, `padding-top: 44` | 고정 136 × 136, 가운데 |
| 2 | 간격 | `margin: 6px 0 0` | `addSpacing(6)` |
| 3 | `WordmarkLabel` | Silkscreen 80 · 행간 1 · 그림자 `5px 5px 0` | **임시** 고정 460 × 85, 가운데 (A4에서 `sizeHint`로) |
| 4 | 간격 | `margin: 16px 0 0` | `addSpacing(16 − 5)` |
| 5 | 부제 `QLabel` | 도현 22 | 가운데 |
| 6 | 간격 | 세대 블록 `margin-top: 24` | `addSpacing(24)` |
| 7 | 세대 라벨 `QLabel` | 12 · 700 | 가운데 |
| 8 | 간격 | `gap: 6` | `addSpacing(6)` |
| 9 | `GenerationButton` | 높이 46 · 그림자 3, 폭은 내용(캡처 약 162) | **임시** 고정 162 × 49, 가운데 (A4에서 `sizeHint`로) |
| 10 | 간격 | 메뉴 `margin-top: 24` | `addSpacing(24 − 3)` |
| 11 | `PanelFrame` + `IntroMenu` | `width: 520` | 폭 고정 520, 높이는 레이아웃이 계산, 가운데 |
| 12 | stretch | (푸터는 `position:absolute; bottom:26`) | `addStretch()` |
| 13 | `IntroFooter` | `left/right: 32` | 전체 폭 |

- 위아래 빨강 띠(12 + 먹선 3)는 `position:absolute`라 레이아웃 공간을 차지하지 않는다. A5에서 바탕(`IntroBackground`)이 그린다
- 세대 메뉴 팝업도 `position:absolute`다. A8에서 `Qt::Popup` 창으로 띄우며, 레이아웃에 넣지 않는다

### `PanelFrame`(메뉴 창) — margins 좌 · 위 · 우 13, 아래 13 + 6

`13 = 먹선 3 + 여백 8 + 이중 테 2`. 이 값을 왜 이렇게 나눴는지는 2-4 참고.

### `IntroMenu` — `QVBoxLayout`, margins 6, spacing 2

| 항목 | CSS 원본 | Qt |
|---|---|---|
| `IntroMenuItem` × 4 | `height: 58`, `gap: 2` | 높이 58 고정. 도감 대백과 / 아이템 대백과 / SixSquad / 설정 |
| 구분선 자리 | `gap 2` + `margin 4 · 선 2 · margin 4` + `gap 2` = 14 | `addSpacing(?)` — 2-3의 규칙으로 직접 계산. 점선은 A5에서 그린다 |
| `IntroMenuItem`(종료) | `height: 42` | 높이 42 고정. 종료 |

### `IntroFooter` — `QHBoxLayout`, margins 0

버전 `QLabel`(`"v%1"` + `QApplication::applicationVersion()`) · stretch · 키 안내 `QLabel`("↑↓ 이동 · Enter 선택 · 1–4 바로 가기", 키캡 모양은 A5) · stretch · 출처 `QLabel`("데이터: PokéAPI · 비공식 팬 도구").
가운데 키 안내는 **창 가운데가 아니라 좌우 라벨 사이의 가운데**에 놓인다. CSS의 `margin: 0 auto`도 똑같이 동작한다.

### 확인할 좌표 (창 좌표)

| 상자 | x | y | 폭 × 높이 |
|---|---|---|---|
| `MarkWidget` | 652 | 44 | 136 × 136 |
| `WordmarkLabel` | 490 | 186 | 460 × 85 |
| `GenerationButton` | 639 | — | 162 × 49 |
| `PanelFrame` | 460 | — | 520 × **?** |
| `IntroMenuItem` × 4 | **?** | — | **?** × 58 |
| `IntroMenuItem`(종료) | 같은 x | — | 같은 폭 × 42 |
| `IntroFooter` | 32 | 바닥 = 874 | 1376 × (라벨 높이) |

- **?** 칸은 직접 계산해 표를 채워 오자. 캡처에서 메뉴 줄의 노란 칸은 x 479, 폭 482이고, 메뉴 창은 테두리 상자 332 + 그림자 6이다. 계산이 이 값과 맞으면 된다
- `GenerationButton` 아래의 y는 부제 · 라벨 높이에 따라 달라진다. 글꼴이 들어오는 A4 전에는 맞지 않아도 된다

## 4. 작업 순서

**체크포인트 A — 두 단계 스택**
1. `MainWindow`: 2-1의 트리로 바꾼다. `m_pages`에는 임시로 빈 `QWidget` 하나를 넣는다
   - 확인: 창 전체가 `…::HomePage` 점선 상자(1440 × 900)다. `m_screens->setCurrentIndex(1)`로 잠깐 바꾸면 위에 `AppBar`(60)가 보인다. 확인이 끝나면 0으로 되돌린다

**체크포인트 B — 인트로 뼈대**
2. `widgets/`: `PanelFrame`(`setBody`), `MarkWidget`, `WordmarkLabel`, `GenerationButton`
3. `home/`: `IntroMenuItem`, `IntroMenu`(메뉴 줄 5개 + 문구), `IntroFooter`
4. `HomePage`: §3 표대로 쌓는다
5. `src/ui/CMakeLists.txt`에 새 파일 추가. `scripts/linux/build.sh --clean --format` 경고 0 · 포맷 통과

## 5. 실험 (진단 때 결과를 알려 줄 것)

1. **좌표 찍기**: 창 좌표로 각 상자의 위치와 크기를 로그로 찍는다
   - `findChildren<QWidget *>()`로 자식을 순회하고, `className()`과 `QRect(w->mapTo(window, QPoint(0, 0)), w->size())`를 `qCDebug(lcUi)`로 찍는다. `geometry()`가 아니라 `mapTo()`인 이유는?
   - **생성자 안**에서 찍을 때와 **`show()` 뒤**에서 찍을 때 값이 다른가? 왜 그런가?
   - 이 값으로 §3의 좌표 표를 확인한다(`run.sh --log`)
2. **stretch 위치**: 12번 stretch를 빼면 무엇이 어디로 가는가? 맨 위(1번 앞)로 옮기면?
3. **정렬 vs 고정 크기**: `MarkWidget`의 `setFixedSize()`를 빼고 `Qt::AlignHCenter`는 그대로 두면 어떻게 되는가? 정렬도 같이 빼면? 2-2와 연결해 설명한다
4. **보이지 않는 페이지**: 인트로가 보이는 동안 `m_pages` 안의 위젯 좌표는 어떻게 찍히는가? `QStackedWidget`이 보이지 않는 페이지를 어떻게 다루는지 문서에서 찾아본다

## 6. 완료 조건

- [ ] `m_screens`[인트로 │ 본 화면], 본 화면 = `AppBar` + `m_pages`
- [ ] 1440×900에서 §3 좌표 표와 같다(x · 폭 · 높이 ±1px, y는 마크 · 워드마크만)
- [ ] 창 폭을 바꿔도 모든 상자가 가로 가운데를 유지하고, 정보 줄은 바닥에서 26 위에 붙어 있다
- [ ] 클래스 · 파일이 1절 표의 폴더에 있고, `widgets/`가 `home/`을 include하지 않는다
- [ ] 수치가 각 `.cpp` 위쪽의 `constexpr` 블록에 모여 있다
- [ ] 빌드 경고 0, `clang-format --dry-run --Werror` 통과, `ctest` 통과
- [ ] 실험 1~4 결과를 설명할 수 있다

## 7. 막히면 먼저 볼 것

| 증상 | 의심할 곳 |
|---|---|
| `cannot declare variable … to be of abstract type` | `QAbstractButton` 자식에 `paintEvent()` 재정의가 없다(2-7) |
| 상자가 안 보인다 | 그 클래스의 `paintEvent()`에서 가이드를 부르는가, 레이아웃에 넣었는가, 정렬을 줬는데 크기가 없는가(2-2) |
| 상자가 왼쪽에 붙는다 | `addWidget`의 정렬 인자 |
| 묶음이 퍼지거나 가운데로 몰린다 | stretch 위치와 개수(2-2) |
| 메뉴 창 높이가 계산과 다르다 | `IntroMenu`의 spacing과 `addSpacing`(2-3), `PanelFrame`의 margins(2-4) |
| 새 파일에서 `undefined reference to vtable` / `paintEvent` | 헤더를 `qt_add_library` 목록에 넣었는가, 선언한 함수를 정의했는가 |
| 좌표가 전부 0 | 레이아웃이 아직 계산되기 전이다(실험 1) |

## 8. 읽을 문서

- [Layout Management](https://doc.qt.io/qt-6/layout.html) — "Adding Widgets to a Layout"의 크기 협상 규칙
- [QBoxLayout](https://doc.qt.io/qt-6/qboxlayout.html) — `addStretch`, `addSpacing`, `addWidget`의 alignment 인자
- [QWidget::sizeHint](https://doc.qt.io/qt-6/qwidget.html#sizeHint-prop) — 레이아웃이 없으면 무효값을 돌려준다는 설명
- [QStackedWidget](https://doc.qt.io/qt-6/qstackedwidget.html)
- [QAbstractButton](https://doc.qt.io/qt-6/qabstractbutton.html) — "Subclassing" 절

## 부록 — CSS 수치를 Qt로 읽는 법

`design/source/*.dc.html`은 텍스트로 열어 인라인 스타일을 읽는다.

| CSS | Qt |
|---|---|
| `display:flex; flex-direction:column` / 가로 | `QVBoxLayout` / `QHBoxLayout` |
| `display:grid; grid-template-columns: repeat(n, 1fr)` | `QGridLayout` + 열마다 같은 stretch |
| `gap` | `setSpacing()` — 구분 요소가 끼면 2-3의 환산 |
| `padding` | 레이아웃의 `setContentsMargins()` |
| `margin-top` (flex 자식) | 앞 칸의 `addSpacing()` |
| `flex-grow:1`, `margin: 0 auto` | stretch factor, 양쪽 `addStretch()` |
| `align-items: center` (column) | `addWidget(w, 0, Qt::AlignHCenter)` |
| `width` / `height` (+ `box-sizing: border-box`) | `setFixedWidth` / `setFixedHeight` 또는 `sizeHint()` — 테두리 포함 |
| `box-shadow` · `text-shadow`의 오프셋 | 위젯 rect 안에 그린다 → 크기 +N, 뒤 간격 −N |
| `position: absolute` | 레이아웃 밖. 바탕이 그리거나(띠) 별도 창(팝업) |
| `outline` (선택 테 · 포커스 링) | 상자 바깥. 위젯 스스로 그리지 않고 자리만 비워 둔다(A7, 열린 질문 7) |
