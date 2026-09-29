#include "ui/home/firstrunpanel.h"

#include "data/update/csvsource.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"
#include "ui/widgets/segmentprogress.h"
#include "ui/widgets/shadowbutton.h"
#include "ui/widgets/svgicon.h"

#include <QApplication>
#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QStackedWidget>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

// IntroStates.dc.html의 패널: 먹선 3 · 반경 10 · 그림자 4 · 안쪽 여백 14 · 줄 사이 gap 10
constexpr PanelStyle kNormal {
        .outline = 3, .radius = 10, .shadow = 4, .fill = tok::kWhite, .ink = tok::kInk};
// 실패: 테두리와 그림자가 빨강, 바탕이 red.tint
constexpr PanelStyle kFailed {
        .outline = 3, .radius = 10, .shadow = 4, .fill = tok::kRedTint, .ink = tok::kRed};
constexpr int kPadding = 14;
constexpr int kGap = 10;
constexpr int kIconSize = 15;
constexpr int kWarningSize = 18;

// 디자인 원본의 인라인 SVG. 색 자리(%1)는 토큰 색으로 채운다.
const char kDownloadSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" )"
          R"(stroke="%1" stroke-width="2.8"><path d="M12 4v11M7 10l5 5 5-5M5 20h14"/></svg>)";
const char kWarningSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" )"
          R"(stroke="%1" stroke-width="2.8"><path d="M12 4l9 16H3z"/><path d="M12 10v4M12 17v.5"/></svg>)";

QString svgWithColor(const char *svg, QRgb color)
{
    return QString::fromLatin1(svg).arg(QColor(color).name());
}

// objectName은 app.qss의 QLabel#… 규칙이 모양(글꼴 · 크기 · 색)을 고르는 이름이다.
QLabel *makeLabel(const QString &text, const char *objectName, bool wrap = false)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(QString::fromLatin1(objectName));
    label->setWordWrap(wrap);
    return label;
}

QVBoxLayout *pageLayout(QWidget *page)
{
    QVBoxLayout *layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kGap);
    return layout;
}
} // namespace

namespace com::yamada::studio {
FirstRunPanel::FirstRunPanel(QWidget *parent)
    : QWidget(parent)
{
    // 겉모양 두께(먹선 3 + 안쪽 여백 14, 아래는 그림자 4 더)만큼 들여서 내용을 놓는다.
    QVBoxLayout *layout = new QVBoxLayout(this);
    const int side = kNormal.outline + kPadding;
    layout->setContentsMargins(side, side, side, side + kNormal.shadow);

    // 세 상태를 페이지로 만들어 겹쳐 두고 하나만 보여 준다. QStackedWidget은 가장 큰 페이지에 맞춘
    // 크기를 가지므로, 상태가 바뀌어도 패널 높이가 출렁이지 않는다(아래 메뉴가 들썩이지 않는다).
    m_pages = new QStackedWidget;
    m_pages->setObjectName(
            QStringLiteral("firstRunPages")); // app.qss: 전역 paper 배경을 칠하지 않게
    m_pages->addWidget(buildReadyPage());
    m_pages->addWidget(buildDownloadingPage());
    m_pages->addWidget(buildFailedPage());
    layout->addWidget(m_pages);

    connect(m_startButton, &ShadowButton::clicked, this, &FirstRunPanel::startRequested);
    connect(m_retryButton, &ShadowButton::clicked, this, &FirstRunPanel::startRequested);
    connect(m_cancelButton, &ShadowButton::clicked, this, &FirstRunPanel::cancelRequested);

    setState(State::Ready);
}

QWidget *FirstRunPanel::buildReadyPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = pageLayout(page);
    layout->addWidget(makeLabel(tr("도감 데이터가 아직 없어요"), "firstRunTitle"));
    layout->addWidget(makeLabel(tr("PokéAPI에서 포켓몬 · 기술 · 아이템 정보를 받아 와요. "
                                   "한 번만 받으면 인터넷 없이도 써요."),
                                "firstRunBody", true));
    // 예상 크기 = 받을 CSV 전체 크기(csvsource.h). 받는 쪽 목록이 바뀌면 이 숫자도 저절로 바뀐다.
    const double megabytes = csvsource::kTotalSize / 1'000'000.0;
    layout->addWidget(
            makeLabel(tr("예상 크기 약 %1 MB").arg(megabytes, 0, 'f', 1), "firstRunMeta"));

    m_startButton = new ShadowButton(ShadowButton::Variant::Primary);
    m_startButton->setText(tr("데이터 받기"));
    m_startButton->setIcon(
            svgicon::icon(svgWithColor(kDownloadSvg, tok::kWhite), kIconSize, devicePixelRatioF()));
    layout->addWidget(m_startButton);
    layout->addStretch(); // 페이지 높이가 가장 큰 페이지에 맞춰질 때 남는 칸은 아래로
    return page;
}

QWidget *FirstRunPanel::buildDownloadingPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = pageLayout(page);
    layout->addWidget(makeLabel(tr("데이터 받는 중…"), "firstRunTitle"));

    // 단계 이름(왼쪽) · 진행 수치(오른쪽) 한 줄
    QHBoxLayout *row = new QHBoxLayout;
    m_stepLabel = makeLabel(QString(), "firstRunStep");
    m_percentLabel = makeLabel(QString(), "firstRunPercent");
    row->addWidget(m_stepLabel);
    row->addStretch();
    row->addWidget(m_percentLabel);
    layout->addLayout(row);

    m_progress = new SegmentProgress;
    layout->addWidget(m_progress);
    layout->addWidget(makeLabel(tr("창을 닫아도 다음에 이어서 받아요."), "firstRunNote"));

    m_cancelButton = new ShadowButton(ShadowButton::Variant::Secondary);
    m_cancelButton->setText(tr("취소"));
    layout->addWidget(m_cancelButton);
    layout->addStretch();
    return page;
}

QWidget *FirstRunPanel::buildFailedPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = pageLayout(page);

    // ⚠ 아이콘 + 제목 한 줄
    QHBoxLayout *title = new QHBoxLayout;
    title->setSpacing(8);
    QLabel *icon = new QLabel;
    icon->setPixmap(svgicon::pixmap(svgWithColor(kWarningSvg, tok::kRed), kWarningSize,
                                    devicePixelRatioF()));
    title->addWidget(icon);
    title->addWidget(makeLabel(tr("데이터를 받지 못했어요"), "firstRunFailedTitle"));
    title->addStretch();
    layout->addLayout(title);

    layout->addWidget(
            makeLabel(tr("네트워크 연결을 확인하고 다시 시도해 주세요. 받은 부분은 남아 있어요."),
                      "firstRunBody", true));
    m_errorLabel = makeLabel(QString(), "firstRunMeta", true);
    layout->addWidget(m_errorLabel);

    m_retryButton = new ShadowButton(ShadowButton::Variant::Primary);
    m_retryButton->setText(tr("다시 시도"));
    layout->addWidget(m_retryButton);
    layout->addStretch();
    return page;
}

void FirstRunPanel::setState(State state)
{
    // 패널 안의 버튼에 키보드 포커스가 있었다면, 페이지가 바뀐 뒤 새 페이지의 버튼으로 옮긴다
    // (누른 버튼이 사라지면 포커스가 갈 곳을 잃는다).
    const bool hadFocus = QWidget::isAncestorOf(QApplication::focusWidget());
    m_state = state;
    m_pages->setCurrentIndex(static_cast<int>(state)); // 페이지 순서 = State 순서
    if (hadFocus)
        focusTarget()->setFocus(Qt::OtherFocusReason);
    QWidget::update(); // 실패면 테두리 색이 바뀐다 → 다시 그린다
}

void FirstRunPanel::setProgress(int percent, const QString &label)
{
    if (m_state != State::Downloading)
        setState(State::Downloading);
    m_stepLabel->setText(label);
    m_percentLabel->setText(QStringLiteral("%1%").arg(percent));
    m_progress->setValue(percent);
}

void FirstRunPanel::showFailure(const QString &detail)
{
    m_errorLabel->setText(detail);
    setState(State::Failed);
}

QWidget *FirstRunPanel::focusTarget() const
{
    switch (m_state) {
    case State::Ready:
        return m_startButton;
    case State::Downloading:
        return m_cancelButton;
    case State::Failed:
        return m_retryButton;
    }
    return m_startButton;
}

void FirstRunPanel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    paintPanel(painter, rect(), m_state == State::Failed ? kFailed : kNormal);
}
} // namespace com::yamada::studio
