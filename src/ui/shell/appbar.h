#pragma once

#include <QWidget>

namespace com::yamada::studio {
class AppBar : public QWidget
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit AppBar(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
