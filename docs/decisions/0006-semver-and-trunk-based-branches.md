# 0006. SemVer(Phase = MINOR)와 main 하나 + 짧은 작업 브랜치

- 상태: Accepted
- 날짜: 2026-09-28

## 맥락

개발자 한 명이 로드맵 단계 단위로 작업하고, 단계마다 진단을 받은 뒤 다음으로 넘어간다.
버전 번호와 브랜치 규칙이 필요하다. 후보로 나온 것은 두 가지였다.
- 작업 종류로 나누는 브랜치(`feat/`, `fix/`, `develop/`, `debug/` …)
- 버전 이름 브랜치(`develop/0.1.0`)

## 선택지

1. **GitFlow** (`main` + `develop` + `feature/*` + `release/*` + `hotfix/*`)
   - 장점: 여러 명이 병행 개발하고 릴리스를 따로 안정화할 때 좋다
   - 단점: 혼자서는 두 줄 사이의 merge만 늘어난다
2. **버전 이름 브랜치** (`develop/0.1.0`)
   - 장점: 브랜치를 보면 어느 버전인지 안다
   - 단점: 버전은 한 시점인데 브랜치는 계속 움직이는 선이다. 태그가 할 일을 브랜치가 하게 된다
3. **main 하나 + 종류별 짧은 브랜치** (GitHub Flow 계열)
   - 장점: 규칙이 가장 적고, `main`이 항상 녹색이며, 버전은 태그로 찍는다
   - 단점: 여러 버전을 동시에 유지보수하는 데는 부족하다(지금은 필요 없음)

버전 올리는 단위도 두 가지를 비교했다.
- 단계마다 PATCH: 번호가 자주 바뀌지만 "PATCH = 버그 수정"이라는 SemVer 의미가 깨진다
- Phase마다 MINOR: 의미가 유지되고, 사용자에게 보이는 기능 묶음과 버전이 맞아떨어진다

## 결정

- **3번.** 브랜치 이름은 `<type>/<step>-<slug>`, type은 Conventional Commits type과 맞춘다. 실험용 `exp/`는 merge하지 않는다
- **SemVer, Phase 완료 = MINOR**, PATCH는 릴리스 후 버그 수정만. 1.0.0은 3개 OS 공개 릴리스
- 버전 값은 `CMakeLists.txt` 한 곳, 태그는 `vX.Y.Z` annotated

자세한 규칙: [versioning-and-git.md](../versioning-and-git.md)

## 결과

- 단계 하나 = 브랜치 하나 = `--no-ff` merge 커밋 하나. 진단 범위가 `git diff main...<branch>`로 명확해진다
- 다시 검토할 조건: 기여자가 생기면 PR 필수로 바꾸고, 구 버전 유지보수가 필요하면 `release/x.y` 브랜치를 도입한다
