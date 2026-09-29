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
    // AppDataLocation 폴더 + "/pokesix.sqlite"
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/pokesix.sqlite");
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
            // meta 표에서 schema_version을 읽어 schema::kVersion과 같으면 usable = true
            query.exec(QStringLiteral("SELECT value FROM meta WHERE key = 'schema_version'"));
            if (query.next()) {
                if (query.value(0).toInt() == schema::kVersion) {
                    usable = true;
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return usable;
}
} // namespace com::yamada::studio::gamedatabase
