// pokesix-import-csv <CSV 폴더> <DB 파일>
#include "data/update/csvimporter.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTextStream>

using com::yamada::studio::CsvImporter;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTextStream out(stdout);

    const QStringList args = QCoreApplication::arguments();

    if (args.size() != 3) {
        out << "usage: pokesix-import-csv <csv-dir> <db-file>" << Qt::endl;
        return 1;
    }

    const QString csvDir = args.at(1);
    const QString dbPath = args.at(2);

    QElapsedTimer timer;
    timer.start();

    CsvImporter importer;
    if (!importer.run(csvDir, dbPath)) {
        out << "failed: " << importer.errorString() << Qt::endl;
        return 1;
    }

    out << "done: " << dbPath << " in " << timer.elapsed() << " ms" << Qt::endl;

    return 0;
}
