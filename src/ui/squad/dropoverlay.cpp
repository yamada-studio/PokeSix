#include "ui/squad/dropoverlay.h"

#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QPainter>

namespace com::yamada::studio {
namespace {
constexpr int kInset = 16;  // 페이지 가장자리 ↔ 점선 테두리
constexpr int kBorder = 3;  // 점선 굵기
constexpr int kRadius = 12; // 테두리 둥글기
constexpr int kWashAlpha = 215; // 종이색 막의 불투명도(0–255) — 뒤의 카드가 흐리게 비친다
} // namespace

DropOverlay::DropOverlay(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    hide();
}

void DropOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor wash(tok::kPaper);
    wash.setAlpha(kWashAlpha);
    painter.fillRect(rect(), wash);

    QPen pen(QColor(tok::kBlue), kBorder, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(kInset, kInset, -kInset, -kInset), kRadius,
                            kRadius);

    const int half = height() / 2;
    const QRect top = rect().adjusted(0, 0, 0, -half);
    const QRect bottom = rect().adjusted(0, half, 0, 0);
    painter.setPen(QColor(tok::kText1));
    painter.setFont(theme::font(theme::kFamilyTitle, 28));
    painter.drawText(top, Qt::AlignHCenter | Qt::AlignBottom, tr("여기에 놓으면 불러와요"));

    painter.setPen(QColor(tok::kText2));
    painter.setFont(theme::font(theme::kFamilyBody, 14));
    painter.drawText(bottom, Qt::AlignHCenter | Qt::AlignTop,
                     tr("스쿼드 파일(.pks) · 게임 세이브(.sav · .dsv)"));
}
} // namespace com::yamada::studio
