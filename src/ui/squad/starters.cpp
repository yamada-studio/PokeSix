#include "ui/squad/starters.h"

#include "ui/logging/logging.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
// 게임 → (종 번호 → 계통 번호)
using Table = QHash<QString, QHash<int, int>>;

Table load()
{
    Table table;
    QFile file(QStringLiteral(":/data/starters.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(com::yamada::studio::lcUi) << "cannot open starters.json";
        return table;
    }
    const QJsonObject games = QJsonDocument::fromJson(file.readAll())
                                      .object()
                                      .value(QStringLiteral("starters"))
                                      .toObject();
    for (auto game = games.begin(); game != games.end(); ++game) {
        const QJsonArray lines = game.value().toArray();
        QHash<int, int> &species = table[game.key()];
        for (qsizetype line = 0; line < lines.size(); ++line)
            for (const QJsonValue &id : lines.at(line).toArray())
                species.insert(id.toInt(), int(line));
    }
    return table;
}
} // namespace

namespace com::yamada::studio::starters {
int lineOf(const QString &versionGroup, int speciesId)
{
    static const Table table = load(); // 처음 부를 때 한 번
    return table.value(versionGroup).value(speciesId, -1);
}
} // namespace com::yamada::studio::starters
