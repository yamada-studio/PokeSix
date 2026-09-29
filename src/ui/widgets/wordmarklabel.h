#pragma once

#include <QWidget>

namespace com::yamada::studio {
class WordmarkLabel : public QWidget
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit WordmarkLabel(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
