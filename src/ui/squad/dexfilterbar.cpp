#include "ui/squad/dexfilterbar.h"

#include "ui/theme/dexstyle.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/versionchip.h"

#include <QButtonGroup>
#include <QHBoxLayout>

namespace com::yamada::studio {
DexFilterBar::DexFilterBar(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, &DexFilterBar::dexSelected);
}

int DexFilterBar::currentDex() const
{
    return std::max(kNational, m_group->checkedId());
}

void DexFilterBar::setDexes(const QList<DexInfo> &dexes, Language language, int checkedId)
{
    for (QAbstractButton *button : m_group->buttons()) {
        m_group->removeButton(button);
        delete button;
    }
    auto add = [this](VersionChip *chip, const QString &toolTip, int id) {
        chip->setToolTip(toolTip);
        m_group->addButton(chip, id);
        m_layout->addWidget(chip);
    };
    add(new VersionChip({{tr("전국"), QColor(tok::kWhite), QColor(tok::kInk)}}, {}),
        tr("그 세대까지 나온 포켓몬 전부"), kNational);

    // 칩 글자(버전 약칭 이어 붙임)가 겹치는 도감(칼로스 센트럴 · 코스트 · 마운틴 = 모두 XY)은
    // dexstyle의 이름을 뒤에 붙인다
    QList<DexInfo> shown;
    QList<QList<VersionChip::Part>> parts;
    QStringList keys;
    for (const DexInfo &dex : dexes) {
        if (dexstyle::dex(dex.identifier).hidden)
            continue;
        shown.append(dex);
        parts.append(VersionChip::partsFor(dex.versions, dex.versionNames));
        keys.append(VersionChip::keyOf(parts.last()));
    }
    for (qsizetype i = 0; i < shown.size(); ++i) {
        const DexInfo &dex = shown.at(i);
        const dexstyle::DexStyle rule = dexstyle::dex(dex.identifier);
        const QString region
                = rule.label.isEmpty() ? dex.region.text(language) : rule.label.text(language);
        const QString suffix = keys.count(keys.at(i)) > 1 ? region : QString();
        QStringList names;
        for (const LocalizedText &name : dex.versionNames)
            names.append(name.text(language));
        add(new VersionChip(parts.at(i), suffix),
            tr("%1 도감 — %2").arg(region, names.join(QStringLiteral(" · "))), dex.pokedexId);
    }
    QAbstractButton *checked = m_group->button(checkedId);
    (checked ? checked : m_group->button(kNational))->setChecked(true);
    QWidget::updateGeometry();
}
} // namespace com::yamada::studio
