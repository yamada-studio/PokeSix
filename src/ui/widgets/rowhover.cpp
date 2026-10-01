#include "ui/widgets/rowhover.h"

#include "ui/theme/cursors.h"

#include <QAbstractItemView>
#include <QMouseEvent>

namespace {
constexpr int kFrameMs = 110; // 한 장의 길이
constexpr int kFrames = 4;    // 쥠 → 폄 → 쥠 → 폄 (두 번 꾹꾹)
} // namespace

namespace com::yamada::studio {
RowHover::RowHover(QAbstractItemView *view)
    : QObject(view)
    , m_view(view)
{
    // 마우스 버튼을 누르지 않아도 움직임(MouseMove)을 받으려면 추적을 켜야 한다
    m_view->setMouseTracking(true);
    m_view->viewport()->installEventFilter(this);
    m_animation.setInterval(kFrameMs);
    connect(&m_animation, &QTimer::timeout, this, &RowHover::nextFrame);
}

bool RowHover::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_view->viewport()) {
        if (event->type() == QEvent::MouseMove) {
            const QPoint pos = static_cast<QMouseEvent *>(event)->position().toPoint();
            setRow(m_view->indexAt(pos).row()); // 빈 곳이면 −1
        } else if (event->type() == QEvent::Leave) {
            setRow(-1);
        }
    }
    return QObject::eventFilter(watched, event); // 엿보기만 — 이벤트는 뷰가 그대로 받는다
}

void RowHover::setRow(int row)
{
    if (row == m_row)
        return;
    m_row = row;
    m_view->viewport()->update(); // 강조할 줄이 바뀌었다 → 다시 그리기 예약
    if (row < 0) {
        m_animation.stop();
        m_view->viewport()->unsetCursor();
        return;
    }
    // 새 줄에 올라갔다: 꾹꾹(쥠 · 폄 두 번) 하고 가리키기 장갑으로 쉰다
    m_frame = 0;
    m_view->viewport()->setCursor(cursors::grab());
    m_animation.start();
}

void RowHover::nextFrame()
{
    ++m_frame;
    if (m_frame >= kFrames) {
        m_animation.stop();
        m_view->viewport()->setCursor(cursors::pointer());
        return;
    }
    m_view->viewport()->setCursor(m_frame % 2 == 0 ? cursors::grab() : cursors::pointer());
}
} // namespace com::yamada::studio
