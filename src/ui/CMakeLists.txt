qt_add_library(pokesix_ui STATIC
    logging/logging.h
    logging/logging.cpp
    widgets/layoutguide.h
    widgets/layoutguide.cpp
    home/homepage.h
    home/homepage.cpp
    shell/appbar.h
    shell/appbar.cpp
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