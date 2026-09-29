include(FetchContent)

# 다운로드 파일의 타임스탬프를 추출 시각으로 (CMake 3.24+ 경고 방지)
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# GoogleTest — 릴리스 tarball 을 SHA256 으로 고정한다.
# 태그(GIT_TAG)는 옮겨질 수 있지만 해시는 바뀌지 않으므로 재현성이 더 확실하다.
# 업데이트: URL 버전을 바꾸고 `sha256sum` 으로 새 해시를 넣는다.
FetchContent_Declare(googletest
    URL      https://github.com/google/googletest/releases/download/v1.17.0/googletest-1.17.0.tar.gz
    URL_HASH SHA256=65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c
)
# MSVC: gtest 가 /MT 로 빌드되어 우리 /MD 와 런타임이 섞이는 문제 방지
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)

add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(env)
