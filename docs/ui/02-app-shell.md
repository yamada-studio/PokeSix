# 02. 앱 뼈대 — Application → MainWindow → 화면 · 페이지

앱이 켜져서 창이 뜨기까지, 그리고 창 안의 큰 틀(인트로 ↔ 본 화면, 앱 막대, 페이지 스택)이다.
파일: `src/main.cpp` · `src/app/application.*` · `src/ui/shell/*`

## 1. 누가 무엇을 만드나 — Application (composition root)

`Application`은 **Qt 클래스가 아니다**(QObject 상속 없음, 복사 금지). 객체를 만들고 서로 잇는 유일한 곳이다.
ROS 2로 치면 `main()`에서 노드 · executor를 만들어 묶는 launch 코드의 자리다.

```
main.cpp  ──  Application app(argc, argv);  return app.run();

Application (스택 객체, application.h:61-69 — 선언 순서 = 생성 순서, 소멸은 역순)
 ├ m_app         QApplication           값으로 들고 있음. 이벤트 루프의 주인
 ├ m_translator  QTranslator            값. 영어 · 일본어일 때만 설치
 ├ m_repository  unique_ptr<Repository>  DB 창구 (QObject 아님)
 ├ m_state       unique_ptr<AppState>    세대 · 게임 · 언어 (QObject, 부모 없음)
 ├ m_updater     unique_ptr<DataUpdater> 첫 실행 데이터 받기 (QObject, 부모 없음)
 └ m_window      unique_ptr<MainWindow>  최상위 창 (부모 없음)
```

**`run()`의 순서** (`application.cpp:227-249`)

```
parseArguments()                --language · --screenshot 해석, 번역기 설치
buildObjects()
  1. theme::apply(m_app)        글꼴 등록 + app.qss를 QApplication에 한 번 적용  → 10-styling.md
  2. Repository(DB 경로)
  3. AppState
  4. DataUpdater
  5. MainWindow(repository*, state*, updater*)    ← 포인터만 넘긴다. 소유는 Application
m_window->show(); return m_app.exec();            ← 이벤트 루프 시작 (ROS 2의 spin)
```

`--screenshot` 모드에서는 창 크기를 정하고 해당 페이지를 연 뒤, `QTimer::singleShot(지연, …)`으로 `grab()`해서 PNG로 저장하고 끝낸다.

**주입(injection)이 핵심**: `MainWindow`와 각 페이지는 `Repository*` · `AppState*`를 **생성자 인자로 받기만** 하고 지우지 않는다.
이 객체들은 위젯 트리 밖(Application)에 살고, 창보다 오래 산다(소멸 순서가 창 → updater → state → repository).

## 2. MainWindow의 구조

```
MainWindow : QMainWindow ★      제목 "PokeSix <빌드 식별 문자열>" · 1440×900 (최소 크기는 minimumSizeHint에 맡김)
└ centralWidget ─ [S] m_screens : QStackedWidget        enum Screen { IntroScreen = 0, MainScreen = 1 }
   │
   ├ 0 ─ home : HomePage ★                               → 03-home.md
   │
   └ 1 ─ shell : QWidget
          [V] 여백 0 · 간격 0
          ├ m_appBar : AppBar ★                          고정 높이 60
          └ [S] m_pages : QStackedWidget                 남은 높이 전부(앱 막대가 고정이라 stretch 없이도)
               ├ 0  Page::Dex       m_dexPage : DexPage ★          → 04-dex.md
               ├ 1  Page::Items     ItemsPage ★                     → 06-items.md
               ├ 2  Page::Map       TownMapPage ★                   → 07-map.md
               ├ 3  Page::Squad     squad : SquadPage ★             → 05-squad.md
               └ 4  Page::Settings  QLabel#pagePlaceholder "설정 — 준비 중이에요" (가운데 정렬)
```

- `enum class Page { Dex, Items, Map, Squad, Settings }` (`shell/page.h`) — **이 순서가 곧 `m_pages`의 번호이자 탭 버튼의 id**다.
  enum을 정수로 바꿔 `setCurrentIndex(int(page))` 하므로, 페이지를 넣는 순서와 enum 순서가 어긋나면 엉뚱한 페이지가 열린다
- 생성 방식: 모든 페이지를 **부모 없이** `new`로 만들고 `m_pages->addWidget()`에 넣는다. `addWidget`이 부모를 `m_pages`로 바꾼다
- 시작 화면: `QStackedWidget`은 처음 넣은 장(0)을 보여 주므로 인트로가 먼저 뜬다. `m_pages`도 0번(도감)이지만 탭은 `open()`이 불릴 때까지 아무것도 켜지지 않는다

### 화면 전환 함수

```cpp
void MainWindow::open(Page page)          // mainwindow.cpp:115-125
    // 이미 도감에서 도감을 또 누르면 → m_dexPage->showList() (상세에서 목록으로)
    m_pages->setCurrentIndex(int(page));
    m_appBar->setCurrentPage(page);         // 탭 켜짐 표시 — 시그널을 다시 내지 않는다(무한 반복 방지)
    m_screens->setCurrentIndex(MainScreen);

void MainWindow::showIntro()              // :127-130
    m_screens->setCurrentIndex(IntroScreen);   // HomePage::showEvent가 메뉴에 포커스를 준다
```

### 단축키

모두 `new QShortcut(키, shell)` — 부모가 `shell`이고 문맥이 `Qt::WidgetWithChildrenShortcut`이라 **본 화면이 보일 때만** 동작한다(인트로에서는 안 먹는다).

| 키 | 동작 |
|---|---|
| Ctrl+1 · 2 · 3 · 4 · 5 | `open(Page::Dex · Items · Map · Squad · Settings)` |
| Ctrl+0 | `showIntro()` |

(도감 상세의 Esc는 `DexDetailPage`가 따로 가진다 → 04-dex.md)

## 3. AppBar — 빨강 앱 막대

```
m_appBar : AppBar ★  고정 높이 60  ⟨paint⟩ 빨강 채움 + 아래 먹선 3px
[H] 여백 (13, 0, 24, 0) · 간격 0
┌──────────────────────────────────────────────────────────────────────────────────────────┐
│ ┌ m_mark ──────────┐     ┌ m_tabs : AppTabBar ★ ───────────────────────┐          ┌──────┐│
│ │ MarkButton ★     │ ·12·│ [도감][아이템][타운맵][스쿼드][설정]          │·12· ~~~~ │ ▾ 4세대││
│ │ 캡슐 36 + POKESIX│     │ TabButton ★ × 5, 간격 4, 아래 정렬           │ stretch  │m_gener-││
│ │ 세로 가운데      │     │ (켜진 탭이 막대 아래 먹선을 덮는다)          │          │ation  ││
│ └──────────────────┘     └──────────────────────────────────────────────┘          └──────┘│
│══════════════════════════════════ 먹선 3px ═══════════════════════════════════════════════│
└──────────────────────────────────────────────────────────────────────────────────────────┘
```

| 순서 | 항목 | 정렬 |
|---|---|---|
| 1 | `m_mark : MarkButton ★` | 세로 가운데 |
| 2 | `addSpacing(12)` | — |
| 3 | `m_tabs : AppTabBar ★` | **아래**(`AlignBottom`) — 켜진 탭(높이 45)이 막대 바닥의 먹선 위로 올라오게 |
| 4 | `addSpacing(12)` + `addStretch()` | 남는 폭을 먹어 세대 버튼을 오른쪽 끝으로 민다 |
| 5 | `m_generation : GenerationButton ★`(Compact) | 세로 가운데 |

- 왼쪽 여백 13은 "20 − 마크 버튼의 점선 테 자리 7"이다 — hover 테가 그려질 자리를 위젯 크기에 넣었기 때문
- 막대는 **무엇이 눌렸는지 알리기만** 한다: 시그널 `homeRequested()` · `pageSelected(Page)`. 화면을 바꾸는 건 MainWindow다
  - `m_mark::clicked → AppBar::homeRequested` (시그널 → 시그널 중계)
  - `m_tabs::pageSelected → AppBar::pageSelected` (중계)

### AppTabBar · TabButton

```
m_tabs : AppTabBar ★     [H] 여백 0 · 간격 4, 크기 정책 Fixed
├ m_group : QButtonGroup(this)   exclusive — 버튼을 "참조"만 한다(소유 X). id = int(Page)
└ TabButton ★ × 5  ⟨paint⟩      apptabbar.cpp 안의 내부 클래스, Q_OBJECT 없음(clicked만 씀)

  켜짐:  높이 45 · 종이색 · 빨강 글자 · 위 · 좌 · 우 먹선 3 (아래 없음 → 페이지와 이어져 보임)
  꺼짐:  높이 38 · red.deep · 흰 글자 · 바닥 먹선 위에 선다
  sizeHint 폭 = 좌우 여백 + 아이콘 16 + 7 + 글자 폭  (켜지면 여백이 커져 폭이 바뀐다 → toggled에서 updateGeometry)
```

`QButtonGroup::idClicked`는 **사용자 클릭에만** 나온다. 그래서 `setCurrentPage()`(코드가 켜는 것)는 시그널을 내지 않고, 되먹임 고리가 생기지 않는다.

### MarkButton

`QAbstractButton` ★ ⟨paint⟩. 자식 없음. 캡슐 마크(인라인 SVG → `QPixmap`) + "POKE"(흰색) "SIX"(연노랑).
hover · 키보드 포커스 때 연노랑 2px 점선 테 — 포커스 테는 Tab/Backtab으로 들어왔을 때만(마우스 클릭으로 받은 포커스에는 없음).
툴팁 "처음 화면으로". 크기 정책 Fixed, 높이 50.

## 4. 시그널 흐름 — 화면이 바뀌는 길

```
① 인트로 메뉴로 들어가기
   IntroMenuItem::clicked  /  IntroMenu에서 Enter · 1–3
     → IntroMenu::activated(i)
     → HomePage 람다: {Squad, Dex, Items, Map}[i]          ← 인트로 순서 ≠ Page 순서, 표로 바꾼다
     → HomePage::openRequested(page)
     → MainWindow::open(page)

② 탭
   TabButton 클릭 → QButtonGroup::idClicked(id) → AppTabBar::pageSelected(Page(id))
     → AppBar::pageSelected → MainWindow::open

③ 인트로로 돌아가기
   MarkButton::clicked → AppBar::homeRequested → MainWindow::showIntro     (Ctrl+0도 같은 곳)

④ 세대 동기화 (앱 상태가 진실, 위젯은 그것을 비춘다)
   GenerationButton::generationSelected(n) → AppState::setGeneration(n)
   AppState::generationChanged(n) → GenerationButton::setGeneration(n)     (앱 막대)
                                  → CardBarrel 회전                         (인트로)
                                  → 각 페이지 다시 읽기(보이면 즉시, 아니면 다음 표시 때)

⑤ 첫 실행 데이터 완료
   DataUpdater::finished → MainWindow 람다: m_repository->close(); squad->reloadData()
                         → HomePage::onDataReady (첫 실행 패널 제거, 메뉴 잠금 해제)
```

ROS 2에 빗대면 `AppState`는 **latched 토픽을 내는 상태 노드**다. 위젯은 서로 직접 부르지 않고 `AppState`의 시그널을 구독해서 자기 모양을 맞춘다.
그래서 세대 버튼 · 카드 배럴 · 페이지가 서로를 몰라도 항상 같은 세대를 보인다.

## 5. 앱 전체 객체 트리 (실행 중, 위쪽만)

`[L]` 레이아웃이 부모를 바꿈 · `[S]` 스택의 addWidget · `[C]` setCentralWidget · `(this)` 명시적 부모 · `*` 필요할 때 만들어짐

```
Application (C++ 객체 — 트리 밖)
 ├ m_app : QApplication · m_translator : QTranslator · m_repository : Repository
 ├ m_state : AppState (QObject, 부모 없음) · m_updater : DataUpdater (QObject, 부모 없음)
 └ m_window : MainWindow                                  최상위 창
    └ m_screens : QStackedWidget                           [C]
       ├ home : HomePage                                   [S]   → 03-home.md §객체 트리
       └ shell : QWidget                                   [S]
          ├ QVBoxLayout
          ├ m_appBar : AppBar                              [L]
          │  ├ QHBoxLayout
          │  ├ m_mark : MarkButton                         [L]
          │  ├ m_tabs : AppTabBar                          [L]
          │  │  ├ m_group : QButtonGroup                   (this)  — 버튼을 소유하지 않는다
          │  │  ├ QHBoxLayout
          │  │  └ TabButton × 5                            [L]
          │  └ m_generation : GenerationButton             [L]
          │     └ * GenerationMenu (Qt::Popup)             (this) 누를 때마다 새로, 닫히면 스스로 지움(WA_DeleteOnClose)
          ├ m_pages : QStackedWidget                       [L]
          │  ├ m_dexPage : DexPage                         [S]   → 04-dex.md
          │  ├ ItemsPage                                   [S]   → 06-items.md
          │  ├ TownMapPage                                 [S]   → 07-map.md
          │  ├ squad : SquadPage                           [S]   → 05-squad.md
          │  └ QLabel#pagePlaceholder                      [S]
          └ QShortcut × 6 (Ctrl+1–5, Ctrl+0)               (shell)
```

`QMainWindow` 내부 레이아웃(`QMainWindowLayout`)은 생략했다.
