# Qt 런타임 환경 점검 — 코드 로직이 아니라 "이 머신에서 Qt 플러그인이 로드되는가"를 본다.
# docs/build.md 의 흔한 에러(sqldrivers 누락, TLS 백엔드 없음)를 ctest 한 번으로 확인한다.
add_executable(pokesix_env_check
    env_check.cpp
)
pokesix_target_defaults(pokesix_env_check)

target_link_libraries(pokesix_env_check PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Network
)

add_test(NAME env.qt_runtime COMMAND pokesix_env_check)
