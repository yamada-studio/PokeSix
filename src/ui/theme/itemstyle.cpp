#include "ui/theme/itemstyle.h"

#include "ui/logging/logging.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace {
using namespace com::yamada::studio;

constexpr char kPath[] = ":/theme/itemstyle.json";

struct Table
{
    QList<itemstyle::Group> groups;
    QHash<QString, QString> groupOfCategory; // category → group key
    QHash<QString, QString> groupOfPocket;   // pocket → group key
    QString restGroup;                       // 어디에도 안 드는 아이템의 묶음(기타)
    QSet<QString> hidden;
};

QStringList strings(const QJsonValue &value)
{
    QStringList result;
    for (const QJsonValue &item : value.toArray())
        result.append(item.toString());
    return result;
}

// 파일이 없거나 깨졌으면 경고만 남기고 "전체" 하나만 있는 표로 간다 — 분류 창이 비어 보일 뿐 앱은
// 돈다.
Table load()
{
    Table table;
    QFile file(QString::fromLatin1(kPath));
    QJsonParseError error {};
    const QJsonDocument document = file.open(QIODevice::ReadOnly)
                                           ? QJsonDocument::fromJson(file.readAll(), &error)
                                           : QJsonDocument();
    if (!document.isObject()) {
        qCWarning(lcUi) << "cannot read" << kPath << error.errorString();
        table.groups.append(
                {QString::fromLatin1(itemstyle::kAll), QObject::tr("전체"), QColor(Qt::white)});
        return table;
    }
    const QJsonObject root = document.object();
    for (const QJsonValue &value : root.value(QStringLiteral("groups")).toArray()) {
        const QJsonObject g = value.toObject();
        const QString key = g.value(QStringLiteral("key")).toString();
        table.groups.append({key, g.value(QStringLiteral("label")).toString(),
                             QColor::fromString(g.value(QStringLiteral("color")).toString())});
        for (const QString &category : strings(g.value(QStringLiteral("categories"))))
            table.groupOfCategory.insert(category, key);
        for (const QString &pocket : strings(g.value(QStringLiteral("pockets"))))
            table.groupOfPocket.insert(pocket, key);
        if (g.value(QStringLiteral("rest")).toBool())
            table.restGroup = key;
    }
    for (const QString &category : strings(root.value(QStringLiteral("hiddenCategories"))))
        table.hidden.insert(category);
    return table;
}

const Table &table()
{
    static const Table instance = load(); // 처음 부를 때 한 번(C++11부터 스레드 안전)
    return instance;
}
} // namespace

namespace com::yamada::studio::itemstyle {
const QList<Group> &groups()
{
    return table().groups;
}

QString groupOf(const QString &category, const QString &pocket)
{
    const Table &t = table();
    if (t.hidden.contains(category))
        return {};
    if (const auto it = t.groupOfCategory.constFind(category); it != t.groupOfCategory.cend())
        return *it; // 분류가 주머니보다 먼저(예: misc 주머니의 evolution → 진화)
    if (const auto it = t.groupOfPocket.constFind(pocket); it != t.groupOfPocket.cend())
        return *it;
    return t.restGroup;
}

ItemFilterProxy::CategoryFilter filterFor(const QString &groupKey)
{
    if (groupKey == QLatin1String(kAll))
        return [](const QString &category, const QString &pocket) {
            return !groupOf(category, pocket).isEmpty(); // 숨길 분류만 뺀다
        };
    return [groupKey](const QString &category, const QString &pocket) {
        return groupOf(category, pocket) == groupKey;
    };
}
} // namespace com::yamada::studio::itemstyle
