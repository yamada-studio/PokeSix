# H6 — 세이브가 세대를 알려 준다 (기기별 리더 · 4세대 + 5세대 BW · B2W2)

> 학습 루프 ① 가이드. 브랜치: `feat/h6-multigen-save` (Claude가 만들어 둠)
> 뼈대 · 테스트는 준비돼 있다. **`// TODO(H6-CPn-m)`만 채우면 된다.** 빈칸이 있어도 빌드된다.
> 앞 단계: [H1](h1-save-reader.md)(4세대 core 파서) · [H2](h2-squad-sav-import.md)(앱에서 불러오기) ·
> 구조 결정: [ADR 0019](../decisions/0019-save-readers-per-platform.md) · 5세대 수치: [data/gen5/](../data/gen5/README.md)

## 전체 그림 — 세 단계

H2까지의 파서는 **게임 단위로는** 자동 판별(DP · Pt · HGSS)이지만 **세대 단위로는** 4세대에 고정돼 있었다.
`saveimport.cpp`의 `constexpr int kGeneration = 4;`가 그 증거다. H6은 이걸 세 단계로 푼다:

```
.sav 바이트
   │
   ① 판별   "어느 기기 · 어느 시리즈의 세이브인가"                              CP1 · CP2 · CP3
   │        readParty(save)
   │          for (리더 : partyReaders())        기기 — 지금은 NDS 하나
   │            리더->read(save)
   │              for (시리즈 : kNdsSeries)      DP · Pt · HGSS · BW · B2W2
   │                검증 방식으로 체크섬 확인    FooterSlots(4세대식) | ChecksumTable(5세대식)
   │
   ② 추출   그 시리즈의 숫자로 파티를 꺼낸다                                    CP4
   │        파티 수 · 위치 · 포켓몬 형식(PK4 236 B | PK5 220 B · 성격 칸 · 숨겨진 특성 칸)
   │        → ReadParty { generation, versionGroup, partyOffset, members }   ← 기기 · 세대와 상관없는 같은 모양
   │
   ③ 구성   그 세대의 번호표로 Portable을 만든다                                 CP5
            party->generation → 버전 다수결 · 물건 번호 변환 · Portable.generation
```

### 무엇을 어디로 나눴나 ([ADR 0019](../decisions/0019-save-readers-per-platform.md))

세이브 구조는 세대보다 **기기**를 따라간다(같은 기기의 게임은 같은 저장 방식을 쓴다). 그래서:

| 축 | 표현 | 위치 |
|---|---|---|
| **기기**(NDS · 나중에 GBA · 3DS · Switch) | 클래스 — 부모 `PartyReader`의 자식 | `partyreader.h`(부모) · `partyndsreader.h`(NDS) |
| **시리즈**(DP · Pt · HGSS · BW · B2W2) | 표의 행 — `kNdsSeries` | `partyndsreader.h` |
| **검증 구조**(4세대식 · 5세대식) | 함수 — 구조가 다를 때만 나눈다 | `saveblock.h/.cpp` |
| **포켓몬 형식**(PK4 · PK5) | 데이터 — `PkmFormat` 행 | `pkmcodec.h` |

```
src/core/save/
├ savebytes.h/.cpp       readU16 · CRC — 공통
├ pkmcodec.h/.cpp        포켓몬 암호 + PkmFormat(kPk4 · kPk5)          — 기기와 무관
├ saveblock.h/.cpp       NDS 검증 방식: FooterSlots · ChecksumTable
├ partyreader.h/.cpp     부모: ReadMember · ReadParty · parseMember · PartyReader · readParty
└ partyndsreader.h/.cpp  자식: kNdsSeries(5줄) · class PartyNdsReader
  (나중에) partygbareader · party3dsreader · partyswitchreader — 기기마다 한 쌍
```

**`if (gen == 5)`가 한 줄도 없다.** 6세대를 더할 때 4 · 5세대 코드를 건드리지 않고, 표에 줄을 더하거나 기기 리더를 한 쌍 더한다.

| 단계 | | 체크포인트 | 파일 | 배우는 것 |
|---|---|---|---|---|
| ① 판별 | ⬜ | **CP1 기기 리더 등록** | `partyreader.cpp` | 가상 함수 · 부모 포인터로 자식 부르기(다형성) · 함수 안의 `static` |
| ① 판별 | ⬜ | **CP2 NDS 시리즈 판별** | `partyndsreader.cpp` | `std::variant` · `std::visit` · 오버로드 · `static` 멤버 함수 |
| ① 판별 | ⬜ | **CP3 5세대식 검증** | `saveblock.cpp` · `partyndsreader.cpp` | 두 겹 CRC(블록 CRC + 모음 블록 사본), 파일 밖 읽기 막기 |
| ② 추출 | ⬜ | **CP4 PK5** | `pkmcodec.cpp` · `partyreader.cpp` · `partyndsreader.cpp` | `std::optional`로 "칸이 있을 때만", 스트림 암호의 앞부분 |
| ③ 구성 | ⬜ | **CP5 Portable** | `src/data/store/saveimport.cpp` | 상수 지우고 컴파일러에게 고칠 곳 묻기 |
| (선택) | ⬜ | **CP6 readsav 로그** | `tools/readsav/main.cpp` | 판별 과정 찍기 |
| | ⬜ | **완료 확인** | 테스트 · 실파일 · 앱 | 4세대 회귀 없음 |

### 준비된 것 (Claude가 만들어 둠)

| 파일 | 상태 |
|---|---|
| `partyreader.h` | **완성.** `PartyReader` 인터페이스 · `ReadParty`(기기 중립) · `parseMember(pkm, format)` · `partyReaders()` · `readParty()` 선언 |
| `partyndsreader.h` | **완성.** `NdsSeries` · `kNdsSeries` 5줄 · `class PartyNdsReader` 선언 |
| `saveblock.h` | **완성.** `FooterSlots`(옛 `SaveLayout`의 블록 숫자) · `ChecksumTable` · 함수 선언 |
| `pkmcodec.h` | **완성.** `PkmFormat` · `kPk4` · `kPk5` |
| **옮겨 둔 H1 코드** | `readFooter` · `activeGeneralBlock`(인자 타입만 `FooterSlots`로), `parseMember`, 그리고 H1의 4세대 파티 조립을 `PartyNdsReader::readSeries`로 — **로직은 그대로** |
| `readSeries` | 이미 짜 둔 부분: 크기 검사 · `ReadParty` 세 칸(지난번 CP1에서 쓰신 세 줄이 `series.…`로 여기 들어갔다) · 파티 수 · 반복문. 빈칸은 판별(CP2-2)과 형식(CP4-3) |
| `tests/` | 새 테스트 + H1 테스트 옮김. `partyndsreader_test.cpp`(새) · `saveblock_test.cpp`(5세대식 추가) · `partyreader_test.cpp`(부모 쪽). 일회용 참조 구현으로 **163개 전부 통과 · 실제 소울실버 세이브 결과 동일** 확인 |
| `tests/core/syntheticsave.h` | `seriesOf("platinum")` · `buildSave`(4세대식) · `buildTableSave`(5세대식). **진짜 BW 세이브가 없어서** 5세대는 PKHeX 수치로 만든 합성 세이브로 확인한다 |

> 첫 뼈대(세대별 `gen5.*` · `saveformat.*`)는 [ADR 0019](../decisions/0019-save-readers-per-platform.md)로 기기별 구조로 바뀌었다.
> 거기서 채우신 CP1 세 줄은 `readSeries`의 `series.…` 세 줄로, 형식 표는 CP1-1의 리더 목록으로 자리를 옮겼다.

지금 상태: **테스트 19개가 빨강**(나머지 144개는 초록). CP마다 해당 테스트가 초록으로 바뀐다.

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug -R "SaveBlock|PkmCodec|PartyReader|PartyNdsReader|SaveImport" --output-on-failure
```

| CP | 초록으로 바뀌는 테스트 |
|---|---|
| CP1 | `PartyReader.ListsTheNdsReader` (1) |
| CP2 | `PartyNdsReader.ReadsAPlatinumParty` · `ReadsTheNewestSlot` · `AcceptsADeSmuMEFooter` · `PartyReader.ReadsAFourthGenerationSave` + 4세대 `SaveImportTest` 넷 (8) |
| CP3 | `SaveBlock.ValidatesTheChecksumTableOfItsOwnSeriesOnly` · `ChecksThePartyBlockCrcInBothPlaces` (2) |
| CP4 | `PkmCodec.DecodesA220BytePk5` · `PartyReader.ReadsTheNatureAndHiddenAbilityOfAPk5` · `ReadsAFifthGenerationSave` · `PartyNdsReader.ReadsABlackWhiteParty` · `DetectsEverySeries` (5) |
| CP5 | `SaveImport.NamesTheFifthGenerationOriginGames` · `SaveImportTest.TurnsABlackWhitePartyIntoASquad` · `PicksBlack2OrWhite2ByOrigin` (3) |

"거절" 테스트(`RejectsACorruptedChecksumTable` · `ChecksumTableOutsideTheFileIsNeverValid` · `RejectsAPartyBlockThatDoesNotCheckOut` ·
`NoSeriesReadsAnotherSeriesSave` · `IgnoresTheFifthGenerationBytesInAPk4` …)는 빈칸으로도 지금 초록이다.
채운 뒤에도 **초록으로 남아 있어야** 하는 안전망이다 — 채우다가 빨강이 되면 판정이 너무 느슨하거나, 4세대에 5세대 규칙이 새어 든 것.

---

# ① 판별

## CP1 — 기기 리더 등록 (`partyreader.cpp`)

### 새 개념: 가상 함수 인터페이스

```cpp
class PartyReader {                                              // 부모 — partyreader.h
public:
    virtual ~PartyReader() = default;
    virtual std::string_view platform() const = 0;               // = 0: 몸통 없음, 자식이 반드시 구현
    virtual std::optional<ReadParty> read(Bytes save) const = 0;
};

class PartyNdsReader final : public PartyReader {                // 자식 — partyndsreader.h
public:
    std::string_view platform() const override;                  // override: "부모의 그 함수를 구현한다"
    std::optional<ReadParty> read(Bytes save) const override;    //   — 이름 · 인자가 틀리면 컴파일 오류로 잡아 준다
};
```

- `= 0`이 하나라도 있으면 **추상 클래스** — `PartyReader` 자체는 객체로 못 만든다. 모양(인터페이스)만 정한다
- 부모 포인터 `const PartyReader *r = &nds;`로 `r->read(save)`를 부르면, **실행할 때** 실제 객체(자식)의 `read`가 불린다.
  이게 다형성이다. 객체마다 숨은 "가상 함수 표(vtable)" 포인터가 있어서 거기서 함수를 찾는다
- `virtual ~PartyReader() = default;` — 부모 포인터로 자식을 지울 때 자식 소멸자도 불리게 하는 관례. 상속 받을 클래스엔 늘 둔다
- `final` — 이 클래스를 더 상속하지 않는다(NDS 안의 차이는 상속이 아니라 표의 행으로 표현하므로)

ROS 2로 치면 **pluginlib의 base class**다. 다만 pluginlib는 실행 중에 `.so`를 찾아 올리지만, 여기는 기기가 몇 개 안 되고 미리 정해져
있으니 **목록에 직접 적는다**(CP1-1).

### TODO

- `TODO(H6-CP1-1)` `partyReaders()` — 리더 객체와 목록을 **함수 안의 `static`**으로:
  ```cpp
  static const PartyNdsReader nds;
  static const std::array<const PartyReader *, 1> readers = {&nds};
  return readers;   // std::array → std::span 으로 저절로 바뀐다(복사 아님, 창)
  ```
  함수 안의 `static`은 **처음 불릴 때 한 번만** 만들어지고(C++11부터 스레드 안전) 프로그램이 끝날 때까지 산다 — 그래서 그 주소를 돌려줘도 된다.
  지역 변수(`static` 없이)였다면 함수가 끝나는 순간 사라져서, 돌려준 포인터가 허공을 가리킨다
- `TODO(H6-CP1-2)` `readParty` — `partyReaders()`를 차례로 돌며 `reader->read(save)`가 값을 주면 바로 돌려준다. 끝까지 없으면 `nullopt`.
  H1의 `for (layout : kGen4Layouts) if (auto start = …)`와 같은 모양이다

확인: `ctest --preset linux-debug -R PartyReader.ListsTheNdsReader` → 초록. 나머지는 NDS 리더의 `read`가 아직 빈칸이라 CP2에서

## CP2 — NDS 안의 시리즈 판별 (`partyndsreader.cpp`)

### 새 개념: `std::variant` · `std::visit`

시리즈 표의 행:

```cpp
struct NdsSeries {
    int generation;
    std::string_view versionGroup;
    std::variant<FooterSlots, ChecksumTable> check;   // 둘 중 "하나"가 든 상자
    std::size_t partyCountOffset, partyOffset;        // 기준(base)에서부터 센 위치
    const PkmFormat *pkm;
};
// {4, "platinum",    FooterSlots {0xCF2C, 0x14},                      0x9C,    0xA0,    &kPk4}
// {5, "black-white", ChecksumTable {0x24000, 0x8C, 0x18E00, …},       0x18E04, 0x18E08, &kPk5}
```

`std::variant<A, B>`는 **A 또는 B 중 하나**를 담는 타입 안전한 union이다(ROS 2 메시지에는 없는 개념 — 굳이 빗대면 "type 필드 +
필드 묶음 여러 개"를 컴파일러가 대신 관리해 주는 것). 4세대 행에는 `FooterSlots`가, 5세대 행에는 `ChecksumTable`이 든다.

그 안의 것을 꺼내 쓰는 표준 방법이 `std::visit`:

```cpp
const std::optional<std::size_t> base = std::visit(
        [&](const auto &check) { return findBase(save, check); },   // auto = 실제로 든 타입
        series.check);
```

- 람다의 `auto` 인자는 variant에 실제로 든 타입이 된다. `check`가 `FooterSlots`면 `findBase(Bytes, const FooterSlots &)`가,
  `ChecksumTable`이면 `findBase(Bytes, const ChecksumTable &)`가 불린다 — **이름이 같고 인자 타입만 다른 함수(오버로드)를 컴파일러가 고른다**
- 그래서 "검증 방식마다 함수 하나"가 if 없이 된다. 6세대식 검증이 생기면 variant에 타입 하나, `findBase` 오버로드 하나를 더한다.
  하나를 빠뜨리면 **컴파일 오류**로 알려 준다(if 사슬은 빠뜨려도 조용히 지나간다)

**기준(base)**: 파티 위치는 검증이 알려 주는 "기준"에서부터 센다. 4세대식은 고른 슬롯의 일반 블록 시작(0 또는 `0x40000`),
5세대식은 슬롯이 없으니 0(파일 시작). 그래서 표의 5세대 행은 `0x18E04` · `0x18E08`처럼 파일 기준 위치를 그대로 적는다.

**`static` 멤버 함수**: `readSeries` · `findBase`는 `static`이다 — 객체 상태(`this`)를 쓰지 않으니 `PartyNdsReader::readSeries(save, series)`처럼
객체 없이 부를 수 있다. 테스트 · readsav가 "이 시리즈로 읽어 봐"를 직접 부를 때 쓴다.

### TODO

- `TODO(H6-CP2-1)` `read()`: `kNdsSeries`를 차례로 `readSeries(save, series)`에 넣어 값이 나오면 돌려준다. 끝까지 없으면 `nullopt`
- `TODO(H6-CP2-2)` `readSeries`의 판별 줄: 지금 `const std::optional<std::size_t> base;`(빈 값) — 위의 `std::visit` 줄로 바꾼다
- `TODO(H6-CP2-3)` `findBase(save, const FooterSlots &)`: H1의 `activeGeneralBlock(save, check)`를 돌려주면 끝(한 줄)

5세대 쪽 `findBase`는 아직 늘 `nullopt`라서 이 단계에서는 4세대만 읽힌다. 그래도 앱 경로(`saveimport` → `readParty`)가 이걸 타므로
**H2 테스트 넷이 여기서 다시 초록**이 된다.

확인: `ctest --preset linux-debug -R "PartyNdsReader|PartyReader|SaveImportTest" --output-on-failure` → CP2 표의 8개 초록

## CP3 — 5세대식 검증 (`saveblock.cpp` · `partyndsreader.cpp`)

### 5세대 파일 지도

```
.sav 512 KiB — 앞쪽 "본 세이브"(BW 0x24000 · B2W2 0x26000 바이트)만 읽는다
0x00000 ┬ …
0x18E00 ├ 파티 블록 (0x534 B)        +4 파티 수(u8) · +8부터 PK5 220 B × 6
0x19336 ├ 파티 블록의 CRC (u16)      ← 저장 위치 ①  partyCrcAt
        ├ …
끝−0x100├ 체크섬 모음 블록 (BW 0x8C · B2W2 0x94 B)   모든 블록의 CRC를 한 곳에 다시 모은 표
        │   +0x34 = 파티 블록 CRC의 사본           ← 저장 위치 ②  partyCrcMirror
        └ 모음 블록 + 길이 + 0x0E = 모음 블록 자신의 CRC (u16)
("끝" = mainSize, 본 세이브 크기)
```

| | 4세대식 `FooterSlots` | 5세대식 `ChecksumTable` |
|---|---|---|
| 슬롯 | 두 슬롯을 번갈아 쓴다 → 최신 판정 | 번갈아 쓰지 않는다 → 최신 판정 없음 |
| 블록 검증 | 블록 끝 footer의 CRC 하나 | 블록 CRC + **모음 블록에 사본** — 두 곳 |
| 시리즈 판별 | 일반 블록 **크기**가 달라 footer 자리가 다르다 | 본 세이브 **크기**가 달라 모음 블록 자리가 다르다 |
| CRC 계산 | `crc16Ccitt` | **같은 함수** |

판별 원리는 같다 — "BW라면 여기에 모음 블록이 있을 것"이라 가정하고 CRC를 맞춰 본다. 틀린 가정이면 엉뚱한 바이트를 CRC로 읽어 맞을 수 없다.

### TODO

`checksumTableStart(check)`(= `mainSize − 0x100`)는 본보기로 채워 뒀다.

- `TODO(H6-CP3-1)` `checksumTableValid(save, check)` (`saveblock.cpp`)
  1. `start = checksumTableStart(check)`, CRC 저장 위치 `at = start + check.tableLength + 0x0E`
  2. **먼저** `at + 2 > save.size()`면 `false` — H1 CP2에서 겪은 "파일 밖을 0으로 읽으면 0 = 0으로 맞아 보인다"를 막는다
     (`std::span`의 `operator[]` · `subspan`은 범위 검사를 하지 않는다 — 넘으면 정의되지 않은 동작)
  3. `crc16Ccitt(save.subspan(start, check.tableLength)) == readU16(save, at)`
- `TODO(H6-CP3-2)` `partyBlockValid(save, check)` (`saveblock.cpp`)
  1. 두 저장 위치(`check.partyCrcAt`, `checksumTableStart(check) + check.partyCrcMirror`)가 파일 안인지부터
  2. `crc = crc16Ccitt(save.subspan(check.partyBlock, check.partyBlockSize))`
  3. 두 위치의 u16 **둘 다**와 같은가
- `TODO(H6-CP3-3)` `findBase(save, const ChecksumTable &)` (`partyndsreader.cpp`): 둘 다 맞으면 `0`(기준 = 파일 시작), 아니면 `nullopt`

**왜 사본까지 보나**: 블록 뒤 CRC만 맞고 사본이 틀리면, 게임이 저장하다 끊긴 것이거나 누가 블록만 고친 파일이다.
게임은 두 곳을 함께 갱신하므로 둘 다 맞아야 "정상 저장"이다. 테스트 `ChecksThePartyBlockCrcInBothPlaces`가 사본만 망가뜨려 확인한다.

확인: `ctest --preset linux-debug -R SaveBlock --output-on-failure` → 전부 초록

---

# ② 추출

## CP4 — PK5 (`pkmcodec.cpp` · `partyreader.cpp` · `partyndsreader.cpp`)

### PK5 = PK4와 거의 같다 — 다른 건 숫자뿐

| | PK4 | PK5 | `PkmFormat` 칸 |
|---|---|---|---|
| 파티 레코드 | 236 B = 저장 136 + 배틀 스탯 100 | **220 B** = 저장 136 + 배틀 스탯 **84** | `partySize` |
| 암호화 · 블록 섞기 · 체크섬 | LCRNG XOR, `((pid >> 13) & 0x1F) % 24`, u16 합 | **같다** | — |
| 레벨 · HP · 능력치 위치 | `0x8C` · `0x8E` · `0x90` … | **같다** | — |
| 성격 | `pid % 25` | **`0x41` 바이트에 따로 저장** | `natureAt` = 없음 · `0x41` |
| 숨겨진 특성 | 없음 | **`0x42`의 비트 0** | `hiddenAbilityAt` = 없음 · `0x42` |

```cpp
inline constexpr PkmFormat kPk4 {"PK4", 236, std::nullopt, std::nullopt};
inline constexpr PkmFormat kPk5 {"PK5", 220, 0x41, 0x42};
```

새 디코더를 만들지 않는다. **H1의 `decodePkm`을 220 B도 받게 넓히고, `parseMember`가 형식의 숫자를 보게** 한다.

### 왜 220 B를 그냥 받아도 되나 — 스트림 암호의 앞부분

배틀 스탯은 PID로 시작한 난수열을 **앞에서부터 한 u16씩** XOR한다. 84바이트를 풀 때 쓰는 난수는 100바이트를 풀 때의 **앞 42개와 똑같다**.
그러니 길이만 `encrypted.size() − kStoredSize`로 바꾸면 220 · 236 둘 다 맞게 풀린다. (CAN으로 치면 같은 키 스트림으로 짧은 프레임을 푸는 것)

### `std::optional` — "칸이 있을 때만"

```cpp
if (format.natureAt)                          // 값이 있나? (bool처럼 쓴다)
    member.nature = data[*format.natureAt];   // *로 꺼낸다
```

PK4는 `natureAt`이 비어 있어 이 줄을 건너뛰고, 위에서 계산한 `pid % 25`가 남는다. **세대를 묻지 않고 "그 칸이 있나"를 묻는다** —
그래서 `parseMember`에 `if (gen == 5)`가 없다. 7세대 형식이 성격을 다른 위치에 두면 `PkmFormat` 행의 숫자만 다르다.

### TODO

- `TODO(H6-CP4-1)` `decodePkm` (`pkmcodec.cpp`)
  - 크기 검사: "`kPk4.partySize`도 `kPk5.partySize`도 아니면" 빈 결과
  - 배틀 스탯 `cryptArray`의 길이: `kPartyPkmSize − kStoredSize` → `encrypted.size() − kStoredSize`
  - `std::copy`는 그대로 — `pkm.data`(236 B 배열)의 앞 220 B만 채우고 나머지는 0으로 남는다. `parseMember`가 읽는 칸(최대 `0x91`)은 전부 앞쪽이다
- `TODO(H6-CP4-2)` `parseMember` (`partyreader.cpp`): 위의 `optional` 두 줄 — 성격, 그리고 숨겨진 특성 `(data[…] & 0x01) != 0`
- `TODO(H6-CP4-3)` `readSeries`의 반복문 (`partyndsreader.cpp`): `kPartyPkmSize` 두 군데 → `series.pkm->partySize`,
  `parseMember(pkm)` → `parseMember(pkm, *series.pkm)`

테스트 `ReadsABlackWhiteParty`의 조로아크는 PID % 25 = 4인데 `0x41` = 9(촐랑)다. CP4-2를 빠뜨리면 바로 빨강이 된다.
반대로 `IgnoresTheFifthGenerationBytesInAPk4`는 PK4에서 `0x41`을 성격으로 읽지 않는지(4세대의 그 자리는 빛나는 잎) 지킨다.

확인: `ctest --preset linux-debug -R "PkmCodec|PartyReader|PartyNdsReader" --output-on-failure` → 전부 초록

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

H1의 readsav는 4세대 단계별 경로(footer → 슬롯 → PKM)를 찍는 도구다(이번에 `kNdsSeries`의 4세대 행을 쓰도록 옮겨 뒀다).
마지막의 `readParty` 비교 앞에 `TODO(H6-CP6)` 자리가 있다:

```cpp
for (const save::NdsSeries &s : save::kNdsSeries)
    qCDebug(lcSave).noquote() << /* 시리즈 이름 · (PartyNdsReader::readSeries(bytes, s) ? "OK" : "--") */;
```

그리고 `readParty:` 줄에 `gen %1`을 더한다. 기대 출력(`--verbose`, 소울실버):

```
D pokesix.save: diamond-pearl --
D pokesix.save: platinum --
D pokesix.save: heartgold-soulsilver OK
D pokesix.save: black-white --
D pokesix.save: black-2-white-2 --
I pokesix.save: readParty: gen 4 · heartgold-soulsilver · 6 members
```

`string_view`는 `QString::fromUtf8(name.data(), qsizetype(name.size()))`로 바꿔 찍는다(파일 안의 다른 줄과 같은 요령).
5세대 세이브는 이 도구의 앞쪽 4세대 경로에서 "no valid general block"으로 끝난다 — 5세대 단계별 경로까지 넣는 건 BW 세이브가 생겼을 때 할 일이다.

---

## 완료 확인 = H6 완료

### 4세대가 그대로인지 (회귀)

구조를 바꿨어도 손에 있는 진짜 세이브는 전과 같이 읽혀야 한다(참조 구현으로는 확인했다 — 직접 채운 코드로 다시):

```bash
export POKESIX_SAVE_HGSS="$HOME/pokesix-saves/포켓몬스터 소울실버(K).sav"
ctest --preset linux-debug -R RealSave --output-on-failure -V
```
첫 줄에 `generation 4, party at 0x00098`, 이어서 H1 때와 같은 6마리(프테라 Lv.48 · 물건 217 · 성격 4 …).
Pt · BW · B2W2는 파일이 없으면 `Skipped`가 정상이다.

### 완료 조건

- [ ] `ctest --preset linux-debug` 전부 통과(빨강 19개 → 0), 경고 0, `clang-format -i` (바꾼 파일들)
- [ ] `RealSave.HeartGoldSoulSilver` 통과 — 4세대 실파일 회귀 없음
- [ ] 앱에서 소울실버 세이브 불러오기 · 끌어다 놓기가 H2 때와 똑같이 된다(게임 소울실버, 6마리, 성격 · 물건까지)
- [ ] `grep -rn "kGeneration\|gen >= \|gen == " src/core/save src/data/store` 결과가 비어 있다 — 세대 숫자로 가르는 곳이 없다
- [ ] (선택) CP6 readsav 출력

5세대는 **합성 세이브로만** 확인한다 — 수치 출처는 PKHeX이고 실파일 검증은 ◇다([data/gen5/](../data/gen5/README.md)).
BW · B2W2 세이브가 생기면 `POKESIX_SAVE_BW` · `POKESIX_SAVE_B2W2`로 `RealSave`를 돌려 ✓로 바꾼다.

다 되면 **"H6 진단해줘"** → Claude가 diff · 빌드 · 테스트를 보고 커밋 · merge한다(새 `tr()` 문구가 없으니 번역 갱신은 없다).

## 다음 — 이 뒤에 할 수 있는 것

- 5세대 단계별 readsav 경로 · 실파일 검증(세이브가 생기면)
- 폼 표(로토무 · 기라티나 …, 5세대는 볼트로스 · 토네로스 · 큐레무까지) — 지금은 기본 폼
- 숨겨진 특성(`hiddenAbility`)을 스쿼드 화면에 표시 — 특성 번호(`0x15`)는 이미 맞는 특성이 온다. 표시만의 문제
- 다른 기기: `PartyGbaReader`(3세대 — 섹션 회전 · 다른 암호, `PkmFormat`로는 안 되고 디코더도 새로) · `Party3dsReader`(6 · 7세대 — 같은 암호, 칸 위치가 많이 달라 `PkmFormat`이 커진다)
