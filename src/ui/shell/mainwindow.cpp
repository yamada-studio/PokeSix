#include "ui/shell/mainwindow.h"

#include "ui/home/homepage.h"
#include "ui/logging/logging.h"
#include "ui/shell/appbar.h"

#include <QApplication>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace com::yamada::studio {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QMainWindow::setWindowTitle(
            QStringLiteral("PokeSix %1").arg(QApplication::applicationVersion()));
    QMainWindow::setMinimumSize(960, 640);
    QMainWindow::resize(1440, 900);

    // m_screens (central)
    //  ├ [0] HomePage
    //  └ [1] shell
    //         └ QVBoxLayout (margins 0, spacing 0)
    //            ├ AppBar    fixed 60
    //            └ m_pages
    //               └ (placeholder) QWidget
    m_screens = new QStackedWidget;
    m_screens->addWidget(new HomePage);

    QWidget *shell = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(shell); // installs itself on shell
    layout->setContentsMargins(0, 0, 0, 0);       // no style default margins (~9-11px)
    layout->setSpacing(0);                        // no style default spacing (~6px)

    m_pages = new QStackedWidget;
    m_pages->addWidget(new QWidget);

    layout->addWidget(new AppBar);
    layout->addWidget(m_pages);

    m_screens->addWidget(shell);

    QMainWindow::setCentralWidget(m_screens);

    qCInfo(lcUi) << "MainWindow initialized";
}
} // namespace com::yamada::studio
