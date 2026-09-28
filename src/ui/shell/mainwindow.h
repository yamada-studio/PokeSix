#pragma once

#include <QMainWindow>

namespace com::yamada::studio {
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
