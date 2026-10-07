#include "ui/squad/squadpage.h"

#include "core/analysis/resourceledger.h"
#include "core/rules/generationfeatures.h"
#include "core/types/typekey.h"
#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "data/state/squadsession.h"
#include "data/store/squadfile.h"
#include "data/store/squadstore.h"
#include "ui/dex/dexdetailpage.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/dex/gameselector.h"
#include "ui/dex/guidebook.h"
#include "ui/dex/moveeffect.h"
#include "ui/dex/naturepicker.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/logging/logging.h"
#include "ui/squad/dexfilterbar.h"
#include "ui/squad/heatmapview.h"
#include "ui/squad/listpicker.h"
#include "ui/squad/problemlist.h"
#include "ui/squad/shuttledialog.h"
#include "ui/squad/slotcard.h"
#include "ui/squad/splitbar.h"
#include "ui/squad/squadpaint.h"
#include "ui/squad/starters.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/itemstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/shadowbutton.h"
#include "ui/widgets/svgicon.h"
#include "ui/widgets/typechip.h"

#include <QBoxLayout>
#include <QClipboard>
#include <QDialog>
#include <QFileDialog>
#include <QFontMetricsF>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QGuiApplication>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmapCache>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScreen>
#include <QScrollArea>
#include <QScrollBar>
#include <QStandardPaths>
#include <QStyle>
#include <QToolButton>

namespace {
using namespace com::yamada::studio;

// 편집 도구 아이콘(전형적인 undo · redo · reset 둥근 화살표). 색은 %1로 끼운다
const char kUndoSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="%1" )"
          R"(stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">)"
          R"(<path d="M9 14 4 9l5-5"/><path d="M4 9h10.5a5.5 5.5 0 0 1 0 11H11"/></svg>)";
const char kRedoSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="%1" )"
          R"(stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">)"
          R"(<path d="M15 14l5-5-5-5"/><path d="M20 9H9.5a5.5 5.5 0 0 0 0 11H13"/></svg>)";
const char kClearSvg[]
        = R"(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="%1" )"
          R"(stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">)"
          R"(<path d="M22 4v6h-6"/><path d="M20.5 15a8.5 8.5 0 1 1-2-8.9L22 10"/></svg>)";

// 화면 공통 여백(02 SCR 공통). 아래만 12: 분석 창 · 카드가 창 바닥에 더 가깝게(사용자 의견)
constexpr QMargins kPageMargins {20, 16, 20, 12};
constexpr int kCardGap = 12;
constexpr int kWideCardAreaWidth = 2 * 300 + kCardGap; // 넓은 화면: 슬롯 그리드 600
// 카드 끌기 애니메이션: 밀려나는 카드 · 놓은 카드가 칸에 들어가는 시간, 끝에 닿을 때 굴리는 양
constexpr int kShiftMs = 180;
constexpr int kDropMs = 140;
constexpr int kAutoScrollMargin = 40;
constexpr int kAutoScrollStep = 18;
// 좁은 배치에서 문제 칸이 펴지는 최대 줄 수(그 이상은 칸 안에서 스크롤). 넓은 배치는 남는 세로를
// 쓴다
constexpr int kProblemVisibleRows = 4;
constexpr QSize kProblemDialogSize {820, 640}; // "크게 보기" 창 — 제목 · 설명이 잘리지 않는 폭
constexpr PanelStyle kAnalysisPanel {.outline = 2,
                                     .radius = 8,
                                     .shadow = 3,
                                     .fill = tok::kWhite,
                                     .ink = tok::kInk,
                                     .header = 38,
                                     .headerColor = tok::kBlue};

QString keyOf(Type type)
{
    const std::string_view key = typeKey(type);
    return QString::fromLatin1(key.data(), qsizetype(key.size()));
}

QString times(double multiplier)
{
    const QString text = squadpaint::multiplierText(multiplier);
    return QStringLiteral("×") + (text.isEmpty() ? QStringLiteral("1") : text);
}

QPixmap itemPixmap(SpriteCache *icons, const ItemRow &item, int generation)
{
    const QString key = ItemRowDelegate::iconKey(item.identifier, item.machineType, generation);
    const QString file = icons->path(key);
    if (file.isEmpty()) {
        icons->request(key);
        return {};
    }
    QPixmap pixmap;
    const QString cacheKey = QStringLiteral("pokesix.item.") + key;
    if (!QPixmapCache::find(cacheKey, &pixmap) && pixmap.load(file))
        QPixmapCache::insert(cacheKey, pixmap);
    return pixmap;
}
} // namespace

namespace com::yamada::studio {
// 슬롯 핍 6개(첫 타입 색, 빈 칸은 흰색)
class SquadPips : public QWidget
{
public:
    explicit SquadPips(SquadSession *session, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_session(session)
    {
        QWidget::setFixedSize(6 * 16, 24);
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        for (int slot = 0; slot < 6; ++slot) {
            const PokemonDetail &detail = m_session->detail(slot);
            const tok::TypeColor *type
                    = detail.types.isEmpty() ? nullptr : typechip::find(detail.types.first());
            painter.setPen(QPen(QColor(tok::kInk), 1.5));
            painter.setBrush(QColor(type ? type->fill : tok::kWhite));
            painter.drawRoundedRect(QRectF(slot * 16 + 1, 6, 12, 12), 2, 2);
        }
    }

private:
    SquadSession *m_session;
};

SquadPage::SquadPage(Repository *repository, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_state(state)
    , m_store(new SquadStore(QString(), this))
    , m_session(new SquadSession(repository, m_store, state, this))
    , m_pokemonIcons(new SpriteCache(SpriteCache::Kind::PokemonIcon, this))
    , m_itemIcons(new SpriteCache(SpriteCache::Kind::Item, this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(14);
    layout->addWidget(buildTopBar());

    // 카드 · 분석 창은 한 장으로 세로 스크롤(좁은 창에서는 분석 창이 카드 아래로 간다)
    QWidget *content = new QWidget;
    content->setObjectName(QStringLiteral("squadContent")); // app.qss: 투명
    m_columns = new QBoxLayout(QBoxLayout::TopToBottom, content);
    m_columns->setContentsMargins(0, 0, 4, 4); // 오른쪽 4: 스크롤바와 그림자 사이
    m_columns->setSpacing(16);
    m_cardArea = new QWidget;
    m_grid = new QGridLayout(m_cardArea);
    m_grid->setContentsMargins(0, 0, 0, 0);
    m_grid->setSpacing(kCardGap - 6); // 카드 안에 테 자리(3)가 양쪽에 있다
    for (int slot = 0; slot < 6; ++slot) {
        SlotCard *card = new SlotCard(slot, m_session, m_pokemonIcons, m_itemIcons);
        connect(card, &SlotCard::selectRequested, this, &SquadPage::selectSlot);
        connect(card, &SlotCard::addRequested, this, &SquadPage::pickPokemon);
        connect(card, &SlotCard::replaceRequested, this, &SquadPage::pickPokemon);
        connect(card, &SlotCard::removeRequested, this, [this](int s) { m_session->clearSlot(s); });
        connect(card, &SlotCard::detailRequested, this, &SquadPage::showMemberDetail);
        connect(card, &SlotCard::moveRequested, this, &SquadPage::pickMove);
        connect(card, &SlotCard::abilityRequested, this, &SquadPage::pickAbility);
        connect(card, &SlotCard::natureRequested, this, &SquadPage::pickNature);
        connect(card, &SlotCard::dragStarted, this, &SquadPage::onDragStarted);
        connect(card, &SlotCard::dragMoved, this, &SquadPage::onDragMoved);
        connect(card, &SlotCard::dragFinished, this, &SquadPage::onDragFinished);
        connect(card, &SlotCard::itemRequested, this,
                [this](int s, const QPoint &) { pickItem(s); });
        m_cards.append(card);
    }
    m_columns->addWidget(m_cardArea, 0, Qt::AlignTop);
    // 분석 열: 카드가 선택 테 자리(SlotCard::kRing)만큼 안쪽에 그려지므로, 분석 창도 같은 만큼
    // 들여 넣어야 두 열의 시각적 위 · 아래 끝단이 맞는다
    m_analysisColumn = new QWidget;
    QVBoxLayout *analysisColumn = new QVBoxLayout(m_analysisColumn);
    analysisColumn->setContentsMargins(0, SlotCard::kRing, 0, SlotCard::kRing);
    analysisColumn->setSpacing(0);
    analysisColumn->addWidget(buildAnalysis());
    m_columns->addWidget(m_analysisColumn, 1); // 넓은 배치: 세로를 채운다(placeCards가 바꾼다)
    m_columns->addStretch();
    placeCards(false);

    m_scroll = new QScrollArea;
    m_scroll->setObjectName(QStringLiteral("squadScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setWidget(content);
    m_scroll->viewport()->installEventFilter(this); // 크기가 정해지면 카드 높이를 맞춘다
    layout->addWidget(m_scroll, 1);

    // 세션이 바뀌면(편집 · 세대) 전부 다시 그린다. 언어가 바뀌면 이름 · 타입 글자만 바뀐다
    connect(m_session, &SquadSession::changed, this, &SquadPage::refresh);
    connect(m_state, &AppState::languageChanged, this, &SquadPage::refresh);
    connect(m_store, &SquadStore::saveScheduled, this, [this] {
        m_saveStatus->setProperty("state", QStringLiteral("pending"));
        m_saveStatus->setText(tr("저장 중…"));
        style()->polish(m_saveStatus); // 동적 속성으로 QSS 선택자가 바뀌었다 → 다시 입힌다
    });
    connect(m_store, &SquadStore::saved, this, [this](bool ok) {
        m_saveStatus->setProperty("state", ok ? QString() : QStringLiteral("error"));
        m_saveStatus->setText(ok ? tr("✓ 자동 저장됨") : tr("저장하지 못했어요"));
        style()->polish(m_saveStatus);
    });
    refresh();
}

QWidget *SquadPage::buildTopBar()
{
    QWidget *bar = new QWidget;
    m_topBar = bar;
    QHBoxLayout *layout = new QHBoxLayout(bar);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);
    m_name = new QLineEdit;
    m_name->setObjectName(QStringLiteral("squadName")); // app.qss: 제목 글꼴 · 테 없음
    m_name->setMaxLength(24);
    m_name->setToolTip(tr("눌러서 이름을 바꿔요"));
    // 이름 칸은 글자 폭만큼(늘어나면 옆의 알약 · 버튼이 오른쪽 끝으로 밀린다)
    m_name->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_name, &QLineEdit::textChanged, this, [this](const QString &text) {
        const QFontMetricsF metrics(theme::font(theme::kFamilyTitle, 28));
        m_name->setFixedWidth(
                int(metrics.horizontalAdvance(text.isEmpty() ? QStringLiteral("…") : text)) + 20);
    });
    connect(m_name, &QLineEdit::editingFinished, this, [this] {
        const QString name = m_name->text().trimmed();
        m_session->setName(name == m_session->defaultName() ? QString() : name);
        m_name->clearFocus();
    });
    layout->addWidget(m_name);
    m_rule = new QLabel;
    m_rule->setObjectName(QStringLiteral("squadRulePill"));
    m_rule->setToolTip(tr("분석이 이 세대의 규칙을 따라요 — 상성표(페어리는 6세대부터),\n"
                          "물리·특수 구분(3세대까지 타입, 4세대부터 기술),\n"
                          "특성·성격(3세대부터) · 지닌 물건(2세대부터)"));
    layout->addWidget(m_rule);
    // 게임 칩: 게임마다 스쿼드가 따로 저장된다(도감 · 나오는 포켓몬 · 기술이 게임마다 다르다)
    m_game = new GameSelector;
    m_game->setSplitVersions(true); // 스쿼드는 버전마다 따로 — [D] [P] [Pt] [HG] [SS]
    connect(m_game, &GameSelector::versionSelected, m_session, &SquadSession::setVersion);
    layout->addWidget(m_game);
    m_pips = new SquadPips(m_session);
    layout->addWidget(m_pips);
    m_count = new QLabel;
    m_count->setObjectName(QStringLiteral("squadCount"));
    layout->addWidget(m_count);
    // 비전셔틀(7번째 멤버): 카드 6장을 건드리지 않게 버튼 → 창으로
    m_shuttleButton = new QPushButton;
    m_shuttleButton->setObjectName(QStringLiteral("squadShuttleButton"));
    m_shuttleButton->setCursor(Qt::PointingHandCursor);
    // 아이콘을 글자 뒤에: QPushButton은 아이콘이 늘 글자 앞이라 방향을 뒤집는다("비전셔틀 ·
    // 다꼬리 <아이콘>"). 한국어는 LTR 글자라 글자 순서는 그대로다
    m_shuttleButton->setLayoutDirection(Qt::RightToLeft);
    m_shuttleButton->setToolTip(
            tr("파도타기 · 괴력 같은 비전머신을 대신 드는 7번째 멤버 — 분석에는 안 들어가요"));
    connect(m_shuttleButton, &QPushButton::clicked, this, &SquadPage::showShuttleDialog);
    connect(m_pokemonIcons, &SpriteCache::ready, this, &SquadPage::refreshShuttleButton);
    layout->addWidget(m_shuttleButton);
    // 편집 도구: 되돌리기 · 다시 실행 · 일괄 비우기 — 테두리 없는 아이콘 버튼(엑셀 식)
    auto iconButton = [&](const char *svg, const QString &tip) {
        QToolButton *button = new QToolButton;
        button->setObjectName(QStringLiteral("squadIconButton"));
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(tip);
        const auto colored = [svg](QRgb color) {
            return QString::fromLatin1(svg).arg(QColor(color).name());
        };
        QIcon icon = svgicon::icon(colored(tok::kText1), 20, devicePixelRatioF());
        icon.addPixmap(svgicon::pixmap(colored(tok::kText3), 20, devicePixelRatioF()),
                       QIcon::Disabled);
        button->setIcon(icon);
        button->setIconSize(QSize(20, 20));
        layout->addWidget(button);
        return button;
    };
    m_undoButton = iconButton(kUndoSvg, tr("되돌리기"));
    connect(m_undoButton, &QToolButton::clicked, m_session, &SquadSession::undo);
    m_redoButton = iconButton(kRedoSvg, tr("다시 실행"));
    connect(m_redoButton, &QToolButton::clicked, m_session, &SquadSession::redo);
    m_clearButton = iconButton(kClearSvg,
                               tr("일괄 비우기 — 멤버와 비전셔틀을 모두 비워요(되돌릴 수 있어요)"));
    connect(m_clearButton, &QToolButton::clicked, m_session, &SquadSession::clearAll);
    layout->addStretch();
    // 공유 묶음: 스쿼드를 파일로 주고받거나(불러오기 · 내보내기) 이미지로 공유한다
    auto tool = [&](const QString &text, const QString &tip) {
        QPushButton *button = new QPushButton(text);
        button->setObjectName(QStringLiteral("squadToolButton"));
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(tip);
        layout->addWidget(button);
        return button;
    };
    connect(tool(tr("불러오기"), tr("내보냈던 스쿼드 파일(.json)을 읽어 와요")),
            &QPushButton::clicked, this, &SquadPage::importSquad);
    connect(tool(tr("내보내기"), tr("이 스쿼드를 파일(.json)로 저장해요 — 다른 PC의 PokeSix에서 "
                                    "불러올 수 있어요")),
            &QPushButton::clicked, this, &SquadPage::exportSquad);
    QPushButton *image = tool(tr("이미지 ▾"), tr("카드 6장과 분석 창을 한 장의 그림으로"));
    QMenu *imageMenu = new QMenu(image);
    imageMenu->addAction(tr("클립보드로 복사"), this, &SquadPage::copyImage);
    imageMenu->addAction(tr("PNG로 저장"), this, &SquadPage::saveImage);
    image->setMenu(imageMenu);
    layout->addSpacing(4);
    m_saveStatus = new QLabel(tr("✓ 자동 저장"));
    m_saveStatus->setObjectName(QStringLiteral("squadSaveStatus"));
    layout->addWidget(m_saveStatus);
    return bar;
}

QWidget *SquadPage::buildAnalysis()
{
    m_analysis = new PanelFrame;
    m_analysis->setPanelStyle(kAnalysisPanel);
    m_problemPill = new QLabel;
    m_problemPill->setObjectName(QStringLiteral("squadProblemPill"));
    m_analysis->setHeaderWidget(m_problemPill);

    // 빈 스쿼드면 안내 문구만, 아니면 분석. QStackedWidget은 큰 쪽 높이를 잡아서 빈 상태에도 창이
    // 길어진다 → 둘 다 두고 하나만 보이게 한다
    m_emptyAnalysis = new QLabel(tr("첫 포켓몬을 추가하면 분석이 시작돼요"));
    m_emptyAnalysis->setObjectName(QStringLiteral("squadEmpty"));
    m_emptyAnalysis->setAlignment(Qt::AlignCenter);
    m_emptyAnalysis->setMinimumHeight(120);

    QWidget *body = new QWidget;
    m_analysisBody = body;
    QVBoxLayout *layout = new QVBoxLayout(body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);
    // 순서: 히트맵(항상 맨 위에 고정) → 문제 목록(자기 칸 안에서만 스크롤) → 물리 · 특수 →
    // 리소스 투자. 덩이들이 한눈에 들어오게 — 문제가 13개여도 히트맵과 분포가 밀려나지 않는다
    QHBoxLayout *heatTitle = new QHBoxLayout;
    QLabel *heatLabel = new QLabel(tr("방어 상성 히트맵"));
    heatLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    heatTitle->addWidget(heatLabel);
    heatTitle->addStretch();
    QLabel *heatNote = new QLabel(tr("행 = 포켓몬 · 열 = 공격 타입 · 숫자 = 받는 배율"));
    heatNote->setObjectName(QStringLiteral("squadNote"));
    heatTitle->addWidget(heatNote);
    layout->addLayout(heatTitle);
    m_heatmap = new HeatmapView(m_session);
    connect(m_heatmap, &HeatmapView::slotClicked, this, &SquadPage::selectSlot);
    layout->addWidget(m_heatmap);
    layout->addSpacing(6);

    QHBoxLayout *problemTitle = new QHBoxLayout;
    problemTitle->setSpacing(10);
    QLabel *problemLabel = new QLabel(tr("문제 항목"));
    problemLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    problemTitle->addWidget(problemLabel);
    m_problemCount = new QLabel;
    m_problemCount->setObjectName(QStringLiteral("squadCount"));
    problemTitle->addWidget(m_problemCount);
    problemTitle->addStretch();
    QPushButton *expand = new QPushButton(tr("크게 보기 ↗"));
    expand->setObjectName(QStringLiteral("squadLinkButton")); // app.qss: 글자만 있는 링크 버튼
    expand->setCursor(Qt::PointingHandCursor);
    expand->setFlat(true);
    expand->setToolTip(tr("문제 전체를 큰 창에서 봐요"));
    connect(expand, &QPushButton::clicked, this, &SquadPage::showProblemDialog);
    problemTitle->addWidget(expand);
    layout->addLayout(problemTitle);
    m_problems = new ProblemList;
    connect(m_problems, &ProblemList::rowHovered, this, &SquadPage::onProblemHovered);
    connect(m_problems, &ProblemList::rowClicked, this, &SquadPage::onProblemClicked);
    m_problemScroll = new QScrollArea;
    m_problemScroll->setObjectName(QStringLiteral("squadScroll")); // 투명
    m_problemScroll->setWidgetResizable(
            true); // 폭은 칸을 따라가고, 높이는 ProblemList(Fixed)가 정한다
    m_problemScroll->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_problemScroll->setFrameShape(QFrame::NoFrame);
    m_problemScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_problemScroll->setWidget(m_problems);
    layout->addWidget(m_problemScroll, 1); // 넓은 배치에서 남는 세로를 이 칸이 가져간다
    layout->addSpacing(6);

    QHBoxLayout *splitTitle = new QHBoxLayout;
    splitTitle->setSpacing(10);
    QLabel *splitLabel = new QLabel(tr("물리 · 특수 분포"));
    splitLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    splitTitle->addWidget(splitLabel);
    splitTitle->addStretch();
    m_moveCount = new QLabel;
    m_moveCount->setObjectName(QStringLiteral("squadCount"));
    splitTitle->addWidget(m_moveCount);
    layout->addLayout(splitTitle);
    m_split = new SplitBar;
    layout->addWidget(m_split);

    // 리소스 투자: 기술 배치가 쓰는 소모 자원(하트비늘 · 기술머신 · 가르침 비용). 들어가는
    // 자원이 없으면 칸을 통째로 숨긴다
    m_resourceBox = new QWidget;
    QVBoxLayout *resourceLayout = new QVBoxLayout(m_resourceBox);
    resourceLayout->setContentsMargins(0, 6, 0, 0);
    resourceLayout->setSpacing(6);
    QHBoxLayout *resourceTitle = new QHBoxLayout;
    resourceTitle->setSpacing(10);
    QLabel *resourceLabel = new QLabel(tr("리소스 투자"));
    resourceLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    resourceTitle->addWidget(resourceLabel);
    resourceTitle->addStretch();
    m_resourceTotal = new QLabel;
    m_resourceTotal->setObjectName(QStringLiteral("squadCount"));
    resourceTitle->addWidget(m_resourceTotal);
    resourceLayout->addLayout(resourceTitle);
    m_resources = new QLabel;
    m_resources->setObjectName(QStringLiteral("squadResources"));
    m_resources->setWordWrap(true);
    m_resources->setTextFormat(Qt::RichText);
    resourceLayout->addWidget(m_resources);
    layout->addWidget(m_resourceBox);

    QWidget *frameBody = new QWidget;
    QVBoxLayout *frameLayout = new QVBoxLayout(frameBody);
    frameLayout->setContentsMargins(14, 10, 14, 12);
    frameLayout->addWidget(m_emptyAnalysis);
    frameLayout->addWidget(body, 1); // 남는 세로는 몸통(→ 문제 칸)이 먼저 가져간다
    frameLayout->addStretch(); // 문제 칸이 내용 높이에 닿으면 그 뒤 남는 공간은 아래로

    // 분석이 창보다 길면 분석 "안"에서만 스크롤한다(넓은 배치). 페이지 전체가 밀리지 않게.
    m_analysisScroll = new QScrollArea;
    m_analysisScroll->setObjectName(QStringLiteral("squadScroll")); // app.qss: 투명
    m_analysisScroll->setWidgetResizable(true);
    m_analysisScroll->setFrameShape(QFrame::NoFrame);
    m_analysisScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_analysisScroll->setWidget(frameBody);
    m_analysis->setBody(m_analysisScroll);
    return m_analysis;
}

void SquadPage::syncProblemHeight()
{
    const int content = m_problems->sizeHint().height();
    if (m_wide) {
        // 최소 두 줄은 보이게, 내용보다 커지지는 않게(그 뒤는 frameLayout의 stretch가 받는다)
        m_problemScroll->setMinimumHeight(std::min(content, ProblemList::heightForRows(2)));
        m_problemScroll->setMaximumHeight(content);
    } else {
        // 좁은 배치는 페이지가 스크롤한다 → 문제 칸은 kProblemVisibleRows 줄까지만 펴고 안에서
        // 스크롤
        const int h = std::min(content, ProblemList::heightForRows(kProblemVisibleRows));
        m_problemScroll->setMinimumHeight(h);
        m_problemScroll->setMaximumHeight(h);
    }
}

void SquadPage::syncAnalysisHeight()
{
    syncProblemHeight(); // 바깥 높이를 재기 전에 — 문제 칸의 최소 · 최대가 내용 높이에 들어간다
    if (m_wide) {
        m_analysisScroll->setMinimumHeight(0); // 세로는 열이 정한다 — 넘치면 안에서 스크롤
        return;
    }
    QWidget *content = m_analysisScroll->widget();
    m_analysisScroll->setMinimumHeight(content->sizeHint().height());
}

void SquadPage::fitCardHeights()
{
    // 카드 줄(넓으면 3줄, 좁으면 2줄)이 스크롤 없이 들어가게 카드 높이를 맞춘다.
    // SlotCard가 [kMinimumHeight, kHeight]로 자르므로, 창이 더 낮으면 그대로 스크롤이 생긴다.
    const int rows = m_wide ? 3 : 2;
    const int columns = m_wide ? 2 : 3;
    const int available = m_scroll->viewport()->height() - m_columns->contentsMargins().bottom()
                          - (rows - 1) * m_grid->verticalSpacing();
    // 나머지 px는 윗줄부터 1px씩 — 카드 열의 바닥이 분석 창과 정확히 같은 줄에 온다
    const int height = available / rows;
    const int extra = available - height * rows;
    for (int slot = 0; slot < m_cards.size(); ++slot)
        m_cards.at(slot)->setCardHeight(height + (slot / columns < extra ? 1 : 0));
}

void SquadPage::placeCards(bool wide)
{
    m_wide = wide;
    for (SlotCard *card : std::as_const(m_cards))
        m_grid->removeWidget(card);
    const int columns = wide ? 2 : 3;
    for (int slot = 0; slot < m_cards.size(); ++slot)
        m_grid->addWidget(m_cards.at(slot), slot / columns, slot % columns);
    for (int c = 0; c < 3; ++c)
        m_grid->setColumnStretch(c, c < columns ? 1 : 0);
    m_columns->setDirection(wide ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom);
    if (wide) {
        m_cardArea->setFixedWidth(kWideCardAreaWidth);
    } else {
        m_cardArea->setMinimumWidth(0);
        m_cardArea->setMaximumWidth(QWIDGETSIZE_MAX);
    }
    // 넓으면: 분석 창이 열 세로를 채우고, 넘치는 내용은 창 안에서 스크롤 → 바깥은 안 밀린다.
    // 좁으면: 분석 창을 내용 높이대로 펴고(안쪽 스크롤 없음) 페이지 전체가 스크롤한다(전처럼).
    m_columns->setAlignment(m_analysisColumn, wide ? Qt::Alignment() : Qt::AlignTop);
    m_analysisScroll->setVerticalScrollBarPolicy(wide ? Qt::ScrollBarAsNeeded
                                                      : Qt::ScrollBarAlwaysOff);
    syncAnalysisHeight();
}

void SquadPage::reloadData()
{
    m_evolvesFrom.clear();
    m_session->reload();
}

void SquadPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 첫 실행에 데이터를 받기 전에 만들어졌다면(상성표가 비어 있다) 지금 다시 읽는다
    if (m_session->chart().types.isEmpty())
        m_session->reload();
}

void SquadPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    const bool wide = width() >= kWideWidth;
    if (wide != m_wide)
        placeCards(wide);
    // 카드 높이는 여기서 재지 않는다 — viewport의 Resize(eventFilter)가 정확한 시점이다
}

bool SquadPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_scroll->viewport() && event->type() == QEvent::Resize) {
        fitCardHeights();
        // 상단 막대의 오른쪽 끝을 분석 창과 맞춘다 — 스크롤바 폭 + 열의 오른쪽 여백(4)만큼
        const int gutter = m_scroll->width() - m_scroll->viewport()->width() + 4;
        m_topBar->layout()->setContentsMargins(0, 0, gutter, 0);
    }
    return QWidget::eventFilter(watched, event); // 엿보기만 하고 이벤트는 그대로 흘려보낸다
}

QString SquadPage::typeName(const QString &key) const
{
    const tok::TypeColor *type = typechip::find(key);
    return type ? typechip::label(*type, m_state->language()) : key;
}

void SquadPage::refresh()
{
    const Language language = m_state->language();
    const int generation = m_session->generation();
    const Squad &squad = m_session->squad();
    // 이름 칸: 고치는 중이면 그대로 둔다. 단 다른 스쿼드(세대 · 버전)로 바뀌었으면 새 이름을 먼저
    // 넣고 포커스를 뺀다 — 칩은 누를 때 포커스를 가져가지 않아서, 그대로 두면 옛 이름이 포커스가
    // 빠질 때(editingFinished) 새 스쿼드에 저장된다
    const QString squadKey = QStringLiteral("%1/%2").arg(generation).arg(m_session->version());
    if (!m_name->hasFocus() || squadKey != m_nameSquad) {
        m_name->setText(squad.name.isEmpty() ? m_session->defaultName() : squad.name);
        m_name->clearFocus();
    }
    m_nameSquad = squadKey;
    m_rule->setText(tr("%1세대 규칙").arg(generation));
    m_game->setGames(m_session->games(), language, m_session->version());
    m_game->setVisible(m_session->games().size() > 1); // 게임이 하나뿐인 세대는 고를 것이 없다
    m_pips->update();
    m_count->setText(QStringLiteral("%1 / 6").arg(squad.filled()));
    refreshShuttleButton();
    m_undoButton->setEnabled(m_session->canUndo());
    m_redoButton->setEnabled(m_session->canRedo());
    m_clearButton->setEnabled(squad.filled() > 0 || !squad.shuttle.isEmpty());

    if (m_selected >= 0 && !m_session->detail(m_selected).isValid())
        m_selected = -1;
    const QString hint = suggestion();
    for (SlotCard *card : std::as_const(m_cards)) {
        card->refresh(language);
        card->setSelected(m_cards.indexOf(card) == m_selected);
        card->setAlert(false);
        card->setSuggestion(hint);
        const int slot = int(m_cards.indexOf(card));
        const PokemonDetail &detail = m_session->detail(slot);
        card->setWarning(detail.isValid() ? warningFor(slot, detail.speciesId) : QString());
    }
    refreshAnalysis();
}

void SquadPage::refreshAnalysis()
{
    const Language language = m_state->language();
    const int generation = m_session->generation();
    const SquadAnalysis &analysis = m_session->analysis();
    const TypeChart &chart = m_session->chart();

    // 머리: "4세대 상성표 · 17타입" — 그 세대에 없는 것(페어리 등)을 굳이 적지 않는다. 표 자체가
    // 세대별이고(Repository::typeChart), 없는 타입은 히트맵에서 사선 열로 보인다
    m_analysis->setTitle(tr("스쿼드 분석"),
                         tr("%1세대 상성표 · %2타입").arg(generation).arg(chart.types.size()));
    m_emptyAnalysis->setVisible(analysis.filled == 0);
    m_analysisBody->setVisible(analysis.filled > 0);

    // 문제 목록
    auto nameOf = [&](int slot) {
        return m_session->detail(slot).name.text(language);
    };
    QList<ProblemList::Row> rows;
    QSet<QString> problemTypes;
    for (const Problem &problem : analysis.problems) {
        const QString key = keyOf(problem.type);
        const QString type = typeName(key);
        const std::size_t t = std::size_t(problem.type);
        ProblemList::Row row;
        row.typeKey = key;
        problemTypes.insert(key);
        if (problem.kind == ProblemKind::NoCoverage) {
            row.offense = true;
            row.title = tr("%1 — 효과가 굉장한 기술 없음").arg(type);
            QStringList attackers;
            for (const QString &attack : chart.types)
                if (chart.at(attack, key) > 1.0)
                    attackers.append(typeName(attack));
            row.detail = tr("%1 타입 공격 기술이 스쿼드에 없어요")
                                 .arg(attackers.join(QStringLiteral(" · ")));
        } else {
            const int weak = analysis.weak[t];
            const int resist = analysis.resist[t];
            if (problem.kind == ProblemKind::WeakStack)
                row.title = tr("%1 — 약점 %2, 받아낼 포켓몬 %3").arg(type).arg(weak).arg(resist);
            else if (problem.kind == ProblemKind::NoResist)
                row.title = tr("%1 — 약점 %2, 받아낼 포켓몬 없음").arg(type).arg(weak);
            else
                row.title = tr("%1 — 4배 약점").arg(type);
            QStringList hurt;
            QStringList tank;
            for (int slot = 0; slot < 6; ++slot) {
                if (!m_session->detail(slot).isValid())
                    continue;
                const double m = analysis.received[std::size_t(slot)][t];
                if (m > 1.0)
                    hurt.append(nameOf(slot) + QLatin1Char(' ') + times(m));
                else if (m < 1.0)
                    tank.append(nameOf(slot) + QLatin1Char(' ') + times(m));
            }
            row.detail
                    = hurt.join(QStringLiteral(" · ")) + QStringLiteral(" / ")
                      + (tank.isEmpty() ? tr("받아낼 포켓몬이 없어요")
                                        : tr("받아냄: %1").arg(tank.join(QStringLiteral(" · "))));
        }
        rows.append(row);
    }
    m_problemRows = rows;
    m_problemEmptyText = tr("문제가 없어요 — 약점이 고르게 나뉘어 있어요");
    m_problems->setRows(rows, m_problemEmptyText, language);
    if (m_dialogProblems) // 크게 보기 창이 열려 있으면 같이
        m_dialogProblems->setRows(rows, m_problemEmptyText, language);
    const int count = int(analysis.problems.size());
    m_problemCount->setText(count == 0 ? QString() : tr("%1개").arg(count));
    m_problemPill->setVisible(analysis.filled > 0);
    m_problemPill->setProperty("ok", count == 0);
    m_problemPill->setText(count == 0 ? tr("✓ 문제 없음") : tr("⚠ 문제 %1").arg(count));
    style()->polish(m_problemPill);
    m_problemPill->adjustSize();
    m_problemPill->updateGeometry();

    m_heatmap->refresh(language);
    m_heatmap->setSelectedSlot(m_selected);
    m_heatmap->setProblemTypes(problemTypes);
    m_heatmap->setHotType(QString());

    // 물리 · 특수(그 세대 규칙으로 판정된 값 — 규칙 설명은 "N세대 규칙" 알약 툴팁에)
    const MoveSplit &s = analysis.split;
    const int total = s.physical + s.special + s.status + s.empty;
    m_moveCount->setText(tr("기술 %1 / %2").arg(total - s.empty).arg(total));
    m_split->setSplit(s);
    refreshResources();
    syncAnalysisHeight(); // 내용(문제 수)이 바뀌었다 — 좁은 배치의 최소 높이 갱신
}

void SquadPage::refreshResources()
{
    // 채운 기술 칸마다 배우는 방법을 정해 core(resourceledger)에 센다. 자력 · 하트비늘은 멤버의
    // 기술 목록(movereach가 매긴 떠올리기 표시)에서, 기술머신 공급 · 가르침 비용은 입수 사전에서
    const Language language = m_state->language();
    const QString version = m_session->version();
    const QString group = m_session->versionGroup();
    std::vector<resourceledger::TeachPlan> plans;
    std::map<std::string, resourceledger::MachineSupply> supplies;
    QHash<QString, QString> machineNames; // "tm26" → "TM26 지진"
    struct TutorUse
    {
        QString name;
        int count = 0;
        QList<std::pair<int, QString>> cost; // 한 번 배우는 비용
    };
    QMap<int, TutorUse> tutors; // move id 순

    auto costsOf = [](const QList<std::pair<int, QString>> &costs) {
        std::vector<resourceledger::Cost> out;
        for (const auto &[amount, unit] : costs)
            out.push_back({amount, unit.toStdString()});
        return out;
    };

    for (int slot = 0; slot < 6; ++slot) {
        const PokemonDetail &detail = m_session->detail(slot);
        if (!detail.isValid())
            continue;
        for (const auto &slotMove : m_session->slotMoves(slot)) {
            if (!slotMove || !slotMove->learnable)
                continue;
            const MoveEntry &move = slotMove->move;
            resourceledger::TeachPlan plan;
            plan.slot = slot;
            // 자력(레벨업)이 먼저다 — 떠올리기 표시가 없는 줄이 하나라도 있으면 공짜
            const MoveEntry *level = nullptr;
            for (const MoveEntry &entry : detail.levelMoves)
                if (entry.moveId == move.moveId && (!level || !entry.needsReminder))
                    level = &entry;
            const MoveEntry *machine = nullptr;
            for (const MoveEntry &entry : detail.machineMoves)
                if (entry.moveId == move.moveId)
                    machine = &entry;
            bool tutor = false;
            for (const MoveEntry &entry : detail.tutorMoves)
                tutor = tutor || entry.moveId == move.moveId;
            if (level) {
                plan.means = level->needsReminder ? resourceledger::Means::Reminder
                                                  : resourceledger::Means::LevelUp;
            } else if (machine && !machine->hiddenMachine) { // 비전머신은 몇 번이든 가르친다
                plan.means = resourceledger::Means::Machine;
                plan.machine = machine->machineItem.toStdString();
                if (!supplies.count(plan.machine)) {
                    const guidebook::ItemSupply supply
                            = guidebook::itemSupply(group, machine->machineItem, version);
                    supplies[plan.machine] = {supply.known, supply.copies, supply.repeatable,
                                              costsOf(supply.repeatCost), supply.postGameOnly};
                }
                machineNames.insert(machine->machineItem,
                                    QStringLiteral("TM%1 %2")
                                            .arg(machine->machineNumber, 2, 10, QLatin1Char('0'))
                                            .arg(move.name.text(language)));
            } else if (tutor) {
                plan.means = resourceledger::Means::Tutor;
                const auto cost = guidebook::tutorCostAmounts(group, move.identifier, version);
                plan.cost = costsOf(cost);
                TutorUse &use = tutors[move.moveId];
                use.name = move.name.text(language);
                ++use.count;
                use.cost = cost;
            } // 그 밖(알 기술 · 못 배우는 기술)은 집계 밖
            plans.push_back(plan);
        }
    }
    const resourceledger::Ledger ledger = resourceledger::tally(plans, supplies);

    // 합계(제목 오른쪽): 하트비늘 → BP · 코인 · 조각 · 돈 순
    QStringList totals;
    if (ledger.heartScales > 0)
        totals.append(guidebook::costLabel(ledger.heartScales, QStringLiteral("heart-scale")));
    static const QStringList unitOrder = {
            QStringLiteral("bp"),           QStringLiteral("coins"),
            QStringLiteral("red-shard"),    QStringLiteral("blue-shard"),
            QStringLiteral("yellow-shard"), QStringLiteral("green-shard"),
            QStringLiteral("money"),
    };
    std::vector<resourceledger::Cost> costs = ledger.totals;
    std::stable_sort(costs.begin(), costs.end(), [](const auto &a, const auto &b) {
        const auto rank = [](const std::string &unit) {
            const qsizetype i = unitOrder.indexOf(QString::fromStdString(unit));
            return i < 0 ? unitOrder.size() : i; // 목록 밖 단위는 뒤에(이미 이름 순)
        };
        return rank(a.unit) < rank(b.unit);
    });
    for (const resourceledger::Cost &cost : costs)
        totals.append(guidebook::costLabel(cost.amount, QString::fromStdString(cost.unit)));

    // 줄: 기술머신(모자람 경고 · 구매 · 엔딩 후) → 가르침. 공짜 1개로 해결되는 기술머신은 줄도
    // 쓰지 않는다
    QStringList lines;
    const QString warn = QStringLiteral("<span style=\"color:%1\">⚠</span> ")
                                 .arg(QColor(tok::kRedText).name());
    for (const resourceledger::MachineUse &use : ledger.machines) {
        const QString name = machineNames.value(QString::fromStdString(use.machine));
        const int uses = int(use.members.size());
        const QString head = uses > 1 ? tr("%1 ×%2").arg(name).arg(uses) : name;
        QStringList notes;
        if (use.shortfall)
            notes.append(tr("이 게임에 %1개뿐이에요").arg(use.supply.copies));
        if (use.bought > 0) {
            QStringList price;
            for (const resourceledger::Cost &cost : use.supply.repeatCost)
                price.append(guidebook::costLabel(cost.amount * use.bought,
                                                  QString::fromStdString(cost.unit)));
            notes.append(price.isEmpty() ? tr("%1개는 더 구해요").arg(use.bought)
                                         : tr("%1개는 더 구해요 (%2)")
                                                   .arg(use.bought)
                                                   .arg(price.join(QStringLiteral(" + "))));
        }
        if (use.supply.postGameOnly)
            notes.append(tr("관동 — 엔딩 후"));
        if (notes.isEmpty())
            continue;
        lines.append((use.shortfall ? warn : QString()) + head + QStringLiteral(" — ")
                     + notes.join(QStringLiteral(" · ")));
    }
    for (const TutorUse &use : tutors) {
        QStringList price;
        for (const auto &[amount, unit] : use.cost)
            price.append(guidebook::costLabel(amount * use.count, unit));
        if (price.isEmpty())
            continue; // 공짜 가르침은 셀 것이 없다
        const QString head = use.count > 1 ? tr("%1 ×%2").arg(use.name).arg(use.count) : use.name;
        lines.append(tr("%1 — 가르침 %2").arg(head, price.join(QStringLiteral(" + "))));
    }

    m_resourceBox->setVisible(!totals.isEmpty() || !lines.isEmpty());
    m_resourceTotal->setText(totals.join(QStringLiteral(" · ")));
    m_resources->setText(lines.join(QStringLiteral("<br>")));
    m_resources->setVisible(!lines.isEmpty());
}

QString SquadPage::suggestion() const
{
    // 문제를 함께 풀 포켓몬: 방어 문제 타입을 받아내고, 공격 문제 타입을 칠 기술을 가진
    const SquadAnalysis &analysis = m_session->analysis();
    const TypeChart &chart = m_session->chart();
    QStringList defense;
    QStringList offense;
    int solvable = 0;
    for (const Problem &problem : analysis.problems) {
        const QString key = keyOf(problem.type);
        if (problem.kind == ProblemKind::NoCoverage) {
            ++solvable;
            for (const QString &attack : chart.types) {
                const QString name = typeName(attack);
                if (chart.at(attack, key) > 1.0 && !offense.contains(name) && offense.size() < 3)
                    offense.append(name);
            }
        } else if (!defense.contains(typeName(key))) {
            ++solvable;
            if (defense.size() < 3)
                defense.append(typeName(key));
        }
    }
    const QString d = defense.join(QStringLiteral(" · "));
    const QString o = offense.join(QStringLiteral(" · "));
    if (!defense.isEmpty() && !offense.isEmpty())
        return tr("%1 공격에 강하고 %2 기술을 가진 포켓몬이면 문제 %3건이 함께 풀려요.")
                .arg(d, o)
                .arg(solvable);
    if (!defense.isEmpty())
        return tr("%1 공격에 강한 포켓몬이면 문제 %2건이 풀려요.").arg(d).arg(solvable);
    if (!offense.isEmpty())
        return tr("%1 기술을 가진 포켓몬이면 문제 %2건이 풀려요.").arg(o).arg(solvable);
    return analysis.filled == 0 ? tr("이 세대의 포켓몬만 고를 수 있어요.")
                                : tr("빈 자리에 포켓몬을 더해 보세요.");
}

QString SquadPage::warningFor(int slot, int speciesId) const
{
    // 경고만 한다(고르는 것은 막지 않는다 — 교환 · 교배 · 치트로 얼마든지 가능하다)
    const Language language = m_state->language();
    const QString game = m_session->versionGroup();
    const int line = starters::lineOf(game, speciesId);
    for (int other = 0; other < int(kSquadSize); ++other) {
        const PokemonDetail &detail = m_session->detail(other);
        if (other == slot || !detail.isValid())
            continue;
        if (detail.speciesId == speciesId)
            return tr("이미 멤버에 있어요(%1번 자리)").arg(other + 1);
    }
    // 진화 전 · 후 관계(갈래가 다른 형제 — 블래키 · 에브이 진화형끼리 — 는 경고하지 않는다)
    if (m_evolvesFrom.isEmpty())
        m_evolvesFrom = m_repository->evolvesFrom();
    auto isAncestor = [this](int ancestor, int species) {
        for (int s = m_evolvesFrom.value(species); s > 0; s = m_evolvesFrom.value(s))
            if (s == ancestor)
                return true;
        return false;
    };
    for (int other = 0; other < int(kSquadSize); ++other) {
        const PokemonDetail &detail = m_session->detail(other);
        if (other == slot || !detail.isValid())
            continue;
        if (isAncestor(speciesId, detail.speciesId))
            return tr("진화한 모습(%1)이 이미 있어요").arg(detail.name.text(language));
        if (isAncestor(detail.speciesId, speciesId))
            return tr("진화 전 모습(%1)이 이미 있어요").arg(detail.name.text(language));
    }
    if (line < 0)
        return {};
    for (int other = 0; other < int(kSquadSize); ++other) {
        const PokemonDetail &detail = m_session->detail(other);
        if (other == slot || !detail.isValid())
            continue;
        if (starters::lineOf(game, detail.speciesId) >= 0) // 한 정주행에 스타팅은 하나
            return tr("이미 스타팅 포켓몬(%1)이 있어요").arg(detail.name.text(language));
    }
    return {};
}

void SquadPage::selectSlot(int slot)
{
    m_selected = m_selected == slot ? -1 : slot; // 다시 누르면 풀린다
    for (int i = 0; i < m_cards.size(); ++i)
        m_cards.at(i)->setSelected(i == m_selected);
    m_heatmap->setSelectedSlot(m_selected);
}

void SquadPage::slideTo(QWidget *card, const QPoint &target, int durationMs)
{
    // 카드마다 애니메이션 하나: 이미 움직이는 중이면 그 자리에서 새 목표로 다시 출발한다
    QPropertyAnimation *slide = m_slides.value(card);
    if (!slide) {
        slide = new QPropertyAnimation(card, "pos", this);
        slide->setEasingCurve(QEasingCurve::OutCubic);
        m_slides.insert(card, slide);
    }
    slide->stop();
    if (card->pos() == target)
        return;
    slide->setDuration(durationMs);
    slide->setStartValue(card->pos());
    slide->setEndValue(target);
    slide->start();
}

void SquadPage::onDragStarted(int slot, const QPoint &globalPos)
{
    if (m_dragging)
        return;
    m_dragging = true;
    // 지금 자리(레이아웃이 잡아 둔 칸)를 재고 레이아웃을 멈춘다 — 끄는 동안 카드는 직접 옮긴다
    m_cells.clear();
    m_order.clear();
    for (int i = 0; i < m_cards.size(); ++i) {
        m_cells.append(m_cards.at(i)->geometry());
        m_order.append(i);
    }
    m_grid->setEnabled(false);
    SlotCard *card = m_cards.at(slot);
    card->raise(); // 다른 카드 위로
    // 들어 올린 느낌: 먹색 그림자(아래로 8px, 흐림 18)
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 8);
    QColor ink(tok::kInk);
    ink.setAlpha(110);
    shadow->setColor(ink);
    card->setGraphicsEffect(shadow);
    m_dragOffset = m_cardArea->mapFromGlobal(globalPos) - card->pos();
}

void SquadPage::onDragMoved(int slot, const QPoint &globalPos)
{
    if (!m_dragging)
        return;
    SlotCard *card = m_cards.at(slot);
    if (QPropertyAnimation *slide = m_slides.value(card))
        slide->stop();
    // 카드는 마우스를 따라가되 카드 영역 밖으로는 나가지 않는다
    const QPoint mouse = m_cardArea->mapFromGlobal(globalPos);
    QPoint topLeft = mouse - m_dragOffset;
    topLeft.setX(std::clamp(topLeft.x(), 0, std::max(0, m_cardArea->width() - card->width())));
    topLeft.setY(std::clamp(topLeft.y(), 0, std::max(0, m_cardArea->height() - card->height())));
    card->move(topLeft);

    // 끄는 카드의 가운데에서 가장 가까운 칸 = 놓일 자리. 바뀌면 나머지 카드가 밀리거나 당겨진다
    const QPoint center = QRect(topLeft, card->size()).center();
    int target = 0;
    for (int i = 1; i < m_cells.size(); ++i)
        if ((m_cells.at(i).center() - center).manhattanLength()
            < (m_cells.at(target).center() - center).manhattanLength())
            target = i;
    const int current = int(m_order.indexOf(slot));
    if (target != current) {
        m_order.move(current, target);
        for (int i = 0; i < m_order.size(); ++i)
            if (m_order.at(i) != slot)
                slideTo(m_cards.at(m_order.at(i)), m_cells.at(i).topLeft(), kShiftMs);
    }

    // 스크롤 영역 위 · 아래 끝에 닿으면 그쪽으로 조금씩 굴린다(좁은 창: 분석 창이 카드 아래)
    const QPoint inViewport = m_scroll->viewport()->mapFromGlobal(globalPos);
    QScrollBar *bar = m_scroll->verticalScrollBar();
    if (inViewport.y() < kAutoScrollMargin)
        bar->setValue(bar->value() - kAutoScrollStep);
    else if (inViewport.y() > m_scroll->viewport()->height() - kAutoScrollMargin)
        bar->setValue(bar->value() + kAutoScrollStep);
}

void SquadPage::onDragFinished(int slot)
{
    if (!m_dragging)
        return;
    // 놓은 자리로 미끄러져 들어간 뒤 순서를 저장한다
    const int to = int(m_order.indexOf(slot));
    SlotCard *card = m_cards.at(slot);
    slideTo(card, m_cells.at(to).topLeft(), kDropMs);
    QPropertyAnimation *slide = m_slides.value(card);
    if (slide && slide->state() == QAbstractAnimation::Running)
        connect(
                slide, &QPropertyAnimation::finished, this,
                [this, slot, to] { finishDrag(slot, to); }, Qt::SingleShotConnection);
    else
        finishDrag(slot, to);
}

void SquadPage::finishDrag(int from, int to)
{
    for (QPropertyAnimation *slide : std::as_const(m_slides))
        slide->stop();
    m_cards.at(from)->setGraphicsEffect(nullptr); // 그림자를 지운다(효과 객체도 함께 지워진다)
    // 선택한 자리도 같이 옮긴다
    if (m_selected == from)
        m_selected = to;
    else if (from < to && m_selected > from && m_selected <= to)
        --m_selected;
    else if (to < from && m_selected >= to && m_selected < from)
        ++m_selected;
    // 카드 위젯은 자리 번호에 묶여 있다(n번 카드 = n번 자리). 데이터를 옮기고 레이아웃을 되살리면
    // 각 카드가 제 칸으로 돌아가며 새 자리의 내용을 그린다 — 화면에는 놓은 모습 그대로 보인다
    m_session->moveSlot(from, to);
    m_grid->setEnabled(true);
    m_grid->invalidate();
    m_grid->activate();
    m_dragging = false;
    if (from == to)
        refresh(); // 데이터가 그대로라 changed가 오지 않는다 → 선택 표시만 다시
    qCInfo(lcUi) << "squad slot" << from << "moved to" << to;
}

void SquadPage::onProblemHovered(int row)
{
    const auto &problems = m_session->analysis().problems;
    QList<int> members;
    QString key;
    if (row >= 0 && row < int(problems.size())) {
        members = QList<int>(problems[std::size_t(row)].members.begin(),
                             problems[std::size_t(row)].members.end());
        key = keyOf(problems[std::size_t(row)].type);
    }
    for (int i = 0; i < m_cards.size(); ++i)
        m_cards.at(i)->setAlert(members.contains(i));
    m_heatmap->setHotType(key);
}

void SquadPage::onProblemClicked(int row)
{
    const auto &problems = m_session->analysis().problems;
    if (row < 0 || row >= int(problems.size()))
        return;
    m_scroll->ensureWidgetVisible(m_heatmap, 0, 40);
    m_heatmap->flashType(keyOf(problems[std::size_t(row)].type));
}

void SquadPage::showProblemDialog()
{
    // 분석 칸은 좁아서 제목 · 설명이 … 으로 잘린다. 큰 창에서는 전부 보인다.
    // 줄 위에 마우스 · 클릭은 본 화면과 같은 동작(카드 경고 · 히트맵 열 강조 · 깜빡임)
    QDialog dialog(this);
    dialog.setWindowTitle(tr("문제 항목"));
    dialog.resize(kProblemDialogSize);
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);

    QHBoxLayout *heading = new QHBoxLayout;
    heading->setSpacing(10);
    QLabel *title = new QLabel(tr("문제 항목"));
    title->setObjectName(QStringLiteral("dexSectionLabel"));
    heading->addWidget(title);
    QLabel *pill = new QLabel(m_problemPill->text());
    pill->setObjectName(QStringLiteral("squadProblemPill"));
    pill->setProperty("ok", m_problemRows.isEmpty());
    heading->addWidget(pill);
    heading->addStretch();
    QLabel *note = new QLabel(tr("줄을 누르면 히트맵에서 그 타입 열이 깜빡여요"));
    note->setObjectName(QStringLiteral("squadNote"));
    heading->addWidget(note);
    layout->addLayout(heading);

    ProblemList *list = new ProblemList;
    list->setRows(m_problemRows, m_problemEmptyText, m_state->language());
    connect(list, &ProblemList::rowHovered, this, &SquadPage::onProblemHovered);
    connect(list, &ProblemList::rowClicked, this, &SquadPage::onProblemClicked);
    QScrollArea *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("squadScroll"));
    scroll->setWidgetResizable(true);
    scroll->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidget(list);
    layout->addWidget(scroll, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    buttons->addStretch();
    ShadowButton *close = new ShadowButton(ShadowButton::Variant::Secondary);
    close->setText(tr("닫기"));
    connect(close, &ShadowButton::clicked, &dialog, &QDialog::reject);
    buttons->addWidget(close);
    layout->addLayout(buttons);

    m_dialogProblems = list; // 열려 있는 동안 분석이 바뀌면 refreshAnalysis가 같이 갱신한다
    dialog.exec();
    m_dialogProblems = nullptr;
    onProblemHovered(-1); // 창을 닫으면 카드 경고 · 열 강조를 푼다
}

void SquadPage::refreshShuttleButton()
{
    // 7세대부터는 비전머신이 내장 시스템(포켓라이드 · 포켓치 …)으로 대체돼 셔틀이 필요 없다
    m_shuttleButton->setVisible(featuresOf(m_session->generation()).hiddenMachines);
    const PokemonDetail &shuttle = m_session->shuttleDetail();
    if (!shuttle.isValid()) {
        m_shuttleButton->setText(tr("+ 비전셔틀"));
        m_shuttleButton->setIcon(QIcon());
        return;
    }
    m_shuttleButton->setText(tr("비전셔틀 · %1").arg(shuttle.name.text(m_state->language())));
    // 박스 아이콘(없으면 받기 시작 — ready가 다시 부른다)
    const QString key = QString::number(shuttle.pokemonId);
    const QString path = m_pokemonIcons->path(key);
    if (path.isEmpty()) {
        m_pokemonIcons->request(key);
        m_shuttleButton->setIcon(QIcon());
        return;
    }
    const QPixmap icon = squadpaint::trimmedIcon(path, 22);
    m_shuttleButton->setIcon(icon);
    m_shuttleButton->setIconSize(icon.size());
}

void SquadPage::showShuttleDialog()
{
    ShuttleDialog dialog(m_repository, m_state, m_session, m_pokemonIcons, this);
    connect(&dialog, &ShuttleDialog::pickRequested, this,
            [this] { pickPokemon(SquadSession::kShuttleSlot); });
    dialog.exec();
}

void SquadPage::exportSquad()
{
    squadfile::Portable portable {m_session->generation(), m_session->version(),
                                  m_session->squad()};
    if (portable.squad.name.isEmpty()) // 파일만 봐도 어느 스쿼드인지 알게
        portable.squad.name = m_session->defaultName();
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getSaveFileName(
            this, tr("스쿼드 내보내기"), dir + QLatin1Char('/') + squadfile::fileName(portable),
            tr("PokeSix 스쿼드 (*.json)"));
    if (path.isEmpty())
        return;
    QString error;
    if (squadfile::save(path, portable, &error))
        flashStatus(tr("✓ 스쿼드를 내보냈어요"));
    else
        QMessageBox::warning(this, tr("스쿼드 내보내기"), error);
}

void SquadPage::importSquad()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString path = QFileDialog::getOpenFileName(this, tr("스쿼드 불러오기"), dir,
                                                      tr("PokeSix 스쿼드 (*.json)"));
    if (path.isEmpty())
        return;
    QString error;
    const std::optional<squadfile::Portable> loaded = squadfile::load(path, &error);
    if (!loaded) {
        QMessageBox::warning(this, tr("스쿼드 불러오기"), error);
        return;
    }
    // 파일의 세대 · 게임으로 옮겨 가서 그 스쿼드를 바꾼다
    const QString version = m_repository->resolveVersion(loaded->generation, loaded->game);
    if (version.isEmpty()) {
        QMessageBox::warning(this, tr("스쿼드 불러오기"),
                             tr("%1세대는 아직 몰라요").arg(loaded->generation));
        return;
    }
    const Squad existing = m_store->squad(loaded->generation, version);
    if (existing.filled() > 0 || !existing.shuttle.isEmpty()) {
        const QString name
                = loaded->squad.name.isEmpty() ? tr("불러온 스쿼드") : loaded->squad.name;
        if (QMessageBox::question(
                    this, tr("스쿼드 불러오기"),
                    tr("지금 저장된 스쿼드를 '%1'(으)로 바꿔요. 계속할까요?").arg(name))
            != QMessageBox::Yes)
            return;
    }
    if (m_state->generation() != loaded->generation)
        m_state->setGeneration(loaded->generation);
    if (m_state->game() != version)
        m_state->setGame(version);
    m_session->replaceSquad(loaded->squad);
    flashStatus(tr("✓ 스쿼드를 불러왔어요"));
}

QPixmap SquadPage::squadImage()
{
    // 화면에 그려진 카드 영역 · 분석 창을 그대로 떠서 제목 띠와 함께 한 장으로 엮는다
    const QPixmap cards = m_cardArea->grab();
    const QPixmap analysis = m_analysis->grab();
    const qreal dpr = devicePixelRatioF();
    const QSizeF cardSize = cards.deviceIndependentSize();
    const QSizeF analysisSize = analysis.deviceIndependentSize();
    const int pad = 20;
    const int titleHeight = 56;
    const int width = int(cardSize.width() + analysisSize.width()) + pad * 3;
    const int height = int(qMax(cardSize.height(), analysisSize.height())) + titleHeight + pad;

    QPixmap image(int(width * dpr), int(height * dpr));
    image.setDevicePixelRatio(dpr);
    image.fill(QColor(tok::kPaper));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    // 제목 띠: 스쿼드 이름 — 오른쪽에 세대 규칙과 앱 이름
    painter.setFont(theme::font(theme::kFamilyTitle, 26));
    painter.setPen(QColor(tok::kText1));
    painter.drawText(QRect(pad, 0, width - pad * 2, titleHeight), Qt::AlignLeft | Qt::AlignVCenter,
                     m_name->text());
    painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
    painter.setPen(QColor(tok::kText3));
    painter.drawText(QRect(pad, 0, width - pad * 2, titleHeight), Qt::AlignRight | Qt::AlignVCenter,
                     tr("PokeSix · %1세대 규칙").arg(m_session->generation()));
    painter.drawPixmap(QPointF(pad, titleHeight), cards);
    painter.drawPixmap(QPointF(pad * 2 + cardSize.width(), titleHeight), analysis);
    return image;
}

void SquadPage::copyImage()
{
    QGuiApplication::clipboard()->setPixmap(squadImage());
    flashStatus(tr("✓ 이미지를 복사했어요"));
}

void SquadPage::saveImage()
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    const QString path
            = QFileDialog::getSaveFileName(this, tr("이미지 저장"),
                                           dir
                                                   + QStringLiteral("/pokesix-squad-%1-%2.png")
                                                             .arg(m_session->generation())
                                                             .arg(m_session->version()),
                                           tr("PNG 이미지 (*.png)"));
    if (path.isEmpty())
        return;
    if (squadImage().save(path))
        flashStatus(tr("✓ 이미지를 저장했어요"));
    else
        QMessageBox::warning(this, tr("이미지 저장"), tr("파일에 쓰지 못했어요"));
}

void SquadPage::flashStatus(const QString &text)
{
    m_saveStatus->setProperty("state", QString());
    m_saveStatus->setText(text);
    style()->polish(m_saveStatus);
    QTimer::singleShot(2500, m_saveStatus, [this, text] {
        if (m_saveStatus->text() == text) // 그 사이 저장 상태가 바뀌었으면 그대로 둔다
            m_saveStatus->setText(tr("✓ 자동 저장됨"));
    });
}

void SquadPage::showMemberDetail(int slot)
{
    const int pokemonId = m_session->member(slot).pokemonId;
    if (pokemonId <= 0)
        return;
    // 도감 상세를 그대로 모달로 띄운다 — 그림 · 종족값 · 특성 · 상성 · 진화(조건) · 기술(하트비늘
    // · 획득처 · NPC 가르침 비용) 전부. 기준 게임은 스쿼드의 게임.
    QDialog dialog(this);
    dialog.setWindowTitle(m_session->detail(slot).name.text(m_state->language()));
    QVBoxLayout *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(
            kPageMargins); // 도감 화면과 같은 여백(버튼 · 스크롤바가 창에 붙지 않게)
    DexDetailPage *detail = new DexDetailPage(m_repository, m_state);
    detail->setFollowsAppGame(false); // 모달의 칩은 모달 안에서만(뒤의 스쿼드는 그대로)
    detail->showPokemon(pokemonId, m_session->version());
    layout->addWidget(detail);
    connect(detail, &DexDetailPage::backRequested, &dialog, &QDialog::accept); // [← 목록] = 닫기
    // 내용 폭 1320(도감 상세가 넉넉한 폭) + 여백. 작은 화면에서는 화면 안에 들어오게 줄인다
    const QSize wanted(1320 + kPageMargins.left() + kPageMargins.right(),
                       840 + kPageMargins.top() + kPageMargins.bottom());
    dialog.resize(wanted.boundedTo(screen()->availableGeometry().size() * 0.94));
    dialog.exec();
}

void SquadPage::pickPokemon(int slot)
{
    // kShuttleSlot = 비전셔틀(7번째 멤버): 같은 선택 창을 쓰되 중복 경고 없이, 셔틀에 저장한다
    const bool shuttle = slot == SquadSession::kShuttleSlot;
    const Language language = m_state->language();
    const int generation = m_session->generation();
    // 도감 칩: 그 세대의 지방 도감. 처음엔 스쿼드 게임의 도감(Pt → 신오 Pt), 한 번 바꾸면 그 게임
    // 동안은 마지막에 고른 도감을 기억한다
    const QList<DexInfo> dexes = m_repository->dexesForGeneration(generation);
    const QString game = m_session->versionGroup();
    int dexId = m_pickerDex.value(game, -1);
    if (dexId < 0) {
        dexId = DexFilterBar::kNational;
        for (const DexInfo &dex : dexes)
            if (dex.versionGroups.contains(game) && !dexstyle::dex(dex.identifier).hidden) {
                dexId = dex.pokedexId;
                break;
            }
    }
    const QSet<int> elsewhere = m_repository->otherVersionSpecies(m_session->version());
    QList<SpeciesRow> rows; // painter가 참조로 본다 → 도감을 바꾸면 이 목록을 먼저 바꾼다
    QStringList search;
    QStringList warnings; // 줄마다 경고(같은 포켓몬 · 스타팅 둘). 없으면 빈 칸
    int current = -1;
    auto load = [&](int id) {
        rows = id == DexFilterBar::kNational ? m_repository->speciesForGeneration(generation)
                                             : m_repository->speciesForDex(id, generation);
        // 이 버전에서 얻을 수 없는 같은 묶음의 다른 버전 한정 포켓몬은 뺀다
        rows.removeIf(
                [&elsewhere](const SpeciesRow &row) { return elsewhere.contains(row.speciesId); });
        search.clear();
        warnings.clear();
        current = -1;
        for (qsizetype i = 0; i < rows.size(); ++i) {
            const SpeciesRow &row = rows.at(i);
            // 지방 번호 · 전국 번호 둘 다로 찾을 수 있게
            search.append(QStringLiteral("%1 %2 %3")
                                  .arg(row.dexNumber)
                                  .arg(row.speciesId)
                                  .arg(row.name.all()));
            warnings.append(shuttle ? QString() : warningFor(slot, row.speciesId));
            const int chosenId
                    = shuttle ? m_session->shuttle().pokemonId : m_session->member(slot).pokemonId;
            if (row.pokemonId == chosenId)
                current = int(i);
        }
    };
    load(dexId);
    auto heading = [&] {
        return tr("포켓몬 고르기 · %1마리").arg(rows.size());
    };
    SpriteCache *icons = m_pokemonIcons;
    ListPicker picker(
            shuttle ? tr("비전셔틀 고르기") : tr("%1세대 포켓몬 고르기").arg(generation), search,
            [this, &rows, &warnings, icons, language](QPainter &painter, const QRect &r, int i,
                                                      bool) {
                if (i < 0) { // 칸 제목
                    painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold));
                    painter.setPen(QColor(tok::kText3));
                    painter.drawText(QRect(r.left() + 14, r.top(), 60, r.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter, tr("No."));
                    painter.drawText(QRect(r.left() + 112, r.top(), 100, r.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter, tr("이름"));
                    painter.drawText(QRect(r.left() + 290, r.top(), 100, r.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter, tr("타입"));
                    painter.drawText(QRect(r.right() - 90, r.top(), 76, r.height()),
                                     Qt::AlignRight | Qt::AlignVCenter, tr("합계"));
                    return;
                }
                const SpeciesRow &row = rows.at(i);
                const QString warning = warnings.value(i);
                if (!warning.isEmpty()) { // 경고 줄: 빨강 연한 바탕 + 왼쪽 빨강 띠
                    painter.fillRect(r, QColor(tok::kRedTint));
                    painter.fillRect(QRect(r.left(), r.top(), 4, r.height()), QColor(tok::kRed));
                }
                painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
                painter.setPen(QColor(tok::kText3));
                painter.drawText(QRect(r.left() + 14, r.top(), 52, r.height()),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 QStringLiteral("%1").arg(row.dexNumber, 3, 10, QLatin1Char('0')));
                DexRowDelegate::paintPokemonIcon(
                        &painter, QRectF(r.left() + 62, r.top() + 3, 40, r.height() - 6),
                        row.pokemonId, icons);
                painter.setFont(theme::font(theme::kFamilyBody, 14, QFont::ExtraBold));
                painter.setPen(QColor(tok::kText1));
                if (warning.isEmpty()) {
                    painter.drawText(QRect(r.left() + 112, r.top(), 170, r.height()),
                                     Qt::AlignLeft | Qt::AlignVCenter, row.name.text(language));
                } else { // 이름은 위, 경고 문구는 아래 작은 빨강 글자
                    painter.drawText(QRect(r.left() + 112, r.top() + 2, 170, 20),
                                     Qt::AlignLeft | Qt::AlignVCenter, row.name.text(language));
                    painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold));
                    painter.setPen(QColor(tok::kRed));
                    const int width = r.right() - 100 - (r.left() + 112);
                    painter.drawText(QRect(r.left() + 112, r.top() + 21, width, 16),
                                     Qt::AlignLeft | Qt::AlignVCenter,
                                     QFontMetricsF(painter.font())
                                             .elidedText(QStringLiteral("! ") + warning,
                                                         Qt::ElideRight, width));
                }
                // 타입 칩: 경고 줄이면 윗줄(이름과 같은 높이)로 올려 아래 경고 문구와 겹치지 않게
                qreal x = r.left() + 290;
                const qreal chipTop
                        = warning.isEmpty() ? r.center().y() - typechip::kHeight / 2 : r.top() + 2;
                for (const QString &key : row.types)
                    if (const tok::TypeColor *type = typechip::find(key))
                        x += typechip::paint(painter, QPointF(x, chipTop), *type, language)
                             + typechip::kGap;
                painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
                painter.setPen(QColor(tok::kText2));
                painter.drawText(QRect(r.right() - 90, r.top(), 76, r.height()),
                                 Qt::AlignRight | Qt::AlignVCenter,
                                 QStringLiteral("%1").arg(row.total));
            },
            DexRowDelegate::kRowHeight, this);
    connect(icons, &SpriteCache::ready, picker.view()->viewport(), qOverload<>(&QWidget::update));
    picker.setHeading(heading());
    DexFilterBar *filter = new DexFilterBar;
    filter->setDexes(dexes, language, dexId);
    picker.setHeadingWidget(filter);
    connect(filter, &DexFilterBar::dexSelected, &picker, [&, game](int id) {
        load(id);
        picker.setSearchTexts(search);
        picker.setHeading(heading());
        picker.setCurrentRow(current);
        m_pickerDex.insert(game, id);
    });
    picker.showHeader();
    picker.setCurrentRow(current);
    if (picker.exec() != QDialog::Accepted || picker.chosenRow() < 0)
        return;
    const int pokemonId = rows.at(picker.chosenRow()).pokemonId;
    if (shuttle)
        m_session->setShuttlePokemon(pokemonId);
    else
        m_session->setPokemon(slot, pokemonId);
    qCInfo(lcUi) << "squad slot" << (shuttle ? QStringLiteral("shuttle") : QString::number(slot))
                 << "=" << pokemonId;
}

void SquadPage::pickMove(int slot, int index)
{
    const Language language = m_state->language();
    const QList<SquadSession::LearnableMove> rows = m_session->learnableMoves(slot);
    const auto &current = m_session->slotMoves(slot)[std::size_t(index)];
    QStringList search;
    QStringList methods; // "자력(1레벨) · TM26 · NPC 가르침 · 알"
    QList<moveeffect::Parts> effects; // 변화 기술의 효과 줄(공격 ▲2 …). 공격 기술은 비어 있다
    const QStringList userTypes = m_session->detail(slot).types;
    int currentRow = -1;
    for (qsizetype i = 0; i < rows.size(); ++i) {
        const SquadSession::LearnableMove &row = rows.at(i);
        QStringList parts;
        if (row.level >= 0) // 레벨업 = 스스로 배운다("자력")
            parts.append(tr("자력(%1레벨)").arg(row.level));
        if (!row.machine.isEmpty())
            parts.append(row.machine);
        if (row.tutor) {
            const QString cost = guidebook::tutorCost(m_session->versionGroup(),
                                                      row.move.identifier, language);
            parts.append(cost.isEmpty() ? tr("NPC 가르침") : tr("NPC 가르침(%1)").arg(cost));
        }
        if (row.egg)
            parts.append(tr("알"));
        methods.append(parts.join(QStringLiteral(" · ")));
        effects.append(row.move.damageClass == 1
                               ? moveeffect::describe(row.move, m_session->generation(), userTypes,
                                                      language)
                               : moveeffect::Parts());
        // 효과 글자로도 찾을 수 있게("회피율" · "마비")
        search.append(row.move.name.all() + QLatin1Char(' ') + typeName(row.move.type)
                      + QLatin1Char(' ') + methods.last() + QLatin1Char(' ')
                      + moveeffect::plainText(effects.last()));
        if (current && current->move.moveId == row.move.moveId)
            currentRow = int(i);
    }
    const QString title = tr("%1의 기술 · %2개")
                                  .arg(m_session->detail(slot).name.text(language))
                                  .arg(rows.size());
    ListPicker picker(
            title, search,
            [this, &rows, &methods, &effects, language](QPainter &painter, const QRect &r, int i,
                                                        bool) {
                if (i < 0) { // 칸 제목
                    painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold));
                    painter.setPen(QColor(tok::kText3));
                    const std::pair<int, QString> titles[]
                            = {{14, tr("타입")}, {96, tr("기술")}, {236, tr("분류")}};
                    for (const auto &[x, text] : titles)
                        painter.drawText(QRect(r.left() + x, r.top(), 80, r.height()),
                                         Qt::AlignLeft | Qt::AlignVCenter, text);
                    const QString stats[3] = {tr("위력"), tr("명중"), tr("PP")};
                    for (int s = 0; s < 3; ++s)
                        painter.drawText(QRect(r.left() + 270 + s * 46, r.top(), 40, r.height()),
                                         Qt::AlignRight | Qt::AlignVCenter, stats[s]);
                    painter.drawText(QRect(r.left() + 420, r.top(), r.right() - 12 - r.left() - 420,
                                           r.height()),
                                     Qt::AlignRight | Qt::AlignVCenter, tr("배우는 방법"));
                    return;
                }
                const MoveEntry &move = rows.at(i).move;
                // 변화 기술은 두 줄: 위 = 이름 · 분류 · 수치, 아래 = 효과(공격 ▲2 · 상대 마비 …)
                const moveeffect::Parts &effect = effects.at(i);
                const QRect line
                        = effect.isEmpty() ? r : QRect(r.left(), r.top() + 1, r.width(), 20);
                if (const tok::TypeColor *type = typechip::find(move.type))
                    typechip::paint(painter,
                                    QPointF(r.left() + 14, r.center().y() - typechip::kHeight / 2),
                                    *type, language);
                painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
                painter.setPen(QColor(tok::kText1));
                painter.drawText(
                        QRect(r.left() + 96, line.top(), 140, line.height()),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        QFontMetricsF(painter.font())
                                .elidedText(move.name.text(language), Qt::ElideRight, 140));
                squadpaint::paintDamageClass(painter,
                                             QRectF(r.left() + 240, line.center().y() - 9, 22, 18),
                                             move.damageClass);
                painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
                painter.setPen(QColor(tok::kText2));
                auto number = [](int value) {
                    return value > 0 ? QString::number(value) : QStringLiteral("—");
                };
                const QString stats[3]
                        = {number(move.power), number(move.accuracy), QString::number(move.pp)};
                for (int s = 0; s < 3; ++s)
                    painter.drawText(QRect(r.left() + 270 + s * 46, line.top(), 40, line.height()),
                                     Qt::AlignRight | Qt::AlignVCenter, stats[s]);
                if (!effect.isEmpty())
                    moveeffect::paint(painter, QRectF(r.left() + 96, r.top() + 19, 314, 15), effect,
                                      theme::font(theme::kFamilyBody, 11, QFont::Bold));
                painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::Bold));
                painter.setPen(QColor(tok::kText3));
                const int left = r.left() + 420;
                painter.drawText(
                        QRect(left, r.top(), r.right() - 12 - left, r.height()),
                        Qt::AlignRight | Qt::AlignVCenter,
                        QFontMetricsF(painter.font())
                                .elidedText(methods.at(i), Qt::ElideLeft, r.right() - 12 - left));
            },
            36, this);
    picker.showHeader();
    picker.setCurrentRow(currentRow);
    if (current)
        picker.setNoneText(tr("기술 비우기"));
    if (picker.exec() != QDialog::Accepted)
        return;
    m_session->setMove(slot, index,
                       picker.chosenRow() < 0 ? 0 : rows.at(picker.chosenRow()).move.moveId);
}

void SquadPage::pickAbility(int slot, const QPoint &globalPos)
{
    const PokemonDetail &detail = m_session->detail(slot);
    QMenu menu(this);
    for (const AbilityEntry &ability : detail.abilities) {
        QString text = ability.name.text(m_state->language());
        if (ability.hidden)
            text += tr(" (숨겨진 특성)");
        QAction *action = menu.addAction(text);
        action->setCheckable(true);
        action->setChecked(ability.abilityId == m_session->member(slot).abilityId);
        action->setToolTip(ability.effect.text(m_state->language()));
        connect(action, &QAction::triggered, this,
                [this, slot, id = ability.abilityId] { m_session->setAbility(slot, id); });
    }
    menu.setToolTipsVisible(true);
    menu.exec(globalPos);
}

void SquadPage::pickNature(int slot, const QPoint &globalPos)
{
    // 도감 상세와 같은 5×5 성격표 팝업(한 번 쓰고 버린다)
    NaturePicker *picker = new NaturePicker(m_session->natures(), m_session->member(slot).natureId,
                                            m_state->language(), this);
    connect(picker, &NaturePicker::natureChosen, this,
            [this, slot](int id) { m_session->setNature(slot, id); });
    picker->move(globalPos);
    picker->show();
}

void SquadPage::pickItem(int slot)
{
    const Language language = m_state->language();
    const int generation = m_session->generation();
    // 지닌 물건 후보: 그 세대 아이템 중 중요한 물건 · 기술머신을 뺀 것(화면에서 숨기는 분류도 뺀다)
    const ItemFilterProxy::CategoryFilter visible = itemstyle::filterFor(itemstyle::kAll);
    QList<ItemRow> rows;
    for (const ItemRow &item : m_session->items())
        if (item.pocket != QLatin1String("key") && item.pocket != QLatin1String("machines")
            && (!visible || visible(item.category, item.pocket)))
            rows.append(item);
    // 순서: 지닌 물건다운 것(지닌 물건 · 구애 · 타입 강화 · 플레이트 …) → 나무열매 → 나머지
    // (회복약 · 배틀용 · 몬스터볼 · 메일). 같은 무리 안은 게임 번호 순 그대로
    static const QStringList kHeldFirst
            = {QStringLiteral("held-items"),       QStringLiteral("choice"),
               QStringLiteral("type-enhancement"), QStringLiteral("bad-held-items"),
               QStringLiteral("plates"),           QStringLiteral("species-specific"),
               QStringLiteral("effort-training"),  QStringLiteral("jewels"),
               QStringLiteral("mega-stones"),      QStringLiteral("z-crystals"),
               QStringLiteral("memories"),         QStringLiteral("type-protection"),
               QStringLiteral("in-a-pinch"),       QStringLiteral("picky-healing")};
    auto rank = [](const ItemRow &item) {
        const qsizetype held = kHeldFirst.indexOf(item.category);
        if (held >= 0)
            return int(held);
        if (item.pocket == QLatin1String("berries"))
            return 100;
        if (item.pocket == QLatin1String("pokeballs") || item.pocket == QLatin1String("mail"))
            return 300;
        return 200;
    };
    std::stable_sort(rows.begin(), rows.end(),
                     [&](const ItemRow &a, const ItemRow &b) { return rank(a) < rank(b); });
    QStringList search;
    int current = -1;
    for (qsizetype i = 0; i < rows.size(); ++i) {
        search.append(rows.at(i).name.all());
        if (rows.at(i).id == m_session->member(slot).itemId)
            current = int(i);
    }
    SpriteCache *icons = m_itemIcons;
    ListPicker picker(
            tr("지닌 물건 고르기 · %1개").arg(rows.size()), search,
            [&rows, icons, generation, language](QPainter &painter, const QRect &r, int i, bool) {
                const ItemRow &item = rows.at(i);
                const QPixmap pixmap = itemPixmap(icons, item, generation);
                if (!pixmap.isNull())
                    painter.drawPixmap(QRect(r.left() + 12, r.center().y() - 15, 30, 30), pixmap);
                painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
                painter.setPen(QColor(tok::kText1));
                painter.drawText(
                        QRect(r.left() + 52, r.top(), 170, r.height()),
                        Qt::AlignLeft | Qt::AlignVCenter,
                        QFontMetricsF(painter.font())
                                .elidedText(item.name.text(language), Qt::ElideRight, 170));
                painter.setFont(theme::font(theme::kFamilyBody, 12));
                painter.setPen(QColor(tok::kText3));
                const int left = r.left() + 230;
                painter.drawText(QRect(left, r.top(), r.right() - 12 - left, r.height()),
                                 Qt::AlignLeft | Qt::AlignVCenter,
                                 QFontMetricsF(painter.font())
                                         .elidedText(item.effect.text(language), Qt::ElideRight,
                                                     r.right() - 12 - left));
            },
            40, this);
    connect(icons, &SpriteCache::ready, picker.view()->viewport(), qOverload<>(&QWidget::update));
    picker.setCurrentRow(current);
    if (current >= 0)
        picker.setNoneText(tr("물건 없음"));
    if (picker.exec() != QDialog::Accepted)
        return;
    m_session->setItem(slot, picker.chosenRow() < 0 ? 0 : rows.at(picker.chosenRow()).id);
}
} // namespace com::yamada::studio
