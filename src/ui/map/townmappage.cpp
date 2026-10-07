#include "ui/map/townmappage.h"

#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/dex/gameselector.h"
#include "ui/dex/guidebook.h"
#include "ui/map/mapview.h"
#include "ui/map/townmapbook.h"
#include "ui/squad/squadpaint.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelframe.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSet>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

constexpr QMargins kPageMargins {20, 16, 20, 12}; // 스쿼드와 같은 여백
constexpr int kDetailWidth = 340;

constexpr PanelStyle kMapPanel {.outline = 2,
                                .radius = 8,
                                .shadow = 3,
                                .fill = tok::kWhite,
                                .ink = tok::kInk,
                                .header = 38,
                                .headerColor = tok::kGreen};
constexpr PanelStyle kDetailPanel {.outline = 2,
                                   .radius = 8,
                                   .shadow = 3,
                                   .fill = tok::kWhite,
                                   .ink = tok::kInk,
                                   .header = 38,
                                   .headerColor = tok::kBlue};
} // namespace

namespace com::yamada::studio {
TownMapPage::TownMapPage(Repository *repository, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_state(state)
    , m_pokemonIcons(new SpriteCache(SpriteCache::Kind::PokemonIcon, this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(kPageMargins);
    layout->setSpacing(14);

    // ── 상단 막대: 제목 · 게임 칩(버전마다) · 지방 칩 ───────────────────────
    QHBoxLayout *top = new QHBoxLayout;
    top->setSpacing(12);
    QLabel *title = new QLabel(tr("타운맵 백과"));
    title->setObjectName(QStringLiteral("pageTitle"));
    top->addWidget(title);
    m_game = new GameSelector;
    m_game->setSplitVersions(true); // 출현이 버전마다 다르다 — [HG] [SS]
    connect(m_game, &GameSelector::versionSelected, this, [this](const QString &version) {
        m_state->setGame(version); // 앱 전체의 게임을 바꾼다(gameChanged → refresh)
        if (m_state->game() == version)
            refresh();
    });
    top->addWidget(m_game);
    m_regionChips = new QHBoxLayout;
    m_regionChips->setSpacing(6);
    top->addLayout(m_regionChips);
    m_regionGroup = new QButtonGroup(this);
    m_regionGroup->setExclusive(true);
    top->addStretch();
    layout->addLayout(top);

    // ── 본문: 지도(왼쪽, 남는 폭) + 장소 상세(오른쪽) ───────────────────────
    QHBoxLayout *body = new QHBoxLayout;
    body->setSpacing(16);
    m_mapPanel = new PanelFrame;
    m_mapPanel->setPanelStyle(kMapPanel);
    m_view = new MapView;
    connect(m_view, &MapView::locationSelected, this, &TownMapPage::showLocation);
    QWidget *mapBody = new QWidget;
    QVBoxLayout *mapLayout = new QVBoxLayout(mapBody);
    mapLayout->setContentsMargins(3, 2, 3, 3);
    mapLayout->addWidget(m_view);
    m_mapPanel->setBody(mapBody);
    body->addWidget(m_mapPanel, 1);

    m_detail = new PanelFrame;
    m_detail->setPanelStyle(kDetailPanel);
    m_detail->setFixedWidth(kDetailWidth);
    QWidget *detailBody = new QWidget;
    QVBoxLayout *detailOuter = new QVBoxLayout(detailBody);
    detailOuter->setContentsMargins(14, 10, 14, 12);
    m_empty = new QLabel(tr("지도에서 장소를 고르면\n그 게임의 야생 출현이 보여요"));
    m_empty->setObjectName(QStringLiteral("squadEmpty"));
    m_empty->setAlignment(Qt::AlignCenter);
    detailOuter->addWidget(m_empty, 1);
    QScrollArea *scroll = new QScrollArea;
    scroll->setObjectName(QStringLiteral("squadScroll")); // app.qss: 투명
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_detailBody = new QWidget;
    m_detailLayout = new QVBoxLayout(m_detailBody);
    m_detailLayout->setContentsMargins(0, 0, 0, 0);
    m_detailLayout->setSpacing(8);
    m_detailLayout->addStretch();
    scroll->setWidget(m_detailBody);
    detailOuter->addWidget(scroll, 1);
    scroll->hide();
    m_detail->setBody(detailBody);
    body->addWidget(m_detail);
    layout->addLayout(body, 1);

    connect(m_state, &AppState::generationChanged, this, &TownMapPage::refresh);
    connect(m_state, &AppState::gameChanged, this, &TownMapPage::refresh);
    connect(m_state, &AppState::languageChanged, this, &TownMapPage::refresh);
    connect(m_pokemonIcons, &SpriteCache::ready, this, [this] {
        if (!m_view->selected().isEmpty())
            showLocation(m_view->selected());
    });
    refresh();
}

void TownMapPage::refresh()
{
    const Language language = m_state->language();
    const int generation = m_state->generation();
    const QList<GameInfo> games = m_repository->gamesForGeneration(generation);
    m_version = m_repository->resolveVersion(generation, m_state->game());
    m_game->setGames(games, language, m_version);
    m_game->setVisible(games.size() > 1);

    // 지방 칩: 이 버전의 야생 출현이 걸친 지방(성도 · 관동), 지도가 준비된 것만. 하나뿐이면 숨긴다
    while (QLayoutItem *item = m_regionChips->takeAt(0))
        delete item->widget(), delete item;
    QStringList regions;
    for (const QString &region : m_repository->encounterRegions(m_version))
        if (townmapbook::regionMap(region).isValid())
            regions.append(region);
    if (regions.isEmpty()) // 지도가 아직 없는 세대 — 안내만
        regions = {};
    if (!regions.contains(m_region))
        m_region = regions.value(0);
    for (const QString &region : regions) {
        QPushButton *chip = new QPushButton(m_repository->regionDisplayName(region).text(language));
        chip->setObjectName(QStringLiteral("mapRegionChip"));
        chip->setCheckable(true);
        chip->setCursor(Qt::PointingHandCursor);
        chip->setChecked(region == m_region);
        m_regionGroup->addButton(chip);
        m_regionChips->addWidget(chip);
        connect(chip, &QPushButton::clicked, this, [this, region] { selectRegion(region); });
        chip->setVisible(regions.size() > 1);
    }
    selectRegion(m_region);
}

void TownMapPage::selectRegion(const QString &region)
{
    m_region = region;
    // 장소 이름: 성도 · 관동 한국어 지명은 PokéAPI에 없다 — 장소 사전(guidebook) → 도로 규칙 →
    // DB 이름 순으로 채운다. 경계 노드(성도 지도 속 토지폭포 = 관동)를 위해 지도에 나오는
    // 지방의 이름을 전부 모은다
    m_names.clear();
    const townmapbook::RegionMap &map = townmapbook::regionMap(region);
    QSet<QString> regions = {region};
    for (const townmapbook::Node &node : map.nodes)
        regions.insert(node.region);
    for (const QString &one : regions) {
        if (one.isEmpty())
            continue;
        const QHash<QString, LocalizedText> db = m_repository->locationNames(one);
        for (auto it = db.cbegin(); it != db.cend(); ++it) {
            LocalizedText name;
            name.ko = guidebook::placeName(it.key(), it.value(), Language::Korean);
            name.en = guidebook::placeName(it.key(), it.value(), Language::English);
            name.ja = guidebook::placeName(it.key(), it.value(), Language::Japanese);
            m_names.insert(it.key(), name);
        }
    }
    const Language language = m_state->language();
    m_view->setRegion(region, m_names, language);
    const LocalizedText name = m_repository->regionDisplayName(region);
    m_detail->setTitle(tr("장소"), QString());
    showLocation(QString());

    // 지도 창 머리: "성도 타운맵 · 소울실버 · 45곳"
    QString versionName;
    for (const GameInfo &game : m_repository->gamesForGeneration(m_state->generation())) {
        const qsizetype index = game.versions.indexOf(m_version);
        if (index >= 0 && index < game.versionNames.size())
            versionName = game.versionNames.at(index).text(language);
    }
    m_mapPanel->setTitle(region.isEmpty() ? tr("타운맵") : tr("%1 타운맵").arg(name.text(language)),
                         map.isValid() ? tr("%1 · %2곳").arg(versionName).arg(map.nodes.size())
                                       : QString());
}

void TownMapPage::showLocation(const QString &location)
{
    // 목록을 처음부터 다시 만든다(장소 하나의 줄 수는 적다)
    while (QLayoutItem *item = m_detailLayout->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    QScrollArea *scroll = m_detail->findChild<QScrollArea *>();
    if (location.isEmpty()) {
        m_detail->setTitle(tr("장소"), QString());
        m_empty->show();
        if (scroll)
            scroll->hide();
        return;
    }
    const Language language = m_state->language();
    m_empty->hide();
    if (scroll)
        scroll->show();
    m_detail->setTitle(m_names.value(location).text(language), QString());

    const QList<Repository::EncounterSpot> spots = m_repository->encountersAt(location, m_version);
    if (spots.isEmpty()) {
        QLabel *none = new QLabel(tr("이 게임에는 이곳의 야생 출현 자료가 없어요"));
        none->setObjectName(QStringLiteral("squadNote"));
        none->setWordWrap(true);
        m_detailLayout->addWidget(none);
    }
    QString method;
    for (const Repository::EncounterSpot &spot : spots) {
        if (spot.method != method) { // 방법(풀숲 · 파도타기 · 낚시 …)마다 소제목
            method = spot.method;
            QLabel *head = new QLabel(guidebook::methodName(method, language));
            head->setObjectName(QStringLiteral("dexSectionLabel"));
            m_detailLayout->addWidget(head);
        }
        QWidget *row = new QWidget;
        QHBoxLayout *h = new QHBoxLayout(row);
        h->setContentsMargins(0, 0, 0, 0);
        h->setSpacing(8);
        const QString key = QString::number(spot.pokemonId);
        const QString file = m_pokemonIcons->path(key);
        if (file.isEmpty()) {
            m_pokemonIcons->request(key);
        } else {
            QLabel *icon = new QLabel;
            icon->setPixmap(squadpaint::trimmedIcon(file, 20));
            h->addWidget(icon);
        }
        QLabel *name = new QLabel(spot.name.text(language));
        name->setFont(theme::font(theme::kFamilyBody, 13, QFont::ExtraBold));
        h->addWidget(name);
        h->addStretch();
        const QString levels = spot.minLevel == spot.maxLevel
                                       ? tr("Lv.%1").arg(spot.minLevel)
                                       : tr("Lv.%1–%2").arg(spot.minLevel).arg(spot.maxLevel);
        QLabel *info = new QLabel(spot.rarity > 0 ? tr("%1 · %2%").arg(levels).arg(spot.rarity)
                                                  : levels);
        info->setObjectName(QStringLiteral("squadCount"));
        h->addWidget(info);
        m_detailLayout->addWidget(row);
    }
    m_detailLayout->addStretch();
}
} // namespace com::yamada::studio
