# pokesix_core — 순수 C++20. Qt 에 링크하지 않는다.
# 여기서 <QString> 같은 Qt 헤더를 include 하면 컴파일 에러가 나는 것이 정상이다.
add_library(pokesix_core STATIC
    types/type.h
    types/typechart.h
    types/typechart.cpp
    types/typekey.h
    types/typekey.cpp
    rules/generationfeatures.h
    rules/damageclass.h
    analysis/squadanalyzer.h
    analysis/squadanalyzer.cpp
)
add_library(PokeSix::core ALIAS pokesix_core)

pokesix_target_defaults(pokesix_core)

# include 기준은 src/ → `#include "core/types/typechart.h"`
target_include_directories(pokesix_core PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)
