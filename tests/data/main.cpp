// data 테스트의 main. GoogleTest의 기본 main(gtest_main) 대신 이것을 쓴다.
// QSqlDatabase는 SQLite 드라이버를 플러그인으로 불러오는데, 플러그인 로딩에는 QCoreApplication이
// 있어야 한다. 없으면 "QSqlDatabase requires a QCoreApplication" 경고 뒤 open()에서 죽는다.
#include <QCoreApplication>

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv); // 이벤트 루프(exec)는 돌리지 않는다. 존재만 하면 된다
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
