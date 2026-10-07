// data 테스트의 main. GoogleTest의 기본 main(gtest_main) 대신 이것을 쓴다.
// QSqlDatabase는 SQLite 드라이버를 플러그인으로 불러오는데, 플러그인 로딩에는 QCoreApplication이
// 있어야 한다. 없으면 "QSqlDatabase requires a QCoreApplication" 경고 뒤 open()에서 죽는다.
#include <QCoreApplication>
#include <QSettings>

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv); // 이벤트 루프(exec)는 돌리지 않는다. 존재만 하면 된다
    // AppState는 QSettings()를 쓴다. 앱(main.cpp)과 같이 .ini 형식으로 두고 테스트용 이름을 준다.
    // 조직 이름이 없으면 Windows 레지스트리 백엔드는 쓰기를 조용히 버린다(Linux 파일 백엔드는
    // "Unknown Organization"으로 그냥 써서 거기서만 통과하던 테스트가 Windows에서 깨졌다).
    // 각 테스트는 QSettings::setPath(IniFormat, UserScope, 임시 폴더)로 사용자 설정을 피한다.
    QCoreApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QCoreApplication::setApplicationName(QStringLiteral("PokeSixTests"));
    QSettings::setDefaultFormat(QSettings::IniFormat);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
