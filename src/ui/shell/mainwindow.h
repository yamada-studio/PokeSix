#pragma once

#include <QMainWindow>

class QStackedWidget;

namespace com::yamada::studio {
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QStackedWidget *m_pages = nullptr;
    QStackedWidget *m_screens = nullptr;
};
} // namespace com::yamada::studio
