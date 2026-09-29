#include "ui/home/intromenu.h"

#include "ui/home/intromenuitem.h"
#include "ui/widgets/layoutguide.h"

#include <QVBoxLayout>

namespace {
constexpr QMargins kMenuMargins = {6, 6, 6, 6};
constexpr int kItemSpacing = 2;
constexpr int kItemHeight = 58;
constexpr int kQuitHeight = 42;
// CSS: gap 2 + divider (margin 4 + line 2 + margin 4) + gap 2 = 14 between the
// last row and the quit row. QBoxLayout adds kItemSpacing only once around a
// spacer item, so the spacer itself is 14 - 2.
constexpr int kDividerSpacing = 14 - kItemSpacing;
} // namespace

namespace com::yamada::studio {
IntroMenu::IntroMenu(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kMenuMargins);
    layout->setSpacing(kItemSpacing);

    for (const QString &name : {tr("도감 백과"), tr("아이템 백과"), tr("SixSquad"), tr("설정")}) {
        IntroMenuItem *item = new IntroMenuItem;
        item->setText(name);
        item->setFixedHeight(kItemHeight);
        layout->addWidget(item);
    }

    layout->addSpacing(kDividerSpacing);

    IntroMenuItem *quit = new IntroMenuItem;
    quit->setText(tr("종료"));
    quit->setFixedHeight(kQuitHeight);
    layout->addWidget(quit);
}

void IntroMenu::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
