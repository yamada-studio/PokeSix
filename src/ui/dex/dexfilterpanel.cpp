#include "ui/dex/dexfilterpanel.h"

#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/rangeslider.h"
#include "ui/widgets/shadowbutton.h"
#include "ui/widgets/typechip.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>

namespace com::yamada::studio {
namespace {
constexpr int kTypeColumns = 3;

QLabel *caption(const QString &text)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(QStringLiteral("filterCaption")); // app.qss가 모양을 정한다
    return label;
}
} // namespace

// 타입 칩 토글 버튼: 켜지면 제 색, 꺼지면 흐리게. 그리기는 typechip::paint 그대로.
class TypeToggle : public QAbstractButton
{
public:
    TypeToggle(const QString &identifier, Language language, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_identifier(identifier)
        , m_language(language)
    {
        QAbstractButton::setCheckable(true);
        QAbstractButton::setCursor(cursors::pointer());
        const tok::TypeColor *type = typechip::find(identifier);
        QAbstractButton::setText(type ? typechip::label(*type, language) : identifier);
    }

    QString identifier() const { return m_identifier; }

    QSize sizeHint() const override
    {
        const tok::TypeColor *type = typechip::find(m_identifier);
        return {type ? int(typechip::width(*type, m_language)) : 40, int(typechip::kHeight)};
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        const tok::TypeColor *type = typechip::find(m_identifier);
        if (!type)
            return;
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setOpacity(isChecked() ? 1.0 : 0.35); // 꺼진 칩은 흐리게
        typechip::paint(painter, QPointF(0, 0), *type, m_language);
    }

private:
    QString m_identifier;
    Language m_language;
};

DexFilterPanel::DexFilterPanel(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    layout->addWidget(caption(tr("타입")));
    m_typeGrid = new QGridLayout;
    m_typeGrid->setContentsMargins(0, 0, 0, 0);
    m_typeGrid->setHorizontalSpacing(4);
    m_typeGrid->setVerticalSpacing(5);
    layout->addLayout(m_typeGrid);

    layout->addSpacing(6);
    layout->addWidget(caption(tr("종족값 합계")));
    m_total = new RangeSlider(kTotalMinimum, kTotalMaximum, kTotalStep);
    layout->addWidget(m_total);
    m_totalLabel = new QLabel;
    m_totalLabel->setObjectName(QStringLiteral("filterValue"));
    layout->addWidget(m_totalLabel);
    connect(m_total, &RangeSlider::valuesChanged, this, [this] {
        updateTotalLabel();
        emit changed();
    });

    layout->addSpacing(6);
    m_legendary = new QCheckBox(tr("전설 · 환상 제외"));
    m_finalOnly = new QCheckBox(tr("최종 진화만"));
    for (QCheckBox *box : {m_legendary, m_finalOnly}) {
        box->setCursor(cursors::pointer());
        layout->addWidget(box);
        connect(box, &QCheckBox::toggled, this, &DexFilterPanel::changed);
    }

    layout->addStretch();
    m_reset = new ShadowButton(ShadowButton::Variant::Secondary);
    m_reset->setText(tr("필터 초기화"));
    layout->addWidget(m_reset);
    connect(m_reset, &ShadowButton::clicked, this, &DexFilterPanel::reset);

    updateTotalLabel();
}

void DexFilterPanel::setTypes(const QStringList &types, Language language)
{
    for (TypeToggle *button : m_typeButtons) {
        m_typeGrid->removeWidget(button);
        button->deleteLater(); // 클릭 신호 처리 중에 다시 만들 수도 있다 → delete는 뒤로 미룬다
    }
    m_typeButtons.clear();
    for (qsizetype i = 0; i < types.size(); ++i) {
        TypeToggle *button = new TypeToggle(types.at(i), language);
        m_typeGrid->addWidget(button, int(i) / kTypeColumns, int(i) % kTypeColumns,
                              Qt::AlignLeft | Qt::AlignVCenter);
        connect(button, &QAbstractButton::toggled, this, &DexFilterPanel::changed);
        m_typeButtons.append(button);
    }
}

void DexFilterPanel::reset()
{
    // 각 setChecked · setValues가 저마다 changed를 보낸다(바뀐 것이 있을 때만) — 거르기가 서너 번
    // 되풀이되지만 목록이 짧아(종 1천 줄) 체감되지 않는다. 묶고 싶어지면 blockSignals로 한 번만.
    for (TypeToggle *button : m_typeButtons)
        button->setChecked(false);
    m_total->setValues(kTotalMinimum, kTotalMaximum);
    m_legendary->setChecked(false);
    m_finalOnly->setChecked(false);
}

QSet<QString> DexFilterPanel::selectedTypes() const
{
    QSet<QString> types;
    for (TypeToggle *button : m_typeButtons)
        if (button->isChecked())
            types.insert(button->identifier());
    return types;
}

int DexFilterPanel::minimumTotal() const
{
    return m_total->lowerValue() <= kTotalMinimum ? 0 : m_total->lowerValue();
}

int DexFilterPanel::maximumTotal() const
{
    return m_total->upperValue() >= kTotalMaximum ? 9999 : m_total->upperValue();
}

bool DexFilterPanel::excludeLegendary() const
{
    return m_legendary->isChecked();
}

bool DexFilterPanel::finalEvolutionOnly() const
{
    return m_finalOnly->isChecked();
}

void DexFilterPanel::updateTotalLabel()
{
    const bool noLower = m_total->lowerValue() <= kTotalMinimum;
    const bool noUpper = m_total->upperValue() >= kTotalMaximum;
    if (noLower && noUpper)
        m_totalLabel->setText(tr("전체"));
    else
        m_totalLabel->setText(QStringLiteral("%1 – %2").arg(
                noLower ? tr("최소") : QString::number(m_total->lowerValue()),
                noUpper ? tr("최대") : QString::number(m_total->upperValue())));
}
} // namespace com::yamada::studio
