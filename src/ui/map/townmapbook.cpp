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
        const auto rectOf = [](const QJsonArray &r) {
            return r.size() == 4 ? QRect(r[0].toInt(), r[1].toInt(), r[2].toInt(), r[3].toInt())
                                 : QRect();
        };
        QPoint translate; // 옛 형식: 노드가 원본 좌표 → 크롭만큼 캔버스로 당긴다
        if (root.contains(QStringLiteral("layers"))) { // 새 형식(합본): 레이어 + 캔버스 좌표
            const QJsonArray canvas = root.value(QStringLiteral("canvas")).toArray();
            if (canvas.size() == 2)
                map.canvas = QSize(canvas[0].toInt(), canvas[1].toInt());
            for (const QJsonValue &value : root.value(QStringLiteral("layers")).toArray()) {
                const QJsonObject l = value.toObject();
                townmapbook::Layer layer;
                layer.source = l.value(QStringLiteral("source")).toString();
                layer.url = l.value(QStringLiteral("url")).toString();
                const QJsonArray size = l.value(QStringLiteral("size")).toArray();
                if (size.size() == 2)
                    layer.size = QSize(size[0].toInt(), size[1].toInt());
                layer.crop = rectOf(l.value(QStringLiteral("crop")).toArray());
                const QJsonArray offset = l.value(QStringLiteral("offset")).toArray();
                if (offset.size() == 2)
                    layer.offset = QPoint(offset[0].toInt(), offset[1].toInt());
                map.layers.append(layer);
            }
        } else { // 옛 형식: 그림 한 장 + 원본 좌표 노드
            const QJsonObject image = root.value(QStringLiteral("image")).toObject();
            townmapbook::Layer layer;
            layer.source = map.region;
            layer.url = image.value(QStringLiteral("url")).toString();
            layer.size = QSize(image.value(QStringLiteral("width")).toInt(),
                               image.value(QStringLiteral("height")).toInt());
            layer.crop = rectOf(image.value(QStringLiteral("crop")).toArray());
            if (layer.crop.isNull())
                layer.crop = QRect(QPoint(0, 0), layer.size);
            map.canvas = layer.crop.size();
            translate = -layer.crop.topLeft();
            map.layers.append(layer);
        }
        for (const QJsonValue &value : root.value(QStringLiteral("nodes")).toArray()) {
            const QJsonObject n = value.toObject();
            const QRect rect = rectOf(n.value(QStringLiteral("rect")).toArray());
            if (rect.isNull())
                continue;
            townmapbook::Node node;
            node.location = n.value(QStringLiteral("location")).toString();
            node.region = n.value(QStringLiteral("region")).toString(map.region);
            node.rect = rect.translated(translate);
            if (QRect(QPoint(0, 0), map.canvas).intersects(node.rect)) // 캔버스 밖은 버린다
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
