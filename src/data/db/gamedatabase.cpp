#include "data/db/gamedatabase.h"

#include "data/db/schema.h"
#include "data/logging/logging.h"
#include "data/update/csvsource.h"

#include <QFile>
#include <QHash>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QVariant>

namespace {
// meta 표(key → value)를 읽는다. 파일이 없거나 열리지 않거나 표가 없으면 빈 해시.
// 연결은 이 함수 안에서만 쓰고 바로 지운다(연결 이름은 전역이라 겹치지 않게).
QHash<QString, QString> readMeta(const QString &path)
{
    QHash<QString, QString> meta;
    if (!QFile::exists(path))
        return meta;
    const QString connection = QStringLiteral("pokesix.check");
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connection);
        db.setDatabaseName(path);
        db.setConnectOptions(QStringLiteral("QSQLITE_OPEN_READONLY"));
        if (db.open()) {
            QSqlQuery query(db);
            if (query.exec(QStringLiteral("SELECT key, value FROM meta")))
                while (query.next())
                    meta.insert(query.value(0).toString(), query.value(1).toString());
        }
    }
    QSqlDatabase::removeDatabase(connection);
    return meta;
}
} // namespace

namespace com::yamada::studio::gamedatabase {
QString defaultPath()
{
    // AppDataLocation 폴더 + "/pokesix.sqlite"
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/pokesix.sqlite");
}

bool isUsable(const QString &path)
{
    // meta 표의 schema_version이 schema::kVersion과 같으면 쓸 수 있다
    return readMeta(path).value(QStringLiteral("schema_version")).toInt() == schema::kVersion;
}

QString pendingPath(const QString &dbPath)
{
    return dbPath + QStringLiteral(".importing");
}

bool matchesThisBuild(const QString &path)
{
    const QHash<QString, QString> meta = readMeta(path);
    return meta.value(QStringLiteral("schema_version")).toInt() == schema::kVersion
           && meta.value(QStringLiteral("source_commit")) == QLatin1StringView(csvsource::kCommit);
}

bool replaceWith(const QString &dbPath, const QString &newFile)
{
    // QFile::rename은 대상이 있으면 실패하므로 먼저 지운다. 둘 중 하나라도 막히면(Windows: 다른
    // 프로세스가 dbPath를 열어 둠) newFile은 그대로 둔다 — 다음 기회에 adoptPendingImport가 넣는다.
    if (QFile::exists(dbPath) && !QFile::remove(dbPath)) {
        qCWarning(lcData) << "cannot remove" << dbPath << "(opened by another process?)";
        return false;
    }
    if (!QFile::rename(newFile, dbPath)) {
        qCWarning(lcData) << "cannot move" << newFile << "to" << dbPath;
        return false;
    }
    return true;
}

bool adoptPendingImport(const QString &dbPath)
{
    const QString pending = pendingPath(dbPath);
    if (!QFile::exists(pending))
        return false;
    if (!matchesThisBuild(pending)) {
        // 반쪽짜리(변환 도중 죽음)거나 옛 빌드가 남긴 것 → 버린다
        qCInfo(lcData) << "discarding a stale pending import" << pending;
        QFile::remove(pending);
        return false;
    }
    if (!replaceWith(dbPath, pending))
        return false;
    qCInfo(lcData) << "adopted the pending import" << pending << "->" << dbPath;
    return true;
}
} // namespace com::yamada::studio::gamedatabase
