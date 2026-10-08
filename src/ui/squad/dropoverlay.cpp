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
    // TODO(H2-CP8-1) 이벤트를 아래로 통과시키기: setAttribute(Qt::WA_TransparentForMouseEvents)
    setAttribute(Qt::WA_TransparentForMouseEvents);
    // TODO(H2-CP8-2) 처음엔 숨겨 둔다: hide()
    hide();
}

void DropOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // TODO(H2-CP8-3) 막: rect() 전체를 종이색 반투명으로 — QColor wash(tok::kPaper);
    //   wash.setAlpha(kWashAlpha); painter.fillRect(rect(), wash);
    QColor wash(tok::kPaper);
    wash.setAlpha(kWashAlpha);
    painter.fillRect(rect(), wash);

    // TODO(H2-CP8-4) 점선 테두리: QPen pen(QColor(tok::kBlue), kBorder, Qt::DashLine);
    //   painter.setPen(pen); painter.setBrush(Qt::NoBrush);
    //   painter.drawRoundedRect(QRectF(rect()).adjusted(...), kRadius, kRadius)
    //   — adjusted로 kInset만큼 안쪽으로. 펜 굵기의 절반(kBorder / 2.0)만큼 더 들이면 선이 잘리지
    //   않는다
    QPen pen(QColor(tok::kBlue), kBorder, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(QRectF(rect()).adjusted(kInset, kInset, -kInset, -kInset), kRadius,
                            kRadius);

    // TODO(H2-CP8-5) 글자 두 줄을 가운데에: 제목 tr("여기에 놓으면 불러와요")를
    //   theme::font(theme::kFamilyTitle, 28)로, 설명 tr("스쿼드 파일(.pks) · 게임 세이브(.sav ·
    //   .dsv)")를 theme::font(theme::kFamilyBody, 14)로. painter.drawText(사각형, Qt::AlignHCenter
    //   | Qt::AlignBottom / AlignTop, 글자) — 화면 세로 가운데 선을 기준으로 제목은 그 위, 설명은
    //   그 아래 사각형에
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
