# pokesix_app — 앱 부트스트랩 (composition root).
# 각 레이어의 객체를 만들어 서로 연결(wiring)하는 유일한 곳:
#   명령행 인자 해석(--language, --screenshot), 로그 형식, 글꼴 로드·테마 적용,
#   AppState / Repository / DataUpdater 생성, MainWindow 생성과 주입.
# 레이어 중 가장 위에 있으므로 누구도 app 에 의존하지 않는다. 실행 파일(src/main.cpp)은
# Application 하나만 만든다.

qt_add_library(pokesix_app STATIC
    application.h
    application.cpp
)
add_library(PokeSix::app ALIAS pokesix_app)
pokesix_qt_target(pokesix_app)

target_link_libraries(pokesix_app
    PUBLIC  pokesix_ui
    PRIVATE pokesix_data      # AppState · Repository · DataUpdater · gamedatabase::defaultPath
)

target_include_directories(pokesix_app PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

# Application이 setApplicationVersion에 쓴다. 기준 값은 최상위 project(VERSION) 한 곳.
target_compile_definitions(pokesix_app PRIVATE
    POKESIX_VERSION="${PROJECT_VERSION}"
)
