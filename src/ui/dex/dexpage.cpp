#include "ui/dex/dexpage.h"

#include "data/models/speciesfilterproxy.h"
#include "data/models/speciestablemodel.h"
#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/searchfield.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStyle>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>
#include <QtMath>

#include <algorithm>
#include <iterator>

namespace {
using namespace com::yamada::studio;

// 화면 공통 여백(02 SCR 공통): 좌우 20 · 위 16 · 아래 20
constexpr QMargins kPageMargins {20, 16, 20, 20};
// 목록 창: 먹선 2 · 반경 8 · 그림자 3 · 빨강 머리 38 (Dex.dc.html의 <main>)
constexpr PanelStyle kListPanel {.outline = 2,
                                 .radius = 8,
                                 .shadow = 3,
                                 .fill = tok::kWhite,
                                 .ink = tok::kInk,
                                 .header = 38,
                                 .headerColor = tok::kRed};
constexpr QMargins kBodyMargins {12, 8, 12, 0};
constexpr int kSearchDebounceMs = 150; // 입력 즉시가 아니라 멈춘 뒤 150ms에 거른다(02 SCR-02)
// TODO(A8): AppState의 현재 세대로 바꾼다. 지금은 세대 버튼과 같은 4(신오)로 고정.
constexpr int kGeneration = 4;

// 열 폭 — CSS grid의 "24px 52px 44px 3fr <타입 fit> 1fr 1fr 1fr 1fr 1fr 1fr 1.2fr"와 같은 생각이다.
// QHeaderView에는 비율(fr · %) 모드가 없어서, 표 폭이 바뀔 때마다 직접 나눈다(layoutColumns).
//   고정 칸: 내용 폭이 정해진 칸(▶ · 번호 · 아이콘, 타입은 칩 두 개 폭을 계산)
//   비율 칸: 남는 폭을 가중치대로 나눠 갖는 칸(이름 · 종족값 · 합계). 최소 폭 아래로는 줄지 않는다.
// 폭에는 칸 좌우 여백 4씩(delegate의 kCellPadding)이 들어 있다.
struct FixedColumn
{
    int column;
    int width;
};
constexpr FixedColumn kFixedColumns[] = {
        {SpeciesTableModel::CursorColumn, 24},
        {SpeciesTableModel::NumberColumn, 52}, // "1025"(코딩 13 굵게) + 여백
        {SpeciesTableModel::IconColumn, 44},   // 아이콘 34 + 여백
        // TypesColumn은 DexRowDelegate::typesColumnWidth()로 (생성자에서)
};

struct FlexColumn
{
    int column;
    qreal weight;
    int minimum;
};
constexpr FlexColumn kFlexColumns[] = {
        {SpeciesTableModel::NameColumn, 3, 96},
        {SpeciesTableModel::HpColumn, 1, 44},
        {SpeciesTableModel::AttackColumn, 1, 44},
        {SpeciesTableModel::DefenseColumn, 1, 44},
        {SpeciesTableModel::SpAttackColumn, 1, 44},
        {SpeciesTableModel::SpDefenseColumn, 1, 44},
        {SpeciesTableModel::SpeedColumn, 1, 44},
        {SpeciesTableModel::TotalColumn, 1.2, 52}, // 세 자리 굵은 14 — 조금 넓게
};

// 목록 창의 최대 폭(CSS max-width). 이보다 넓은 창에서는 가운데에 두고 좌우 여백이 늘어난다 —
// 칸이 끝없이 벌어지면 줄을 따라 읽기 어렵다.
constexpr int kListMaxWidth = 1100;
} // namespace

namespace com::yamada::studio {
DexPage::DexPage(Repository *repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_model(new SpeciesTableModel(this))
    , m_proxy(new SpeciesFilterProxy(this))
    , m_sprites(new SpriteCache(this))
    , m_searchDelay(new QTimer(this))
{
    m_proxy->setSourceModel(m_model); // 프록시는 원본 모델 위에 얹힌다. 뷰는 프록시를 본다

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPageMargins);

    m_panel = new PanelFrame;
    m_panel->setPanelStyle(kListPanel);
    // 창은 페이지 폭을 채운다. 최대 폭(kListMaxWidth)은 resizeEvent가 좌우 여백으로 맞춘다.
    layout->addWidget(m_panel);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(kBodyMargins);
    bodyLayout->setSpacing(8);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_search = new SearchField(tr("이름 · 번호로 찾기"));
    toolbar->addWidget(m_search);
    toolbar->addStretch();
    bodyLayout->addLayout(toolbar);

    m_table = new QTableView;
    m_table->setObjectName(QStringLiteral("dexTable"));
    m_table->setModel(m_proxy);
    m_table->setItemDelegate(
            new DexRowDelegate(m_sprites, m_table)); // 모든 칸을 이 delegate가 그린다
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(DexRowDelegate::kRowHeight); // 줄 높이 40
    // 모든 칸을 Fixed로 두고 폭은 코드가 정한다(사용자가 머리 칸 경계를 끌어 바꾸지 못하게).
    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    m_fixedWidth = 0;
    for (const FixedColumn &fixed : kFixedColumns) {
        header->resizeSection(fixed.column, fixed.width);
        m_fixedWidth += fixed.width;
    }
    const int typesWidth = DexRowDelegate::typesColumnWidth();
    header->resizeSection(SpeciesTableModel::TypesColumn, typesWidth);
    m_fixedWidth += typesWidth;
    header->setHighlightSections(false);
    m_table->setSortingEnabled(true);
    // 기본은 도감 번호 오름차순(사용자 결정. 디자인 기본은 합계 높은 순). 머리 칸을 누르면 바뀐다.
    m_table->sortByColumn(SpeciesTableModel::NumberColumn, Qt::AscendingOrder);
    header->setStretchLastSection(false);
    // 칸 폭의 합이 늘 viewport 폭과 같으므로 가로 스크롤은 필요 없다. 세로 스크롤바는 늘 보이게
    // 해서 (목록 길이에 따라 생겼다 사라지면 viewport 폭이 바뀌어 칸이 출렁인다) 폭 계산을
    // 안정시킨다.
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // 최소 폭 = 고정 칸 + 비율 칸 최소 + 세로 스크롤바. 이보다 좁아지지 않으니 칸이 잘리지 않는다.
    int minimum = m_fixedWidth + m_table->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    for (const FlexColumn &flex : kFlexColumns)
        minimum += flex.minimum;
    m_table->setMinimumWidth(minimum);
    // viewport(칸이 그려지는 안쪽 위젯)의 크기 변화를 엿본다 → 바뀔 때마다 비율 칸을 다시 나눈다.
    m_table->viewport()->installEventFilter(this);
    bodyLayout->addWidget(m_table, 1);

    m_panel->setBody(body);

    // 아이콘 파일이 하나 받아질 때마다 표를 다시 그린다. update()는 그리기를 "예약"만 하고, 이벤트
    // 루프가 여러 번의 예약을 한 번의 paintEvent로 합친다 → 수백 개가 연달아 와도 부담이 없다.
    connect(m_sprites, &SpriteCache::ready, m_table->viewport(), qOverload<>(&QWidget::update));

    // 검색: 글자가 바뀔 때마다 타이머를 다시 건다 → 입력이 150ms 멈추면 한 번만 거른다(디바운스).
    m_searchDelay->setSingleShot(true);
    m_searchDelay->setInterval(kSearchDebounceMs);
    connect(m_search->lineEdit(), &QLineEdit::textChanged, m_searchDelay,
            qOverload<>(&QTimer::start));
    connect(m_searchDelay, &QTimer::timeout, this, [this] {
        m_proxy->setSearchText(m_search->lineEdit()->text());
        updateTitle();
    });

    updateTitle();
}

bool DexPage::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_table->viewport() && event->type() == QEvent::Resize)
        layoutColumns();
    return QWidget::eventFilter(watched, event); // 엿보기만 하고 이벤트는 그대로 흘려보낸다
}

void DexPage::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // 최대 폭: 페이지가 넓으면 좌우 여백을 늘려 창을 가운데에 kListMaxWidth로 둔다.
    // 여백을 바꾸면 레이아웃이 자식(창)만 다시 배치한다 — 이 위젯 자신의 크기는 그대로라 루프가
    // 없다.
    const int side = std::max(kPageMargins.left(), (width() - kListMaxWidth) / 2);
    QWidget::layout()->setContentsMargins(side, kPageMargins.top(), side, kPageMargins.bottom());
}

void DexPage::layoutColumns()
{
    // 남는 폭 = viewport 폭(스크롤바 제외) − 고정 칸. 이걸 가중치대로 나누고, 반올림으로 남거나
    // 모자란 몇 px는 마지막 칸(합계)이 가져간다 → 칸 합이 viewport 폭과 정확히 같다.
    QHeaderView *header = m_table->horizontalHeader();
    const int free = m_table->viewport()->width() - m_fixedWidth;
    qreal totalWeight = 0;
    for (const FlexColumn &flex : kFlexColumns)
        totalWeight += flex.weight;

    int used = 0;
    const int last = static_cast<int>(std::size(kFlexColumns)) - 1;
    for (int i = 0; i <= last; ++i) {
        const FlexColumn &flex = kFlexColumns[i];
        const int share = i == last ? free - used : qFloor(free * flex.weight / totalWeight);
        const int width = std::max(flex.minimum, share);
        header->resizeSection(flex.column, width);
        used += width;
    }
}

void DexPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_loaded)
        load();
}

void DexPage::load()
{
    m_model->setRows(m_repository->speciesForGeneration(kGeneration));
    m_loaded = m_model->rowCount() > 0; // 비어 있으면(DB가 아직 없음) 다음에 보일 때 다시 읽는다
    qCInfo(lcUi) << "dex loaded" << m_model->rowCount() << "species for generation" << kGeneration;
    updateTitle();
}

void DexPage::updateTitle()
{
    // "도감 백과 · 493마리" — 검색 중이면 걸러진 수
    m_panel->setTitle(tr("도감 백과"), tr("%1마리").arg(m_proxy->rowCount()));
}
} // namespace com::yamada::studio
