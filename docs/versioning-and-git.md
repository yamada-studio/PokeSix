# 버전 번호와 Git 운용

혼자 개발하는 학습 프로젝트라는 점을 기준으로, 필요한 만큼만 규칙을 둔다.
근거는 [ADR 0006](decisions/0006-semver-and-trunk-based-branches.md).

## 0. 역할 — Git은 Claude가 전담한다

사용자는 코드만 쓴다. 다음은 전부 Claude가 한다.

| 시점 | Claude가 하는 일 |
|---|---|
| 단계 시작 | `main`에서 `feat/<step>-<slug>` 브랜치 생성, 가이드 작성 |
| 진단 통과 | 사용자 코드를 커밋(`Refs: <step>`), `main`에 `--no-ff` merge, 브랜치 삭제, CHANGELOG `Unreleased` 갱신, 로드맵 체크 |
| Phase 완료 | 릴리스 커밋 · `vX.Y.Z` 태그 (§4) |
| 원격이 생기면 | `git push --follow-tags` |

force push, reset, 병합 안 된 브랜치 삭제처럼 기록을 지우는 작업은 먼저 묻는다.

## 1. 버전 번호 — Semantic Versioning

형식은 `MAJOR.MINOR.PATCH`([semver.org](https://semver.org/lang/ko/)).

| 자리 | 올리는 때 |
|---|---|
| MAJOR | 1.0.0 = 세 OS에서 빌드·실행되는 첫 공개 릴리스. 그 전까지는 0 |
| MINOR | **로드맵의 Phase 하나가 끝났을 때** |
| PATCH | 이미 릴리스한 기능의 버그 수정만 릴리스할 때 |

- 1.0.0 전(0.x)에는 호환성을 깨는 변경도 MINOR로 올린다(SemVer 규칙상 0.x는 무엇이든 바뀔 수 있다)
- 학습 단계(A1, A2 …) 하나가 끝났다고 버전을 올리지는 않는다. 단계는 브랜치와 merge 커밋으로 추적한다

### Phase와 버전 (예정)

| 버전 | 완료되는 것 |
|---|---|
| **v0.0.1** | Phase 0 — 프로젝트 골격, 빌드·테스트 파이프라인, OS별 스크립트, 문서 |
| v0.1.0 | Phase A — 홈 화면 완성 (창, 레이아웃, 캡처 도구, 테마, PanelFrame, 세대 카드, 앱 막대) |
| v0.2.0 | Phase B — 컴포넌트 갤러리 |
| v0.3.0 | Phase C — core 규칙과 분석(04 문서 테스트 통과) |
| v0.4.0 | Phase D — SQLite, 모델, 스쿼드 저장, PokéAPI 가져오기 |
| v0.5.0 | Phase E — 도감 · 스쿼드 · 아이템 · 설정 화면 |
| v0.6.0 | Phase F — 반응형, 설정, 다크 테마 |
| **v1.0.0** | Phase G — 3개 OS CI, 패키징, 첫 공개 릴리스 |

B와 C처럼 순서가 바뀌어 끝나면 **먼저 끝난 Phase가 다음 MINOR 번호를 받는다.** 표는 예정일 뿐이다.

### 버전 값은 한 곳에만

- 기준은 루트 `CMakeLists.txt`의 `project(PokeSix VERSION x.y.z)` 하나다
- 이 값은 **마지막으로 릴리스한 버전**이다. 릴리스 커밋에서만 바꾼다
- 앱은 `QApplication::applicationVersion()`으로 읽는다(`main.cpp`에서 `POKESIX_VERSION`으로 설정)
- 태그는 `v` 접두사를 붙인 annotated tag: `v0.1.0`

## 2. 브랜치 — main 하나 + 짧은 작업 브랜치

```
main ──●────────●──────────────●────────●──▶   (항상 빌드·테스트 통과, 릴리스 태그는 여기에)
        \      /  \            /
         ●──●─●    ●──●──●──●─●
     feat/a1-mainwindow   feat/a2-app-state
```

- 오래 사는 브랜치는 `main` 하나뿐이다
- 작업마다 `main`에서 브랜치를 따고, 끝나면 `main`에 merge한 뒤 지운다

### 이름 규칙

```
<type>/<slug>                 일반 작업         docs/build-vscode
<type>/<step>-<slug>          로드맵 단계       feat/a1-mainwindow
```

| type | 용도 | Conventional Commits type과 대응 |
|---|---|---|
| `feat/` | 기능, 로드맵 단계의 구현 | `feat` |
| `fix/` | 버그 수정 | `fix` |
| `refactor/` | 동작 변화 없는 구조 변경 | `refactor` |
| `test/` | 테스트만 추가·수정 | `test` |
| `docs/` | 문서만 | `docs` |
| `build/` | CMake, 프리셋, 스크립트, CI | `build` |
| `chore/` | 그 외 잡무(의존성 버전 등) | `chore` |
| `exp/` | 실험·학습용. **merge하지 않고 버린다** | — |

- 소문자와 하이픈만 쓴다. step id는 로드맵의 id를 소문자로(`a1`, `b3`, `d4`)
- 로드맵 단계는 대부분 `feat/`다. 코어 TDD 단계는 `feat/d1-type-chart`처럼 쓴다
- 디버깅만을 위한 브랜치 type(`debug/`)은 두지 않는다. 버그를 고치면 `fix/`,
  "Q_OBJECT를 빼면 어떻게 되나" 같은 실험은 `exp/`
- `develop/`이나 버전 이름 브랜치(`develop/0.1.0`)를 두지 않는 이유
  - 혼자 개발하면 `develop`과 `main` 두 줄을 오가는 merge가 늘기만 하고 얻는 게 없다
  - 버전은 "선(line)"이 아니라 "점(point)"이다. 점은 태그로 찍는다
  - 여러 버전을 동시에 유지보수하게 되면(예: 1.x를 고치면서 2.x 개발) 그때 `release/1.x`를 만든다

### merge

```bash
git switch main
git merge --no-ff feat/a1-mainwindow      # merge 커밋으로 "단계 하나" 단위를 남긴다
git branch -d feat/a1-mainwindow
```

- merge 전에 `main`이 앞서 나갔으면 `git rebase main`으로 브랜치를 최신화한다
- merge 조건: 빌드 경고 0 · `ctest` 통과 · `clang-format --dry-run --Werror` 통과 · (로드맵 단계라면) 진단 완료
- 브랜치 안의 WIP 커밋은 허용한다. 학습 과정이 기록으로 남는 편이 낫다

## 3. 커밋 메시지 — Conventional Commits

```
<type>(<scope>): <summary>

<body: 무엇을, 왜 — 선택>

<footer — 선택>
```

- **영어**, 명령형 현재 시제, 요약은 72자 이내, 끝에 마침표 없음
- type: `feat` `fix` `refactor` `test` `docs` `build` `style` `chore`
- scope: `core` `data` `ui` `app` `build` `test` `docs` `design` `release`
- footer
  - 로드맵 단계: `Refs: A1`
  - 호환성을 깨는 변경: `BREAKING CHANGE: …` (또는 `feat(core)!: …`)

예:
```
feat(ui): add MainWindow with minimum window size

Replace the placeholder QMainWindow in main.cpp with a Q_OBJECT subclass
so later steps can attach signals and a central widget.

Refs: A1
```
```
fix(core): treat steel as neutral to ghost from gen 6
test(core): cover gen 1 type chart exceptions
build: pin GoogleTest to 1.17.0
chore(release): v0.1.0
```

## 4. 릴리스 절차

Phase의 모든 단계가 ✅가 되면 다음 순서로 릴리스한다.

1. `main`에서 빌드, `ctest`, 포맷 검사가 통과하는지 확인한다
2. [CHANGELOG.md](../CHANGELOG.md)의 `Unreleased` 항목을 새 버전 제목 아래로 옮긴다
3. `CMakeLists.txt`의 `project(... VERSION x.y.z)`를 올린다
4. 커밋한다: `chore(release): vX.Y.Z`
5. 태그를 단다: `git tag -a vX.Y.Z -m "PokeSix vX.Y.Z — <Phase 요약>"`
6. (원격이 생기면) `git push --follow-tags`
7. [roadmap.md](roadmap.md)의 Phase 제목 옆에 버전을 적는다

## 5. CHANGELOG

- [Keep a Changelog](https://keepachangelog.com/ko/1.1.0/) 형식, 영어(공개 리포)
- merge할 때마다 `Unreleased`에 한 줄씩 추가한다(`Added` / `Changed` / `Fixed` / `Removed`)
- 커밋 로그를 그대로 복사하지 않는다. **사용자에게 의미 있는 변화**만 적는다

## 6. Git이 무시하는 것

[.gitignore](../.gitignore) 참고.
- 빌드 산출물: `build/`, `out/`, `install/`
- IDE: `.vscode/`, `.idea/`, `cmake-build-*/`, `.vs/`, `*.user`, `.qtcreator/`
- 개인 설정: `CMakeUserPresets.json`
- 런타임 데이터: `*.sqlite`, `*.db`, `aqtinstall.log`
- 게임 에셋: `/resources/sprites/`
