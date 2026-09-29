# pokesix-import-csv — 받아 둔 PokéAPI CSV 폴더를 게임 데이터 DB로 바꾼다(CsvImporter를 그대로 쓴다).
qt_add_executable(pokesix-import-csv
    main.cpp
)
pokesix_qt_target(pokesix-import-csv)

target_link_libraries(pokesix-import-csv PRIVATE
    pokesix_data
)
