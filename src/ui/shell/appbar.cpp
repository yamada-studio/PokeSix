#include "ui/shell/appbar.h"

#include "ui/widgets/layoutguide.h"

#include <QPainter>

namespace com::yamada::studio {
constexpr int kHeight = 60;

AppBar::AppBar(QWidget *parent)
    : QWidget(parent)
{
    setFixedHeight(kHeight);
}

void AppBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
