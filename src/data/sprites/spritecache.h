#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class QNetworkAccessManager;
class QNetworkReply;

namespace com::yamada::studio {
// 작은 그림(포켓몬 아이콘 · 아이템 아이콘) 파일 캐시.
//
// 스프라이트는 게임 에셋이라 리포에 커밋하지 않는다(CLAUDE.md §7). 그래서 CSV와 같은 방식으로
// **실행 중에 PokéAPI/sprites 저장소에서 받아 사용자 캐시 폴더에** 둔다:
//   ~/.cache/YamadaStudio/PokeSix/sprites/icons/<pokemonId>.png     (Kind::PokemonIcon)
//   ~/.cache/YamadaStudio/PokeSix/sprites/items/[gen5/]<identifier>.png  (Kind::Item)
// 한 번 받은 파일은 다시 받지 않는다 → 두 번째 실행부터는 오프라인.
//
// data 계층은 QtGui를 모른다(QPixmap 없음). 그래서 이 클래스는 "파일 경로"만 다루고, 그림으로
// 읽는 건 ui(delegate)가 한다. 흐름:
//   delegate: path(key) → 비어 있으면 request(key) → (비동기) ready(key) → 뷰 다시 그리기 →
//   path(key)
// key는 Kind마다 다르다: 포켓몬 = pokemon id 숫자("445"), 아이템 = identifier("fire-stone").
// 아이템은 그림 모양 폴더를 앞에 붙일 수 있다("gen5/fire-stone" — 5세대 그림, 없으면 기본 그림).
class SpriteCache : public QObject
{
    Q_OBJECT
public:
    enum class Kind {
        PokemonIcon, // 8세대 박스 아이콘(68×56), 없으면 정면 스프라이트(96×96)
        Item,        // 아이템 아이콘(30×30). 기술머신은 타입별 CD("tm-fire")
    };

    explicit SpriteCache(Kind kind, QObject *parent = nullptr);

    // 받아 둔 파일이 있으면 그 경로, 없으면 빈 문자열. 디스크 확인 결과는 기억해 둔다(그릴 때마다
    // stat하지 않게).
    QString path(const QString &key);
    // 아직 없으면 받기 시작한다. 이미 받는 중이거나 실패한 key는 무시한다(같은 세션에서 재시도 안
    // 함).
    void request(const QString &key);

signals:
    void ready(const QString &key);

private:
    void fetch(const QString &key, int attempt);
    void onFinished(QNetworkReply *reply, const QString &key, int attempt);
    QString filePath(const QString &key) const;
    QString urlOf(const QString &key, int attempt) const;

    QStringList m_sources; // URL 틀(%1 = key). 앞에서부터 시도한다
    QString m_dir;
    QNetworkAccessManager *m_network = nullptr; // 호스트당 동시 연결 6개로 알아서 줄 세운다
    QHash<QString, QString> m_known;            // 있는 파일
    QSet<QString> m_pending;                    // 받는 중
    QSet<QString> m_failed;                     // 이번 실행에서 실패
};
} // namespace com::yamada::studio
