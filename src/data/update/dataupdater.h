#pragma once

#include <QObject>
#include <QString>

class QThread;

namespace com::yamada::studio {
class CsvDownloader;

// worker 스레드에서 도는 쪽. CsvImporter::run()(동기, 수 초)을 UI 스레드가 아닌 곳에서 부르기 위한
// 껍데기다. moveToThread()로 다른 스레드에 옮겨지므로 부모를 가질 수 없다(부모와 자식은 같은
// 스레드에 있어야 한다).
class ImportWorker : public QObject
{
    Q_OBJECT
public:
    // DataUpdater가 보낸 신호로 불린다. 이 함수 안은 worker 스레드다.
    void importCsv(const QString &csvDir, const QString &dbPath);

signals:
    void importFinished(bool ok, const QString &error);
};

// 첫 실행 흐름 전체: 받기(CsvDownloader, 비동기) → 변환(ImportWorker, worker 스레드) → 끝.
// 디자인 v2 03b §3의 DataUpdater. 인트로의 FirstRunPanel이 이 신호를 받는다.
//
//   UI 스레드                                   worker 스레드
//   ─────────                                   ─────────────
//   start() → CsvDownloader가 받는다
//   onDownloadFinished()
//     emit importRequested ──(큐로 넘어감)──▶ ImportWorker::importCsv()  ← CsvImporter::run()
//   onImportFinished()  ◀──(큐로 넘어옴)── emit importFinished
//     emit finished / failed
//
// 서로 다른 스레드의 객체를 connect하면 Qt가 알아서 "큐 연결(Qt::QueuedConnection)"로 만든다.
// 신호의 인자를 복사해서 받는 쪽 스레드의 이벤트 큐에 넣고, 그 스레드의 이벤트 루프가 슬롯을
// 부른다. ROS 2로 치면 콜백을 다른 executor 스레드에서 돌리고, 결과를 토픽으로 되돌려 받는 것과
// 같다.
class DataUpdater : public QObject
{
    Q_OBJECT
public:
    explicit DataUpdater(QObject *parent = nullptr);
    ~DataUpdater() override;

    // 쓸 수 있는 게임 데이터 DB가 이미 있는가(= 첫 실행이 아닌가)
    static bool hasData();

    void start();

signals:
    void progress(int percent, const QString &label); // 0–100, 지금 하는 일
    void finished();                                  // DB 준비 끝
    void failed(const QString &message);

    // 내부용: worker에게 일을 넘기는 신호. 밖에서 connect하지 않는다.
    void importRequested(const QString &csvDir, const QString &dbPath);

private:
    void onDownloadFinished();
    void onImportFinished(bool ok, const QString &error);

    CsvDownloader *m_downloader = nullptr;
    QThread *m_thread = nullptr;
    ImportWorker *m_worker = nullptr;
};
} // namespace com::yamada::studio
