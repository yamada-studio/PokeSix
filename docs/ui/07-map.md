# 07. 타운맵 백과 — TownMapPage

`m_pages`의 2번(`Page::Map`). 인게임 타운맵 그림 위에 우리 조작(호버 · 클릭 · 휠 줌 · 끌기)을 얹고, 장소를 고르면 오른쪽에 정보를 보인다.
파일: `src/ui/map/*`(지도 그림은 실행 시 받아 캐시 — 커밋하지 않는다)

## 1. 배치 그림

```
TownMapPage ★   [V] 여백 (20,16,20,12) · 간격 14
┌ top [H] 간격 12 ─────────────────────────────────────────────────────────────────────────────────────────────┐
│ [title QLabel#pageTitle "타운맵 백과"] [m_game : GameSelector ★ (묶음 칩)] [m_regionChips [H] 간격 6: (성도)(관동)] ~stretch~ │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
┌ body [H] 간격 16  (stretch 1) ─────────────────────────────────────────────────────────────────────────────────┐
│ ┌ m_mapPanel : PanelFrame ★ (초록 머리)  stretch 1 ──────────────┐ ┌ m_detail : PanelFrame ★ (파랑 머리) 고정 폭 340 ┐│
│ │▓ 성도 · 관동 타운맵  92곳 ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│ │▓ 금빛시티 ▓▓▓▓▓ [야생][아이템][랜드마크] ▓▓▓▓▓│ │
│ │ mapBody [V] 여백 (3,2,3,3)                                     │ │   tabs : QWidget = 머리 위젯(레이아웃 밖)       │ │
│ │ ┌ m_view : MapView ★  ⟨paint⟩  최소 320×280 ──────────────┐ │ │   [H] 간격 4, QPushButton#mapInfoTab × 3       │ │
│ │ │ 정수 배율로 키운 타운맵 (없으면 노드 스키매틱)            │ │ │ detailBody [V] 여백 (14,10,14,12)               │ │
│ │ │ "성도" 라벨(왼쪽 위) · "관동" 라벨(오른쪽 위)             │ │ │ ┌ m_empty : QLabel#squadEmpty  stretch 1 ┐    │ │
│ │ │ 선택: 노란 테 · 호버: 먹색 테 + 이름 말풍선               │ │ │ │ "지도에서 장소를 고르면 …"            │    │ │
│ │ │ 휠 = 줌(1–12배, 커서 고정) · 빈 곳 끌기 = 이동            │ │ │ └ 또는 ─────────────────────────────────┘    │ │
│ │ └─────────────────────────────────────────────────────────┘ │ │ ┌ scroll : QScrollArea#squadScroll ──────┐    │ │
│ └────────────────────────────────────────────────────────────────┘ │ │ └ m_detailBody [V] 간격 8 (탭마다 다시) │    │ │
│                                                                    │ └────────────────────────────────────────┘    │ │
│                                                                    └────────────────────────────────────────────────┘ │
└──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘
```

- `m_game`은 게임이 하나뿐인 세대면 숨김. `m_regionChips`의 지방 칩(`QPushButton#mapRegionChip`)은 지방이 둘 이상이고 **합본 지도가 없을 때만** 보인다(HGSS는 "johto-kanto" 합본이 있어 칩이 없다)
- 탭 버튼 셋은 `PanelFrame::setHeaderWidget(tabs)`로 파랑 머리 띠 오른쪽에 놓인다 — 레이아웃 밖, `PanelFrame`이 손으로 배치
- 이 페이지는 **바로 읽는다**: 생성자 끝에서 `refresh()` (아이템 · 도감 페이지와 달리 늦게 읽기가 아니다)

## 2. MapView ★ — 지도 뷰어

`QWidget` ⟨paint⟩. 자식 위젯 없음, 자식 객체는 `m_cache : SpriteCache(TownMap)` 하나(지도 그림 받기).

**그리는 순서** (`paintEvent`)

```
1. 바탕 paper.alt
2. 지도가 없으면 "이 지방의 지도는 아직 준비 중이에요" 하고 끝
3. 지도 그림(레이어를 캔버스에 합성 → 정수 배율 FastTransformation)을 origin()에
   그림이 아직 없으면: 격자 + 노드 사각형 + "지도 그림을 받는 중이에요…" (스키매틱 폴백)
4. 여러 레이어면 레이어 라벨 칩 (첫 레이어 왼쪽 위, 나머지 오른쪽 위)
5. 선택된 장소의 모든 조각에 노란 3px 테
6. 호버 장소(선택과 다르면)에 먹색 2px 테 + 이름 말풍선(위에 자리 없으면 아래로)
```

**좌표 계산** — 한 곳에 모여 있어 그리기와 클릭 판정이 같은 값을 쓴다

| 함수 | 뜻 |
|---|---|
| `fitScale()` | 세로에 맞춘 기본 배율 = max(1, (높이 − 24) / 캔버스 높이) — 넓은 합본은 가로로 끌어서 본다 |
| `scale()` | 기본 배율 + 휠 줌(`m_zoom`) |
| `origin()` | 작으면 가운데, 크면 끌기 위치(`m_pan`, 범위 제한) |
| `nodeAt(pos)` | 그 점을 품은 노드 중 **가장 작은 것** — 마을이 겹친 도로보다 이긴다 |

**이벤트**

| 이벤트 | 동작 |
|---|---|
| `mouseMoveEvent` | 끄는 중이면 이동, 아니면 호버 갱신 + 커서(노드 위 손가락 · 빈 곳 펼친 손) |
| `mousePressEvent` | 빈 곳을 누르면 끌기 시작(노드 위에서는 끌지 않는다) |
| `mouseReleaseEvent` | 4px 넘게 움직였으면 무시(끌기였음), 아니면 그 노드 선택 → 바뀌었으면 `locationSelected(loc)` (빈 곳 = "") |
| `wheelEvent` | 배율 ±1(1–12), **커서 아래 점이 그대로 있게** 이동 보정 |
| `leaveEvent` · `resizeEvent` | 호버 지우기 · 이동 범위 다시 제한 |

공개 API: `setRegion(region, 이름표, 언어, 레이어 라벨)`(줌 · 이동 · 선택 초기화) · `select(loc)`(시그널 없이) · `selected()`. 시그널 `locationSelected(QString)`.

`townmapbook`(네임스페이스, 클래스 아님)이 `resources/data/townmap/*.json`의 캔버스 · 레이어 · 노드(장소별 픽셀 사각형)와 랜드마크 사전을 처음 쓸 때 한 번 읽어 둔다.

## 3. 장소 패널 — 탭마다 다시 만드는 목록

`m_detailBody`의 내용은 **장소를 고르거나 탭을 바꿀 때마다 전부 지우고 다시 만든다**(`showLocation`). 위젯은 모두 `m_detailBody`의 자식이 된다.

| 탭 | 만드는 위젯 |
|---|---|
| 장소 없음 | 제목 "장소", `m_empty` 보이고 스크롤 숨김 |
| **야생** | 방법마다 `QLabel#dexSectionLabel`, 출현마다 줄 `QWidget`[H]: 아이콘 `QLabel`(20px, 캐시에 있을 때만) · 이름 · ~stretch~ · `QLabel#squadCount` "Lv.x–y · r%". 없으면 `QLabel#squadNote` |
| **아이템** | 아이템마다 줄 `QWidget`[H]: 아이콘(18px) · 이름 · ~stretch~, 그 아래 얻는 법마다 `QLabel#squadResources`(들여쓰기 26) |
| **랜드마크** | 시설마다 이름 `QLabel` + 설명 `QLabel#squadResources` (`townmap/landmarks/<게임>.json`) |

탭 버튼은 `QButtonGroup`(exclusive)으로 묶이고, 누르면 `m_infoTab`을 바꾼 뒤 `showLocation(m_view->selected())`를 다시 부른다.
"한 장소의 정보를 다시 그린다"는 함수 하나로 탭 · 장소 · 아이콘 도착을 모두 처리하는 구조다.

## 4. 연결

| 시그널 | 받는 곳 |
|---|---|
| `m_game::versionSelected(v)` | `AppState::setGame(v)` (같은 게임이면 바로 `refresh()`) |
| 지방 칩 `clicked` | `selectRegion(region)` → `m_view->setRegion(...)` · 패널 제목 |
| `m_view::locationSelected(loc)` | `showLocation(loc)` |
| 탭 버튼 `clicked` | 탭 바꾸고 `showLocation` |
| `AppState::generationChanged` · `gameChanged` · `languageChanged` | `refresh()` — 게임 칩 · 지방 칩 다시, 합본 지도 고르기 |
| `m_pokemonIcons::ready` | 선택된 장소가 있으면 `showLocation` 다시(아이콘 채우기) |

## 5. 객체 트리

```
TownMapPage                                             [S] m_pages
├ QVBoxLayout
├ m_pokemonIcons · m_itemIcons : SpriteCache · m_regionGroup : QButtonGroup · tabGroup : QButtonGroup   (this)
├ title : QLabel#pageTitle                              [L]
├ m_game : GameSelector                                 [L]  └ QButtonGroup, * VersionChip × n
├ * QPushButton#mapRegionChip × n                       [L]  (refresh 때 다시)
├ m_mapPanel : PanelFrame                               [L]
│  └ mapBody : QWidget                                  [L]
│     └ m_view : MapView                                [L]
│        └ m_cache : SpriteCache(TownMap)               (this)
└ m_detail : PanelFrame                                 [L]
   ├ tabs : QWidget                                     [H]  └ QPushButton#mapInfoTab × 3
   └ detailBody : QWidget                               [L]
      ├ m_empty : QLabel#squadEmpty                     [L]
      └ scroll : QScrollArea#squadScroll                [L]
         └ viewport └ m_detailBody : QWidget            [SA]
            └ * 탭 내용 (QLabel · 줄 QWidget …)         [L]  (showLocation 때마다 다시)
```
