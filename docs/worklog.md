# 작업 기록 (세션 인수인계)

이 프로젝트는 여러 머신 · 여러 Claude Code 세션(집 Windows 11 · 회사 · Linux)에서 이어 간다.
**새 세션은 [CLAUDE.md](../CLAUDE.md) → [roadmap.md](roadmap.md) → 이 문서** 순으로 읽는다.
Claude는 작업 덩어리가 끝날 때마다 이 문서의 "지금 상태"와 "최근 세션"을 갱신한다(커밋에 포함).
코드 · 커밋 · CHANGELOG에 이미 있는 것은 적지 않는다. 여기 적는 것은 **어디까지 검증됐고, 무엇이 미확인이고, 다음에 무엇부터 할지**다.

---

## 지금 상태 — 2026-10-07

- `main` 최신: `fix(windows): make quickstart.bat pass end to end on MSVC` merge까지. 작업 트리 깨끗.
- 원격 `origin` = github.com/yamada-studio/PokeSix. 태그 없음, 릴리스 없음. 릴리스 CI(`.github/workflows/release.yml`)는 **실제 러너에서 아직 한 번도 돌지 않았다** → 아래 "진행 중".
- 버전 `0.0.1`(Phase 0). Phase A~E가 섞여 진행 중이고 Phase G는 "lite"(패키징 스크립트 · CI 파일)만 들어왔다. 단계별 상태는 roadmap.md.

### 검증 매트릭스

| | setup | build + ctest | run | package | quickstart |
|---|---|---|---|---|---|
| Linux (Ubuntu 24.04) | ✅ 최소 Qt(`--archives`) 포함 | ✅ 96 | ✅ | ✅ AppImage · `--install` | ✅ |
| Windows 11 (MSVC 2022) | ✅ 전체 Qt. **최소 Qt 미검증**(이미 설치돼 있어 경로를 안 탐) | ✅ 96 (Release · Debug) | ✅ | ✅ ZIP 17.6MB, Qt 없는 PATH에서 실행 확인 | ✅ `--no-run` |
| macOS | ⚠ 실기 없음 | ⚠ | ⚠ | ⚠ dmg 스크립트만 | ⚠ |
| GitHub Actions | — | ✅ 3 OS(러너에서 ctest 포함) | — | ✅ `workflow_dispatch` 실행 37600397225: AppImage 38.9MB · dmg 31.4MB · ZIP 17.5MB 아티팩트. 태그 릴리스는 아직 안 만듦 | — |

### 진행 중

- **첫 태그 릴리스**: CI는 검증됐다(위). 남은 것은 **어느 버전으로 태그를 칠지**의 결정 — 규칙은 "Phase 완료 = MINOR"인데 완료된 Phase가 없다. 선택지: (a) 지금 CMake 버전 그대로 `v0.0.1` 태그 → 패키지 이름 `PokeSix-0.0.1-*`(CHANGELOG의 0.0.1 절은 Phase 0만 적혀 있어 어긋남), (b) Phase A를 끝내고 `v0.1.0`, (c) 중간 빌드는 태그 없이 `workflow_dispatch` 아티팩트(90일 보관)로만 공유. 사용자 결정 대기.

### 열린 일 (작은 것부터)

0. **VS 2026 지원**: GitHub의 `windows-latest`는 이제 `windows-2025-vs2026`(VS 18)이라 CI 잡을 `windows-2022`로 고정해 두었다. `windows-2022` 이미지가 은퇴하기 전에 `CMakePresets.json`에 "Visual Studio 18 2026" 프리셋과 `env.bat :find_vs`의 `[17.0,19.0)` 범위를 더해야 한다(Qt `msvc2022_64` 바이너리는 v145 툴셋과 ABI 호환)
1. MSVC 경고 C4305(`double`→`float`) 두 곳: `src/ui/widgets/versionchip.cpp:170`, `src/ui/dex/statradar.cpp:80`. 동작 영향 없음. `static_cast<float>` 또는 `qreal` 유지로 정리하면 된다(사용자 코드 영역 — 가이드 대상)
2. Windows 최소 Qt 설치 검증: `C:\Qt`가 없는 PC에서 `scripts\windows\setup.bat` → `--archives qtbase qtsvg qttools`만 받아 빌드되는지
3. 스쿼드 분석 패널의 문제 칸 높이(넓은 배치 최소 2줄 · 좁은 배치 최대 4줄, `squadpage.cpp`의 `kProblemVisibleRows`) — 사용자 피드백 대기
4. 에뮬레이터 오버레이: [overlay-design.md](overlay-design.md)는 검토 문서. 시작하려면 ADR로 결정을 확정하고 roadmap에 Phase H를 올린다(H1 = core 세이브 파서)
5. Windows 설정이 레지스트리 → `%APPDATA%\YamadaStudio\PokeSix.ini`로 바뀌었다(2026-10-07). 이전에 레지스트리에 저장된 세대 · 게임은 이어지지 않는다(한 번 다시 고르면 끝). 레지스트리 잔여물은 `HKCU\Software\YamadaStudio`에 남아 있어도 무해하다

---

## 환경 메모

### Windows 11 (집)

- VS 2022 Community 17.14(C++ 데스크톱, CMake 3.31 번들, clang-format은 `VC\Tools\Llvm\bin`), uv 0.12(winget portable → `%LOCALAPPDATA%\Microsoft\WinGet\Packages\astral-sh.uv_*`), Qt 6.8.3 전체 → `C:\Qt\6.8.3\msvc2022_64`. `QT_ROOT_DIR` 환경 변수는 **설정하지 않았다** — 스크립트가 기본 경로를 찾는다. IDE에서 프리셋을 직접 쓰려면 `setx QT_ROOT_DIR "C:\Qt\6.8.3\msvc2022_64"`
- 셸은 관리자 아님. winget으로 VS를 설치할 때 UAC 창이 뜬다
- `gh` CLI 없음. git 원격 인증은 Git Credential Manager
- 사용자는 이 머신에서 melonDS로 4세대(HGSS · Pt)를 플레이한다 → 오버레이 요구의 출처

### Linux (Ubuntu 24.04)

- `~/Qt/6.8.3/gcc_64`, `QT_ROOT_DIR`은 `~/.bashrc`. 142개 커밋(Phase G lite · Phase T · 스쿼드 기능)은 여기서 이루어졌다

---

## 최근 세션 (역순)

### 2026-10-07 — Windows · quickstart 통과 · 릴리스 CI

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
