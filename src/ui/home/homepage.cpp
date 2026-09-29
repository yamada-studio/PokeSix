#include "ui/home/homepage.h"

#include "ui/home/introfooter.h"
#include "ui/home/intromenu.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/generationbutton.h"
#include "ui/widgets/markwidget.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/wordmarklabel.h"

#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtMath>

namespace {
using namespace com::yamada::studio;

// ── 배치 (Intro.dc.html, 1440×900) ────────────────────────────────────────────
// 세로는 위 여백 44에서 시작해 차례로 쌓고, 정보 줄만 바닥(아래 26)에 붙는다.
// 오프셋 그림자는 위젯 rect 안에 들어 있으므로(ADR 0007) 그 아래 간격에서 그림자만큼 뺀다.
constexpr QMargins kPageMargins
        = {32, 44, 32, 26};        // 좌 · 위 · 우 · 아래 (푸터 left/right 32, bottom 26)
constexpr int kMarkToWordmark = 6; // h1 margin-top: 6
constexpr int kWordmarkToSubtitle = 16 - 5; // p margin-top: 16 − 워드마크 글자 그림자 5
constexpr int kSubtitleToGeneration = 24;   // 세대 블록 margin-top: 24
constexpr int kGenerationLabelToButton = 6; // 세대 블록 gap: 6
constexpr int kGenerationToMenu = 24 - 3;   // nav margin-top: 24 − 세대 버튼 그림자 3
constexpr int kMenuWidth = tok::kSizeIntroMenuWidth; // 520
// 글자 줄의 높이. CSS의 line-height: normal은 브라우저가 글꼴 파일의 줄 간격 값으로 정하는데,
// Qt의 QLabel은 다른 값(QFontMetrics::height)을 쓴다. 도현 22는 Qt가 32로 잡아서 아래가 전부
// 밀리므로 기준 이미지(30_intro_1440.png)에서 잰 브라우저 값으로 고정한다.
constexpr int kSubtitleLine = 26;        // 도현 22
constexpr int kGenerationLabelLine = 14; // 나눔고딕 12

// 메뉴 창의 겉모양: 먹선 3 · 반경 12 · 그림자 6, 안쪽 8 떨어진 곳에 line 색 2px 이중 테(반경 8).
// PanelStyle에는 QRgb만 들어 있으므로 constexpr로 만들 수 있다.
constexpr PanelStyle kMenuWindow {
        .outline = 3,
        .radius = 12,
        .shadow = 6,
        .fill = tok::kWhite,
        .ink = tok::kInk,
        .innerInset = 8,
        .innerLine = 2,
        .innerRadius = 8,
        .innerColor = tok::kLine,
};

// ── 바탕 (02b SCR-01 #0) ──────────────────────────────────────────────────────
// CSS: repeating-linear-gradient(135deg, paper 0–26px, intro.stripe 26–52px)
//   135deg = 오른쪽 아래 방향. 그 방향으로 잰 거리 t = (x + y) / √2 가 52px마다 되풀이되고,
//   t가 26–52 구간인 곳이 무늬 색이다. 즉 "x + y = 상수" 인 직선(↗ 방향)을 경계로 하는 띠다.
constexpr qreal kStripeBand = 26;
constexpr qreal kStripePeriod = 52;
// 위아래 빨강 띠: 높이 12 + 먹선 3 (position: absolute라 레이아웃 공간을 차지하지 않는다)
constexpr int kBandHeight = 12;
constexpr int kBandLine = 3;

void paintIntroBackground(QPainter &painter, const QRect &rect)
{
    painter.fillRect(rect, QColor(tok::kPaper));

    // 사선 띠: x + y 가 [a, b) 인 영역 = 두 직선 사이의 평행사변형. 넘치는 부분은 위젯 경계에서
    // 잘린다.
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kIntroStripe));
    const qreal h = rect.height();
    const qreal maxSum = rect.width() + h; // 오른쪽 아래 모서리의 x + y
    const qreal scale = M_SQRT2;           // t(거리) → x + y
    for (qreal t = kStripeBand; t * scale < maxSum; t += kStripePeriod) {
        const qreal a = t * scale;
        const qreal b = (t + kStripeBand) * scale;
        const QPointF band[] = {{a, 0}, {b, 0}, {b - h, h}, {a - h, h}};
        painter.drawPolygon(band, 4);
    }
    painter.restore();

    // 위 · 아래 빨강 띠와 먹선
    const int w = rect.width();
    painter.fillRect(0, 0, w, kBandHeight, QColor(tok::kRed));
    painter.fillRect(0, kBandHeight, w, kBandLine, QColor(tok::kInk));
    painter.fillRect(0, rect.height() - kBandHeight - kBandLine, w, kBandLine, QColor(tok::kInk));
    painter.fillRect(0, rect.height() - kBandHeight, w, kBandHeight, QColor(tok::kRed));
}

QLabel *makeLabel(const QString &text, const char *objectName)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(
            QString::fromLatin1(objectName)); // app.qss의 QLabel#… 규칙이 모양을 정한다
    return label;
}
} // namespace

namespace com::yamada::studio {
HomePage::HomePage(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this); // installs itself on this
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(0); // 간격은 아래에서 칸마다 addSpacing으로 직접 넣는다

    // 크기는 각 위젯의 sizeHint()가 알려 준다. Qt::AlignHCenter를 주면 레이아웃은 위젯을
    // 칸 폭으로 늘리지 않고 sizeHint 크기 그대로 가로 가운데에 둔다.
    layout->addWidget(new MarkWidget, 0, Qt::AlignHCenter);
    layout->addSpacing(kMarkToWordmark);

    layout->addWidget(new WordmarkLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(kWordmarkToSubtitle);

    QLabel *subtitle = makeLabel(tr("도감 · 파티 도우미"), "introSubtitle");
    subtitle->setFixedHeight(kSubtitleLine);
    layout->addWidget(subtitle, 0, Qt::AlignHCenter);
    layout->addSpacing(kSubtitleToGeneration);

    QLabel *generationLabel = makeLabel(tr("세대를 선택하여 시작하세요:"), "introGenerationLabel");
    generationLabel->setFixedHeight(kGenerationLabelLine);
    layout->addWidget(generationLabel, 0, Qt::AlignHCenter);
    layout->addSpacing(kGenerationLabelToButton);

    // TODO(A8): AppState의 현재 세대와 연결하고, 누르면 세대 메뉴(1–9) 팝업을 띄운다.
    GenerationButton *generationButton = new GenerationButton;
    generationButton->setGeneration(4, tr("신오"));
    layout->addWidget(generationButton, 0, Qt::AlignHCenter);
    layout->addSpacing(kGenerationToMenu);

    // 메뉴 창 = 겉모양(PanelFrame) + 내용(IntroMenu). 폭은 쓰는 쪽(HomePage)이 정한다.
    m_menu = new IntroMenu;
    PanelFrame *menuFrame = new PanelFrame;
    menuFrame->setPanelStyle(kMenuWindow);
    menuFrame->setFixedWidth(kMenuWidth);
    menuFrame->setBody(m_menu);
    layout->addWidget(menuFrame, 0, Qt::AlignHCenter);

    layout->addStretch(); // 남는 세로 공간은 메뉴 창과 정보 줄 사이로 간다
    layout->addWidget(new IntroFooter); // 정렬 없음 → 가로로 꽉 찬다

    // 메뉴의 "몇 번째 줄 실행"을 화면 의미(페이지 열기 / 종료)로 바꿔서 밖에 알린다.
    connect(m_menu, &IntroMenu::activated, this, [this](int index) {
        if (index == IntroMenu::kQuitIndex)
            emit quitRequested();
        else
            emit openRequested(static_cast<Page>(index)); // 메뉴 순서 = Page 순서 (page.h)
    });
}

void HomePage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    paintIntroBackground(painter, rect());
}

void HomePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 인트로가 보이면 키보드 입력을 메뉴가 받게 한다(↑↓ Enter가 바로 동작하도록).
    // 이게 없으면 Tab 순서상 첫 위젯인 세대 버튼이 포커스를 가져간다.
    m_menu->setFocus(Qt::OtherFocusReason);
}
} // namespace com::yamada::studio
