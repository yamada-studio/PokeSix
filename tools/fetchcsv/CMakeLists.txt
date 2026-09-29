# pokesix-fetch-csv — 앱이 첫 실행에 받을 PokéAPI CSV(고정 커밋)를 명령행에서 받는다.
# 스키마 · CsvImporter를 만들 때 실제 파일을 열어 보는 용도이며, CsvDownloader를 그대로 쓴다.
qt_add_executable(pokesix-fetch-csv
    main.cpp
)
pokesix_qt_target(pokesix-fetch-csv)

target_link_libraries(pokesix-fetch-csv PRIVATE
    pokesix_data
)
