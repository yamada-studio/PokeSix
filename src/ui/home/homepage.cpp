#include "ui/home/homepage.h"

#include "ui/widgets/layoutguide.h"

#include <QPainter>

namespace com::yamada::studio {
HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
}

void HomePage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
