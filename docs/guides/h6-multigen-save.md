# H6 — 세이브가 세대를 알려 준다 (4세대 + 5세대 BW · B2W2)

> 학습 루프 ① 가이드. 브랜치: `feat/h6-multigen-save` (Claude가 만들어 둠)
> 뼈대 · 테스트는 준비돼 있다. **`// TODO(H6-CPn-m)`만 채우면 된다.** 빈칸이 있어도 빌드된다.
> 앞 단계: [H1](h1-save-reader.md)(4세대 core 파서) · [H2](h2-squad-sav-import.md)(앱에서 불러오기) ·
> 설계: [overlay-design.md §5](../overlay-design.md#5-세이브-파싱--core에-둔다-qt-없음) · 5세대 수치: [data/gen5/](../data/gen5/README.md)

## 전체 그림 — 세 단계

H2까지의 파서는 **게임 단위로는** 자동 판별(DP · Pt · HGSS)이지만 **세대 단위로는** 4세대에 고정돼 있었다.
`saveimport.cpp`의 `constexpr int kGeneration = 4;`가 그 증거다. H6은 이걸 세 단계로 푼다:

```
.sav 바이트
   │
   ① 판별   "이 파일은 몇 세대 · 어느 시리즈인가"          CP1 · CP2 · CP3
   │        readParty(save)
   │          for (형식 : saveFormats())   4세대 → 5세대
   │              형식.read(save)          각 형식이 자기 체크섬을 검증 — 맞는 형식은 하나뿐
   │
   ② 추출   그 형식의 규칙으로 파티를 꺼낸다               CP4
   │        4세대: 슬롯 · footer · PK4 236 B          5세대: 체크섬 모음 블록 · 파티 블록 · PK5 220 B
   │        → ReadParty { generation, versionGroup, partyOffset, members }   ← 세대와 상관없는 같은 모양
   │
   ③ 구성   그 세대의 번호표로 Portable을 만든다            CP5
            party->generation → 버전 다수결 · 물건 번호 변환 · Portable.generation
```

**핵심 설계 두 가지**

1. **파일 크기로 고르지 않는다.** 4세대도 5세대도 512 KiB다. 대신 형식마다 "이 구조로 읽으면 체크섬이 맞는가"를 본다.
   H1에서 DP · Pt · HGSS를 가른 원리(표의 행마다 footer CRC 검증)를 **세대 단위로 한 단계 올린 것**이다.
2. **세대 차이는 if가 아니라 표의 행이다**(CLAUDE.md §4). 6세대를 넣을 때 `if (gen == 6)`을 찾아다니지 않고 형식 표에 한 줄 더한다.
   CAN 덤프에 빗대면: ID별 디코더 표인데, 이 파일에는 ID가 없어서 디코더마다 체크섬으로 "내 프레임인가"를 스스로 확인하는 셈이다.

| 단계 | | 체크포인트 | 파일 | 배우는 것 |
|---|---|---|---|---|
| ① 판별 | ⬜ | **CP1 공통 결과 모양** | `src/core/save/partyreader.cpp` | 세대 중립 구조체 |
| ① 판별 | ⬜ | **CP2 형식 목록** | `src/core/save/saveformat.cpp` | 함수 포인터 표 · `constexpr std::array` · `std::span` |
| ① 판별 | ⬜ | **CP3 5세대 판별** | `src/core/save/gen5.cpp` | 두 겹 CRC(블록 CRC + 모음 블록), 파일 밖 읽기 막기 |
| ② 추출 | ⬜ | **CP4 5세대 파티 · PK5** | `pkmcodec.cpp` · `gen5.cpp` | 220 B 레코드, 스트림 암호의 앞부분, 세대별로 다른 칸 덮어쓰기 |
| ③ 구성 | ⬜ | **CP5 Portable** | `src/data/store/saveimport.cpp` | 상수 지우고 컴파일러에게 고칠 곳 묻기 |
| (선택) | ⬜ | **CP6 readsav 로그** | `tools/readsav/main.cpp` | `span` 순회, 판별 과정 찍기 |
| | ⬜ | **완료 확인** | 테스트 · 앱 | 4세대 회귀 없음 |

### 준비된 것 (Claude가 만들어 둠)

| 파일 | 상태 |
|---|---|
| `src/core/save/partyreader.h` | **완성.** `ReadParty`(세대 중립) · `ReadMember`에 `hiddenAbility` · 4세대 읽기는 `readGen4Party`로 이름이 바뀜 |
| `src/core/save/saveformat.h` | **완성.** `SaveFormat` · `saveFormats()` · `readParty()` 선언 |
| `src/core/save/gen5.h` | **완성.** `Gen5Layout` 표(BW · B2W2) · 상수 · 함수 선언. 머리 주석이 5세대 파일 지도 |
| `saveformat.cpp` · `gen5.cpp` · `pkmcodec.cpp` · `partyreader.cpp` · `saveimport.cpp` | 고칠 자리에 `TODO(H6-…)` |
| `gen5.cpp`의 `infoBlockStart` | **완성**(한 줄 — 본보기) |
| `tests/` | 새 테스트 — `Gen5` 11 · `SaveFormat` 4 · `SaveImport` 3, 기존 4세대 테스트는 새 이름에 맞춤. 일회용 참조 구현으로 전부 통과 확인 |
| `tests/core/syntheticsave.h` | 합성 5세대 세이브 `buildGen5Save` — **진짜 BW 세이브가 없어서** 5세대는 PKHeX 수치로 만든 가짜 파일로 확인한다 |

지금 상태: **테스트 18개가 빨강**(나머지 144개는 초록). CP마다 해당 테스트가 초록으로 바뀐다.

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug -R "PartyReader|SaveFormat|Gen5|SaveImport" --output-on-failure
```

| CP | 초록으로 바뀌는 테스트 |
|---|---|
| CP1 | `PartyReader.ReadsAPlatinumParty` · `DetectsEachGameFromTheBlockSize` · `ReadsTheNewestSlot` (3) |
| CP2 | `SaveFormat.ListsTheGenerationsInOrder` · `ReadsAFourthGenerationSave` + 4세대 `SaveImportTest` 넷 (6) |
| CP3 | `Gen5.ValidatesTheInfoBlockOfItsOwnGameOnly` · `ChecksThePartyBlockCrcInBothPlaces` (2) |
| CP4 | `Gen5.Decodes220BytePartyPokemon` · `ReadsABlackWhiteParty` · `ReadsABlack2White2Party` · `SaveFormat.ReadsAFifthGenerationSave` (4) |
| CP5 | `SaveImport.NamesTheFifthGenerationOriginGames` · `SaveImportTest.TurnsABlackWhitePartyIntoASquad` · `PicksBlack2OrWhite2ByOrigin` (3) |

`Gen5.RejectsACorruptedInfoBlock` · `NeverValidatesOutsideTheFile` · `RejectsAnEmptyParty` 같은 "거절" 테스트는 빈칸(늘 `false`)으로도 지금 초록이다.
채운 뒤에도 **초록으로 남아 있어야** 하는 안전망이다 — 채우다가 빨강이 되면 판정이 너무 느슨한 것.

---

# ① 판별

## CP1 — 공통 결과 모양 (`partyreader.cpp`, 3줄)

H2의 `ReadParty`는 4세대 전용 칸(`layout` · 슬롯 시작)을 들고 있었다. 이제 바깥(saveimport · readsav)은 **어느 세대인지 몰라도**
결과를 쓸 수 있어야 하므로, 결과에는 세대와 상관없는 칸 세 개만 남겼다:

```cpp
struct ReadParty {
    int generation = 0;              // 4 · 5 — ③ 단계가 이걸로 번호표를 고른다
    std::string_view versionGroup;   // "heartgold-soulsilver" · "black-white" — PokéAPI 이름 그대로
    std::size_t partyOffset = 0;     // 파일에서 첫 포켓몬의 위치 (로그 · 디버깅용)
    std::vector<ReadMember> members;
};
```

- `TODO(H6-CP1)`: `readGen4Party` 안에서 세 칸을 채운다. 값은 TODO 주석에 그대로 있다 — `layout`과 `general`(고른 슬롯의 일반 블록 시작)
- `versionGroup`이 `string_view`여도 되는 이유: 가리키는 글자가 `kGen4Layouts` 표의 문자열 리터럴이라 프로그램이 끝날 때까지 살아 있다

확인: `ctest --preset linux-debug -R PartyReader` → 전부 초록

## CP2 — 형식 목록 (`saveformat.cpp`)

### 새 개념: 함수 포인터 표

```cpp
struct SaveFormat {
    int generation = 0;
    std::string_view name;
    std::optional<ReadParty> (*read)(Bytes save) = nullptr;   // "Bytes를 받아 optional<ReadParty>를 주는 함수"의 주소
};
```

`read`는 **함수를 가리키는 변수**다. `&readGen4Party`처럼 함수 이름 앞에 `&`를 붙이면 부르지 않고 주소만 담고,
나중에 `format.read(save)`로 보통 함수처럼 부른다. ROS 2로 치면 콜백을 등록해 두는 것과 같다 — 다만 상태(캡처)가 없는 함수라
`std::function`도, 가상 함수를 가진 클래스 계층도 필요 없다.

| 선택지 | 장점 | 이 경우 |
|---|---|---|
| **함수 포인터 표** (추천) | `constexpr` 표 한 장, 힙 · 가상 호출 없음, 세대 추가 = 한 줄 | 형식마다 상태가 없다 — 딱 맞다 |
| `std::function` | 람다 캡처 가능 | 캡처할 게 없다 — 무겁기만 하다 |
| `class SaveFormat { virtual … }` | 형식마다 함수 여러 개(쓰기 · 박스 읽기 …)를 묶기 좋다 | 함수가 여럿 생기면(세이브 쓰기 · 박스) 그때 옮긴다 |

### TODO

- `TODO(H6-CP2-1)` 형식 표. 지금은 빈 표(`std::array<SaveFormat, 0>`) — 두 행으로 바꾼다:
  ```cpp
  constexpr std::array kFormats = {
          SaveFormat {4, "4세대 NDS (DP · Pt · HGSS)", &readGen4Party},
          SaveFormat {5, /* 이름 */, /* 5세대 읽기 함수의 주소 */},
  };
  ```
  `std::array kFormats = {…}`처럼 크기를 안 써도 된다(C++17 CTAD — 원소 수를 컴파일러가 센다).
  `saveFormats()`는 이미 `return kFormats;` — `std::array`가 `std::span`으로 저절로 바뀐다(복사 아님, 창)
- `TODO(H6-CP2-2)` `readParty`: 표를 차례로 돌며 `format.read(save)`가 값을 주면 바로 그 값을 돌려준다. 끝까지 없으면 `nullopt`.
  H1의 `for (layout : kGen4Layouts) if (auto start = …)` 와 똑같은 모양이다

5세대 행의 `readGen5Party`는 지금 늘 `nullopt`라서, 이 단계에서는 4세대만 읽힌다. 그래도 4세대 앱 경로가 이미 이걸 탄다 —
`saveimport::load`가 `save::readParty`를 부르기 때문이다. 그래서 **H2 테스트 넷이 여기서 다시 초록**이 된다.

**순서가 중요한가?** 맞는 형식이 하나뿐이라 결과는 같다. 다만 "4세대 세이브를 5세대 규칙으로 읽으면 반드시 실패하는가"는
테스트로 못 박아 둔다(`Gen5.IsNotFooledByAFourthGenerationSave`).

확인: `ctest --preset linux-debug -R "SaveFormat|SaveImportTest" --output-on-failure` → `ReadsAFifth…` · 5세대 SaveImport 둘을 뺀 나머지 초록

## CP3 — 5세대 판별 (`gen5.cpp`)

### 5세대 파일 지도

```
.sav 512 KiB — 앞쪽 "본 세이브"(BW 0x24000 · B2W2 0x26000 바이트)만 읽는다
0x00000 ┬ …
0x18E00 ├ 파티 블록 (0x534 B)        +4 파티 수(u8) · +8부터 PK5 220 B × 6
0x19336 ├ 파티 블록의 CRC (u16)      ← 저장 위치 ①
        ├ …
끝−0x100├ 체크섬 모음 블록 (BW 0x8C · B2W2 0x94 B)   모든 블록의 CRC를 한 곳에 다시 모은 표
        │   +0x34 = 파티 블록 CRC의 사본           ← 저장 위치 ②
        └ 모음 블록 + 길이 + 0x0E = 모음 블록 자신의 CRC (u16)
("끝" = 본 세이브 크기 — BW 0x24000 · B2W2 0x26000)
```

4세대와 다른 점:

| | 4세대 | 5세대 |
|---|---|---|
| 슬롯 | 두 슬롯을 번갈아 쓴다 → 최신 판정 | 번갈아 쓰지 않는다 → 최신 판정 없음 |
| 블록 검증 | 블록 끝 footer의 CRC 하나 | 블록 CRC + **모음 블록에 사본** — 두 곳 |
| 게임 판별 | 일반 블록 **크기**가 시리즈마다 달라 footer 자리가 다르다 | 본 세이브 **크기**가 달라 모음 블록 자리가 다르다 |
| CRC 계산 | CRC-16/CCITT-FALSE (`crc16Ccitt`) | **같은 함수** |

판별 원리는 같다 — "BW라면 여기에 모음 블록이 있을 것"이라 가정하고 CRC를 맞춰 본다. 틀린 가정이면 엉뚱한 바이트를 CRC로 읽어 맞을 수 없다.

### TODO

`infoBlockStart(layout)`(= `mainSize − 0x100`)은 본보기로 채워 뒀다.

- `TODO(H6-CP3-1)` `infoBlockValid(save, layout)`
  1. `start = infoBlockStart(layout)`, CRC 저장 위치 `at = start + layout.infoLength + 0x0E`
  2. **먼저** `at + 2 > save.size()`면 `false` — H1 CP2에서 겪은 "파일 밖을 0으로 읽으면 0 = 0으로 맞아 보인다"를 막는다
     (`std::span`의 `operator[]`는 범위 검사를 하지 않는다 — 넘으면 정의되지 않은 동작)
  3. `crc16Ccitt(save.subspan(start, layout.infoLength)) == readU16(save, at)`
- `TODO(H6-CP3-2)` `partyBlockValid(save, layout)`
  1. 같은 요령으로 파일 크기부터 확인(파티 CRC 위치 `kGen5PartyCrc + 2`와 사본 위치 둘 다)
  2. `crc = crc16Ccitt(save.subspan(kGen5PartyBlock, kGen5PartyBlockSize))`
  3. `crc == readU16(save, kGen5PartyCrc)` **그리고** `crc == readU16(save, infoBlockStart(layout) + kGen5PartyCrcMirror)`
- `TODO(H6-CP3-3)` `readGen5Party` 맨 앞: `kGen5Layouts`를 돌며 `infoBlockValid`인 첫 행을 고른다(`const Gen5Layout *` 하나). 없으면 `nullopt`

```cpp
const Gen5Layout *layout = nullptr;
for (const Gen5Layout &candidate : kGen5Layouts)
    if (/* ? */) { layout = &candidate; break; }
if (!layout) return std::nullopt;
```

**왜 사본까지 보나**: 블록 뒤 CRC만 맞고 사본이 틀리면, 게임이 저장하다 끊긴 것이거나 누가 블록만 고친 파일이다.
게임은 두 곳을 함께 갱신하므로 둘 다 맞아야 "정상 저장"이다. 테스트 `ChecksThePartyBlockCrcInBothPlaces`가 사본만 망가뜨려 확인한다.

확인: `ctest --preset linux-debug -R Gen5 --output-on-failure` → CP3 둘 초록. 거절 테스트 셋은 계속 초록

---

# ② 추출

## CP4 — 5세대 파티 · PK5 (`pkmcodec.cpp` · `gen5.cpp`)

### PK5 = PK4와 거의 같다

| | PK4 (4세대) | PK5 (5세대) |
|---|---|---|
| 파티 레코드 | 236 B = 저장 136 + 배틀 스탯 100 | **220 B** = 저장 136 + 배틀 스탯 **84** |
| 암호화 · 블록 섞기 · 체크섬 | LCRNG XOR, `((pid >> 13) & 0x1F) % 24`, u16 합 | **같다** |
| 레벨 · HP · 능력치 위치 | `0x8C` · `0x8E` · `0x90` … | **같다** |
| 성격 | `pid % 25` | **`0x41` 바이트에 따로 저장** |
| 숨겨진 특성 | 없음 | **`0x42`의 비트 0** |
| 출신 게임(`0x5F`) | 7 HG · 8 SS · 10 D · 11 P · 12 Pt | 20 W · 21 B · 22 W2 · 23 B2 |

그래서 새 디코더를 만들지 않고 **H1의 `decodePkm`을 220 B도 받게 넓히고**, 다른 칸 둘만 덮어쓴다.

### 왜 220 B를 그냥 받아도 되나 — 스트림 암호의 앞부분

배틀 스탯은 PID로 시작한 난수열을 **앞에서부터 한 u16씩** XOR한다. 84바이트를 풀 때 쓰는 난수는 100바이트를 풀 때의 **앞 42개와 똑같다**.
그러니 길이만 `encrypted.size() − kStoredSize`로 바꾸면 220 · 236 둘 다 맞게 풀린다. (CAN으로 치면 같은 키 스트림으로 짧은 프레임을 푸는 것)

### TODO

- `TODO(H6-CP4-1)` `decodePkm` (`pkmcodec.cpp`)
  - 크기 검사: `!= kPartyPkmSize` → "236도 아니고 220도 아니면"
  - 배틀 스탯 `cryptArray`의 길이: `kPartyPkmSize − kStoredSize` → `encrypted.size() − kStoredSize`
  - `std::copy`는 그대로 — `pkm.data`(236 B 배열)의 앞 220 B만 채우고 나머지는 0으로 남는다. `parseMember`가 읽는 칸(최대 `0x91`)은 전부 앞쪽이다
- `TODO(H6-CP4-2)` CP3에서 고른 `layout`으로 `partyBlockValid`가 아니면 `nullopt`
- `TODO(H6-CP4-3)` 파티 수 = `save[kGen5PartyBlock + kGen5PartyCountOffset]`, 1–6이 아니면 `nullopt`(4세대와 같은 규칙)
- `TODO(H6-CP4-4)` 한 마리씩:
  ```cpp
  const std::size_t at = kGen5PartyBlock + kGen5PartyOffset + std::size_t(i) * kGen5PartyPkmSize;
  const DecodedPkm pkm = decodePkm(save.subspan(at, kGen5PartyPkmSize));
  ReadMember member = parseMember(pkm);   // 4 · 5세대 공통 칸은 여기서
  member.nature = /* ? */;                // 5세대 진짜 성격: 풀린 data의 0x41
  member.hiddenAbility = /* ? */;         // 0x42의 비트 0 — (x & 0x01) != 0
  party.members.push_back(member);
  ```
  `parseMember`가 넣은 `pid % 25`를 **덮어쓰는** 것이 포인트다. 5세대는 싱크로 · 변하지않는돌 등으로 성격이 PID와 따로 정해질 수 있어서
  게임이 성격을 따로 저장한다. 테스트 `ReadsABlackWhiteParty`의 조로아크는 PID % 25 = 4인데 `0x41` = 9(촐랑)라서, 덮어쓰지 않으면 바로 빨강이 된다.
- `TODO(H6-CP4-5)` `ReadParty{ generation = 5, versionGroup = layout->versionGroup, partyOffset = kGen5PartyBlock + kGen5PartyOffset, members }`

**`parseMember`에 `if (gen == 5)`를 넣지 않는 이유**: 공통 칸 해석은 한 곳, 세대마다 다른 칸은 **그 세대의 읽기 함수**가 덮어쓴다.
세대 차이가 `gen5.cpp` 안에 모여 있어서 6세대를 더할 때 4 · 5세대 코드를 건드리지 않는다.

확인: `ctest --preset linux-debug -R "Gen5|SaveFormat" --output-on-failure` → 전부 초록

---

# ③ 구성

## CP5 — Portable을 세이브의 세대로 (`saveimport.cpp`)

### 컴파일러에게 고칠 곳 묻기

- `TODO(H6-CP5-1)` `constexpr int kGeneration = 4;`를 **지우고 빌드한다.** 이 상수를 쓰던 곳이 전부 컴파일 오류로 나온다 — 그게 고칠 목록이다.
  상수가 하던 일을 이제 `party->generation`(① 단계의 결과)이 한다
  ```
  saveimport.cpp:…: error: 'kGeneration' was not declared in this scope   ← versionsOfGroup 안
  saveimport.cpp:…: error: 'kGeneration' was not declared in this scope   ← itemIdForGameIndex
  saveimport.cpp:…: error: 'kGeneration' was not declared in this scope   ← portable.generation
  ```
- `TODO(H6-CP5-2)` `versionsOfGroup(repository, versionGroup)` → `versionsOfGroup(repository, generation, versionGroup)`.
  안에서 `gamesForGeneration(generation)`. 부르는 쪽은 `party->generation`을 넘긴다. 나머지 두 곳도 `party->generation`으로
- `TODO(H6-CP5-3)` `versionOfOriginGame`의 표에 5세대 네 줄: `{20, "white"}` · `{21, "black"}` · `{22, "white-2"}` · `{23, "black-2"}`
  (블랙이 21이고 화이트가 20 — 순서가 거꾸로인 것에 주의. 출처: PKHeX `GameVersion`)

### 번호 변환은 세대마다 다른가

| 값 | 4세대 | 5세대 | 이 코드에서 |
|---|---|---|---|
| 종 · 기술 · 특성 | PokéAPI id와 같다 | 같다 | 그대로 |
| 성격 | 게임 순서 0–24 | **같은 순서** | `natureIdForGameIndex` 그대로 |
| 물건 | 4세대 게임 번호 | **5세대 게임 번호**(다르다) | `itemIdForGameIndex(party->generation, …)` — DB의 `item_generations`가 세대별 표를 이미 갖고 있다(H2 CP2) |

물건이 이미 "세대 + 게임 번호"로 찾게 돼 있던 건 H2 설계 덕분이다 — 세대만 바르게 넘기면 된다.

확인: `ctest --preset linux-debug -R SaveImport --output-on-failure` → 전부 초록 (5세대 픽스처: 촐랑 = 게임 번호 9 → `natures.id` 18, 물건 234 → 211)

---

## CP6 (선택) — readsav가 판별 과정을 보여 주게 (`tools/readsav/main.cpp`)

H1의 readsav는 4세대 단계별 경로(footer → 슬롯 → PKM)를 찍는 도구다. 마지막의 `readParty` 비교 앞에 `TODO(H6-CP6)` 자리가 있다:

```cpp
for (const save::SaveFormat &format : save::saveFormats())
    qCDebug(lcSave).noquote() << /* 형식 이름 · format.read(bytes) ? "OK" : "--" */;
```

그리고 `readParty:` 줄에 `gen %1`을 더한다. 기대 출력(`--verbose`, 소울실버 — 형식 이름은 CP2에서 정한 것):

```
D pokesix.save: 4세대 NDS (DP · Pt · HGSS) OK
D pokesix.save: 5세대 NDS (BW · B2W2) --
I pokesix.save: readParty: gen 4 · heartgold-soulsilver · 6 members
```

`string_view`는 `QString::fromUtf8(name.data(), qsizetype(name.size()))`로 바꿔 찍는다(파일 안의 다른 줄과 같은 요령).
5세대 세이브는 이 도구의 앞쪽 4세대 경로에서 "no valid general block"으로 끝난다 — 5세대 단계별 경로까지 넣는 건 BW 세이브가 생겼을 때 할 일이다.

---

## 완료 확인 = H6 완료

### 4세대가 그대로인지 (회귀)

세대 판별 구조로 바꿨어도 손에 있는 진짜 세이브는 전과 같이 읽혀야 한다:

```bash
export POKESIX_SAVE_HGSS=~/melonDS-1.1/gen4/<소울실버>.sav
export POKESIX_SAVE_PT=~/melonDS-1.1/gen4/<플라티나>.sav
ctest --preset linux-debug -R RealSave --output-on-failure
```
출력 첫 줄에 `generation 4, party at 0x…`가 찍힌다(HGSS 첫 포켓몬 = 고른 슬롯 시작 + `0x98`). BW · B2W2는 파일이 없으니 `SKIPPED`가 정상이다.

### 완료 조건

- [ ] `ctest --preset linux-debug` 전부 통과(빨강 18개 → 0), 경고 0, `clang-format -i` (바꾼 파일들)
- [ ] `RealSave.HeartGoldSoulSilver` · `RealSave.Platinum` 통과 — 4세대 실파일 회귀 없음
- [ ] 앱에서 소울실버 세이브 불러오기 · 끌어다 놓기가 H2 때와 똑같이 된다(게임 소울실버, 6마리, 성격 · 물건까지)
- [ ] `grep -rn kGeneration src/` 결과가 비어 있다 — 세대 숫자를 박아 둔 곳이 없다
- [ ] (선택) CP6 readsav 출력

5세대는 **합성 세이브로만** 확인한다 — 수치 출처는 PKHeX이고 실파일 검증은 ◇다([data/gen5/](../data/gen5/README.md)).
BW · B2W2 세이브가 생기면 `POKESIX_SAVE_BW` · `POKESIX_SAVE_B2W2`로 `RealSave`를 돌려 ✓로 바꾼다.

다 되면 **"H6 진단해줘"** → Claude가 diff · 빌드 · 테스트를 보고 커밋 · merge한다(새 `tr()` 문구가 없으니 번역 갱신은 없다).

## 다음 — 이 뒤에 할 수 있는 것

- 5세대 단계별 readsav 경로 · 실파일 검증(세이브가 생기면)
- 폼 표(로토무 · 기라티나 …, 5세대는 볼트로스 · 토네로스 · 큐레무까지) — 지금은 기본 폼
- 숨겨진 특성(`hiddenAbility`)을 스쿼드의 특성 고르기에 반영 — 지금은 `ability` 번호가 그대로 들어가므로 이미 맞는 특성이 온다. 표시만의 문제
- 6세대 이후(3DS 세이브 · PK6, 체크섬 구조가 또 다르다) — 형식 표에 한 줄 + `gen6.cpp`
