#include "data/sprites/spritecache.h"

#include "data/logging/logging.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
// PokéAPI/sprites 저장소. 8세대 박스 아이콘(68×56 도트)이 목록 한 줄에 알맞다. 8세대 아이콘이 없는
// 번호(9세대 등)는 기본 정면 스프라이트(96×96)로 대신한다.
// TODO: CSV처럼 커밋을 고정할지 정한다. 지금은 master(이미지는 거의 바뀌지 않는다).
constexpr const char *kSources[] = {
        "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/versions/"
        "generation-viii/icons/%1.png",
        "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/pokemon/%1.png",
};
constexpr int kSourceCount = sizeof(kSources) / sizeof(kSources[0]);
} // namespace

namespace com::yamada::studio {
SpriteCache::SpriteCache(QObject *parent)
    : QObject(parent)
    , m_dir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/sprites/icons"))
    , m_network(new QNetworkAccessManager(this))
{
    QDir().mkpath(m_dir);
}

QString SpriteCache::filePath(int pokemonId) const
{
    return QStringLiteral("%1/%2.png").arg(m_dir).arg(pokemonId);
}

QString SpriteCache::path(int pokemonId)
{
    if (const auto it = m_known.constFind(pokemonId); it != m_known.cend())
        return *it;
    if (m_pending.contains(pokemonId) || m_failed.contains(pokemonId))
        return {};
    const QString file = filePath(pokemonId);
    if (!QFile::exists(file))
        return {};
    m_known.insert(pokemonId, file);
    return file;
}

void SpriteCache::request(int pokemonId)
{
    if (m_known.contains(pokemonId) || m_pending.contains(pokemonId)
        || m_failed.contains(pokemonId))
        return;
    m_pending.insert(pokemonId);
    fetch(pokemonId, 0);
}

void SpriteCache::fetch(int pokemonId, int attempt)
{
    const QUrl url(QString::fromLatin1(kSources[attempt]).arg(pokemonId));
    QNetworkReply *reply = m_network->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, pokemonId, attempt] { onFinished(reply, pokemonId, attempt); });
}

void SpriteCache::onFinished(QNetworkReply *reply, int pokemonId, int attempt)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (attempt + 1 < kSourceCount) { // 다음 출처로
            fetch(pokemonId, attempt + 1);
            return;
        }
        qCWarning(lcData) << "sprite" << pokemonId << "failed:" << reply->errorString();
        m_pending.remove(pokemonId);
        m_failed.insert(pokemonId);
        return;
    }
    // QSaveFile: 다 쓴 뒤에 이름을 바꾼다 → 받다 끊겨도 반쪽 PNG가 캐시에 남지 않는다
    QSaveFile file(filePath(pokemonId));
    if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0 || !file.commit()) {
        m_pending.remove(pokemonId);
        m_failed.insert(pokemonId);
        return;
    }
    m_pending.remove(pokemonId);
    m_known.insert(pokemonId, file.fileName());
    emit ready(pokemonId);
}
} // namespace com::yamada::studio
