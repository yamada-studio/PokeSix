#include "ui/map/townmapbook.h"

#include "ui/logging/logging.h"

#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
using namespace com::yamada::studio;

QHash<QString, townmapbook::RegionMap> load()
{
    QHash<QString, townmapbook::RegionMap> maps;
    QDirIterator files(QStringLiteral(":/data/townmap"), {QStringLiteral("*.json")});
    while (files.hasNext()) {
        const QString path = files.next();
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(lcUi) << "cannot open" << path;
            continue;
        }
        QJsonParseError error {};
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
        if (!document.isObject()) {
            qCWarning(lcUi) << path << "is not valid JSON:" << error.errorString();
            continue;
        }
        const QJsonObject root = document.object();
        townmapbook::RegionMap map;
        map.region
                = root.value(QStringLiteral("region")).toString(QFileInfo(path).completeBaseName());
        const QJsonObject image = root.value(QStringLiteral("image")).toObject();
        map.imageUrl = image.value(QStringLiteral("url")).toString();
        map.imageSize = QSize(image.value(QStringLiteral("width")).toInt(),
                              image.value(QStringLiteral("height")).toInt());
        const QJsonArray crop = image.value(QStringLiteral("crop")).toArray();
        if (crop.size() == 4)
            map.crop = QRect(crop[0].toInt(), crop[1].toInt(), crop[2].toInt(), crop[3].toInt());
        for (const QJsonValue &value : root.value(QStringLiteral("nodes")).toArray()) {
            const QJsonObject n = value.toObject();
            const QJsonArray r = n.value(QStringLiteral("rect")).toArray();
            if (r.size() != 4)
                continue;
            townmapbook::Node node;
            node.location = n.value(QStringLiteral("location")).toString();
            node.region = n.value(QStringLiteral("region")).toString(map.region);
            node.rect = QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt());
            if (map.view().intersects(node.rect)) // 크롭 밖 노드는 버린다
                map.nodes.append(node);
        }
        maps.insert(map.region, map);
    }
    return maps;
}

const QHash<QString, townmapbook::RegionMap> &book()
{
    static const QHash<QString, townmapbook::RegionMap> instance = load();
    return instance;
}
} // namespace

namespace com::yamada::studio::townmapbook {
const RegionMap &regionMap(const QString &region)
{
    static const RegionMap empty;
    const auto it = book().constFind(region);
    return it == book().constEnd() ? empty : *it;
}

QStringList regions()
{
    QStringList keys = book().keys();
    keys.sort();
    return keys;
}
} // namespace com::yamada::studio::townmapbook
