# 0017 — 빌드 식별 문자열: git describe를 빌드마다 헤더로

- 상태: 채택 (2026-10-08)
- 관련: [versioning-and-git.md §1](../versioning-and-git.md), [ADR 0006](0006-semver-and-trunk-based-branches.md)

## 배경

`project(VERSION)`은 마지막 릴리스 번호 하나뿐이라, 창 제목의 "PokeSix 0.1.0"만으로는
태그 빌드인지 그 뒤 수십 커밋이 쌓인 개발 빌드인지, Debug인지 Release인지 구분할 수 없었다.
여러 머신(집 Windows · 회사 Linux)과 패키지(msi · zip · AppImage · dmg)가 생기면서
"지금 뜬 창이 정확히 뭔지"가 실제로 문제가 됐다(앱 메뉴에 구버전이 남아 실행된 사건).

## 결정

1. **문자열 형식** — `<기준>[+<태그 뒤 커밋 수>.g<sha>[.dirty]][ Debug]`.
   `+` 뒤는 SemVer build metadata 문법이라 버전 비교 규칙을 깨지 않는다.
   정식 릴리스 빌드(태그 커밋 · 클린 트리 · Release)만 깨끗한 `0.1.0`이 된다.
2. **생성 시점은 configure가 아니라 빌드** — `add_custom_target`이 매 빌드
   `cmake/PokeSixBuildInfo.cmake`를 돌려 `buildinfo.h`를 쓴다. configure 시점에 박으면
   커밋이 쌓여도 reconfigure 전까지 낡은 값이 남는다. 스크립트는 내용이 같으면 파일을
   안 건드려서 재컴파일을 유발하지 않는다(실측: 반복 빌드에서 컴파일 0개).
3. **Debug 표시는 C++에서** — 구성(Config)은 멀티 컨피그 생성기(VS)에서 빌드 시점에만
   정해지므로 헤더에 넣을 수 없다. `application.cpp`가 `QT_NO_DEBUG`로 덧붙인다.
4. **노출 위치** — `applicationVersion()` 하나만 설정하면 창 제목(`PokeSix %1`)과
   `--version`(QCommandLineParser `addVersionOption`)이 따라온다.

## 결과

- CI checkout은 `fetch-depth: 0`이어야 한다(얕은 clone엔 태그가 없어 `+unknown`이 박힌다)
- 소스 zip처럼 git이 없는 트리는 `0.1.0+unknown` — 의도된 동작(출처 불명 표시)
- `buildinfo.h`는 빌드 산출물이다. 커밋하지 않는다(`build/` 안에만 생긴다)
