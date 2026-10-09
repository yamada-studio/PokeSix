#include "data/update/dataupdater.h"

#include "data/db/gamedatabase.h"
#include "data/logging/logging.h"
#include "data/update/csvdownloader.h"
#include "data/update/csvimporter.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>

namespace com::yamada::studio {
// ── ImportWorker (worker 스레드) ─────────────────────────────────────────────
void ImportWorker::importCsv(const QString &csvDir, const QString &dbPath)
{
    qCInfo(lcData) << "importing on thread" << QThread::currentThread();

    // SQLite는 폴더가 없으면 파일을 만들지 못한다. 첫 실행이면 AppDataLocation 폴더가 아직 없다.
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    // CsvImporter importer; 를 만들고 importer.run(csvDir, dbPath)의 결과를
    //        emit importFinished(결과, importer.errorString()); 으로 알린다
    CsvImporter importer;
    const bool ok = importer.run(csvDir, dbPath);
    emit importFinished(ok, importer.errorString(), importer.pendingSwap());
}

// ── DataUpdater (UI 스레드) ──────────────────────────────────────────────────
DataUpdater::DataUpdater(QObject *parent)
    : QObject(parent)
    , m_downloader(new CsvDownloader(CsvDownloader::defaultDirectory(), this))
    , m_thread(new QThread(this))
    , m_worker(new ImportWorker) // 부모 없음: 다른 스레드로 옮길 객체는 부모를 가질 수 없다
{
    // worker를 m_thread로 옮긴다
    m_worker->moveToThread(m_thread);
    // 스레드가 끝나면 worker를 지우게 한다 (worker는 부모가 없어서 object tree가 지워 주지
    // 않는다)
    connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
    // 일 보내기: this의 importRequested → m_worker의 importCsv
    connect(this, &DataUpdater::importRequested, m_worker, &ImportWorker::importCsv);
    // 결과 받기: m_worker의 importFinished → this의 onImportFinished
    connect(m_worker, &ImportWorker::importFinished, this, &DataUpdater::onImportFinished);

    // 다운로더의 신호를 이 클래스의 신호로 바꿔 전한다. 받기는 전체 진행의 0–90%로 친다.
    connect(m_downloader, &CsvDownloader::progress, this,
            [this](int, int, qint64 bytes, qint64 totalBytes, const QString &) {
                const int percent = totalBytes > 0 ? static_cast<int>(bytes * 90 / totalBytes) : 0;
                emit progress(percent, tr("데이터 받는 중"));
            });
    connect(m_downloader, &CsvDownloader::finished, this, &DataUpdater::onDownloadFinished);
    connect(m_downloader, &CsvDownloader::failed, this,
            [this](CsvDownloader::Error error, const QString &detail) {
                m_busy = false;
                // 사용자가 취소한 것이면 "실패"가 아니라 "취소"로 알린다(화면이 받기 전 상태로
                // 돌아가야 한다)
                if (m_cancelling || error == CsvDownloader::Error::Cancelled) {
                    m_cancelling = false;
                    emit cancelled();
                } else {
                    emit failed(detail);
                }
            });

    // 스레드를 시작한다. 이제부터 worker 스레드의 이벤트 루프가 돌며 신호를 기다린다
    m_thread->start();
}

DataUpdater::~DataUpdater()
{
    // 스레드의 이벤트 루프를 끝내고(quit) 실제로 끝날 때까지 기다린다(wait).
    //        기다리지 않으면 QThread 객체가 돌고 있는 스레드보다 먼저 지워져 앱이 죽는다.
    m_thread->quit();
    m_thread->wait();
}

bool DataUpdater::hasData()
{
    return gamedatabase::isUsable(gamedatabase::defaultPath());
}

void DataUpdater::start()
{
    if (m_busy)
        return;
    m_busy = true;
    m_cancelling = false;
    emit progress(0, tr("데이터 받는 중"));
    m_downloader->start(); // 이미 받아 둔 파일은 건너뛴다(이어받기)
}

void DataUpdater::cancel()
{
    if (!m_busy)
        return;
    m_cancelling = true;
    m_downloader->cancel(); // 받는 중이면 곧바로 failed(Cancelled) → 위의 연결이 cancelled()로 바꿔
                            // 보낸다
}

void DataUpdater::onDownloadFinished()
{
    if (m_cancelling) { // 마지막 파일이 도착하는 순간 취소를 눌렀다 → 변환하지 않는다
        m_cancelling = false;
        m_busy = false;
        emit cancelled();
        return;
    }
    emit progress(90, tr("데이터 정리하는 중"));
    // DB 파일이 바뀔 참이다 → 이 프로세스의 연결부터 닫게 한다(MainWindow가 Repository::close)
    emit aboutToReplaceDatabase();
    // worker에게 변환을 시킨다: emit importRequested(CSV 폴더, DB 경로);
    //        CSV 폴더 = m_downloader->directory(), DB 경로 = gamedatabase::defaultPath()
    emit importRequested(m_downloader->directory(), gamedatabase::defaultPath());
}

void DataUpdater::onImportFinished(bool ok, const QString &error, bool pendingSwap)
{
    qCInfo(lcData) << "import result on thread" << QThread::currentThread();
    // ok면 emit progress(100, tr("준비 완료")); 와 emit finished();
    //        아니면 emit failed(error);
    m_busy = false;
    if (ok) {
        emit progress(100, tr("준비 완료"));
        emit finished();
    } else if (pendingSwap) {
        // 새 DB는 다 만들었다. 옛 파일을 다른 PokeSix 창이 잡고 있어 못 바꿔 넣었을
        // 뿐이다(Windows).
        emit replaceBlocked(error);
    } else {
        emit failed(error);
    }
}
} // namespace com::yamada::studio
