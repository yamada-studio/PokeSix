#include "ui/squad/dexfilterbar.h"

#include "ui/theme/cursors.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QFontMetricsF>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>

namespace {
using namespace com::yamada::studio;

constexpr int kChipHeight = 26;
constexpr int kPadding = 9;     // 칩 좌우 여백
constexpr qreal kDimmed = 0.72; // 꺼진 칩의 불투명도(마우스가 오르면 조금 진하게)

QFont chipFont()
{
    return theme::font(theme::kFamilyBody, 12, QFont::ExtraBold);
}

// 칩 하나. parts = 버전 약칭 조각(각자 바탕 · 글자색). 전국 칩은 흰 바탕 · 먹 글자 한 조각.
class DexChip : public QAbstractButton
{
public:
    struct Part
    {
        QString text;
        QColor background;
        QColor color;
    };

    DexChip(const QList<Part> &parts, const QString &suffix, QWidget *parent = nullptr)
        : QAbstractButton(parent)
        , m_parts(parts)
        , m_suffix(suffix)
    {
        QAbstractButton::setCheckable(true);
        QAbstractButton::setCursor(cursors::pointer());
        QAbstractButton::setFocusPolicy(Qt::TabFocus);
    }

    QSize sizeHint() const override
    {
        const QFontMetricsF metrics(chipFont());
        qreal width = 0;
        for (const Part &part : m_parts)
            width += metrics.horizontalAdvance(part.text);
        if (!m_suffix.isEmpty())
            width += 9 + QFontMetricsF(suffixFont()).horizontalAdvance(m_suffix);
        return {int(width) + 2 * kPadding + 2, kChipHeight + 3}; // 3 = 그림자
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        const bool on = isChecked();
        painter.setOpacity(on ? 1.0 : (underMouse() ? 0.8 : kDimmed));
        const QRectF box(1, 1, width() - 2, kChipHeight - 2);
        if (on) { // 켠 칩만 아래 그림자(눌린 버튼처럼 떠 보이게)
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(tok::kInk));
            painter.drawRoundedRect(box.translated(0, 2), 5, 5);
        }
        // 바탕: 약칭 자리만 조각마다 버전 색(글자 폭 비율), 뒤에 붙는 이름 자리는 흰 바탕
        QPainterPath clip;
        clip.addRoundedRect(box, 5, 5);
        painter.save();
        painter.setClipPath(clip);
        painter.fillRect(box, QColor(tok::kWhite));
        const QFontMetricsF metrics(chipFont());
        qreal total = 0;
        for (const Part &part : m_parts)
            total += metrics.horizontalAdvance(part.text);
        const qreal colored = m_suffix.isEmpty() ? box.width() : kPadding + total + 3;
        // 조각 경계 = 글자 경계(D|P의 사이). 첫 조각은 왼쪽 여백까지, 마지막 조각은 끝까지
        qreal x = box.left();
        qreal boundary = kPadding + 1;
        for (qsizetype i = 0; i < m_parts.size(); ++i) {
            boundary += metrics.horizontalAdvance(m_parts.at(i).text);
            const qreal end = i == m_parts.size() - 1 ? box.left() + colored : boundary;
            painter.fillRect(QRectF(x, box.top(), end - x, box.height()), m_parts.at(i).background);
            x = end;
        }
        if (!m_suffix.isEmpty()) {
            painter.setPen(QPen(QColor(tok::kLine), 1));
            painter.drawLine(QPointF(x, box.top()), QPointF(x, box.bottom()));
        }
        painter.restore();
        painter.setPen(QPen(QColor(on ? tok::kInk : tok::kLineStrong), on ? 2 : 1.5));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(box, 5, 5);

        // 글자: 조각마다 그 버전 색으로 이어 쓴다
        qreal textX = kPadding + 1;
        painter.setFont(chipFont());
        for (const Part &part : m_parts) {
            const qreal w = metrics.horizontalAdvance(part.text);
            painter.setPen(part.color);
            painter.drawText(QRectF(textX, box.top(), w + 1, box.height()), Qt::AlignCenter,
                             part.text);
            textX += w;
        }
        if (!m_suffix.isEmpty()) {
            painter.setFont(suffixFont());
            painter.setPen(QColor(tok::kText2));
            painter.drawText(QRectF(textX + 7, box.top(), width() - textX, box.height()),
                             Qt::AlignLeft | Qt::AlignVCenter, m_suffix);
        }
        if (hasFocus()) {
            painter.setPen(QPen(QColor(tok::kBlue), 2));
            painter.drawRoundedRect(box.adjusted(-0.5, -0.5, 0.5, 0.5), 6, 6);
        }
    }

private:
    static QFont suffixFont() { return theme::font(theme::kFamilyBody, 10, QFont::Bold); }

    QList<Part> m_parts;
    QString m_suffix;
};
} // namespace

namespace com::yamada::studio {
DexFilterBar::DexFilterBar(QWidget *parent)
    : QWidget(parent)
    , m_group(new QButtonGroup(this))
    , m_layout(new QHBoxLayout(this))
{
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(5);
    m_group->setExclusive(true);
    connect(m_group, &QButtonGroup::idClicked, this, &DexFilterBar::dexSelected);
}

int DexFilterBar::currentDex() const
{
    return std::max(kNational, m_group->checkedId());
}

void DexFilterBar::setDexes(const QList<DexInfo> &dexes, Language language, int checkedId)
{
    for (QAbstractButton *button : m_group->buttons()) {
        m_group->removeButton(button);
        delete button;
    }
    auto add = [this](DexChip *chip, const QString &toolTip, int id) {
        chip->setToolTip(toolTip);
        m_group->addButton(chip, id);
        m_layout->addWidget(chip);
    };
    add(new DexChip({{tr("전국"), QColor(tok::kWhite), QColor(tok::kInk)}}, {}),
        tr("그 세대까지 나온 포켓몬 전부"), kNational);

    // 칩 글자(버전 약칭 이어 붙임)가 겹치는 도감(칼로스 센트럴 · 코스트 · 마운틴 = 모두 XY)은
    // dexstyle의 이름을 뒤에 붙인다
    QList<DexInfo> shown;
    QList<QList<DexChip::Part>> parts;
    QStringList keys;
    for (const DexInfo &dex : dexes) {
        if (dexstyle::dex(dex.identifier).hidden)
            continue;
        QList<DexChip::Part> chip;
        QStringList seen;
        for (qsizetype i = 0; i < dex.versions.size(); ++i) {
            const dexstyle::VersionStyle style = dexstyle::version(
                    dex.versions.at(i), dex.versionNames.value(i).text(Language::English));
            if (seen.contains(style.shortName))
                continue; // 같은 약칭은 한 번(관동: 일본판 레드 = 레드 = R)
            seen.append(style.shortName);
            chip.append({style.shortName, style.background, style.text});
        }
        shown.append(dex);
        parts.append(chip);
        keys.append(seen.join(QString()));
    }
    for (qsizetype i = 0; i < shown.size(); ++i) {
        const DexInfo &dex = shown.at(i);
        const dexstyle::DexStyle rule = dexstyle::dex(dex.identifier);
        const QString region
                = rule.label.isEmpty() ? dex.region.text(language) : rule.label.text(language);
        const QString suffix = keys.count(keys.at(i)) > 1 ? region : QString();
        QStringList names;
        for (const LocalizedText &name : dex.versionNames)
            names.append(name.text(language));
        add(new DexChip(parts.at(i), suffix),
            tr("%1 도감 — %2").arg(region, names.join(QStringLiteral(" · "))), dex.pokedexId);
    }
    QAbstractButton *checked = m_group->button(checkedId);
    (checked ? checked : m_group->button(kNational))->setChecked(true);
    QWidget::updateGeometry();
}
} // namespace com::yamada::studio
