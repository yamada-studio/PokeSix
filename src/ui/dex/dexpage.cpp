#include "ui/dex/dexpage.h"

#include "data/models/speciesfilterproxy.h"
#include "data/models/speciestablemodel.h"
#include "data/repository/repository.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/searchfield.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

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
constexpr int kSearchWidth = 280;
constexpr int kSearchHeight = 36;
constexpr int kSearchDebounceMs = 150; // 입력 즉시가 아니라 멈춘 뒤 150ms에 거른다(02 SCR-02)
// TODO(A8): AppState의 현재 세대로 바꾼다. 지금은 세대 버튼과 같은 4(신오)로 고정.
constexpr int kGeneration = 4;

// 열 폭: Dex.dc.html의 grid-template-columns: 18 40 28 1fr 146 216 40 + gap 8 (칸마다 좌우 4씩
// 포함)
struct ColumnWidth
{
    int column;
    int width;
};
constexpr ColumnWidth kColumnWidths[] = {
        {SpeciesTableModel::CursorColumn, 18 + 8},   {SpeciesTableModel::NumberColumn, 40 + 8},
        {SpeciesTableModel::NameColumn, 160 + 8},    {SpeciesTableModel::HpColumn, 36 + 8},
        {SpeciesTableModel::AttackColumn, 36 + 8},   {SpeciesTableModel::DefenseColumn, 36 + 8},
        {SpeciesTableModel::SpAttackColumn, 36 + 8}, {SpeciesTableModel::SpDefenseColumn, 36 + 8},
        {SpeciesTableModel::SpeedColumn, 36 + 8},    {SpeciesTableModel::TotalColumn, 40 + 8},
};
} // namespace

namespace com::yamada::studio {
DexPage::DexPage(Repository *repository, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_model(new SpeciesTableModel(this))
    , m_proxy(new SpeciesFilterProxy(this))
    , m_searchDelay(new QTimer(this))
{
    m_proxy->setSourceModel(m_model); // 프록시는 원본 모델 위에 얹힌다. 뷰는 프록시를 본다

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPageMargins);

    m_panel = new PanelFrame;
    m_panel->setPanelStyle(kListPanel);
    layout->addWidget(m_panel);

    QWidget *body = new QWidget;
    QVBoxLayout *bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(kBodyMargins);
    bodyLayout->setSpacing(8);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_search = new SearchField(tr("이름 · 번호로 찾기"));
    m_search->setFixedSize(kSearchWidth, kSearchHeight);
    toolbar->addWidget(m_search);
    toolbar->addStretch();
    bodyLayout->addLayout(toolbar);

    m_table = new QTableView;
    m_table->setObjectName(QStringLiteral("dexTable"));
    m_table->setModel(m_proxy);
    m_table->setItemDelegate(new DexRowDelegate(m_table)); // 모든 칸을 이 delegate가 그린다
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(tok::kSizeTableRow); // 줄 높이 34
    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    // 디자인은 이름 칸(1fr)이 남는 폭을 갖지만, 거기엔 좌우에 필터 · 상세 창이 있어 목록이 좁다.
    // 지금은 목록 창이 화면 폭 전체라 이름이 늘어나면 타입 칩이 멀리 떨어진다. 그래서 이름은
    // 160으로 두고 타입 칸(최소 146)이 남는 폭을 갖는다. E1에서 창 셋이 되면 디자인대로 되돌린다.
    header->setSectionResizeMode(SpeciesTableModel::TypesColumn, QHeaderView::Stretch);
    for (const ColumnWidth &width : kColumnWidths)
        header->resizeSection(width.column, width.width);
    header->setHighlightSections(false);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(SpeciesTableModel::TotalColumn,
                          Qt::DescendingOrder); // 디자인 기본: 합계 높은 순
    bodyLayout->addWidget(m_table, 1);

    m_panel->setBody(body);

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
