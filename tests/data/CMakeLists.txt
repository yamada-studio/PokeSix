# data 레이어 테스트 — Qt Core가 필요하지만 네트워크는 쓰지 않는다(ctest가 인터넷에 의존하지 않게).
add_executable(pokesix_data_tests
    csvdownloader_test.cpp
    csvreader_test.cpp
)
pokesix_target_defaults(pokesix_data_tests)

target_link_libraries(pokesix_data_tests PRIVATE
    pokesix_data
    GTest::gtest_main
)

gtest_discover_tests(pokesix_data_tests)
