#pragma once

#include <QAbstractButton>
#include <QWidget>

namespace com::yamada::studio {
class GenerationButton : public QAbstractButton
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit GenerationButton(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
