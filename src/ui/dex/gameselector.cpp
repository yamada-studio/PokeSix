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
        auto *chip = static_cast<VersionChip *>(m_group->button(index)); // 칩만 넣는다
        if (chip == nullptr || chip->partCount() == 0)
            return;
        int part = chip->pressedPart();
        if (part < 0) // 키보드: 이미 켠 칩이면 다음 버전, 아니면 첫 버전
            part = chip->selectedPart() >= 0
                                   && chip->part(chip->selectedPart()).version == m_current
                           ? (chip->selectedPart() + 1) % chip->partCount()
                           : 0;
        const QString version = chip->part(part).version;
        if (version == m_current) {
            markCurrent(); // 같은 버전 — 켜진 상태만 되돌린다
            return;
        }
        m_current = version;
        markCurrent();
        emit versionSelected(version);
    });
}

void GameSelector::markCurrent()
{
    for (QAbstractButton *button : m_group->buttons()) {
        auto *chip = static_cast<VersionChip *>(button);
        int selected = -1;
        for (int i = 0; i < chip->partCount(); ++i)
            if (chip->part(i).version == m_current)
                selected = i;
        // 약칭이 같아 합쳐진 조각(관동 일본판 R · G)의 다른 버전도 그 칩으로 찾는다
        const int index = m_group->id(button);
        if (selected < 0 && index >= 0 && index < m_games.size()
            && m_games.at(index).versions.contains(m_current))
            selected = 0;
        chip->setChecked(selected >= 0);
        chip->setSelectedPart(selected);
    }
}

void GameSelector::setGames(const QList<GameInfo> &groups, Language language,
                            const QString &current)
{
    // 버전마다 칩 하나: 묶음을 버전 하나짜리 게임들로 편다(HGSS → HG · SS)
    QList<GameInfo> games;
    for (const GameInfo &group : groups) {
        if (!m_splitVersions) {
            games.append(group);
            continue;
        }
        for (qsizetype i = 0; i < group.versions.size(); ++i)
            games.append(GameInfo {
                    group.versionGroup, {group.versions.at(i)}, {group.versionNames.value(i)}});
    }
    // 같은 게임 목록이면 버튼은 그대로 두고 켤 칩 · 툴팁(언어)만 바꾼다. 칩을 눌러 이 함수가 불리는
    // 경우(→ 다시 읽기 → 다시 그리기) 지금 눌린 버튼을 그 클릭 처리 도중에 지우면 안 된다
    bool same = games.size() == m_games.size() && !m_group->buttons().isEmpty();
    for (qsizetype i = 0; same && i < games.size(); ++i)
        same = games.at(i).versionGroup == m_games.at(i).versionGroup
               && games.at(i).versions == m_games.at(i).versions;
    m_current = current;
    if (same) {
        m_games = games;
        for (qsizetype i = 0; i < games.size(); ++i) {
            QAbstractButton *chip = m_group->button(int(i));
            QStringList names;
            for (const LocalizedText &name : games.at(i).versionNames)
                names.append(name.text(language));
            chip->setToolTip(names.join(QStringLiteral(" · ")));
        }
        markCurrent();
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
        if (keys.mid(0, i).contains(keys.at(i))) // 같은 약칭이 앞에 이미 있을 때만
            suffix = !label.isEmpty() ? label.text(language) : names.value(0);
        VersionChip *chip = new VersionChip(parts.at(i), suffix);
        chip->setToolTip(names.join(QStringLiteral(" · ")));
        m_group->addButton(chip, int(i));
        m_layout->addWidget(chip);
    }
    markCurrent();
    QWidget::updateGeometry();
}
} // namespace com::yamada::studio
