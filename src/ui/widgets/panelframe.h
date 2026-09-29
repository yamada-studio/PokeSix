#pragma once

#include <QWidget>

class QVBoxLayout;

namespace com::yamada::studio {
// The chrome of a panel window (outline, header, shadow). The content widget
// passed to setBody() does not know it sits inside a panel (ADR 0007).
class PanelFrame : public QWidget
{
    Q_OBJECT
public:
    explicit PanelFrame(QWidget *parent = nullptr);

    // Call once. PanelFrame takes ownership of body.
    void setBody(QWidget *body);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVBoxLayout *m_layout = nullptr;
};
} // namespace com::yamada::studio
