# 03. 인트로(홈) 화면 — HomePage

앱을 켜면 처음 보이는 화면이다. 앱 막대 없이 창 전체를 쓴다. `MainWindow::m_screens`의 0번 장이다([02-app-shell.md](02-app-shell.md)).
파일: `src/ui/home/*`, `src/ui/widgets/{markwidget,wordmarklabel}.*`

## 1. 배치 그림 (1440×900, 데이터 있음)

```
HomePage ★  ⟨paint⟩ 종이색 + 135° 사선 무늬 + 위 · 아래 빨강 띠(12) + 먹선(3) — 띠는 그리기만, 레이아웃 자리 없음
[V] 여백 (32, 44, 32, 26) · 간격 0
┌────────────────────────────────────────────────────────────────────────────────┐
│████████████████████████ 빨강 띠 12 + 먹선 3 (그림) █████████████████████████████│
│                         ┌ m_mark : MarkWidget ★ 80×80 ┐                          │  가운데 정렬
│                         └─────────────────────────────┘                          │
│                                     ·6·                                          │
│                 ┌ WordmarkLabel ★ "POKESIX" (멤버 없음) ┐                         │  가운데
│                 └──────────────────────────────────────┘                         │
│                          m_subtitleGap (QSpacerItem 11)                          │
│                    ┌ m_subtitle : QLabel#introSubtitle ┐  높이 26                 │  "도감 · 파티 도우미"
│                                     ·8·                                          │
│  ┌ m_fan : CardBarrel ★  ⟨paint⟩ ─── stretch 1 (남는 높이를 가져감) ──────────┐  │
│  │           세대 카드 9장 원통 — 정면 카드 = 선택된 세대                       │  │  최소 640×220
│  └──────────────────────────────────────────────────────────────────────────────┘  │
│                                     ·2·                                          │
│        ┌ m_firstRunBlock : QWidget — 데이터가 없을 때만 ─────────────┐          │
│        │   m_firstRun : FirstRunPanel ★  폭 520                      │          │
│        └──────────────────────────────────────────────────────────────┘          │
│              ┌ m_menu : IntroMenu ★  폭 530 ────────────────────┐               │  가운데
│              │ [V] 여백 5 · 간격 10                              │               │
│              │ ┌ IntroMenuItem ★ Primary "스쿼드"      h62 ─┐   │               │
│              │ ┌ IntroMenuItem ★ Entry   "도감 백과"   h62 ─┐   │               │
│              │ ┌ IntroMenuItem ★ Entry   "아이템 백과" h62 ─┐   │               │
│              │ ┌ IntroMenuItem ★ Entry   "타운맵 백과" h62 ─┐   │               │
│              └───────────────────────────────────────────────────┘               │
│                                  ~stretch~                                       │
│ ┌ IntroFooter ★ : QLabel#introVersion "v…"  ~stretch~ ~stretch~  QLabel#introSource ┐│  전체 폭
│████████████████████████ 먹선 3 + 빨강 띠 12 (그림) █████████████████████████████│
└────────────────────────────────────────────────────────────────────────────────┘
```

| 순서 | 항목 | stretch · 정렬 | 비고 |
|---|---|---|---|
| 1 | `m_mark : MarkWidget` | 0 · 가운데 | `setMarkSize(80)`, 첫 실행(압축 모드)엔 72 |
| 2 | `addSpacing(6)` | | |
| 3 | `WordmarkLabel` | 0 · 가운데 | 멤버로 들고 있지 않다(레이아웃 · 부모가 지닌다) |
| 4 | `m_subtitleGap : QSpacerItem(0, 11)` | | 11 = 16 − 워드마크 그림자 5. 압축 모드에서 0으로 바꾼다(`changeSize`) |
| 5 | `m_subtitle : QLabel#introSubtitle` | 0 · 가운데 | 고정 높이 26 |
| 6 | `addSpacing(8)` | | |
| 7 | `m_fan : CardBarrel` | **1** | Expanding — 남는 세로 공간을 가져간다 |
| 8 | `addSpacing(2)` | | |
| 9 | `m_firstRunBlock` | 0 · 가운데 | **데이터가 없을 때만** 만든다 |
| 10 | `m_menu : IntroMenu` | 0 · 가운데 | 고정 폭 530(= 520 + 포커스 링 자리 5 × 2) |
| 11 | `addStretch()` | | 배럴이 숨겨지면(첫 실행) 이 칸이 빈자리를 먹는다 |
| 12 | `IntroFooter` | 0 · 정렬 없음 | 전체 폭 |

**압축 모드**(`setCompact(true)`, 첫 실행 중): 마크 72, `m_subtitle` · `m_fan` 숨김, 간격 0 → 첫 실행 패널과 메뉴가 화면에 들어오게 한다.
데이터를 다 받으면 `onDataReady()`가 첫 실행 블록을 `deleteLater()`로 지우고 원래 모양으로 돌린다.

## 2. 부품

### IntroMenu ★ — 카드 버튼 목록 + 키보드

```
m_menu : IntroMenu ★  ⟨paint⟩ 선택된 카드 바깥의 파란 포커스 링만(링은 부모가 그린다 — ADR 0016)
[V] 여백 5(= 포커스 링 자리) · 간격 10, 포커스 정책 StrongFocus (메뉴 자체가 포커스를 받는다)
├ 0  IntroMenuItem(Primary) "스쿼드"       — 여섯 자리 파티 편성과 타입 분석
├ 1  IntroMenuItem(Entry)   "도감 백과"
├ 2  IntroMenuItem(Entry)   "아이템 백과"
└ 3  IntroMenuItem(Entry)   "타운맵 백과"
      각 높이 62, 포커스 NoFocus (선택 상태는 메뉴가 들고 있다)
```

- 키보드: ↑↓ 이동(끝에서 돌지 않음) · Enter → `activated(i)` · 숫자 1–3 바로 선택(4번 타운맵은 숫자 키 없음) · ←→는 처리하지 않고 부모(`HomePage`)로 넘겨 배럴을 돌린다
- 마우스: 카드에 올라가면 `hovered` → 선택 이동, 클릭 → `activated(i)`
- 데이터 잠금: `setDataLocked(true)`면 0–2번 카드를 끈다(타운맵은 잠기지 않는다)

### IntroMenuItem ★ — 카드 버튼 하나

`QAbstractButton` ⟨paint⟩, 자식 없음. 먹선 2 · 반경 10 · 그림자 3의 카드, Primary는 빨강 채움.
눌리면 2px 내려가고 그림자 1. 선택되면 안쪽 노란 테 3. 36px 먹색 배지 + 아이콘, 이름(도현 20) · 설명(본문 12), 오른쪽 `›`.
꺼지면 45% 불투명 + "데이터가 필요해요". 높이는 메뉴가 정한다(`sizeHint` 재정의 없음).

### CardBarrel ★ — 세대 카드 원통

`QWidget` ⟨paint⟩. 카드 9장을 **위젯이 아니라 그림으로** 그린다(카드 데이터는 `homecards.json`, `homecards::all()`).

```
m_fan : CardBarrel ★   Expanding, 최소 640×220, 마우스 추적
├ m_fronts : SpriteCache(PokemonFront)   (this) — 카드의 포켓몬 그림
└ m_turnAnimation : QVariantAnimation    (this) — 회전 각도 애니메이션
주입: AppState* (소유 X)
```

| 이벤트 | 동작 |
|---|---|
| `showEvent` | 760ms 등장 회전 |
| `hideEvent` | 돌리던 세대를 확정 |
| 마우스 누름 · 끌기 · 놓기 | 끌어서 돌리기, 옆 카드 클릭 → 정면으로 |
| 휠 | 한 칸씩 |

연결: 애니메이션 `finished` → `AppState::setGeneration` / `AppState::generationChanged` → 그 카드로 회전 / 그림 `ready` · 언어 변경 → `update()`.

### FirstRunPanel ★ — 첫 실행 패널 (데이터가 없을 때만)

```
m_firstRunBlock : QWidget   [V] 여백 (0,0,0,12)
└ m_firstRun : FirstRunPanel ★  ⟨paint⟩ 패널 겉(실패면 빨강 테)   폭 520
   [V] 여백 (17,17,17,21) = 먹선 3 + 안쪽 14 (아래는 그림자 4 더함)
   └ [S] m_pages : QStackedWidget#firstRunPages (배경 투명)     enum State 순서
      ├ 0 Ready        [V] 간격 10
      │   QLabel#firstRunTitle "도감 데이터가 아직 없어요" · #firstRunBody · #firstRunMeta "예상 크기 약 X MB"
      │   m_startButton : ShadowButton(Primary) "데이터 받기"  · ~stretch~
      ├ 1 Downloading  [V] 간격 10
      │   QLabel#firstRunTitle "데이터 받는 중…"
      │   [H] m_stepLabel#firstRunStep  ~stretch~  m_percentLabel#firstRunPercent
      │   m_progress : SegmentProgress ★ (10칸)
      │   QLabel#firstRunNote · m_cancelButton : ShadowButton(Secondary) "취소" · ~stretch~
      └ 2 Failed       [V] 간격 10
          [H 간격 8] 아이콘 QLabel · QLabel#firstRunFailedTitle · ~stretch~
          QLabel#firstRunBody · m_errorLabel#firstRunMeta · m_retryButton : ShadowButton(Primary) "다시 시도" · ~stretch~
```

시그널 `startRequested()` · `cancelRequested()`. 상태 전환은 `setState()`가 스택 번호를 바꾸는 것뿐이다 — 화면 세 장을 미리 만들어 두고 보이는 장만 바꾸는 패턴.

### IntroFooter ★

`QWidget`, 그리기 없음, QLabel 조합. [H] 여백 0 · 간격 0: `QLabel#introVersion` · `~stretch~` · `~stretch~`(키 안내를 뺀 자리에 남은 중복 stretch) · `QLabel#introSource` "데이터: PokéAPI · 비공식 팬 도구".

### MarkWidget ★ · WordmarkLabel ★

| | 그리는 것 | 크기 |
|---|---|---|
| `MarkWidget` | 캡슐 마크 SVG를 `QSvgRenderer`(자식 `m_renderer`)로 `rect()`에 | Fixed, `setMarkSize(n)` → n×n (기본 136, 인트로 80) |
| `WordmarkLabel` | "POKE"(먹색) + "SIX"(빨강) + 노란 그림자 (5,5) — Silkscreen 굵게 80 | Fixed, 글자 폭 + 10 × 85 |

`QLabel`을 쓰지 않은 이유: 한 줄 안에서 글자색이 둘이고, 블러 없는 오프셋 그림자는 QSS로 못 그린다.

## 3. 이벤트와 연결

| 이벤트 · 시그널 | 처리 |
|---|---|
| `HomePage::keyPressEvent` ← / → | `m_fan->selectNeighbor(∓1)` (배럴이 보일 때) |
| `HomePage::showEvent` | 첫 실행 패널이 있으면 그 버튼에, 없으면 `m_menu`에 포커스 |
| `m_menu::activated(i)` | 람다: `{Squad, Dex, Items, Map}[i]` → `openRequested(page)` → `MainWindow::open` |
| 첫 실행만: `m_firstRun::startRequested` / `cancelRequested` | `DataUpdater::start` / `cancel` |
| `DataUpdater::progress` · `failed` · `cancelled` | 패널의 진행 · 실패 · Ready 상태 |
| `DataUpdater::finished` | `HomePage::onDataReady` |

## 4. 객체 트리

```
home : HomePage                                   [S] m_screens
├ QVBoxLayout
├ m_mark : MarkWidget                             [L]
│  └ m_renderer : QSvgRenderer                    (this)
├ WordmarkLabel                                   [L]
├ m_subtitle : QLabel#introSubtitle               [L]
├ m_fan : CardBarrel                              [L]
│  ├ m_fronts : SpriteCache                       (this)
│  └ m_turnAnimation : QVariantAnimation          (this)
├ * m_firstRunBlock : QWidget                     [L]  첫 실행만, 끝나면 deleteLater
│  ├ QVBoxLayout
│  └ m_firstRun : FirstRunPanel                   [L]
│     ├ QVBoxLayout
│     └ m_pages : QStackedWidget#firstRunPages    [L]
│        ├ QWidget(Ready):  QLabel × 3, m_startButton : ShadowButton
│        ├ QWidget(Downloading): QLabel, m_stepLabel, m_percentLabel, m_progress : SegmentProgress, QLabel, m_cancelButton
│        └ QWidget(Failed): QLabel(아이콘), QLabel × 2, m_errorLabel, m_retryButton
├ m_menu : IntroMenu                              [L]
│  ├ QVBoxLayout
│  └ IntroMenuItem × 4                            [L]  (m_items는 참조만)
└ IntroFooter                                     [L]
   ├ QHBoxLayout
   └ QLabel#introVersion, QLabel#introSource      [L]
```
