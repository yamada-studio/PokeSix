# core 단위 테스트 — Qt 없이 빌드·실행된다.
add_executable(pokesix_core_tests
    typechart_test.cpp
)
pokesix_target_defaults(pokesix_core_tests)

target_link_libraries(pokesix_core_tests PRIVATE
    pokesix_core
    GTest::gtest_main
)

# 빌드 후 테스트 바이너리를 실행해 TEST() 목록을 읽고 ctest 에 개별 등록한다.
# ctest -R TypeChart 처럼 테스트 단위로 필터링할 수 있다.
gtest_discover_tests(pokesix_core_tests)
