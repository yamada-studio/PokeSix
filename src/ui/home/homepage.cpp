#include "ui/home/homepage.h"

#include "ui/home/intromenu.h"
#include "ui/widgets/generationbutton.h"
#include "ui/widgets/layoutguide.h"
#include "ui/widgets/markwidget.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/wordmarklabel.h"

#include <QLabel>
#include <QVBoxLayout>

namespace {
constexpr QMargins kPageMargins = {32, 44, 32, 26}; // left, top, right, bottom
constexpr QSize kMarkSize = {136, 136};
constexpr QSize kWordmarkSize = {460, 85};
constexpr QSize kGenerationButtonSize = {162, 49};
constexpr int kMenuWidth = 520;
} // namespace

namespace com::yamada::studio {

HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this); // installs itself on this
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(0); // no style default spacing (~6px)

    MarkWidget *mark = new MarkWidget;
    mark->setFixedSize(kMarkSize);
    layout->addWidget(mark, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    WordmarkLabel *wordmark = new WordmarkLabel;
    wordmark->setFixedSize(kWordmarkSize);
    layout->addWidget(wordmark, 0, Qt::AlignHCenter);
    layout->addSpacing(11);

    QLabel *subtitleLabel = new QLabel(tr("도감 · 파티 도우미"));
    layout->addWidget(subtitleLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(24);

    QLabel *generationLabel = new QLabel(tr("세대를 선택하여 시작하세요:"));
    layout->addWidget(generationLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(6);

    GenerationButton *generationButton = new GenerationButton;
    generationButton->setFixedSize(kGenerationButtonSize);
    layout->addWidget(generationButton, 0, Qt::AlignHCenter);
    layout->addSpacing(21);

    PanelFrame *menuFrame = new PanelFrame;
    menuFrame->setFixedWidth(kMenuWidth); // the width belongs to the caller
    menuFrame->setBody(new IntroMenu);
    layout->addWidget(menuFrame, 0, Qt::AlignHCenter);

    layout->addStretch();
}

void HomePage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintLayoutGuide(*this);
}
} // namespace com::yamada::studio
