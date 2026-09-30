# E3 — 아이템 대백과 화면 (분류 + 목록)

브랜치: `feat/e3-items-page` · 커밋 footer: `Refs: E3`
기준 이미지: [13_items_1440.png](../../design/handoff-v1/images/screens/13_items_1440.png) · 수치: [Items.dc.html](../../design/handoff-v1/design/source/Items.dc.html)

## 목표

앱 막대의 **아이템** 탭(지금은 "아이템 — 준비 중이에요")을 실제 화면으로 바꾼다. 이번 단계는 왼쪽 **분류 창**과 가운데 **목록 창**까지다.
오른쪽 상세 창은 다음 단계에서 한다.

```
┌ 분류 ────────┐  ┌ 아이템 대백과  진화 · 39개 ───────────────────────────────────┐
│ □ 전체        │  │ [🔍 이름 · 효과 검색            ]                              │
│ ■ 회복        │  │     이름         효과                           1 2 3 4 5 6 7 8 9 │
│ ■ 볼          │  │ ▶ ◎ 각성의돌    어느 특정 포켓몬을 진화시키는 …  ░ ░ ░ ▓ ■ ■ ■ ■ ■ │
│▶■ 진화        │  │   ◎ 얼음의돌    …  [4세대 없음]  (흐리게)        ░ ░ ░ ▯ ░ ░ ■ ■ ■ │
│ …            │  │   …                                                           │
│──────────────│  │                                                               │
│ ☐ 4세대에 있는 것만 │                                                               │
└──────────────┘  └───────────────────────────────────────────────────────────────┘
```

## 이미 준비된 것 (Claude가 만든 데이터 쪽)

화면은 이 부품들을 **가져다 쓰기만** 하면 된다. 도감 화면(`DexPage`)과 같은 구조다.

| 부품 | 파일 | 하는 일 |
|---|---|---|
| `Repository::itemsForGeneration(g)` | [repository.h](../../src/data/repository/repository.h) | 아이템 전부(`ItemRow`). 세대별 존재 비트 + g세대 효과 문구 |
| `ItemTableModel` | [itemtablemodel.h](../../src/data/models/itemtablemodel.h) | 칸: ▶ · 아이콘 · 이름 · 효과 · 세대. `setRows(rows, generation)` |
| `ItemFilterProxy` | [itemfilterproxy.h](../../src/data/models/itemfilterproxy.h) | `setSearchText` · `setCategoryFilter` · `setOnlyInGeneration` |
| `itemstyle` | [itemstyle.h](../../src/ui/theme/itemstyle.h) · [itemstyle.json](../../resources/theme/itemstyle.json) | 분류 묶음 목록(`groups()`: key · label · color)과 `filterFor(key)` |
| `SpriteCache(Kind::Item)` | [spritecache.h](../../src/data/sprites/spritecache.h) | 아이템 아이콘 파일. key = `IdentifierRole` 값("fire-stone") |
| `AppState` | [appstate.h](../../src/data/state/appstate.h) | 지금 세대 + `generationChanged` |

모델이 주는 role:

| role | 값 | 쓰는 곳 |
|---|---|---|
| `Qt::DisplayRole` | 이름 칸 = 이름, 효과 칸 = 효과 문구 | delegate |
| `IdentifierRole` | "fire-stone" | 아이콘 |
| `GenerationsRole` | int 비트. `(bits >> (g - 1)) & 1` = g세대에 있다 | 세대 칸 9개 |
| `InGenerationRole` | bool. 지금 세대에 있나 | 흐리게 · "n세대 없음" |
| `IntroGenerationRole` | 처음 나온 세대 | "n세대 없음" 문구 |

## 완료 조건 (4세대 기준, 실제 데이터에서 잰 값)

- [ ] 아이템 탭에 분류 창(폭 220)과 목록 창이 나란히 보인다(사이 16, 페이지 여백 위 16 · 좌우 20 · 아래 20)
- [ ] 분류는 위에서부터 `전체 · 회복 · 볼 · 기술머신 · 진화 · 배틀 · 나무열매 · 기타`이고, 처음에는 **전체**가 선택되어 있다
- [ ] 제목이 `아이템 대백과` + `전체 · 1258개`이다. **진화**를 누르면 `진화 · 39개`가 된다
- [ ] "4세대에 있는 것만"을 켜면 `진화 · 17개`, 전체는 `480개`가 된다
- [ ] 4세대에 없는 아이템(얼음의돌 등)은 줄 전체가 흐리고(불투명도 55%), 효과 뒤에 `7세대 없음`이 아니라 **`4세대 없음`** 태그가 붙는다
- [ ] 세대 칸 9개: 있는 세대 = 초록, 없는 세대 = 점선, 4세대 칸 = 노란 테 2px
- [ ] 검색 `돌` → 진화 묶음에서 돌 아이템만 남는다. 효과 문구로도 찾는다(`회복`)
- [ ] 세대 버튼으로 7세대를 고르면 얼음의돌이 진하게 바뀌고, 세대 칸의 노란 테가 7로 간다
- [ ] `scripts/linux/build.sh --format` 통과

---

## CP1 — 페이지 뼈대와 연결

**새 파일**: `src/ui/items/itemspage.h/.cpp` (src/ui/CMakeLists.txt에 추가)
**본보기**: [dexpage.h](../../src/ui/dex/dexpage.h) · [dexpage.cpp](../../src/ui/dex/dexpage.cpp)의 생성자 · `showEvent` · `load` · `onGenerationChanged`

```cpp
// itemspage.h
class ItemsPage : public QWidget
{
    Q_OBJECT
public:
    // repository · state는 소유하지 않는다(MainWindow가 소유). DexPage와 같다.
    ItemsPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

protected:
    void showEvent(QShowEvent *event) override; // TODO 1: 처음 보일 때 load() (DexPage와 같게)

private:
    void load();                // TODO 2: m_model->setRows(m_repository->itemsForGeneration(g), g)
    void onGenerationChanged(); // TODO 3: DexPage와 같게(보이면 바로, 아니면 다음 showEvent)
    void updateTitle();         // CP3

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    ItemTableModel *m_model = nullptr;
    ItemFilterProxy *m_proxy = nullptr;
    bool m_loaded = false;
};
```

```cpp
// itemspage.cpp — 생성자
    // TODO 4: 페이지 여백 {20, 16, 20, 20}, 가로 레이아웃(QHBoxLayout), 창 사이 16
    //         [분류 창: 고정 폭 220] [목록 창: 남는 폭(stretch 1)]
    //         지금은 둘 다 빈 PanelFrame으로 둔다(CP2 · CP3에서 채운다).
    //   분류 창: PanelStyle { outline 2, radius 8, shadow 3, fill white, ink, header 38, headerColor tok::kGreen }
    //            m_categoryPanel->setTitle(tr("분류"));
    //   목록 창: 같은 모양, headerColor tok::kRed
```

**MainWindow** ([mainwindow.cpp](../../src/ui/shell/mainwindow.cpp)): 자리 표시 목록에서 "아이템"을 빼고 실제 페이지를 넣는다. 페이지 순서 = `Page` 순서라서 **도감 바로 다음**이어야 한다.

```cpp
    m_pages->addWidget(new DexPage(m_repository.get(), m_state));   // [0] 도감
    // TODO 5: m_pages->addWidget(new ItemsPage(m_repository.get(), m_state)); // [1] 아이템
    const QString names[] = {tr("스쿼드"), tr("설정")};             // TODO 6: "아이템" 빼기
```

**확인**: 아이템 탭(Ctrl+2)에 초록 머리 "분류" 창과 빨강 머리 창이 나란히 보인다.

## CP2 — 분류 창

**새 파일**: `src/ui/items/categorybutton.h/.cpp` — 분류 한 줄.
**본보기**: [dexselector.cpp](../../src/ui/dex/dexselector.cpp)의 `DexButton` (QAbstractButton을 상속해 직접 그리기)

모양 (Items.dc.html의 분류 버튼):

| 부분 | 값 |
|---|---|
| 줄 | 높이 44 · 좌우 여백 10 · 반경 6 · 칸 사이 10: [▶ 자리 12] [견본] [글자] |
| 선택됨 | 바탕 `yellow.tint` + 먹선 2 + ▶ 자리에 먹색 삼각형(폭 9 · 높이 12). 선택 안 됨은 테두리 없음 |
| 견본 | 16×16 · 반경 4 · 먹선 1.5 · 바탕 = `itemstyle::Group::color` |
| 글자 | 도현 18 · `text.1` |

```cpp
class CategoryButton : public QAbstractButton
{
public:
    CategoryButton(const itemstyle::Group &group, QWidget *parent = nullptr);
    QSize sizeHint() const override;              // TODO 7: {폭은 레이아웃이 정하니 아무 값, 높이 44}
protected:
    void paintEvent(QPaintEvent *event) override; // TODO 8: 위 표대로. isChecked()면 선택 모양
private:
    itemstyle::Group m_group;
};
// 생성자: setCheckable(true), setCursor(Qt::PointingHandCursor), setText(group.label)
//         setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed)  — 가로는 창 폭만큼
```

ItemsPage에서 분류 창 몸통을 만든다:

```cpp
    // TODO 9: 분류 창 body = QVBoxLayout(여백 10, 사이 4)
    //   for (const itemstyle::Group &group : itemstyle::groups()) {
    //       CategoryButton *button = new CategoryButton(group);
    //       layout->addWidget(button);
    //       m_groupButtons->addButton(button);   // QButtonGroup, setExclusive(true)
    //       connect(button, &QAbstractButton::clicked, this, [this, key = group.key] { selectGroup(key); });
    //   }
    //   첫 버튼(전체)을 setChecked(true)
    //   layout->addStretch();
    //   아래: 점선 구분(생략 가능) + QCheckBox m_onlyThisGeneration + 설명 QLabel("끄면 다른 세대 아이템도 흐리게 함께 보여요.")
    //   체크 글자: tr("%1세대에 있는 것만").arg(세대) — 세대가 바뀌면 글자도 바꾼다(onGenerationChanged)
```

```cpp
void ItemsPage::selectGroup(const QString &key)
{
    // TODO 10: m_groupKey = key; m_proxy->setCategoryFilter(itemstyle::filterFor(key)); updateTitle();
}
// 생성자 끝에서 selectGroup(itemstyle::kAll) — "전체"도 숨길 분류는 빼야 하니 꼭 한 번 부른다
// TODO 11: connect(m_onlyThisGeneration, &QCheckBox::toggled, …) → m_proxy->setOnlyInGeneration(on); updateTitle();
```

> **왜 `filterFor(key)`를 쓰나**: PokéAPI 분류는 55가지다. 어느 분류가 "진화"인지는 `itemstyle.json`에 있다.
> 화면은 key("evolution")만 넘기고, 규칙은 모른다. 규칙을 바꾸고 싶으면 JSON만 고친다.

**확인**: 분류를 누를 때마다 선택 모양이 옮겨 간다(목록은 아직 없다).

## CP3 — 목록 창 (표 · 검색 · 제목)

**본보기**: DexPage 생성자의 표 설정 부분(`m_table` ~ `sortByColumn`)과 검색 디바운스(`m_searchDelay`)

```cpp
    // TODO 12: 목록 창 body = QVBoxLayout(여백 12, 8, 12, 0 · 사이 8)
    //   [SearchField(tr("이름 · 효과 검색")) + addStretch]
    //   [QTableView m_table (stretch 1)]
    // TODO 13: 표 설정 — DexPage와 같게:
    //   setModel(m_proxy), 선택 = 줄 단위 · 하나만, 편집 없음, 격자 없음, 테두리 없음, 세로 머리 숨김,
    //   줄 높이 34, 정렬 켜기 + 이름 오름차순(가나다), 가로 스크롤 끔
    // TODO 14: 칸 폭 — Fixed, 효과 칸만 Stretch
    //   ▶ 24 · 아이콘 38 · 이름 122 · 효과(남는 폭) · 세대 186 (= 18 × 9 + 2 × 8 + 좌우 4)
    // TODO 15: 검색 디바운스 150ms → m_proxy->setSearchText(text); updateTitle();
```

```cpp
void ItemsPage::updateTitle()
{
    // TODO 16: "아이템 대백과" + "진화 · 39개"
    //   묶음 이름 = itemstyle::groups()에서 m_groupKey의 label, 개수 = m_proxy->rowCount()
    //   m_listPanel->setTitle(tr("아이템 대백과"), tr("%1 · %2개").arg(label).arg(count));
}
```

**확인**: 기본 delegate라 글자만 나오지만, 완료 조건의 숫자(1258 · 39 · 17 · 480)가 제목에 맞게 나와야 한다.

## CP4 — 줄 그리기 (delegate)

**새 파일**: `src/ui/items/itemrowdelegate.h/.cpp`
**본보기**: [dexrowdelegate.cpp](../../src/ui/dex/dexrowdelegate.cpp)의 `paint()` · `paintIcon()`

```cpp
void ItemRowDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                            const QModelIndex &index) const
{
    painter->save();
    // TODO 17: 줄 바탕 — DexRowDelegate와 같다(선택 yellow.tint > 홀수 줄 paper.alt > 흰색) + 아래 선 line.soft
    // TODO 18: 지금 세대에 없으면 흐리게:
    //   if (!index.data(ItemTableModel::InGenerationRole).toBool()) painter->setOpacity(0.55);
    //   (바탕을 칠한 뒤에 부른다 — 바탕까지 흐리면 줄무늬가 어긋나 보인다)
    switch (index.column()) {
    // TODO 19: CursorColumn — 선택된 줄이면 ▶ (DexRowDelegate에서 그대로)
    // TODO 20: IconColumn — SpriteCache(Kind::Item)에서 IdentifierRole로 파일을 받아 26×26 안에.
    //          파일이 없으면(기술머신 등) 지름 26 원(먹선 1.5, 바탕 paper.alt)을 자리 표시로
    // TODO 21: NameColumn — 나눔고딕 13 ExtraBold, text.1
    // TODO 22: EffectColumn — 나눔고딕 12, text.2. 칸보다 길면 말줄임:
    //            QFontMetrics(font).elidedText(text, Qt::ElideRight, 폭)
    //          없는 세대면 뒤에 태그 "n세대 없음"(n = 모델의 generation()) — 10px 굵게, 점선 1.5 text.disabled, 반경 3
    // TODO 23: GenerationsColumn — 칸 9개(18×16, 사이 2, 반경 3), g = 1…9:
    //            있음: 바탕 green + 먹선 1.5 / 없음: 흰 바탕 + 점선 1.5 line.strong
    //            g == 지금 세대: 테를 노랑(yellow) 2px로 (있음 · 없음 둘 다)
    //          점선 펜: QPen pen(color, 1.5); pen.setStyle(Qt::DashLine);
    }
    painter->restore();
}
```

> 지금 세대는 모델이 안다(`ItemTableModel::generation()`). delegate에서는
> `static_cast<const ItemTableModel *>(proxy->sourceModel())`로 꺼내거나, `setGeneration(int)`을 delegate에 두고 ItemsPage가 넣어 준다.
> 두 번째 쪽이 delegate가 모델 종류를 몰라도 되어서 깔끔하다.

아이콘이 받아지면 표를 다시 그리는 연결도 잊지 말 것. DexPage에 있는 한 줄과 같다.

```cpp
connect(m_sprites, &SpriteCache::ready, m_table->viewport(), qOverload<>(&QWidget::update));
```

**확인**: 완료 조건의 모양(흐림, 태그, 세대 칸, 노란 테)을 스크린샷으로 [13_items_1440.png](../../design/handoff-v1/images/screens/13_items_1440.png)와 비교한다.

## CP5 (선택) — 세대 머리 칸 1–9

머리 칸의 "세대" 글자 대신 `1 2 3 … 9` 숫자를 칸 위치에 맞춰 그리고, 지금 세대 숫자는 노란 바탕으로 한다.
[dexheaderview.cpp](../../src/ui/dex/dexheaderview.cpp)의 `paintSection`이 본보기다.
세대 칸만 다르게 그리고, 나머지 칸은 글자만 그리면 된다.

## 확인

```bash
scripts/linux/build.sh --format
scripts/linux/run.sh
```

- 스키마가 버전 3이 되어 **첫 실행 패널이 한 번 더 뜬다**(정상). "데이터 받기"를 누르면 새 CSV 하나(`item_flavor_text`, 6.3 MB)만 받고 변환한다
- 완료 조건의 숫자를 차례로 확인한다

다 되면 CP 단위로든 한꺼번에든 "했음"이라고 말해 달라. `git diff main...feat/e3-items-page`로 진단한다.
