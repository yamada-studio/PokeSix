#include "ui/dex/guidebook.h"

#include "ui/logging/logging.h"
#include "ui/theme/dexstyle.h"

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
    QStringList versions; // 이 버전에서만(["white-2"]). 비면 묶음의 모든 버전
    QString region; // 진행 기준 지방("johto" · "kanto") — 두 지방을 오가는 게임만 적는다
};

struct GameBook
{
    QHash<QString, QList<Source>> items;  // 아이템 identifier →
    QHash<QString, QList<Source>> tutors; // 기술 identifier →
    // 이 목록이 "그 게임에서 얻을 수 있는 아이템 전부"인가. 자동 수집 사전(Serebii)은 상점 ·
    // BP 교환 일부가 빠져 있어 false — 입수처 표시에만 쓰고 존재 필터링에는 쓰지 않는다.
    bool completeItems = false;
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
        source.region = o.value(QStringLiteral("region")).toString();
        for (const QJsonValue &version : o.value(QStringLiteral("versions")).toArray())
            source.versions.append(version.toString());
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
        game.completeItems = root.value(QStringLiteral("completeItemList")).toBool();
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
    const QString content = source.content.text(language);
    if (!where.isEmpty() && where != content) // "배틀프런티어 · 배틀프런티어 32BP" → 한 번만
        parts.append(where);
    QStringList price;
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
QStringList itemSources(const QString &versionGroup, const QString &item, Language language,
                        const QString &version)
{
    QStringList lines;
    const QList<Source> sources = book().games.value(versionGroup).items.value(item);
    for (const Source &source : sources) {
        if (source.versions.isEmpty()) {
            lines.append(sourceText(source, language));
        } else if (version.isEmpty()) {
            // 묶음 전체를 볼 때(아이템 백과): 버전 한정 입수처는 그 버전 약칭을 앞에("W2 — 13번
            // 도로")
            QStringList names;
            for (const QString &only : source.versions)
                names.append(dexstyle::version(only, {}).shortName);
            lines.append(QStringLiteral("%1 — %2").arg(names.join(QLatin1Char('/')),
                                                       sourceText(source, language)));
        } else if (source.versions.contains(version)) {
            lines.append(sourceText(source, language));
        }
    }
    static const char *const otherVersion = QT_TRANSLATE_NOOP(
            "com::yamada::studio::guidebook", "이 버전에서는 얻을 수 없어요(다른 버전 한정)");
    if (lines.isEmpty() && !sources.isEmpty())
        lines.append(tr(otherVersion));
    return lines;
}

bool hasItemBook(const QString &versionGroup)
{
    // 사전이 스스로 "완전 목록"이라고 선언할 때만(completeItemList) 존재 필터링에 쓴다.
    // 자동 수집 사전은 상점 · BP 교환 일부가 빠져 있어서, 이걸로 거르면 초이스밴드처럼
    // 실제로 얻을 수 있는 아이템이 목록에서 사라진다.
    const auto it = book().games.constFind(versionGroup);
    return it != book().games.constEnd() && it->completeItems && !it->items.isEmpty();
}

QSet<QString> itemsIn(const QString &versionGroup)
{
    const QList<QString> keys = book().games.value(versionGroup).items.keys();
    return {keys.cbegin(), keys.cend()};
}

ItemSupply itemSupply(const QString &versionGroup, const QString &item, const QString &version)
{
    // 한 번만 얻는 방법 / 반복해서 얻는 방법 — roadmap "다음 세션 할 일" 2의 판정 기준
    static const QSet<QString> once
            = {QStringLiteral("field"), QStringLiteral("hidden"), QStringLiteral("gift"),
               QStringLiteral("reward"), QStringLiteral("trade")};
    static const QSet<QString> repeat
            = {QStringLiteral("shop"), QStringLiteral("exchange"), QStringLiteral("prize")};
    // 엔딩 후에야 가는 지방(1차 근사) — 배지별 진행표 자료가 오면 그걸로 교체한다
    static const QHash<QString, QString> postGameRegion = {
            {QStringLiteral("gold-silver"), QStringLiteral("kanto")},
            {QStringLiteral("crystal"), QStringLiteral("kanto")},
            {QStringLiteral("heartgold-soulsilver"), QStringLiteral("kanto")},
    };
    const QString postGame = postGameRegion.value(versionGroup);
    ItemSupply supply;
    bool beforeEnding = false; // 엔딩 전에 가는 입수처가 하나라도 있다
    for (const Source &source : book().games.value(versionGroup).items.value(item)) {
        if (!source.versions.isEmpty() && !version.isEmpty() && !source.versions.contains(version))
            continue;
        const bool isOnce = once.contains(source.how);
        if (!isOnce && !repeat.contains(source.how))
            continue; // 픽업 같은 랜덤 입수(other)는 셀 수 없다
        supply.known = true;
        if (isOnce) {
            ++supply.copies;
        } else {
            supply.repeatable = true;
            if (supply.repeatCost.isEmpty())
                supply.repeatCost = source.cost;
        }
        if (postGame.isEmpty() || source.region != postGame)
            beforeEnding = true;
    }
    supply.postGameOnly = supply.known && !postGame.isEmpty() && !beforeEnding;
    return supply;
}

QList<std::pair<int, QString>> tutorCostAmounts(const QString &versionGroup, const QString &move,
                                                const QString &version)
{
    for (const Source &source : book().games.value(versionGroup).tutors.value(move)) {
        if (!source.versions.isEmpty() && !version.isEmpty() && !source.versions.contains(version))
            continue;
        if (!source.cost.isEmpty())
            return source.cost;
    }
    return {};
}

QString costLabel(int amount, const QString &unit)
{
    return costText(amount, unit);
}

QList<ItemAt> itemsAt(const QString &versionGroup, const QString &where, Language language,
                      const QString &version)
{
    QList<ItemAt> found;
    const GameBook &game = book().games.value(versionGroup);
    for (auto it = game.items.cbegin(); it != game.items.cend(); ++it)
        for (const Source &source : it.value()) {
            if (source.where != where)
                continue;
            if (!source.versions.isEmpty() && !version.isEmpty()
                && !source.versions.contains(version))
                continue;
            Source line = source; // 이미 그 장소에 있다 — 장소는 빼고 방법 · 비용 · 조건만
            line.where.clear();
            line.place = {};
            found.append({it.key(), sourceText(line, language)});
        }
    return found;
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
