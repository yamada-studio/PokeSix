# pokesix_app — 앱 부트스트랩 (composition root).
# 각 레이어의 객체를 만들어 서로 연결(wiring)하는 유일한 곳:
#   명령행 인자 해석(--gallery, --screenshot), 로깅 초기화, 글꼴 로드·테마 적용,
#   AppState / Repository 생성, MainWindow 생성과 주입.
# 레이어 중 가장 위에 있으므로 누구도 app 에 의존하지 않는다.
#
# 첫 소스 파일을 추가할 때 아래 주석을 풀고, src/CMakeLists.txt 의
# PokeSix 실행 파일이 pokesix_app 을 링크하도록 바꾼다.
#
# qt_add_library(pokesix_app STATIC
#     application.h
#     application.cpp
# )
# add_library(PokeSix::app ALIAS pokesix_app)
# pokesix_qt_target(pokesix_app)
#
# target_link_libraries(pokesix_app
#     PUBLIC pokesix_ui
# )
