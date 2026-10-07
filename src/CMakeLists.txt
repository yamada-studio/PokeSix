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
)
pokesix_qt_target(PokeSix)

# Resources (styles, 12 MB of fonts, mark and window icons). qt_add_big_resources embeds
# them as a separate object file instead of a huge generated .cpp, so builds stay fast.
qt_add_big_resources(POKESIX_RCC ${POKESIX_RES}/resources.qrc)
target_sources(PokeSix PRIVATE ${POKESIX_RCC})

# ── 실행 파일 아이콘 (OS 셸이 보는 아이콘) ─────────────────────
if(WIN32)
    target_sources(PokeSix PRIVATE ${POKESIX_RES}/platform/windows/pokesix.rc)
elseif(APPLE)
    set(POKESIX_ICNS ${POKESIX_RES}/icons/app/macos/PokeSix.icns)
    target_sources(PokeSix PRIVATE ${POKESIX_ICNS})
    set_source_files_properties(${POKESIX_ICNS} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set_target_properties(PokeSix PROPERTIES MACOSX_BUNDLE_ICON_FILE PokeSix.icns)
endif()

# ── 화면 문구 번역 (Qt Linguist) ──────────────────────────────────
# 소스 언어는 한국어(tr("…")에 적은 글자). 영어 · 일본어 번역은 translations/*.ts에 있다.
#   문구를 더하거나 바꾼 뒤:  cmake --build --preset linux-debug --target update_translations
#   → lupdate가 ui · data · 이 실행 파일의 tr()을 모아 .ts를 갱신한다. 번역을 채우고 빌드하면
#     lrelease가 .qm을 만들어 실행 파일 리소스(:/i18n/pokesix_en.qm …)에 넣는다.
# 게임 데이터 이름(포켓몬 · 아이템 …)은 번역 파일이 아니라 DB의 ko/en/ja 열에서 온다.
qt_add_translations(TARGETS PokeSix
    SOURCE_TARGETS PokeSix pokesix_ui pokesix_data
    TS_FILES
        ${PROJECT_SOURCE_DIR}/translations/pokesix_en.ts
        ${PROJECT_SOURCE_DIR}/translations/pokesix_ja.ts
    NO_GENERATE_PLURALS_TS_FILE
    RESOURCE_PREFIX /i18n
    LUPDATE_OPTIONS -source-language ko_KR -locations none
)

target_compile_definitions(PokeSix PRIVATE
    POKESIX_VERSION="${PROJECT_VERSION}"
)

# TODO: app / ui / data 타깃이 생기면 여기를 `pokesix_app` 하나로 바꾼다.
target_link_libraries(PokeSix PRIVATE
    pokesix_core
    pokesix_ui
)

# Linux: 설치한 실행 파일이 Qt(~/Qt/6.8.3/…/lib)를 찾을 수 있도록, 링크한 라이브러리 경로를
# 설치본의 RPATH에 남긴다. 기본값은 설치할 때 RPATH를 지워서 데스크톱 런처에서 실행하면
# "libQt6Core.so.6: not found"로 뜨지 않는다. Qt를 동봉하는 배포 패키징은 Phase G에서 따로 한다.
set_target_properties(PokeSix PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.yamada.studio.pokesix"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
)

# ── Install ──────────────────────────────────────────────────
install(TARGETS PokeSix
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Windows · macOS: 설치할 때 Qt 런타임을 같이 넣는다(windeployqt / macdeployqt) — 설치 폴더째가
# 포터블 배포본이 된다(docs/deploy.md §2-3). Linux는 Qt deploy가 지원하지 않아 조용히 건너뛴다:
# AppImage는 scripts/linux/package.sh가 linuxdeploy로 만든다.
if(WIN32)
    set(POKESIX_DEPLOY_OPTIONS DEPLOY_TOOL_OPTIONS
        --compiler-runtime          # vc_redist가 없는 PC용 VC++ 런타임
        --no-system-d3d-compiler --no-opengl-sw # QPainter 래스터만 쓴다(약 20MB 절약)
        --exclude-plugins qsqlodbc,qsqlpsql,qsqlmimer # SQLite만 쓴다
    )
else()
    set(POKESIX_DEPLOY_OPTIONS "")
endif()
qt_generate_deploy_app_script(
    TARGET PokeSix
    OUTPUT_SCRIPT pokesix_deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    NO_TRANSLATIONS                 # Qt 자체 번역은 안 쓴다(우리 번역은 :/i18n 안)
    ${POKESIX_DEPLOY_OPTIONS}
)
install(SCRIPT ${pokesix_deploy_script})

# Linux: 데스크톱 환경(런처·독)이 찾는 표준 위치에 .desktop 과 hicolor 아이콘 테마를 설치
if(UNIX AND NOT APPLE)
    install(FILES ${POKESIX_RES}/platform/linux/com.yamada.studio.pokesix.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install(DIRECTORY ${POKESIX_RES}/icons/app/linux/hicolor
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
endif()
