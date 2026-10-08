# core 단위 테스트 — Qt 없이 빌드·실행된다.
add_executable(pokesix_core_tests
    typechart_test.cpp
    squadanalyzer_test.cpp
    movereach_test.cpp
    resourceledger_test.cpp
    # H1 세이브 읽기 — CP마다 한 파일. 합성 세이브 도우미는 syntheticsave.h
    savebytes_test.cpp
    saveblock_test.cpp
    pkmcodec_test.cpp
    partyreader_test.cpp
    realsave_test.cpp
    # H6 — 세대별 형식
    gen5_test.cpp
    saveformat_test.cpp
)
pokesix_target_defaults(pokesix_core_tests)

target_link_libraries(pokesix_core_tests PRIVATE
    pokesix_core
    GTest::gtest_main
)

# 빌드 후 테스트 바이너리를 실행해 TEST() 목록을 읽고 ctest 에 개별 등록한다.
# ctest -R TypeChart 처럼 테스트 단위로 필터링할 수 있다.
gtest_discover_tests(pokesix_core_tests)
