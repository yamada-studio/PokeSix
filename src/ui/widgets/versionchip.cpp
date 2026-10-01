#include "ui/widgets/versionchip.h"

#include "ui/theme/cursors.h"
#include "ui/theme/dexstyle.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"

#include <QFontMetricsF>
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

QFont suffixFont()
{
    return theme::font(theme::kFamilyBody, 10, QFont::Bold);
}
} // namespace

namespace com::yamada::studio {
VersionChip::VersionChip(const QList<Part> &parts, const QString &suffix, QWidget *parent)
    : QAbstractButton(parent)
    , m_parts(parts)
    , m_suffix(suffix)
{
    QAbstractButton::setCheckable(true);
    QAbstractButton::setCursor(cursors::pointer());
    QAbstractButton::setFocusPolicy(Qt::TabFocus);
}

QList<VersionChip::Part> VersionChip::partsFor(const QStringList &versions,
                                               const QList<LocalizedText> &names)
{
    QList<Part> parts;
    QStringList seen;
    for (qsizetype i = 0; i < versions.size(); ++i) {
        const dexstyle::VersionStyle style
                = dexstyle::version(versions.at(i), names.value(i).text(Language::English));
        if (seen.contains(style.shortName))
            continue;
        seen.append(style.shortName);
        parts.append({style.shortName, style.background, style.text});
    }
    return parts;
}

QString VersionChip::keyOf(const QList<Part> &parts)
{
    QString key;
    for (const Part &part : parts)
        key += part.text;
    return key;
}

QSize VersionChip::sizeHint() const
{
    const QFontMetricsF metrics(chipFont());
    qreal width = 0;
    for (const Part &part : m_parts)
        width += metrics.horizontalAdvance(part.text);
    if (!m_suffix.isEmpty())
        width += 9 + QFontMetricsF(suffixFont()).horizontalAdvance(m_suffix);
    return {int(width) + 2 * kPadding + 2, kChipHeight + 3}; // 3 = 그림자
}

void VersionChip::paintEvent(QPaintEvent *)
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
        painter.drawText(QRectF(textX, box.top(), w + 1, box.height()), Qt::AlignCenter, part.text);
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
} // namespace com::yamada::studio
