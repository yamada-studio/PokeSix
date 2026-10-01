#pragma once

#include "data/store/squad.h"

#include <QHash>
#include <QObject>
#include <QTimer>

namespace com::yamada::studio {
// 스쿼드 저장소: AppDataLocation/squads.json (Linux: ~/.local/share/YamadaStudio/PokeSix/).
//
// setSquad()는 메모리만 바꾸고 저장은 잠시 뒤에 한 번 한다(디바운스) — 메모를 한 글자씩 칠 때마다
// 파일을 쓰지 않는다. 저장은 QSaveFile(임시 파일에 다 쓴 뒤 바꿔치기)이라 쓰다가 꺼져도 옛 파일이
// 남는다. 창을 닫을 때는 flush()로 기다리지 않고 바로 쓴다.
//
// 파일 꼴(version 2): 세대 → 게임 묶음 → 스쿼드, 그리고 세대마다 마지막에 본 게임
//   { "version": 2, "squads": { "4": { "current": "platinum", "games": {
//       "platinum": { "name": "…", "members": [ { "pokemon": 445, "memo": "…",
//                     "moves": [89, 200, 444, 14], "ability": 8, "nature": 0, "item": 0 }, … 6개 ]
//                     },
//       "heartgold-soulsilver": { … } } } } }
// version 1(세대마다 스쿼드 하나 + "versionGroup")은 읽을 때 그 게임의 스쿼드로 옮긴다.
class SquadStore : public QObject
{
    Q_OBJECT
public:
    // path가 비어 있으면 기본 위치. 테스트는 임시 폴더를 넘긴다
    explicit SquadStore(const QString &path = QString(), QObject *parent = nullptr);
    ~SquadStore() override; // 남은 저장을 마친다

    static constexpr int kSaveDelayMs = 600;

    // 그 게임의 스쿼드(없으면 빈 스쿼드 — 이름은 비어 있다)
    Squad squad(int generation, const QString &versionGroup) const;
    // 같은 값이면 아무것도 하지 않는다
    void setSquad(int generation, const QString &versionGroup, const Squad &squad);
    // 세대마다 마지막에 본 게임(없으면 빈 칸 → 세션이 대표 게임을 고른다)
    QString currentGame(int generation) const { return m_current.value(generation); }
    void setCurrentGame(int generation, const QString &versionGroup);

    bool flush(); // 기다리는 저장을 지금 한다. 실패하면 false
    bool hasPendingSave() const { return m_timer.isActive(); }
    QString path() const { return m_path; }

signals:
    void saveScheduled(); // "저장 중…"
    void saved(bool ok);  // "✓ 자동 저장됨" / 실패

private:
    void load();
    bool write();

    QString m_path;
    QHash<int, QHash<QString, Squad>> m_squads; // 세대 → 게임 → 스쿼드
    QHash<int, QString> m_current;              // 세대 → 마지막에 본 게임
    void schedule();
    QTimer m_timer;
};
} // namespace com::yamada::studio
