#include "data/store/squadstore.h"

#include "data/logging/logging.h"

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

constexpr int kFormatVersion = 1;

QJsonObject toJson(const Squad &squad)
{
    QJsonArray members;
    for (const SquadMember &m : squad.members) {
        QJsonArray moves;
        for (const int move : m.moves)
            moves.append(move);
        members.append(QJsonObject {{QStringLiteral("pokemon"), m.pokemonId},
                                    {QStringLiteral("memo"), m.memo},
                                    {QStringLiteral("moves"), moves},
                                    {QStringLiteral("ability"), m.abilityId},
                                    {QStringLiteral("nature"), m.natureId},
                                    {QStringLiteral("item"), m.itemId}});
    }
    return {{QStringLiteral("name"), squad.name},
            {QStringLiteral("versionGroup"), squad.versionGroup},
            {QStringLiteral("members"), members}};
}

Squad fromJson(const QJsonObject &object)
{
    Squad squad;
    squad.name = object.value(QStringLiteral("name")).toString();
    squad.versionGroup = object.value(QStringLiteral("versionGroup")).toString();
    const QJsonArray members = object.value(QStringLiteral("members")).toArray();
    for (qsizetype i = 0; i < members.size() && i < qsizetype(squad.members.size()); ++i) {
        const QJsonObject m = members.at(i).toObject();
        SquadMember &member = squad.members[std::size_t(i)];
        member.pokemonId = m.value(QStringLiteral("pokemon")).toInt();
        member.memo = m.value(QStringLiteral("memo")).toString().left(SquadMember::kMemoLength);
        const QJsonArray moves = m.value(QStringLiteral("moves")).toArray();
        for (qsizetype j = 0; j < moves.size() && j < qsizetype(member.moves.size()); ++j)
            member.moves[std::size_t(j)] = moves.at(j).toInt();
        member.abilityId = m.value(QStringLiteral("ability")).toInt();
        member.natureId = m.value(QStringLiteral("nature")).toInt();
        member.itemId = m.value(QStringLiteral("item")).toInt();
    }
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

Squad SquadStore::squad(int generation) const
{
    return m_squads.value(generation);
}

void SquadStore::setSquad(int generation, const Squad &squad)
{
    if (m_squads.value(generation) == squad)
        return;
    m_squads.insert(generation, squad);
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
    const QJsonObject squads = document.object().value(QStringLiteral("squads")).toObject();
    for (auto it = squads.begin(); it != squads.end(); ++it) {
        bool ok = false;
        const int generation = it.key().toInt(&ok);
        if (ok)
            m_squads.insert(generation, fromJson(it.value().toObject()));
    }
    qCInfo(lcData) << "loaded" << m_squads.size() << "squads from" << m_path;
}

bool SquadStore::write()
{
    QJsonObject squads;
    for (auto it = m_squads.cbegin(); it != m_squads.cend(); ++it)
        squads.insert(QString::number(it.key()), toJson(it.value()));
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
