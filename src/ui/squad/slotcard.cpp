#include "ui/squad/slotcard.h"

#include "core/rules/generationfeatures.h"
#include "core/types/typekey.h"
#include "data/sprites/spritecache.h"
#include "data/state/squadsession.h"
#include "ui/dex/dexrowdelegate.h"
#include "ui/items/itemrowdelegate.h"
#include "ui/squad/squadpaint.h"
#include "ui/theme/cursors.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/panelpainter.h"
#include "ui/widgets/typechip.h"

#include <QFontMetricsF>
#include <QHelpEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmapCache>
#include <QToolTip>

namespace {
using namespace com::yamada::studio;

constexpr int kRing = 3;    // 선택(노랑) · 경고(빨강) 테 자리
constexpr int kHeader = 34; // 머리 띠
constexpr int kPadding = 12;
constexpr int kMoveHeight = 28;
constexpr int kRowGap = 6;

const tok::TypeColor *firstType(const PokemonDetail &detail)
{
    return detail.types.isEmpty() ? nullptr : typechip::find(detail.types.first());
}

QPixmap itemPixmap(SpriteCache *icons, const ItemRow &item, int generation)
{
    const QString key = ItemRowDelegate::iconKey(item.identifier, item.machineType, generation);
    const QString file = icons->path(key);
    if (file.isEmpty()) {
        icons->request(key); // 받으면 ready → 카드를 다시 그린다
        return {};
    }
    QPixmap pixmap;
    const QString cacheKey = QStringLiteral("pokesix.item.") + key; // ItemRowDelegate와 같은 key
    if (!QPixmapCache::find(cacheKey, &pixmap) && pixmap.load(file))
        QPixmapCache::insert(cacheKey, pixmap);
    return pixmap;
}
} // namespace

namespace com::yamada::studio {
SlotCard::SlotCard(int slot, SquadSession *session, SpriteCache *pokemonIcons,
                   SpriteCache *itemIcons, QWidget *parent)
    : QWidget(parent)
    , m_slot(slot)
    , m_session(session)
    , m_pokemonIcons(pokemonIcons)
    , m_itemIcons(itemIcons)
{
    QWidget::setFixedHeight(kHeight);
    QWidget::setMinimumWidth(kMinimumWidth);
    QWidget::setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QWidget::setMouseTracking(true);
    m_memo = new QLineEdit(this);
    m_memo->setProperty("role", QStringLiteral("memo")); // app.qss: 메모 칸 모양
    m_memo->setMaxLength(SquadMember::kMemoLength);
    m_memo->setPlaceholderText(tr("역할 메모 (예: 선봉 · 고속 스위퍼)"));
    // 한 글자씩 세션에 넘긴다 — 파일 저장은 SquadStore가 입력이 멈춘 뒤 한 번 한다
    connect(m_memo, &QLineEdit::textEdited, this,
            [this](const QString &text) { m_session->setMemo(m_slot, text); });
    connect(m_pokemonIcons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
    connect(m_itemIcons, &SpriteCache::ready, this, qOverload<>(&QWidget::update));
}

bool SlotCard::isEmptySlot() const
{
    return !m_session->detail(m_slot).isValid();
}

void SlotCard::refresh(Language language)
{
    m_language = language;
    const bool empty = isEmptySlot();
    m_memo->setVisible(!empty);
    if (!m_memo->hasFocus()) // 치는 중이면 덮어쓰지 않는다
        m_memo->setText(m_session->member(m_slot).memo);
    QWidget::update();
}

void SlotCard::setSelected(bool selected)
{
    if (m_selected == selected)
        return;
    m_selected = selected;
    QWidget::update();
}

void SlotCard::setAlert(bool alert)
{
    if (m_alert == alert)
        return;
    m_alert = alert;
    QWidget::update();
}

void SlotCard::setSuggestion(const QString &text)
{
    if (m_suggestion == text)
        return;
    m_suggestion = text;
    QWidget::update();
}

SlotCard::Geometry SlotCard::geometry() const
{
    Geometry g;
    g.card = rect().adjusted(kRing, kRing, -kRing, -kRing);
    g.header = QRect(g.card.left() + 2, g.card.top() + 2, g.card.width() - 4, kHeader);
    g.menu = QRect(g.header.right() - 34, g.header.top(), 34, kHeader);
    const int left = g.card.left() + kPadding;
    const int inner = g.card.width() - 2 * kPadding - 3; // 3 = 그림자
    int y = g.header.bottom() + 10;
    g.types = QRect(left, y, inner, int(typechip::kHeight));
    y += 28;
    g.memo = QRect(left, y, inner, 26);
    y += 26 + 8;
    // 특성 · 성격 · 물건: 그 세대에 있는 것만 같은 폭으로 나눈다
    const GenerationFeatures features = featuresOf(m_session->generation());
    const bool shown[3] = {features.abilities, features.natures, features.heldItems};
    int count = 0;
    for (const bool s : shown)
        count += s ? 1 : 0;
    const int traitWidth = count > 0 ? (inner - (count - 1) * kRowGap) / count : 0;
    int x = left;
    for (int i = 0; i < 3; ++i) {
        if (!shown[i])
            continue;
        g.traits[i] = QRect(x, y, traitWidth, 24);
        x += traitWidth + kRowGap;
    }
    y += 24 + 8;
    const int moveWidth = (inner - kRowGap) / 2;
    for (int i = 0; i < 4; ++i)
        g.moves[i] = QRect(left + (i % 2) * (moveWidth + kRowGap),
                           y + (i / 2) * (kMoveHeight + kRowGap), moveWidth, kMoveHeight);
    y += 2 * kMoveHeight + kRowGap + 10;
    g.weak = QRect(left, y, inner, 20);
    // 빈 자리: 가운데 버튼
    g.add = QRect(g.card.center().x() - 80, g.card.top() + 104, 160, 38);
    return g;
}

SlotCard::Hit SlotCard::hitAt(const QPoint &pos) const
{
    const Geometry g = geometry();
    if (isEmptySlot())
        return g.card.contains(pos) ? Hit::Add : Hit::None;
    if (g.menu.contains(pos))
        return Hit::Menu;
    if (g.header.contains(pos))
        return Hit::Header;
    const Hit traits[3] = {Hit::Ability, Hit::Nature, Hit::Item};
    for (int i = 0; i < 3; ++i)
        if (g.traits[i].contains(pos))
            return traits[i];
    const Hit moves[4] = {Hit::Move0, Hit::Move1, Hit::Move2, Hit::Move3};
    for (int i = 0; i < 4; ++i)
        if (g.moves[i].contains(pos))
            return moves[i];
    return Hit::None;
}

void SlotCard::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    m_memo->setGeometry(geometry().memo);
}

void SlotCard::mouseMoveEvent(QMouseEvent *event)
{
    const Hit hit = hitAt(event->position().toPoint());
    if (hit != m_hot) {
        m_hot = hit;
        if (hit == Hit::None)
            QWidget::unsetCursor();
        else
            QWidget::setCursor(cursors::pointer());
        QWidget::update();
    }
}

void SlotCard::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_hot = Hit::None;
    QWidget::unsetCursor();
    QWidget::update();
}

void SlotCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }
    const Geometry g = geometry();
    const QPoint pos = event->position().toPoint();
    auto below = [this](const QRect &rect) {
        return mapToGlobal(rect.bottomLeft() + QPoint(0, 4));
    };
    switch (hitAt(pos)) {
    case Hit::Add:
        emit addRequested(m_slot);
        break;
    case Hit::Header:
        emit selectRequested(m_slot);
        break;
    case Hit::Menu:
        emit menuRequested(m_slot, below(g.menu));
        break;
    case Hit::Ability:
        emit abilityRequested(m_slot, below(g.traits[0]));
        break;
    case Hit::Nature:
        emit natureRequested(m_slot, below(g.traits[1]));
        break;
    case Hit::Item:
        emit itemRequested(m_slot, below(g.traits[2]));
        break;
    case Hit::Move0:
    case Hit::Move1:
    case Hit::Move2:
    case Hit::Move3:
        emit moveRequested(m_slot, int(hitAt(pos)) - int(Hit::Move0));
        break;
    case Hit::None:
        QWidget::mousePressEvent(event);
        break;
    }
}

bool SlotCard::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip && !isEmptySlot()) {
        const QPoint pos = static_cast<QHelpEvent *>(event)->pos();
        const Geometry g = geometry();
        for (int i = 0; i < 4; ++i) {
            const auto &slotMove = m_session->slotMoves(m_slot)[std::size_t(i)];
            if (!g.moves[i].contains(pos) || !slotMove)
                continue;
            const MoveEntry &m = slotMove->move;
            const QString text
                    = slotMove->learnable
                              ? tr("위력 %1 · 명중 %2 · PP %3")
                                        .arg(m.power > 0 ? QString::number(m.power)
                                                         : QStringLiteral("—"),
                                             m.accuracy > 0 ? QString::number(m.accuracy)
                                                            : QStringLiteral("—"))
                                        .arg(m.pp)
                              : tr("이 게임에서는 배울 수 없는 기술이에요. 다시 골라 주세요.");
            QToolTip::showText(static_cast<QHelpEvent *>(event)->globalPos(), text, this);
            return true;
        }
        // 특성 · 성격 · 물건: 잘린 이름 전체(특성은 효과까지)
        QString trait;
        if (g.traits[0].contains(pos)) {
            if (const AbilityEntry *a = m_session->ability(m_slot))
                trait = a->name.text(m_language) + QStringLiteral("\n")
                        + a->effect.text(m_language);
        } else if (g.traits[1].contains(pos)) {
            if (const Nature *n = m_session->nature(m_slot))
                trait = n->name.text(m_language);
        } else if (g.traits[2].contains(pos)) {
            if (const ItemRow *i = m_session->item(m_slot))
                trait = i->name.text(m_language) + QStringLiteral("\n")
                        + i->effect.text(m_language);
        }
        if (!trait.isEmpty()) {
            QToolTip::showText(static_cast<QHelpEvent *>(event)->globalPos(), trait.trimmed(),
                               this);
            return true;
        }
        QToolTip::hideText();
        event->ignore();
        return true;
    }
    return QWidget::event(event);
}

void SlotCard::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const Geometry g = geometry();
    if (m_selected || m_alert) { // 바깥 테: 경고(빨강)가 선택(노랑)보다 앞선다
        painter.setPen(QPen(QColor(m_alert ? tok::kRed : tok::kYellow), kRing));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(QRectF(rect()).adjusted(1.5, 1.5, -1.5, -1.5), 10, 10);
    }
    if (isEmptySlot())
        paintEmpty(painter, g);
    else
        paintFilled(painter, g);
}

void SlotCard::paintFilled(QPainter &painter, const Geometry &g)
{
    const PokemonDetail &detail = m_session->detail(m_slot);
    const tok::TypeColor *type = firstType(detail);
    const QRgb headerText = type ? type->text : tok::kWhite;
    paintPanel(painter, g.card,
               PanelStyle {.outline = 2,
                           .radius = 8,
                           .shadow = 3,
                           .fill = tok::kWhite,
                           .ink = tok::kInk,
                           .header = kHeader,
                           .headerColor = type ? type->fill : tok::kInk});

    // 머리: [▶] 01 [아이콘] 이름 ······ ⋯
    int x = g.header.left() + 10;
    painter.setPen(QColor(headerText));
    if (m_selected) {
        painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::ExtraBold));
        painter.drawText(QRect(x, g.header.top(), 12, kHeader), Qt::AlignCenter,
                         QStringLiteral("▶"));
        x += 14;
    }
    painter.setFont(theme::font(theme::kFamilyPixel, 11));
    painter.drawText(QRect(x, g.header.top(), 22, kHeader), Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("%1").arg(m_slot + 1, 2, 10, QLatin1Char('0')));
    x += 24;
    const QRect iconBox(x, g.header.top() + 4, 34, kHeader - 8);
    painter.setPen(QPen(QColor(tok::kInk), 1.5));
    painter.setBrush(QColor(tok::kWhite));
    painter.drawRoundedRect(QRectF(iconBox).adjusted(0.75, 0.75, -0.75, -0.75), 4, 4);
    DexRowDelegate::paintPokemonIcon(&painter, QRectF(iconBox), detail.pokemonId, m_pokemonIcons);
    x = iconBox.right() + 8;
    painter.setFont(theme::font(theme::kFamilyTitle, 19));
    painter.setPen(QColor(headerText));
    const int nameWidth = g.menu.left() - x - 4;
    painter.drawText(QRect(x, g.header.top(), nameWidth, kHeader), Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetricsF(painter.font())
                             .elidedText(detail.name.text(m_language), Qt::ElideRight, nameWidth));
    if (m_hot == Hit::Menu) {
        painter.setPen(Qt::NoPen);
        QColor glow(tok::kWhite);
        glow.setAlpha(60);
        painter.setBrush(glow);
        painter.drawRoundedRect(QRectF(g.menu).adjusted(4, 6, -4, -6), 4, 4);
    }
    painter.setPen(QColor(headerText));
    painter.setFont(theme::font(theme::kFamilyBody, 16, QFont::ExtraBold));
    painter.drawText(g.menu, Qt::AlignCenter, QStringLiteral("⋯"));

    // 타입 칩
    qreal chipX = g.types.left();
    for (const QString &identifier : detail.types)
        if (const tok::TypeColor *t = typechip::find(identifier))
            chipX += typechip::paint(painter, QPointF(chipX, g.types.top()), *t, m_language)
                     + typechip::kGap;

    // 특성 · 성격 · 물건 (그 세대에 있는 것만)
    const GenerationFeatures features = featuresOf(m_session->generation());
    if (!features.abilities && !features.natures && !features.heldItems) {
        painter.setFont(theme::font(theme::kFamilyBody, 12));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(g.types.left(), g.memo.bottom() + 8, g.types.width(), 24),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         tr("1세대에는 특성 · 성격 · 지닌 물건이 없어요"));
    }
    if (features.abilities) {
        const AbilityEntry *ability = m_session->ability(m_slot);
        paintTrait(painter, g.traits[0], tr("특성"),
                   ability ? ability->name.text(m_language) : QString(), m_hot == Hit::Ability);
    }
    if (features.natures) {
        const Nature *nature = m_session->nature(m_slot);
        paintTrait(painter, g.traits[1], tr("성격"),
                   nature ? nature->name.text(m_language) : QString(), m_hot == Hit::Nature);
    }
    if (features.heldItems) {
        const ItemRow *item = m_session->item(m_slot);
        paintTrait(painter, g.traits[2], tr("물건"), item ? item->name.text(m_language) : QString(),
                   m_hot == Hit::Item, item ? item->identifier : QString());
    }

    // 기술 4칸
    for (int i = 0; i < 4; ++i)
        paintMove(painter, g.moves[i], i, int(m_hot) - int(Hit::Move0) == i);

    // 약점: ×4 먼저, 그다음 타입 순
    painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::Bold));
    painter.setPen(QColor(tok::kText3));
    const QString label = tr("약점");
    painter.drawText(g.weak, Qt::AlignLeft | Qt::AlignVCenter, label);
    qreal wx = g.weak.left() + QFontMetricsF(painter.font()).horizontalAdvance(label) + 8;
    const auto &received = m_session->analysis().received[std::size_t(m_slot)];
    const QStringList &existing = m_session->chart().types;
    bool any = false;
    for (const bool quad : {true, false}) {
        for (const tok::TypeColor &t : tok::kTypes) {
            if (!existing.contains(QLatin1String(t.key)))
                continue;
            // tok::kTypes는 게임 표기 순서, received는 Type 순(PokéAPI id − 1) → key로 잇는다
            const std::optional<Type> core = typeFromKey(t.key);
            const double multiplier = core ? received[std::size_t(*core)] : 1.0;
            if (multiplier <= 1.0 || (multiplier >= 4.0) != quad)
                continue;
            const QString text = squadpaint::typeAbbr(t, m_language)
                                 + (quad ? QStringLiteral("×4") : QString());
            const qreal width = std::max<qreal>(
                    20, QFontMetricsF(theme::font(theme::kFamilyBody, 11, QFont::ExtraBold))
                                        .horizontalAdvance(text)
                                + 8);
            if (wx + width > g.weak.right())
                break;
            squadpaint::paintTypeBox(painter, QRectF(wx, g.weak.top(), width, 20), t, text);
            wx += width + 3;
            any = true;
        }
    }
    if (!any) {
        painter.setFont(theme::font(theme::kFamilyBody, 11, QFont::Bold));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRectF(wx, g.weak.top(), g.weak.right() - wx, 20),
                         Qt::AlignLeft | Qt::AlignVCenter, tr("없음"));
    }
}

void SlotCard::paintTrait(QPainter &painter, const QRect &rect, const QString &label,
                          const QString &value, bool hot, const QString &itemIcon)
{
    painter.setPen(QPen(QColor(hot ? tok::kInk : tok::kLineStrong), 1.5));
    painter.setBrush(QColor(hot ? tok::kYellowTint : tok::kWhite));
    painter.drawRoundedRect(QRectF(rect).adjusted(0.75, 0.75, -0.75, -0.75), 5, 5);
    int x = rect.left() + 7;
    if (!itemIcon.isEmpty()) {
        const ItemRow *item = m_session->item(m_slot);
        if (item) {
            const QPixmap pixmap = itemPixmap(m_itemIcons, *item, m_session->generation());
            if (!pixmap.isNull()) {
                painter.drawPixmap(QRect(x - 3, rect.top(), 24, 24), pixmap);
                x += 20;
            }
        }
    } else {
        const QFont labelFont = theme::font(theme::kFamilyBody, 10, QFont::Bold);
        painter.setFont(labelFont);
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(x, rect.top(), rect.width(), rect.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, label);
        x += int(QFontMetricsF(labelFont).horizontalAdvance(label)) + 5;
    }
    const QFont valueFont = theme::font(theme::kFamilyBody, 11, QFont::ExtraBold);
    painter.setFont(valueFont);
    painter.setPen(QColor(value.isEmpty() ? tok::kTextDisabled : tok::kText1));
    const int width = rect.right() - 14 - x;
    painter.drawText(QRect(x, rect.top(), width, rect.height()), Qt::AlignLeft | Qt::AlignVCenter,
                     QFontMetricsF(valueFont).elidedText(
                             value.isEmpty() ? (itemIcon.isEmpty() && label == tr("물건")
                                                        ? tr("없음")
                                                        : QStringLiteral("—"))
                                             : value,
                             Qt::ElideRight, width));
    painter.setPen(QColor(tok::kText3));
    painter.drawText(QRect(rect.right() - 14, rect.top(), 12, rect.height()), Qt::AlignCenter,
                     QStringLiteral("▾"));
}

void SlotCard::paintMove(QPainter &painter, const QRect &rect, int index, bool hot)
{
    const auto &slotMove = m_session->slotMoves(m_slot)[std::size_t(index)];
    const QRectF box = QRectF(rect).adjusted(0.75, 0.75, -0.75, -0.75);
    if (!slotMove) { // 빈 칸: 점선 "+ 기술 추가"
        QPen pen(QColor(hot ? tok::kBlue : tok::kLineStrong), 1.5, Qt::DashLine);
        painter.setPen(pen);
        painter.setBrush(QColor(hot ? tok::kBlueTint : tok::kWhite));
        painter.drawRoundedRect(box, 4, 4);
        painter.setFont(theme::font(theme::kFamilyBody, 12, QFont::Bold));
        painter.setPen(QColor(hot ? tok::kBlueDeep : tok::kText3));
        painter.drawText(rect, Qt::AlignCenter, tr("+ 기술 추가"));
        return;
    }
    const MoveEntry &move = slotMove->move;
    QPen pen(QColor(slotMove->learnable ? tok::kInk : tok::kRed), 1.5,
             slotMove->learnable ? Qt::SolidLine : Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(QColor(hot ? tok::kYellowTint : tok::kWhite));
    painter.drawRoundedRect(box, 4, 4);
    // 타입 색 네모 · 이름 · 분류 배지
    if (const tok::TypeColor *type = typechip::find(move.type)) {
        painter.setPen(QPen(QColor(tok::kInk), 1));
        painter.setBrush(QColor(type->fill));
        painter.drawRect(QRectF(rect.left() + 8, rect.center().y() - 4.5, 9, 9));
    }
    const QRectF badge(rect.right() - 26, rect.top() + 5, 20, rect.height() - 10);
    squadpaint::paintDamageClass(painter, badge, move.damageClass);
    const QFont font = theme::font(theme::kFamilyBody, 12, QFont::ExtraBold);
    painter.setFont(font);
    painter.setPen(QColor(slotMove->learnable ? tok::kText1 : tok::kRedText));
    const int left = rect.left() + 24;
    const int width = int(badge.left()) - 4 - left;
    painter.drawText(
            QRect(left, rect.top(), width, rect.height()), Qt::AlignLeft | Qt::AlignVCenter,
            QFontMetricsF(font).elidedText(move.name.text(m_language), Qt::ElideRight, width));
}

void SlotCard::paintEmpty(QPainter &painter, const Geometry &g)
{
    // 점선 테 · 가운데 + 원 · "06 빈 슬롯" · [+ 개체 추가] · 제안 문구
    const bool hot = m_hot == Hit::Add;
    QPen pen(QColor(hot ? tok::kInk : tok::kLineStrong), 2, Qt::DashLine);
    painter.setPen(pen);
    painter.setBrush(QColor(hot ? tok::kPaperAlt : tok::kPaper));
    painter.drawRoundedRect(QRectF(g.card).adjusted(1, 1, -4, -4), 8, 8);

    const QPointF center(g.card.center().x() - 1.5, g.card.top() + 40);
    painter.setPen(QPen(QColor(tok::kLineStrong), 2));
    painter.setBrush(QColor(tok::kWhite));
    painter.drawEllipse(center, 15, 15);
    painter.setFont(theme::font(theme::kFamilyBody, 18, QFont::Bold));
    painter.setPen(QColor(tok::kText3));
    painter.drawText(QRectF(center.x() - 15, center.y() - 15, 30, 30), Qt::AlignCenter,
                     QStringLiteral("+"));

    painter.setFont(theme::font(theme::kFamilyTitle, 18));
    painter.setPen(QColor(tok::kText2));
    painter.drawText(QRect(g.card.left(), g.card.top() + 62, g.card.width() - 3, 30),
                     Qt::AlignCenter, tr("%1 빈 슬롯").arg(m_slot + 1, 2, 10, QLatin1Char('0')));

    // [+ 개체 추가]: 빨강 채움 · 먹선 · 아래 그림자
    const QRectF button(g.add);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(tok::kInk));
    painter.drawRoundedRect(button.translated(0, 3), 6, 6);
    painter.setPen(QPen(QColor(tok::kInk), 2));
    painter.setBrush(QColor(hot ? tok::kRedDeep : tok::kRed));
    painter.drawRoundedRect(button.adjusted(1, 1, -1, -1), 6, 6);
    painter.setFont(theme::font(theme::kFamilyTitle, 18));
    painter.setPen(QColor(tok::kWhite));
    painter.drawText(button, Qt::AlignCenter, tr("+ 개체 추가"));

    if (!m_suggestion.isEmpty()) {
        painter.setFont(theme::font(theme::kFamilyBody, 12));
        painter.setPen(QColor(tok::kText3));
        painter.drawText(QRect(g.card.left() + 18, g.add.bottom() + 14, g.card.width() - 39, 60),
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, m_suggestion);
    }
}
} // namespace com::yamada::studio
