#include "ui/home/homepage.h"

#include "data/update/dataupdater.h"
#include "ui/home/cardfan.h"
#include "ui/home/firstrunpanel.h"
#include "ui/home/introfooter.h"
#include "ui/home/intromenu.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/markwidget.h"
#include "ui/widgets/wordmarklabel.h"

#include <QKeyEvent>
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
constexpr int kSubtitleToFan = 8;           // 부제 ↔ 세대 카드 부채꼴
constexpr int kFanToMenu = 2; // 부채꼴 ↔ 메뉴 창 (부채꼴 아래 여백이 이미 넉넉하다)
constexpr int kMenuWidth = tok::kSizeIntroMenuWidth; // 520
constexpr int kFirstRunToMenu = 12;                  // 첫 실행 패널과 메뉴 창 사이
// 첫 실행 패널이 들어가면 세로가 모자란다. 화면 정의서(02b SCR-01): "높이가 부족하면 마크 → 부제
// 순으로 줄인다. 메뉴는 절대 잘리지 않는다." 패널이 있는 동안에는 마크를 작게(960 배치와 같은
// 72) 하고 부제와 세대 카드를 숨긴다(세대는 데이터를 받은 뒤 고르면 된다).
constexpr int kCompactMark = 72;
constexpr int kHomeMark = 80; // v2의 136보다 작게 — 세대 카드 부채꼴의 세로 자리(ADR 0015)
// 글자 줄의 높이. CSS의 line-height: normal은 브라우저가 글꼴 파일의 줄 간격 값으로 정하는데,
// Qt의 QLabel은 다른 값(QFontMetrics::height)을 쓴다. 도현 22는 Qt가 32로 잡아서 아래가 전부
// 밀리므로 기준 이미지(30_intro_1440.png)에서 잰 브라우저 값으로 고정한다.
constexpr int kSubtitleLine = 26; // 도현 22

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
HomePage::HomePage(DataUpdater *updater, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_updater(updater)
{
    QVBoxLayout *layout = new QVBoxLayout(this); // installs itself on this
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(0); // 간격은 아래에서 칸마다 addSpacing으로 직접 넣는다

    // 크기는 각 위젯의 sizeHint()가 알려 준다. Qt::AlignHCenter를 주면 레이아웃은 위젯을
    // 칸 폭으로 늘리지 않고 sizeHint 크기 그대로 가로 가운데에 둔다.
    m_mark = new MarkWidget;
    m_mark->setMarkSize(kHomeMark);
    layout->addWidget(m_mark, 0, Qt::AlignHCenter);
    layout->addSpacing(kMarkToWordmark);

    layout->addWidget(new WordmarkLabel, 0, Qt::AlignHCenter);
    // 간격을 addSpacing 대신 QSpacerItem으로 만들어 들고 있으면, 나중에 changeSize()로 바꿀 수
    // 있다.
    m_subtitleGap
            = new QSpacerItem(0, kWordmarkToSubtitle, QSizePolicy::Minimum, QSizePolicy::Fixed);
    layout->addSpacerItem(m_subtitleGap);

    m_subtitle = makeLabel(tr("도감 · 파티 도우미"), "introSubtitle");
    m_subtitle->setFixedHeight(kSubtitleLine);
    layout->addWidget(m_subtitle, 0, Qt::AlignHCenter);
    layout->addSpacing(kSubtitleToFan);

    // 세대 선택 = 카드 부채꼴. AppState와 바로 이어져 있어서(누르면 setGeneration) 여기서 더 이을
    // 것이 없다. 남는 세로 공간은 부채꼴이 가진다(stretch 1).
    m_fan = new CardFan(state);
    layout->addWidget(m_fan, 1);
    layout->addSpacing(kFanToMenu);

    // 첫 실행이면 메뉴 창 위에 패널을 끼운다. 패널과 그 아래 간격을 한 상자(m_firstRunBlock)에 담아
    // 두면, 끝났을 때 상자만 지우면 간격까지 함께 사라진다.
    const bool firstRun = !DataUpdater::hasData();
    if (firstRun) {
        m_firstRunBlock = new QWidget;
        QVBoxLayout *block = new QVBoxLayout(m_firstRunBlock);
        block->setContentsMargins(0, 0, 0, kFirstRunToMenu);
        m_firstRun = new FirstRunPanel;
        m_firstRun->setFixedWidth(kMenuWidth);
        block->addWidget(m_firstRun);
        layout->addWidget(m_firstRunBlock, 0, Qt::AlignHCenter);
    }

    // 메뉴 = 카드 버튼 3개 + 종료 줄(IntroMenu가 직접 그린다 — 바깥 창 없음, ADR 0015)
    m_menu = new IntroMenu;
    m_menu->setDataLocked(firstRun); // 데이터가 필요한 버튼(스쿼드 · 도감 · 아이템)을 잠근다
    m_menu->setFixedWidth(kMenuWidth);
    layout->addWidget(m_menu, 0, Qt::AlignHCenter);

    layout->addStretch(); // 남는 세로 공간은 메뉴 창과 정보 줄 사이로 간다
    layout->addWidget(new IntroFooter); // 정렬 없음 → 가로로 꽉 찬다

    // 메뉴의 "몇 번째 줄 실행"을 화면 의미(페이지 열기)로 바꿔서 밖에 알린다.
    // 메뉴 순서(스쿼드 · 도감 · 아이템)는 Page 순서와 다르므로 표로 바꾼다.
    connect(m_menu, &IntroMenu::activated, this, [this](int index) {
        constexpr Page kPages[] = {Page::Squad, Page::Dex, Page::Items};
        emit openRequested(kPages[index]);
    });

    if (m_firstRun) {
        setCompact(true);
        // 패널의 버튼 → DataUpdater, DataUpdater의 진행 · 결과 → 패널. 두 쪽은 서로를 모른다.
        // HomePage가 이 자리에서 둘을 잇기만 한다(패널은 UI, DataUpdater는 data 레이어).
        connect(m_firstRun, &FirstRunPanel::startRequested, m_updater, &DataUpdater::start);
        connect(m_firstRun, &FirstRunPanel::cancelRequested, m_updater, &DataUpdater::cancel);
        connect(m_updater, &DataUpdater::progress, m_firstRun, &FirstRunPanel::setProgress);
        connect(m_updater, &DataUpdater::failed, m_firstRun, &FirstRunPanel::showFailure);
        connect(m_updater, &DataUpdater::cancelled, m_firstRun,
                [this] { m_firstRun->setState(FirstRunPanel::State::Ready); });
        connect(m_updater, &DataUpdater::finished, this, &HomePage::onDataReady);
    }
}

void HomePage::setCompact(bool compact)
{
    // 첫 실행 패널이 들어간 만큼 위쪽을 줄인다. 1440×900에서 메뉴 창 아래 끝이 정보 줄(858)을 넘지
    // 않게.
    m_mark->setMarkSize(compact ? kCompactMark : kHomeMark);
    m_subtitle->setVisible(!compact);
    m_fan->setVisible(!compact); // 패널이 있는 동안은 부채꼴을 숨겨 세로를 번다
    m_subtitleGap->changeSize(0, compact ? 0 : kWordmarkToSubtitle, QSizePolicy::Minimum,
                              QSizePolicy::Fixed);
    QWidget::layout()->invalidate(); // 간격(spacer)은 위젯이 아니라서 바뀐 것을 스스로 레이아웃에
                                     // 알리지 못한다
}

void HomePage::onDataReady()
{
    // 패널 상자를 지운다. deleteLater: 지금 처리 중인 신호가 끝난 뒤(이벤트 루프로 돌아간 뒤)
    // 지운다. 이 함수는 DataUpdater의 신호 처리 도중에 불리므로, 바로 delete하는 것보다 안전하다.
    m_firstRunBlock->deleteLater();
    m_firstRunBlock = nullptr;
    m_firstRun = nullptr;
    setCompact(false);
    m_menu->setDataLocked(false); // 잠금 해제, 선택은 도감 백과
    m_menu->setFocus(Qt::OtherFocusReason);
}

void HomePage::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    paintIntroBackground(painter, rect());
}

void HomePage::keyPressEvent(QKeyEvent *event)
{
    // 메뉴(포커스 소유)가 처리하지 않은 키가 여기로 전파된다. ← → 는 세대 카드로 보낸다.
    if (event->key() == Qt::Key_Left || event->key() == Qt::Key_Right) {
        if (m_fan->isVisible())
            m_fan->selectNeighbor(event->key() == Qt::Key_Left ? -1 : +1);
        return;
    }
    QWidget::keyPressEvent(event);
}

void HomePage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 인트로가 보이면 키보드 입력을 메뉴가 받게 한다(↑↓ Enter가 바로 동작하도록).
    // 이게 없으면 Tab 순서상 첫 위젯인 세대 버튼이 포커스를 가져간다.
    if (m_firstRun)
        m_firstRun->focusTarget()->setFocus(Qt::OtherFocusReason); // 첫 포커스 = "데이터 받기"(02b)
    else
        m_menu->setFocus(Qt::OtherFocusReason);
}
} // namespace com::yamada::studio
