#pragma once

#include <QWidget>

namespace com::yamada::studio {
// Content of the intro menu window: four menu rows, a divider gap and the
// quit row. Selection and keyboard handling come in A7.
class IntroMenu : public QWidget
{
    Q_OBJECT
public:
    explicit IntroMenu(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
} // namespace com::yamada::studio
