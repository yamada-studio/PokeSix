#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QButtonGroup;
class QHBoxLayout;

namespace com::yamada::studio {
// 도감 상세의 기준 게임 칩: [BW] [B2W2] (그 세대의 본편 게임 묶음). 고르면 기술 · 기술머신 번호 ·
// 진화 조건 · 야생 출현이 그 게임 기준으로 바뀐다. 칩 모양은 스쿼드의 도감 칩과 같다(VersionChip).
class GameSelector : public QWidget
{
    Q_OBJECT
public:
    explicit GameSelector(QWidget *parent = nullptr);

    void setGames(const QList<GameInfo> &games, Language language, const QString &current);

signals:
    void gameSelected(const QString &versionGroup); // 사용자가 누를 때만

private:
    QButtonGroup *m_group = nullptr;
    QHBoxLayout *m_layout = nullptr;
    QList<GameInfo> m_games;
};
} // namespace com::yamada::studio
