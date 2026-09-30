# D2b — 전국도감과 지방 도감 나누기

브랜치: `feat/d2b-regional-dex` · 커밋 footer: `Refs: D2b`

## 목표

도감 백과 머리 띠 오른쪽에 **도감 선택 버튼**을 단다. 처음에는 지금처럼 **전국**이 선택되어 있고,
지방 도감을 누르면 그 도감의 **지방 번호 순** 목록으로 바뀐다.

```
┌ 도감 백과  493마리 ─────────────────── [전국] [신오 D·P] [신오 Pt] [성도 HG·SS] ┐
```

세대마다 어떤 버튼이 나올지는 **코드에 적지 않는다.** PokéAPI의 "도감 ↔ 게임 ↔ 세대" 연결을 DB로
옮겨 두고 조회한다. 그래서 세대만 바꾸면 버튼이 저절로 바뀐다.
- 4세대 → 신오(DP) · 신오(Pt) · 성도(HGSS)
- 3세대 → 관동(FRLG) · 호연(RS · E)
- 8세대 → 가라르 · … · 신오(BDSP)

## 완료 조건

- [ ] 도감 백과 머리 띠 오른쪽에 `전국 · 신오 D·P · 신오 Pt · 성도 HG·SS` 버튼 4개가 있고, 처음에는 **전국**이 켜져 있다
- [ ] **신오 D·P**를 누르면 제목이 `151마리`가 되고, 첫 줄이 `1 모부기`, 한카리아스가 `111`번이다
- [ ] **신오 Pt** → `210마리` / **성도 HG·SS** → `256마리`, 첫 줄 `1 치코리타` / **전국** → `493마리`로 돌아온다
- [ ] 지방 도감에서도 검색(이름 · 지방 번호 · 전국 번호)과 머리 칸 정렬이 된다
- [ ] 새 테스트(아래 CP3)를 포함해 `scripts/linux/build.sh --format`이 통과한다

## 먼저 알아야 할 것 — PokéAPI 도감 데이터의 구조

```
generations ─< version_groups ─< versions           (세대 → 게임 묶음 → 버전)
                     │                                4세대: diamond-pearl → 디아루가 · 펄기아
                     │                                       platinum → 기라티나
                     │                                       heartgold-soulsilver → 하트골드 · 소울실버
      pokedex_version_groups                          (도감 ↔ 게임 묶음, 다대다)
                     │
regions ─< pokedexes ─< pokemon_dex_numbers >─ species (도감별 번호)
           5 original-sinnoh  (DP, BDSP)     387 모부기 → 신오 1
           6 extended-sinnoh  (Pt)           445 한카리아스 → 신오 111
           7 updated-johto    (HGSS)         1 이상해씨 → 성도 231 (신오에는 없다)
```

- **버튼 하나 = 도감 하나**다. 한 도감을 여러 게임 묶음이 쓰면(관동도감 = 레드 · 블루 · 피카츄 …)
  버전 이름을 모아 한 버튼에 붙인다.
- **"그 세대의 도감"** = 그 세대 게임 묶음에 연결된 도감이다. 같은 신오도감(5)이라도 4세대(DP)와
  8세대(BDSP) 양쪽에 나온다.
- **전국도감**은 pokedex id 1인데, 표로 조회하지 않는다. 지금처럼 `speciesForGeneration(g)`
  (그 세대까지 나온 종)을 쓰고, 버튼에서는 id 0을 "전국"이라는 뜻으로 쓴다.
- **이름**: PokéAPI에는 도감의 한국어 이름이 없다. 대신 지방의 한국어 이름은 있다
  (`region_names`: 관동 · 성도 · 호연 · 신오 …).
  - 버튼 이름: 지방 이름 (예: "신오")
  - 버전 표시: 영어 이름의 약칭(D · P · Pt · HG · SS). 약칭은 데이터에 없어서 ui에 작은 표로 둔다(CP5).

---

## CP1 — CSV 4개를 더 받는다

**파일**: [src/data/update/csvsource.h](../../src/data/update/csvsource.h)

`pokedexes`와 `pokemon_dex_numbers`는 이미 받고 있다. 받지 않고 있던 4개를 추가한다.
크기와 해시는 고정 커밋 `168b1e8…`에서 잰 값이다.

```cpp
        // ── 공통 블록 (version_groups 아래에)
        {"versions", 907, "70083465865a6a69a9aad2be3fc3915078c6ff740f14a090b1367d8ec9cfc3cd"},
        {"version_names", 8943,
         "23e3e9062f98e1f83d475b9eeac57ddff4375b46c175948a65c4fe8d3e1d87b4"},
        {"region_names", 999, "9380d909d2179a7f3bbad09969a0c89f623be135a100686596b6284c10231b48"},
        // ── 도감 블록 (pokedexes 아래에)
        {"pokedex_version_groups", 274,
         "e842691103129f6ad30b8a4de3503edfa5086f55a117630cbab6fddcfa83e82c"},
```

`kTotalSize`는 목록을 합산해서 계산하므로 손댈 필요가 없다. 첫 실행 패널의 "예상 크기"도 저절로 바뀐다.

**확인**: 빌드한 뒤 개발 도구로 캐시에 받는다. 이미 받은 파일은 해시가 맞으면 건너뛰므로, 새 4개만 받는다.

```bash
cmake --build --preset linux-debug
build/linux-debug/tools/fetchcsv/pokesix-fetch-csv
# 예상: [40/40] … done: ~/.cache/YamadaStudio/PokeSix/pokeapi-csv/168b1e8…
```

## CP2 — 스키마와 변환

**파일**: [src/data/db/schema.h](../../src/data/db/schema.h), [src/data/update/csvimporter.h/.cpp](../../src/data/update/csvimporter.cpp)

### 2-1. 표 추가 (schema.h)

```cpp
inline constexpr int kVersion = 2; // TODO 1: 1 → 2. 기존 DB는 isUsable()이 false → 첫 실행 패널이 다시 뜬다

        // TODO 2: kStatements 끝에 추가
        R"(CREATE TABLE regions (id INTEGER PRIMARY KEY, name_ko TEXT, name_en TEXT, name_ja TEXT))",

        // sort_order: 게임이 나온 순서(version_groups.order). 버튼 순서에 쓴다
        R"(CREATE TABLE version_groups (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, generation INTEGER NOT NULL,
        sort_order INTEGER NOT NULL))",

        R"(CREATE TABLE versions (
        id INTEGER PRIMARY KEY, version_group_id INTEGER NOT NULL REFERENCES version_groups(id),
        identifier TEXT NOT NULL, name_ko TEXT, name_en TEXT, name_ja TEXT))",

        R"(CREATE TABLE pokedexes (
        id INTEGER PRIMARY KEY, identifier TEXT NOT NULL, region_id INTEGER REFERENCES regions(id),
        is_main_series INTEGER NOT NULL))",

        R"(CREATE TABLE pokedex_version_groups (
        pokedex_id INTEGER NOT NULL REFERENCES pokedexes(id),
        version_group_id INTEGER NOT NULL REFERENCES version_groups(id)))",

        // 도감별 번호. 목록은 (pokedex_id, number) 순서로 읽는다
        R"(CREATE TABLE dex_numbers (
        pokedex_id INTEGER NOT NULL REFERENCES pokedexes(id),
        species_id INTEGER NOT NULL REFERENCES species(id), number INTEGER NOT NULL))",
        R"(CREATE INDEX dex_numbers_dex ON dex_numbers (pokedex_id, number))",
```

> 스키마 첫머리 주석에 `[도감]` 절을 한 단락 추가해 두면 좋다: "버튼 = 도감, 세대 → 게임 묶음 → 도감".

### 2-2. 변환 함수 (csvimporter)

`importGenerations`와 같은 모양이다. `Insert` + `forEachRecord` 한 쌍이면 된다.

```cpp
// csvimporter.h — private:
    bool importRegions(QSqlDatabase &db);       // TODO 3
    bool importVersionGroups(QSqlDatabase &db); // TODO 4
    bool importVersions(QSqlDatabase &db);      // TODO 5
    bool importPokedexes(QSqlDatabase &db);     // TODO 6: pokedexes + pokedex_version_groups + dex_numbers
```

| TODO | 읽을 CSV · 열 | 넣을 표 | 주의 |
|---|---|---|---|
| 3 | `region_names`: `region_id, local_language_id, name` | `regions` | 한 지방이 언어마다 한 줄이다. `importSpecies`가 이름을 ko(3) · en(9) · ja(11)로 모으는 방식(QHash에 모았다가 INSERT)을 그대로 쓴다 |
| 4 | `version_groups`: `id, identifier, generation_id, order` | `version_groups` | `order`는 SQL 예약어라서 열 이름을 `sort_order`로 바꿔 둔 것이다 |
| 5 | `versions`: `id, version_group_id, identifier` + `version_names` | `versions` | 이름은 3과 같은 방식으로 모은다 |
| 6 | `pokedexes`: `id, identifier, region_id, is_main_series` | `pokedexes` | `region_id`가 빈 칸이면 NULL. 전국 · conquest처럼 지방이 없는 도감이다. 이미 있는 `intOrNull()`을 쓴다 |
| 6 | `pokedex_version_groups`: `pokedex_id, version_group_id` | 같은 이름 | |
| 6 | `pokemon_dex_numbers`: `species_id, pokedex_id, pokedex_number` | `dex_numbers` | 8천 줄이다. 이미 한 트랜잭션 안에서 돌고 있어서 빠르다 |

```cpp
// csvimporter.cpp — run()의 연결 순서. 참조하는 표(species · regions · version_groups)가 먼저다.
            ok = ok && createSchema(db) && importGenerations(db) && importTypes(db)
                 && importTypeChart(db) && importStats(db) && importSpecies(db) && importPokemon(db)
                 && importPokemonTypes(db) && importPokemonStats(db)
                 // TODO 7: && importRegions(db) && importVersionGroups(db) && importVersions(db)
                 //         && importPokedexes(db)
                 && writeMeta(db);
```

### 2-3. 테스트 시드 CSV

**파일**: `tests/fixtures/pokeapi-csv/`. 시드 README의 규칙대로, 실제 파일의 줄을 **그대로** 옮긴다.

```bash
C=~/.cache/YamadaStudio/PokeSix/pokeapi-csv/168b1e89467054cda2e7df43ccebbb69b459497a
F=tests/fixtures/pokeapi-csv
cp $C/pokedexes.csv $C/pokedex_version_groups.csv $C/versions.csv $F/          # 작은 표는 통째로
awk -F, 'NR==1 || $2==3 || $2==9 || $2==11' $C/version_names.csv > $F/version_names.csv
awk -F, 'NR==1 || $2==3 || $2==9 || $2==11' $C/region_names.csv  > $F/region_names.csv
# 도감 번호는 시드 5종(1 · 35 · 36 · 37 · 445)만
awk -F, 'NR==1 || $1==1 || $1==35 || $1==36 || $1==37 || $1==445' $C/pokemon_dex_numbers.csv \
    > $F/pokemon_dex_numbers.csv
```

README의 "통째로 둔 작은 표" 목록에 `pokedexes` · `pokedex_version_groups` · `versions`를 추가하고,
이름 표 목록에 `version_names` · `region_names`를 추가한다.

**확인**: 기존 테스트 39개가 그대로 통과해야 한다. 스키마와 변환만 바꿨으니 조회 결과는 달라지면 안 된다.

## CP3 — 조회 (Repository)

**파일**: [src/data/repository/repository.h/.cpp](../../src/data/repository/repository.cpp), [tests/data/repository_test.cpp](../../tests/data/repository_test.cpp)

### 3-1. 타입과 종족값 채우기를 함수로 뺀다

`speciesForGeneration()`의 2) 타입, 3) 종족값 부분은 지방 도감 목록에도 똑같이 필요하다.
그래서 먼저 private 함수로 빼낸다. 동작이 바뀌지 않는 리팩터링이므로, 빼낸 뒤 테스트가 그대로 통과하면 된다.

```cpp
// repository.h — private:
    // rows의 기본 모습(pokemonId)에 세대 generation의 타입 · 종족값을 채운다
    void fillTypesAndStats(QList<SpeciesRow> &rows, int generation); // TODO 8
```

> `rowOfPokemon`(pokemon id → 줄 위치) 해시도 이 함수 안에서 `rows`를 보고 만든다.

### 3-2. 새 타입과 함수

```cpp
struct SpeciesRow
{
    int speciesId = 0; // 전국도감 번호
    int dexNumber = 0; // TODO 9: 보고 있는 도감의 번호(전국이면 speciesId와 같다)
    ...
};

// 도감 선택 버튼 하나
struct DexInfo
{
    int pokedexId = 0;         // 0 = 전국
    QString regionKo;          // "신오". 전국이면 비어 있다
    QStringList versionsEn;    // {"Diamond", "Pearl"} — 약칭은 ui가 붙인다
    QStringList versionsKo;    // {"디아루가", "펄기아"} — 툴팁
};

    // 세대 generation 게임들의 지방 도감(본편만, 게임이 나온 순서). 전국은 넣지 않는다.
    QList<DexInfo> dexesForGeneration(int generation); // TODO 10
    // 지방 도감 pokedexId의 종(도감 번호 순). 타입 · 종족값은 generation 기준.
    QList<SpeciesRow> speciesForDex(int pokedexId, int generation); // TODO 11
```

TODO 10은 질의 하나로 가져와서 C++에서 도감별로 묶는다.

```sql
SELECT d.id, r.name_ko, v.name_en, v.name_ko
FROM pokedexes d
JOIN pokedex_version_groups pvg ON pvg.pokedex_id = d.id
JOIN version_groups vg ON vg.id = pvg.version_group_id
JOIN versions v ON v.version_group_id = vg.id
LEFT JOIN regions r ON r.id = d.region_id
WHERE vg.generation = :g AND d.is_main_series = 1 AND d.region_id IS NOT NULL
ORDER BY vg.sort_order, v.id
```

```cpp
    // 줄마다 (도감, 버전) 하나가 나온다. 같은 도감이 처음 나오면 DexInfo를 새로 만들고, 다음부터는
    // 그 DexInfo에 버전 이름만 더한다. 순서를 지키려면 QHash<int, qsizetype>(도감 id → 위치) + QList.
    // (speciesForGeneration의 rowOfPokemon과 같은 방식)
```

TODO 11은 `speciesForGeneration`의 1)번 질의에서 조건만 바뀐다.

```sql
SELECT s.id, p.id, s.name_ko, s.name_en, s.name_ja, dn.number
FROM dex_numbers dn
JOIN species s ON s.id = dn.species_id
JOIN pokemon p ON p.species_id = s.id AND p.is_default = 1
WHERE dn.pokedex_id = :dex
ORDER BY dn.number
```

그다음 `fillTypesAndStats(rows, generation)`을 부른다. `speciesForGeneration()`에서는 `row.dexNumber = row.speciesId`로 채운다.

### 3-3. 테스트 (시드 데이터 기준 — 실제 번호)

```cpp
TEST_F(RepositoryTest, ListsTheRegionalDexesOfAGeneration)
{
    // TODO 12: dexesForGeneration(4) → pokedexId {5, 6, 7} 순서, regionKo {"신오","신오","성도"},
    //          [0].versionsEn == {"Diamond","Pearl"}, [2].versionsEn == {"HeartGold","SoulSilver"}
}

TEST_F(RepositoryTest, RegionalDexUsesItsOwnNumbers)
{
    // TODO 13: speciesForDex(5, 4) — 신오(DP)
    //   시드 5종 중 삐삐 100 · 픽시 101 · 한카리아스 111만 있다(이상해씨 · 식스테일은 없다) → 3줄,
    //   dexNumber 순서 {100, 101, 111}
    // speciesForDex(7, 4) — 성도(HGSS): 삐삐 41 · 픽시 42 · 식스테일 127 · 이상해씨 231 → 4줄
    // 한카리아스의 types는 {"dragon","ground"}, total은 600 (fillTypesAndStats가 채웠는지)
}
```

## CP4 — 모델이 도감 번호를 쓰게 한다

**파일**: [src/data/models/speciestablemodel.cpp](../../src/data/models/speciestablemodel.cpp)

```cpp
    // TODO 14: NumberColumn의 DisplayRole · SortRole → row.dexNumber (지금은 speciesId)
    // TODO 15: SearchTextRole에 dexNumber도 넣는다: "%1 %2 %3 %4 %5"(지방 번호 · 전국 번호 · 이름 3개)
    //          → 신오도감에서 "111"로도 "445"로도 한카리아스를 찾는다
```

> 테스트의 `ProxySearchesNamesAndNumbers` · `ProxySortsNumbersAsNumbers`가 `NumberColumn`을 읽는다.
> 전국 목록에서는 dexNumber = speciesId라서 그대로 통과해야 한다.

## CP5 — 도감 선택 버튼

### 5-1. 머리 띠 오른쪽 자리 (PanelFrame)

**파일**: [src/ui/widgets/panelframe.h/.cpp](../../src/ui/widgets/panelframe.cpp)

머리 띠는 레이아웃 **바깥**이다. 레이아웃은 `chromeMargins`만큼 들인 안쪽 사각형만 다루기 때문이다.
그래서 머리 띠에 올릴 위젯은 **`resizeEvent`에서 직접 자리를 잡는다**(`setGeometry`).
레이아웃 없이 위젯을 배치하는 첫 경험이다.

```cpp
// panelframe.h
    // 머리 띠 오른쪽에 위젯을 올린다(style.header > 0일 때). 소유권은 PanelFrame이 가져간다.
    void setHeaderWidget(QWidget *widget); // TODO 16
protected:
    void resizeEvent(QResizeEvent *event) override; // TODO 17
private:
    QWidget *m_headerWidget = nullptr;
```

```cpp
void PanelFrame::setHeaderWidget(QWidget *widget)
{
    // TODO 16: widget->setParent(this) — 레이아웃에 넣지 않으니 부모를 직접 정해야 object tree에 들어가고
    //          화면에도 나타난다. m_headerWidget = widget; widget->show(); 그리고 자리 잡기 함수 호출
}

void PanelFrame::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // TODO 17: m_headerWidget이 없으면(nullptr) 그냥 돌아간다 — 머리 위젯이 없는 창도 이 함수를 탄다.
    //   있으면 sizeHint() 크기로, 머리 띠 오른쪽 끝(안쪽 여백 8)에, 세로 가운데에:
    //   const QSize size = m_headerWidget->sizeHint();
    //   const int left = width() - m_style.outline - 8 - size.width();
    //   const int top = m_style.outline + (m_style.header - size.height()) / 2;
    //   m_headerWidget->setGeometry(left, top, size.width(), size.height());
    //   (이름을 x · y로 하지 말 것: QWidget::x() · y() 멤버 함수와 겹친다)
}
```

> 버튼 수가 바뀌면(세대 변경) `sizeHint`도 바뀐다. 그때도 자리를 다시 잡아야 하니,
> 자리 잡는 코드를 `placeHeaderWidget()` 같은 함수로 빼서 `setHeaderWidget` · `resizeEvent` 양쪽에서 부른다.

### 5-2. 버튼 줄 (새 위젯)

**새 파일**: `src/ui/dex/dexselector.h/.cpp` (src/ui/CMakeLists.txt에 추가)

[AppTabBar](../../src/ui/shell/apptabbar.cpp)와 같은 방식이다: `QButtonGroup` + `idClicked`.
다른 점은 버튼을 직접 그리지 않고 **체크 가능한 `QPushButton` + QSS**로 만든다는 것이다.

```cpp
class DexSelector : public QWidget
{
    Q_OBJECT
public:
    explicit DexSelector(QWidget *parent = nullptr);
    // 버튼을 다시 만든다: [전국] + dexes. 선택은 전국으로. (세대가 바뀔 때도 부른다 — A8)
    void setDexes(const QList<DexInfo> &dexes); // TODO 18
signals:
    void dexSelected(int pokedexId); // 0 = 전국
private:
    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
};
```

```cpp
void DexSelector::setDexes(const QList<DexInfo> &dexes)
{
    // TODO 18
    // 1) 기존 버튼 지우기: for (QAbstractButton *b : m_group->buttons()) { m_group->removeButton(b); delete b; }
    //    (delete하면 레이아웃에서도 저절로 빠진다 — object tree의 소멸자가 부모와 레이아웃에 알린다)
    // 2) 버튼 만들기: QPushButton *b = new QPushButton(글자); b->setCheckable(true);
    //    b->setObjectName("dexButton"); b->setToolTip(버전 한국어 이름들 " · "로);
    //    m_layout->addWidget(b); m_group->addButton(b, pokedexId);   // id = pokedexId
    //    글자: 전국 → tr("전국"), 지방 → regionKo + " " + 약칭들(" · "로)  예: "신오 D · P"
    // 3) 전국 버튼 setChecked(true)
    // 4) updateGeometry() — sizeHint가 바뀌었다고 알린다(PanelFrame이 자리를 다시 잡게)
}
// 생성자: m_group->setExclusive(true);
//         connect(m_group, &QButtonGroup::idClicked, this, &DexSelector::dexSelected);
```

**버전 약칭**은 데이터에 없으니 cpp 안에 작은 표로 둔다(타입 색 표와 같은 "보여 주기 규칙"이다).

```cpp
// 영어 버전 이름 → 약칭. 표에 없으면 영어 이름 그대로(새 게임이 나와도 깨지지 않는다).
constexpr std::pair<const char *, const char *> kVersionShort[] = {
        {"Diamond", "D"}, {"Pearl", "P"}, {"Platinum", "Pt"},
        {"HeartGold", "HG"}, {"SoulSilver", "SS"},
        // 다른 세대는 A8에서 세대 전환이 생길 때 채운다
};
```

**QSS** ([resources/styles/app.qss](../../resources/styles/app.qss)): 빨강 띠 위의 작은 버튼. 높이는 26 정도(띠 38 안에 여유 6씩).

```css
/* ── 도감 선택 (도감 백과 머리 띠 오른쪽) ── */
QPushButton#dexButton {
  font-family: "Do Hyeon"; font-size: 15px; color: @text.1;
  background: @white; border: 2px solid @ink; border-radius: 6px;
  padding: 2px 10px; min-height: 18px;
}
QPushButton#dexButton:hover   { background: @yellow.tint; }
QPushButton#dexButton:checked { background: @yellow; }
```

> 버튼 사이 간격은 `m_layout->setSpacing(6)`, 바깥 여백은 `setContentsMargins(0, 0, 0, 0)`이다.
> (A2에서 겪은 기본 여백 문제)

### 5-3. DexPage에 연결

**파일**: [src/ui/dex/dexpage.h/.cpp](../../src/ui/dex/dexpage.cpp)

```cpp
    // 생성자: m_selector = new DexSelector; m_panel->setHeaderWidget(m_selector);
    //         connect(m_selector, &DexSelector::dexSelected, this, &DexPage::showDex);   // TODO 19
    // load():  m_selector->setDexes(m_repository->dexesForGeneration(kGeneration));  // TODO 20
    //          그리고 showDex(0)
    // TODO 21: void showDex(int pokedexId)
    //   0이면 speciesForGeneration(kGeneration), 아니면 speciesForDex(pokedexId, kGeneration)
    //   → m_model->setRows(...); 정렬은 번호 오름차순으로 되돌린다(sortByColumn); updateTitle();
```

> 정렬을 되돌리는 이유: 합계 순으로 보다가 도감을 바꾸면 합계 순이 유지되는 게 이상하지는 않다.
> 하지만 "도감을 고른다 = 그 도감 순서로 본다"가 더 자연스럽다. 유지하는 쪽이 좋으면 이 줄만 빼면 된다.

## 확인

```bash
scripts/linux/build.sh --format
scripts/linux/run.sh
```

1. **첫 실행 패널**: 스키마 버전이 올랐으므로 앱을 켜면 첫 실행 패널이 다시 뜬다(정상).
   "데이터 받기"를 누르면 CP1에서 이미 받은 CSV는 건너뛰고, 변환만 0.2초 남짓 걸린다.
2. **도감 탭**: 완료 조건의 숫자를 차례로 확인한다(151 · 210 · 256 · 493, 모부기 1, 한카리아스 111, 치코리타 1).
3. **검색**: 신오 D·P에서 `111`과 `445`로 각각 한카리아스가 나오는지 본다.
4. **툴팁**: 버튼 위에 마우스를 올리면 `디아루가 · 펄기아`가 뜨는지 본다.

## 이번에 하지 않는 것

- **한 게임에 도감이 여럿인 세대**(XY 칼로스 3개, SM · USUM 알로라 + 섬 4개): 지금 규칙대로면 "칼로스" 버튼이
  3개 생긴다. 4세대에는 없는 경우라서, 6–7세대를 볼 때 "버튼 = 게임, 도감이 여럿이면 안에서 한 번 더 고른다"로
  바꾼다.
- **지방 모습**(알로라 라이츄 등): 도감 번호가 종 단위라서 기본 모습으로 나온다.
- **버전 배지 색**(레퍼런스의 D 파랑 · P 분홍): 새 토큰이 필요하다. 디자인 요청(`design/requests/`)으로 따로 받는다.
- **세대 전환**: 지금은 4세대 고정이다(A8에서 `AppState`).

다 되면 CP 단위로든 한꺼번에든 "했음"이라고 말해 달라. `git diff main...feat/d2b-regional-dex`로 진단한다.
