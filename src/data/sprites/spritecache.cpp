#include "data/sprites/spritecache.h"

#include "data/logging/logging.h"

#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
// PokéAPI/sprites 저장소.
// TODO: CSV처럼 커밋을 고정할지 정한다. 지금은 master(이미지는 거의 바뀌지 않는다).
constexpr char kBase[] = "https://raw.githubusercontent.com/PokeAPI/sprites/master/sprites/";
} // namespace

namespace com::yamada::studio {
SpriteCache::SpriteCache(Kind kind, QObject *parent)
    : QObject(parent)
    , m_network(new QNetworkAccessManager(this))
{
    const QString base = QString::fromLatin1(kBase);
    QString folder;
    switch (kind) {
    case Kind::PokemonIcon:
        // 8세대 박스 아이콘이 목록 한 줄에 알맞다. 없는 번호(9세대 등)는 정면 스프라이트로
        // 대신한다.
        m_sources = {base + QStringLiteral("pokemon/versions/generation-viii/icons/%1.png"),
                     base + QStringLiteral("pokemon/%1.png")};
        folder = QStringLiteral("icons");
        break;
    case Kind::Item:
        m_sources = {base + QStringLiteral("items/%1.png")};
        folder = QStringLiteral("items");
        break;
    }
    m_dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
            + QStringLiteral("/sprites/") + folder;
    QDir().mkpath(m_dir);
}

QString SpriteCache::filePath(const QString &key) const
{
    return QStringLiteral("%1/%2.png").arg(m_dir, key);
}

QString SpriteCache::path(const QString &key)
{
    if (const auto it = m_known.constFind(key); it != m_known.cend())
        return *it;
    if (m_pending.contains(key) || m_failed.contains(key))
        return {};
    const QString file = filePath(key);
    if (!QFile::exists(file))
        return {};
    m_known.insert(key, file);
    return file;
}

void SpriteCache::request(const QString &key)
{
    if (key.isEmpty() || m_known.contains(key) || m_pending.contains(key) || m_failed.contains(key))
        return;
    m_pending.insert(key);
    fetch(key, 0);
}

void SpriteCache::fetch(const QString &key, int attempt)
{
    const QUrl url(m_sources.at(attempt).arg(key));
    QNetworkReply *reply = m_network->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, key, attempt] { onFinished(reply, key, attempt); });
}

void SpriteCache::onFinished(QNetworkReply *reply, const QString &key, int attempt)
{
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        if (attempt + 1 < m_sources.size()) { // 다음 출처로
            fetch(key, attempt + 1);
            return;
        }
        qCDebug(lcData) << "sprite" << key << "failed:" << reply->errorString();
        m_pending.remove(key);
        m_failed.insert(key);
        return;
    }
    // QSaveFile: 다 쓴 뒤에 이름을 바꾼다 → 받다 끊겨도 반쪽 PNG가 캐시에 남지 않는다
    QSaveFile file(filePath(key));
    if (!file.open(QIODevice::WriteOnly) || file.write(reply->readAll()) < 0 || !file.commit()) {
        m_pending.remove(key);
        m_failed.insert(key);
        return;
    }
    m_pending.remove(key);
    m_known.insert(key, file.fileName());
    emit ready(key);
}
} // namespace com::yamada::studio
