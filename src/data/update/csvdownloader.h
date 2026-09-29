#pragma once

#include "data/update/csvsource.h"

#include <QCryptographicHash>
#include <QObject>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;
class QSaveFile;

namespace com::yamada::studio {
// PokéAPI CSV 원본(csvsource.h의 kFiles)을 고정 커밋에서 받아 한 폴더에 모은다.
// ADR 0011, 로드맵 D4.
//
//   CsvDownloader downloader(CsvDownloader::defaultDirectory());
//   connect(&downloader, &CsvDownloader::progress, …);  // 진행
//   connect(&downloader, &CsvDownloader::finished, …);  // 전부 받고 검증 끝
//   connect(&downloader, &CsvDownloader::failed, …);    // 네트워크 · 해시 · 디스크 오류
//   downloader.start();                                 // 곧바로 돌아온다(비동기)
//
// [비동기] start()는 첫 요청만 보내고 즉시 돌아온다. 데이터는 이벤트 루프가 돌면서
// QNetworkReply의 시그널(readyRead, finished)로 들어온다. 그래서 UI 스레드에서 써도
// 화면이 멈추지 않는다. ROS 2로 치면 spin() 중에 콜백이 불리는 것과 같다.
//
// [이어받기] 이미 있는 파일은 크기와 SHA-256이 맞으면 건너뛴다.
// 중간에 끊겨도 다음에는 남은 파일만 받는다.
//
// [검증] 받는 동안 SHA-256을 조금씩 계산하고, 끝나면 목록의 값과 비교한다.
// 다르면 파일을 남기지 않는다.
//
// [원자적 저장] QSaveFile은 임시 파일에 쓰다가 commit()할 때 한 번에 이름을 바꾼다.
// 도중에 끊기거나 검증에 실패하면 반쯤 쓴 파일이 남지 않는다.
class CsvDownloader : public QObject
{
    Q_OBJECT
public:
    enum class Error {
        Network,   // 연결 실패 · 시간 초과 · HTTP 오류
        Checksum,  // 받은 내용의 크기나 SHA-256이 목록과 다르다
        Disk,      // 폴더를 만들거나 파일을 쓸 수 없다
        Cancelled, // cancel()로 멈췄다
    };
    Q_ENUM(Error) // 로그에 숫자 대신 이름("Network")이 찍히게 한다(moc가 이름 표를 만든다)

    explicit CsvDownloader(const QString &directory, QObject *parent = nullptr);
    ~CsvDownloader() override;

    // CacheLocation/pokeapi-csv/<커밋>/ (Linux: ~/.cache/YamadaStudio/PokeSix/pokeapi-csv/<커밋>/)
    // 커밋을 폴더 이름에 넣어서, 커밋을 올리면 옛 파일과 섞이지 않는다.
    static QString defaultDirectory();

    // 파일 하나가 목록의 크기 · SHA-256과 일치하는가. 이어받기 판단과 테스트에 쓴다.
    static bool isValidCopy(const QString &path, qint64 size, std::string_view sha256);

    QString directory() const { return m_directory; }

    // 모든 파일이 이미 받아져 있고 검증을 통과하는가(네트워크를 쓰지 않는다).
    bool isComplete() const;

    void start();  // 이미 받는 중이면 무시한다
    void cancel(); // 받는 중인 파일을 버리고 failed(Cancelled)를 보낸다

signals:
    // filesDone / filesTotal: 끝난 파일 수, bytesDone / bytesTotal: 전체 바이트 기준 진행
    void progress(int filesDone, int filesTotal, qint64 bytesDone, qint64 bytesTotal,
                  const QString &currentFile);
    void finished();
    void failed(com::yamada::studio::CsvDownloader::Error error, const QString &detail);

private:
    void startNext();
    void onReadyRead();
    void onReplyFinished();
    void fail(Error error, const QString &detail);
    QString pathOf(const csvsource::CsvFile &file) const;
    qint64 bytesBefore(int index) const; // index번째 파일 앞까지의 누적 크기

    QString m_directory;
    QNetworkAccessManager *m_network = nullptr;
    QNetworkReply *m_reply = nullptr; // 지금 받는 중인 응답. 없으면 nullptr
    QSaveFile *m_file = nullptr;      // 지금 쓰는 중인 파일
    QCryptographicHash m_hash {QCryptographicHash::Sha256};
    int m_index = -1; // kFiles에서 지금 처리 중인 위치. -1 = 멈춤
};
} // namespace com::yamada::studio
