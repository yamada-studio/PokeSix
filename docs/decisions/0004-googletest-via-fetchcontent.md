# 0004. GoogleTest를 FetchContent + 해시로 고정한다

- 상태: Accepted
- 날짜: 2026-09-28

## 맥락

테스트 대상은 우선 core(Qt 없음)다. 개발자는 ROS 2(`ament_cmake_gtest`)에서 GoogleTest를 써 봤다.
디자인 핸드오프는 QtTest를 가정한다. Windows에는 시스템 패키지 관리자가 없어서, 세 OS에서
같은 버전을 쓰려면 소스로 가져와야 한다.

## 선택지

1. **GoogleTest** — 익숙하다. gmock이 있고 `gtest_discover_tests`로 ctest에 테스트 단위로 등록된다
2. **Catch2 v3** — 표 기반 테스트(`GENERATE(table<…>)`)가 간결하다. 새로 익혀야 한다
3. **QtTest** — Qt와 통합되어 있다. 하지만 core가 Qt에 의존하게 된다(0002와 충돌)

가져오는 방법: 시스템 패키지(OS마다 버전이 다르고 Windows에는 없음), FetchContent `GIT_TAG`(태그가 옮겨질 수 있음),
FetchContent `URL` + `URL_HASH`(내용이 고정됨).

## 결정

**GoogleTest 1.17.0을 릴리스 tarball URL + SHA256으로 FetchContent**한다.
새로 배우는 데 쓸 여유는 Qt에 쓴다. 04 문서의 표는 `TEST_P` / `INSTANTIATE_TEST_SUITE_P`의 데이터로 옮긴다.

## 결과

- 첫 configure에 인터넷이 필요하다. 이후에는 빌드 폴더에 캐시된다
- 업데이트: URL의 버전을 바꾸고 `sha256sum`으로 새 해시를 넣는다
- MSVC CRT 불일치를 막기 위해 `gtest_force_shared_crt=ON`
- data 레이어(Qt 사용) 테스트도 GoogleTest로 쓴다. `QCoreApplication`이 필요하면 테스트 `main`에서 만든다
