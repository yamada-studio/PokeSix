#include "ui/items/itemdetailpane.h"

#include "data/sprites/spritecache.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/spritefit.h"
#include "ui/widgets/typechip.h"

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
    m_extraCaption = new QLabel;
    m_extra = new QLabel;
    m_extra->setWordWrap(true);
    m_extra->setObjectName(QStringLiteral("itemEffect"));
    for (QLabel *caption : {m_generationCaption, m_effectCaption, m_extraCaption})
        caption->setObjectName(QStringLiteral("filterCaption"));
    for (QWidget *w :
         std::initializer_list<QWidget *> {m_generationCaption, m_generations, m_effectCaption,
                                           m_effect, m_extraCaption, m_extra})
        layout->addWidget(w);
    layout->addStretch();
    clear();
}

void ItemDetailPane::setItem(const ItemRow &item, int generation, Language language,
                             const QList<ItemEvolution> &evolutions,
                             const QStringList &machinePlaces)
{
    m_empty->hide();
    for (QWidget *w : std::initializer_list<QWidget *> {m_head, m_generationCaption, m_generations,
                                                        m_effectCaption, m_effect})
        w->show();
    m_head->set(item, generation, language);
    m_generations->set(item.generations, generation);
    const QString effect = item.effect.text(language);
    m_effect->setText(effect.isEmpty() ? tr("효과 설명이 없어요") : effect);

    // 셋째 칸: 진화 아이템이면 진화 대상, 기술머신이면 획득처. 없으면 숨긴다.
    QStringList lines;
    QString caption;
    if (!evolutions.isEmpty()) {
        caption = tr("진화");
        for (const ItemEvolution &evolution : evolutions) {
            const QString from = evolution.from.text(language);
            const QString to = evolution.to.text(language);
            QString line = from.isEmpty() ? to : tr("%1 → %2").arg(from, to);
            if (evolution.held)
                line += tr(" (지니고 교환)");
            lines.append(line);
        }
    } else if (!machinePlaces.isEmpty()) {
        caption = tr("획득처");
        lines = machinePlaces;
    }
    m_extraCaption->setText(caption);
    m_extra->setText(QStringLiteral("· ") + lines.join(QStringLiteral("\n· ")));
    m_extraCaption->setVisible(!lines.isEmpty());
    m_extra->setVisible(!lines.isEmpty());
}

void ItemDetailPane::clear()
{
    for (QWidget *w :
         std::initializer_list<QWidget *> {m_head, m_generationCaption, m_generations,
                                           m_effectCaption, m_effect, m_extraCaption, m_extra})
        w->hide();
    m_empty->show();
}
} // namespace com::yamada::studio
