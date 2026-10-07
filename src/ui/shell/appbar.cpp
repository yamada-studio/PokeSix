#include "ui/shell/appbar.h"

#include "ui/shell/apptabbar.h"
#include "ui/shell/markbutton.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/generationbutton.h"

#include <QColor>
#include <QHBoxLayout>
#include <QPainter>

namespace {
using namespace com::yamada::studio;

constexpr int kHeight = tok::kSizeAppBar; // 60 (아래 먹선 3 포함)
constexpr int kBottomLine = 3;
constexpr int kPaddingX = 20; // padding: 0 20px
// 오른쪽만 24: 본문 오른쪽 여백(20) + 열 오른쪽 여백(4) — 세대 버튼의 오른쪽 끝이 본문(스쿼드의
// "자동 저장됨" · 분석 창)과 같은 줄에 선다
constexpr int kPaddingRight = 24;
constexpr int kGap = 24; // 항목 사이 gap: 24px
constexpr int kMinGap = 12; // 탭 ↔ 세대 버튼 최소 간격(디자인은 1fr 빈칸이라 최소값이 없다)

// MarkButton은 hover 모양(안쪽 여백 3 · 8 + 점선 테 2 + 간격 2)이 들어갈 자리까지 위젯 크기에
// 넣었다 (위젯 밖에는 그릴 수 없다). 디자인의 평소 배치에는 그 자리가 없으므로(마크 링크는 padding
// 없음), 막대의 왼쪽 여백과 다음 간격에서 그만큼 뺀다 → 마크가 디자인과 같은 x=20에, 탭이 같은
// 자리에 선다.
constexpr int kMarkLeftExtra = 3 + 4;  // 왼쪽 padding 3 + 테 4
constexpr int kMarkRightExtra = 8 + 4; // 오른쪽 padding 8 + 테 4
} // namespace

namespace com::yamada::studio {
AppBar::AppBar(QWidget *parent)
    : QWidget(parent)
{
    QWidget::setFixedHeight(kHeight);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(kPaddingX - kMarkLeftExtra, 0, kPaddingRight, 0);
    layout->setSpacing(0); // 칸마다 addSpacing으로

    m_mark = new MarkButton;
    layout->addWidget(m_mark, 0, Qt::AlignVCenter);
    layout->addSpacing(kGap - kMarkRightExtra);

    // 탭은 막대의 아래 끝에 붙인다: 활성 탭(45)이 아래 먹선(3)을 덮어 본문과 이어져 보이게.
    m_tabs = new AppTabBar;
    layout->addWidget(m_tabs, 0, Qt::AlignBottom);
    // 탭과 세대 버튼 사이: 창이 가장 좁아도 12는 띄운다(없으면 둘이 맞붙는다). 남는 폭은 stretch가.
    layout->addSpacing(kMinGap);
    layout->addStretch();

    // 세대 버튼이 막대의 오른쪽 끝이다(검색 칸은 기능 없는 틀이라 뺐다 — 사용자 결정)
    m_generation = new GenerationButton(GenerationButton::Size::Compact);
    // 현재 세대 · 메뉴 범위는 MainWindow가 AppState와 연결한다(generationButton()).
    layout->addWidget(m_generation, 0, Qt::AlignVCenter);

    connect(m_mark, &MarkButton::clicked, this, &AppBar::homeRequested);
    connect(m_tabs, &AppTabBar::pageSelected, this,
            &AppBar::pageSelected); // 신호 → 신호로 그대로 전달
}

void AppBar::setCurrentPage(Page page)
{
    m_tabs->setCurrentPage(page);
}

void AppBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    // 빨강 바탕 + 아래 먹선 3. 활성 탭은 자식이라 이 위에 그려져 먹선을 덮는다(부모 → 자식 순서로
    // 그린다).
    QPainter painter(this);
    painter.fillRect(rect(), QColor(tok::kRed));
    painter.fillRect(0, height() - kBottomLine, width(), kBottomLine, QColor(tok::kInk));
}
} // namespace com::yamada::studio
