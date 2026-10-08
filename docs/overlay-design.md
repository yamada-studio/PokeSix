# 에뮬레이터 오버레이 — 검토와 설계

- 상태: **검토 문서(Proposed)**. 1단계(세이브 읽기)는 [ADR 0018](decisions/0018-save-import-read-only.md)로 확정됐다 — 새 화면 대신 **스쿼드 불러오기가 `.sav`를 받는다**(읽기 전용). 로드맵 Phase H
- 날짜: 2026-10-04
- 대상: melonDS에서 4 · 5세대(DS) 정주행. "OP.GG처럼" 게임 창 옆에 현재 파티의 상성 · 문제점 · 추천 기술을 띄운다

관련: [architecture.md §9](architecture.md#9-확장-여지) · [roadmap.md 장기 목표](roadmap.md#장기-목표-phase-ag-이후)

---

## 1. 목표와 범위

**목표** — 플레이 중인 게임의 **실제 파티**를 PokeSix가 읽어, 스쿼드 화면과 같은 분석(약점 쌓임 · 내성 없음 · ×4 · 커버리지 구멍)과
"다음에 배울 만한 기술"을 **에뮬레이터 창 옆 작은 창**에 보여 준다. 지금 수동으로 짜는 SixSquad의 입력을 게임이 대신 채우는 것이다.

**1차 범위**
- 에뮬레이터: melonDS (Windows 먼저). 세이브 포맷은 에뮬레이터와 무관하므로 DeSmuME · 실기 덤프도 같은 코드로 읽힌다
- 게임: 4세대(DP · Pt · HGSS), 5세대(BW · B2W2). 3세대 이하(GBA)는 포맷이 다르므로 나중
- **읽기 전용.** 세이브 · ROM · 에뮬레이터 메모리에 절대 쓰지 않는다

**비범위(지금은 안 한다)** — 배틀 중 상대 포켓몬 읽기(라이브 메모리 필요, §3 B안), 치트 · 세이브 편집, 에뮬레이터 입력 자동화

## 2. 사용자 흐름

1. melonDS로 Platinum을 플레이하다가 게임 안에서 **리포트(저장)** 한다
2. PokeSix는 `.sav` 파일이 바뀐 것을 감지해 파티 6마리(종 · 레벨 · 기술 4 · 특성 · 성격 · 지닌 물건)를 읽는다
3. 오버레이 창이 게임 창 오른쪽에 붙어 **파티 카드 6 · 상성 히트맵 · 문제 목록 · 추천 기술**을 갱신한다
4. 사용자는 게임을 계속한다. 다음 저장 때 다시 갱신된다
5. (2차) "라이브 모드"를 켜면 저장 없이도 실시간으로 갱신되고, 배틀 중 상대의 타입이 보인다

## 3. 데이터를 어디서 읽나 — 선택지

| | A. `.sav` 파일 감시 | B. melonDS GDB stub | C. 프로세스 메모리 읽기 | D. melonDS Lua 포크 |
|---|---|---|---|---|
| 방법 | `QFileSystemWatcher`로 세이브 파일 변경 감지 → 파싱 | melonDS 설정 Devtools → "Enable GDB stub"(ARM9 3333 · ARM7 3334). GDB 원격 프로토콜 `m` 패킷으로 DS RAM 읽기 | `ReadProcessMemory`로 melonDS 프로세스의 메모리에서 DS 메인 RAM을 찾아 읽기 | NPO-197의 Lua 지원 포크로 스크립트가 RAM을 읽어 소켓으로 전달 |
| 갱신 시점 | 게임 안에서 저장할 때만 | 실시간 | 실시간 | 실시간 |
| 에뮬레이터 의존 | **없음**(포맷은 게임 것) | melonDS 공식 기능 | melonDS 빌드 · 버전마다 깨짐. JIT 빌드는 RAM을 특수하게 할당해 포인터 추적이 필요 | upstream이 아님 |
| 플랫폼 | 전부 | 전부(TCP) | Windows 전용(Win32 API) | 전부 |
| 제약 | 저장을 눌러야 한다 | **JIT와 상호 배타** → 에뮬 속도 저하. 메모리 읽기가 느리다(개발자 명시). 연결 중 에뮬이 멈추는지는 검증 필요 | 취약, 안티치트류 오해 소지, 유지보수 비용 큼 | 사용자가 포크 빌드를 써야 한다 |
| 구현 난이도 | 낮음(core 파서 + 파일 감시) | 중간(RSP 클라이언트 + RAM 주소 표) | 높음 | 낮음(그러나 배포 불가) |

**추천: A를 1차로, B를 2차 opt-in "라이브 모드"로. C · D는 하지 않는다.**

- A만으로도 목표의 대부분(파티 분석 · 추천 기술)이 된다. 정주행에서 파티는 저장 사이에 크게 바뀌지 않는다
- A의 파서는 B에서도 그대로 쓴다. 메모리 안의 파티 구조는 세이브의 파티 구조와 같은 PKM 포맷이다. 바뀌는 것은 "바이트를 어디서 가져오느냐"뿐이므로 **입력원(`PartySource`)을 인터페이스로 두고 A · B를 바꿔 끼운다**
- B의 열린 질문: GDB 프로토콜은 원래 CPU가 멈춘 상태에서 메모리를 읽는다. melonDS stub이 실행 중에 `m` 패킷을 받아 주는지, 아니면 매번 interrupt → 읽기 → continue(프레임 드랍)를 해야 하는지 실측해야 한다. 멈춰야 한다면 갱신 주기를 길게(수 초) 잡는다

### melonDS의 저장 동작

melonDS는 게임이 플래시에 쓰는 순간 **동기적으로** `.sav`에 쓴다(비동기 flush가 없어 저장 때 프레임이 떨어진다는 이슈가 있다). 즉 게임 안에서 저장하면 파일 변경이 바로 보인다.
단, 쓰는 도중에 읽을 수 있으므로 **디바운스(300ms 정도) + 체크섬 검증 실패 시 재시도**가 필요하다. 세이브 위치는 기본으로 ROM과 같은 폴더의 같은 이름(`게임.sav`)이고, 설정으로 바꿀 수 있다.

## 4. 게임 식별 — ROM은 헤더만 본다

- 어느 게임인지는 **ROM 헤더의 게임 코드 4바이트(오프셋 0x0C)** 로 안다. 예: `CPUE` = Platinum(미국), 4번째 글자가 지역(E 미국 · J 일본 · K 한국 · O 유럽/다국어)
- 세이브 파일만 있고 ROM 경로를 모르면: 같은 폴더의 같은 이름 `.nds`를 찾고, 없으면 사용자가 게임을 고른다(세이브 크기 512 KiB는 4 · 5세대가 같아 구분이 안 된다)
- 게임 코드 → PokéAPI `version`(과 `version_group`) 표는 `resources/data/gamecodes.json`에 둔다. PokéAPI에는 게임 코드가 없다
- **ROM 내용은 헤더 16바이트 외에 읽지도, 저장하지도 않는다.** 리포에는 게임 코드 표만 들어간다(에셋 아님)

| 게임 | 코드(앞 3글자) | 세대 | version_group |
|---|---|---|---|
| Diamond / Pearl | `ADA` / `APA` | 4 | diamond-pearl |
| Platinum | `CPU` | 4 | platinum |
| HeartGold / SoulSilver | `IPK` / `IPG` | 4 | heartgold-soulsilver |
| Black / White | `IRB` / `IRA` | 5 | black-white |
| Black 2 / White 2 | `IRE` / `IRD` | 5 | black-2-white-2 |

(코드는 구현 때 실제 헤더로 검증한다.)

## 5. 세이브 파싱 — core에 둔다 (Qt 없음)

세이브는 사용자 데이터이고 포맷은 공개 문서(Project Pokémon · Bulbapedia)와 PKHeX로 잘 알려져 있다.
**PKHeX는 GPL-3.0이므로 코드를 복사하지 않는다. 수치(오프셋 · 상수)만 참고하고 구현은 직접 한다.** 아래 수치는 모두 구현 때 재검증한다.

### 4세대 (DP · Pt · HGSS)

- 파일 512 KiB. **두 슬롯**(0x00000, 0x40000)에 각각 일반(small) 블록 + 보관(big) 블록. 각 블록 끝의 footer에 저장 카운터와 CRC-16/CCITT-FALSE가 있다 → CRC가 맞는 슬롯 중 **(major, minor) 카운터가 큰 쪽이 현재**(블록 끝 − 0x14 · − 0x10, PKHeX `SAV4BlockDetection`). CRC는 블록 끝의 u16, 범위는 블록에서 footer를 뺀 부분
- 일반 블록 크기 · footer 길이는 게임마다 다르다: **DP 0xC100 / 0x14 · Pt 0xCF2C / 0x14 · HGSS 0xF628 / 0x10**(HGSS 보관 블록은 0xF700에서 시작 — 앞선 판의 "0xF700"은 보관 블록 위치였다) → 게임별 `SaveLayout` 표 하나. 블록 크기가 달라 틀린 표로는 CRC가 맞지 않으므로 **세이브만으로 게임을 판별할 수 있다**(§4의 ROM 헤더는 DP 안에서 D/P를 가를 때만 필요)
- 파티: 일반 블록 시작 기준 파티 수 1바이트(**DP 0x94 · Pt 0x9C · HGSS 0x94**) + 그 4바이트 뒤부터 포켓몬 6 × **236바이트**(DP 0x98 · Pt 0xA0 · HGSS 0x98)
- PKM 236바이트 = 헤더 8(PID · 체크섬) + 데이터 128(32바이트 블록 A · B · C · D, **PID로 정한 순서로 섞여 있음**: `((PID >> 0xD) & 0x1F) % 24`) + 배틀 스탯 100
  - 데이터 128바이트는 **체크섬을 시드로 한 LCRNG**(`X[n+1] = 0x41C64E6D·X[n] + 0x6073`)의 상위 16비트와 2바이트씩 XOR 해서 푼다. 배틀 스탯은 PID를 시드로 같은 방식
  - 블록 A: 종(0x08) · 지닌 물건(0x0A) · 경험치 · 특성(0x15) · 노력치(0x18–0x1D). 블록 B: 기술 4(0x28–0x2F) · PP · 개체값(0x38, 5비트 × 6 + 비트 30 알 · 31 별명) · 폼(0x40의 위 5비트). 성격 = `PID % 25`(4세대는 성격 칸이 없다 — 5세대 PK5는 0x41에 따로 있다). 배틀 스탯: 레벨(0x8C) · 현재/최대 HP(0x8E · 0x90) · 능력치
- 알(egg) 플래그가 켜진 자리는 분석에서 뺀다

### 5세대 (BW · B2W2)

- 파일 512 KiB. 주 세이브 0x00000, 백업 0x24000. 블록마다 CRC16-CCITT가 있고 체크섬 모음 블록이 0x23F00 부근에 있다
- 파티 블록은 0x18E00 부근(BW), 포켓몬 6 × **220바이트**. 암호화 · 셔플은 4세대와 같은 방식, 필드 배치가 조금 다르다(PK5)
- B2W2는 블록 배치가 BW와 다르다 → 역시 `SaveLayout` 표의 행 하나
- **4세대와 다른 점(2026-10-08 정정)**: 슬롯 구조(주 · 백업)와 체크섬 방식(블록별 CRC를 모아 둔 표)이 4세대 footer와 다르고,
  PKM은 220바이트에 성격이 0x41에 따로 있다(PID % 25가 아니다). 그래서 "표의 행만 추가"로는 안 되고 **세대별 형식 전략**이 필요하다(아래)

### 여러 세대를 자동 판별하는 구조 (H1 · H2 이후 정리, 2026-10-08)

H1 · H2는 4세대만 다룬다. 4세대 **안의** 게임(DP · Pt · HGSS)은 표(`kGen4Layouts`)로 판별되지만, 세대 단위로는
코드가 4세대 모양이다: 슬롯 · footer 구조(`saveblock.h`), PKM 236바이트 · 칸 위치(`pkmcodec.h` · `parseMember`),
`readParty`가 `kGen4Layouts`만 순회, `saveimport`의 `kGeneration = 4` 상수.

다른 세대를 더할 때는 판별 원리를 한 단계 올린다 — **파일 크기가 아니라 체크섬 검증으로** 고른다(4 · 5세대는 둘 다 512 KiB):

```
struct SaveFormat {                 // 세대마다 하나 — 데이터(레이아웃 표 · 칸 지도) + 알고리즘(체크섬 검증 · PKM 풀기)
    int generation;
    std::optional<ReadParty> (*read)(Bytes save);   // 체크섬이 맞을 때만 값을 준다
};
inline constexpr std::array kFormats = { gen4::format, gen5::format /*, gen3::format */ };

readParty(save): kFormats를 차례로 시도 → 첫 성공. ReadParty에 generation을 담는다
saveimport:      kGeneration 상수 대신 party->generation. 출신 게임 번호 표도 세대별로 늘린다
```

- 같은 모양의 차이(블록 크기 · 칸 위치 · PKM 길이)는 **표**, 알고리즘이 다른 부분(체크섬 방식 · 3세대의 XOR 암호)은 **세대별 전략** — CLAUDE.md §4의 "데이터(테이블/전략)"
- 5세대는 4세대와 암호화 · 섞기가 같아 `pkmcodec`을 공유한다. 3세대(128 KiB · 섹션 회전 · PID ^ 트레이너 ID XOR · 100바이트 PKM)는 형식을 새로 짠다

### 게임 내부 번호 → PokéAPI id

- 종 번호 = 전국도감 번호(1–493 · 1–649) = PokéAPI `pokemon_species.id`. **폼**(기라티나 오리진, 로토무 폼, 쉐이미 스카이 …)은 폼 비트 → `pokemon.id`(10xxx) 표가 필요하다
- 기술 · 특성 번호 = PokéAPI id와 같다(4 · 5세대 범위 안에서)
- **지닌 물건은 다르다.** PokéAPI `item_game_indices.csv`(세대별 내부 번호)를 받아 변환한다 → [ADR 0011](decisions/0011-data-from-pinned-pokeapi-csv.md)의 고정 목록에 추가하고 스키마 버전을 올린다
- 성격 = `PID % 25` → PokéAPI `natures.game_index`

### 출력 타입 (core)

```cpp
// core/save/party.h — 세이브 · 메모리 어디서 읽든 같은 결과
struct ReadMember { int species; int form; int level; int item; int ability; int nature;
                    std::array<int, 4> moves; std::array<int, 6> ivs, evs; int hp, maxHp; bool egg; };
struct ReadParty  { std::string gameCode; int saveCount; std::array<std::optional<ReadMember>, 6> members; };
std::optional<ReadParty> readParty(std::span<const std::byte> save, const SaveLayout &layout);
```
세대 차이(블록 크기 · 오프셋 · PKM 길이)는 전부 `SaveLayout` **데이터**로 표현한다. `if (gen == 5)` 분기를 두지 않는다([CLAUDE.md §4](../CLAUDE.md)).

### 테스트

- 실제 세이브는 **리포에 넣지 않는다**(개인 데이터 + 게임 에셋 성격). 대신 테스트 코드가 **합성 세이브를 만든다**: 알려진 멤버 6마리를 PKM으로 인코딩(셔플 · 암호화의 역방향) → 블록 footer 체크섬 계산 → 두 슬롯 중 하나를 더 최신으로. 파서가 그 6마리를 복원하면 통과
- 인코더가 파서와 같은 수치를 쓰면 오프셋 오류를 못 잡는다. 그래서 **오프셋 표는 PKHeX · Project Pokémon 문서로 사람이 검증**하고, 가능하면 개발자 본인의 세이브로 로컬에서 한 번 맞춰 본다(커밋 안 함)

## 6. 아키텍처

의존 방향은 그대로 `app → ui → data → core`. 새 모듈만 더한다.

```
core/save/        SaveLayout(게임별 표) · PkmCodec(복호화 · 셔플) · readParty()          — 순수 C++20, gtest
data/emulator/    PartySource(인터페이스, 시그널 partyChanged)                            — QtCore
                  ├─ SaveFileSource   QFileSystemWatcher + 디바운스 + 체크섬 재시도 → core::readParty
                  └─ GdbMemorySource  (2차) QTcpSocket으로 GDB RSP, DS RAM 주소 표 → 같은 PKM 바이트 → core
                  GameIdentifier      ROM 헤더 · gamecodes.json → version_group
                  PartyBridge         ReadParty → Squad(id 변환: item_game_indices · 폼 표) → SquadSession(읽기 전용 "라이브" 모드)
ui/overlay/       OverlayWindow(항상 위 · 프레임 없음 · 반투명) · 슬롯 미니 카드 · HeatmapWidget 재사용 · 추천 기술 목록
                  EmulatorWindowTracker  (Windows) 게임 창 위치 추적 → 오버레이를 오른쪽에 붙임
app/              --overlay [sav 경로] 인자, 설정 탭 "오버레이" 항목
```

- **`SquadSession`을 그대로 쓴다.** 오버레이가 보여 주는 분석은 스쿼드 화면과 같은 `analyzeSquad` 결과다. 라이브 모드일 때는 세션을 읽기 전용으로 잠그고(편집 UI 비활성) 저장소(`squads.json`)에는 쓰지 않는다 — 게임이 진실이므로
- 플랫폼 코드(Win32 `EnumWindows` · `GetWindowRect` · `SetWinEventHook`)는 `ui/overlay/platform/`에 `#ifdef Q_OS_WIN`으로 가둔다. **Linux Wayland는 창 위치를 앱이 정할 수 없다** → 모든 OS에서 기본은 "항상 위 작은 창(사용자가 둔 자리 기억)"이고, Windows(와 X11)에서만 "게임 창에 붙이기"를 더한다
- 오버레이 창은 `Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint` + `WA_TranslucentBackground`. 게임 입력을 뺏지 않도록 포커스를 받지 않는다(`WA_ShowWithoutActivating`)
- 별도 실행 파일로 뺄 필요는 없다. 한 프로세스에서 메인 창을 숨기고 오버레이만 띄우는 "오버레이 모드"로 충분하다

## 7. 화면 (디자인 요청서로 넘길 초안)

```
┌─ melonDS ──────────────────────────┐ ┌─ PokeSix ─────────── 320 ─┐
│                                    │ │ Pt · 리포트 14:02      ⟳ ⚙ │
│                                    │ │ ──────────────────────────│
│                                    │ │ ▣ 토대부기  Lv42  풀 땅  ██▌│
│           (게임 화면)              │ │ ▣ 찌르호크  Lv40  노말 비행│
│                                    │ │ ▣ 루카리오  Lv39  격투 강철│
│                                    │ │ ▣ 비로드    Lv38  물      │
│                                    │ │ ▣ 로즈레이드 Lv37 풀 독   │
│                                    │ │ ▢ (빈 자리)               │
│                                    │ │ ──────────────────────────│
│                                    │ │ 상성  ⚠ 얼음 ×3  ⚠ 비행 ×2│
│                                    │ │ [노 불 물 풀 전 얼 격 독 …]│  ← 18칸 히트맵(기존 HeatmapWidget)
│                                    │ │ ──────────────────────────│
│                                    │ │ 추천 기술                 │
│                                    │ │ · 루카리오 Lv42 인파이트  │
│                                    │ │ · 비로드  TM13 냉동빔 ✔보유│
└────────────────────────────────────┘ └───────────────────────────┘
```

- 폭 320 고정, 높이는 게임 창에 맞춘다. 디자인 토큰 · 폰트는 기존 `ui/theme`(종이색 바탕, 먹선 패널) 그대로. 반투명은 바탕만
- 정식 시안은 `design/requests/overlay.md`로 요청해 `design/handoff-v3/`으로 받는다

## 8. 추천 기술 — core 로직

입력: `SquadAnalysis`(커버리지 구멍 `covered[t] == false`, 문제 목록) + 멤버별 배울 수 있는 기술(`SquadSession::learnableMoves` — 레벨업 · TM/HM · 가르침, 이미 있음) + (세이브에서 읽은) TM 가방.

규칙(표로 두고 가중치는 조정 가능하게):
1. **구멍 메우기 우선** — 커버되지 않는 방어 타입을 효과가 굉장하게 치는 기술
2. 같은 구멍이면 **자속(STAB) > 위력 ≥ 60 > 명중 ≥ 90** 순
3. **도달 가능성** — 레벨업 기술은 `현재 레벨 + 5` 안에 배우는 것만, TM은 **가방에 있는 것**(세이브의 TM 포켓)에 ✔ 표시, 없는 것은 획득처(기존 공략 사전, [ADR 0014](decisions/0014-guidebook-dictionaries.md))
4. 멤버당 최대 2개, 전체 최대 6개

출력은 "누가 · 무엇을 · 왜(메우는 타입) · 어떻게(Lv N / TM N 보유 / 획득처)". 계산은 core(`core/analysis/movesuggester`), 문구는 ui.

## 9. 법적 · 윤리 체크

- 읽기 전용. 게임 결과를 바꾸지 않고, 멀티플레이가 없으므로 치트 도구가 아니다(기존 세이브 편집기 · 상성 계산기와 같은 범주)
- ROM은 식별용 헤더만 읽고 저장하지 않는다. 세이브 · ROM · 스프라이트는 **리포에 커밋하지 않는다**([CLAUDE.md §7](../CLAUDE.md))
- PKHeX(GPL-3.0)는 수치 참고만. MIT 리포에 GPL 코드를 넣지 않는다
- "비공식 팬 도구" 표기는 인트로와 같은 문구를 오버레이 설정에도 둔다(로드맵 열린 질문 10)

## 10. 리스크와 열린 질문

| # | 질문 | 언제 |
|---|---|---|
| 1 | ~~두 슬롯 중 "현재"를 저장 횟수로만 판단해도 되는가~~ → CRC가 맞는 슬롯만 후보, 그중 (major, minor)가 큰 쪽(PKHeX 규칙). 한 번도 안 쓴 슬롯은 0xFF로 차 있어 CRC가 맞지 않는다 | H1 ✅ |
| 2 | melonDS가 파일을 쓰는 도중 읽었을 때 체크섬 검증으로 충분히 걸러지는가. 디바운스 값 | H2 |
| 3 | DeSmuME `.dsv`는 끝에 footer(122바이트)가 붙는다. 크기로 감지해 잘라낸다 | H2 |
| 4 | 폼 비트 → `pokemon.id` 표의 범위(4 · 5세대 폼만) | H2 |
| 5 | Wayland에서는 도킹이 불가 → "항상 위 창"만. X11은 도킹 가능 | H4 |
| 6 | GDB stub: 실행 중 메모리 읽기 가능 여부, JIT 끈 melonDS의 속도가 플레이 가능한지 | H7 |
| 7 | 라이브 모드에서 파티 **RAM 주소**(게임 · 지역 코드마다 다름) 표를 어디서 가져오나 | H7 |
| 8 | 오버레이가 보일 때 `SquadSession`의 편집을 막는 방식(읽기 전용 플래그 vs 세션 분리) | H2 |

## 11. 단계 나누기 (로드맵 Phase H 후보)

가치가 가장 빨리 보이는 순서다. **H2까지 하면 오버레이 창이 없어도 "게임 파티가 스쿼드 화면에 자동으로 들어오는" 결과가 나온다.**

| 단계 | 내용 | 배우는 것 | 완료 조건 |
|---|---|---|---|
| **H1** | core `save/`: DP · Pt · HGSS `SaveLayout` · PKM 복호화 · 셔플 · `readParty()` + 디버그 CLI `pokesix-read-sav`. 합성 세이브 gtest ([가이드](guides/h1-save-reader.md)) | 비트 연산 · LCRNG · `std::span` · 표로 표현한 포맷 · `QLoggingCategory` | 합성 세이브 테스트 통과. 본인 Pt · SS 세이브로 로컬 확인 |
| **H2** | (ADR 0018) 아이템 · 성격 내부 번호 → PokéAPI id(`item_game_indices`의 번호 · `natures.game_index`, 스키마 올림) · `PartyBridge`(ReadParty → Squad) · 스쿼드 **불러오기가 `.sav`를 받는다**(게임 전환 확인 · 드래그 앤 드롭). 파일 감시 · "라이브" 배지는 그다음 | `QFileDialog` 필터 · 드래그 앤 드롭(`QDropEvent`) · 레이어 경계의 타입 변환 | 불러오기로 Pt · SS 세이브를 고르면 스쿼드에 파티 6마리가 들어온다 |
| **H3** | `OverlayWindow`: 항상 위 · 프레임 없음 · 반투명 · 위치 기억 · `--overlay` 인자 | 창 플래그 · `WA_TranslucentBackground` · 포커스 정책 | 모든 OS에서 작은 창으로 파티 · 히트맵 · 문제가 보인다 |
| **H4** | Windows 창 추적(`EmulatorWindowTracker`) → 게임 창 오른쪽에 도킹, 이동 · 크기 변경 따라가기 | Win32 API를 Qt와 섞는 법, `#ifdef` 가두기 | melonDS 창을 옮기면 오버레이가 따라온다 |
| **H5** | 추천 기술(`movesuggester`) + TM 가방 읽기 | 점수 규칙을 표로, gtest | 커버리지 구멍마다 추천이 나온다 |
| **H6** | 세대별 형식 전략(`SaveFormat`)으로 구조 바꾸기 + 5세대(BW · B2W2) 형식 · PK5 | 표와 전략으로 세대 늘리기, 체크섬 검증으로 세대 자동 판별 | 4세대 테스트가 그대로 통과 + BW 세이브 6마리 복원 |
| **H7** | (선택) `GdbMemorySource` 라이브 모드 · 배틀 상대 타입 표시 | `QTcpSocket` · GDB RSP | 저장 없이 갱신, 상대 타입이 보인다 |

선행: Phase C(core 타입)와 D(DB · `SquadSession`)는 이미 있다. Phase E2(스쿼드 화면)의 히트맵을 재사용하므로 E2 뒤에 시작한다. Phase F · G와는 독립이다.

## 12. 참고 자료

- melonDS GDB stub: [PR #1583](https://github.com/melonDS-emu/melonDS/pull/1583) · [BlocksDS 디버깅 가이드](https://blocksds.skylyrac.net/docs/guides/debugging/)(포트 · 설정 위치)
- melonDS RAM 외부 읽기 논의: [melonDS board #476](https://melonds.kuribo64.net/board/thread.php?id=476)(JIT 빌드의 RAM 할당) · 저장 동기 쓰기: [Issue #477](https://github.com/melonDS-emu/melonDS/issues/477)
- melonDS Lua 포크(upstream 아님): [NPO-197/melonDS-lua](https://github.com/NPO-197/melonDS-lua) · [PR #1671](https://github.com/melonDS-emu/melonDS/pull/1671)
- 4세대 세이브 · PKM: [Bulbapedia — Save data structure (Gen IV)](https://bulbapedia.bulbagarden.net/wiki/Save_data_structure_(Generation_IV)) · [Pokémon data structure (Gen IV)](https://bulbapedia.bulbagarden.net/wiki/Pok%C3%A9mon_data_structure_(Generation_IV)) · Project Pokémon [DP](https://projectpokemon.org/home/docs/gen-4/dp-save-structure-r74/) · [Pt](https://projectpokemon.org/home/docs/gen-4/platinum-save-structure-r81/) · [HGSS](https://projectpokemon.org/docs/gen-4/hgss-save-structure-r76/) · [PKM](https://projectpokemon.org/home/docs/gen-4/pkm-structure-r65/) · [체크섬](https://projectpokemon.org/home/docs/gen-4/pok%C3%A9mon-nds-save-file-checksum-r79/)
- 5세대 세이브: Project Pokémon [BW](https://projectpokemon.org/docs/gen-5/bw-save-structure-r73) · [B2W2](https://projectpokemon.org/home/docs/gen-5/b2w2-save-structure-r72/)
- 수치 검증 기준: [PKHeX](https://github.com/kwsch/PKHeX) (GPL-3.0 — 참고만)
