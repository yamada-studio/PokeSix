#include "ui/shell/mainwindow.h"

#include "ui/logging/logging.h"

#include <QApplication>

namespace com::yamada::studio {
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QMainWindow::setWindowTitle(
            QStringLiteral("PokeSix %1").arg(QApplication::applicationVersion()));
    QMainWindow::setMinimumSize(960, 640);
    QMainWindow::resize(1440, 900);

    qCInfo(lcUi) << "MainWindow initialized";
}
} // namespace com::yamada::studio
