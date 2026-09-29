#include "data/update/csvdownloader.h"

#include "data/logging/logging.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

namespace {
using com::yamada::studio::csvsource::CsvFile;
using com::yamada::studio::csvsource::kCommit;
using com::yamada::studio::csvsource::kFiles;

constexpr int kTransferTimeoutMs = 30'000; // 30초 동안 한 바이트도 오지 않으면 실패로 본다

QString toQString(std::string_view text)
{
    return QString::fromLatin1(text.data(), static_cast<qsizetype>(text.size()));
}

QUrl urlOf(const CsvFile &file)
{
    return QUrl(QStringLiteral(
                        "https://raw.githubusercontent.com/PokeAPI/pokeapi/%1/data/v2/csv/%2.csv")
                        .arg(QLatin1StringView(kCommit), toQString(file.name)));
}

// 해시를 목록의 16진수 문자열과 비교한다(대소문자 구분 없이).
bool sameDigest(const QByteArray &digest, std::string_view expectedHex)
{
    return digest.toHex()
           == QByteArray(expectedHex.data(), static_cast<qsizetype>(expectedHex.size())).toLower();
}
} // namespace

namespace com::yamada::studio {
CsvDownloader::CsvDownloader(const QString &directory, QObject *parent)
    : QObject(parent)
    , m_directory(directory)
    , m_network(new QNetworkAccessManager(this)) // 부모 = this → 함께 소멸한다
{
    m_network->setTransferTimeout(kTransferTimeoutMs);
}

CsvDownloader::~CsvDownloader()
{
    if (m_reply) {
        m_reply->disconnect(this); // 소멸 중에 onReplyFinished가 불리지 않게 먼저 연결을 끊는다
        m_reply->abort();
    }
    // m_file(QSaveFile)은 commit()하지 않은 채 소멸하면 임시 파일을 지운다. m_network는 부모가
    // 지운다.
}

QString CsvDownloader::defaultDirectory()
{
    return QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
           + QStringLiteral("/pokeapi-csv/") + QLatin1StringView(kCommit);
}

bool CsvDownloader::isValidCopy(const QString &path, qint64 size, std::string_view sha256)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() != size)
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    if (!hash.addData(
                &file)) // 파일을 조금씩 읽으며 해시한다(12 MB도 한 번에 메모리에 올리지 않는다)
        return false;
    return sameDigest(hash.result(), sha256);
}

bool CsvDownloader::isComplete() const
{
    for (const CsvFile &file : kFiles) {
        if (!isValidCopy(pathOf(file), file.size, file.sha256))
            return false;
    }
    return true;
}

void CsvDownloader::start()
{
    if (m_index >= 0)
        return; // 이미 받는 중
    m_index = 0;

    // 실제 일은 이벤트 루프로 미룬다(Qt::QueuedConnection). 그래서 start()는 어떤 경우에도
    // 시그널을 "동기적으로" 보내지 않는다. 이유: 전부 받아 둔 상태면 finished가 곧바로 나가는데,
    // 그게 start() 안에서 일어나면 호출한 쪽이 아직 이벤트 루프(exec)를 시작하기 전일 수 있다.
    // 그때 받은 쪽이 QCoreApplication::exit()를 부르면 무시되고, 뒤이은 exec()가 끝나지 않는다.
    // "비동기 API는 항상 비동기로 알린다"를 지키면 호출 순서를 신경 쓰지 않아도 된다.
    QMetaObject::invokeMethod(
            this,
            [this] {
                if (!QDir().mkpath(m_directory)) {
                    fail(Error::Disk, tr("폴더를 만들 수 없어요: %1").arg(m_directory));
                    return;
                }
                qCInfo(lcData) << "downloading PokéAPI CSV at" << kCommit << "into" << m_directory;
                startNext();
            },
            Qt::QueuedConnection);
}

void CsvDownloader::cancel()
{
    if (m_index >= 0)
        fail(Error::Cancelled, tr("취소했어요"));
}

void CsvDownloader::startNext()
{
    const int total = static_cast<int>(kFiles.size());

    // 이어받기: 이미 검증된 파일은 건너뛴다.
    while (m_index < total
           && isValidCopy(pathOf(kFiles[m_index]), kFiles[m_index].size, kFiles[m_index].sha256)) {
        qCDebug(lcData) << "already have" << toQString(kFiles[m_index].name);
        ++m_index;
        emit progress(m_index, total, bytesBefore(m_index), csvsource::kTotalSize,
                      toQString(kFiles[m_index - 1].name));
    }

    if (m_index == total) {
        qCInfo(lcData) << "all" << total << "CSV files are present and verified";
        m_index = -1;
        emit finished();
        return;
    }

    const CsvFile &file = kFiles[m_index];
    m_file = new QSaveFile(pathOf(file), this);
    if (!m_file->open(QIODevice::WriteOnly)) {
        fail(Error::Disk, tr("파일을 쓸 수 없어요: %1").arg(m_file->fileName()));
        return;
    }
    m_hash.reset();

    QNetworkRequest request(urlOf(file));
    // 누가 받아 가는지 밝힌다(PokéAPI 공정 이용 정책의 "예의 바르게"). 리다이렉트는 Qt 6 기본
    // 정책으로 따라간다.
    request.setHeader(QNetworkRequest::UserAgentHeader,
                      QStringLiteral("PokeSix/%1 (+https://github.com/yamada-studio/PokeSix)")
                              .arg(QCoreApplication::applicationVersion()));
    m_reply = m_network->get(request);

    // 시그널/슬롯: 응답 객체가 보내는 신호를 이 객체의 멤버 함수에 잇는다.
    connect(m_reply, &QNetworkReply::readyRead, this, &CsvDownloader::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &CsvDownloader::onReplyFinished);
    connect(m_reply, &QNetworkReply::downloadProgress, this,
            [this, total](qint64 received, qint64) {
                emit progress(m_index, total, bytesBefore(m_index) + received,
                              csvsource::kTotalSize, toQString(kFiles[m_index].name));
            });
    qCDebug(lcData) << "GET" << request.url().toString();
}

void CsvDownloader::onReadyRead()
{
    // 도착한 만큼만 읽어서 해시에 넣고 파일에 쓴다 — 큰 파일도 메모리를 적게 쓴다.
    const QByteArray chunk = m_reply->readAll();
    m_hash.addData(chunk);
    if (m_file->write(chunk) != chunk.size())
        fail(Error::Disk, tr("파일을 쓸 수 없어요: %1").arg(m_file->fileName()));
}

void CsvDownloader::onReplyFinished()
{
    QNetworkReply *reply = m_reply;
    m_reply = nullptr;
    reply->deleteLater(); // 시그널을 보낸 객체를 그 시그널 처리 중에 바로 delete하지 않는다

    const CsvFile &file = kFiles[m_index];
    if (reply->error() != QNetworkReply::NoError) {
        fail(Error::Network,
             tr("%1을(를) 받지 못했어요: %2").arg(toQString(file.name), reply->errorString()));
        return;
    }
    if (m_file->size() != file.size || !sameDigest(m_hash.result(), file.sha256)) {
        fail(Error::Checksum,
             tr("%1의 내용이 예상과 달라요(크기 또는 SHA-256)").arg(toQString(file.name)));
        return;
    }
    if (!m_file->commit()) { // 임시 파일 → 진짜 이름으로 한 번에 교체
        fail(Error::Disk, tr("파일을 저장할 수 없어요: %1").arg(m_file->fileName()));
        return;
    }
    delete m_file;
    m_file = nullptr;

    ++m_index;
    emit progress(m_index, static_cast<int>(kFiles.size()), bytesBefore(m_index),
                  csvsource::kTotalSize, toQString(file.name));
    startNext();
}

void CsvDownloader::fail(Error error, const QString &detail)
{
    if (m_reply) {
        m_reply->disconnect(this); // abort()가 finished를 곧바로 보내므로, 먼저 끊어서
                                   // onReplyFinished로 되돌아오지 않게 한다
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file) {
        m_file->cancelWriting(); // 반쯤 쓴 임시 파일을 버린다
        delete m_file;
        m_file = nullptr;
    }
    m_index = -1;
    qCWarning(lcData) << "CSV download failed:" << error << detail;
    emit failed(error, detail);
}

QString CsvDownloader::pathOf(const CsvFile &file) const
{
    return m_directory + QLatin1Char('/') + toQString(file.name) + QStringLiteral(".csv");
}

qint64 CsvDownloader::bytesBefore(int index) const
{
    qint64 sum = 0;
    for (int i = 0; i < index; ++i)
        sum += kFiles[i].size;
    return sum;
}
} // namespace com::yamada::studio
