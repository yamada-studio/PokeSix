# data 레이어 테스트 — Qt Core가 필요하지만 네트워크는 쓰지 않는다(ctest가 인터넷에 의존하지 않게).
add_executable(pokesix_data_tests
    main.cpp            # QCoreApplication을 만드는 main (gtest_main 대신)
    csvdownloader_test.cpp
    csvreader_test.cpp
    genranges_test.cpp
    csvimporter_test.cpp
    repository_test.cpp
)
pokesix_target_defaults(pokesix_data_tests)

target_link_libraries(pokesix_data_tests PRIVATE
    pokesix_data
    Qt6::Sql            # 테스트가 만든 DB에 직접 질의한다
    GTest::gtest
)

# 시드 CSV 폴더(tests/fixtures). 실제 PokéAPI 파일에서 필요한 줄만 잘라 둔 것이다.
target_compile_definitions(pokesix_data_tests PRIVATE
    POKESIX_FIXTURES_DIR="${PROJECT_SOURCE_DIR}/tests/fixtures"
)

# Windows: 빌드 직후 실행 파일을 돌려 테스트 목록을 읽으면 PATH에 Qt DLL이 없어 0xc0000135로 실패한다.
# PRE_TEST 는 ctest 가 실행될 때 목록을 읽으므로, 테스트 프리셋이 PATH 에 넣은 Qt bin 폴더를 쓴다.
gtest_discover_tests(pokesix_data_tests DISCOVERY_MODE PRE_TEST)
