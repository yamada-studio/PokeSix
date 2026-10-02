#include "ui/dex/guidebook.h"

#include "ui/logging/logging.h"

#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {
using namespace com::yamada::studio;

QJsonObject readJson(const char *path)
{
    QFile file(QString::fromLatin1(path));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcUi) << "cannot open" << path;
        return {};
    }
    QJsonParseError error {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (!document.isObject())
        qCWarning(lcUi) << path << "is not valid JSON:" << error.errorString();
    return document.object();
}

// 글자 하나(한국어) 또는 {ko, en, ja}
LocalizedText localizedOf(const QJsonValue &value)
{
    if (!value.isObject())
        return {value.toString(), {}, {}};
    const QJsonObject o = value.toObject();
    return {o.value(QStringLiteral("ko")).toString(), o.value(QStringLiteral("en")).toString(),
            o.value(QStringLiteral("ja")).toString()};
}

struct Book
{
    QHash<QString, QHash<QString, QStringList>> machinePlaces; // 묶음 → 아이템 → 획득처
    QHash<QString, QHash<QString, LocalizedText>> tutorCosts;  // 묶음 → 기술 → 비용
    QHash<QString, LocalizedText> places;
    QHash<QString, LocalizedText> methods;
};

Book load()
{
    Book book;
    const QJsonObject machines = readJson(":/data/machine-locations.json");
    for (auto group = machines.begin(); group != machines.end(); ++group) {
        if (group.key().startsWith(QLatin1Char('_')) || !group->isObject())
            continue; // "_comment"
        const QJsonObject items = group->toObject();
        for (auto item = items.begin(); item != items.end(); ++item) {
            QStringList places;
            for (const QJsonValue &place :
                 item->toObject().value(QStringLiteral("places")).toArray())
                places.append(place.toString());
            book.machinePlaces[group.key()].insert(item.key(), places);
        }
    }
    const QJsonObject tutors = readJson(":/data/tutor-costs.json");
    for (auto group = tutors.begin(); group != tutors.end(); ++group) {
        if (group.key().startsWith(QLatin1Char('_')) || !group->isObject())
            continue; // "_comment"
        const QJsonObject moves = group->toObject();
        for (auto move = moves.begin(); move != moves.end(); ++move)
            book.tutorCosts[group.key()].insert(move.key(), localizedOf(*move));
    }
    const QJsonObject places
            = readJson(":/data/place-names.json").value(QStringLiteral("places")).toObject();
    for (auto it = places.begin(); it != places.end(); ++it)
        book.places.insert(it.key(), localizedOf(*it));
    const QJsonObject methods
            = readJson(":/data/encounter-methods.json").value(QStringLiteral("methods")).toObject();
    for (auto it = methods.begin(); it != methods.end(); ++it)
        book.methods.insert(it.key(), localizedOf(*it));
    return book;
}

const Book &book()
{
    static const Book instance = load(); // 처음 부를 때 한 번(C++11부터 스레드 안전)
    return instance;
}
} // namespace

namespace com::yamada::studio::guidebook {
QStringList machinePlaces(const QString &versionGroup, const QString &machineItem)
{
    return book().machinePlaces.value(versionGroup).value(machineItem);
}

QString tutorCost(const QString &versionGroup, const QString &move, Language language)
{
    const LocalizedText cost = book().tutorCosts.value(versionGroup).value(move);
    const QString text = cost.text(language);
    return text.isEmpty() ? cost.ko : text; // "48BP"처럼 언어 공통이면 ko 칸 하나만 쓴다
}

QString placeName(const QString &identifier, const LocalizedText &pokeapiName, Language language)
{
    LocalizedText name = book().places.value(identifier);
    // 도로 · 수로: "sinnoh-route-201" · "sinnoh-sea-route-220" · "kanto-route-1"
    static const QRegularExpression route(QStringLiteral("(?:^|-)(sea-)?route-(\\d+)$"));
    const QRegularExpressionMatch match = route.match(identifier);
    if (match.hasMatch()) {
        const QString number = match.captured(2);
        const bool sea = !match.captured(1).isEmpty();
        if (name.ko.isEmpty())
            name.ko = sea ? QStringLiteral("%1번 수로").arg(number)
                          : QStringLiteral("%1번 도로").arg(number);
        if (name.ja.isEmpty())
            name.ja = sea ? QStringLiteral("%1ばんすいどう").arg(number)
                          : QStringLiteral("%1ばんどうろ").arg(number);
    }
    // 사전 · 규칙이 비운 칸은 PokéAPI 이름으로(영어는 거의 늘 있다)
    if (name.ko.isEmpty())
        name.ko = pokeapiName.ko;
    if (name.en.isEmpty())
        name.en = pokeapiName.en;
    if (name.ja.isEmpty())
        name.ja = pokeapiName.ja;
    const QString text = name.text(language);
    return text.isEmpty() ? identifier : text;
}

QString methodName(const QString &identifier, Language language)
{
    const QString text = book().methods.value(identifier).text(language);
    return text.isEmpty() ? QString(identifier).replace(QLatin1Char('-'), QLatin1Char(' ')) : text;
}
} // namespace com::yamada::studio::guidebook
