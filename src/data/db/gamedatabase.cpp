#include "data/db/gamedatabase.h"

#include "data/db/schema.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace com::yamada::studio::gamedatabase {
QString defaultPath()
{
    // TODO A AppDataLocation 폴더 + "/pokesix.sqlite"
    //        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
    return QString(); // ← 이 줄을 TODO A의 코드로 바꾼다
}

bool isUsable(const QString &path)
{
    if (!QFile::exists(path))
        return false;

    const QString connection = QStringLiteral("pokesix.check");
    bool usable = false;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(path);
        if (db.open()) {
            QSqlQuery query(db);
            // TODO B meta 표에서 schema_version을 읽어 schema::kVersion과 같으면 usable = true
            //        query.exec("SELECT value FROM meta WHERE key = 'schema_version'")
            //        query.next()가 true면 query.value(0).toInt()가 버전
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return usable;
}
} // namespace com::yamada::studio::gamedatabase
