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
#include <QScrollBar>
#include <QStyle>
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
constexpr int kSearchDebounceMs = 150; // 입력 즉시가 아니라 멈춘 뒤 150ms에 거른다(02 SCR-02)
// TODO(A8): AppState의 현재 세대로 바꾼다. 지금은 세대 버튼과 같은 4(신오)로 고정.
constexpr int kGeneration = 4;

// 열 폭(칸마다 좌우 여백 4씩 포함). 디자인(Dex.dc.html: 18 40 28 1fr 146 216 40 + gap 8)은 이름
// 칸이 남는 폭을 갖지만, 거기엔 좌우에 필터 · 상세 창이 있어 목록이 좁다. 지금은 목록 창 하나라
// 칸을 늘리면 칸 사이가 휑하다 → 모든 칸을 고정 폭으로 두고(디자인보다 조금씩 넓게) 표를 가운데에
// 놓는다. E1에서 창 셋이 되면 다시 본다. (design/README.md "의도한 차이")
struct ColumnWidth
{
    int column;
    int width;
};
constexpr int kStatWidth = 54;
constexpr ColumnWidth kColumnWidths[] = {
        {SpeciesTableModel::CursorColumn, 24},
        {SpeciesTableModel::NumberColumn, 52},
        {SpeciesTableModel::IconColumn, 44},
        {SpeciesTableModel::NameColumn, 150},
        {SpeciesTableModel::TypesColumn, 170},
        {SpeciesTableModel::HpColumn, kStatWidth},
        {SpeciesTableModel::AttackColumn, kStatWidth},
        {SpeciesTableModel::DefenseColumn, kStatWidth},
        {SpeciesTableModel::SpAttackColumn, kStatWidth},
        {SpeciesTableModel::SpDefenseColumn, kStatWidth},
        {SpeciesTableModel::SpeedColumn, kStatWidth},
        {SpeciesTableModel::TotalColumn, 62},
};

constexpr int columnsWidth()
{
    int sum = 0;
    for (const ColumnWidth &width : kColumnWidths)
        sum += width.width;
    return sum;
}
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
    // 가로만 가운데 정렬: 창은 내용(표) 폭만큼만, 세로는 화면 높이 전체를 쓴다.
    // (정렬 플래그에 세로 성분이 없으면 레이아웃은 세로로는 칸을 꽉 채운다)
    layout->addWidget(m_panel, 0, Qt::AlignHCenter);

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
    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    for (const ColumnWidth &width : kColumnWidths)
        header->resizeSection(width.column, width.width);
    header->setHighlightSections(false);
    m_table->setSortingEnabled(true);
    // 기본은 도감 번호 오름차순(사용자 결정. 디자인 기본은 합계 높은 순). 머리 칸을 누르면 바뀐다.
    m_table->sortByColumn(SpeciesTableModel::NumberColumn, Qt::AscendingOrder);
    header->setStretchLastSection(false);
    // 표 폭 = 칸 합 + 세로 스크롤바. 가로 스크롤은 생기지 않게 고정한다.
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    m_table->setFixedWidth(columnsWidth()
                           + m_table->style()->pixelMetric(QStyle::PM_ScrollBarExtent));
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
