#pragma once

#include <QWidget>

namespace com::yamada::studio {
class HomePage : public QWidget
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit HomePage(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
