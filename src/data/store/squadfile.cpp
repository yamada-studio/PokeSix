#include "data/store/squadfile.h"

#include "data/store/squadstore.h"

#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>

namespace {
QString tr(const char *text)
{
    return QCoreApplication::translate("com::yamada::studio::squadfile", text);
}
} // namespace

namespace com::yamada::studio::squadfile {
bool save(const QString &path, const Portable &portable, QString *error)
{
    const QJsonObject root {{QStringLiteral("pokesix-squad"), kFormatVersion},
                            {QStringLiteral("generation"), portable.generation},
                            {QStringLiteral("game"), portable.game},
                            {QStringLiteral("squad"), squadToJson(portable.squad)}};
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        if (error)
            *error = tr("파일에 쓰지 못했어요: %1").arg(file.errorString());
        return false;
    }
    return true;
}

std::optional<Portable> load(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = tr("파일을 열지 못했어요: %1").arg(file.errorString());
        return std::nullopt;
    }
    QJsonParseError parse {};
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parse);
    if (!document.isObject()) {
        if (error)
            *error = tr("JSON이 아니에요: %1").arg(parse.errorString());
        return std::nullopt;
    }
    const QJsonObject root = document.object();
    const int version = root.value(QStringLiteral("pokesix-squad")).toInt();
    if (version < 1) {
        if (error)
            *error = tr("PokeSix 스쿼드 파일이 아니에요");
        return std::nullopt;
    }
    Portable portable;
    portable.generation = root.value(QStringLiteral("generation")).toInt();
    portable.game = root.value(QStringLiteral("game")).toString();
    portable.squad = squadFromJson(root.value(QStringLiteral("squad")).toObject());
    if (portable.generation < 1) {
        if (error)
            *error = tr("세대 값이 이상해요: %1").arg(portable.generation);
        return std::nullopt;
    }
    return portable;
}

QString fileName(const Portable &portable)
{
    // 어느 OS에서나 안전하게 ASCII로 — 스쿼드 이름은 파일 안에 있다
    return QStringLiteral("pokesix-squad-%1-%2.json")
            .arg(portable.generation)
            .arg(portable.game.isEmpty() ? QStringLiteral("squad") : portable.game);
}
} // namespace com::yamada::studio::squadfile
