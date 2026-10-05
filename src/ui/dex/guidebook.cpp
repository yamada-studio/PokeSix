#include "ui/dex/guidebook.h"

#include "ui/logging/logging.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
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

// 입수 방법 한 가지 (acquisition/<게임>.json의 배열 한 칸)
struct Source
{
    QString how; // field · hidden · gift · shop · exchange · prize · reward · held · trade
                 // · tutor · other (비어도 된다)
    QString where; // PokéAPI 장소 identifier(장소 이름 사전 · 도로 규칙으로 한국어가 된다)
    LocalizedText place;   // identifier가 없을 때의 장소 글자
    LocalizedText content; // 컨텐츠(배틀프런티어 · 포케슬론 · 게임코너 …)
    QList<std::pair<int, QString>> cost; // (양, 단위): (48, "bp") · (10000, "money")
    LocalizedText detail;                // 조건 · 메모(서핑 필요 · 엔딩 후 …)
};

struct GameBook
{
    QHash<QString, QList<Source>> items;  // 아이템 identifier →
    QHash<QString, QList<Source>> tutors; // 기술 identifier →
};

struct Book
{
    QHash<QString, GameBook> games; // 버전 그룹 →
    QHash<QString, LocalizedText> places;
    QHash<QString, LocalizedText> methods;
};

QList<Source> sourcesOf(const QJsonValue &value)
{
    QList<Source> sources;
    for (const QJsonValue &entry : value.toArray()) {
        const QJsonObject o = entry.toObject();
        Source source;
        source.how = o.value(QStringLiteral("how")).toString();
        source.where = o.value(QStringLiteral("where")).toString();
        source.place = localizedOf(o.value(QStringLiteral("place")));
        source.content = localizedOf(o.value(QStringLiteral("content")));
        source.detail = localizedOf(o.value(QStringLiteral("detail")));
        for (const QJsonValue &cost : o.value(QStringLiteral("cost")).toArray()) {
            const QJsonObject c = cost.toObject();
            source.cost.append({c.value(QStringLiteral("amount")).toInt(),
                                c.value(QStringLiteral("unit")).toString()});
        }
        sources.append(source);
    }
    return sources;
}

Book load()
{
    Book book;
    // 게임별 입수 사전: 폴더에 있는 파일 전부(파일 이름 = 버전 그룹)
    QDirIterator files(QStringLiteral(":/data/acquisition"), {QStringLiteral("*.json")});
    while (files.hasNext()) {
        const QString path = files.next();
        const QJsonObject root = readJson(path.toUtf8().constData());
        const QString group = root.value(QStringLiteral("versionGroup"))
                                      .toString(QFileInfo(path).completeBaseName());
        GameBook &game = book.games[group];
        const QJsonObject items = root.value(QStringLiteral("items")).toObject();
        for (auto it = items.begin(); it != items.end(); ++it)
            game.items.insert(it.key(), sourcesOf(*it));
        const QJsonObject tutors = root.value(QStringLiteral("tutors")).toObject();
        for (auto it = tutors.begin(); it != tutors.end(); ++it)
            game.tutors.insert(it.key(), sourcesOf(*it));
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

// 화면 글자 (lupdate가 이 문맥으로 모은다)
QString tr(const char *text)
{
    return QCoreApplication::translate("com::yamada::studio::guidebook", text);
}

// 입수 방법 이름. 모르는 값은 비운다(줄에 방법 없이 장소만 나온다)
QString howLabel(const QString &how)
{
    static const QHash<QString, const char *> labels = {
            {QStringLiteral("field"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "필드")},
            {QStringLiteral("hidden"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "숨겨진 아이템")},
            {QStringLiteral("gift"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "받기")},
            {QStringLiteral("shop"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "상점")},
            {QStringLiteral("exchange"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "교환")},
            {QStringLiteral("prize"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "경품")},
            {QStringLiteral("reward"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "보상")},
            {QStringLiteral("held"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "야생 포켓몬 소지")},
            {QStringLiteral("trade"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "교환(통신)")},
            {QStringLiteral("tutor"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "NPC 가르침")},
    };
    const auto it = labels.constFind(how);
    return it == labels.constEnd() ? QString() : tr(*it);
}

// 비용 한 덩이: (48, "bp") → "48BP", (10000, "money") → "10,000원". 모르는 단위는 "8 red-shard"처럼
QString costText(int amount, const QString &unit)
{
    const QString number = QLocale(QLocale::Korean).toString(amount);
    static const QHash<QString, const char *> units = {
            {QStringLiteral("money"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "%1원")},
            {QStringLiteral("bp"), QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "%1BP")},
            {QStringLiteral("coins"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "코인 %1")},
            {QStringLiteral("red-shard"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "빨강조각 %1개")},
            {QStringLiteral("blue-shard"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "파랑조각 %1개")},
            {QStringLiteral("yellow-shard"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "노랑조각 %1개")},
            {QStringLiteral("green-shard"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "초록조각 %1개")},
            {QStringLiteral("heart-scale"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "하트비늘 %1개")},
            {QStringLiteral("athlete-points"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "%1포인트")},
            {QStringLiteral("watts"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "%1와트")},
            {QStringLiteral("league-points"),
             QT_TRANSLATE_NOOP("com::yamada::studio::guidebook", "%1LP")},
    };
    const auto it = units.constFind(unit);
    return it == units.constEnd() ? QStringLiteral("%1 %2").arg(number, unit) : tr(*it).arg(number);
}

// 한 줄로: [방법] · [장소] · [컨텐츠 비용] (조건)
QString sourceText(const Source &source, Language language)
{
    QStringList parts;
    const QString how = howLabel(source.how);
    if (!how.isEmpty())
        parts.append(how);
    QString where = source.place.text(language);
    if (where.isEmpty() && !source.where.isEmpty())
        where = com::yamada::studio::guidebook::placeName(source.where, {}, language);
    if (!where.isEmpty())
        parts.append(where);
    QStringList price;
    const QString content = source.content.text(language);
    if (!content.isEmpty())
        price.append(content);
    QStringList costs;
    for (const auto &[amount, unit] : source.cost)
        costs.append(costText(amount, unit));
    if (!costs.isEmpty())
        price.append(costs.join(QStringLiteral(" + ")));
    if (!price.isEmpty())
        parts.append(price.join(QLatin1Char(' ')));
    QString line = parts.join(QStringLiteral(" · "));
    const QString detail = source.detail.text(language);
    if (!detail.isEmpty())
        line += QStringLiteral(" (%1)").arg(detail);
    return line;
}

const Book &book()
{
    static const Book instance = load(); // 처음 부를 때 한 번(C++11부터 스레드 안전)
    return instance;
}
} // namespace

namespace com::yamada::studio::guidebook {
QStringList itemSources(const QString &versionGroup, const QString &item, Language language)
{
    QStringList lines;
    for (const Source &source : book().games.value(versionGroup).items.value(item))
        lines.append(sourceText(source, language));
    return lines;
}

bool hasItemBook(const QString &versionGroup)
{
    const auto it = book().games.constFind(versionGroup);
    // 기술머신만 적힌 사전(플라티나 등)으로 다른 아이템을 숨기면 안 된다 — 기술머신 · 비전머신이
    // 아닌 아이템이 하나라도 있어야 "그 게임 전체 목록"으로 본다
    if (it == book().games.constEnd())
        return false;
    for (auto item = it->items.cbegin(); item != it->items.cend(); ++item)
        if (!item.key().startsWith(QLatin1String("tm"))
            && !item.key().startsWith(QLatin1String("hm"))
            && !item.key().startsWith(QLatin1String("tr")))
            return true;
    return false;
}

QSet<QString> itemsIn(const QString &versionGroup)
{
    const QList<QString> keys = book().games.value(versionGroup).items.keys();
    return {keys.cbegin(), keys.cend()};
}

QString tutorCost(const QString &versionGroup, const QString &move, Language language)
{
    QStringList lines;
    for (const Source &source : book().games.value(versionGroup).tutors.value(move)) {
        Source price = source;
        price.how.clear(); // 칸이 이미 "NPC 가르침"이다 — 방법 이름은 빼고 장소 · 컨텐츠 · 비용만
        lines.append(sourceText(price, language));
    }
    return lines.join(QStringLiteral(" / "));
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
    if (!text.isEmpty())
        return text;
    // 사전 · PokéAPI 이름이 둘 다 없으면 identifier를 읽기 좋게("oreburgh-mine" → "Oreburgh Mine")
    QStringList words = identifier.split(QLatin1Char('-'), Qt::SkipEmptyParts);
    for (QString &word : words)
        word[0] = word[0].toUpper();
    return words.join(QLatin1Char(' '));
}

QString methodName(const QString &identifier, Language language)
{
    const QString text = book().methods.value(identifier).text(language);
    return text.isEmpty() ? QString(identifier).replace(QLatin1Char('-'), QLatin1Char(' ')) : text;
}
} // namespace com::yamada::studio::guidebook
