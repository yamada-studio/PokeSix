#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QButtonGroup;
class QHBoxLayout;

namespace com::yamada::studio {
// 게임 칩: [DP] [Pt] [HGSS] (그 세대의 본편 게임 묶음). 묶음 칩 하나에 버전 조각(HG | SS)이 있고,
// 조각을 눌러 버전을 고른다 — 고른 조각만 버전 색, 같은 칩의 다른 조각은 흐리다. 키보드로 이미 켠
// 칩을 다시 누르면 다음 버전으로 넘어간다. 칩 모양은 스쿼드의 도감 칩과 같다(VersionChip).
// setSplitVersions(true)면 묶음 대신 버전마다 칩 하나를 둔다.
class GameSelector : public QWidget
{
    Q_OBJECT
public:
    explicit GameSelector(QWidget *parent = nullptr);

    // current: 켤 버전("soulsilver")
    void setGames(const QList<GameInfo> &groups, Language language, const QString &current);
    // 버전마다 칩 하나([D] [P] [Pt] [HG] [SS]) — 스쿼드처럼 버전을 자주 오가는 화면. setGames 전에
    void setSplitVersions(bool split) { m_splitVersions = split; }

signals:
    void versionSelected(const QString &version); // 사용자가 누를 때만

private:
    void markCurrent(); // m_current가 든 칩을 켜고 그 조각을 고른다

    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QList<GameInfo> m_games; // 칩마다 하나(나눈 모드에서는 버전 하나짜리)
    QString m_current;
    bool m_splitVersions = false;
};
} // namespace com::yamada::studio
