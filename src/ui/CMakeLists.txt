# pokesix_ui — Qt Widgets 계층.
#   shell/     MainWindow, AppBar, AppTabBar, ResponsiveController (화면 틀)
#   theme/     Tokens, TypeColors, Theme::apply, FontLoader
#   widgets/   재사용 컴포넌트 (PanelFrame, TypeChip, StatBar, HeatmapWidget ...)
#   home/ dex/ items/ squad/ settings/   화면(page)별 위젯
# data 의 state/model 을 위젯에 연결만 하고, 비즈니스 로직은 두지 않는다.
#
# 첫 소스 파일을 추가할 때 아래 주석을 풀고 파일 목록을 채운다.
#
qt_add_library(pokesix_ui STATIC
    logging/logging.h
    logging/logging.cpp
    shell/mainwindow.h
    shell/mainwindow.cpp
)

add_library(PokeSix::ui ALIAS pokesix_ui)
pokesix_qt_target(pokesix_ui)

target_link_libraries(pokesix_ui
    PUBLIC  Qt6::Widgets
    # PRIVATE pokesix_data     # data 를 아직 안 만들었으면 이 줄은 나중에
)

target_include_directories(pokesix_ui PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)