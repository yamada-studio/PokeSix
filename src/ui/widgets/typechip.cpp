#include "ui/widgets/typechip.h"

#include "ui/theme/theme.h"

#include <QColor>
#include <QFontMetricsF>
#include <QPainter>

#include <iterator>

namespace {
using com::yamada::studio::Language;

constexpr qreal kPaddingX = 7;
constexpr qreal kRadius = 4;
constexpr qreal kBorder = 1.5;

// 영어 · 일본어 타입 이름(게임 표기). 한국어는 tokens.h의 kTypes에 있다. 순서 = kTypes 순서.
// 타입 이름은 세대가 바뀌어도 그대로라서 DB에서 읽지 않고 여기 둔다(칩은 그리는 곳마다 쓰인다).
constexpr const char16_t *kEnglish[]
        = {u"Normal",   u"Fire",   u"Water",  u"Grass",  u"Electric", u"Ice",
           u"Fighting", u"Poison", u"Ground", u"Flying", u"Psychic",  u"Bug",
           u"Rock",     u"Ghost",  u"Dragon", u"Dark",   u"Steel",    u"Fairy"};
constexpr const char16_t *kJapanese[]
        = {u"ノーマル", u"ほのお",   u"みず",     u"くさ",   u"でんき",   u"こおり",
           u"かくとう", u"どく",     u"じめん",   u"ひこう", u"エスパー", u"むし",
           u"いわ",     u"ゴースト", u"ドラゴン", u"あく",   u"はがね",   u"フェアリー"};
static_assert(std::size(kEnglish) == com::yamada::studio::tok::kTypes.size());
static_assert(std::size(kJapanese) == com::yamada::studio::tok::kTypes.size());
} // namespace

namespace com::yamada::studio::typechip {
const tok::TypeColor *find(const QString &identifier)
{
    const QByteArray key = identifier.toLatin1();
    for (const tok::TypeColor &type : tok::kTypes) {
        if (key == type.key)
            return &type;
    }
    return nullptr;
}

QFont font()
{
    return theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
}

QString label(const tok::TypeColor &type, Language language)
{
    const auto index = &type - tok::kTypes.data(); // kTypes 안의 위치
    if (language == Language::English)
        return QString::fromUtf16(kEnglish[index]);
    if (language == Language::Japanese)
        return QString::fromUtf16(kJapanese[index]);
    return QString::fromUtf16(type.ko);
}

qreal width(const tok::TypeColor &type, Language language)
{
    return QFontMetricsF(font()).horizontalAdvance(label(type, language)) + 2 * kPaddingX;
}

qreal paint(QPainter &painter, const QPointF &topLeft, const tok::TypeColor &type,
            Language language)
{
    const QString name = label(type, language);
    const QRectF chip(topLeft, QSizeF(width(type, language), kHeight));
    const qreal half = kBorder / 2.0;
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(QColor(tok::kInk), kBorder));
    painter.setBrush(QColor(type.fill));
    painter.drawRoundedRect(chip.adjusted(half, half, -half, -half), kRadius - half,
                            kRadius - half);
    painter.setFont(font());
    painter.setPen(QColor(type.text));
    painter.drawText(chip, Qt::AlignCenter, name);
    painter.restore();
    return chip.width();
}
} // namespace com::yamada::studio::typechip
