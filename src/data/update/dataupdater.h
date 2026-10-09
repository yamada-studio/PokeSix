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
    // pendingSwap: 새 DB는 만들었지만 자리에 넣지 못했다(CsvImporter::pendingSwap)
    void importFinished(bool ok, const QString &error, bool pendingSwap);
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

    void start(); // 이미 진행 중이면 무시한다
    void
    cancel(); // 받는 중이면 멈추고 cancelled()를 보낸다. 변환(수 초 이내)은 멈추지 않고 끝까지 간다
    bool isBusy() const { return m_busy; }

signals:
    void progress(int percent, const QString &label); // 0–100, 지금 하는 일
    void finished();                                  // DB 준비 끝
    void failed(const QString &message);
    // 받기 · 변환은 끝났는데 새 DB를 자리에 넣지 못했다(다른 PokeSix 창이 옛 DB를 열어 둠 —
    // Windows). 임시 DB는 남아 있어 다음 실행 · 다시 시도 때 들어간다. 화면은 "네트워크" 대신 "다른
    // 창"을 안내한다.
    void replaceBlocked(const QString &detail);
    void cancelled(); // cancel()로 멈췄다. 받은 파일은 남아 있다(이어받기)
    // 변환이 끝나면 DB 파일이 통째로 바뀐다. 그 전에 이 프로세스가 연 연결(Repository)을 닫으라는
    // 신호 — Windows는 열린 파일을 바꿔치기하지 못한다.
    void aboutToReplaceDatabase();

    // 내부용: worker에게 일을 넘기는 신호. 밖에서 connect하지 않는다.
    void importRequested(const QString &csvDir, const QString &dbPath);

private:
    void onDownloadFinished();
    void onImportFinished(bool ok, const QString &error, bool pendingSwap);

    CsvDownloader *m_downloader = nullptr;
    QThread *m_thread = nullptr;
    ImportWorker *m_worker = nullptr;
    bool m_busy = false;       // start()부터 finished · failed · cancelled까지
    bool m_cancelling = false; // cancel()을 불렀다 → 다운로더의 실패를 "취소"로 알린다
};
} // namespace com::yamada::studio
