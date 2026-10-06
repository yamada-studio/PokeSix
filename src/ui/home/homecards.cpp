#include "ui/home/homecards.h"

#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {
using namespace com::yamada::studio;

constexpr char kPath[] = ":/theme/homecards.json";

QList<homecards::Card> load()
{
    QList<homecards::Card> cards;
    QFile file(QString::fromLatin1(kPath));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcUi) << "cannot open" << kPath;
        return cards;
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (!document.isObject()) {
        qCWarning(lcUi) << kPath << "is not valid JSON:" << error.errorString();
        return cards;
    }
    const QJsonArray array = document.object().value(QStringLiteral("cards")).toArray();
    for (const QJsonValue &value : array) {
        const QJsonObject c = value.toObject();
        homecards::Card card;
        card.generation = c.value(QStringLiteral("generation")).toInt();
        for (const QJsonValue &id : c.value(QStringLiteral("pokemon")).toArray())
            card.pokemon.append(id.toInt());
        const QJsonArray range = c.value(QStringLiteral("range")).toArray();
        card.rangeFrom = range.size() > 0 ? range.at(0).toInt() : 0;
        card.rangeTo = range.size() > 1 ? range.at(1).toInt() : 0;
        card.accent = QColor::fromString(c.value(QStringLiteral("accent")).toString());
        if (!card.accent.isValid())
            card.accent = QColor(tok::kWhite);
        const QJsonObject region = c.value(QStringLiteral("region")).toObject();
        card.region = {region.value(QStringLiteral("ko")).toString(),
                       region.value(QStringLiteral("en")).toString(),
                       region.value(QStringLiteral("ja")).toString()};
        if (card.generation > 0)
            cards.append(card);
    }
    return cards;
}
} // namespace

namespace com::yamada::studio::homecards {
const QList<Card> &all()
{
    static const QList<Card> instance = load();
    return instance;
}
} // namespace com::yamada::studio::homecards
