#include "ui/widgets/generationbutton.h"

#include "ui/widgets/layoutguide.h"

namespace com::yamada::studio {
GenerationButton::GenerationButton(QWidget *parent)
    : QAbstractButton(parent)
{
}

void GenerationButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
