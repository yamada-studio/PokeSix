cmake_minimum_required(VERSION 3.21)

project(PokeSix
    VERSION 0.2.0
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

# ── Windows 설치본(.msi) — CPack WIX ─────────────────────────
# 무료 WiX Toolset(3.14, GitHub windows 러너에 프리인스톨)으로 install() 트리(windeployqt 포함)를
# 그대로 .msi 로 만든다. scripts\windows\package.bat 이 zip(포터블) 다음에 cpack -G WIX 를 돌린다.
# 상세 · 트레이드오프는 docs/deploy.md §2-4.
if(WIN32 AND PROJECT_IS_TOP_LEVEL)
    install(FILES LICENSE THIRD_PARTY_NOTICES.md DESTINATION .)
    set(CPACK_GENERATOR WIX)
    set(CPACK_PACKAGE_NAME "PokeSix")
    set(CPACK_PACKAGE_VENDOR "Yamada Studio")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "PokeSix") # C:\Program Files\PokeSix
    set(CPACK_PACKAGE_FILE_NAME "PokeSix-${PROJECT_VERSION}-win64")
    set(CPACK_PACKAGE_EXECUTABLES "PokeSix" "PokeSix") # 시작 메뉴 바로가기
    set(CPACK_WIX_PROGRAM_MENU_FOLDER ".") # 폴더 없이 시작 메뉴에 바로 (언인스톨 숏컷 생략)
    # 같은 GUID = 같은 제품: 새 버전 msi 가 설치된 구버전을 교체(업그레이드)한다. 절대 바꾸지 않는다
    set(CPACK_WIX_UPGRADE_GUID "D70F9D30-1E5E-4464-A920-98D36A0FCA34")
    set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/resources/icons/app/windows/pokesix.ico")
    # WiX 의 라이선스 동의 화면은 .rtf 또는 .txt(자동 변환)만 받는다 — 확장자 없는 LICENSE 를 복사
    configure_file(LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt" COPYONLY)
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_BINARY_DIR}/LICENSE.txt")
    include(CPack)
endif()
