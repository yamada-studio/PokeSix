#include "ui/dex/dexdetailpage.h"

#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "ui/dex/abilitylist.h"
#include "ui/dex/encounterlist.h"
#include "ui/dex/evolutionview.h"
#include "ui/dex/matchupview.h"
#include "ui/dex/movelist.h"
#include "ui/dex/naturepicker.h"
#include "ui/dex/statradar.h"
#include "ui/logging/logging.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/dropdownbutton.h"
#include "ui/widgets/panelframe.h"
#include "ui/widgets/shadowbutton.h"
#include "ui/widgets/typechip.h"

#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QKeySequence>
#include <QLabel>
#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QShortcut>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

constexpr int kGap = 16;
constexpr int kFirstNatureGeneration = 3; // 성격 · 특성은 3세대(RS)부터
constexpr int kTopRowGap = 10; // 위 줄 카드 사이(아래 섹션 사이 kGap보다 좁게)
constexpr int kCardMinWidth
        = 236; // 위 줄 카드 셋은 같은 폭(1 : 1 : 1)으로 나누고, 이보다 좁아지지 않는다
// 위 줄 카드 셋(그림 · 종족값 · 획득법)의 높이 — 그림 카드의 내용 높이에 맞춘다. 획득법이 더 길면
// 카드 안에서 스크롤한다.
constexpr int kTopRowHeight = 340;
constexpr int kSpriteScale = 2; // 80×80 도트를 2배(정수 배라 흐려지지 않는다)

PanelStyle sectionStyle(QRgb headerColor)
{
    return {.outline = 2,
            .radius = 8,
            .shadow = 3,
            .fill = tok::kWhite,
            .ink = tok::kInk,
            .header = 34,
            .headerColor = headerColor};
}

// 겉모양(PanelFrame) + 안쪽 여백을 둔 몸통 하나
PanelFrame *section(QRgb headerColor, QWidget *content, QMargins margins = {12, 10, 12, 12})
{
    PanelFrame *frame = new PanelFrame;
    frame->setPanelStyle(sectionStyle(headerColor));
    QWidget *body = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(body);
    layout->setContentsMargins(margins);
    // 스크롤 영역 · 세로로 늘어나는 위젯(레이더)은 남는 높이를 채우고, 나머지는 위에 붙인다
    const bool fills = content->sizePolicy().verticalPolicy() == QSizePolicy::Expanding
                       || qobject_cast<QScrollArea *>(content) != nullptr;
    layout->addWidget(content, fills ? 1 : 0);
    if (!fills)
        layout->addStretch();
    frame->setBody(body);
    return frame;
}

// 세대마다 정면 그림 폴더(PokéAPI/sprites의 versions/…). 없는 세대(7 · 8)는 기본 그림(96×96).
QString spriteKey(int generation, int pokemonId)
{
    static constexpr const char *kFolders[] = {"generation-i/yellow",
                                               "generation-ii/crystal",
                                               "generation-iii/emerald",
                                               "generation-iv/platinum",
                                               "generation-v/black-white",
                                               "generation-vi/x-y",
                                               nullptr,
                                               nullptr,
                                               "generation-ix/scarlet-violet"};
    const char *folder = generation >= 1 && generation <= 9 ? kFolders[generation - 1] : nullptr;
    return folder ? QStringLiteral("%1/%2").arg(QLatin1String(folder)).arg(pokemonId)
                  : QStringLiteral("default/%1").arg(pokemonId); // 폴더 없음 → 2번째 출처(기본)
}
} // namespace

namespace com::yamada::studio {
// 왼쪽 위 카드: 그림 · 번호 · 이름(다른 두 언어 이름) · 분류 · 타입 · 키 · 몸무게
class ProfileCard : public QWidget
{
public:
    ProfileCard(SpriteCache *fronts, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_fronts(fronts)
    {
        QWidget::setFixedHeight(kTopRowHeight);
        QWidget::setMinimumWidth(kCardMinWidth);
        connect(m_fronts, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    }

    void setDetail(const PokemonDetail &detail, Language language)
    {
        m_detail = detail;
        m_language = language;
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        // 머리 띠 없는 창(먹선 2 · 반경 8 · 그림자 3)
        paintPanel(painter, rect(),
                   PanelStyle {.outline = 2,
                               .radius = 8,
                               .shadow = 3,
                               .fill = tok::kWhite,
                               .ink = tok::kInk});
        if (!m_detail.isValid())
            return;
        const int inner = width() - 2 * 14;
        int y = 14;
        // 그림: 바탕 칸(paper.alt) 가운데에 2배
        const QRect frame(14, y, inner, 176);
        painter.setPen(QPen(QColor(tok::kLine), 1.5));
        painter.setBrush(QColor(tok::kPaperAlt));
        painter.drawRoundedRect(QRectF(frame).adjusted(0.75, 0.75, -0.75, -0.75), 6, 6);
        const QString key = spriteKey(m_detail.generation, m_detail.pokemonId);
        const QString file = m_fronts->path(key);
        QPixmap sprite;
        if (file.isEmpty() || !sprite.load(file)) {
            m_fronts->request(key);
        } else {
            const QSize size = sprite.size() * kSpriteScale;
            const QRect target(frame.center() - QPoint(size.width() / 2, size.height() / 2), size);
            painter.setRenderHint(QPainter::SmoothPixmapTransform,
                                  false); // 도트는 이웃 픽셀 그대로
            painter.drawPixmap(target, sprite);
        }
        y = frame.bottom() + 10;

        // 번호 · 이름 · 다른 언어 이름 · 분류
        painter.setFont(theme::font(theme::kFamilyData, 13, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(14, y, inner, 18), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("No. %1").arg(m_detail.speciesId, 3, 10, QLatin1Char('0')));
        y += 20;
        painter.setFont(theme::font(theme::kFamilyTitle, 26));
        painter.setPen(QColor(tok::kText1));
        painter.drawText(QRect(14, y, inner, 32), Qt::AlignLeft | Qt::AlignVCenter,
                         m_detail.name.text(m_language));
        y += 32;
        QStringList others;
        for (const Language other : {Language::Korean, Language::Japanese, Language::English})
            if (other != m_language && !m_detail.name.exact(other).isEmpty())
                others.append(m_detail.name.exact(other));
        painter.setFont(theme::font(theme::kFamilyBody, 12));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(14, y, inner, 18), Qt::AlignLeft | Qt::AlignVCenter,
                         others.join(QStringLiteral(" · ")));
        y += 20;
        painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::Bold));
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(14, y, inner, 18), Qt::AlignLeft | Qt::AlignVCenter,
                         m_detail.genus.text(m_language));
        y += 26;

        // 타입 칩 · 키 · 몸무게
        qreal x = 14;
        for (const QString &identifier : m_detail.types)
            if (const tok::TypeColor *type = typechip::find(identifier))
                x += typechip::paint(painter, QPointF(x, y), *type, m_language) + typechip::kGap;
        painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
        painter.setPen(QColor(tok::kText2));
        painter.drawText(QRect(int(x) + 6, y, inner, int(typechip::kHeight)),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("%1 m · %2 kg")
                                 .arg(m_detail.height / 10.0, 0, 'f', 1)
                                 .arg(m_detail.weight / 10.0, 0, 'f', 1));
    }

private:
    SpriteCache *m_fronts = nullptr;
    PokemonDetail m_detail;
    Language m_language = Language::Korean;
};

DexDetailPage::DexDetailPage(Repository *repository, AppState *state, QWidget *parent)
    : QWidget(parent)
    , m_repository(repository)
    , m_state(state)
    , m_fronts(new SpriteCache(SpriteCache::Kind::PokemonFront, this))
    , m_icons(new SpriteCache(SpriteCache::Kind::Item, this))
    , m_pokemonIcons(new SpriteCache(SpriteCache::Kind::PokemonIcon, this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // 위 줄: [← 목록] … 기술 기준 게임
    QHBoxLayout *top = new QHBoxLayout;
    ShadowButton *back = new ShadowButton(ShadowButton::Variant::Secondary);
    back->setText(tr("← 목록"));
    back->setFocusPolicy(Qt::TabFocus);
    top->addWidget(back);
    top->addStretch();
    m_basis = new QLabel;
    m_basis->setObjectName(QStringLiteral("dexDetailBasis"));
    top->addWidget(m_basis);
    layout->addLayout(top);

    // 나머지는 한 장으로 세로 스크롤(기술 목록이 길다)
    m_scroll = new QScrollArea;
    m_scroll->setObjectName(QStringLiteral("dexDetailScroll"));
    m_scroll->setWidgetResizable(true);
    m_scroll->setFrameShape(QFrame::NoFrame);
    m_scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scroll->setWidget(buildContent());
    layout->addWidget(m_scroll, 1);

    connect(back, &ShadowButton::clicked, this, &DexDetailPage::backRequested);
    QShortcut *escape = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    escape->setContext(Qt::WidgetWithChildrenShortcut);
    connect(escape, &QShortcut::activated, this, &DexDetailPage::backRequested);
    connect(m_state, &AppState::generationChanged, this, &DexDetailPage::reload);
    connect(m_state, &AppState::languageChanged, this, &DexDetailPage::applyLanguage);
}

QWidget *DexDetailPage::buildContent()
{
    QWidget *content = new QWidget;
    content->setObjectName(QStringLiteral("dexDetailContent")); // app.qss: 투명
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(0, 0, 4, 8); // 오른쪽 4: 세로 스크롤바와 그림자 사이
    layout->setSpacing(kGap);

    // 1) 그림 · 종족값 · 획득법
    QHBoxLayout *row = new QHBoxLayout;
    row->setSpacing(kTopRowGap);
    m_profile = new ProfileCard(m_fronts);
    row->addWidget(m_profile, 1);
    m_stats = new StatRadar;
    m_statsPanel = section(tok::kBlue, m_stats, {6, 4, 6, 6});
    m_statsPanel->setFixedHeight(kTopRowHeight);
    // 머리 띠 오른쪽 [성격 ▾]: 5×5 성격표 팝업 → 레이더에 ▲▼
    m_natureButton = new DropdownButton(tr("성격"));
    m_statsPanel->setHeaderWidget(m_natureButton);
    connect(m_natureButton, &DropdownButton::clicked, this, &DexDetailPage::showNaturePicker);
    m_statsPanel->setMinimumWidth(kCardMinWidth);
    row->addWidget(m_statsPanel, 1);
    // 획득법: [진화 트리] + [야생 출현]. 줄이 많으면(캐이시 27줄) 카드 높이 안에서 스크롤
    QWidget *acquisition = new QWidget;
    acquisition->setObjectName(QStringLiteral("dexAcquisition")); // app.qss: 흰 바탕
    QVBoxLayout *acquisitionLayout = new QVBoxLayout(acquisition);
    acquisitionLayout->setContentsMargins(0, 0, 0, 0);
    acquisitionLayout->setSpacing(4);
    m_evolutionLabel = new QLabel(tr("진화"));
    m_evolutionLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    acquisitionLayout->addWidget(m_evolutionLabel);
    m_evolution = new EvolutionView(m_pokemonIcons);
    acquisitionLayout->addWidget(m_evolution);
    QLabel *wildLabel = new QLabel(tr("야생 출현"));
    wildLabel->setObjectName(QStringLiteral("dexSectionLabel"));
    acquisitionLayout->addSpacing(6);
    acquisitionLayout->addWidget(wildLabel);
    m_encounters = new EncounterList;
    acquisitionLayout->addWidget(m_encounters);
    acquisitionLayout->addStretch();
    QScrollArea *encounterScroll = new QScrollArea;
    encounterScroll->setObjectName(QStringLiteral("dexEncounterScroll")); // app.qss: 투명
    encounterScroll->setWidgetResizable(true);
    encounterScroll->setFrameShape(QFrame::NoFrame);
    encounterScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    encounterScroll->setWidget(acquisition);
    PanelFrame *encounters = section(tok::kGreen, encounterScroll, {12, 8, 6, 10});
    encounters->setTitle(tr("획득법"));
    // 진화 트리의 다른 포켓몬을 누르면 그 상세로
    connect(m_evolution, &EvolutionView::pokemonClicked, this,
            [this](int pokemonId) { showPokemon(pokemonId, m_versionGroup); });
    encounters->setFixedHeight(kTopRowHeight);
    encounters->setMinimumWidth(kCardMinWidth);
    row->addWidget(encounters, 1);
    layout->addLayout(row);

    // 2) 특성(그 세대 · 숨겨진 특성은 5세대부터)
    m_abilities = new AbilityList;
    PanelFrame *abilities = section(tok::kBlueDeep, m_abilities);
    abilities->setTitle(tr("특성"));
    layout->addWidget(abilities);

    // 3) 상성 · 4) 레벨업 기술 · 5) 기술머신
    m_matchups = new MatchupView;
    PanelFrame *matchups = section(tok::kRed, m_matchups);
    matchups->setTitle(tr("타입 상성"));
    layout->addWidget(matchups);

    m_levelMoves = new MoveList(MoveList::Mode::LevelUp, m_icons);
    PanelFrame *level = section(tok::kInk, m_levelMoves, {8, 6, 8, 10});
    level->setTitle(tr("레벨업으로 익히는 기술"));
    layout->addWidget(level);

    m_machineMoves = new MoveList(MoveList::Mode::Machine, m_icons);
    m_machinePanel = section(tok::kBlueDeep, m_machineMoves, {8, 6, 8, 10});
    layout->addWidget(m_machinePanel);
    layout->addStretch();
    return content;
}

void DexDetailPage::showPokemon(int pokemonId, const QString &versionGroup)
{
    m_detail.pokemonId = pokemonId;
    m_versionGroup = versionGroup;
    reload();
    m_scroll->verticalScrollBar()->setValue(0);
}

void DexDetailPage::reload()
{
    if (m_detail.pokemonId <= 0)
        return;
    const int generation = m_state->generation();
    // 받은 기준 게임이 이 세대 것이 아니면(세대를 바꿨다) Repository가 대표 게임으로 대신한다
    PokemonDetail detail
            = m_repository->pokemonDetail(m_detail.pokemonId, generation, m_versionGroup);
    if (detail.types.isEmpty()) { // 이 세대에는 없는 포켓몬(세대를 앞으로 돌렸다)
        emit backRequested();
        return;
    }
    m_detail = std::move(detail);
    m_chart = m_repository->typeChart(generation);
    qCInfo(lcUi) << "dex detail" << m_detail.pokemonId << "generation" << generation
                 << m_detail.versionGroup;
    applyLanguage();
}

void DexDetailPage::showNaturePicker()
{
    if (m_natures.isEmpty())
        m_natures = m_repository->natures();
    // 팝업은 한 번 쓰고 버린다(WA_DeleteOnClose). 버튼 바로 아래, 버튼 오른쪽 끝에 맞춰 연다.
    NaturePicker *picker = new NaturePicker(m_natures, m_natureId, m_state->language(), this);
    connect(picker, &NaturePicker::natureChosen, this, [this](int id) {
        m_natureId = id;
        applyNature();
    });
    const QPoint bottomRight = m_natureButton->mapToGlobal(
            QPoint(m_natureButton->width(), m_natureButton->height() + 4));
    picker->move(bottomRight - QPoint(picker->width(), 0));
    picker->show();
}

void DexDetailPage::applyNature()
{
    // 성격은 3세대부터 — 1 · 2세대에서는 버튼을 숨기고 표시도 지운다
    const bool available = m_detail.generation >= kFirstNatureGeneration;
    m_natureButton->setVisible(available);
    const Nature *nature = nullptr;
    for (const Nature &n : std::as_const(m_natures))
        if (n.id == m_natureId)
            nature = &n;
    if (!available || !nature) {
        m_natureButton->setText(tr("성격"));
        m_stats->setNature(-1, -1);
    } else {
        m_natureButton->setText(nature->name.text(m_state->language()));
        m_stats->setNature(nature->increasedStat - 1,
                           nature->decreasedStat - 1); // stats.id → 배열 위치
    }
    m_natureButton->updateGeometry(); // 글자 폭이 바뀌었다 → PanelFrame이 자리를 다시 잡는다
}

void DexDetailPage::applyLanguage()
{
    if (!m_detail.isValid())
        return;
    const Language language = m_state->language();
    // "기술 기준: 하트골드 · 소울실버 (HG · SS)" — 한국어판 제목(기라티나 = 플래티넘)만으로는
    // 헷갈려서 도감 버튼과 같은 약칭을 붙인다
    QStringList games;
    QStringList shorts;
    for (qsizetype i = 0; i < m_detail.groupGames.size(); ++i) {
        games.append(m_detail.groupGames.at(i).text(language));
        shorts.append(dexstyle::version(m_detail.groupVersions.value(i),
                                        m_detail.groupGames.at(i).text(Language::English))
                              .shortName);
    }
    m_basis->setText(
            tr("기술 기준: %1 (%2)")
                    .arg(games.join(QStringLiteral(" · ")), shorts.join(QStringLiteral(" · "))));
    m_profile->setDetail(m_detail, language);
    m_stats->setStats(m_detail.stats);
    m_statsPanel->setTitle(tr("종족값"), tr("합계 %1").arg(m_detail.total));
    const bool evolves = !m_detail.evolution.isEmpty();
    m_evolutionLabel->setVisible(evolves);
    m_evolution->setVisible(evolves);
    m_evolution->setEvolution(m_detail.evolution, m_detail.speciesId, language);
    bool evolvedForm = false; // 진화 전 단계가 있다(야생에 없으면 그 단계에서 진화시킨다)
    for (const EvolutionStep &step : std::as_const(m_detail.evolution))
        evolvedForm = evolvedForm || (step.speciesId == m_detail.speciesId && step.depth > 0);
    m_encounters->setEncounters(m_detail.encounters, language, evolvedForm);
    m_matchups->setMatchups(m_detail.types, m_chart, language);
    m_abilities->setAbilities(m_detail.abilities, m_detail.generation, language);
    applyNature();
    m_levelMoves->setMoves(m_detail.levelMoves, m_detail.versionGroup, m_detail.generation,
                           m_detail.types, language);
    m_machineMoves->setMoves(m_detail.machineMoves, m_detail.versionGroup, m_detail.generation,
                             m_detail.types, language);
    m_machinePanel->setTitle(tr("기술머신 · 비전머신"),
                             tr("%1개").arg(m_detail.machineMoves.size()));
}
} // namespace com::yamada::studio
