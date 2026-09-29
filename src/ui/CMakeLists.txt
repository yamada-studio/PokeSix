qt_add_library(pokesix_ui STATIC
    logging/logging.h
    logging/logging.cpp
    theme/tokens.h
    theme/theme.h
    theme/theme.cpp
    widgets/panelpainter.h
    widgets/panelpainter.cpp
    widgets/layoutguide.h
    widgets/layoutguide.cpp
    widgets/markwidget.h
    widgets/markwidget.cpp
    widgets/wordmarklabel.h
    widgets/wordmarklabel.cpp
    widgets/generationbutton.h
    widgets/generationbutton.cpp
    widgets/panelframe.h
    widgets/panelframe.cpp
    home/homepage.h
    home/homepage.cpp
    home/intromenu.h
    home/intromenu.cpp
    home/intromenuitem.h
    home/intromenuitem.cpp
    home/introfooter.h
    home/introfooter.cpp
    shell/page.h
    shell/appbar.h
    shell/appbar.cpp
    shell/mainwindow.h
    shell/mainwindow.cpp
)

add_library(PokeSix::ui ALIAS pokesix_ui)
pokesix_qt_target(pokesix_ui)

target_link_libraries(pokesix_ui
    PUBLIC  Qt6::Widgets
    PRIVATE Qt6::Svg          # MarkWidget이 캡슐 마크 SVG를 그린다(QSvgRenderer)
    # PRIVATE pokesix_data     # data 를 아직 안 만들었으면 이 줄은 나중에
)

target_include_directories(pokesix_ui PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)