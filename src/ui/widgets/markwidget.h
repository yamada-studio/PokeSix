#pragma once

#include <QWidget>

namespace com::yamada::studio {
class MarkWidget : public QWidget
{
    Q_OBJECT
protected:
    void paintEvent(QPaintEvent *event) override;

public:
    explicit MarkWidget(QWidget *parent = nullptr);
};
} // namespace com::yamada::studio
