# H2 — 스쿼드 "불러오기"가 게임 세이브(.sav)를 받는다

> 학습 루프 ① 가이드. 브랜치: `feat/h2-squad-sav-import` (Claude가 만들어 둠)
> 뼈대 · 테스트는 준비돼 있다. **`// TODO(H2-CPn-m)`만 채우면 된다.** 빈칸이 있어도 빌드된다.
> 결정 근거: [ADR 0018](../decisions/0018-save-import-read-only.md) · 이어지는 단계: [H1](h1-save-reader.md)(core 파서, 완료)

## 전체 그림

H1은 "세이브를 **읽을 수 있다**"였다. H2는 그걸 앱에서 **쓸 수 있게** 잇는다 — 스쿼드 화면의 [불러오기]로 `.sav`를 고르거나
파일을 끌어다 놓으면, 게임 속 파티 6마리가 스쿼드에 채워진다.

```
        ┌───────────── 이미 있는 길(.pks 불러오기) ─────────────┐
[불러오기] ─▶ 파일 경로 ─▶ squadfile::load ─▶ Portable ─▶ 세대 · 게임 전환 ─▶ 덮어쓰기 확인 ─▶ replaceSquad
                 │                               ▲
                 │ .sav · .dsv 이면               │  같은 모양으로 돌려준다 — 화면은 거의 안 바뀐다
                 └▶ saveimport::load ────────────┘
                      ├ ① 파일 → QByteArray → save::Bytes(span)      레이어 경계의 타입 변환
                      ├ ② save::readParty (H1)                       게임 번호들
                      ├ ③ 버전 고르기(출신 게임 다수결)               하트골드? 소울실버?
                      └ ④ 번호 변환(Repository)                       물건 · 성격 · 기본 폼 → DB id
```

**핵심 설계**: 세이브를 스쿼드 파일(`.pks`)과 **같은 결과 모양**(`squadfile::Portable`)으로 바꾸기만 하면, 세대 · 게임 전환이나
덮어쓰기 확인 같은 화면 쪽 흐름은 이미 있는 코드를 그대로 쓴다. 새로 짜는 화면 코드는 "어느 변환기를 부를까" 한 줄과 끌어다 놓기뿐이다.

| | 체크포인트 | 파일 | 배우는 것 |
|---|---|---|---|
| ⬜ | **CP1 출신 게임** | `src/core/save/partyreader.cpp` | (H1 복습) 한 칸 더 읽기 |
| ⬜ | **CP2 DB에 게임 번호 칸** | `src/data/db/schema.h` · `src/data/update/csvimporter.cpp` | SQLite 스키마, CSV → INSERT, 스키마 버전 |
| ⬜ | **CP3 번호 조회** | `src/data/repository/repository.cpp` | `QSqlQuery` prepare · bindValue · exec · next |
| ⬜ | **CP4 세이브 → 스쿼드** | `src/data/store/saveimport.cpp` | `QFile` · `QByteArray` ↔ `std::span`, 레이어 경계, `qCInfo(lcData)` |
| ⬜ | **CP5 불러오기 창** | `src/ui/squad/squadpage.cpp` | 함수 뽑아내기, `QFileDialog` 필터 |
| ⬜ | **CP6 끌어다 놓기** | `src/ui/squad/squadpage.cpp` | Qt 이벤트: `dragEnterEvent` · `dropEvent` · `QMimeData` |
| ⬜ | **CP7 실제 확인 = 완료** | 앱 실행 | VS Code로 앱 디버깅 |

### 준비된 것 (Claude가 만들어 둠)

| 파일 | 상태 |
|---|---|
| `src/data/store/saveimport.h` | **완성.** 함수 세 개의 선언과 설명 |
| `src/data/store/saveimport.cpp` | 몸통이 `TODO(H2-CP4-n)` — 단계 ①–⑤ 주석이 길잡이 |
| `src/data/repository/repository.h` | **완성.** 조회 함수 세 개 선언 |
| `repository.cpp` · `schema.h` · `csvimporter.cpp` · `partyreader.cpp` | 고칠 자리에 `TODO(H2-…)` |
| `src/ui/squad/squadpage.{h,cpp}` | `importFile` · `dragEnterEvent` · `dropEvent` 선언과 빈 몸통, 고칠 자리에 TODO |
| `tests/` | 새 테스트 11개 — core 1 · Repository 3 · SaveImport 7. 일회용 참조 구현으로 전부 통과 확인 |

지금 상태: **새 테스트 10개가 빨강**(나머지 132개는 초록). CP마다 해당 테스트가 초록으로 바뀐다.

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug -R "PartyReader|RepositoryTest.Maps|RepositoryTest.Finds|SaveImport" --output-on-failure
```

---

## CP1 — 출신 게임 (`partyreader.cpp`, 1줄)

세이브만으로는 "하트골드 · 소울실버 묶음"까지만 알 수 있다(두 버전의 세이브 구조가 같다). 어느 쪽인지는 파티 포켓몬의
**출신 게임 바이트**(PK4 `0x5F` — 그 게임에서 잡았으면 그 버전)로 가린다. [pk4.md](../data/gen4/pk4.md) 블록 C 표 참고.

- `TODO(H2-CP1)`: `member.originGame = data[0x5F];` 꼴로 한 줄
- 확인: `ctest --preset linux-debug -R PartyReader.ReadsTheOriginGame`

---

## CP2 — DB에 "게임 번호" 칸 더하기 (`schema.h` · `csvimporter.cpp`)

### 왜

세이브의 물건 · 성격 번호는 **게임 번호**라 DB의 id와 다르다(H1 때 손으로 바꿨던 217 → 선제공격손톱).
PokéAPI CSV에는 그 번호표가 이미 있다 — `item_game_indices.csv`의 `game_index`, `natures.csv`의 `game_index`.
지금 DB는 그 칸을 버리고 있어서, 저장하도록 바꾼다.

### ① 스키마 (`schema.h`)

- `TODO(H2-CP2-1)` `item_generations` 표: `generation INTEGER NOT NULL,` 뒤에 `game_index INTEGER,` 를 넣는다
- `TODO(H2-CP2-2)` `natures` 표: `decreased_stat …` 뒤에 `game_index INTEGER,`
- `TODO(H2-CP2-3)` `kVersion`을 **12**로

**스키마 버전을 올리는 이유**: 사용자 PC에는 이미 옛 모양(칸 없음)의 DB가 있다. 앱은 시작할 때 DB의 `meta.schema_version`과
코드의 `kVersion`을 비교해서 **다르면 그 DB를 버리고 새로 만든다**(첫 실행 패널이 다시 뜬다 — CSV는 사용자 캐시에 있어서
다시 받지 않고 변환만 한다, 수 초). 올리지 않으면 옛 DB에 `game_index` 칸이 없어 조회가 실패한다.
ROS 2로 치면 메시지 정의를 바꾸고 인터페이스 버전을 올리는 것과 같다.

### ② 변환기 (`csvimporter.cpp`)

같은 요령으로 두 곳:

- `TODO(H2-CP2-4)` `item_generations` INSERT — SQL 문의 칸 목록에 `game_index`, `VALUES`에 `?` 하나, 읽을 CSV 칸 목록에
  `QStringLiteral("game_index")`, `exec({...})`에 `v[2].toInt()`
- `TODO(H2-CP2-5)` `natures` INSERT — 같은 요령. 이 표는 이름 칸들이 뒤에 붙어 있으니 **칸 순서와 `?` 순서, exec 인자 순서를
  셋 다 맞추는 것**만 조심

`forEachRecord(파일, {칸 이름들}, 람다)`는 CSV의 각 줄에서 **적어 준 칸만** 순서대로 `v[0]`, `v[1]` …로 넘겨준다.

확인은 CP3 테스트로 한다(테스트가 픽스처 CSV로 DB를 새로 만든다).

---

## CP3 — 번호 조회 세 개 (`repository.cpp`)

```cpp
int natureIdForGameIndex(int gameIndex);                // 4 → 17 (개구쟁이)
int itemIdForGameIndex(int generation, int gameIndex); // (4, 234) → 211
int defaultPokemonId(int speciesId);                    // 445 → 445 (한카리아스 기본 폼)
```

### QSqlQuery 네 단계

이 파일의 다른 함수들과 같은 모양이다(예: `Repository::evolutionsWithItem` — 이름 붙은 자리 `:item`):

```cpp
query.prepare(QStringLiteral("SELECT … WHERE game_index = :index"));  // ① SQL 틀 — 값 자리는 :이름
query.bindValue(QStringLiteral(":index"), gameIndex);                  // ② 자리에 값 넣기
if (query.exec() && query.next())                                      // ③ 실행 ④ 첫 줄로
    return query.value(0).toInt();                                      //    SELECT의 첫 칸
return 0;                                                               // 없으면 0
```

값을 SQL 문자열에 직접 이어 붙이지 않고 `bindValue`로 넣는다 — 따옴표 · 이스케이프 문제가 없고, 같은 틀을 재사용할 수 있다
(ROS 2 파라미터처럼 "틀"과 "값"을 분리).

| TODO | 표 · 조건 |
|---|---|
| `H2-CP3-1` | `natures` · `game_index = :index` → `id` |
| `H2-CP3-2` | `item_generations` · `generation = :generation AND game_index = :index` → `item_id` |
| `H2-CP3-3` | `pokemon` · `species_id = :species AND is_default = 1` → `id` |

확인: `ctest --preset linux-debug -R "RepositoryTest.Maps|RepositoryTest.Finds" --output-on-failure` → 3/3

---

## CP4 — 세이브 → 스쿼드 (`saveimport.cpp`)

함수 세 개. 앞의 둘은 몸풀기, `load`가 본론이다. 주석의 ①–⑤를 위에서부터 채운다.

### CP4-1 · 2 몸풀기

- `isSaveFile`: `QFileInfo(path).suffix().toLower()`가 `"sav"`나 `"dsv"`인가
- `versionOfOriginGame`: `static const QHash<int, QString>` 표 하나 + `value(key)`. 함수 안의 `static const`는 **처음 불릴 때 한 번만**
  만들어진다(C++11부터 스레드 안전)

### CP4-3 · 4 파일 → 파티 — 레이어 경계

```cpp
QFile file(path);
if (!file.open(QIODevice::ReadOnly)) { /* error 채우고 nullopt */ }
const QByteArray raw = file.readAll();
const save::Bytes bytes(reinterpret_cast<const std::uint8_t *>(raw.constData()), std::size_t(raw.size()));
```

core는 Qt를 모르므로 `QByteArray`를 그대로 넘길 수 없다. **data 레이어가 경계에서 `std::span`으로 바꿔 넘긴다**(CLAUDE.md §4).
복사가 아니라 "같은 메모리를 다른 이름으로 보는 창"이라 비용이 없다 — 단, `raw`가 살아 있는 동안만 유효하다
(`raw`보다 오래 `bytes`를 들고 있으면 안 된다).

### CP4-5 버전 고르기 — 다수결

```
묶음의 버전들 = repository.gamesForGeneration(4) 중 versionGroup이 같은 GameInfo의 versions   예) {heartgold, soulsilver}
표 = 멤버마다 versionOfOriginGame(originGame)이 그 목록에 있으면 한 표
가장 많이 받은 버전. 아무도 안 맞으면(전부 다른 게임에서 데려온 포켓몬) 목록의 첫 버전
```
`QHash<QString, int>`로 세면 된다. 테스트 두 개(`PicksTheVersion…` · `FallsBack…`)가 두 경우를 확인한다.

### CP4-6 멤버 → SquadMember

| SquadMember 칸 | 값 |
|---|---|
| (알이면) | 건너뛴다 — 그 자리는 빈 자리 |
| `pokemonId` | `repository.defaultPokemonId(member.species)` · `form != 0`이면 `qCWarning(lcData)` (폼 표는 아직 없다) |
| `moves` · `abilityId` | 그대로 — 번호가 DB id와 같다 |
| `natureId` | `repository.natureIdForGameIndex(member.nature)` |
| `itemId` | 0이면 0, 아니면 `repository.itemIdForGameIndex(4, member.heldItem)` |

**자리 순서를 지킨다** — `party->members[i]` → `squad.members[i]`. 그래야 게임 속 파티 순서가 그대로 온다.

### CP4-7 마무리

`Portable{ generation = 4, game = 고른 버전, squad }`를 돌려주고, `qCInfo(lcData)`로 한 줄 남긴다:
```
I pokesix.data: save import: soulsilver · 6 members
```
(앱 전체 로그 형식이라 시각도 붙는다. `run.sh --log`나 아래 CP7의 디버그 설정으로 볼 수 있다)

확인: `ctest --preset linux-debug -R SaveImport --output-on-failure` → 7/7

---

## CP5 — 불러오기 창이 세이브도 받게 (`squadpage.cpp`)

### ① 함수 뽑아내기 — `TODO(H2-CP5-1)`

지금 `importSquad()`는 "파일 창 열기"와 "파일 하나 불러오기"를 한 함수에서 한다. 끌어다 놓기(CP6)도 "파일 하나 불러오기"가
필요하니 뒤쪽을 `importFile(path)`로 옮긴다:

```
importSquad()                       importFile(path)
  파일 창 → path                      squadfile 또는 saveimport로 loaded 얻기   ← 여기만 갈래
  비었으면 return                     실패 → 경고 창
  importFile(path)                    세대 · 게임 전환 · 덮어쓰기 확인 · replaceSquad · flashStatus   (원래 코드 그대로)
```

갈래는 한 줄이다:
```cpp
const std::optional<squadfile::Portable> loaded = saveimport::isSaveFile(path)
        ? saveimport::load(path, *m_repository, &error)
        : squadfile::load(path, &error);
```
성공 문구는 세이브일 때 `tr("✓ 세이브에서 파티를 불러왔어요")`처럼 따로 쓰면 무엇을 불러왔는지 보인다.

### ② 파일 창 필터 — `TODO(H2-CP5-2)`

`QFileDialog::getOpenFileName`의 마지막 인자가 필터다. `;;`로 여러 개를 이으면 창 아래에 고르는 목록이 생긴다:
```cpp
tr("PokeSix 스쿼드 · 게임 세이브 (*.pks *.json *.sav *.dsv);;PokeSix 스쿼드 (*.pks *.json);;게임 세이브 (*.sav *.dsv)")
```
첫 필터가 기본이다. melonDS 폴더(`~/melonDS-1.1/gen4/`)에 있는 원본 세이브를 바로 골라도 된다 — **읽기만** 하므로 원본은 안전하다.

확인: 앱 실행 → 스쿼드 → [불러오기] → 소울실버 세이브 → 덮어쓰기 확인 → 6마리가 채워진다(CP7에서 자세히).

---

## CP6 — 끌어다 놓기 (`squadpage.cpp`)

### Qt의 끌어다 놓기 — 이벤트 세 개

```
파일을 끌고 창 위로 들어옴   →  dragEnterEvent   "받을 수 있나?" — acceptProposedAction() 하면 커서가 [+]로
(끌면서 움직임              →  dragMoveEvent     기본 동작으로 충분 — 재정의 안 함)
놓음                        →  dropEvent         실제로 처리
```

- **`setAcceptDrops(true)`가 먼저** — 이게 없으면 이 위젯에는 끌어다 놓기 이벤트가 아예 오지 않는다(`TODO(H2-CP6-1)`)
- 끌어온 내용은 `event->mimeData()`에 있다. 파일 관리자(노틸러스)에서 끌어온 파일은 **URL 목록**이다:
  `mimeData()->hasUrls()` · `urls()` · `url.isLocalFile()` · `url.toLocalFile()`(→ 경로 문자열)
- 이벤트는 **마우스 아래 가장 안쪽 위젯부터** 가고, 그 위젯이 드롭을 안 받으면 부모로 올라간다. 그래서 카드 · 분석 창 위 어디에
  놓아도 `SquadPage`가 받는다. 예외: `QLineEdit`(스쿼드 이름 · 카드 메모)은 기본으로 글자 드롭을 받아서, 거기에 놓으면
  파일 경로가 **글자로 입력된다** — 이 단계에서는 그대로 두고, 거슬리면 그 칸들에 `setAcceptDrops(false)`

| TODO | 할 일 |
|---|---|
| `H2-CP6-2` `dragEnterEvent` | URL이 **하나**이고, 로컬 파일이고, 확장자가 `pks` · `json` · `sav` · `dsv` 중 하나면 `event->acceptProposedAction()`. 아니면 아무것도 안 한다(그러면 거절 — 커서가 🚫) |
| `H2-CP6-3` `dropEvent` | `importFile(urls().constFirst().toLocalFile())` 후 `event->acceptProposedAction()` |

ROS 2에 빗대면 `dragEnterEvent`는 메시지를 받기 전의 **필터**(이 토픽 · 이 타입만), `dropEvent`는 콜백이다.

---

## CP7 — 실제로 확인하기 = H2 완료

### 첫 실행: 데이터 다시 만들기

스키마 버전을 올렸으므로(CP2) 앱을 켜면 인트로에 **첫 실행 패널**이 다시 뜬다 → [데이터 받기]. CSV는 캐시에 있어 몇 초면 끝난다.

### VS Code로 따라가 보기

1. `src/data/store/saveimport.cpp`의 `load` 안, `readParty`를 부르는 줄에 breakpoint
2. 실행 및 디버그 → **"PokeSix 앱 (로그 켬)"** → F5
3. 스쿼드 → [불러오기] → 소울실버 세이브 → breakpoint에서 멈춘다
4. **CALL STACK**을 보면 `saveimport::load` ← `SquadPage::importFile` ← `SquadPage::importSquad` ← … `QAbstractButton::clicked` — 버튼 클릭이 시그널을
   거쳐 여기까지 온 길이 보인다
5. F10으로 내려가며 `party`(VARIABLES에서 펼치기) → 고른 버전 → 멤버 변환을 확인. F5로 계속

### 완료 조건

- [ ] `ctest --preset linux-debug` 전부 통과(새 테스트 11개 포함), 경고 0, `clang-format`
- [ ] 앱에서 [불러오기]로 소울실버 세이브를 고르면: 게임이 **소울실버**로 바뀌고, 스쿼드에 **프테라 · 전룡 · 홍수몬 · 맘모꾸리 · 블레이범 ·
  왕구리** 순서로 채워진다. 기술 4개 · 특성 · **성격 · 지닌 물건**까지(H1 때 표와 같다 — 예: 프테라 = 선제공격손톱 · 개구쟁이)
- [ ] 같은 파일을 스쿼드 화면에 **끌어다 놓아도** 같은 결과
- [ ] 세이브가 아닌 파일(예: 아무 텍스트 파일 이름을 `.sav`로)을 고르면 경고 창이 뜨고 스쿼드는 그대로
- [ ] 로그에 `save import: soulsilver · 6 members`

다 되면 **"H2 진단해줘"** → Claude가 diff · 빌드 · 테스트를 보고, 새 문구의 번역(영어 · 일본어)을 채우고, 커밋 · merge한다.

## 다음 — 이 뒤에 할 수 있는 것

- 폼 표(로토무 · 기라티나 …의 폼 번호 → `pokemon.id`) — 지금은 기본 폼으로 들어간다
- 플라티나 세이브로 한 번 더 확인(`RealSave.Platinum`도 같이)
- 전투 분석의 입력 넓히기 — [data/battle-inputs.md](../data/battle-inputs.md) §4
- (overlay-design.md의 H2 원안) 세이브 파일 감시 — 게임에서 저장하면 스쿼드가 저절로 갱신(`QFileSystemWatcher`)
