#include "ui/dex/gameselector.h"

#include "ui/theme/dexstyle.h"
#include "ui/widgets/versionchip.h"

#include <QButtonGroup>
#include <QHBoxLayout>

namespace com::yamada::studio {
GameSelector::GameSelector(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, [this](int index) {
        if (index >= 0 && index < m_games.size())
            emit gameSelected(m_games.at(index).versionGroup);
    });
}

void GameSelector::setGames(const QList<GameInfo> &games, Language language, const QString &current)
{
    // 같은 게임 목록이면 버튼은 그대로 두고 켤 칩 · 툴팁(언어)만 바꾼다. 칩을 눌러 이 함수가 불리는
    // 경우(→ 다시 읽기 → 다시 그리기) 지금 눌린 버튼을 그 클릭 처리 도중에 지우면 안 된다
    bool same = games.size() == m_games.size() && !m_group->buttons().isEmpty();
    for (qsizetype i = 0; same && i < games.size(); ++i)
        same = games.at(i).versionGroup == m_games.at(i).versionGroup;
    if (same) {
        m_games = games;
        for (qsizetype i = 0; i < games.size(); ++i) {
            QAbstractButton *chip = m_group->button(int(i));
            QStringList names;
            for (const LocalizedText &name : games.at(i).versionNames)
                names.append(name.text(language));
            chip->setToolTip(names.join(QStringLiteral(" · ")));
            chip->setChecked(games.at(i).versionGroup == current);
        }
        return;
    }
    for (QAbstractButton *button : m_group->buttons()) {
        m_group->removeButton(button);
        m_layout->removeWidget(button);
        button->hide();
        button->deleteLater(); // 신호 처리 중일 수 있다 → 이벤트 루프로 돌아간 뒤 지운다
    }
    m_games = games;
    // 약칭이 겹치는 게임(소드 · 실드 본편과 DLC: 모두 SwSh)은 dexstyle의 짧은 이름(외딴섬 · 설원)을
    // 뒤에 붙여 가른다. 짧은 이름이 없으면 첫 버전 이름
    QList<QList<VersionChip::Part>> parts;
    QStringList keys;
    for (const GameInfo &game : games) {
        parts.append(VersionChip::partsFor(game.versions, game.versionNames));
        keys.append(VersionChip::keyOf(parts.last()));
    }
    for (qsizetype i = 0; i < games.size(); ++i) {
        const GameInfo &game = games.at(i);
        QStringList names;
        for (const LocalizedText &name : game.versionNames)
            names.append(name.text(language));
        const LocalizedText label = dexstyle::groupLabel(game.versionGroup);
        QString suffix;
        if (keys.count(keys.at(i)) > 1 && i > 0)
            suffix = !label.isEmpty() ? label.text(language) : names.value(0);
        VersionChip *chip = new VersionChip(parts.at(i), suffix);
        chip->setToolTip(names.join(QStringLiteral(" · ")));
        m_group->addButton(chip, int(i));
        m_layout->addWidget(chip);
        chip->setChecked(game.versionGroup == current);
    }
    QWidget::updateGeometry();
}
} // namespace com::yamada::studio
