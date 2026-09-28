# 우리 프로젝트의 모든 타깃에 공통으로 거는 설정.
# 전역 변수(CMAKE_CXX_FLAGS 등)를 건드리지 않으므로 FetchContent 로 가져온
# 서드파티(GoogleTest)에는 우리 경고 수준이 전파되지 않는다.

if(POKESIX_ENABLE_CLANG_TIDY)
    find_program(POKESIX_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
endif()

# pokesix_target_defaults(<target>)
#   - C++20 요구
#   - 컴파일러 경고
#   - (옵션) clang-tidy
function(pokesix_target_defaults target)
    # PUBLIC: 이 타깃을 링크하는 쪽도 C++20 이상이어야 헤더를 읽을 수 있다
    target_compile_features(${target} PUBLIC cxx_std_20)

    if(MSVC)
        target_compile_options(${target} PRIVATE
            /W4
            /permissive-        # 표준 준수 모드
            /Zc:__cplusplus     # __cplusplus 매크로를 실제 값으로 (기본은 199711L)
            /utf-8              # 소스·실행 문자셋 UTF-8 (한국어 Windows 기본 CP949 문제 방지)
        )
        if(POKESIX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
        )
        if(POKESIX_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()

    if(POKESIX_ENABLE_CLANG_TIDY)
        set_target_properties(${target} PROPERTIES CXX_CLANG_TIDY "${POKESIX_CLANG_TIDY_EXE}")
    endif()
endfunction()

# pokesix_qt_target(<target>)
#   Qt 코드 생성기(moc / uic / rcc)를 켠다. Qt 를 쓰는 타깃에만 호출한다.
#   core 에는 호출하지 않는다 — core 는 Qt 를 모른다.
function(pokesix_qt_target target)
    pokesix_target_defaults(${target})
    set_target_properties(${target} PROPERTIES
        AUTOMOC ON
        AUTOUIC ON
        AUTORCC ON
    )
endfunction()
