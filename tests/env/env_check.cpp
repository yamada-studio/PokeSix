// Qt 런타임 플러그인 점검. 실패 시 원인과 docs/build.md 섹션을 출력한다.
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSslSocket>
#include <QTextStream>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);
    int failures = 0;

    out << "Qt " << qVersion() << "\n";

    out << "SQL drivers: " << QSqlDatabase::drivers().join(u", ") << "\n";
    if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
        out << "FAIL: QSQLITE driver not found (see docs/build.md#sqldrivers)\n";
        ++failures;
    }

    out << "TLS backends: " << QSslSocket::availableBackends().join(u", ") << "\n";
    if (!QSslSocket::supportsSsl()) {
        out << "FAIL: no TLS backend, HTTPS to PokéAPI will fail (see docs/build.md#tls)\n";
        ++failures;
    } else {
        out << "TLS library: " << QSslSocket::sslLibraryVersionString() << "\n";
    }

    out << (failures == 0 ? "OK\n" : "FAILED\n");
    out.flush();
    return failures == 0 ? 0 : 1;
}
