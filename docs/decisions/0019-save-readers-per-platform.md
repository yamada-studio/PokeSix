# 0019 — 세이브 리더는 기기별 클래스로, 기기 안의 시리즈는 표의 행으로

- 상태: 채택 (2026-10-08)
- 관련: [ADR 0018](0018-save-import-read-only.md) · [overlay-design.md §5](../overlay-design.md#5-세이브-파싱--core에-둔다-qt-없음) · 가이드 [h6-multigen-save.md](../guides/h6-multigen-save.md) · 로드맵 H6

## 배경

H1 · H2의 파서는 4세대 **안의** 시리즈(DP · Pt · HGSS)는 표로 판별했지만, 세대는 고정이었다(`saveimport`의 `kGeneration = 4`).
5세대(BW · B2W2)를 더하는 H6의 첫 뼈대는 **세대마다 파일 한 쌍**(`gen5.h/.cpp`)과 세대별 형식 목록(`SaveFormat`)이었다.
사용자 검토(2026-10-08)에서 두 가지 문제가 나왔다:

1. 세대가 늘 때마다 `genN.h/.cpp`가 생기고, 파티를 읽는 반복문이 세대마다 복사된다 — `if (gen == 5)`가 파일 단위로 바뀌었을 뿐이다
2. 숫자로 표현할 수 있는 차이(성격 칸 위치 · 숨겨진 특성 비트 · 레코드 크기)까지 세대별 코드에 들어가 있었다

세이브 구조는 세대보다 **기기**를 따라간다 — 같은 기기의 게임들은 같은 저장 매체 · 체크섬 방식을 쓴다
(GBA = 3세대, NDS = 4 · 5세대, 3DS = 6 · 7세대, Switch = 그 이후. Switch는 기기 안에서도 구조가 2–3가지다).

## 결정

1. **부모 인터페이스 `PartyReader`(`partyreader.h`) + 기기마다 자식 클래스 하나.** 파일 이름은 부모 이름을 앞에 두고 기기를 끼운다:
   `PartyNdsReader` → `partyndsreader.h/.cpp`, 나중에 `PartyGbaReader` · `Party3dsReader` · `PartySwitchReader`.
   파일 목록에서 `party…`끼리 모여 보이고, 기기 수만큼(최대 4쌍)만 늘어난다
2. **기기 안의 시리즈는 표의 행이다**(`kNdsSeries`: DP · Pt · HGSS · BW · B2W2). 행끼리 다른 것은 숫자와 "검증 방식" 하나
3. **함수는 검증 구조가 다를 때만 나눈다.** NDS는 두 가지: `FooterSlots`(4세대식 — 두 슬롯 + footer CRC) ·
   `ChecksumTable`(5세대식 — 체크섬 모음 블록 + 블록 CRC 사본). 행은 `std::variant`로 둘 중 하나를 담고, `std::visit`이 맞는 함수를 고른다.
   검증 함수는 `saveblock.h/.cpp`에 둔다
4. **포켓몬 한 마리 형식(`PkmFormat`: PK4 · PK5)은 기기와 다른 축이다.** 같은 암호가 NDS · 3DS · Switch에 걸쳐 쓰이므로
   `pkmcodec.h`에 데이터(크기 · 성격 위치 · 숨겨진 특성 위치)로 두고, 시리즈 행이 가리킨다
5. **판별은 그대로 체크섬 검증**(파일 크기가 아님). `readParty`가 기기 리더를 차례로, 리더가 자기 표의 행을 차례로 시도한다.
   결과 `ReadParty`는 기기 · 세대와 상관없는 모양(`generation` · `versionGroup` · `partyOffset` · `members`)이다

## 대안

- **세대마다 파일(`genN.*`) + 함수 포인터 형식 목록** — 첫 뼈대. 위 배경의 이유로 버렸다
- **기기 안에서 시리즈마다 함수**(`readDP` · `readBW` …) — 파일이 함수로 바뀔 뿐 같은 문제다
- **모든 것을 표 하나 + enum 검증 방식으로** — 검증 방식마다 필요한 숫자가 달라(footer 길이 vs 모음 블록 길이 · 사본 위치)
  한 구조체에 섞으면 "이 칸은 이 방식에서만 쓴다"가 늘어난다. `variant`로 방식별 숫자를 따로 묶었다
- **상태 없는 namespace 함수** — 클래스 대신 가능하지만, 기기 리더를 한 모양(부모 포인터 목록)으로 다루고 기기 내부를
  `private`으로 숨기려고 클래스를 택했다(가상 함수 · 다형성 학습 목적도 있다)

## 결과

- 6세대 이후를 더할 때: 3DS는 `Party3dsReader` 한 쌍 + 검증 방식 하나 + `PkmFormat` 행(칸 위치가 많이 달라 형식 표가 커진다).
  Switch는 기기 안에서 검증 방식이 여럿이다
- 4세대 코드(footer · 슬롯 판정 · 파티 조립)는 로직 변경 없이 옮겼다 — 실제 소울실버 세이브로 같은 결과 확인
- 5세대 수치는 PKHeX 기준이고 실파일 미검증(◇) — [data/gen5/](../data/gen5/README.md)
