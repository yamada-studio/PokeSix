cmake_minimum_required(VERSION 3.21)

project(PokeSix
    VERSION 0.1.0
    DESCRIPTION "Generation-aware Pokédex and party analyzer"
    HOMEPAGE_URL "https://github.com/yamada-studio/PokeSix"
    LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
    message(FATAL_ERROR "In-source build 금지. `cmake --preset <name>` 을 사용하세요.")
endif()

# ── Options ───────────────────────────────────────────────────
option(POKESIX_BUILD_TESTS "Build unit tests" ${PROJECT_IS_TOP_LEVEL})
option(POKESIX_ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)
option(POKESIX_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(POKESIX_BUILD_TOOLS "Build developer command-line tools (tools/)" ${PROJECT_IS_TOP_LEVEL})

# ── Global language settings ─────────────────────────────────
# 표준 버전 자체는 타깃별로 cxx_std_20 을 건다 (cmake/PokeSixTargetDefaults.cmake).
# 여기서는 GNU 확장(-std=gnu++20)만 끈다.
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(PokeSixTargetDefaults)

# ── Dependencies ─────────────────────────────────────────────
# core 는 Qt 없이 빌드되지만, 앱 전체는 Qt 6.8 LTS 이상을 요구한다.
find_package(Qt6 6.8 REQUIRED COMPONENTS Core Widgets Svg Sql Network LinguistTools)

# ── Targets ──────────────────────────────────────────────────
add_subdirectory(src)

if(POKESIX_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(POKESIX_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
