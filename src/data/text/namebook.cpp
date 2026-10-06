#include "data/text/namebook.h"

#include "data/logging/logging.h"

#include <QFile>
#include <QHash>
#include <QJsonDocument>
#include <QJsonObject>

#include <array>

namespace {
using namespace com::yamada::studio;

constexpr char kPath[] = ":/data/names.json";

// JSON 묶음 이름 ↔ Kind (순서 = Kind 값)
constexpr std::array<const char *, 7> kSections
        = {"items",        "moves",        "abilities",      "versions",
           "item-effects", "move-effects", "ability-effects"};

// 사전 한 칸: 값 + "PokéAPI 값을 바로잡는다"(fix: true — PokéAPI 쪽 오탈자. 빈 칸이 아니어도
// 덮는다)
struct Entry
{
    LocalizedText text;
    bool fix = false;
};

using Book = std::array<QHash<QString, Entry>, kSections.size()>;

// 글자 하나(한국어) 또는 {ko, en, ja}
LocalizedText textOf(const QJsonValue &value)
{
    if (!value.isObject())
        return {value.toString(), {}, {}};
    const QJsonObject o = value.toObject();
    return {o.value(QStringLiteral("ko")).toString(), o.value(QStringLiteral("en")).toString(),
            o.value(QStringLiteral("ja")).toString()};
}

Book load()
{
    Book book;
    QFile file(QString::fromLatin1(kPath));
    if (!file.open(QIODevice::ReadOnly))
        return book; // 테스트 등 리소스가 없는 실행 — 사전 없이(PokéAPI 값 그대로) 간다
    QJsonParseError error {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (!document.isObject()) {
        qCWarning(lcData) << kPath << "is not valid JSON:" << error.errorString();
        return book;
    }
    const QJsonObject root = document.object();
    for (std::size_t i = 0; i < kSections.size(); ++i) {
        const QJsonObject section = root.value(QLatin1String(kSections[i])).toObject();
        for (auto it = section.begin(); it != section.end(); ++it)
            book[i].insert(it.key(),
                           {textOf(*it), it->toObject().value(QStringLiteral("fix")).toBool()});
    }
    return book;
}

const Book &book()
{
    static const Book instance = load(); // 처음 부를 때 한 번(C++11부터 스레드 안전)
    return instance;
}
} // namespace

namespace com::yamada::studio::namebook {
void fill(Kind kind, const QString &identifier, LocalizedText &text)
{
    const auto &section = book()[static_cast<std::size_t>(kind)];
    const auto it = section.constFind(identifier);
    if (it == section.constEnd())
        return;
    const LocalizedText &entry = it->text;
    // 보통은 빈 칸만 채운다. fix 항목은 PokéAPI 값이 틀린 것이라 사전 값으로 덮는다.
    if (!entry.ko.isEmpty() && (text.ko.isEmpty() || it->fix))
        text.ko = entry.ko;
    if (!entry.en.isEmpty() && (text.en.isEmpty() || it->fix))
        text.en = entry.en;
    if (!entry.ja.isEmpty() && (text.ja.isEmpty() || it->fix))
        text.ja = entry.ja;
}
} // namespace com::yamada::studio::namebook
