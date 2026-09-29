#include "ui/widgets/panelframe.h"

#include "ui/widgets/layoutguide.h"

#include <QVBoxLayout>

namespace {
// Chrome thickness: outline 3 + gap 8 + inner line 2, plus the 6px shadow at
// the bottom. Intro menu window values; other variants come with A5.
constexpr QMargins kChromeMargins = {13, 13, 13, 13 + 6};
} // namespace

namespace com::yamada::studio {
PanelFrame::PanelFrame(QWidget *parent)
    : QWidget(parent)
    , m_layout(new QVBoxLayout(this))
{
    m_layout->setContentsMargins(kChromeMargins);
    m_layout->setSpacing(0);
}

void PanelFrame::setBody(QWidget *body)
{
    m_layout->addWidget(body); // reparents body to this
}

void PanelFrame::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
