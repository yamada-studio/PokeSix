#include "ui/items/itemdetailpane.h"

#include "data/sprites/spritecache.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/spritefit.h"
#include "ui/widgets/typechip.h"

#include <QFontMetricsF>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

namespace {
using namespace com::yamada::studio;

constexpr int kIconBox = 72; // 아이콘 30×30 도트의 2배 + 여백
} // namespace

namespace com::yamada::studio {
// 위쪽: 아이콘 칸 · 이름 · (기술머신이면 타입 칩 + 기술 이름) · 가격
class ItemDetailPane::Head : public QWidget
{
public:
    Head(SpriteCache *sprites, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_sprites(sprites)
    {
        QWidget::setFixedHeight(kIconBox + 8 + 26 + 22 + 20);
        connect(m_sprites, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    }

    void set(const ItemRow &item, int generation, Language language)
    {
        m_item = item;
        m_generation = generation;
        m_language = language;
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_item.id <= 0)
            return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 아이콘 칸 (paper.alt · line 테), 가운데에 2배 도트
        const QRect frame((width() - kIconBox) / 2, 0, kIconBox, kIconBox);
        painter.setPen(QPen(QColor(tok::kLine), 1.5));
        painter.setBrush(QColor(tok::kPaperAlt));
        painter.drawRoundedRect(QRectF(frame).adjusted(0.75, 0.75, -0.75, -0.75), 6, 6);
        // 아이콘 키는 목록과 같은 규칙(기술머신 = 담긴 기술의 타입 CD, 1–5세대 = BW 모양)
        const QString key
                = ItemRowDelegate::iconKey(m_item.identifier, m_item.machineType, m_generation);
        const QString file = m_sprites->path(key);
        QPixmap icon;
        if (file.isEmpty() || !icon.load(file))
            m_sprites->request(key);
        else
            spritefit::draw(painter, frame.adjusted(4, 4, -4, -4), icon);

        int y = frame.bottom() + 8;
        painter.setFont(theme::font(theme::kFamilyTitle, 20));
        painter.setPen(QColor(tok::kText1));
        painter.drawText(QRect(0, y, width(), 26), Qt::AlignHCenter | Qt::AlignVCenter,
                         m_item.name.text(m_language));
        y += 28;
        // 기술머신: 담긴 기술(타입 칩 + 이름)을 가운데에
        if (!m_item.machineMove.isEmpty()) {
            const tok::TypeColor *type = typechip::find(m_item.machineType);
            const QString move = m_item.machineMove.text(m_language);
            painter.setFont(theme::font(theme::kFamilyBody, 13, QFont::Bold));
            const qreal moveWidth = QFontMetricsF(painter.font()).horizontalAdvance(move);
            const qreal chipWidth = type ? typechip::width(*type, m_language) + 6 : 0;
            qreal x = (width() - chipWidth - moveWidth) / 2;
            if (type)
                x += typechip::paint(painter, QPointF(x, y), *type, m_language) + 6;
            painter.setPen(QColor(tok::kText1));
            painter.drawText(QPointF(x, y + typechip::kHeight - 5), move);
        }
        y += 24;
        painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(0, y, width(), 18), Qt::AlignHCenter | Qt::AlignVCenter,
                         m_item.cost > 0 ? ItemDetailPane::tr("%L1원").arg(m_item.cost)
                                         : ItemDetailPane::tr("비매품"));
    }

private:
    SpriteCache *m_sprites = nullptr;
    ItemRow m_item;
    int m_generation = 9;
    Language m_language = Language::Korean;
};

// 세대별 존재: 1–9 칸. 있는 세대는 초록, 지금 세대는 노란 테로 강조(디자인 13_items의 세대 칸)
class ItemDetailPane::GenerationCells : public QWidget
{
public:
    explicit GenerationCells(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        QWidget::setFixedHeight(kCell + 14);
    }

    void set(quint16 generations, int current)
    {
        m_generations = generations;
        m_current = current;
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const int count = 9;
        const qreal step = kCell + 4;
        qreal x = (width() - (count * step - 4)) / 2;
        painter.setFont(theme::font(theme::kFamilyData, 9, QFont::Bold));
        for (int generation = 1; generation <= count; ++generation) {
            const QRectF cell(x, 0, kCell, kCell);
            const bool exists = m_generations & (1u << (generation - 1));
            painter.setPen(QPen(QColor(tok::kInk), 1.2));
            painter.setBrush(QColor(exists ? tok::kGreen : tok::kPaperAlt));
            painter.drawRoundedRect(cell, 3, 3);
            if (generation == m_current) { // 지금 세대: 노란 테
                painter.setPen(QPen(QColor(tok::kYellow), 2));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(cell.adjusted(-1.5, -1.5, 1.5, 1.5), 4, 4);
            }
            painter.setPen(QColor(exists ? tok::kWhite : tok::kText3));
            painter.drawText(cell, Qt::AlignCenter, QString::number(generation));
            x += step;
        }
    }

private:
    static constexpr int kCell = 18;

    quint16 m_generations = 0;
    int m_current = 0;
};

// 진화 대상 줄: [아이콘] 무우마 → [아이콘] 무우마직  (지니고 교환 · P 한정)
class ItemDetailPane::EvolutionRows : public QWidget
{
public:
    struct Row
    {
        int fromPokemonId = 0;
        int toPokemonId = 0;
        QString from;
        QString to;
        QString note;
    };

    EvolutionRows(SpriteCache *icons, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_icons(icons)
    {
        connect(m_icons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    }

    void setRows(const QList<Row> &rows)
    {
        m_rows = rows;
        QWidget::setFixedHeight(int(rows.size()) * kRowHeight);
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const QFont nameFont = theme::font(theme::kFamilyBody, 12, QFont::Bold);
        const QFont noteFont = theme::font(theme::kFamilyBody, 11);
        for (qsizetype i = 0; i < m_rows.size(); ++i) {
            const Row &row = m_rows.at(i);
            const qreal top = qreal(i) * kRowHeight;
            qreal x = 0;
            auto pokemon = [&](int pokemonId, const QString &name) {
                if (pokemonId > 0) {
                    DexRowDelegate::paintPokemonIcon(&painter, QRectF(x, top, kIcon, kRowHeight),
                                                     pokemonId, m_icons);
                    x += kIcon + 2;
                }
                painter.setFont(nameFont);
                painter.setPen(QColor(tok::kText1));
                const qreal w = QFontMetricsF(nameFont).horizontalAdvance(name);
                painter.drawText(QRectF(x, top, w + 1, kRowHeight),
                                 Qt::AlignLeft | Qt::AlignVCenter, name);
                x += w;
            };
            if (!row.from.isEmpty()) {
                pokemon(row.fromPokemonId, row.from);
                painter.setPen(QColor(tok::kText3));
                painter.drawText(QRectF(x, top, 22, kRowHeight), Qt::AlignCenter,
                                 QStringLiteral("→"));
                x += 22;
            }
            pokemon(row.toPokemonId, row.to);
            if (!row.note.isEmpty()) {
                painter.setFont(noteFont);
                painter.setPen(QColor(tok::kText3));
                const QRectF rest(x + 6, top, width() - x - 6, kRowHeight);
                painter.drawText(
                        rest, Qt::AlignLeft | Qt::AlignVCenter,
                        QFontMetricsF(noteFont).elidedText(row.note, Qt::ElideRight, rest.width()));
            }
        }
    }

private:
    static constexpr int kRowHeight = 30;
    static constexpr int kIcon = 34;
    SpriteCache *m_icons = nullptr;
    QList<Row> m_rows;
};

ItemDetailPane::ItemDetailPane(SpriteCache *sprites, QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_empty = new QLabel(tr("목록에서 아이템을 고르면\n여기에 자세히 보여요"));
    m_empty->setObjectName(QStringLiteral("previewEmpty"));
    m_empty->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_empty, 1);

    m_head = new Head(sprites);
    layout->addWidget(m_head);
    m_generationCaption = new QLabel(tr("세대별 존재"));
    m_generations = new GenerationCells;
    m_effectCaption = new QLabel(tr("효과"));
    m_effect = new QLabel;
    m_effect->setWordWrap(true);
    m_effect->setObjectName(QStringLiteral("itemEffect"));
    m_pokemonIcons = new SpriteCache(SpriteCache::Kind::PokemonIcon, this);
    m_extraCaption = new QLabel(tr("진화"));
    m_extra = new EvolutionRows(m_pokemonIcons);
    m_learnerCaption = new QLabel;
    m_learners = new PokemonIconGrid(m_pokemonIcons);
    m_sourceCaption = new QLabel(tr("입수처"));
    m_sources = new QLabel;
    m_sources->setWordWrap(true);
    m_sources->setObjectName(QStringLiteral("itemEffect"));
    for (QLabel *caption :
         {m_generationCaption, m_effectCaption, m_extraCaption, m_learnerCaption, m_sourceCaption})
        caption->setObjectName(QStringLiteral("filterCaption"));
    // 입수처를 배울 수 있는 포켓몬보다 위에 — 포켓몬 목록은 길어질 수 있다
    for (QWidget *w : std::initializer_list<QWidget *> {
                 m_generationCaption, m_generations, m_effectCaption, m_effect, m_extraCaption,
                 m_extra, m_sourceCaption, m_sources, m_learnerCaption, m_learners})
        layout->addWidget(w);
    layout->addStretch();
    clear();
}

void ItemDetailPane::setItem(const ItemRow &item, int generation, Language language,
                             const QList<ItemEvolution> &evolutions, const QStringList &sources,
                             const QList<PokemonIconGrid::Entry> &learners)
{
    m_empty->hide();
    for (QWidget *w :
         std::initializer_list<QWidget *> {m_head, m_generationCaption, m_generations,
                                           m_effectCaption, m_effect, m_sourceCaption, m_sources})
        w->show();
    m_head->set(item, generation, language);
    m_generations->set(item.generations, generation);
    const QString effect = item.effect.text(language);
    m_effect->setText(effect.isEmpty() ? tr("효과 설명이 없어요") : effect);

    // 진화 아이템이면 진화 대상(없으면 칸째 숨김): 아이콘 + 이름, 뒤에 조건(지니고 교환 · 버전
    // 한정)
    QList<EvolutionRows::Row> rows;
    for (const ItemEvolution &evolution : evolutions) {
        QStringList notes;
        if (evolution.held)
            notes.append(tr("지니고 교환"));
        if (!evolution.onlyVersions.isEmpty()) { // 버전 한정 포켓몬: "SS 한정"
            QStringList names;
            for (const QString &version : evolution.onlyVersions)
                names.append(dexstyle::version(version, {}).shortName);
            notes.append(tr("%1 한정").arg(names.join(QLatin1Char('/'))));
        }
        rows.append({evolution.fromPokemonId, evolution.toPokemonId, evolution.from.text(language),
                     evolution.to.text(language), notes.join(QStringLiteral(" · "))});
    }
    m_extra->setRows(rows);
    m_extraCaption->setVisible(!rows.isEmpty());
    m_extra->setVisible(!rows.isEmpty());

    // 기술머신이면 배울 수 있는 포켓몬(고른 게임 묶음 기준)
    m_learnerCaption->setText(tr("배울 수 있는 포켓몬 · %1마리").arg(learners.size()));
    m_learners->setEntries(learners);
    m_learnerCaption->setVisible(!learners.isEmpty());
    m_learners->setVisible(!learners.isEmpty());

    // 입수처: 고른 게임의 입수 사전. 아직 없으면 그렇다고 알린다(빈칸보다 낫다)
    m_sources->setText(sources.isEmpty()
                               ? tr("이 게임의 입수 정보가 아직 없어요")
                               : QStringLiteral("· ") + sources.join(QStringLiteral("\n· ")));
}

void ItemDetailPane::clear()
{
    for (QWidget *w : std::initializer_list<QWidget *> {
                 m_head, m_generationCaption, m_generations, m_effectCaption, m_effect,
                 m_extraCaption, m_extra, m_sourceCaption, m_sources, m_learnerCaption, m_learners})
        w->hide();
    m_empty->show();
}
} // namespace com::yamada::studio
