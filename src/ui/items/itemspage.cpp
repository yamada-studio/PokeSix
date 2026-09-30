#include "ui/items/itemspage.h"

#include "data/models/itemfilterproxy.h"
#include "data/models/itemtablemodel.h"
#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/items/categorybutton.h"
#include "ui/items/itemheaderview.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/logging/logging.h"
#include "ui/theme/itemstyle.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"
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

// 목록 칸 폭(칸 좌우 여백 4 포함). 효과 칸이 남는 폭을 갖는다. 디자인: 18 · 28 · 112 · 1fr · 178 +
// gap 10
constexpr int kCursorWidth = 24;
constexpr int kIconWidth = 40;
constexpr int kNameWidth = 132;
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
    layout->addWidget(m_listPanel, 1);

    connect(m_state, &AppState::generationChanged, this, &ItemsPage::onGenerationChanged);
    selectGroup(QString::fromLatin1(itemstyle::kAll)); // "전체"도 숨길 분류는 빼야 해서 꼭 한 번
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
    m_header->resizeSection(ItemTableModel::GenerationsColumn,
                            ItemRowDelegate::generationsColumnWidth());
    m_header->setStretchLastSection(false);
    m_table->setSortingEnabled(true);
    m_table->sortByColumn(ItemTableModel::NameColumn, Qt::AscendingOrder); // 가나다순
    layout->addWidget(m_table, 1);

    // 아이콘 파일이 받아질 때마다 다시 그린다(update는 예약만 — 여러 번이 한 번으로 합쳐진다)
    connect(m_sprites, &SpriteCache::ready, m_table->viewport(), qOverload<>(&QWidget::update));

    // 검색 디바운스: 입력이 150ms 멈추면 한 번만 거른다
    m_searchDelay->setSingleShot(true);
    m_searchDelay->setInterval(kSearchDebounceMs);
    connect(m_search->lineEdit(), &QLineEdit::textChanged, m_searchDelay,
            qOverload<>(&QTimer::start));
    connect(m_searchDelay, &QTimer::timeout, this, [this] {
        m_proxy->setSearchText(m_search->lineEdit()->text());
        updateTitle();
    });
    return body;
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
    m_model->setRows(m_repository->itemsForGeneration(generation), generation);
    m_delegate->setGeneration(generation);
    m_header->setGeneration(generation);
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

void ItemsPage::updateTitle()
{
    // "아이템 대백과" + "진화 · 39개" (검색 · 세대 필터까지 반영한 수)
    QString label;
    for (const itemstyle::Group &group : itemstyle::groups())
        if (group.key == m_groupKey)
            label = group.label;
    m_listPanel->setTitle(tr("아이템 대백과"), tr("%1 · %2개").arg(label).arg(m_proxy->rowCount()));
}
} // namespace com::yamada::studio
