#include "ui/home/intromenuitem.h"

#include "ui/widgets/layoutguide.h"

namespace com::yamada::studio {
IntroMenuItem::IntroMenuItem(QWidget *parent)
    : QAbstractButton(parent)
{
}

void IntroMenuItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
