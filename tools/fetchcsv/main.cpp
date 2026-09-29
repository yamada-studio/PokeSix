// pokesix-fetch-csv [폴더]
//   앱과 같은 CsvDownloader로 PokéAPI CSV를 받는다. 폴더를 생략하면 앱이 쓰는 기본 위치
//   (Linux: ~/.cache/YamadaStudio/PokeSix/pokeapi-csv/<커밋>/)에 받는다. 이미 받은 파일은 건너뛴다.
#include "data/update/csvdownloader.h"

#include <QCoreApplication>
#include <QTextStream>

using com::yamada::studio::CsvDownloader;

int main(int argc, char *argv[])
{
    // 창이 없는 도구라 QApplication이 아닌 QCoreApplication. 이벤트 루프(exec)는 똑같이 필요하다 —
    // 네트워크 응답은 이벤트 루프가 돌아야 시그널로 들어온다.
    QCoreApplication app(argc, argv);
    // 앱(PokeSix)과 같은 이름을 써야 기본 폴더(QStandardPaths)가 앱과 같아진다.
    QCoreApplication::setOrganizationName(QStringLiteral("YamadaStudio"));
    QCoreApplication::setApplicationName(QStringLiteral("PokeSix"));

    const QStringList args = QCoreApplication::arguments();
    const QString directory = args.size() > 1 ? args.at(1) : CsvDownloader::defaultDirectory();

    QTextStream out(stdout);
    CsvDownloader downloader(directory);

    QObject::connect(
            &downloader, &CsvDownloader::progress, &app,
            [&out](int done, int total, qint64 bytes, qint64 totalBytes, const QString &file) {
                out << QStringLiteral("\r[%1/%2] %3 MB / %4 MB  %5")
                                .arg(done)
                                .arg(total)
                                .arg(bytes / 1e6, 0, 'f', 1)
                                .arg(totalBytes / 1e6, 0, 'f', 1)
                                .arg(file, -24)
                    << Qt::flush;
            });
    QObject::connect(&downloader, &CsvDownloader::finished, &app, [&] {
        out << "\ndone: " << directory << Qt::endl;
        app.exit(0);
    });
    QObject::connect(&downloader, &CsvDownloader::failed, &app,
                     [&](CsvDownloader::Error, const QString &detail) {
                         out << "\nfailed: " << detail << Qt::endl;
                         app.exit(1);
                     });

    downloader.start(); // 곧바로 돌아온다. 실제 일은 아래 exec()가 도는 동안 일어난다
    return app.exec();
}
