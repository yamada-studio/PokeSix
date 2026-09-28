# A2 — 레이아웃: 홈의 뼈대

> 학습 루프 ① 가이드. 코드는 직접 작성하고, 끝나면 "A2 진단해줘"라고 요청한다.
> 브랜치: `feat/a2-home-layout` (Claude가 만들어 둠)

## 목표

홈 화면을 이루는 위젯을 **최종 클래스 이름 그대로** 만들고, 색 없이 **점선 상자와 클래스 이름만으로**
[10_home_1440.png](../design-handoff/images/screens/10_home_1440.png)와 같은 위치에 배치한다.
A4(토큰) · A5(`paintEvent`) · A6(세대 카드)은 새 파일을 만들지 않고, 여기서 만든 상자의 안을 채운다.

구조의 근거는 [ADR 0007](../decisions/0007-ui-component-structure.md)이다. 먼저 읽고 오자. 특히 4항(위젯 rect = 상자 + 그림자)과 5항(상자 바깥 장식)이 이번 단계의 수치에 직접 걸린다.

**눈에 보이는 결과**: 1440×900 창에 앱 막대 자리(60), 제목 줄(글자 3개), 세대 카드 상자 9개,
하단 왼쪽 "최근 스쿼드" 창 상자, 오른쪽 바로 가기 상자 3개가 캡처와 같은 x 좌표에 선다.

---

## 1. 만들 것

| 파일 (`src/ui/…`) | 클래스 | 베이스 | 이번 단계에서 할 일 |
|---|---|---|---|
| `shell/mainwindow` | `MainWindow` (수정) | `QMainWindow` | central widget = `AppBar` 위 + `QStackedWidget`(페이지) 아래 |
| `shell/appbar` | `AppBar` | `QWidget` | 높이 60 고정. 내용은 A7 |
| `home/homepage` | `HomePage` | `QWidget` | **이번 단계의 본체.** 홈 전체 레이아웃 |
| `home/generationcard` | `GenerationCard` | `QAbstractButton` | 세대 카드 한 장. 클릭 · 선택은 A6 |
| `home/recentsquadlist` | `RecentSquadList` | `QWidget` | "최근 스쿼드" 창의 **내용**(행 3개 자리). 행은 A8 |
| `home/shortcutcard` | `ShortcutCard` | `QAbstractButton` | 바로 가기 카드 한 장 |
| `widgets/panelframe` | `PanelFrame` | `QWidget` | 창의 **겉모양**. 내용 위젯 하나를 받아 안에 끼운다. 그림은 A5 |
| `widgets/layoutguide` | 자유 함수 | — | 임시: 점선 테두리 + 클래스 이름을 그린다. A5에서 지운다 |

- `HomePage`가 `PanelFrame`에 `RecentSquadList`를 끼운다. `RecentSquadList`는 자기가 창 안에 있다는 걸 모른다(ADR 0007 3항)
- `widgets/`는 `home/`을 include하지 않는다. 반대 방향만 된다

## 2. 알아야 할 개념

### 2-1. 디자인 원본(CSS)을 Qt 레이아웃으로 읽는 법

수치는 [Home.dc.html](../design-handoff/design/source/Home.dc.html)의 인라인 스타일에서 읽는다(텍스트로 열 것).
대응표:

| CSS | Qt |
|---|---|
| `display:flex; flex-direction:column` | `QVBoxLayout` |
| `display:flex` (가로) | `QHBoxLayout` |
| `display:grid; grid-template-columns: repeat(9, minmax(0,1fr))` | `QGridLayout` + 열마다 같은 stretch |
| `gap` | `setSpacing()` (모든 칸 같을 때) / `addSpacing()` (칸마다 다를 때) |
| `padding` | 레이아웃의 `setContentsMargins()` |
| `flex-grow:1`, `margin-left:auto`, 빈 `<span style="flex-grow:1">` | stretch factor, `addStretch()` |
| `align-items: flex-end` | `addWidget(w, 0, Qt::AlignBottom)` |
| `grid-template-columns: 1fr 440px` | 오른쪽 `setFixedWidth(440)`, 왼쪽 stretch 1 |
| `height: 290px` (+ `box-sizing: border-box`) | `setFixedHeight()` — 테두리 포함 |
| `box-shadow: 0 4px 0` | CSS에선 레이아웃 공간을 안 먹는다. **Qt에선 위젯 rect 안에 그린다** → 높이 +4, 아래 간격 −4 |
| `outline` (선택 테) | 둘 다 레이아웃 밖. Qt에선 그 자리만 비워 둔다(그리드의 `padding: 6px 4px 0`) |

마지막 두 줄이 핵심이다. **Qt 위젯은 자기 rect 밖에 그릴 수 없다.** 자식은 부모 안에서 잘리고, 형제끼리는 겹칠 수 없다(레이아웃이 겹치지 않게 배치한다).
그래서 그림자처럼 한쪽으로만 삐져나오는 장식은 위젯 크기에 포함시키고, 간격에서 그만큼 뺀다.

### 2-2. 레이아웃이 크기를 정하는 방식

레이아웃은 자식마다 다음을 묻고 남는 공간을 나눈다.
- `sizeHint()`: 원하는 크기
- `minimumSizeHint()`, `minimumSize()` / `maximumSize()`: 이보다 작게 / 크게는 안 된다
- `sizePolicy()`: hint보다 커지거나 작아져도 되는지 (`Fixed`, `Preferred`, `Expanding` …)
- stretch factor: 남는 공간을 어떤 비율로 나눌지

`setFixedHeight(n)`은 최소 = 최대 = n으로 묶는다(정책 값 자체는 바꾸지 않지만, 레이아웃은 그 범위를 벗어나지 못한다).
`QAbstractButton`을 상속한 클래스는 기본 size policy가 레이아웃 의도와 다를 수 있다. 가로로 늘어나야 하는 카드라면 정책을 **직접 지정**하고, 실제로 늘어나는지 눈으로 확인한다.

### 2-3. 레이아웃과 object tree

```cpp
auto *layout = new QVBoxLayout(this);   // this 위젯에 레이아웃을 설치
layout->addWidget(child);               // child 의 부모가 this 로 바뀐다 (reparent)
layout->addLayout(innerLayout);         // 안쪽 레이아웃의 위젯도 부모는 여전히 this
```
- 레이아웃은 위젯을 **소유하지 않는다.** 위젯의 부모는 레이아웃이 설치된 위젯이다
- `setCentralWidget(w)`는 `w`의 소유권을 `QMainWindow`로 가져간다
- 그래서 `new`로 만든 위젯을 레이아웃에 넣었다면 `delete`할 곳이 없어도 누수가 아니다(A1 1-3)

### 2-4. `QStackedWidget`

여러 페이지 중 **하나만 보이는** 컨테이너다. `addWidget()`으로 페이지를 넣고 `setCurrentIndex()` / `setCurrentWidget()`으로 바꾼다.
지금은 `HomePage` 하나만 넣는다. 나머지 4개와 탭 전환은 A7에서 붙인다.

### 2-5. `QAbstractButton`은 추상 클래스다

`paintEvent()`가 **순수 가상 함수**라서 재정의하지 않으면 인스턴스를 만들 수 없다(컴파일 에러).
대신 클릭, 키보드(Space), 포커스, `clicked()` 시그널, 접근성 역할을 공짜로 준다. 세대 카드와 바로 가기가 이 클래스를 고른 이유다.

### 2-6. 임시 레이아웃 가이드

아무것도 그리지 않는 `QWidget`은 화면에 보이지 않는다. 배치를 눈으로 확인하려고
**임시 자유 함수 하나**를 `widgets/layoutguide`에 두고, 각 상자 클래스의 `paintEvent()`에서 부른다.
- `QPainter`를 그 위젯 위에 연다
- 펜을 점선(`Qt::DashLine`)으로 둔다
- `rect()`를 테두리로 그린다. `adjusted(0, 0, -1, -1)`이 왜 필요한지 생각해 보자(펜 폭 1의 사각형이 차지하는 픽셀)
- 가운데에 `metaObject()->className()`을 쓴다. A1 실험 3에서 본 그 값이다

그리기 방식 자체는 A5에서 제대로 배운다. 지금은 "보이게만" 하면 된다.

### 2-7. 수치를 어디에 둘까

[conventions.md](../conventions.md)는 색 · 크기의 하드코딩을 금지한다. 하지만 토큰은 A4에서 생긴다.
이번 단계에서는 각 `.cpp` 맨 위의 익명 namespace에 **`constexpr` 상수로 모아 둔다.**
A4에서 그 블록만 `ui/theme`로 옮기면 된다. 코드 한가운데에 숫자를 흩뿌리지 않는다.

## 3. 수치 (1440×900 기준)

CSS 원본 값과, 그림자를 환산한 Qt 값이다. 왼쪽 열이 왜 오른쪽 열이 되는지 설명할 수 있어야 한다.

| 위치 | CSS 원본 | Qt |
|---|---|---|
| 앱 막대 | `height: 60px` | 높이 60 고정 |
| 홈 페이지 여백 | `padding: 28px 40px 32px` | margins 좌 40 · 위 28 · 우 40 · 아래 32 |
| 제목 줄 → 세대 그리드 | `gap: 24px` | 24 |
| 제목 줄 | 제목 · 설명 세로 `gap 6`, 오른쪽 문구는 `margin-left:auto`, 모두 아래 정렬 | |
| 세대 그리드 가장자리 | `padding: 6px 4px 0` | margins 좌 4 · 위 6 · 우 4 · 아래 0 |
| 세대 카드 | 9열 같은 폭, `gap 12`, `height 290`, 그림자 4 | 가로 간격 12, 카드 높이 290 + 4 |
| 세대 그리드 → 하단 | `gap: 24px` | 24 − 4 |
| 하단 두 칸 | `1fr 440px`, `gap 24` | 왼쪽 stretch, 오른쪽 폭 440, 간격 24 |
| 최근 스쿼드 창 | 테두리 2 · 머리 38 · 머리 아래 선 2 · 행 84 × 3 · 테두리 2 · 그림자 4 | `PanelFrame` margins: 좌 2 · 위 2+38+2 · 우 2 · 아래 2+4. 내용(`RecentSquadList`) 높이 84 × 3 |
| 바로 가기 | `height 88`, 세로 `gap 12`, 그림자 4 | 카드 높이 88 + 4, 간격 12 − 4 |
| 남는 세로 공간 | 내용 아래가 비어 있다 | 어디에 stretch를 둘지 직접 정한다 |

- `PanelFrame`은 겉모양 두께를 **자기 레이아웃의 contentsMargins**로 확보한다. 내용 위젯은 그 안쪽 사각형만 받는다(ADR 0007 3항).
  `setBody(QWidget *)` 같은 메서드 하나로 내용을 받게 하자
- 제목 줄의 문구 세 개는 `QLabel` + `tr()`로 원본 문구를 그대로 쓴다. 글꼴은 A4까지 기본 글꼴이다
- 세대 카드 9장은 `QGridLayout`에 넣는다. 한 줄뿐인데 그리드를 쓰는 이유: Phase F에서 1100 미만일 때 5열 × 2행으로 **다시 배치만** 하면 된다([설계서 §7](../design-handoff/docs/03_ARCHITECTURE.md#7-반응형-responsivecontroller) "레이아웃을 새로 만들지 말고 위젯을 재배치")

## 4. 작업 순서

작은 단위로 빌드 · 실행하며 쌓는다. 단계마다 창을 띄워 확인한다.

1. `widgets/layoutguide`를 만든다. `AppBar`를 만들어 가이드를 그리게 한다
2. `MainWindow`: central widget 안에 `AppBar` + `QStackedWidget`을 세로로 둔다(여백 0 · 간격 0). 빈 `HomePage`를 스택에 넣는다
   - 확인: 위에 높이 60 점선 상자 "AppBar", 아래 전체가 "HomePage"
3. `HomePage`: 바깥 세로 레이아웃 + 여백, 제목 줄
4. `GenerationCard` 9장 + 그리드
   - 확인: 9장 폭이 같고, 창 폭을 바꿔도 같이 늘고 준다
5. 하단: `PanelFrame`(+ `RecentSquadList`) │ 바로 가기 열(`ShortcutCard` × 3)
6. 남는 세로 공간 처리
7. `src/ui/CMakeLists.txt`에 새 파일 추가. 빌드 경고 0, `scripts/linux/build.sh --format` 통과

## 5. 실험 (진단 때 결과를 알려 줄 것)

1. **좌표 찍기**: 창 좌표로 각 상자의 위치와 크기를 로그로 찍는다
   - `findChildren<QWidget *>()`로 자식을 순회하고, `className()`과 `QRect(w->mapTo(window, QPoint(0, 0)), w->size())`를 `qCDebug(lcUi)`로 찍는다. `geometry()`가 아니라 `mapTo()`인 이유는?
   - **생성자 안**에서 찍을 때와 **`show()` 뒤**에서 찍을 때 값이 다른가? 왜 그런가?
   - 이 값으로 아래 완료 조건의 x 좌표를 확인한다(`run.sh --log`)
2. **stretch 빼 보기**: 6번에서 넣은 남는 공간 처리를 빼면, 남는 약 80px이 어디로 가는가?
3. **바로 가기 열**: 하단 줄의 높이는 왼쪽 창(300)이 정한다. 오른쪽 열(92 × 3 + 8 × 2 = 292)에 정렬이나 stretch를 주지 않으면 카드들이 어떻게 놓이는가?
4. **최소 창**: 창을 960×640까지 줄여 본다. 홈의 세로 내용 합(약 820)이 640보다 크다. 무슨 일이 일어나는가?
   (해결은 Phase F. 지금은 관찰만 한다)

## 6. 완료 조건

- [ ] 1440×900에서 캡처와 같은 위치(x는 ±1px, y는 기본 글꼴 탓에 제목 줄 높이만큼 어긋나도 된다)
  - 세대 카드 첫 장 x = 44, 9장 폭이 같다(±1)
  - 최근 스쿼드 창 x = 40 … 936, 바로 가기 열 x = 960 … 1400
  - 세대 카드 294(= 290 + 4), 최근 스쿼드 창 300, 바로 가기 92 × 3
- [ ] 창 폭을 줄이면 세대 카드와 최근 스쿼드 창만 줄고, 바로 가기 열은 440을 유지한다
- [ ] 클래스 · 파일이 1절 표의 폴더에 있고, `widgets/`가 `home/`을 include하지 않는다
- [ ] 수치가 각 `.cpp` 위쪽의 `constexpr` 블록에 모여 있다
- [ ] 빌드 경고 0, `clang-format --dry-run --Werror` 통과, `ctest` 통과
- [ ] 실험 1~4 결과를 설명할 수 있다

## 7. 막히면 먼저 볼 것

| 증상 | 의심할 곳 |
|---|---|
| `cannot declare variable … to be of abstract type` | `QAbstractButton` 자식에 `paintEvent()` 재정의가 없다(2-5) |
| 상자가 안 보인다 | 그 클래스의 `paintEvent()`에서 가이드를 부르는가. 레이아웃에 `addWidget` 했는가 |
| 카드 9장이 왼쪽에 몰리거나 폭이 제각각 | size policy와 열 stretch(2-2) |
| 새 파일에서 `undefined reference to vtable` | 새 헤더를 `qt_add_library` 목록에 넣었는가(A1과 같은 원인) |
| 좌표가 전부 0 | 레이아웃이 아직 계산되기 전이다(실험 1) |

## 8. 읽을 문서

- [Layout Management](https://doc.qt.io/qt-6/layout.html) — "Adding Widgets to a Layout"의 크기 협상 규칙
- [QSizePolicy](https://doc.qt.io/qt-6/qsizepolicy.html) — `Policy` 표
- [QBoxLayout](https://doc.qt.io/qt-6/qboxlayout.html) — `addStretch`, `addSpacing`, stretch factor
- [QGridLayout](https://doc.qt.io/qt-6/qgridlayout.html) — `setColumnStretch`
- [QStackedWidget](https://doc.qt.io/qt-6/qstackedwidget.html)
- [QAbstractButton](https://doc.qt.io/qt-6/qabstractbutton.html) — "Subclassing" 절
