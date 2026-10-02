#include "ui/dex/dexpage.h"

#include "data/models/speciesfilterproxy.h"
#include "data/models/speciestablemodel.h"
#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/dex/dexdetailpage.h"
#include "ui/dex/dexfilterpanel.h"
#include "ui/dex/dexheaderview.h"
#include "ui/dex/dexpreview.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/dex/dexselector.h"
#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/rowhover.h"
#include "ui/widgets/searchfield.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QResizeEvent>
#include <QScrollBar>
#include <QStackedWidget>
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

// 열 폭 — CSS grid의 "24px 52px 44px 3fr <타입 fit> 1fr ×7 + 끝 여유 8"과 같은 생각이다.
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
        {SpeciesTableModel::TotalColumn, 1, 52}, // 종족값과 같은 폭 → 칸 사이 간격이 고르다
};

// 목록 줄(필터 + 목록 + 미리 보기)의 최대 폭(CSS max-width). 이보다 넓은 창에서는 가운데에 두고
// 좌우 여백이 늘어난다 — 칸이 끝없이 벌어지면 줄을 따라 읽기 어렵다.
constexpr int kRowMaxWidth = 1660;
constexpr int kFilterWidth = 238;  // 왼쪽 필터 창
constexpr int kPreviewWidth = 292; // 오른쪽 미리 보기 창
constexpr int kColumnGap = 12;

// 옆 창(필터 · 미리 보기)의 겉모양: 목록 창과 같은 틀, 머리 색만 역할대로(파랑 = 필터 · 정보)
PanelStyle sidePanelStyle(QRgb headerColor)
{
    PanelStyle style = kListPanel;
    style.headerColor = headerColor;
    return style;
}
} // namespace

namespace com::yamada::studio {
DexPage::DexPage(Repository *repository, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_state(state)
    , m_model(new SpeciesTableModel(this))
    , m_proxy(new SpeciesFilterProxy(this))
    , m_sprites(new SpriteCache(SpriteCache::Kind::PokemonIcon, this))
    , m_searchDelay(new QTimer(this))
{
    m_proxy->setSourceModel(m_model); // 프록시는 원본 모델 위에 얹힌다. 뷰는 프록시를 본다

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPageMargins);

    m_panel = new PanelFrame;
    m_panel->setPanelStyle(kListPanel);
    // 머리 띠 오른쪽: 도감 선택 [전국] [신오 D · P] … 버튼은 load()에서 DB를 읽어 만든다.
    m_selector = new DexSelector;
    m_panel->setHeaderWidget(m_selector);
    connect(m_selector, &DexSelector::dexSelected, this, &DexPage::showDex);
    // 세대가 바뀌면(인트로 · 앱 막대의 세대 메뉴) 도감 버튼과 목록을 그 세대로 다시 만든다.
    connect(m_state, &AppState::generationChanged, this, &DexPage::onGenerationChanged);
    // 언어가 바뀌면 이름 · 타입 칩 · 지방 이름을 그 언어로(다시 읽을 필요 없다 — 행에 세 언어가 다
    // 있다)
    connect(m_state, &AppState::languageChanged, this, &DexPage::applyLanguage);
    // 창은 페이지 폭을 채운다. 최대 폭(kListMaxWidth)은 resizeEvent가 좌우 여백으로 맞춘다.
    // 목록 창과 상세 화면을 겹쳐 두고 하나만 보인다: [0] 목록 · [1] 상세(포켓몬을 누르면)
    // 목록 줄 = [필터 | 목록 | 미리 보기]. 상세로 가면 셋이 통째로 상세와 바뀐다.
    m_listRow = new QWidget;
    QHBoxLayout *row = new QHBoxLayout(m_listRow);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(kColumnGap);

    PanelFrame *filterFrame = new PanelFrame;
    filterFrame->setPanelStyle(sidePanelStyle(tok::kBlue));
    filterFrame->setTitle(tr("필터"));
    filterFrame->setFixedWidth(kFilterWidth);
    m_filter = new DexFilterPanel;
    QWidget *filterBody = new QWidget;
    QVBoxLayout *filterLayout = new QVBoxLayout(filterBody);
    filterLayout->setContentsMargins(kBodyMargins.left(), 10, kBodyMargins.right(), 12);
    filterLayout->addWidget(m_filter);
    filterFrame->setBody(filterBody);
    connect(m_filter, &DexFilterPanel::changed, this, &DexPage::applyFilters);
    row->addWidget(filterFrame);

    row->addWidget(m_panel, 1);

    PanelFrame *previewFrame = new PanelFrame;
    previewFrame->setPanelStyle(sidePanelStyle(tok::kGreen));
    previewFrame->setTitle(tr("미리 보기"));
    previewFrame->setFixedWidth(kPreviewWidth);
    m_preview = new DexPreview;
    QWidget *previewBody = new QWidget;
    QVBoxLayout *previewLayout = new QVBoxLayout(previewBody);
    previewLayout->setContentsMargins(kBodyMargins.left(), 10, kBodyMargins.right(), 12);
    previewLayout->addWidget(m_preview);
    previewFrame->setBody(previewBody);
    connect(m_preview, &DexPreview::detailRequested, this,
            [this] { openDetail(m_table->currentIndex()); });
    row->addWidget(previewFrame);

    m_views = new QStackedWidget;
    m_views->addWidget(m_listRow);
    m_detail = new DexDetailPage(m_repository, m_state);
    m_views->addWidget(m_detail);
    layout->addWidget(m_views);
    connect(m_detail, &DexDetailPage::backRequested, this, &DexPage::showList);

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
    // 머리 칸은 직접 그리는 DexHeaderView로 바꾼다(데이터와 같은 자리 계산). 모델보다 먼저 단다.
    m_table->setHorizontalHeader(new DexHeaderView(m_table));
    m_table->setModel(m_proxy);
    m_delegate = new DexRowDelegate(m_sprites, m_table);
    m_table->setItemDelegate(m_delegate); // 모든 칸을 이 delegate가 그린다
    // 줄 호버: 마우스가 올라간 줄을 칠하고, 누르면 장갑 커서로 꾹꾹(RowHover)
    m_delegate->setHover(new RowHover(m_table));
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
    // 기본 최소 칸 폭(글꼴 기준 ~38)이 ▶ 칸의 24를 막는다 → 칸 합이 viewport보다 넓어져 합계
    // 칸이 잘린다
    header->setMinimumSectionSize(kFixedColumns[0].width);
    for (const FixedColumn &fixed : kFixedColumns)
        header->resizeSection(fixed.column, fixed.width);
    applyLanguage(); // 타입 칸 폭(칩 글자 길이) · 이름 언어 — 고정 칸 합(m_fixedWidth)도 여기서
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
    // viewport(칸이 그려지는 안쪽 위젯)의 크기 변화를 엿본다 → 바뀔 때마다 비율 칸을 다시 나눈다.
    m_table->viewport()->installEventFilter(this);
    bodyLayout->addWidget(m_table, 1);

    m_panel->setBody(body);

    // 아이콘 파일이 하나 받아질 때마다 표를 다시 그린다. update()는 그리기를 "예약"만 하고, 이벤트
    // 루프가 여러 번의 예약을 한 번의 paintEvent로 합친다 → 수백 개가 연달아 와도 부담이 없다.
    connect(m_sprites, &SpriteCache::ready, m_table->viewport(), qOverload<>(&QWidget::update));

    // 한 번 클릭 · ↑↓ = 미리 보기, 더블클릭 · Enter(activated) = 전체 화면 상세
    connect(m_table, &QTableView::clicked, this, &DexPage::showPreview);
    connect(m_table, &QTableView::activated, this, &DexPage::openDetail);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex &current) { showPreview(current); });

    // 검색: 글자가 바뀔 때마다 타이머를 다시 건다 → 입력이 150ms 멈추면 한 번만 거른다(디바운스).
    m_searchDelay->setSingleShot(true);
    m_searchDelay->setInterval(kSearchDebounceMs);
    // searchTextChanged: 한글 조합 중 글자까지(첫 글자 "이"만 쳐도) — SearchField 참고
    connect(m_search, &SearchField::searchTextChanged, m_searchDelay, qOverload<>(&QTimer::start));
    connect(m_searchDelay, &QTimer::timeout, this, [this] {
        m_proxy->setSearchText(m_search->searchText());
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
    const int side = std::max(kPageMargins.left(), (width() - kRowMaxWidth) / 2);
    QWidget::layout()->setContentsMargins(side, kPageMargins.top(), side, kPageMargins.bottom());
}

void DexPage::layoutColumns()
{
    // 남는 폭 = viewport 폭(스크롤바 제외) − 고정 칸 − 끝 여유. 이걸 가중치대로 나눈다.
    // 반올림으로 남거나 모자란 몇 px는 이름 칸이 가져간다 — 왼쪽 정렬이라 폭이 1–2px 달라도 티가
    // 나지 않는다(숫자 칸이 가져가면 그 칸만 간격이 달라 보인다). 합계 칸은 끝 여유 kEndGap을 더
    // 갖는다(글자 자리는 DexRowDelegate::contentRect가 그만큼 들인다) → 칸 합 = viewport 폭.
    QHeaderView *header = m_table->horizontalHeader();
    const int free = m_table->viewport()->width() - m_fixedWidth - DexRowDelegate::kEndGap;
    qreal totalWeight = 0;
    for (const FlexColumn &flex : kFlexColumns)
        totalWeight += flex.weight;

    int widths[std::size(kFlexColumns)] = {};
    int used = 0;
    for (std::size_t i = 0; i < std::size(kFlexColumns); ++i) {
        const FlexColumn &flex = kFlexColumns[i];
        widths[i] = std::max(flex.minimum, qFloor(free * flex.weight / totalWeight));
        used += widths[i];
    }
    widths[0] = std::max(kFlexColumns[0].minimum, widths[0] + free - used); // [0] = 이름 칸

    for (std::size_t i = 0; i < std::size(kFlexColumns); ++i) {
        const bool isTotal = kFlexColumns[i].column == SpeciesTableModel::TotalColumn;
        header->resizeSection(kFlexColumns[i].column,
                              widths[i] + (isTotal ? DexRowDelegate::kEndGap : 0));
    }
}

void DexPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_loaded)
        load();
}

void DexPage::openDetail(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid())
        return;
    // 뷰의 줄(프록시: 정렬 · 검색 뒤 순서) → 원본 모델의 줄
    const QModelIndex source = m_proxy->mapToSource(proxyIndex);
    // 기술 기준 = 지금 고른 도감의 게임(성도 HG·SS → 하트골드·소울실버). 전국이면 세대의 대표 게임
    m_detail->showPokemon(m_model->rowAt(source.row()).pokemonId,
                          m_selector->currentVersionGroup());
    m_views->setCurrentWidget(m_detail);
}

void DexPage::openPokemon(int pokemonId, const QString &versionGroup)
{
    if (!m_loaded)
        load(); // 목록을 아직 안 읽었으면(처음 여는 화면) 도감 버튼부터 만든다
    if (m_selector->selectVersionGroup(versionGroup))
        showDex(m_selector->currentDex());
    m_detail->showPokemon(pokemonId, versionGroup);
    m_views->setCurrentWidget(m_detail);
}

void DexPage::showList()
{
    // 목록 "페이지"는 m_panel이 아니라 세 칸 묶음(m_listRow)이다 — m_panel을 주면 스택의
    // 페이지가 아니라서 아무 일도 일어나지 않는다(상세에서 돌아오지 못하는 버그였다)
    m_views->setCurrentWidget(m_listRow);
    m_table->setFocus(Qt::OtherFocusReason); // 키보드로 이어서 고를 수 있게
}

void DexPage::applyLanguage()
{
    const Language language = m_state->language();
    m_model->setLanguage(language);
    m_delegate->setLanguage(language);
    m_selector->setLanguage(language);
    // 타입 칩을 새 언어로 다시 만든다(고른 타입은 풀린다 — 프록시와 다시 맞춘다). 미리 보기는
    // 고른 줄을 다시 보여 주면 새 언어로 그려진다.
    m_filter->setTypes(m_chart.types, language);
    applyFilters();
    showPreview(m_table->currentIndex());

    // 타입 칸 폭은 칩 글자 길이에 따라 다르다(일본어 "フェアリー"가 가장 길다) → 고정 칸 합과
    // 표 최소 폭을 다시 계산하고 비율 칸을 다시 나눈다.
    QHeaderView *header = m_table->horizontalHeader();
    const int typesWidth = DexRowDelegate::typesColumnWidth(language);
    header->resizeSection(SpeciesTableModel::TypesColumn, typesWidth);
    m_fixedWidth = typesWidth;
    for (const FixedColumn &fixed : kFixedColumns)
        m_fixedWidth += fixed.width;
    // 최소 폭 = 고정 칸 + 비율 칸 최소 + 세로 스크롤바. 이보다 좁아지지 않으니 칸이 잘리지 않는다.
    int minimum = m_fixedWidth + m_table->style()->pixelMetric(QStyle::PM_ScrollBarExtent);
    for (const FlexColumn &flex : kFlexColumns)
        minimum += flex.minimum;
    m_table->setMinimumWidth(minimum + DexRowDelegate::kEndGap);
    layoutColumns();
}

void DexPage::onGenerationChanged()
{
    // 보이는 중이면 바로 다시 읽고, 숨어 있으면 다음에 보일 때(showEvent) 읽는다 — 안 보는 화면을
    // 미리 채우지 않는다(lazy).
    m_loaded = false;
    if (isVisible())
        load();
}

void DexPage::load()
{
    // 이 세대의 지방 도감으로 버튼을 만들고, 전국 목록부터 보여 준다.
    m_selector->setDexes(m_repository->dexesForGeneration(m_state->generation()));
    // 필터도 그 세대로: 타입 칩(1세대에는 악 · 강철 · 페어리가 없다)과 상성표(미리 보기의 약점)
    m_chart = m_repository->typeChart(m_state->generation());
    m_filter->setTypes(m_chart.types, m_state->language());
    m_filter->reset();
    applyFilters(); // reset이 아무것도 안 바꿨어도(첫 로드) 프록시와 한 번 맞춘다
    m_preview->clear();
    showDex(DexSelector::kNational);
    m_loaded = m_model->rowCount() > 0; // 비어 있으면(DB가 아직 없음) 다음에 보일 때 다시 읽는다
}

void DexPage::showDex(int pokedexId)
{
    // 전국 = 그 세대까지 나온 종(번호 = 전국 번호), 지방 = 그 도감의 종(번호 = 지방 번호).
    // 타입 · 종족값은 둘 다 지금 세대 기준.
    m_model->setRows(pokedexId == DexSelector::kNational
                             ? m_repository->speciesForGeneration(m_state->generation())
                             : m_repository->speciesForDex(pokedexId, m_state->generation()));
    // 도감을 고른다 = 그 도감 순서로 본다 → 합계 순 등으로 보고 있었어도 번호 순으로 되돌린다.
    m_table->sortByColumn(SpeciesTableModel::NumberColumn, Qt::AscendingOrder);
    m_table->scrollToTop();
    updateTitle();
    qCInfo(lcUi) << "dex" << pokedexId << "shows" << m_model->rowCount() << "species (generation"
                 << m_state->generation() << ")";
}

void DexPage::showPreview(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid()) {
        m_preview->clear();
        return;
    }
    const QModelIndex source = m_proxy->mapToSource(proxyIndex);
    const SpeciesRow &row = m_model->rowAt(source.row());
    // 약점(받을 때): 공격 타입마다 방어 타입 배율의 곱. 복합 타입은 곱이 4까지 간다.
    QStringList quad;
    QStringList twice;
    for (const QString &attack : m_chart.types) {
        double factor = 1;
        for (const QString &defense : row.types)
            factor *= m_chart.at(attack, defense);
        if (factor >= 3.9)
            quad.append(attack);
        else if (factor >= 1.9)
            twice.append(attack);
    }
    m_preview->setSpecies(row, m_state->generation(), quad, twice, m_state->language());
}

void DexPage::applyFilters()
{
    m_proxy->setTypes(m_filter->selectedTypes());
    m_proxy->setTotalRange(m_filter->minimumTotal(), m_filter->maximumTotal());
    m_proxy->setExcludeLegendary(m_filter->excludeLegendary());
    m_proxy->setFinalEvolutionOnly(m_filter->finalEvolutionOnly());
    updateTitle();
}

void DexPage::updateTitle()
{
    // "도감 백과 · 493마리" — 검색 중이면 걸러진 수
    m_panel->setTitle(tr("도감 백과"), tr("%1마리").arg(m_proxy->rowCount()));
}
} // namespace com::yamada::studio
