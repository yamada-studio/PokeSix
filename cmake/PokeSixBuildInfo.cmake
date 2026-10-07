# buildinfo.h 생성 — 빌드 식별 문자열(docs/versioning-and-git.md §1 "빌드 식별 문자열").
#
#   cmake -DOUTPUT=<header> -DBASE_VERSION=<x.y.z> -DSOURCE_DIR=<repo> -P PokeSixBuildInfo.cmake
#
# POKESIX_BUILD_VERSION 을 정의한다:
#   릴리스 태그 커밋 · 클린 트리        "0.1.0"
#   태그 뒤 12커밋                      "0.1.0+12.g3f4a5b6"      (SemVer build metadata)
#   커밋 안 한 변경 포함                "0.1.0+12.g3f4a5b6.dirty"
#   git 기록 · 태그 없음(소스 zip 등)   "0.1.0+unknown"
# Debug/Release 구분은 여기가 아니라 application.cpp 가 QT_NO_DEBUG 로 덧붙인다(구성은
# 멀티 컨피그 생성기에서 빌드 시점에만 정해지므로 헤더에 넣을 수 없다).
#
# src/app 의 add_custom_target 이 매 빌드 실행한다. 내용이 바뀔 때만 파일을 다시 쓰므로
# 커밋이 없는 한 재컴파일을 유발하지 않는다.

set(version "${BASE_VERSION}+unknown")
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${SOURCE_DIR}" describe --tags --long --dirty --match "v*"
        RESULT_VARIABLE rc
        OUTPUT_VARIABLE described
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    if(rc EQUAL 0 AND described MATCHES "^v([0-9.]+)-([0-9]+)-g([0-9a-f]+)(-dirty)?$")
        set(ahead "${CMAKE_MATCH_2}")
        set(sha "${CMAKE_MATCH_3}")
        set(dirty "${CMAKE_MATCH_4}")
        # 태그의 버전이 BASE_VERSION 과 달라도 기준은 CMakeLists 의 값이다(릴리스 커밋이
        # 버전을 올리고 아직 태그 전인 잠깐의 상태) — 그때는 suffix 를 붙여 dev 빌드로 표시한다.
        if(CMAKE_MATCH_1 STREQUAL BASE_VERSION AND ahead EQUAL 0 AND dirty STREQUAL "")
            set(version "${BASE_VERSION}")
        else()
            set(version "${BASE_VERSION}+${ahead}.g${sha}")
            if(NOT dirty STREQUAL "")
                string(APPEND version ".dirty")
            endif()
        endif()
    endif()
endif()

set(content "#pragma once

// cmake/PokeSixBuildInfo.cmake 가 매 빌드 생성한다 — 편집 · 커밋 금지.
#define POKESIX_BUILD_VERSION \"${version}\"
")

set(old "")
if(EXISTS "${OUTPUT}")
    file(READ "${OUTPUT}" old)
endif()
if(NOT content STREQUAL old)
    file(WRITE "${OUTPUT}" "${content}")
endif()
