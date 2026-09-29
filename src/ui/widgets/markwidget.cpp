#include "ui/widgets/markwidget.h"

#include "ui/widgets/layoutguide.h"

namespace com::yamada::studio {

MarkWidget::MarkWidget(QWidget *parent)
    : QWidget(parent)
{
}

void MarkWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}

} // namespace com::yamada::studio
