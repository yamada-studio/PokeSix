#pragma once

#include <QAbstractButton>

namespace com::yamada::studio {
class IntroMenuItem : public QAbstractButton
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit IntroMenuItem(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
