# UI 구조 지도 — Qt 클래스를 우리 코드가 어떻게 쓰는가

`src/ui` · `src/app`의 화면 코드를 **그림으로 전수조사**한 문서다(2026-10-08 기준 코드).
화면마다 "어떤 Qt 클래스를 상속해서 · 어떤 부모 밑에 · 어떤 레이아웃으로" 놓였는지를 한눈에 보는 것이 목적이다.
코드를 바꾸면 해당 화면 문서의 그림도 같이 고친다.

## 먼저: 세 가지 트리를 구분한다

Qt 위젯 코드를 읽을 때 헷갈리는 이유는 **서로 다른 트리 세 개**가 같은 클래스 이름으로 겹쳐 있기 때문이다.
이 문서의 그림은 항상 셋 중 하나만 그린다.

| 트리 | 질문 | 정해지는 때 | 코드에서 보이는 모양 | 이 문서의 그림 |
|---|---|---|---|---|
| **① 상속 트리** | 이 클래스는 **무엇인가**(is-a) | 컴파일 시간 | `class SlotCard : public QWidget` | [01-class-hierarchy.md](01-class-hierarchy.md) |
| **② 객체 트리**(object tree) | 이 객체를 **누가 소유하나** — 부모가 지워지면 같이 지워진다 | 실행 시간 | `new QLabel(this)`, `setParent()` | 각 화면 문서의 "객체 트리" |
| **③ 레이아웃 트리** | 화면에서 **어디에 놓이나** — 크기 · 위치 계산 | 실행 시간 | `layout->addWidget(w)`, `addLayout`, `addStretch` | 각 화면 문서의 "배치 그림" · "레이아웃 트리" |

ROS 2에 빗대면:

- **① 상속**은 `class MyNode : public rclcpp::Node` — 무엇의 한 종류인가
- **② 객체 트리**는 component container가 노드들을 들고 있다가 같이 내리는 것 — **수명 · 소유권**. Qt에서는 부모 `QObject`의
  소멸자가 자식들을 `delete`한다. 그래서 우리 코드에는 위젯용 `delete`가 거의 없다
- **③ 레이아웃**은 그 노드들이 화면 어디를 차지하는지 — 소유와는 별개인 **배치 규칙**이다

②와 ③이 엮이는 지점 하나만 기억하면 된다:

> **레이아웃에 위젯을 넣으면, 그 위젯의 부모는 "레이아웃이 걸린 위젯"으로 바뀐다.**
> `QHBoxLayout`은 위젯의 부모가 될 수 없다(레이아웃은 위젯이 아니다). 그래서 `new QLabel`처럼 부모 없이 만들어도
> `layout->addWidget(label)` 하는 순간 레이아웃의 주인 위젯이 부모가 된다. 레이아웃 객체 자체도 그 위젯의 자식이다.

그래서 객체 트리는 레이아웃 트리보다 **납작하다**: 레이아웃이 3단으로 중첩돼도 그 안의 위젯들은 모두 같은 부모 위젯의 형제다.

```
레이아웃 트리 (배치)                     객체 트리 (소유)
SlotCard ─ QVBoxLayout                   SlotCard
           ├ QHBoxLayout                 ├ QVBoxLayout      ← 레이아웃도 자식(QObject)
           │ ├ m_name : QLabel           ├ QHBoxLayout
           │ └ m_menu : QToolButton      ├ m_name : QLabel   ← 중첩과 무관하게 모두 SlotCard의 자식
           └ m_moves : QWidget           ├ m_menu : QToolButton
                                         └ m_moves : QWidget
```
(위는 설명용 예다. 실제 SlotCard는 [05-squad.md](05-squad.md) 참고 — 자식 위젯 없이 직접 그린다.)

## 그림 읽는 법 (범례)

```
┌─ m_name : ClassName ───────┐   상자 = 위젯. "멤버 이름 : 클래스". 멤버가 아닌 지역 위젯은 이름 없이 클래스만
│                            │
└────────────────────────────┘
[V] QVBoxLayout   [H] QHBoxLayout   [G] QGridLayout   [S] QStackedWidget/QStackedLayout(한 번에 한 장)
~stretch~         addStretch — 남는 공간을 먹는 빈 칸
⟨paint⟩           자식 위젯 없이 paintEvent로 직접 그린다
★                 우리 클래스(src/ui) — 나머지는 Qt 기본 클래스
→                 시그널 연결 (sender::signal → receiver)
```

수치(여백 · 간격 · 고정 크기)는 코드 값 그대로 적는다. 디자인 수치의 출처는 `ui/theme/tokens.h`다.

## 목차

| 문서 | 내용 |
|---|---|
| [01-class-hierarchy.md](01-class-hierarchy.md) | **상속 계층** — 우리 클래스 60여 개가 어느 Qt 클래스를 상속하는지, Qt 기본 위젯 사용 현황 |
| [02-app-shell.md](02-app-shell.md) | `Application` → `MainWindow` — 앱 전체의 뼈대, 인트로 ↔ 본 화면 전환, 앱 막대 · 탭 5개 · 페이지 스택, 단축키, **전체 객체 트리** |
| [03-home.md](03-home.md) | 인트로(홈) 화면 — 마크 · 워드마크 · 세대 카드 배럴 · 메뉴 · 첫 실행 패널 |
| [04-dex.md](04-dex.md) | 도감 — 목록(필터 · 표 · 미리 보기)과 상세 페이지 |
| [05-squad.md](05-squad.md) | 스쿼드 — 슬롯 카드 6 + 분석 열, 넓은/좁은 배치, 선택 창 · 비전셔틀 창 |
| [06-items.md](06-items.md) | 아이템 백과 — 분류 · 표 · 상세 창 |
| [07-map.md](07-map.md) | 타운맵 백과 — 지도 뷰 · 장소 패널 탭 |
| [08-widgets.md](08-widgets.md) | 공용 컴포넌트 카탈로그 — `PanelFrame` · `ShadowButton` · 세대 버튼 … |
| [09-model-view.md](09-model-view.md) | 모델/뷰 — 모델 → 프록시 → 뷰 → delegate · header의 연결 |
| [10-styling.md](10-styling.md) | 스타일이 위젯에 닿는 길 — 토큰 → QSS → objectName/property, 글꼴 |

## 전체를 한 장으로

```
main.cpp
 └ Application (app/, Qt 클래스 아님)   ← composition root. QApplication · AppState · Repository · DataUpdater를 만들고
    │                                       MainWindow에 포인터로 넘긴다(소유는 Application)
    └ MainWindow : QMainWindow ★          1440×900, 제목 "PokeSix <빌드 식별 문자열>"
       └ centralWidget = [S] m_screens : QStackedWidget
          ├ 0  HomePage ★                     인트로 — 앱 막대 없음
          └ 1  shell : QWidget  [V] 여백 0 · 간격 0
               ├ m_appBar : AppBar ★           높이 60. 마크 버튼 · 탭 5개 · 세대 버튼
               └ [S] m_pages : QStackedWidget  나머지 높이 전부. 번호 = enum class Page
                    ├ 0 Page::Dex       DexPage ★   (목록 ↔ 상세를 안에서 전환)
                    ├ 1 Page::Items     ItemsPage ★
                    ├ 2 Page::Map       TownMapPage ★
                    ├ 3 Page::Squad     SquadPage ★
                    └ 4 Page::Settings  QLabel "설정 — 준비 중이에요" (자리만)
```
정확한 순서 · 연결 · 단축키는 [02-app-shell.md](02-app-shell.md).

## 주석과 코드가 다른 곳 (조사 중 발견, 2026-10-08)

이 문서는 **코드 기준**으로 그렸다. 아래 주석은 코드보다 오래돼서 틀렸다 — 읽을 때 주의하고, 고칠 때 같이 고친다.

| 위치 | 주석 | 실제 코드 |
|---|---|---|
| `shell/appbar.h:12-13`, `mainwindow.cpp:37-38` | 탭 4개 | 탭 **5개**(도감 · 아이템 · 타운맵 · 스쿼드 · 설정) — `apptabbar.cpp:39-50` |
| `mainwindow.cpp:37` | 앱 막대에 검색 칸 | 검색 칸은 지웠다(`appbar.cpp:53`의 메모) |
| `home/intromenu.h:10` | 카드 버튼 3개 | **4개**(스쿼드 · 도감 백과 · 아이템 백과 · 타운맵 백과) |
| `home/intromenu.cpp:84` | 처음 선택 = 도감 백과 | 0번 = **스쿼드** |
| `shell/page.h:4` | `Page` 순서 = 인트로 메뉴 순서 | 다르다. 인트로는 스쿼드 · 도감 · 아이템 · 지도, `HomePage`가 표로 바꾼다(`homepage.cpp:149-152`) |
| `shell/mainwindow.h:39` | `DexPage`의 부모는 this(MainWindow) | 직접 부모는 `m_pages` — `QStackedWidget::addWidget`이 부모를 바꾼다 |
| `src/ui/settings/` | 설정 화면 폴더 | 비어 있다(`.gitkeep`만). 설정 페이지는 `QLabel` 자리 표시 |
| `widgets/generationbutton.h:7` | 인트로가 Large 크기를 쓴다 | `Size::Large`를 만드는 곳이 없다(앱 막대의 Compact뿐) |
| `dex/dexpage.h:39`, `items/itemspage.h:43` | Repository · AppState는 MainWindow가 소유 | Application이 소유(`unique_ptr`). MainWindow는 포인터만 |
| `squad/squadpage.h:162-163` | 선택 창이 기억하는 도감 설명 | 그 주석이 `m_slides`(카드 애니메이션 표) 위에 붙어 있다 — `m_pickerDex`의 설명 |
| `squad/squadpage.cpp:493` | 카드 높이를 [최소, 기본] 사이로 자른다 | 아래(234)만 자르고 위는 자르지 않는다 |
| `resources/styles/app.qss:39` | ShadowButton은 QPushButton 하위 클래스 | `QAbstractButton` 하위 클래스 |

## 조사 중 발견한 작은 문제 (동작)

고치지 않았다 — 기록만 한다.

| 위치 | 증상 |
|---|---|
| `map/townmappage.cpp:53` | `m_itemIcons`의 `ready` 시그널을 아무 데도 잇지 않았다 → [아이템] 탭의 아이콘이 받아져도 바로 안 보이고, 다른 이유로 다시 그릴 때(포켓몬 아이콘 도착 · 탭 전환) 나타난다 |
| `items/itemspage.cpp:223-225` | 표의 `clicked`와 `currentRowChanged`가 둘 다 `showDetail`에 이어져, 줄을 클릭하면 상세 질의가 두 번 돈다 |
| `resources/styles/app.qss:30-36` | `QPushButton[variant=…]` 규칙을 쓰는 코드가 없다 |
| `widgets/layoutguide.*` · `SearchField`의 단축키 표시 | 정의만 있고 쓰는 곳이 없다 |
