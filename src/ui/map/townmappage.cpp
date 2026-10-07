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
    m_game = new GameSelector; // 묶음 칩([성도 HG|SS]) — 조각으로 버전을 고른다(사용자 결정)
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

    // 지방: 이 버전의 야생 출현이 걸친 지방들. 합본 지도(성도 · 관동을 이어 붙인
    // "johto-kanto")가 있으면 그걸 하나로 쓰고 지방 칩 없이 끌어서 본다(사용자 결정)
    while (QLayoutItem *item = m_regionChips->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    const QStringList covered = m_repository->encounterRegions(m_version);
    QStringList sorted = covered;
    sorted.sort();
    QStringList regions;
    if (covered.size() > 1 && townmapbook::regionMap(sorted.join(QLatin1Char('-'))).isValid()) {
        regions = {sorted.join(QLatin1Char('-'))};
    } else {
        for (const QString &region : covered)
            if (townmapbook::regionMap(region).isValid())
                regions.append(region);
    }
    if (!regions.contains(m_region))
        m_region = regions.value(0);
    for (const QString &region : regions) {
        QPushButton *chip = new QPushButton(regionTitle(region, language));
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

QString TownMapPage::regionTitle(const QString &key, Language language)
{
    // 합본 key("johto-kanto")는 지방 이름을 이어서("성도 · 관동")
    QStringList parts;
    for (const QString &one : key.split(QLatin1Char('-'), Qt::SkipEmptyParts))
        parts.append(m_repository->regionDisplayName(one).text(language));
    return parts.join(QStringLiteral(" · "));
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
    QStringList layerLabels;
    for (const townmapbook::Layer &layer : map.layers)
        layerLabels.append(m_repository->regionDisplayName(layer.source).text(language));
    m_view->setRegion(region, m_names, language, layerLabels);
    m_detail->setTitle(tr("장소"), QString());
    showLocation(QString());

    // 지도 창 머리: "성도 · 관동 타운맵 — 92곳". 버전은 위의 게임 칩이 말해 준다
    m_mapPanel->setTitle(region.isEmpty() ? tr("타운맵")
                                          : tr("%1 타운맵").arg(regionTitle(region, language)),
                         map.isValid() ? tr("%1곳").arg(map.nodes.size()) : QString());
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
