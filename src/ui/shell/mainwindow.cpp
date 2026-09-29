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

    // central
    //  └ QVBoxLayout (margins 0, spacing 0)
    //     ├ AppBar           fixed 60
    //     └ QStackedWidget   the rest
    //        └ HomePage      page 0
    QWidget *central = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(central); // installs itself on central
    layout->setContentsMargins(0, 0, 0, 0);         // no style default margins (~9-11px)
    layout->setSpacing(0);                          // no style default spacing (~6px)

    QStackedWidget *pages = new QStackedWidget;
    pages->addWidget(new HomePage); // reparents HomePage to pages

    layout->addWidget(new AppBar); // reparents AppBar to central
    layout->addWidget(pages);

    QMainWindow::setCentralWidget(central); // MainWindow takes ownership of central

    m_pages = pages; // keep a pointer to the QStackedWidget for later use

    qCInfo(lcUi) << "MainWindow initialized";
}
} // namespace com::yamada::studio
