#include "ui/dex/movelist.h"

#include "data/sprites/spritecache.h"
#include "ui/dex/guidebook.h"
#include "ui/dex/moveeffect.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QEvent>
#include <QFontMetricsF>
#include <QHelpEvent>
#include <QPainter>
#include <QPixmap>
#include <QToolTip>

namespace {
using namespace com::yamada::studio;

constexpr int kRowHeight = 30;
constexpr int kHeaderHeight = 28;
constexpr int kPadding = 6;
constexpr int kNameWidth = 120;
constexpr int kTypeWidth = 74;
constexpr int kClassWidth = 46;
constexpr int kNumberWidth = 44; // 위력 · 명중 · PP
constexpr int kCostWidth = 150;  // NPC 가르침 비용("배틀프런티어 48BP")
constexpr int kFirstWidth = 56;  // Lv · 번호
constexpr char kHeartScale[] = "heart-scale";
constexpr int kVariablePower = 1;
constexpr int kPlacesGap = 14; // PP ↔ 효과 · 효과 ↔ 획득처
constexpr int kEffectMinWidth = 150;

// 분류 칩: 물리 · 특수 · 변화 (cat.physical · cat.special · cat.status)
void paintClass(QPainter &painter, const QRectF &cell, int damageClass)
{
    const QString text = damageClass == 2   ? MoveList::tr("물리")
                         : damageClass == 3 ? MoveList::tr("특수")
                                            : MoveList::tr("변화");
    const QRgb fill = damageClass == 2   ? tok::kCatPhysical
                      : damageClass == 3 ? tok::kCatSpecial
                                         : tok::kCatStatus;
    const QFont font = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    const qreal w = QFontMetricsF(font).horizontalAdvance(text) + 12;
    const QRectF chip(cell.left(), cell.center().y() - 9, w, 18);
    painter.setPen(QPen(QColor(tok::kInk), 1.5));
    painter.setBrush(QColor(fill));
    painter.drawRoundedRect(chip.adjusted(0.75, 0.75, -0.75, -0.75), 3, 3);
    painter.setFont(font);
    painter.setPen(QColor(damageClass == 1 ? tok::kText1 : tok::kWhite));
    painter.drawText(chip, Qt::AlignCenter, text);
}
} // namespace

namespace com::yamada::studio {
MoveList::MoveList(Mode mode, SpriteCache *icons, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_icons(icons)
{
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 하트비늘 아이콘이 받아지면 다시 그린다
    connect(m_icons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
}

void MoveList::setMoves(const QList<MoveEntry> &moves, const QString &versionGroup, int generation,
                        const QStringList &userTypes, Language language)
{
    m_moves = moves;
    m_userTypes = userTypes;
    m_versionGroup = versionGroup;
    m_generation = generation;
    m_language = language;
    QWidget::updateGeometry();
    QWidget::update();
}

QSize MoveList::sizeHint() const
{
    return {600, kHeaderHeight + int(std::max<qsizetype>(m_moves.size(), 1)) * kRowHeight};
}

QList<MoveList::Column> MoveList::columns() const
{
    // [첫 칸] [기술] [타입] [분류] [위력] [명중] [PP] [효과] ([획득처] — 기술머신만)
    // 남는 폭: 레벨업은 효과가 다 갖고, 기술머신은 효과 · 획득처가 반씩
    QList<Column> c;
    int x = kPadding;
    auto add = [&](int w) {
        c.append({x, w});
        x += w + kPadding;
    };
    // Plain(가르침 · 알)은 첫 칸이 없다 — 자리만 0으로 남겨 칸 번호를 다른 모드와 맞춘다
    add(m_mode == Mode::Plain ? -kPadding : kFirstWidth);
    add(kNameWidth);
    add(kTypeWidth);
    add(kClassWidth);
    add(kNumberWidth);
    add(kNumberWidth);
    add(kNumberWidth);
    x += kPlacesGap;
    const int rest = std::max(kEffectMinWidth, width() - x - kPadding);
    if (m_mode == Mode::Machine) {
        const int effect = std::max(kEffectMinWidth, (rest - kPlacesGap) / 2);
        add(effect);
        x += kPlacesGap;
        add(std::max(80, width() - x - kPadding));
    } else if (m_mode == Mode::Plain) {
        // 비용(NPC 가르침 — 컨텐츠 + 재화)은 PP 바로 뒤에 — 맨 오른쪽에 떨어뜨리면 줄 따라
        // 읽기 어렵다. 남는 폭은 효과가 갖는다.
        add(kCostWidth);
        add(std::max(kEffectMinWidth, width() - x - kPadding));
    } else {
        add(rest);
    }
    return c;
}

int MoveList::rowAt(int y) const
{
    const int row = (y - kHeaderHeight) / kRowHeight;
    return (y < kHeaderHeight || row >= m_moves.size()) ? -1 : row;
}

QString MoveList::placesOf(const MoveEntry &move) const
{
    return guidebook::itemSources(m_versionGroup, move.machineItem, m_language)
            .join(QStringLiteral(" / "));
}

void MoveList::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QList<Column> c = columns();
    auto cell = [&](int column, int top, int height) {
        return QRectF(c[column].x, top, c[column].width, height);
    };

    // 머리 줄
    const QFont headerFont = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    painter.setFont(headerFont);
    painter.setPen(QColor(tok::kText2));
    const QString headers[] = {m_mode == Mode::LevelUp   ? tr("Lv")
                               : m_mode == Mode::Machine ? tr("번호")
                                                         : QString(),
                               tr("기술"),
                               tr("타입"),
                               tr("분류"),
                               tr("위력"),
                               tr("명중"),
                               tr("PP"),
                               m_mode == Mode::Plain ? tr("비용") : tr("효과"),
                               m_mode == Mode::Plain ? tr("효과") : tr("획득처")};
    for (qsizetype i = 0; i < c.size(); ++i) {
        const bool numeric = i >= 4 && i <= 6;
        painter.drawText(cell(int(i), 0, kHeaderHeight),
                         (numeric ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignVCenter, headers[i]);
    }
    painter.fillRect(QRectF(0, kHeaderHeight - 2, width(), 2), QColor(tok::kInk));

    if (m_moves.isEmpty()) {
        painter.setFont(theme::font(theme::kFamilyBody, 13));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(kPadding, kHeaderHeight, width(), kRowHeight),
                         Qt::AlignLeft | Qt::AlignVCenter, tr("이 게임에서는 배울 수 없어요"));
        return;
    }

    const QFont nameFont = theme::font(theme::kFamilyBody, 13, QFont::ExtraBold);
    const QFont dataFont = theme::font(theme::kFamilyData, 13, QFont::Bold);
    const QFont placeFont = theme::font(theme::kFamilyBody, 12);
    for (qsizetype i = 0; i < m_moves.size(); ++i) {
        const MoveEntry &move = m_moves.at(i);
        const int top = kHeaderHeight + int(i) * kRowHeight;
        if (i % 2 == 1)
            painter.fillRect(QRect(0, top, width(), kRowHeight), QColor(tok::kPaperAlt));
        painter.fillRect(QRect(0, top + kRowHeight - 1, width(), 1), QColor(tok::kLineSoft));

        // 첫 칸: Lv(1이면 하트비늘) · 기술머신 번호(TM06 · HM01)
        painter.setFont(dataFont);
        painter.setPen(QColor(tok::kText1));
        if (m_mode == Mode::LevelUp) {
            const QRectF first = cell(0, top, kRowHeight);
            painter.drawText(first, Qt::AlignLeft | Qt::AlignVCenter, QString::number(move.level));
            if (move.needsReminder) { // 진화 전 단계에 없는 Lv 1 기술만(기본 기술은 표시하지
                                      // 않는다)
                const QString key = ItemRowDelegate::iconKey(QString::fromLatin1(kHeartScale), {},
                                                             m_generation);
                const QString file = m_icons->path(key);
                QPixmap icon;
                if (file.isEmpty() || !icon.load(file))
                    m_icons->request(key);
                else
                    painter.drawPixmap(
                            QPointF(first.left() + 18, first.center().y() - icon.height() / 2.0),
                            icon);
            }
        } else if (m_mode == Mode::Machine) {
            const QString prefix = move.hiddenMachine ? QStringLiteral("HM") : QStringLiteral("TM");
            painter.drawText(
                    cell(0, top, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                    prefix + QStringLiteral("%1").arg(move.machineNumber, 2, 10, QLatin1Char('0')));
        }
        // 기술 이름
        painter.setFont(nameFont);
        const QRectF name = cell(1, top, kRowHeight);
        painter.drawText(name, Qt::AlignLeft | Qt::AlignVCenter,
                         QFontMetricsF(nameFont).elidedText(move.name.text(m_language),
                                                            Qt::ElideRight, name.width()));
        // 타입 · 분류
        if (const tok::TypeColor *type = typechip::find(move.type))
            typechip::paint(painter, QPointF(c[2].x, top + (kRowHeight - typechip::kHeight) / 2),
                            *type, m_language);
        paintClass(painter, cell(3, top, kRowHeight), move.damageClass);
        // 위력 · 명중 · PP (0 = 없음 → "—")
        painter.setFont(dataFont);
        const int values[] = {move.power, move.accuracy, move.pp};
        for (int k = 0; k < 3; ++k) {
            // 위력 1 = 상대 · 상황에 따라 바뀌는 위력(PokéAPI 관례: 잠재파워 · 은혜갚기 등)
            const bool variable = k == 0 && values[k] == kVariablePower;
            QString text = values[k] > 0 ? QString::number(values[k]) : QStringLiteral("—");
            if (variable)
                text = tr("변동");
            painter.setPen(QColor(values[k] > 0 && !variable ? tok::kText1 : tok::kText3));
            painter.drawText(cell(4 + k, top, kRowHeight), Qt::AlignRight | Qt::AlignVCenter, text);
        }
        // 효과: 변화 기술만("공격 ▲2" · "상대 마비" · 그 세대의 게임 설명문)
        if (move.damageClass == 1)
            moveeffect::paint(painter, cell(effectColumn(), top, kRowHeight),
                              moveeffect::describe(move, m_generation, m_userTypes, m_language),
                              placeFont);
        // 비용(NPC 가르침): 사전(tutor-costs.json)에 있을 때만. 없으면 흐린 "—"
        if (m_mode == Mode::Plain) {
            const QString cost = guidebook::tutorCost(m_versionGroup, move.identifier, m_language);
            painter.setFont(placeFont);
            painter.setPen(QColor(cost.isEmpty() ? tok::kTextDisabled : tok::kText2));
            painter.drawText(cell(costColumn(), top, kRowHeight), Qt::AlignLeft | Qt::AlignVCenter,
                             cost.isEmpty() ? QStringLiteral("—") : cost);
        }
        // 획득처(기술머신)
        if (m_mode == Mode::Machine) {
            const QString places = placesOf(move);
            const QRectF where = cell(placesColumn(), top, kRowHeight);
            painter.setFont(placeFont);
            painter.setPen(QColor(places.isEmpty() ? tok::kTextDisabled : tok::kText2));
            painter.drawText(where, Qt::AlignLeft | Qt::AlignVCenter,
                             places.isEmpty() ? tr("획득처 정보 없음")
                                              : QFontMetricsF(placeFont).elidedText(
                                                        places, Qt::ElideRight, where.width()));
        }
    }
}

bool MoveList::event(QEvent *event)
{
    // 획득처가 잘렸을 때 전체를 툴팁으로. Lv 1 하트비늘에는 설명을.
    if (event->type() == QEvent::ToolTip) {
        const auto *help = static_cast<QHelpEvent *>(event);
        const int row = rowAt(help->pos().y());
        QString tip;
        if (row >= 0) {
            const MoveEntry &move = m_moves.at(row);
            const Column effect = columns().at(effectColumn());
            const bool overEffect
                    = help->pos().x() >= effect.x && help->pos().x() < effect.x + effect.width;
            if (overEffect && move.damageClass == 1) {
                // 효과 전체 + (뼈대로 그렸다면) 게임 설명문
                const moveeffect::Parts parts
                        = moveeffect::describe(move, m_generation, m_userTypes, m_language);
                tip = moveeffect::plainText(parts);
                const QString flavor = move.effect.text(m_language);
                if (!flavor.isEmpty() && flavor != tip)
                    tip += QStringLiteral("\n") + flavor;
            } else if (m_mode == Mode::Machine)
                tip = placesOf(move).replace(QStringLiteral(" · "), QStringLiteral("\n"));
            else if (move.needsReminder && help->pos().x() < columns().at(0).x + kFirstWidth)
                tip = tr("진화 전 단계에서는 배우지 않는 기술이에요. 진화한 뒤 기술 "
                         "떠올리기(하트비늘)로 "
                         "배워요.");
        }
        if (tip.isEmpty())
            QToolTip::hideText();
        else
            QToolTip::showText(help->globalPos(), tip, this);
        return true;
    }
    return QWidget::event(event);
}
} // namespace com::yamada::studio
