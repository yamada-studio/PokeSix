#include "ui/dex/moveeffect.h"

#include "core/rules/generationfeatures.h"
#include "ui/logging/logging.h"
#include "ui/theme/tokens.h"

#include <QCoreApplication>
#include <QFile>
#include <QFontMetricsF>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>

namespace {
using namespace com::yamada::studio;

constexpr char kContext[] = "com::yamada::studio::moveeffect";

QString t(const char *source)
{
    return QCoreApplication::translate(kContext, source);
}

// 대상이 자신(쪽)인 move_targets.id: 3 아군 · 4 자기 필드 · 5 자신 또는 아군 · 7 자신 · 13 자신과
// 아군 · 15 아군 전체. 나머지는 상대 쪽 → 능력치 변화 앞에 "상대"
bool targetsUser(int target)
{
    return target == 3 || target == 4 || target == 5 || target == 7 || target == 13 || target == 15;
}

using StatList = QList<std::pair<int, int>>;

struct History
{
    int untilGen = 0;
    StatList stats;
};

struct Rule
{
    QList<History> history;
    bool hasStats = false;
    StatList stats;
    LocalizedText note;
    LocalizedText ailment;
    QString userType;
    LocalizedText userTypeNote;
};

LocalizedText localizedOf(const QJsonValue &value)
{
    const QJsonObject o = value.toObject();
    return {o.value(QStringLiteral("ko")).toString(), o.value(QStringLiteral("en")).toString(),
            o.value(QStringLiteral("ja")).toString()};
}

StatList statsOf(const QJsonValue &value)
{
    StatList stats;
    for (const QJsonValue &pair : value.toArray())
        stats.append({pair.toArray().at(0).toInt(), pair.toArray().at(1).toInt()});
    return stats;
}

QHash<QString, Rule> load()
{
    QHash<QString, Rule> rules;
    QFile file(QStringLiteral(":/data/move-effects.json"));
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcUi) << "cannot open move-effects.json";
        return rules;
    }
    const QJsonObject moves = QJsonDocument::fromJson(file.readAll())
                                      .object()
                                      .value(QStringLiteral("moves"))
                                      .toObject();
    for (auto it = moves.begin(); it != moves.end(); ++it) {
        const QJsonObject o = it.value().toObject();
        Rule rule;
        for (const QJsonValue &h : o.value(QStringLiteral("history")).toArray())
            rule.history.append({h.toObject().value(QStringLiteral("untilGen")).toInt(),
                                 statsOf(h.toObject().value(QStringLiteral("stats")))});
        rule.hasStats = o.contains(QStringLiteral("stats"));
        rule.stats = statsOf(o.value(QStringLiteral("stats")));
        rule.note = localizedOf(o.value(QStringLiteral("note")));
        rule.ailment = localizedOf(o.value(QStringLiteral("ailment")));
        rule.userType = o.value(QStringLiteral("userType")).toString();
        rule.userTypeNote = localizedOf(o.value(QStringLiteral("userTypeNote")));
        rules.insert(it.key(), rule);
    }
    return rules;
}

const QHash<QString, Rule> &rules()
{
    static const QHash<QString, Rule> instance = load(); // 처음 부를 때 한 번
    return instance;
}

QString statName(int stat, bool specialSplit)
{
    switch (stat) {
    case 1:
        return t("HP");
    case 2:
        return t("공격");
    case 3:
        return t("방어");
    case 4:
        return specialSplit ? t("특공") : t("특수");
    case 5:
        return specialSplit ? t("특방") : t("특수");
    case 6:
        return t("스피드");
    case 7:
        return t("명중률");
    case 8:
        return t("회피율");
    default:
        return {};
    }
}

// 상태이상(move_meta_ailments.id). 이름으로 충분한 것만 — 나머지는 게임 설명문이 낫다
QString ailmentName(int ailment)
{
    switch (ailment) {
    case 1:
        return t("마비");
    case 2:
        return t("잠듦");
    case 3:
        return t("얼음");
    case 4:
        return t("화상");
    case 5:
        return t("독");
    case 6:
        return t("혼란");
    case 7:
        return t("헤롱헤롱");
    case 14:
        return t("하품(다음 턴 잠듦)");
    default:
        return {};
    }
}

// "A한다. B한다." → "A한다." (일본어 。도). 한 문장뿐이면 그대로
QString firstSentence(const QString &text)
{
    for (qsizetype i = 0; i + 1 < text.size(); ++i)
        if ((text.at(i) == QLatin1Char('.') && text.at(i + 1) == QLatin1Char(' '))
            || text.at(i) == QChar(0x3002))
            return text.left(i + 1);
    return text;
}

QString healingText(int healing)
{
    const int amount = healing < 0 ? -healing : healing;
    const QString part = amount == 50    ? QStringLiteral("½")
                         : amount == 25  ? QStringLiteral("¼")
                         : amount == 100 ? t("전부")
                                         : QStringLiteral("%1%").arg(amount);
    return (healing > 0 ? t("HP %1 회복") : t("HP %1 소모")).arg(part);
}
} // namespace

namespace com::yamada::studio::moveeffect {
Parts describe(const MoveEntry &move, int generation, const QStringList &userTypes,
               Language language)
{
    const GenerationFeatures features = featuresOf(generation);
    const Rule rule = rules().value(move.identifier);
    Parts parts;
    auto separator = [&] {
        if (!parts.isEmpty())
            parts.append({QStringLiteral(" · "), tok::kText3});
    };

    // 1) 쓰는 포켓몬의 타입에 따라 아예 다른 효과(저주: 고스트)
    if (!rule.userType.isEmpty() && userTypes.contains(rule.userType)) {
        parts.append({rule.userTypeNote.text(language), tok::kText2});
        return parts;
    }

    // 2) 능력치 변화: 그 세대의 옛 값(history) > 사전의 고정 값(stats) > DB(지금 값)
    StatList stats = rule.hasStats ? rule.stats : move.statChanges;
    for (const History &h : rule.history)
        if (generation <= h.untilGen) {
            stats = h.stats;
            break;
        }
    const bool self = targetsUser(move.target);
    QList<std::pair<QString, int>> named; // 1세대: 특공 · 특방 → "특수" 하나로 합친다
    for (const auto &[stat, change] : stats) {
        const QString name = statName(stat, features.specialSplit);
        if (name.isEmpty() || std::any_of(named.begin(), named.end(), [&](const auto &n) {
                return n.first == name && n.second == change;
            }))
            continue;
        named.append({name, change});
    }
    for (qsizetype i = 0; i < named.size(); ++i) {
        separator();
        if (!self && i == 0)
            parts.append({t("상대") + QLatin1Char(' '), tok::kText3});
        const int change = named.at(i).second;
        parts.append({named.at(i).first + QLatin1Char(' '), tok::kText1});
        parts.append({(change > 0 ? QStringLiteral("▲") : QStringLiteral("▼"))
                              + QString::number(change > 0 ? change : -change),
                      change > 0 ? tok::kRed : tok::kBlue});
    }

    // 3) 상태이상 · 회복
    const QString ailment
            = rule.ailment.isEmpty() ? ailmentName(move.ailment) : rule.ailment.text(language);
    if (!ailment.isEmpty()) {
        separator();
        parts.append({(self ? QString() : t("상대") + QLatin1Char(' ')) + ailment, tok::kText1});
    }
    if (move.healing != 0) {
        separator();
        parts.append({healingText(move.healing), move.healing > 0 ? tok::kGreen : tok::kText1});
    }
    if (!rule.note.isEmpty()) {
        separator();
        parts.append({rule.note.text(language), tok::kText2});
    }

    // 4) 뼈대 · 요약이 없다 → 그 세대의 게임 설명문 첫 문장(전체는 툴팁에서)
    if (parts.isEmpty() && !move.effect.isEmpty())
        parts.append({firstSentence(move.effect.text(language)), tok::kText3});
    return parts;
}

QString plainText(const Parts &parts)
{
    QString text;
    for (const Segment &s : parts)
        text += s.text;
    return text;
}

bool paint(QPainter &painter, const QRectF &rect, const Parts &parts, const QFont &font)
{
    painter.setFont(font);
    const QFontMetricsF metrics(font);
    qreal x = rect.left();
    for (const Segment &s : parts) {
        const qreal w = metrics.horizontalAdvance(s.text);
        painter.setPen(QColor(s.color));
        if (x + w > rect.right()) { // 넘친다: 남은 폭만큼 말줄임하고 멈춘다
            painter.drawText(QRectF(x, rect.top(), rect.right() - x, rect.height()),
                             Qt::AlignLeft | Qt::AlignVCenter,
                             metrics.elidedText(s.text, Qt::ElideRight, rect.right() - x));
            return true;
        }
        painter.drawText(QRectF(x, rect.top(), w + 1, rect.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, s.text);
        x += w;
    }
    return false;
}
} // namespace com::yamada::studio::moveeffect
