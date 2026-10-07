#include "data/store/squadstore.h"

#include "data/logging/logging.h"
#include "data/repository/repository.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
using namespace com::yamada::studio;

constexpr int kFormatVersion = 2;

QJsonObject memberToJson(const SquadMember &m)
{
    QJsonArray moves;
    for (const int move : m.moves)
        moves.append(move);
    return {{QStringLiteral("pokemon"), m.pokemonId}, {QStringLiteral("memo"), m.memo},
            {QStringLiteral("moves"), moves},         {QStringLiteral("ability"), m.abilityId},
            {QStringLiteral("nature"), m.natureId},   {QStringLiteral("item"), m.itemId}};
}

SquadMember memberFromJson(const QJsonObject &object)
{
    SquadMember member;
    member.pokemonId = object.value(QStringLiteral("pokemon")).toInt();
    member.memo = object.value(QStringLiteral("memo")).toString().left(SquadMember::kMemoLength);
    const QJsonArray moves = object.value(QStringLiteral("moves")).toArray();
    for (qsizetype j = 0; j < moves.size() && j < qsizetype(member.moves.size()); ++j)
        member.moves[std::size_t(j)] = moves.at(j).toInt();
    member.abilityId = object.value(QStringLiteral("ability")).toInt();
    member.natureId = object.value(QStringLiteral("nature")).toInt();
    member.itemId = object.value(QStringLiteral("item")).toInt();
    return member;
}

QJsonObject toJson(const Squad &squad)
{
    QJsonArray members;
    for (const SquadMember &m : squad.members)
        members.append(memberToJson(m));
    QJsonObject json {{QStringLiteral("name"), squad.name}, {QStringLiteral("members"), members}};
    if (!squad.shuttle.isEmpty()) // 비전셔틀(7번째 멤버)은 따로 — 옛 버전은 이 키를 모른 채 읽는다
        json.insert(QStringLiteral("shuttle"), memberToJson(squad.shuttle));
    return json;
}

Squad fromJson(const QJsonObject &object)
{
    Squad squad;
    squad.name = object.value(QStringLiteral("name")).toString();
    const QJsonArray members = object.value(QStringLiteral("members")).toArray();
    for (qsizetype i = 0; i < members.size() && i < qsizetype(squad.members.size()); ++i)
        squad.members[std::size_t(i)] = memberFromJson(members.at(i).toObject());
    squad.shuttle = memberFromJson(object.value(QStringLiteral("shuttle")).toObject());
    return squad;
}
} // namespace

namespace com::yamada::studio {
SquadStore::SquadStore(const QString &path, QObject *parent)
    : QObject(parent)
    , m_path(path.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                      + QStringLiteral("/squads.json")
                            : path)
{
    m_timer.setSingleShot(true);
    m_timer.setInterval(kSaveDelayMs);
    connect(&m_timer, &QTimer::timeout, this, [this] { emit saved(write()); });
    load();
}

SquadStore::~SquadStore()
{
    if (m_timer.isActive())
        write(); // 시그널은 보내지 않는다(받을 위젯이 이미 없을 수 있다)
}

Squad SquadStore::squad(int generation, const QString &versionGroup) const
{
    return m_squads.value(generation).value(versionGroup);
}

void SquadStore::setSquad(int generation, const QString &versionGroup, const Squad &squad)
{
    if (m_squads.value(generation).value(versionGroup) == squad)
        return;
    m_squads[generation].insert(versionGroup, squad);
    schedule();
}

void SquadStore::setCurrentGame(int generation, const QString &versionGroup)
{
    if (m_current.value(generation) == versionGroup)
        return;
    m_current.insert(generation, versionGroup);
    schedule();
}

void SquadStore::schedule()
{
    m_timer.start(); // 다시 걸면 처음부터 센다 → 입력이 멈춘 뒤 한 번
    emit saveScheduled();
}

bool SquadStore::flush()
{
    if (!m_timer.isActive())
        return true;
    m_timer.stop();
    const bool ok = write();
    emit saved(ok);
    return ok;
}

void SquadStore::load()
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
        return; // 처음 실행: 파일이 없다
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qCWarning(lcData) << "squads.json is broken:" << error.errorString();
        return;
    }
    const int version = document.object().value(QStringLiteral("version")).toInt(1);
    const QJsonObject squads = document.object().value(QStringLiteral("squads")).toObject();
    for (auto it = squads.begin(); it != squads.end(); ++it) {
        bool ok = false;
        const int generation = it.key().toInt(&ok);
        if (!ok)
            continue;
        const QJsonObject entry = it.value().toObject();
        if (version < 2) {
            // version 1: 세대마다 스쿼드 하나 → 그 스쿼드의 게임(비어 있으면 대표 게임)으로 옮긴다
            QString game = entry.value(QStringLiteral("versionGroup")).toString();
            if (game.isEmpty())
                game = Repository::representativeVersionGroup(generation);
            m_squads[generation].insert(game, fromJson(entry));
            m_current.insert(generation, game);
            continue;
        }
        const QJsonObject games = entry.value(QStringLiteral("games")).toObject();
        for (auto game = games.begin(); game != games.end(); ++game)
            m_squads[generation].insert(game.key(), fromJson(game.value().toObject()));
        m_current.insert(generation, entry.value(QStringLiteral("current")).toString());
    }
    qCInfo(lcData) << "loaded" << m_squads.size() << "squads from" << m_path;
}

bool SquadStore::write()
{
    QJsonObject squads;
    QList<int> generations = m_squads.keys();
    for (const int generation : m_current.keys())
        if (!generations.contains(generation))
            generations.append(generation);
    for (const int generation : generations) {
        QJsonObject games;
        const QHash<QString, Squad> &bySeries = m_squads[generation];
        for (auto it = bySeries.cbegin(); it != bySeries.cend(); ++it)
            games.insert(it.key(), toJson(it.value()));
        squads.insert(QString::number(generation),
                      QJsonObject {{QStringLiteral("current"), m_current.value(generation)},
                                   {QStringLiteral("games"), games}});
    }
    const QJsonObject root {{QStringLiteral("version"), kFormatVersion},
                            {QStringLiteral("squads"), squads}};

    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)
        || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        qCWarning(lcData) << "saving squads failed:" << file.errorString();
        return false;
    }
    return true;
}
} // namespace com::yamada::studio
