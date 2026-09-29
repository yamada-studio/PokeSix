# pokesix_data — Qt 비(非)위젯 계층 전체.
#   update/      게임 데이터 받기와 변환: 고정 커밋의 PokéAPI CSV 다운로드 · 검증(ADR 0011), CSV → SQLite
#   db/          SQLite 스키마 · 연결 (QtSql)
#   repository/  로컬 DB 조회 창구 (네트워크 없음)
#   store/       사용자 데이터 저장 (SquadStore — squads.json)
#   state/       앱 상태 QObject (AppState, SquadSession, Settings)
#   models/      QAbstractItemModel 파생 + 필터 프록시
# Qt::Core / Sql / Network 에만 의존한다. Qt::Widgets · Qt::Gui 금지
# (state/ 와 models/ 를 QML 에서도 그대로 쓸 수 있게 하기 위함).
qt_add_library(pokesix_data STATIC
    logging/logging.h
    logging/logging.cpp
    update/csvsource.h
    update/csvdownloader.h
    update/csvdownloader.cpp
    update/csvreader.h
    update/csvreader.cpp
)
add_library(PokeSix::data ALIAS pokesix_data)
pokesix_qt_target(pokesix_data)

# 헤더가 QObject · QCryptographicHash(QtCore)를 드러내므로 Qt6::Core는 PUBLIC.
# Network · Sql은 .cpp 안에서만 쓴다 → PRIVATE (정적 라이브러리라 링크 단계에는 자동으로 전달된다).
target_link_libraries(pokesix_data
    PUBLIC  pokesix_core Qt6::Core
    PRIVATE Qt6::Sql Qt6::Network
)

target_include_directories(pokesix_data PUBLIC
    "$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/src>"
)
