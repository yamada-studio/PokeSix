# 작업 기록 (세션 인수인계)

이 프로젝트는 여러 머신 · 여러 Claude Code 세션(집 Windows 11 · 회사 · Linux)에서 이어 간다.
**새 세션은 [CLAUDE.md](../CLAUDE.md) → [roadmap.md](roadmap.md) → 이 문서** 순으로 읽는다.
Claude는 작업 덩어리가 끝날 때마다 이 문서의 "지금 상태"와 "최근 세션"을 갱신한다(커밋에 포함).
코드 · 커밋 · CHANGELOG에 이미 있는 것은 적지 않는다. 여기 적는 것은 **어디까지 검증됐고, 무엇이 미확인이고, 다음에 무엇부터 할지**다.

---

## 지금 상태 — 2026-10-09

- 2026-10-09(집 Windows): 회사 세션의 H1 · H2(스키마 12)를 Windows에서 처음 돌리다 만난 **DB 교체 실패(파일 잠금)** 를 고쳤다 — [build.md §5-8](build.md#db-swap-blocked). 145 테스트 통과
- **Phase A 완료 → `v0.1.0`** 태그(2026-10-07). 릴리스 CI가 태그로 3 OS 패키지를 GitHub Release에 붙인다 — 결과는 세션 기록에.
- 원격 `origin` = github.com/yamada-studio/PokeSix. `main`과 동기화.
- 버전 `0.1.0`. Phase B · C · D · E · T가 섞여 진행 중이고 Phase G는 "lite"(패키징 스크립트 · CI)만. 단계별 상태는 roadmap.md.

### 검증 매트릭스

| | setup | build + ctest | run | package | quickstart |
|---|---|---|---|---|---|
| Linux (Ubuntu 24.04) | ✅ 최소 Qt(`--archives`) 포함 | ✅ 96 | ✅ | ✅ AppImage · `--install` | ✅ |
| Windows 11 (MSVC 2022) | ✅ 전체 Qt. **최소 Qt 미검증**(이미 설치돼 있어 경로를 안 탐) | ✅ 96 (Release · Debug) | ✅ | ✅ ZIP 17.6MB, Qt 없는 PATH에서 실행 확인 | ✅ `--no-run` |
| macOS | ⚠ 실기 없음 | ⚠ | ⚠ | ⚠ dmg 스크립트만 | ⚠ |
| GitHub Actions | — | ✅ 3 OS(러너에서 ctest 포함) | — | ✅ `workflow_dispatch` 실행 37600397225: AppImage 38.9MB · dmg 31.4MB · ZIP 17.5MB 아티팩트. 태그 릴리스는 아직 안 만듦 | — |

### 진행 중

- 없음. **Phase H의 H1 · H2 완료**(세이브 → 스쿼드). 다음 후보: 폼 표 · 세이브 파일 감시(자동 갱신) · 전투 분석 입력 넓히기
  ([data/battle-inputs.md](data/battle-inputs.md) §4) · Phase B(컴포넌트 갤러리) · E4(설정). 사용자가 고른다

### 열린 일 (작은 것부터)

0. **VS 2026 지원**: GitHub의 `windows-latest`는 이제 `windows-2025-vs2026`(VS 18)이라 CI 잡을 `windows-2022`로 고정해 두었다. `windows-2022` 이미지가 은퇴하기 전에 `CMakePresets.json`에 "Visual Studio 18 2026" 프리셋과 `env.bat :find_vs`의 `[17.0,19.0)` 범위를 더해야 한다(Qt `msvc2022_64` 바이너리는 v145 툴셋과 ABI 호환)
1. MSVC 경고 C4305(`double`→`float`) 두 곳: `src/ui/widgets/versionchip.cpp:170`, `src/ui/dex/statradar.cpp:80`. 동작 영향 없음. `static_cast<float>` 또는 `qreal` 유지로 정리하면 된다(사용자 코드 영역 — 가이드 대상)
2. Windows 최소 Qt 설치 검증: `C:\Qt`가 없는 PC에서 `scripts\windows\setup.bat` → `--archives qtbase qtsvg qttools`만 받아 빌드되는지
3. 스쿼드 분석 패널의 문제 칸 높이(넓은 배치 최소 2줄 · 좁은 배치 최대 4줄, `squadpage.cpp`의 `kProblemVisibleRows`) — 사용자 피드백 대기
4. 에뮬레이터 오버레이: [overlay-design.md](overlay-design.md)는 검토 문서. 시작하려면 ADR로 결정을 확정하고 roadmap에 Phase H를 올린다(H1 = core 세이브 파서)
5. Windows 설정이 레지스트리 → `%APPDATA%\YamadaStudio\PokeSix.ini`로 바뀌었다(2026-10-07). 이전에 레지스트리에 저장된 세대 · 게임은 이어지지 않는다(한 번 다시 고르면 끝). 레지스트리 잔여물은 `HKCU\Software\YamadaStudio`에 남아 있어도 무해하다
6. ~~**Windows .msi 검증**~~ → 2026-10-09 집 PC에서 통과(WiX 3.14 winget 설치 · `package.bat` → msi · 설치 → 사용자 시작 메뉴 바로가기 → 설치된 exe 실행 → 제거 깨끗). 남은 것: CI 러너(windows-2022, WiX 프리인스톨)에서 msi가 나오는지는 v0.2.0 태그 실행으로 확인
7. macOS 공증(notarization): Apple Developer(연 99달러) + CI secrets(`notarytool`)를 붙이면 내려받은 dmg의 Gatekeeper "손상됨" 차단이 사라진다. 그 전까지는 README Download 절의 `xattr` 안내로 운용(2026-10-08). `scripts/macos/package.sh`의 ad-hoc 재서명은 실기 Mac에서 아직 안 돌려 봤다

---

## 환경 메모

### Windows 11 (집)

- VS 2022 Community 17.14(C++ 데스크톱, CMake 3.31 번들, clang-format은 `VC\Tools\Llvm\bin`), uv 0.12(winget portable → `%LOCALAPPDATA%\Microsoft\WinGet\Packages\astral-sh.uv_*`), Qt 6.8.3 전체 → `C:\Qt\6.8.3\msvc2022_64`. `QT_ROOT_DIR` 환경 변수는 **설정하지 않았다** — 스크립트가 기본 경로를 찾는다. IDE에서 프리셋을 직접 쓰려면 `setx QT_ROOT_DIR "C:\Qt\6.8.3\msvc2022_64"`
- 셸은 관리자 아님. winget으로 VS를 설치할 때 UAC 창이 뜬다
- `gh` CLI 없음. git 원격 인증은 Git Credential Manager
- 사용자는 이 머신에서 melonDS로 4세대(HGSS · Pt)를 플레이한다 → 오버레이 요구의 출처

### Linux (Ubuntu 24.04)

- `~/Qt/6.8.3/gcc_64`, `QT_ROOT_DIR`은 `~/.bashrc`. 142개 커밋(Phase G lite · Phase T · 스쿼드 기능)은 여기서 이루어졌다
- melonDS 1.1(AppImage)은 `~/melonDS-1.1/`, 4세대 ROM · 세이브는 `~/melonDS-1.1/gen4/`(melonDS가 같은 이름의 `.nds` ↔ `.sav`를 짝짓는다).
  2026-10-08 현재 `포켓몬스터 소울실버(K).sav`(한국판, 512 KiB)만 있다 — Pt 세이브는 아직 없다. H1 디버깅은 스냅샷 복사본
  `~/pokesix-saves/soulsilver.sav`로 한다(게임에서 저장하면 원본의 카운터 · 슬롯이 바뀌므로)

---

## 최근 세션 (역순)

### 2026-10-09 — Windows: 스키마 12 첫 실행의 DB 교체 실패 (집 세션)

- 증상: 첫 실행 패널 "데이터를 받지 못했어요 / cannot move …pokesix.sqlite.importing to …pokesix.sqlite". 원인: 설치본 `C:\PokeSix\bin\PokeSix.exe`(v0.1.0 · 스키마 11)가 떠 있어 옛 DB를 읽기 전용으로 잡고 있었고, 새 Debug 빌드(스키마 12)의 변환 결과를 Windows가 바꿔 넣지 못함. PowerShell 배타 열기로 잠금 확인. Linux에서는 열린 파일도 교체되므로 회사 세션에서는 안 보였다
- 수정(`gamedatabase` · `CsvImporter` · `DataUpdater` · `FirstRunPanel` · `Application` · `MainWindow`): 교체가 막히면 완성된 임시 DB를 남기고 `replaceBlocked`로 알림 → 패널이 "다른 PokeSix 창을 닫고 다시 시도" 안내, [다시 시도]는 재변환 없이 교체만, 다음 실행 때 `adoptPendingImport`가 DB를 열기 전에 교체. 같은 프로세스의 Repository는 변환 전에 닫는다(`aboutToReplaceDatabase`). 스키마 · CSV 커밋이 다른 임시 파일은 폐기. 테스트 3개(`PendingImport.*`)
- 실기 확인: 설치본이 떠 있는 상태에서 새 빌드 시작 → 로그 `cannot remove … (opened by another process?)`, 두 파일 모두 보존. **설치본을 닫고 다시 실행하면 11:27에 만든 `.importing`이 자동 적용된다**(사용자 확인 필요). 설치본은 `package.bat`/msi로 다시 만들어 교체하면 스키마가 맞는다
- **v0.2.0 릴리스 · msi 검증**(같은 날 오후): 버전 규칙에 "Phase 중간이라도 기능이 들어가면 MINOR"를 명시하고 0.2.0으로. WiX 3.14를 winget(관리자 UAC, NetFx3 필요)으로 설치해 `package.bat`으로 zip + msi 생성. 발견 · 수정 셋: ① 포터블 exe가 떠 있으면 `rmdir`이 조용히 실패하고 `cmake --install`에서 죽음 → `package.bat`이 바로 "실행 중인 PokeSix를 닫으라"고 멈춤. ② `windeployqt --compiler-runtime`은 VS 환경 변수 없이는 vc_redist를 안 넣어 **지금까지 Windows 패키지에 CRT가 없었다** → `InstallRequiredSystemLibraries`로 `msvcp140*` · `vcruntime140*`를 bin에 동봉(THIRD_PARTY_NOTICES 갱신). ③ Windows 패키지에는 offscreen 플러그인이 없어 `--offscreen` 테스트가 "no Qt platform plugin" 창을 띄웠다(사용자가 그 창을 봄) — 배포본 캡처는 창을 띄운 채로. msi: 설치(UAC) → `%APPDATA%` 시작 메뉴 바로가기(per-user 컴포넌트) → 설치된 exe가 Qt 없는 PATH에서 `--screenshot` 성공 → 제거 후 폴더 · 바로가기 · 제거 항목 모두 사라짐

### 2026-10-08 — H2 완료 · merge (Linux 세션)

- 사용자 구현 CP1–CP8(CP8 = 끌어다 놓기 안내 덮개, 사용자 추가 요청). 진단에서 고친 것: 스키마 · 변환기의 칸 누락과
  쉼표 실수(INTEGER라는 칸이 생김), 조회의 세대 조건 · bindValue 오타, egg 덮어쓰기(중복 줄), importFile 옮기기 미완,
  setAcceptDrops 위치 — 각각 원인을 설명하고 고침. 정리: 다 채운 TODO 삭제, 드롭 후 불러오기를 다음 루프로 미룸
  (확인 창이 드래그 원본을 막지 않게), 번역 7문구 × 2
- **스키마 12**: 업데이트 후 첫 실행에 데이터를 다시 만든다(캐시 CSV, 수 초) — CHANGELOG에 명시
- 실데이터 확인: 캐시 CSV로 만든 DB + 실제 SS 세이브 → soulsilver · 6마리, 성격 · 물건까지 H1 표와 일치. 앱에서 불러오기 · 드롭 · 덮개 사용자 확인
- 클린 빌드에서만 보이던 숨은 경고 2개(repository_test dangling else · VersionChip::Part 초기화) 정리
- UI 자동 테스트는 없다(core · data · env만) — 로직은 data에 두고 UI는 얇게. 화면이 안정되는 F–G쯤 `tests/ui/` 검토

### 2026-10-08 — H1 완료 · merge, H2 시작 (Linux 세션)

- H1 진단 통과(경고 0 · 테스트 131 · RealSave(SS) 통과 · clang-format): 사용자 구현 CP1–CP5 + 로그. 채운 TODO 주석은 지우고 merge
- 진단 중 발견해 고친 것: 파일 밖 footer가 CRC 0 = 0으로 "맞음"이 되던 뼈대 버그(`BlockFooter::inFile`), 도구가 512 KiB 미만 파일에서 멈추지 않던 것
- 사용자 기억의 스쿼드와 비교: 기술 24 · 특성 6 · 성격(홍수몬) 전부 일치, 파티 순서와 기술 칸 순서만 다름(세이브가 게임 순서)
- VS Code 디버깅 실습: 개인 `.vscode/launch.json` · `tasks.json`(git 제외) — 세이브 도구 · core 테스트 · 앱 세 설정

### 2026-10-08 — docs/data/ 세이브 · PK4 바이트 지도 (Linux 세션)

- 사용자 요청(최종 목표 = 전투 분석 · 시뮬): 세대 · 시리즈별 세이브 비트맵 → `docs/data/` (`gen4/` README · pk4 · dp · pt · hgss,
  `battle-inputs.md`). 수치는 PKHeX 대조, 바이트 지도는 정렬이 깨지지 않게 스크립트로 생성(scratch, 커밋 안 함)
- **실파일 검증(SS 한국판)**: 첫 포켓몬 프테라 Lv.48의 저장 능력치 6개가 IV · EV · 성격(PID % 25) · DB 종족값으로 계산한 값과 전부 일치.
  출신 게임 `0x5F` = 8, 만난 날 2026-10-06, 가방 TM 주머니 `0x09A0` 첫 칸 = 328(TM01), 트레이너 언어 8. DP · Pt는 실파일 없음(◇)
- 남은 것: 경험치 → 레벨 표(PokéAPI `experience` · `growth_rates`)는 받는 CSV 목록에 없음 — 박스 포켓몬을 읽을 때 추가.
  4세대 글자표(이름) 미정리. 데미지 공식은 시뮬 시작 때 출처 대조 후 별도 문서

### 2026-10-08 — docs/ui/ UI 구조 지도 (Linux 세션)

- 사용자 요청: Qt UI 구조를 그림으로 전수조사 → `docs/ui/` 11개 문서(상속 계층 · 화면별 배치 그림 · 객체 트리 ·
  시그널 흐름 · 모델/뷰 · 공용 부품 · 스타일). 조사는 영역별 하위 에이전트 5개로 병렬, 핵심 수치는 코드로 재확인
- 발견: 주석이 코드와 다른 곳 12곳(탭 4개 → 실제 5개, 인트로 메뉴 3개 → 4개 등)과 작은 동작 문제 2개(타운맵 아이템 아이콘
  `ready` 미연결, 아이템 표 상세 질의 두 번) — README 표에 기록만 하고 **코드는 고치지 않았다**(사용자 판단 대기)
- H1 작업 중인 브랜치를 건드리지 않으려고 별도 worktree에서 작성해 main에 바로 merge

### 2026-10-08 — H1 시작: 세이브 읽기 설계 확정 · 가이드 (Linux 세션)

- 사용자 결정: 세이브 화면을 새로 만들지 않고 **스쿼드 불러오기가 `.sav`를 받는다**, 읽기 전용(편집 · 내보내기 없음) →
  [ADR 0018](decisions/0018-save-import-read-only.md). 파서는 사용자가 직접(가이드 + 로그 + 디버그 CLI 단계별)
- overlay-design.md §5 수치 교정(PKHeX 대조): 파티 수 DP 0x94 · Pt 0x9C · HGSS 0x94, 파티 DP 0x98 · Pt 0xA0 · HGSS 0x98,
  일반 블록 HGSS 0xF628(앞선 판의 0xF700은 보관 블록 위치), footer 카운터는 세 게임 모두 끝 − 0x14 · − 0x10, CRC-16/CCITT-FALSE
- 뼈대: `src/core/save/`(savebytes · saveblock · pkmcodec · partyreader — 헤더 완성, 몸통 TODO), 도구
  `tools/readsav`(`pokesix-read-sav`, `pokesix.save` 로그 줄이 TODO), 테스트 32개(+ RealSave 2개는 환경 변수 있을 때만).
  테스트는 scratch의 버리는 참조 구현으로 32/32 통과 확인, 도구도 합성 세이브로 출력 확인(가이드의 기대 출력이 그것)
- ~~미확인: footer 끝 − 0x0C가 블록 크기인지~~ → 사용자 CP0(SS 한국판 실파일)으로 확인: `28 f6 00 00` = 0xF628. 같은 출력에서 HGSS는 major가 두 슬롯 모두 0이고 저장 횟수가 minor(100 · 99)에 있음을 확인 — 주석 · 가이드 교정.
  한국판 매직 0x20070903 외 레이아웃 차이는 없다고 보고 있음(PKHeX 기준)

### 2026-10-08 — 빌드 식별 문자열 · Windows .msi (Linux 세션)

- **버전 형상화(ADR 0017)**: 창 제목 · `--version`이 정확한 빌드를 말한다 — 태그 위 클린
  Release만 `0.1.0`, 그 외 `0.1.0+12.g3f4a5b6[.dirty]`, Debug는 ` Debug` 추가.
  `cmake/PokeSixBuildInfo.cmake`(빌드마다 실행, 내용 같으면 안 씀 — 재컴파일 0 실측) →
  `buildinfo.h` → `applicationVersion()`. 규칙 전문은 versioning-and-git.md §1
- **Windows 설치본**: CPack **WIX**로 `PokeSix-<버전>-win64.msi` — 시작 메뉴 바로가기,
  고정 UPGRADE_GUID(`D70F9D30-…`, 바꾸면 업그레이드가 깨진다), 라이선스 화면(MIT txt 자동
  변환). WiX 3.14는 windows-2022 러너에 프리인스톨(InnoSetup · NSIS도 있음 — WIX 채택 근거는
  deploy.md §2-4). `package.bat`이 zip 뒤에 cpack 실행, WiX 없으면 안내 후 건너뜀.
  릴리스 워크플로가 zip + msi 둘 다 첨부, checkout 3곳에 `fetch-depth: 0`(태그 없으면
  식별 문자열이 `+unknown`)
- **미검증**: msi 생성은 Windows에서만 돌므로 실기/CI 확인 전. CPack이
  `install(SCRIPT)`(windeployqt)를 스테이징에 돌리는 경로가 첫 관문 — workflow_dispatch
  한 번으로 확인 가능(이 머신엔 트리거 자격 증명이 없어 사용자가 눌러야 하고, 결과는
  public API로 여기서 읽을 수 있다)
- v0.1.0 Release에는 msi가 없다 — 다음 태그부터 포함. README Download 표에 msi 행 추가

### 2026-10-08 — macOS Gatekeeper "손상됨" (Linux 세션)

- 친구 Mac에서 v0.1.0 dmg가 **"PokeSix은(는) 손상되었기 때문에 열 수 없습니다"** — 파일 손상이
  아니라 Gatekeeper: 공증(notarization) 없는 앱을 브라우저로 받으면 quarantine이 붙고, 서명이
  Developer ID가 아니면 이 문구로 막는다(이 경우 **우클릭 → 열기 우회도 안 된다** — 기존 문서
  3곳의 안내가 틀렸었다)
- 조치: README에 **Download 절** 신설(3 OS 받는 법 + macOS `xattr -d com.apple.quarantine` 안내,
  Windows SmartScreen 안내), deploy.md · package.sh 주석 교정, `scripts/macos/package.sh`에
  macdeployqt 뒤 **ad-hoc 재서명**(`codesign --force --deep --sign -` + verify) 추가 — 서명이
  깨진 arm64 바이너리는 격리를 풀어도 안 떠서, 풀면 반드시 뜨게 하는 보험
- **v0.1.0 릴리스 노트에 macOS 안내를 덧붙이는 건 사용자 몫**(이 머신엔 GitHub 자격 증명 없음) —
  본문은 이 세션이 준비해 전달함. ad-hoc 재서명은 다음 태그 패키지부터 효력
- 공증(Apple Developer 연 99달러 + CI secrets)은 백로그 — 붙이면 경고 자체가 사라진다

### 2026-10-07 — Windows · quickstart 통과 · 릴리스 CI · Phase A 마무리 시작

- 사용자가 "진행해봐"로 추천안(태그는 Phase A 완료 뒤 `v0.1.0`)을 승인 → Phase A 남은 단계를 Claude가 구현하기 시작했다(ADR 0010의 연장)
- **A3 완료**: `src/app/`에 `Application`(composition root) — `main.cpp`는 이것만 만든다. AppState · Repository · DataUpdater를 여기서 만들어 `MainWindow`에 주입(생성자 시그니처 변경). 로그 형식 `hh:mm:ss.zzz L category: message`. `--screenshot <intro|dex|items|map|squad|settings> <WxH> <out.png> [--screenshot-delay ms]`(기본 1200ms = 부채꼴 등장 애니메이션 뒤). Windows offscreen에서 intro · dex · squad 캡처 확인. 디자인 03b의 `intro-gen-open`(세대 메뉴는 이제 카드 부채꼴이라 대상 없음) · `intro-first-run`(상태 강제 API 없음)은 만들지 않았다
- **A7 완료**: 인트로 메뉴 포커스 링 — Tab · Shift+Tab으로 들어온 포커스에만 선택 카드 바깥 파란 링(3px · 간격 2px). 부모(`IntroMenu`)가 여백(`kFocusMargin` 5)에 그린다 → 열린 질문 7을 [ADR 0016](decisions/0016-outside-decorations-drawn-by-parent.md)으로 확정. 메뉴 폭 = 520 + 10. 캡처 모드로는 Tab을 누를 수 없어 링 자체는 눈으로 확인하지 못했다(코드 리뷰 · 컴파일만) → 실기에서 Tab 한 번 눌러 확인할 것
- **A8 · A10 완료 → Phase A 종료**: A8의 "폭 300 팝업"은 ADR 0015 · 사용자 결정으로 다른 모양이 되어 있어 코드 변경 없이 차이를 [design/README.md "의도한 차이"](../design/README.md)에 적는 것으로 마무리. A10: 캡처(`--screenshot intro`) vs `30` · `31` 차이 목록 = 같은 표. 키보드 왕복을 위해 **Ctrl+0 → 인트로** 단축키를 더했다(마크 버튼은 Tab으로 멀다). 실기 키보드 확인은 사용자 몫
- **릴리스 `v0.1.0`**: CHANGELOG `Unreleased` → `[0.1.0] - 2026-10-07`, `CMakeLists.txt` VERSION 0.1.0, README 상태 줄. `chore(release): v0.1.0` 커밋 + 태그 → 릴리스 워크플로가 3 OS 패키지를 Release에 붙인다(결과 아래에 추가)
- **릴리스 결과**: 태그 실행(37604014683)은 3 OS 모두 패키지 · 아티팩트까지 성공했지만 **Release 첨부 단계가 403**("Resource not accessible by integration"). 원인: 조직(yamada-studio) 설정이 `GITHUB_TOKEN` 기본 권한을 read로 고정(리포 설정 API로 바꾸려 하면 409 "disabled by the organization"). 워크플로에 `permissions: contents: write`를 더해 main에 커밋했다(`670a8a7`) — 태그 뒤라 이번 태그에는 안 먹는다. 태그를 옮기지(force) 않고, **그 실행의 아티팩트 3개를 받아 Release를 손으로 만들어 공개했다**: https://github.com/yamada-studio/PokeSix/releases/tag/v0.1.0 (ZIP 17.5MB · AppImage 39.5MB · dmg 32.6MB, SHA-256은 릴리스 노트 안). **다음 태그에서 워크플로가 스스로 붙이는지 확인할 것** — 안 되면 조직 설정에서 "Workflow permissions"를 바꾸거나 PAT secret을 써야 한다

- Linux 쪽 142개 커밋을 받은 뒤 `quickstart.bat`이 Windows에서 세 군데 깨졌다 → 전부 고쳐 통과:
  `cardbarrel.cpp` most vexing parse(MSVC만 거부) · `package.bat`이 VS 번들 cmake를 안 찾음 · `QSettings` 기본 형식(Windows 레지스트리 + 테스트에 조직 이름 없음 → 쓰기 무시)으로 테스트 2개 실패.
  상세는 CHANGELOG `Fixed`와 커밋 `63f2115`
- 릴리스 CI 첫 실행(`workflow_dispatch`, 태그 없음): 1차 — Linux 성공, macOS는 AppleClang 15가 람다의 구조적 바인딩 캡처(`moveeffect.cpp`)를 거부, Windows는 `windows-latest`에 VS 2022가 없음. 둘 다 고친 2차(37600397225)는 3 OS 모두 성공, 소요 Linux 3.5분 · macOS 2.5분 · Windows 7.5분. 커밋 `3996834`
- GitHub API는 `gh` 없이 Git Credential Manager의 토큰(`repo` · `workflow` 범위)으로 호출했다. 헬퍼는 세션 임시 폴더에만 있었고 리포에는 없다 — 회사 세션에서는 `gh`를 쓰는 편이 낫다
- `docs/worklog.md`(이 문서) 시작, CLAUDE.md에서 연결

### 2026-10-04 — Windows 첫 세팅

- Windows 머신 세팅(VS · uv · Qt) 후 Debug 빌드 · 71 테스트 · 실행 확인. 스크립트 버그 3개(`shift` → `shift /1`, gtest discovery `PRE_TEST`, uv PATH) 수정
- 문서: [overlay-design.md](overlay-design.md)(에뮬레이터 오버레이 검토 · 설계), [deploy.md](deploy.md)(실행 CLI · 배포 방안 — 이후 Linux 세션이 방안 B · D를 실제로 적용함)
- 스쿼드 분석 패널 배치 변경(히트맵 고정 · 문제 칸 자체 스크롤 · 크게 보기 모달) — 사용자 요청으로 Claude 구현

### ~2026-10-06 — Linux (CHANGELOG `Unreleased` 참고)

- Phase G lite: `package.*` · `quickstart.*` · `qt_generate_deploy_app_script` · 릴리스 워크플로 · 최소 Qt 설치
- Phase T 타운맵 백과(성도 · 관동 · 신오 · 하나), 스쿼드(비전셔틀 · undo/redo · `.pks` 내보내기 · 이미지 공유 · 기술 출처 배지), 세대 버튼 줄무늬

---

## 이 리포에서 Claude가 배운 함정

- **Windows 전용 빌드 함정**은 [scripts/README.md "스크립트를 고칠 때"](../scripts/README.md)와 [build.md §5](build.md)에 모아 둔다(`shift /1`, `:find_cmake`, gtest `PRE_TEST`, DLL PATH). 새로 발견하면 거기에 추가
- GCC가 받아 주고 MSVC가 거부하는 코드가 있다(most vexing parse, 좁히기 변환 경고). Linux에서 큰 변경을 한 뒤에는 **Windows에서 `quickstart.bat --no-run`을 한 번 돌린다**
- 실행 중인 `PokeSix.exe`가 있으면 링크가 `LNK1168`로 실패한다. 창에 닫기 메시지를 보내면(`CloseMainWindow`) 저장소 소멸자가 flush하므로 편집이 보존된다
- Windows에서 테스트가 `QSettings`를 건드리면 반드시 `IniFormat` + 임시 폴더 `setPath`. 기본(레지스트리)은 조직 이름이 없으면 조용히 실패한다
- Claude Code의 Bash 도구(Git Bash)는 `\\`를 한 번 벗긴다. 백슬래시(Windows 경로 · `.bat`)가 들어가는 편집은 Read + Edit 도구로 한다. `.bat`은 CRLF 유지
