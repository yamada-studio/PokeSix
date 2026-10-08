# pokesix-read-sav — 4세대 세이브(.sav)의 파티를 단계별 로그와 함께 찍는다(H1 디버깅 도구).
#   build/linux-debug/tools/readsav/pokesix-read-sav <세이브> [--verbose] [--hex]
# core(세이브 파서)는 Qt를 모르고, 로그 · 파일 읽기 · 인자 해석만 이 도구가 Qt로 한다.
qt_add_executable(pokesix-read-sav
    main.cpp
)
pokesix_qt_target(pokesix-read-sav)

target_link_libraries(pokesix-read-sav PRIVATE
    pokesix_core
    Qt6::Core
)
