# 09. 모델/뷰 — 표가 그려지는 길

도감 · 아이템 목록과 스쿼드 선택 창은 Qt의 **모델/뷰**로 되어 있다. 줄이 수백 개이고 정렬 · 필터가 필요해서다.
(줄 수가 적고 고정된 목록 — 도감 상세의 기술표 등 — 은 모델/뷰 없이 위젯 하나가 그린다 → [04-dex.md](04-dex.md))

## 1. 다섯 역할

```
        data 레이어 (QtCore만)                         ui 레이어 (QtWidgets)
┌───────────────────────────┐   ┌───────────────────────────┐   ┌──────────────────────────────────────┐
│ 모델                       │──▶│ 프록시                     │──▶│ 뷰  QTableView / QListView           │
│ QAbstractTableModel ★      │   │ QSortFilterProxyModel ★    │   │  ├ 머리   QHeaderView ★ (paintSection)│
│ "몇 줄 몇 칸, 칸의 값"      │   │ "어떤 줄을 어떤 순서로"    │   │  ├ 칸     QStyledItemDelegate ★ (paint)│
│ data(index, role)          │   │ filterAcceptsRow           │   │  └ 선택   QItemSelectionModel (자동) │
└───────────────────────────┘   │ sortRole                   │   └──────────────────────────────────────┘
                                 └───────────────────────────┘
```

| 역할 | 질문 | 우리 클래스 |
|---|---|---|
| **모델** | 줄 · 칸이 몇 개고 각 칸의 값은? (색 · 글꼴은 모른다) | `SpeciesTableModel` · `ItemTableModel` · (선택 창) `QStringListModel` |
| **프록시** | 그중 어떤 줄을 보여 주고 어떤 순서로? 원본은 건드리지 않는다 | `SpeciesFilterProxy` · `ItemFilterProxy` · (선택 창) `QSortFilterProxyModel` 그대로 |
| **뷰** | 스크롤 · 선택 · 키보드 · 칸 배치 | `QTableView`(상속 없이 그대로) · `QListView` |
| **delegate** | 칸 하나를 **어떻게 그릴지** | `DexRowDelegate` · `ItemRowDelegate` · `PickerDelegate` |
| **header** | 표 머리를 어떻게 그릴지, 눌러서 정렬 | `DexHeaderView` · `ItemHeaderView` |

ROS 2에 빗대면 모델은 **메시지를 내는 쪽**, 프록시는 **필터 노드**, 뷰는 **rviz 디스플레이**, delegate는 그 디스플레이의 **렌더 플러그인**이다.
모델이 data 레이어(`src/data/models`, QtCore만)에 있는 이유가 이것이다 — 나중에 QML 화면이 와도 모델은 그대로 쓴다.

## 2. role — 칸 하나에 값이 여러 개

`model->data(index, role)`에서 **role**이 "어떤 값을 달라"는 것이다. `Qt::DisplayRole`(보이는 글자) 말고 우리 role을 `Qt::UserRole`부터 정의한다.

| 모델 | 우리 role |
|---|---|
| `SpeciesTableModel` | `SortRole` · `TypesRole` · `SearchTextRole` · `PokemonIdRole` · `LegendaryRole` · `FinalEvolutionRole` · `TotalRole` |
| `ItemTableModel` | `SortRole` · `IdentifierRole` · `CategoryRole` · `PocketRole` · `GenerationsRole` · `InGenerationRole` · `IntroGenerationRole` · `SearchTextRole` · `MachineMoveRole` · `MachineTypeRole` |

- **프록시**는 role로 걸러낸다: 예) `SpeciesFilterProxy::filterAcceptsRow`가 `SearchTextRole` · `TypesRole` · `TotalRole` · `LegendaryRole` · `FinalEvolutionRole`을 읽어 다섯 조건을 AND
- **정렬**은 `setSortRole(SortRole)` — 보이는 글자가 아니라 정렬용 값(숫자 · 한국어 이름)으로. `setSortLocaleAware(true)`로 가나다순
- **delegate**는 role로 그림 재료를 꺼낸다: 예) `TypesRole` → 타입 칩, `PokemonIdRole` → 아이콘

## 3. 도감 표 한 줄이 그려지기까지

```
1. DexPage::load()      Repository → m_model->setRows(rows)       beginResetModel / endResetModel
2. 프록시              filterAcceptsRow로 보일 줄 고르기, SortRole로 정렬
3. QTableView          보이는 줄만 골라 칸마다 delegate->paint(painter, option, index) 호출
4. DexRowDelegate      index.data(role)로 값을 꺼내 그림:
                       줄 바탕(선택 > hover(RowHover) > 홀수 줄 > 흰색) · ▶ · 번호 · 아이콘 · 이름 · 타입 칩 · 능력치 색
5. DexHeaderView       paintSection — 머리 글자 자리를 delegate와 **같은 함수**(contentRect · alignment)로 계산 → 머리와 데이터가 px 단위로 정렬
```

| 바꾸는 일 | 부르는 것 | 뷰가 받는 신호 |
|---|---|---|
| 세대가 바뀌어 줄 전체 교체 | `model->setRows` | `modelReset` |
| 언어만 바뀜 | `model->setLanguage` | `dataChanged`(전체) — 줄은 그대로, 다시 그리기만 |
| 검색어 · 필터 | `proxy->setSearchText` 등 → `invalidateFilter()` | 프록시가 줄 목록을 다시 계산 |
| 아이콘 도착 | `SpriteCache::ready` → `view->viewport()->update()` | 모델은 그대로, 다시 그리기만 |

**인덱스 변환 주의**: 뷰의 index는 **프록시 기준**이다. 원본 모델의 줄 번호가 필요하면 `m_proxy->mapToSource(index)`(예: `DexPage::openDetail`).

## 4. 세 곳의 구성 비교

| | 도감 목록 | 아이템 목록 | 스쿼드 선택 창(`ListPicker`) |
|---|---|---|---|
| 모델 | `SpeciesTableModel`(12칸) | `ItemTableModel`(5칸) | `QStringListModel` — 검색용 글자만 |
| 프록시 | `SpeciesFilterProxy` | `ItemFilterProxy` | `QSortFilterProxyModel`(대소문자 무시) |
| 뷰 | `QTableView#dexTable` | `QTableView#itemsTable` | `QListView#squadPickerList` |
| delegate | `DexRowDelegate` — 모든 칸 | `ItemRowDelegate` | `PickerDelegate` — **바탕만** 칠하고 내용은 호출한 쪽의 painter 함수 |
| header | `DexHeaderView` | `ItemHeaderView` | `HeaderStrip`(QWidget, 머리 대신) |
| hover | `RowHover` | `RowHover` | `RowHover` |
| 줄 높이 | 40 | 40 | 36 · 40 |

`ListPicker`의 방식은 "모델은 최소로, 그리기는 바깥 함수로"다 — 포켓몬 · 기술 · 물건이 각자 모델 · delegate를 만들 필요 없이 같은 창을 쓴다.

## 5. 소유 관계

```
DexPage
├ m_model : SpeciesTableModel      (this)
├ m_proxy : SpeciesFilterProxy     (this)   setSourceModel(m_model)
└ … m_table : QTableView           [L]      setModel(m_proxy)  → 선택 모델(QItemSelectionModel)을 Qt가 만든다
     ├ DexHeaderView               (m_table) setHorizontalHeader — 기본 머리는 Qt가 지운다
     ├ m_delegate : DexRowDelegate (m_table) setItemDelegate
     └ RowHover                    (m_table)
```
뷰는 모델 · delegate를 **소유하지 않는다**(포인터만). 그래서 모델 · 프록시는 페이지가, delegate · 머리는 뷰가 부모가 되도록 명시적으로 만든다.
