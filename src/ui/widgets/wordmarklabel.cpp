#include "ui/widgets/wordmarklabel.h"

#include "ui/widgets/layoutguide.h"

namespace com::yamada::studio {
WordmarkLabel::WordmarkLabel(QWidget *parent)
    : QWidget(parent)
{
}

void WordmarkLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
