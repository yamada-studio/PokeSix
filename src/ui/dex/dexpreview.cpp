#include "ui/dex/dexpreview.h"

#include "data/sprites/spritecache.h"
#include "data/sprites/spritekeys.h"
#include "ui/dex/statradar.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/shadowbutton.h"
#include "ui/widgets/spritefit.h"
#include "ui/widgets/typechip.h"

#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QVBoxLayout>

namespace {
constexpr int kSpriteBoxHeight = 132;
constexpr int kSpriteScale = 2;
} // namespace

namespace com::yamada::studio {
// 위쪽: 그림 칸 · 번호 · 이름 · 타입 칩 (도감 상세 ProfileCard의 축소판)
class DexPreview::Head : public QWidget
{
public:
    Head(SpriteCache *fronts, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_fronts(fronts)
    {
        QWidget::setFixedHeight(kSpriteBoxHeight + 8 + 20 + 30 + int(typechip::kHeight) + 8);
        connect(m_fronts, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    }

    void set(const SpeciesRow &row, int generation, Language language)
    {
        m_row = row;
        m_generation = generation;
        m_language = language;
        QWidget::update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        if (m_row.speciesId <= 0)
            return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        // 그림 칸 (paper.alt · line 테)
        const QRect frame(0, 0, width(), kSpriteBoxHeight);
        painter.setPen(QPen(QColor(tok::kLine), 1.5));
        painter.setBrush(QColor(tok::kPaperAlt));
        painter.drawRoundedRect(QRectF(frame).adjusted(0.75, 0.75, -0.75, -0.75), 6, 6);
        const QString key = spritekeys::front(m_generation, m_row.pokemonId);
        const QString file = m_fronts->path(key);
        QPixmap sprite;
        if (file.isEmpty() || !sprite.load(file))
            m_fronts->request(key);
        else
            spritefit::draw(painter, frame.adjusted(4, 4, -4, -4), sprite, kSpriteScale);

        int y = frame.bottom() + 8;
        painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(0, y, width(), 18), Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("No. %1").arg(m_row.dexNumber, 3, 10, QLatin1Char('0')));
        y += 18;
        painter.setFont(theme::font(theme::kFamilyTitle, 22));
        painter.setPen(QColor(tok::kText1));
        painter.drawText(QRect(0, y, width(), 30), Qt::AlignLeft | Qt::AlignVCenter,
                         m_row.name.text(m_language));
        y += 32;
        qreal x = 0;
        for (const QString &identifier : m_row.types)
            if (const tok::TypeColor *type = typechip::find(identifier))
                x += typechip::paint(painter, QPointF(x, y), *type, m_language) + typechip::kGap;
    }

private:
    SpriteCache *m_fronts = nullptr;
    SpeciesRow m_row;
    int m_generation = 0;
    Language m_language = Language::Korean;
};

// 약점 줄: "×4" 칩들 / "×2" 칩들. 칩이 넘치면 다음 줄로 넘긴다(높이는 set…이 정한다).
class DexPreview::WeakRows : public QWidget
{
public:
    using QWidget::QWidget;

    void set(const QStringList &quad, const QStringList &twice, Language language)
    {
        m_quad = quad;
        m_twice = twice;
        m_language = language;
        QWidget::setFixedHeight(heightFor(width()));
        QWidget::update();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QWidget::resizeEvent(event);
        QWidget::setFixedHeight(heightFor(width()));
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        int y = 0;
        paintRow(painter, QStringLiteral("×4"), QColor(tok::kRed), m_quad, y);
        paintRow(painter, QStringLiteral("×2"), QColor(tok::kText2), m_twice, y);
        if (m_quad.isEmpty() && m_twice.isEmpty()) {
            painter.setFont(theme::font(theme::kFamilyBody, 12));
            painter.setPen(QColor(tok::kText3));
            painter.drawText(rect(), Qt::AlignLeft | Qt::AlignTop, DexPreview::tr("약점이 없어요"));
        }
    }

private:
    static constexpr int kLabelWidth = 26;
    static constexpr int kRowGap = 6;

    // 그리면서(paint) 재지 않도록, 같은 줄바꿈 계산을 높이 계산에도 쓴다
    int heightFor(int width) const
    {
        int y = 0;
        for (const QStringList *list : {&m_quad, &m_twice}) {
            if (list->isEmpty())
                continue;
            int x = kLabelWidth;
            int lines = 1;
            for (const QString &identifier : *list) {
                const tok::TypeColor *type = typechip::find(identifier);
                if (!type)
                    continue;
                const int w = int(typechip::width(*type, m_language));
                if (x + w > width && x > kLabelWidth) {
                    ++lines;
                    x = kLabelWidth;
                }
                x += w + int(typechip::kGap);
            }
            y += lines * (int(typechip::kHeight) + 4) + kRowGap;
        }
        return std::max(y, int(typechip::kHeight));
    }

    void paintRow(QPainter &painter, const QString &label, const QColor &labelColor,
                  const QStringList &types, int &y) const
    {
        if (types.isEmpty())
            return;
        painter.setFont(theme::font(theme::kFamilyData, 12, QFont::Bold));
        painter.setPen(labelColor);
        painter.drawText(QRect(0, y, kLabelWidth, int(typechip::kHeight)),
                         Qt::AlignLeft | Qt::AlignVCenter, label);
        qreal x = kLabelWidth;
        for (const QString &identifier : types) {
            const tok::TypeColor *type = typechip::find(identifier);
            if (!type)
                continue;
            const qreal w = typechip::width(*type, m_language);
            if (x + w > width() && x > kLabelWidth) {
                y += int(typechip::kHeight) + 4;
                x = kLabelWidth;
            }
            typechip::paint(painter, QPointF(x, y), *type, m_language);
            x += w + typechip::kGap;
        }
        y += int(typechip::kHeight) + 4 + kRowGap;
    }

    QStringList m_quad;
    QStringList m_twice;
    Language m_language = Language::Korean;
};

DexPreview::DexPreview(QWidget *parent)
    : QWidget(parent)
    , m_fronts(new SpriteCache(SpriteCache::Kind::PokemonFront, this))
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    m_empty = new QLabel(tr("목록에서 포켓몬을 고르면\n여기에 요약이 보여요"));
    m_empty->setObjectName(QStringLiteral("previewEmpty"));
    m_empty->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_empty, 1);

    m_head = new Head(m_fronts);
    layout->addWidget(m_head);
    m_statsCaption = new QLabel(tr("종족값"));
    m_statsCaption->setObjectName(QStringLiteral("filterCaption"));
    layout->addWidget(m_statsCaption);
    m_radar = new StatRadar;
    layout->addWidget(m_radar, 0, Qt::AlignHCenter);
    m_weakCaption = new QLabel(tr("약점 (받을 때)"));
    m_weakCaption->setObjectName(QStringLiteral("filterCaption"));
    layout->addWidget(m_weakCaption);
    m_weak = new WeakRows;
    layout->addWidget(m_weak);
    layout->addStretch();
    m_detail = new ShadowButton(ShadowButton::Variant::Primary);
    m_detail->setText(tr("자세히 보기"));
    layout->addWidget(m_detail);
    connect(m_detail, &ShadowButton::clicked, this, &DexPreview::detailRequested);

    clear();
}

void DexPreview::setSpecies(const SpeciesRow &row, int generation, const QStringList &quadWeak,
                            const QStringList &doubleWeak, Language language)
{
    m_empty->hide();
    for (QWidget *w : std::initializer_list<QWidget *> {m_head, m_statsCaption, m_radar,
                                                        m_weakCaption, m_weak, m_detail})
        w->show();
    m_head->set(row, generation, language);
    m_radar->setStats(row.stats);
    m_weak->set(quadWeak, doubleWeak, language);
}

void DexPreview::clear()
{
    for (QWidget *w : std::initializer_list<QWidget *> {m_head, m_statsCaption, m_radar,
                                                        m_weakCaption, m_weak, m_detail})
        w->hide();
    m_empty->show();
}
} // namespace com::yamada::studio
