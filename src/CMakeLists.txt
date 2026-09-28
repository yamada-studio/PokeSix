# 레이어 의존 방향 (아래 → 위로만 의존):
#
#   PokeSix (exe) ─▶ app ─▶ ui ─▶ data ─▶ core
#                                  │
#                         Qt::Core/Sql/Network      core 는 Qt 에 링크하지 않는다
#
# 각 레이어 target 은 해당 디렉터리의 CMakeLists.txt 에서 정의한다.
add_subdirectory(core)
add_subdirectory(data)
add_subdirectory(ui)
add_subdirectory(app)

# ── Executable ───────────────────────────────────────────────
# WIN32: Windows 에서 콘솔 창 없이 GUI 서브시스템으로 실행
# MACOSX_BUNDLE: macOS 에서 .app 번들 생성 (Dock·메뉴바가 정상 동작)
set(POKESIX_RES "${PROJECT_SOURCE_DIR}/resources")

qt_add_executable(PokeSix WIN32 MACOSX_BUNDLE
    main.cpp
    ${POKESIX_RES}/resources.qrc
)
pokesix_qt_target(PokeSix)

# ── 실행 파일 아이콘 (OS 셸이 보는 아이콘) ─────────────────────
if(WIN32)
    target_sources(PokeSix PRIVATE ${POKESIX_RES}/platform/windows/pokesix.rc)
elseif(APPLE)
    set(POKESIX_ICNS ${POKESIX_RES}/icons/app/macos/PokeSix.icns)
    target_sources(PokeSix PRIVATE ${POKESIX_ICNS})
    set_source_files_properties(${POKESIX_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(PokeSix PROPERTIES MACOSX_BUNDLE_ICON_FILE PokeSix.icns)
endif()

target_compile_definitions(PokeSix PRIVATE
    POKESIX_VERSION="${PROJECT_VERSION}"
)

# TODO: app / ui / data 타깃이 생기면 여기를 `pokesix_app` 하나로 바꾼다.
target_link_libraries(PokeSix PRIVATE
    pokesix_core
    pokesix_ui
)

set_target_properties(PokeSix PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.yamada.studio.pokesix"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# ── Install (최소) ────────────────────────────────────────────
# Qt 런타임 동봉은 하지 않는다. 패키징이 필요해지면
# qt_generate_deploy_app_script() 를 붙인다 (docs/build.md "Packaging" 참고).
install(TARGETS PokeSix
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Linux: 데스크톱 환경(런처·독)이 찾는 표준 위치에 .desktop 과 hicolor 아이콘 테마를 설치
if(UNIX AND NOT APPLE)
    install(FILES ${POKESIX_RES}/platform/linux/com.yamada.studio.pokesix.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(DIRECTORY ${POKESIX_RES}/icons/app/linux/hicolor
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
endif()
