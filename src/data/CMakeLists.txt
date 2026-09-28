# pokesix_data — Qt 비(非)위젯 계층 전체.
#   api/         PokéAPI 클라이언트 (QNetworkAccessManager)
#   db/          SQLite 스키마·연결 (QtSql)
#   repository/  "캐시에 있으면 DB, 없으면 API" 를 결정하는 조회 창구
#   store/       사용자 데이터 저장 (SquadStore — squads.json)
#   state/       앱 상태 QObject (AppState, SquadSession, Settings)
#   models/      QAbstractItemModel 파생 + 필터 프록시
# Qt::Core / Sql / Network 에만 의존한다. Qt::Widgets·Qt::Gui 금지
# (state/ 와 models/ 를 QML 에서도 그대로 쓸 수 있게 하기 위함).
#
# 첫 소스 파일을 추가할 때 아래 주석을 풀고 파일 목록을 채운다.
# (소스가 하나도 없는 STATIC 라이브러리는 CMake 가 거부한다)
#
# qt_add_library(pokesix_data STATIC
#     state/appstate.h
#     state/appstate.cpp
# )
# add_library(PokeSix::data ALIAS pokesix_data)
# pokesix_qt_target(pokesix_data)
#
# target_link_libraries(pokesix_data
#     PUBLIC  pokesix_core Qt6::Core
#     PRIVATE Qt6::Sql Qt6::Network
# )
