# H1 — 세이브(.sav)에서 파티 읽기: core 파서 + 로그 + 디버그 CLI

> 학습 루프 ① 가이드. 브랜치: `feat/h1-save-reader` (Claude가 만들어 둠)
> 뼈대 · 테스트 · CLI 도구의 틀은 준비돼 있다. **`// TODO(CPn-m)`만 채우면 된다.** 빈칸이 있어도 빌드된다.
> 결정 근거: [ADR 0018](../decisions/0018-save-import-read-only.md) — 읽기 전용, 스쿼드 불러오기로 착지(UI는 H2)

## 전체 그림

```
platinum.sav (512 KiB)
 ├─ 슬롯 0 @0x00000 ─ 일반 블록 [ … 파티 수 · 파티 236B×6 … | footer ] ─ 보관 블록(박스, 이번엔 안 씀)
 └─ 슬롯 1 @0x40000 ─ 같은 모양의 사본 (게임이 저장할 때마다 번갈아 쓴다)

CP2  두 footer의 CRC · 카운터 비교 ──▶ "지금" 일반 블록 (+ CRC가 맞는 표 = 게임 판별)
CP3  236바이트 한 마리 ──▶ 복호화(LCRNG XOR) ──▶ 블록 순서 되돌리기 ──▶ 체크섬 확인
CP4  평문 PKM ──▶ 종 · 레벨 · 기술 4 · 특성 · 성격 · 물건 · 개체값 …   (ReadMember)
CP5  위를 묶어 readParty(save) ──▶ ReadParty { 게임, 슬롯, 멤버 1–6 }
```

| | 체크포인트 | 파일 | 배우는 것 |
|---|---|---|---|
| ⬜ | **CP0 세이브를 눈으로 보기** | (코드 없음) `xxd` | 리틀 엔디언, footer, 두 슬롯 |
| ⬜ | **CP1 바이트 · CRC** | `src/core/save/savebytes.cpp` | 정수 승격 함정, 비트 시프트, CRC-16 |
| ⬜ | **CP2 footer · 슬롯** | `src/core/save/saveblock.cpp` + 도구 로그 | `std::span::subspan`, 이중 슬롯 저장, `QLoggingCategory` |
| ⬜ | **CP3 PKM 풀기** | `src/core/save/pkmcodec.cpp` + 도구 로그 | LCRNG, XOR 대칭성, 순열 표, gdb로 메모리 보기 |
| ⬜ | **CP4 필드 읽기** | `src/core/save/partyreader.cpp` `parseMember` + 도구 로그 | 비트 필드, "게임 번호 ≠ DB id" |
| ⬜ | **CP5 조립 = 완료** | `partyreader.cpp` `readParty` + 도구 로그 | 표로 하는 게임 판별, 본인 세이브 검증 |

각 CP는 같은 세 칸으로 진행한다: **① 구현**(core TODO) → **② 로그**(도구 TODO) → **③ 실행 · 디버그**(명령과 기대 출력).

### 준비된 것 (Claude가 만들어 둠)

| 파일 | 상태 |
|---|---|
| `src/core/save/*.h` | **완성.** 구조체 · 함수 선언 · 수치 표(`kGen4Layouts`, `kBlockPosition`) · 주석. 읽기만 한다 |
| `src/core/save/*.cpp` | 함수 몸통이 `TODO(CPn-m)` — 여기를 채운다 |
| `tools/readsav/main.cpp` | 인자 해석 · 파일 읽기 · 흐름 완성. 단계별 로그 줄이 `TODO(CPn-log)` |
| `tests/core/{savebytes,saveblock,pkmcodec,partyreader,realsave}_test.cpp` | **완성.** CP마다 완료 조건. 외부 검증 값(CRC 카탈로그 · 파이썬으로 계산한 LCRNG · Bulbapedia 순서 표)으로 짰다 |
| `tests/core/syntheticsave.h` | 알려진 포켓몬으로 합성 세이브를 만드는 도우미(파서의 역방향) |
| `.gitignore` | `*.sav *.dsv *.nds` — 세이브 · ROM이 실수로 커밋되지 않게 |

테스트는 Claude가 버리는 참조 구현으로 32개 전부 통과하는 것을 확인했다(참조 구현은 리포에 없다).
지금 뼈대 상태로는 **24개가 실패**하고 8개는 빈 몸통으로도 통과한다(예: "범위 밖은 0", "빈 세이브는 거절") — 정상이다.
빨강에서 시작해 CP마다 초록으로 바꿔 가는 TDD다.

---

## 시작 전에 (한 번)

### 세이브 복사 — 리포 밖에

```bash
mkdir -p ~/pokesix-saves
cp "<melonDS 세이브 폴더>/<플래티넘>.sav"  ~/pokesix-saves/platinum.sav
cp "<melonDS 세이브 폴더>/<소울실버>.sav"  ~/pokesix-saves/soulsilver.sav
ls -l ~/pokesix-saves          # 둘 다 524288 바이트(512 KiB)여야 한다
```

- 파서는 읽기만 하지만 **복사본으로 작업**한다. 원본은 게임 진행 그 자체다
- melonDS 세이브는 기본으로 ROM과 같은 폴더의 같은 이름 `.sav`. DeSmuME `.dsv`는 끝에 122바이트가 더 붙는데 그대로 읽힌다
- 리포 안에 두지 않는다(CLAUDE.md §7). `.gitignore`가 막아 주지만 처음부터 밖에 둔다

### 빌드와 단축 이름

```bash
cmake --build --preset linux-debug                 # 도구 · 테스트까지 빌드된다
RS=build/linux-debug/tools/readsav/pokesix-read-sav
T=build/linux-debug/tests/core/pokesix_core_tests

$RS ~/pokesix-saves/platinum.sav
```

지금(CP2 전) 기대 출력 — 파일은 읽히지만 블록을 못 고른다:

```
I pokesix.save: file /home/…/platinum.sav — 524288 bytes (0x80000)
C pokesix.save: no valid general block for any 4th-gen game — run with --verbose and read the footer lines
```

### 로그 읽는 법 — 이 가이드 내내 쓴다

core는 Qt를 모르므로 **로그를 찍지 않는다.** 대신 중간값을 구조체(`BlockFooter`, `DecodedPkm`)로 돌려주고,
도구(나중엔 data 레이어)가 받아서 찍는다. ROS 2로 치면 순수 C++ 라이브러리는 값만 돌려주고 노드가 `RCLCPP_*`로 남기는 구조다.

| Qt | ROS 2 | 이 도구에서 |
|---|---|---|
| `Q_LOGGING_CATEGORY(lcSave, "pokesix.save", QtInfoMsg)` | `rclcpp::get_logger("pokesix.save")` + 기본 레벨 INFO | `main.cpp` 위쪽에 이미 있다 |
| `qCDebug(lcSave)` · `qCInfo` · `qCWarning` · `qCCritical` | `RCLCPP_DEBUG` · `INFO` · `WARN` · `ERROR` | 단계 상세는 Debug, 결과 요약은 Info |
| `QT_LOGGING_RULES="pokesix.save.debug=true"` | `--ros-args --log-level pokesix.save:=debug` | 환경 변수로 Debug 켜기 |
| `QLoggingCategory::setFilterRules(...)` | 코드에서 레벨 바꾸기 | `-v` / `--verbose`가 이걸 부른다 |

- 출력 앞의 한 글자가 수준이다: `D` debug · `I` info · `W` warning · `C` critical (`qSetMessagePattern`이 만든 모양)
- **로그는 stderr로 나간다.** 거르려면 `$RS -v 파일 2>&1 | grep member`, 저장하려면 `2> log.txt`
- 메시지 조립은 `qCDebug(lcSave).noquote() << QStringLiteral("a=%1 b=%2").arg(x).arg(y);` 꼴. `.noquote()`가 없으면 QString에 따옴표가 붙는다
- 16진수는 `main.cpp`의 `hex(값, 자릿수)`를 쓴다: `hex(0xCF2C, 5)` → `0x0CF2C`

---

## CP0 — 세이브를 눈으로 보기 (코드 없음, 15분)

**목표**: `saveblock.h`의 표(`kGen4Layouts`)와 footer 주석이 **내 파일에서도 맞는지** 직접 확인한다. 이후 CP의 정답지가 된다.

### 리틀 엔디언 1분 정리

DS는 여러 바이트 정수의 **낮은 바이트를 앞에** 쓴다. 파일에 `2c cf 00 00`이 있으면 u32 값은 `0x0000CF2C`다(거꾸로 읽는다).

### 확인 1 — Pt footer (슬롯 0)

Pt 일반 블록 크기 = `0xCF2C`. footer를 보려면 블록 끝 − 0x14 = `0xCF18`부터 0x14바이트:

```bash
xxd -s 0xCF18 -l 0x14 ~/pokesix-saves/platinum.sav
```

`saveblock.h`의 `BlockFooter` 주석대로 4바이트씩 끊어 읽는다:

| 끝 기준 | 바이트(예) | 뜻 |
|---|---|---|
| −0x14 | `xx xx xx xx` | major — 저장 카운터 |
| −0x10 | `xx xx xx xx` | minor |
| −0x0C | `2c cf 00 00`? | 블록 크기 — **0xCF2C가 나오는지 확인**(주석이 "CP0에서 확인"이라고 적은 칸) |
| −0x08 | `23 06 06 20` 또는 `03 09 07 20` | 매직 0x20060623(일본 · 해외판) · 0x20070903(한국판) |
| −0x02 | `xx xx` | 저장된 CRC |

### 확인 2 — 슬롯 1과 비교

```bash
xxd -s $((0x40000 + 0xCF18)) -l 0x14 ~/pokesix-saves/platinum.sav
```

두 major 중 **큰 쪽이 지금 슬롯**이다(보통 1 차이). 노트에 적어 둔다: "Pt: 슬롯 ? 가 최신, major = ?". CP2의 정답이다.

### 확인 3 — SS(HGSS)

HGSS 일반 블록은 `0xF628`, footer는 0x10바이트다. 그래도 카운터는 세 게임 모두 **끝 − 0x14**부터 읽는다(PKHeX의 판정
규칙). HGSS에서는 그 칸이 footer(0x10) 바로 앞, CRC 범위 안에 놓인다 — 내 파일에서 두 슬롯의 이 값이 1 차이로 보이는지 확인한다:

```bash
xxd -s $((0xF628 - 0x14)) -l 0x14 ~/pokesix-saves/soulsilver.sav
xxd -s $((0x40000 + 0xF628 - 0x14)) -l 0x14 ~/pokesix-saves/soulsilver.sav
```

### 확인 4 — 파티 수

최신 슬롯의 시작(0 또는 0x40000) + 파티 수 위치(Pt `0x9C`, HGSS `0x94`)의 1바이트:

```bash
xxd -s $((0x40000 + 0x9C)) -l 1 ~/pokesix-saves/platinum.sav    # 최신이 슬롯 1일 때
xxd -s $((0x40000 + 0xA0)) -l 32 ~/pokesix-saves/platinum.sav   # 첫 포켓몬: 앞 4바이트 PID, 그 뒤는 암호문
```

게임 안 파티 마릿수와 같으면 CP0 끝. 4바이트 뒤(Pt `0xA0`)부터가 포켓몬인데, PID 다음은 의미 없는 바이트처럼 보인다 — 암호화돼 있어서다(CP3).

> 셸 팁: `$((0x40000 + 0xCF18))`은 bash가 10진수로 계산해 넘긴다. 16진수로 보고 싶으면 `printf '%X\n' $((…))`.

**✅ CP0 완료 조건**: 두 파일 모두 크기 512 KiB, 매직이 보이고, 블록 크기 칸이 표와 같고, 최신 슬롯과 파티 수를 노트에 적었다.

---

## CP1 — 바이트 읽기와 CRC (`savebytes.cpp`)

### ① 구현 — TODO CP1-1 … CP1-5

**readU16 / readU32** (CP1-1, CP1-2) — 리틀 엔디언을 정수로. 패턴:

```cpp
// 바이트 두 개 → u16: 높은 쪽을 밀고 OR
static_cast<std::uint16_t>(lo | (hi << 8))
```

함정 하나: `std::uint8_t`끼리의 연산은 먼저 `int`로 **승격**된다. u16은 괜찮지만, u32를 만들 때 `hi << 24`처럼 int로
밀면 부호 비트를 건드릴 수 있다 — 그래서 TODO는 `readU16` 두 개를 `std::uint32_t`로 바꿔서 합치라고 한다.
`SaveBytes.ReadsTheHighBit` 테스트가 이 함정을 잡는다.

**crc16Ccitt** (CP1-3 … CP1-5) — CRC는 "데이터를 다항식으로 나눈 나머지"다. 하드웨어 시프트 레지스터를 그대로 옮긴 비트 단위 방식:

```
crc = 0xFFFF
바이트마다:  crc ^= byte << 8            ← 바이트를 레지스터 위쪽에 넣고
             8번: 맨 위 비트가 1이면  crc = (crc << 1) ^ 0x1021   ← 다항식으로 "빼기"(XOR)
                  아니면              crc = crc << 1
```

결과를 매번 16비트로 잘라야 한다(`std::uint16_t`에 대입하면 저절로). 매개변수 이름이 같은 CRC가 수십 종류라서,
이름 대신 **검증 값**으로 확인한다: CRC-16/CCITT-FALSE("123456789") = `0x29B1`.

### ② 로그 — 이번 CP는 gtest 메시지가 로그다

```bash
ctest --preset linux-debug -R SaveBytes --output-on-failure
```

틀리면 이렇게 나온다 — "실제 값 / 기대 값"을 읽는다:

```
Expected equality of these values:
  crc16Ccitt(digits)
    Which is: 0
  0x29B1
    Which is: 10673
```

### ③ 실행 · 디버그

테스트 바이너리를 직접 돌리면 필터가 자유롭다:

```bash
$T --gtest_filter='SaveBytes.*'
$T --gtest_filter='SaveBytes.Crc16*' --gtest_break_on_failure   # gdb 안에서: 실패한 줄에서 멈춘다
```

gdb로 CRC 루프 따라가기:

```bash
gdb --args $T --gtest_filter='SaveBytes.Crc16MatchesTheCatalogCheckValue'
(gdb) break com::yamada::studio::save::crc16Ccitt
(gdb) run
(gdb) next                    # 한 줄씩
(gdb) display/x crc           # 멈출 때마다 crc를 16진수로 자동 출력
(gdb) info locals
(gdb) finish                  # 함수 끝까지 → 반환값 출력
```

**✅ CP1 완료 조건**: `ctest --preset linux-debug -R SaveBytes` 5/5.

---

## CP2 — footer와 지금 슬롯 (`saveblock.cpp`)

### 왜 슬롯이 두 개인가

게임은 저장할 때 **안 쓰던 쪽 슬롯에** 쓰고 카운터를 올린다. 쓰는 도중 전원이 꺼져도 다른 슬롯은 멀쩡하다.
로봇 펌웨어의 A/B 파티션 업데이트와 같은 이중 버퍼다. 그래서 규칙은:

1. CRC가 맞는 슬롯만 후보 (깨졌거나 한 번도 안 쓴 슬롯 — 전부 `0xFF` — 은 CRC가 안 맞는다)
2. 둘 다 후보면 `(major, minor)`가 큰 쪽

### ① 구현 — TODO CP2-1 … CP2-5

- **readFooter** (CP2-1, CP2-2): 다섯 칸을 `readU32` · `readU16`으로. CRC 범위는 블록 처음부터 footer 앞까지 —
  `save.subspan(blockStart, layout.generalSize - layout.footerSize)`. `std::span::subspan(시작, 길이)`은 복사 없이 창만 좁힌다
- **activeGeneralBlock** (CP2-3 … CP2-5): 두 footer를 읽고 위 규칙대로. 카운터 비교는 `std::pair`가 사전식 비교를 해 준다:

```cpp
std::pair{a.major, a.minor} > std::pair{b.major, b.minor}   // major 먼저, 같으면 minor
```

HGSS는 footer가 0x10이라 끝 − 0x14(major)가 **CRC 범위 안**에 있다 — `SaveBlock.HeartGoldFooterCrcCoversTheMajorCounter`가 이 차이를 지킨다.
코드에 `if (HGSS)` 분기를 둘 필요가 없다는 것도 확인해 본다: `footerSize`라는 **표의 값 하나**가 차이를 다 표현한다(CLAUDE.md §4).

### ② 로그 — `tools/readsav/main.cpp`의 TODO(CP2-log)

`logFooters()` 안, 게임 표 × 슬롯마다 한 줄. 모양 예:

```cpp
qCDebug(lcSave).noquote() << QStringLiteral("footer %1 slot@%2  major=%3 … crc=%7/%8 %9")
                                     .arg(게임이름, -20)   // -20 = 왼쪽 정렬 20칸
                                     .arg(hex(start, 5))
                                     …;
```

`layout.versionGroup`은 `std::string_view`라서 `QString::fromUtf8(sv.data(), qsizetype(sv.size()))`로 바꾼다.
`.arg()`의 인자가 문자열 세 개면 `.arg(a, b, c)` 한 번에 넣을 수도 있다.

### ③ 실행 · 디버그

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug -R SaveBlock --output-on-failure
$RS -v ~/pokesix-saves/platinum.sav
```

기대 출력(합성 세이브로 찍은 모양 — 숫자는 파일마다 다르다):

```
I pokesix.save: file platinum.sav — 524288 bytes (0x80000)
D pokesix.save: footer diamond-pearl        slot@0x00000  major=0 minor=0 size=0x0 magic=0x00000000 crc=0x0000/0x0216 --
D pokesix.save: footer diamond-pearl        slot@0x40000  major=0 minor=0 size=0x0 magic=0x00000000 crc=0x0000/0x29B4 --
D pokesix.save: footer platinum             slot@0x00000  major=41 minor=0 size=0xCF2C magic=0x20060623 crc=0xD224/0xD224 OK
D pokesix.save: footer platinum             slot@0x40000  major=42 minor=0 size=0xCF2C magic=0x20060623 crc=0x3EEB/0x3EEB OK
D pokesix.save: footer heartgold-soulsilver slot@0x00000  major=4294967295 … crc=0xFFFF/0x5257 --
D pokesix.save: footer heartgold-soulsilver slot@0x40000  major=4294967295 … crc=0xFFFF/0x2B06 --
I pokesix.save: game platinum · slot at 0x40000 · party 2
```

읽는 법: **그 게임의 표로 읽은 줄만 CRC가 맞는다**(`OK`). 다른 게임 표는 엉뚱한 자리를 footer로 읽어 쓰레기 값이 나온다 —
이게 CP5 게임 판별의 근거다. `game … slot at …`이 CP0 노트의 최신 슬롯과 같아야 한다. 같은 명령을 `soulsilver.sav`에도.

디버그 — 슬롯을 잘못 고를 때:

```bash
gdb --args $RS ~/pokesix-saves/platinum.sav
(gdb) break com::yamada::studio::save::activeGeneralBlock
(gdb) run
(gdb) next                    # footer 두 개를 읽는 줄을 지날 때까지(엔터 = 반복)
(gdb) p a                     # 구조체 전체가 보인다 (변수 이름은 네가 지은 이름)
(gdb) p/x a.storedCrc
```

**✅ CP2 완료 조건**: `ctest -R SaveBlock` 9/9, 두 세이브 모두 `-v`에서 자기 게임 줄만 `OK`, 고른 슬롯이 CP0 노트와 같다.

---

## CP3 — 포켓몬 한 마리 풀기 (`pkmcodec.cpp`)

### 236바이트의 모양 (`pkmcodec.h` 맨 위 주석)

```
0x00 PID · 0x06 체크섬                    평문
0x08 ┬ 블록 4개 × 32 (A B C D가 섞여 있음)  ← 시드 = 체크섬으로 암호화
0x87 ┘
0x88 ┬ 배틀 스탯(레벨 · HP …)              ← 시드 = PID로 암호화
0xEB ┘
```

### 개념 세 가지

**LCRNG** — `seed = seed × 0x41C64E6D + 0x6073`을 계속 굴리는 의사난수. `std::uint32_t`의 넘침은 C++에서 정의된 동작
(2³²로 나눈 나머지)이라 그대로 쓰면 된다. 매번 **상위 16비트**를 키로 2바이트에 XOR한다. 주의: 첫 키는 seed를
**한 번 굴린 뒤** 나온다(`PkmCodec.CryptAdvancesBeforeTheFirstWord`).

**XOR은 두 번 하면 원래대로** — 그래서 `cryptArray` 하나가 암호화도 복호화도 한다. 테스트 도우미(`syntheticsave.h`)가
같은 함수로 합성 세이브를 암호화하는 이유다.

**블록 섞기** — PID의 13–17비트로 24가지 순서 중 하나를 고른다. `kBlockPosition[s][i]`는 "논리 블록 i가 섞인 데이터의 **몇 번
자리**에 있나"다. s = 3 → `{0, 3, 1, 2}`: A는 0번, B는 3번, C는 1번, D는 2번 자리 → 섞인 순서는 `ACDB`.
되돌리기 = 논리 블록 i마다 `pos = kBlockPosition[s][i]` 자리의 32바이트를 i번째로 복사.

**순서가 중요하다**: 게임은 "섞기 → 암호화" 순으로 저장했다. 그래서 풀 때는 **복호화 → 되돌리기**. 암호 스트림은
위치마다 키가 달라서 순서를 바꾸면 쓰레기가 나온다.

### ① 구현 — TODO CP3-1 … CP3-9

주석이 한 줄씩 짚는다. span 자르기 패턴:

```cpp
std::span(pkm.data).subspan(kHeaderSize, kBlocksSize)   // 0x08부터 128바이트 — 수정 가능한 창
```

### ② 로그 — TODO(CP3-log)

멤버마다 `qCDebug` 한 줄: 몇 번째 · 파일 위치 · PID · shuffle · 체크섬 저장/계산. 예:

```
D pokesix.save: member 1 @0x400A0  pid=0x12345678 shuffle=2 checksum=0x4638/0x4638
```

### ③ 실행 · 디버그

```bash
ctest --preset linux-debug -R PkmCodec --output-on-failure
$RS -v ~/pokesix-saves/platinum.sav 2>&1 | grep member
$RS --hex ~/pokesix-saves/platinum.sav          # 풀린 236바이트를 16바이트씩
```

`--hex` 출력 읽기 — 0x08이 종 번호다:

```
I pokesix.save: 0x0000: 78 56 34 12 00 00 38 46 88 01 ea 00 00 00 00 00
                        └─ PID ───┘       └체크섬┘ └종┘ └물건┘
```

`88 01` → `0x0188` = 392(초염몽). 게임 화면의 포켓몬과 도감 번호가 맞으면 복호화 성공이다.

**증상 → 원인 표**

| 증상 | 의심할 곳 |
|---|---|
| 모든 멤버가 `checksum mismatch` | 시드를 바꿔 넣음(블록 ↔ 배틀), 바이트 순서(낮은 바이트를 `data[i]`에), 첫 키 전에 seed를 안 굴림 |
| 체크섬은 맞는데 종 번호가 이상 | 되돌리기를 안 했거나 방향이 반대(`kBlockPosition`을 "자리 → 블록"으로 읽음) — `PkmCodec.UnshuffleRestoresEveryBulbapediaOrder`가 어느 순서에서 틀리는지 알려 준다 |
| 일부 멤버만 틀림 | 그 멤버의 shuffle 번호를 로그에서 보고, 그 번호로 테스트 필터 |

gdb로 풀리는 과정 보기:

```bash
gdb --args $RS ~/pokesix-saves/platinum.sav
(gdb) break com::yamada::studio::save::decodePkm
(gdb) run
(gdb) next                             # 두 cryptArray 줄을 지날 때까지 반복(엔터 = 직전 명령 반복)
(gdb) x/32xb pkm.data._M_elems + 8     # 블록 영역 32바이트를 16진수로 (std::array의 내부 배열)
(gdb) p pkm.shuffle
(gdb) finish
```

**✅ CP3 완료 조건**: `ctest -R PkmCodec` 10/10, 두 세이브 모두 `-v`에 `checksum mismatch` 경고가 없다.

---

## CP4 — 필드 읽기 (`partyreader.cpp`의 `parseMember`)

> 한 마리의 **전체 바이트 지도**(전투에 쓰이는 칸 표시 · 실파일 검증 · 능력치 공식)는 [docs/data/gen4/pk4.md](../data/gen4/pk4.md). 아래는 CP4에 필요한 칸만 추린 것이다.

### 비트 필드

0x38의 u32 하나에 개체값 6개와 플래그 두 개가 들어 있다:

```
비트  31     30    29–25  24–20  19–15  14–10  9–5   4–0
      별명   알    특방   특공   스피드 방어   공격  HP
```

꺼내는 패턴은 "밀고 가리기": `(iv32 >> (5 * k)) & 0x1F`. 0x40의 바이트도 같은 식으로 위 5비트가 폼이다(`>> 3`).

**성격**은 4세대에 칸이 없다 — `PID % 25`로 정해진다(게임 순서: 0 노력 · 1 외로움 · 2 용감 · 3 고집 …).

### 게임 번호 ≠ DB id

| 칸 | PokéAPI id와 같은가 |
|---|---|
| 종 · 기술 · 특성 | **같다** — DB에서 바로 이름을 찾을 수 있다 |
| 지닌 물건 | **다르다** — 게임 내부 번호. H2에서 `item_game_indices`로 바꾼다 |
| 성격 | **다르다** — 게임 순서. H2에서 `natures.game_index`로 바꾼다 |

그래서 `ReadMember`는 번호만 담는다. 바꾸는 일은 data 레이어 몫이다(core는 DB를 모른다).

### ① 구현 — TODO CP4-1 … CP4-6

모든 위치는 **풀린 PKM 기준**이다(`partyreader.h` 주석). `const Bytes data(pkm.data);`가 이미 있다.

### ② 로그 — TODO(CP4-log)

멤버마다 `qCInfo` 한 줄 요약. 알이면 끝에 `(egg)`, 폼이 있으면 `#479-2`:

```
I pokesix.save: 1  #392 Lv.52  HP 100/120  item 234  ability 66  nature 21  moves 7 53 394 0
```

### ③ 실행 · 확인 — 게임 화면과 나란히

```bash
ctest --preset linux-debug -R PartyReader.Parses --output-on-failure
ctest --preset linux-debug -R PartyReader.ReadsTheEgg --output-on-failure
$RS ~/pokesix-saves/platinum.sav
```

melonDS에서 그 세이브를 불러와 파티 화면을 열고 비교한다. 번호를 이름으로 보려면 앱이 이미 만든 DB를 읽는 셸 함수:

```bash
pk() { python3 -c "import sqlite3,os,sys; db=sqlite3.connect(os.path.expanduser('~/.local/share/YamadaStudio/PokeSix/pokesix.sqlite')); print(db.execute(f'select name_ko from {sys.argv[1]} where id=?', (int(sys.argv[2]),)).fetchone()[0])" "$@"; }
pk species 392     # 초염몽
pk moves 394       # 플레어드라이브
pk abilities 66    # 맹화
```

(아이템 · 성격은 위 표대로 번호가 달라서 이 함수로는 안 맞는다 — 정상이다.)

**✅ CP4 완료 조건**: `PartyReader.ParsesEveryField` · `ReadsTheEggBitApartFromIvs` 통과, Pt · SS 파티의 종 · 레벨 · 기술 4개 · 특성이 게임 화면과 같다.

---

## CP5 — 조립과 게임 판별 (`readParty`) — H1 완료

### ① 구현 — TODO CP5-1 … CP5-4

`kGen4Layouts`를 차례로 `activeGeneralBlock`에 넣어 **값이 나오는 첫 행이 그 게임**이다. CP2 로그에서 본 것처럼 틀린 표로는
CRC가 맞지 않는다. `if (파일 크기 == …)` 같은 분기 없이 **표를 순회하는 것만으로** 게임 판별이 끝난다 — "규칙을 데이터로".

`ReadParty::layout`은 표 원소의 주소(`&layout`)다. `kGen4Layouts`는 `inline constexpr`라 프로그램 내내 살아 있어서 포인터가 안전하다.

### ② 로그 — TODO(CP5-log)

도구는 CP2–CP4에서 단계 함수를 **직접** 불러 한 마리씩 찍었다. 마지막에 `readParty`(앱이 쓸 한 번에 읽기)가 같은 답을
내는지 확인한다: 실패면 `qCCritical` 후 `return 3`, 성공이면 `readParty: <게임> · <n> members`, 멤버 수가 다르면 `qCWarning`.

### ③ 실행 — 완료 확인

```bash
cmake --build --preset linux-debug
ctest --preset linux-debug --output-on-failure            # 전체 초록

export POKESIX_SAVE_PT=$HOME/pokesix-saves/platinum.sav
export POKESIX_SAVE_HGSS=$HOME/pokesix-saves/soulsilver.sav
ctest --preset linux-debug -R RealSave --output-on-failure -V

$RS ~/pokesix-saves/platinum.sav
$RS ~/pokesix-saves/soulsilver.sav
```

`RealSave`는 환경 변수가 없으면 건너뛴다(skip). `-V`로 보면 읽은 파티가 찍힌다:

```
  /home/…/platinum.sav: slot 0x40000, save counter 42
  1  #392 Lv.52  moves 7 53 394 0  item 234  ability 66  nature 21
  …
```

**✅ H1 완료 조건**

- [ ] `ctest --preset linux-debug` 전부 통과(RealSave는 환경 변수 없이 skip, 있으면 pass)
- [ ] Pt · SS 두 세이브의 `$RS` 출력이 게임 파티 화면과 같다(종 · 레벨 · 기술 4 · 특성)
- [ ] `$RS -v`에서 footer 6줄 중 자기 게임 줄만 `OK`, `checksum mismatch` 없음
- [ ] 경고 0, `clang-format -i src/core/save/*.cpp tools/readsav/main.cpp`

다 되면 **"H1 진단해줘"** → Claude가 diff · 빌드 · 테스트를 보고 커밋 · merge한다.

---

## VS Code로 디버깅 (CMake Tools)

[build.md §VS Code](../build.md)대로 `launch.json` 없이 CMake Tools가 디버거를 붙인다.

1. 상태 막대에서 **Launch Target**을 `pokesix-read-sav`로 고른다
2. 인자는 `.vscode/settings.json`(git 제외)에:
   ```json
   "cmake.debugConfig": {
       "args": ["/home/<계정>/pokesix-saves/platinum.sav", "-v"]
   }
   ```
3. `src/core/save/pkmcodec.cpp`에 breakpoint → Ctrl+F5
4. **조사식(Watch)에서 16진수**: `pkm.data,x` · `footer,x` — 이름 뒤 `,x`
5. 테스트를 디버깅하려면 Launch Target을 `pokesix_core_tests`로, args에 `"--gtest_filter=PkmCodec.*"`, `"--gtest_break_on_failure"`

## 막혔을 때 물어보는 법

아래 둘을 붙이면 대부분 바로 원인이 보인다(세이브 파일은 이 머신의 `~/pokesix-saves`에 있으면 Claude가 직접 읽어 볼 수도 있다):

```bash
ctest --preset linux-debug -R "SaveBytes|SaveBlock|PkmCodec|PartyReader" --output-on-failure 2>&1 | tail -40
$RS -v --hex ~/pokesix-saves/platinum.sav 2>&1 | head -40
```

## 다음 — H2 미리 보기 (가이드는 그때 쓴다)

data 레이어: 아이템 · 성격의 게임 번호 → PokéAPI id(DB 스키마에 번호 칸 추가), `ReadParty` → `Squad` 변환,
스쿼드 **불러오기** 필터에 `*.sav` + 게임 전환 확인 + 드래그 앤 드롭. 같은 로그 카테고리(`pokesix.save`)를 data 레이어로 옮긴다.

## 참고

- 수치 출처: PKHeX `SAV4DP` · `SAV4Pt` · `SAV4HGSS` · `SAV4BlockDetection` · `PokeCrypto` · `PK4`
  (**GPL-3.0 — 수치만 참고, 코드는 보지 말고 직접 쓴다**)
- Bulbapedia [Pokémon data structure (Gen IV)](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_data_structure_(Generation_IV)) · [Save data structure (Gen IV)](https://bulbapedia.bulbagarden.net/wiki/Save_data_structure_(Generation_IV))
- Project Pokémon [PKM 구조](https://projectpokemon.org/home/docs/gen-4/pkm-structure-r65/) · [체크섬](https://projectpokemon.org/home/docs/gen-4/pok%C3%A9mon-nds-save-file-checksum-r79/)
- CRC 카탈로그(검증 값): [CRC-16/IBM-3740 (= CCITT-FALSE)](https://reveng.sourceforge.io/crc-catalogue/16.htm)
- 설계 전체: [overlay-design.md §5](../overlay-design.md)
