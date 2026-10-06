#include "ui/items/itemspage.h"

#include "data/models/itemfilterproxy.h"
#include "data/models/itemtablemodel.h"
#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/dex/gameselector.h"
#include "ui/dex/guidebook.h"
#include "ui/items/categorybutton.h"
#include "ui/items/itemdetailpane.h"
#include "ui/items/itemheaderview.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/logging/logging.h"
#include "ui/theme/itemstyle.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/rowhover.h"
#include "ui/widgets/searchfield.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QTableView>
#include <QTimer>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

// Items.dc.html: grid-template-columns 220px 1fr (400px 상세는 다음 단계) · gap 16 · padding 16 20
// 20
constexpr QMargins kPageMargins {20, 16, 20, 20};
constexpr int kGap = 16;
constexpr int kCategoryWidth = 220;
constexpr int kDetailWidth = 292; // 오른쪽 상세 창 (도감 미리 보기와 같은 폭)
constexpr PanelStyle kDetailPanel {.outline = 2,
                                   .radius = 8,
                                   .shadow = 3,
                                   .fill = tok::kWhite,
                                   .ink = tok::kInk,
                                   .header = 38,
                                   .headerColor = tok::kBlue};
constexpr PanelStyle kCategoryPanel {.outline = 2,
                                     .radius = 8,
                                     .shadow = 3,
                                     .fill = tok::kWhite,
                                     .ink = tok::kInk,
                                     .header = 38,
                                     .headerColor = tok::kGreen};
constexpr PanelStyle kListPanel {.outline = 2,
                                 .radius = 8,
                                 .shadow = 3,
                                 .fill = tok::kWhite,
                                 .ink = tok::kInk,
                                 .header = 38,
                                 .headerColor = tok::kRed};
constexpr int kSearchDebounceMs = 150;

// 목록 칸 폭(칸 좌우 여백 4 포함). 효과 칸이 남는 폭을 갖는다. 디자인의 세대 칸(178)은 두지 않고
// 가격 칸을 둔다 — 목록이 이미 지금 세대 아이템만이라서.
constexpr int kCursorWidth = 24;
constexpr int kIconWidth = 40;
constexpr int kNameWidth = 132;
constexpr int kPriceWidth = 84; // "10,000원"
} // namespace

namespace com::yamada::studio {
ItemsPage::ItemsPage(Repository *repository, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_state(state)
    , m_model(new ItemTableModel(this))
    , m_proxy(new ItemFilterProxy(this))
    , m_sprites(new SpriteCache(SpriteCache::Kind::Item, this))
    , m_groups(new QButtonGroup(this))
    , m_searchDelay(new QTimer(this))
{
    m_proxy->setSourceModel(m_model);
    // 목록은 늘 지금 세대에 있는 아이템만(도감과 같다). 세대를 바꾸면 load()가 다시 채운다.
    // (디자인은 다른 세대 아이템도 흐리게 보여 주는 체크박스였지만 쓰지 않는다 — design/README.md)
    m_proxy->setOnlyInGeneration(true);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(kGap);

    m_categoryPanel = new PanelFrame;
    m_categoryPanel->setPanelStyle(kCategoryPanel);
    m_categoryPanel->setTitle(tr("분류"));
    m_categoryPanel->setFixedWidth(kCategoryWidth);
    m_categoryPanel->setBody(buildCategoryBody());
    layout->addWidget(m_categoryPanel);

    m_listPanel = new PanelFrame;
    m_listPanel->setPanelStyle(kListPanel);
    m_listPanel->setBody(buildListBody());
    m_games = new GameSelector;
    m_listPanel->setHeaderWidget(m_games); // 머리 띠 오른쪽: 게임 칩
    connect(m_games, &GameSelector::versionSelected, m_state, &AppState::setGame);
    layout->addWidget(m_listPanel, 1);

    m_detailPanel = new PanelFrame;
    m_detailPanel->setPanelStyle(kDetailPanel);
    m_detailPanel->setTitle(tr("상세"));
    m_detailPanel->setFixedWidth(kDetailWidth);
    m_detail = new ItemDetailPane(m_sprites);
    QWidget *detailBody = new QWidget;
    QVBoxLayout *detailLayout = new QVBoxLayout(detailBody);
    detailLayout->setContentsMargins(12, 10, 12, 12);
    detailLayout->addWidget(m_detail);
    m_detailPanel->setBody(detailBody);
    layout->addWidget(m_detailPanel);

    connect(m_state, &AppState::generationChanged, this, &ItemsPage::onGenerationChanged);
    connect(m_state, &AppState::gameChanged, this, &ItemsPage::onGenerationChanged);
    connect(m_state, &AppState::languageChanged, this, &ItemsPage::applyLanguage);
    selectGroup(QString::fromLatin1(itemstyle::kAll)); // "전체"도 숨길 분류는 빼야 해서 꼭 한 번
    applyLanguage();
}

QWidget *ItemsPage::buildCategoryBody()
{
    QWidget *body = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(body);
    layout->setContentsMargins(10, 10, 10, 0);
    layout->setSpacing(4);

    // 묶음 버튼: 하나만 켜진다(QButtonGroup exclusive). 목록은 itemstyle.json이 준다.
    m_groups->setExclusive(true);
    for (const itemstyle::Group &group : itemstyle::groups()) {
        CategoryButton *button = new CategoryButton(group);
        layout->addWidget(button);
        m_groups->addButton(button);
        connect(button, &QAbstractButton::clicked, this,
                [this, key = group.key] { selectGroup(key); });
    }
    if (!m_groups->buttons().isEmpty())
        m_groups->buttons().first()->setChecked(true); // 전체
    layout->addStretch();

    return body;
}

QWidget *ItemsPage::buildListBody()
{
    QWidget *body = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(body);
    layout->setContentsMargins(12, 8, 12, 0);
    layout->setSpacing(8);

    QHBoxLayout *toolbar = new QHBoxLayout;
    m_search = new SearchField(tr("이름 · 효과 검색"));
    toolbar->addWidget(m_search);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    m_table = new QTableView;
    m_table->setObjectName(QStringLiteral("itemsTable")); // app.qss: 흰 바탕 · 테두리 없음
    m_header = new ItemHeaderView(m_table);
    m_table->setHorizontalHeader(m_header); // 모델보다 먼저 단다
    m_table->setModel(m_proxy);
    m_delegate = new ItemRowDelegate(m_sprites, m_table);
    m_table->setItemDelegate(m_delegate);
    // 줄 호버: 마우스가 올라간 줄을 칠하고, 누르면 장갑 커서로 꾹꾹(RowHover). 아이템 목록은 아직
    // 누를 곳이 없지만(상세는 다음 단계) 같은 손맛을 준다.
    m_delegate->setHover(new RowHover(m_table));
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(false);
    m_table->setWordWrap(false);
    m_table->setFrameShape(QFrame::NoFrame);
    m_table->verticalHeader()->hide();
    m_table->verticalHeader()->setDefaultSectionSize(ItemRowDelegate::kRowHeight);
    m_table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); // 폭이 출렁이지 않게(도감과 같다)

    // 칸 폭: 효과 칸만 남는 폭(Stretch), 나머지는 고정
    m_header->setSectionResizeMode(QHeaderView::Fixed);
    m_header->setSectionResizeMode(ItemTableModel::EffectColumn, QHeaderView::Stretch);
    m_header->resizeSection(ItemTableModel::CursorColumn, kCursorWidth);
    m_header->resizeSection(ItemTableModel::IconColumn, kIconWidth);
    m_header->resizeSection(ItemTableModel::NameColumn, kNameWidth);
    m_header->resizeSection(ItemTableModel::PriceColumn, kPriceWidth);
    m_header->setStretchLastSection(false);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ItemTableModel::NameColumn, Qt::AscendingOrder); // 가나다순
    layout->addWidget(m_table, 1);

    // 아이콘 파일이 받아질 때마다 다시 그린다(update는 예약만 — 여러 번이 한 번으로 합쳐진다)
    connect(m_sprites, &SpriteCache::ready, m_table->viewport(), qOverload<>(&QWidget::update));

    // 한 번 클릭 · ↑↓ = 오른쪽 상세 창
    connect(m_table, &QTableView::clicked, this, &ItemsPage::showDetail);
    connect(m_table->selectionModel(), &QItemSelectionModel::currentRowChanged, this,
            [this](const QModelIndex &current) { showDetail(current); });

    // 검색 디바운스: 입력이 150ms 멈추면 한 번만 거른다
    m_searchDelay->setSingleShot(true);
    m_searchDelay->setInterval(kSearchDebounceMs);
    // searchTextChanged: 한글 조합 중 글자까지(첫 글자 "이"만 쳐도) — SearchField 참고
    connect(m_search, &SearchField::searchTextChanged, m_searchDelay, qOverload<>(&QTimer::start));
    connect(m_searchDelay, &QTimer::timeout, this, [this] {
        m_proxy->setSearchText(m_search->searchText());
        updateTitle();
    });
    return body;
}

void ItemsPage::applyLanguage()
{
    // 게임 데이터 이름 · 문구 · 타입 칩 · 분류 이름을 그 언어로. 행에 세 언어가 다 있어서 다시 읽지
    // 않는다.
    const Language language = m_state->language();
    m_model->setLanguage(language);
    m_delegate->setLanguage(language);
    for (QAbstractButton *button : m_groups->buttons())
        static_cast<CategoryButton *>(button)->setLanguage(language);
    m_table->viewport()->update();
    showDetail(m_table->currentIndex()); // 상세 창도 새 언어로
    updateTitle();
}

void ItemsPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    if (!m_loaded)
        load();
}

void ItemsPage::onGenerationChanged()
{
    // 보이는 중이면 바로, 숨어 있으면 다음에 보일 때(DexPage와 같다)
    m_loaded = false;
    if (isVisible())
        load();
}

void ItemsPage::load()
{
    const int generation = m_state->generation();
    const QList<GameInfo> games = m_repository->gamesForGeneration(generation);
    // 게임 = 앱이 고른 버전(스쿼드 · 도감과 같다). 고른 적이 없으면 대표 게임
    m_version = m_repository->resolveVersion(generation, m_state->game());
    m_versionGroup = versionGroupOf(games, m_version);
    m_games->setGames(games, m_state->language(), m_version);
    // 보고 있던 아이템을 기억해 둔다 — 게임 칩 · 세대를 바꿔도 상세가 풀리지 않게
    const QModelIndex before = m_table->currentIndex();
    const QString selected = before.isValid()
                                     ? m_model->rowAt(m_proxy->mapToSource(before).row()).identifier
                                     : QString();
    m_model->setRows(m_repository->itemsForGeneration(generation, m_versionGroup), generation);
    m_delegate->setGeneration(generation);
    // 그 게임의 입수 사전에 아이템 목록이 있으면 그 목록으로 게임별 존재를 거른다
    m_proxy->setAllowedItems(guidebook::hasItemBook(m_versionGroup)
                                     ? guidebook::itemsIn(m_versionGroup)
                                     : QSet<QString>());
    reselect(selected); // 새 목록에도 있으면 다시 고른다(currentRowChanged → 상세 갱신)
    m_loaded = m_model->rowCount() > 0; // 비어 있으면(DB가 아직 없음) 다음에 보일 때 다시
    updateTitle();
    qCInfo(lcUi) << "items loaded" << m_model->rowCount() << "for generation" << generation;
}

void ItemsPage::selectGroup(const QString &key)
{
    m_groupKey = key;
    m_proxy->setCategoryFilter(itemstyle::filterFor(key)); // 규칙은 itemstyle.json — 화면은 key만
    m_table->scrollToTop();
    updateTitle();
}

void ItemsPage::reselect(const QString &identifier)
{
    if (!identifier.isEmpty()) {
        for (int row = 0; row < m_proxy->rowCount(); ++row) {
            const QModelIndex index = m_proxy->index(row, 0);
            if (index.data(ItemTableModel::IdentifierRole).toString() == identifier) {
                m_table->selectRow(row);
                m_table->scrollTo(index);
                return;
            }
        }
    }
    m_detail->clear(); // 새 목록에 없다(그 게임에 없는 아이템)
}

void ItemsPage::showDetail(const QModelIndex &proxyIndex)
{
    if (!proxyIndex.isValid()) {
        m_detail->clear();
        return;
    }
    const int generation = m_state->generation();
    const ItemRow &item = m_model->rowAt(m_proxy->mapToSource(proxyIndex).row());
    // 진화 대상과 입수처(고른 게임의 입수 사전 — 기술머신 · 도구 모두)
    const QList<ItemEvolution> evolutions = m_repository->evolutionsWithItem(item.id, generation);
    const QStringList places = guidebook::itemSources(m_versionGroup, item.identifier,
                                                      m_state->language(), m_version);
    m_detail->setItem(item, generation, m_state->language(), evolutions, places);
}

void ItemsPage::updateTitle()
{
    // "아이템 대백과" + "진화 · 39개" (검색 · 세대 필터까지 반영한 수)
    QString label;
    for (const itemstyle::Group &group : itemstyle::groups())
        if (group.key == m_groupKey)
            label = group.label.text(m_state->language());
    m_listPanel->setTitle(tr("아이템 대백과"), tr("%1 · %2개").arg(label).arg(m_proxy->rowCount()));
}
} // namespace com::yamada::studio
