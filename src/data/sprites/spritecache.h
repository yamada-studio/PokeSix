#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>

class QNetworkAccessManager;
class QNetworkReply;

namespace com::yamada::studio {
// 포켓몬 아이콘(작은 스프라이트) 파일 캐시.
//
// 스프라이트는 게임 에셋이라 리포에 커밋하지 않는다(CLAUDE.md §7). 그래서 CSV와 같은 방식으로
// **실행 중에 PokéAPI/sprites 저장소에서 받아 사용자 캐시 폴더에** 둔다:
//   ~/.cache/YamadaStudio/PokeSix/sprites/icons/<pokemonId>.png   (Linux 기준)
// 한 번 받은 파일은 다시 받지 않는다 → 두 번째 실행부터는 오프라인.
//
// data 계층은 QtGui를 모른다(QPixmap 없음). 그래서 이 클래스는 "파일 경로"만 다루고, 그림으로
// 읽는 건 ui(delegate)가 한다. 흐름:
//   delegate: path(id) → 비어 있으면 request(id) → (비동기) ready(id) → 뷰 다시 그리기 → path(id)
class SpriteCache : public QObject
{
    Q_OBJECT
public:
    explicit SpriteCache(QObject *parent = nullptr);

    // 받아 둔 파일이 있으면 그 경로, 없으면 빈 문자열. 디스크 확인 결과는 기억해 둔다(그릴 때마다
    // stat하지 않게).
    QString path(int pokemonId);
    // 아직 없으면 받기 시작한다. 이미 받는 중이거나 실패한 id는 무시한다(같은 세션에서 재시도 안
    // 함).
    void request(int pokemonId);

signals:
    void ready(int pokemonId);

private:
    void fetch(int pokemonId, int attempt);
    void onFinished(QNetworkReply *reply, int pokemonId, int attempt);
    QString filePath(int pokemonId) const;

    QString m_dir;
    QNetworkAccessManager *m_network = nullptr; // 호스트당 동시 연결 6개로 알아서 줄 세운다
    QHash<int, QString> m_known;                // 있는 파일
    QSet<int> m_pending;                        // 받는 중
    QSet<int> m_failed;                         // 이번 실행에서 실패
};
} // namespace com::yamada::studio
