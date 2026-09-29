# A9b — 창 크기를 도감 표에 맞추기

브랜치: `feat/a9b-window-size` · 커밋 footer: `Refs: A9b`

## 목표

지금 창은 1440×900으로 열린다. 그런데 도감 표는 폭 840이면 충분해서, 좌우에 빈 공간이 크게 남는다.
이 단계에서는 창을 **약 920×840**으로 열리게 해서 도감 창이 화면에 딱 맞게 한다.
창을 좁힐 때는 앱 막대의 **검색창만 280 → 160까지 줄어들게** 한다.

## 완료 조건

- [ ] 앱을 켜면 창이 920×840으로 열린다. 도감 창과 창 가장자리 사이는 좌우 20 남짓이다
- [ ] 창을 좁혀 보면 **검색창만** 줄어든다. 마크 · 탭 · 세대 버튼은 크기가 그대로다
- [ ] 검색창 폭이 220보다 작아지면 `Ctrl K` 배지가 사라지고, 넓히면 다시 나타난다
- [ ] 창을 약 910×804보다 작게 줄일 수 없다. 이 값은 **코드에 적은 숫자가 아니라 레이아웃이 계산한 값**이다
- [ ] 창을 넓히면 검색창이 280까지 다시 늘어나고, 그보다 크게 늘어나지는 않는다
- [ ] `scripts/linux/build.sh --format`이 통과한다

## 먼저 알아야 할 것 — 레이아웃이 크기를 정하는 방법

레이아웃은 창 크기가 바뀔 때마다 자식 위젯들의 크기를 다시 나눠 준다.
이때 위젯마다 세 가지 값을 본다.

| 값 | 뜻 | 누가 정하나 |
|---|---|---|
| `sizeHint()` | "이 크기면 가장 좋다" | 위젯(override). 레이아웃이 있는 위젯이면 기본값은 그 레이아웃이 계산한 값 |
| `minimumSizeHint()` | "이보다 작으면 망가진다" | 위젯(override). 기본값은 레이아웃이 계산하고, 레이아웃이 없으면 무효 `(-1,-1)` |
| `QSizePolicy` | hint에서 **줄여도 되나 / 늘려도 되나** | `setSizePolicy()` |

QHBoxLayout은 가로 공간을 이런 순서로 나눈다.

1. 모두에게 `sizeHint` 폭을 준다
2. **공간이 모자라면** 줄일 수 있는 위젯(policy가 `Fixed`가 아닌 것)을 `minimumSizeHint`까지 줄인다
3. **공간이 남으면** stretch factor가 있는 항목(`addStretch()`로 넣은 빈칸 등)에 먼저 나눠 준다

그 밖에 알아 둘 규칙이 세 가지 있다.
- `setFixedSize(w, h)`는 최소 = 최대 = (w, h)로 못 박는다. 그러면 위의 협상에서 빠진다. **지금 검색창이 그렇다**
- 숨긴(`hide()`) 위젯은 레이아웃에서 자리를 차지하지 않는다
- `QMainWindow`의 최소 크기는 central widget의 `minimumSizeHint()`에서 나온다.
  `QStackedWidget`의 최소 크기는 페이지들 중 가장 큰 값이다(인트로 높이 804, 앱 막대 폭 …).

> ROS 2에 빗대면, 레이아웃은 위젯마다 "선호 · 최소 · 늘어나도 되나"라는 제약을 받아
> resize 이벤트마다 다시 푸는 **작은 제약 해결기**다. 자식 크기를 부모가 하나하나 정하는 게 아니라,
> 제약만 선언해 두면 레이아웃이 매번 계산한다.

읽을 문서:
- [Layout Management](https://doc.qt.io/qt-6/layout.html) — "Adding Widgets to a Layout" 절의 1–4번 규칙
- [QSizePolicy::Policy](https://doc.qt.io/qt-6/qsizepolicy.html#Policy-enum)
- [QWidget::sizeHint](https://doc.qt.io/qt-6/qwidget.html#sizeHint-prop), [minimumSizeHint](https://doc.qt.io/qt-6/qwidget.html#minimumSizeHint-prop)
- [QWidget::resizeEvent](https://doc.qt.io/qt-6/qwidget.html#resizeEvent)

지금 값(offscreen 측정):

| 위젯 | sizeHint 폭 | 비고 |
|---|---|---|
| AppBar 전체 | **1030** | 여백 13 + 20, 간격 12 + 24 포함 |
| MarkButton | 151 | size policy가 없다 → 기본값 `Preferred` → **줄어들 수 있다** (CP2의 함정) |
| AppTabBar | 400 | `Fixed` |
| GenerationButton | 130 | `Fixed` |
| SearchField | 280 | `setFixedSize(280, 36)` |
| DexPage | 908 | 표 840 + 창 테두리 · 여백 + 페이지 여백 좌우 20 |
| HomePage(인트로) | 높이 **804** | |

검색창이 160까지 줄어들면 앱 막대의 최소 폭은 1030 − 120 = **910**이 된다. 그래서 920이면 들어간다.

---

## CP1 — 검색창이 스스로 크기를 말하게 한다

**파일**: [src/ui/widgets/searchfield.h](../../src/ui/widgets/searchfield.h), [searchfield.cpp](../../src/ui/widgets/searchfield.cpp)

지금은 앱 막대가 검색창에 `setFixedSize(280, 36)`로 크기를 못 박는다.
이걸 "선호 280, 최소 160, 높이 36"으로 바꿔서, 검색창이 레이아웃과 크기를 협상하게 한다.

```cpp
// searchfield.h — public:
    // 디자인 01 §5-7: 앱 막대 검색 폭 280 · 높이 36. 창이 좁으면 160까지 줄어든다.
    static constexpr int kPreferredWidth = 280;
    static constexpr int kMinimumWidth = 160;
    static constexpr int kHeight = 36;

    QSize sizeHint() const override;        // TODO 1
    QSize minimumSizeHint() const override; // TODO 2
```

```cpp
// searchfield.cpp
QSize SearchField::sizeHint() const
{
    // TODO 1: {kPreferredWidth, kHeight}를 돌려준다.
    //   override하면 레이아웃이 계산하던 기본값(196 × 25)을 대신한다.
}

QSize SearchField::minimumSizeHint() const
{
    // TODO 2: {kMinimumWidth, kHeight}
}
```

생성자에도 두 줄이 필요하다.

```cpp
    // TODO 3: 가로는 hint에서 줄이거나 늘릴 수 있고(Preferred), 세로는 36으로 고정(Fixed).
    //         setSizePolicy(가로 policy, 세로 policy)
    // TODO 4: 280보다 크게는 늘어나지 않게 최대 폭을 건다. setMaximumWidth(...)
```

> **왜 최대 폭까지 걸어야 하나**: 이 단계에서는 앱 막대에 `addStretch()` 빈칸이 있으므로,
> 남는 공간은 그 빈칸이 먼저 가져간다. 그래서 최대 폭이 없어도 당장은 문제가 없다.
> 하지만 검색창을 다른 레이아웃에 넣으면 끝없이 늘어날 수 있으니, 위젯 자체의 약속으로 걸어 둔다.

## CP2 — 앱 막대에서 못 박기를 푼다 (그리고 함정 하나)

**파일**: [src/ui/shell/appbar.cpp](../../src/ui/shell/appbar.cpp), [src/ui/shell/markbutton.cpp](../../src/ui/shell/markbutton.cpp)

```cpp
// appbar.cpp
    m_search = new SearchField(tr("포켓몬 · 기술 · 아이템 검색"), QStringLiteral("Ctrl K"));
    // TODO 5: m_search->setFixedSize(kSearchWidth, kSearchHeight); 줄을 지운다.
    //         kSearchWidth · kSearchHeight 상수도 더는 안 쓰면 지운다(SearchField로 옮겼다).
    layout->addWidget(m_search, 0, Qt::AlignVCenter);
```

**함정**: 이대로 창을 좁히면 검색창만이 아니라 **마크 버튼도 같이 줄어든다**.
MarkButton에는 size policy를 따로 준 적이 없어서 기본값 `Preferred`(줄여도 됨)이다.
게다가 `minimumSizeHint()`가 없어서(-1, -1) 최소 폭이 사실상 0이다.
공간이 모자라면 레이아웃은 줄일 수 있는 위젯들 사이에 줄일 폭을 나눈다. 그래서 마크도 찌그러진다.

```cpp
// markbutton.cpp — 생성자
    // TODO 6: 가로 · 세로 모두 Fixed. (GenerationButton · AppTabBar의 TabButton과 같은 방식)
    //         그러면 마크는 늘 sizeHint(151 × 50) 그대로다.
```

> TODO 6을 먼저 하지 말고, **TODO 5만 한 상태로 한 번 실행해 창을 좁혀 보는 걸 추천한다.**
> 마크가 찌그러지는 걸 직접 보면 policy가 무슨 일을 하는지 바로 와닿는다.

## CP3 — 좁으면 `Ctrl K` 배지를 숨긴다

**파일**: searchfield.h/.cpp

160 폭에서는 배지(약 40 + 간격 8)까지 넣으면 입력 칸이 60px 남짓밖에 남지 않는다.
그래서 검색창 폭이 220보다 작으면 배지를 숨긴다.

```cpp
// searchfield.h
protected:
    void resizeEvent(QResizeEvent *event) override; // TODO 7
private:
    QLabel *m_shortcut = nullptr; // TODO 8: 생성자의 지역 변수 shortcut을 이 멤버로 바꾼다 (없으면 nullptr)
    static constexpr int kShortcutMinWidth = 220;
```

```cpp
// searchfield.cpp
void SearchField::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // TODO 7: m_shortcut이 있으면, width()가 kShortcutMinWidth 이상일 때만 보이게 한다.
    //         setVisible(bool) 한 줄. 숨긴 라벨은 검색창 안쪽 레이아웃에서 자리를 내놓는다
    //         → 그 폭을 입력 칸(stretch 1)이 가져간다.
}
```

> **무한 루프가 생기지 않는 이유**: 배지를 숨기면 검색창 **안쪽** 레이아웃만 다시 나뉜다.
> 검색창 자신의 크기는 **바깥**(앱 막대) 레이아웃이 정하고, 그 계산은 CP1에서 고정한
> `sizeHint`와 `minimumSizeHint`만 본다. 그래서 숨기기가 다시 resize를 부르지 않는다.
> 만약 배지 유무에 따라 hint가 달라지게 짰다면 루프가 생길 수 있다.

(선택) 160 폭이면 안내 문구 "포켓몬 · 기술 · 아이템 검색"도 잘린다.
짧게 바꾸고 싶으면 문구만 고치면 되고, 그 경우 [design/README.md](../../design/README.md)의 "의도한 차이" 표에 한 줄을 추가한다.

## CP4 — 창의 기본 크기와 최소 크기

**파일**: [src/ui/shell/mainwindow.cpp](../../src/ui/shell/mainwindow.cpp)

```cpp
    // TODO 9: QMainWindow::setMinimumSize(960, 640); 줄을 지운다.
    //   이 값은 지금 앱 막대(최소 910)와 인트로(최소 804)의 실제 최소 크기와 맞지 않는다.
    //   게다가 폭 960 > 920이라, 이대로 두면 920으로 열 수 없다.
    //   지우면 central widget의 minimumSizeHint(레이아웃 계산값)가 최소 크기가 된다.
    // TODO 10: QMainWindow::resize(1440, 900) → 920 × 840.
    //   숫자는 이름 있는 상수로 둔다. 예: constexpr QSize kDefaultSize{920, 840};
    //   이유를 주석으로 남긴다: "도감 표(840) + 창 테두리 · 여백에 맞춘 크기. 인트로 최소 높이 804".
```

## 확인

```bash
scripts/linux/build.sh --format
scripts/linux/run.sh
```

1. **창 크기**: 창이 거의 도감 창 폭으로 열리는지 본다.
2. **좁히기**: 창 오른쪽 가장자리를 끌어 왼쪽으로 좁힌다.
   - 검색창만 줄어드는지, 배지가 사라지는지, 어느 폭에서 더 좁혀지지 않는지 본다.
3. **숫자로 확인**: `MainWindow` 생성자 끝에 임시로 한 줄을 넣고 실행한다. 확인한 뒤에는 지운다.
   ```cpp
   qCInfo(lcUi) << "min" << QMainWindow::minimumSizeHint() << "search" << m_appBar->searchField()->size();
   ```
   예상 출력(대략):
   ```
   pokesix.ui: min QSize(910, 804) search QSize(170, 36)
   ```
   - 첫 번째 값은 최소 크기다(인트로 · 본 화면 중 큰 값).
   - 두 번째 값은 920 폭일 때의 검색창 크기다: 1030 − 920 = 110만큼 줄어서 170.
   - 폭이 조금 다르면 글꼴 폭 때문일 수 있다. 차이가 크면 진단할 때 같이 보자.
4. **도감 화면**: 도감 탭에서 표가 잘리지 않는지 본다(가로 스크롤이 생기면 안 된다).
5. **인트로**: 로고부터 메뉴 맨 아래 "종료"까지 다 보이는지 본다.

다 되면 "했음"이라고 말해 달라. `git diff main...feat/a9b-window-size`로 진단한다.
