#include "ui/dex/dexselector.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QPushButton>

#include <utility>

namespace {
constexpr int kSpacing = 6; // 버튼 사이
// 버튼 높이: 빨강 띠 38 안에 위아래 6씩. 고정하지 않으면 글자(" · " 등 대체 글꼴)에 따라 1px씩
// 달라진다.
constexpr int kButtonHeight = 26;

// 영어 버전 이름 → 약칭. 약칭은 PokéAPI에 없어서 여기 둔다(타입 색 표와 같은 "보여 주기 규칙").
// 표에 없으면 영어 이름 그대로 — 새 게임이 추가돼도 깨지지 않고 조금 길어질 뿐이다.
// TODO(A8): 세대 전환이 생기면 다른 세대의 약칭도 채운다.
constexpr std::pair<const char *, const char *> kVersionShort[] = {
        {"Diamond", "D"},    {"Pearl", "P"},       {"Platinum", "Pt"},
        {"HeartGold", "HG"}, {"SoulSilver", "SS"},
};

QString shortName(const QString &english)
{
    for (const auto &[name, abbreviation] : kVersionShort)
        if (english == QLatin1String(name))
            return QString::fromLatin1(abbreviation);
    return english;
}
} // namespace

namespace com::yamada::studio {
DexSelector::DexSelector(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0); // 스타일 기본 여백 없이(A2)
    m_layout->setSpacing(kSpacing);
    m_group->setExclusive(true); // 하나를 켜면 나머지는 저절로 꺼진다
    // idClicked: 사용자가 누를 때만 나온다. setDexes()가 setChecked로 전국을 켤 때는 나오지 않는다.
    connect(m_group, &QButtonGroup::idClicked, this, &DexSelector::dexSelected);
    setDexes({});
}

void DexSelector::setDexes(const QList<DexInfo> &dexes)
{
    // 1) 기존 버튼 지우기. delete하면 레이아웃과 버튼 그룹에서도 저절로 빠지지만(소멸자가 알린다),
    //    그룹에서 먼저 빼 두면 순서에 기대지 않아도 된다.
    const QList<QAbstractButton *> old = m_group->buttons();
    for (QAbstractButton *button : old) {
        m_group->removeButton(button);
        delete button;
    }

    // 2) [전국] + 지방 도감. 글자는 "신오 D · P", 툴팁은 한국어 버전 이름 "디아루가 · 펄기아".
    addButton(tr("전국"), tr("그 세대까지 나온 포켓몬 전부"), kNational);
    const QString separator = QStringLiteral(" · ");
    for (const DexInfo &dex : dexes) {
        QStringList shorts;
        for (const QString &version : dex.versionsEn)
            shorts.append(shortName(version));
        addButton(dex.regionKo + QLatin1Char(' ') + shorts.join(separator),
                  tr("%1도감 — %2").arg(dex.regionKo, dex.versionsKo.join(separator)),
                  dex.pokedexId);
    }

    // 3) 전국을 켠다. 4) sizeHint가 바뀌었다고 알린다 → 부모(PanelFrame)가 자리를 다시 잡는다.
    m_group->button(kNational)->setChecked(true);
    QWidget::updateGeometry();
}

void DexSelector::addButton(const QString &text, const QString &toolTip, int id)
{
    QPushButton *button = new QPushButton(text);
    button->setObjectName(QStringLiteral("dexButton")); // app.qss
    button->setCheckable(true);
    button->setFixedHeight(kButtonHeight);
    button->setToolTip(toolTip);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::TabFocus); // 마우스로 눌러도 포커스를 가져가지 않는다(검색 칸 유지)
    m_layout->addWidget(button);
    m_group->addButton(button, id);
}
} // namespace com::yamada::studio
