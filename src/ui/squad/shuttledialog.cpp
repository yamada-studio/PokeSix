#include "ui/squad/shuttledialog.h"

#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "data/state/squadsession.h"
#include "ui/squad/squadpaint.h"
#include "ui/theme/theme.h"
#include "ui/theme/tokens.h"
#include "ui/widgets/typechip.h"

#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace com::yamada::studio {
namespace {
// 셔틀 포켓몬의 박스 아이콘(없으면 받기 시작하고, 도착하면 ready → rebuild가 다시 그린다)
QPixmap shuttleIcon(SpriteCache *icons, int pokemonId, int height)
{
    const QString key = QString::number(pokemonId);
    const QString path = icons->path(key);
    if (path.isEmpty()) {
        icons->request(key);
        return {};
    }
    return squadpaint::trimmedIcon(path, height);
}

// 타입 칩 몇 개를 가로로 — typechip은 그리기 함수라(ADR 0007) 작은 위젯으로 감싼다
class Chips : public QWidget
{
public:
    Chips(const QStringList &types, Language language, QWidget *parent = nullptr)
        : QWidget(parent)
        , m_types(types)
        , m_language(language)
    {
        qreal width = 0;
        for (const QString &key : types)
            if (const tok::TypeColor *type = typechip::find(key))
                width += typechip::width(*type, language) + typechip::kGap;
        setFixedSize(int(width > 0 ? width - typechip::kGap : 0), int(typechip::kHeight));
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        qreal x = 0;
        for (const QString &key : m_types)
            if (const tok::TypeColor *type = typechip::find(key))
                x += typechip::paint(painter, QPointF(x, 0), *type, m_language) + typechip::kGap;
    }

private:
    QStringList m_types;
    Language m_language;
};

// 커버 상황 글자: 상태별 색(app.qss의 shuttleStatus[state=…])
QLabel *statusLabel(const QString &text, const char *state)
{
    QLabel *label = new QLabel(text);
    label->setObjectName(QStringLiteral("shuttleStatus"));
    label->setProperty("state", QLatin1String(state));
    return label;
}
} // namespace

ShuttleDialog::ShuttleDialog(Repository *repository, AppState *state, SquadSession *session,
                             SpriteCache *icons, QWidget *parent)
    : QDialog(parent)
    , m_repository(repository)
    , m_state(state)
    , m_session(session)
    , m_icons(icons)
{
    setWindowTitle(tr("비전셔틀"));
    setMinimumWidth(640);
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(20, 16, 20, 16);
    m_layout->setSpacing(10);
    // 체크 토글 → 세션 변경 → rebuild가 보낸 쪽 체크박스를 지운다 — 시그널 처리 중에 지우지
    // 않도록 큐로 미룬다
    connect(m_session, &SquadSession::changed, this, &ShuttleDialog::rebuild, Qt::QueuedConnection);
    connect(m_icons, &SpriteCache::ready, this, &ShuttleDialog::rebuild, Qt::QueuedConnection);
    rebuild();
}

void ShuttleDialog::rebuild()
{
    delete m_body;
    m_body = new QWidget;
    m_layout->addWidget(m_body);
    QVBoxLayout *layout = new QVBoxLayout(m_body);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    const Language language = m_state->language();
    const SquadMember &shuttle = m_session->shuttle();
    const PokemonDetail &detail = m_session->shuttleDetail();
    const QString shuttleName = detail.name.text(language);

    // ── 머리: 셔틀 포켓몬(큰 아이콘 + 이름 + 타입) · 비우기 · 기술 수 ──────
    QHBoxLayout *head = new QHBoxLayout;
    head->setSpacing(12);
    QPushButton *pick = new QPushButton(detail.isValid() ? shuttleName : tr("+ 포켓몬 고르기"));
    pick->setObjectName(QStringLiteral("shuttlePickButton"));
    pick->setCursor(Qt::PointingHandCursor);
    if (detail.isValid()) {
        const QPixmap icon = shuttleIcon(m_icons, detail.pokemonId, 48);
        if (!icon.isNull()) {
            pick->setIcon(icon);
            pick->setIconSize(icon.size());
        }
        pick->setToolTip(tr("눌러서 다른 포켓몬으로 바꿔요"));
    }
    connect(pick, &QPushButton::clicked, this, &ShuttleDialog::pickRequested);
    head->addWidget(pick);
    if (detail.isValid()) {
        head->addWidget(new Chips(detail.types, language));
        QPushButton *clear = new QPushButton(tr("비우기"));
        clear->setObjectName(QStringLiteral("squadLinkButton"));
        clear->setCursor(Qt::PointingHandCursor);
        clear->setFlat(true);
        connect(clear, &QPushButton::clicked, m_session, &SquadSession::clearShuttle);
        head->addWidget(clear);
    }
    head->addStretch();
    int used = 0;
    for (const int move : shuttle.moves)
        used += move > 0 ? 1 : 0;
    QLabel *count = new QLabel(tr("기술 %1 / 4").arg(used));
    count->setObjectName(QStringLiteral("squadCount"));
    head->addWidget(count);
    layout->addLayout(head);

    // ── 비전머신 표: 이 게임의 HM마다 누가 드는가 ──────────────────────────
    const QList<MoveEntry> machines
            = m_repository->hiddenMachineMoves(m_session->versionGroup(), m_session->generation());
    QFrame *table = new QFrame;
    table->setObjectName(QStringLiteral("shuttleTable"));
    QGridLayout *grid = new QGridLayout(table);
    grid->setContentsMargins(16, 12, 16, 12);
    grid->setHorizontalSpacing(14);
    grid->setVerticalSpacing(8);
    grid->setColumnStretch(4, 1);
    if (machines.isEmpty()) {
        QLabel *none = new QLabel(tr("이 게임에는 비전머신이 없어요"));
        none->setObjectName(QStringLiteral("squadEmpty"));
        none->setAlignment(Qt::AlignCenter);
        none->setMinimumHeight(80);
        grid->addWidget(none, 0, 0, 1, 5);
    } else {
        QLabel *moveHead = new QLabel(tr("비전머신"));
        moveHead->setObjectName(QStringLiteral("dexSectionLabel"));
        grid->addWidget(moveHead, 0, 0, 1, 3);
        QLabel *coverHead = new QLabel(tr("누가 드나"));
        coverHead->setObjectName(QStringLiteral("dexSectionLabel"));
        grid->addWidget(coverHead, 0, 3, 1, 2);
    }
    // 셔틀이 배울 수 있는 비전머신(이 게임 기준)
    QSet<int> learnable;
    for (const MoveEntry &move : detail.machineMoves)
        if (move.hiddenMachine)
            learnable.insert(move.moveId);

    int row = 1;
    for (const MoveEntry &machine : machines) {
        QLabel *number = new QLabel(
                QStringLiteral("HM%1").arg(machine.machineNumber, 2, 10, QLatin1Char('0')));
        number->setObjectName(QStringLiteral("squadCount"));
        grid->addWidget(number, row, 0);
        QLabel *name = new QLabel(machine.name.text(language));
        name->setFont(theme::font(theme::kFamilyBody, 14, QFont::ExtraBold));
        grid->addWidget(name, row, 1);
        grid->addWidget(new Chips({machine.type}, language), row, 2);

        // 본편 멤버 중 이 비전기술을 기술 칸에 둔 포켓몬
        QStringList carriers;
        for (int slot = 0; slot < 6; ++slot) {
            if (!m_session->detail(slot).isValid())
                continue;
            for (const auto &slotMove : m_session->slotMoves(slot))
                if (slotMove && slotMove->learnable && slotMove->move.moveId == machine.moveId)
                    carriers.append(m_session->detail(slot).name.text(language));
        }
        const QString main = carriers.join(QStringLiteral(" · "));

        // 셔틀이 못 배우는 비전머신: 체크박스 대신 경고 글자
        if (detail.isValid() && !learnable.contains(machine.moveId)) {
            grid->addWidget(statusLabel(tr("배울 수 없어요"), "warn"), row, 3);
            grid->addWidget(carriers.isEmpty() ? statusLabel(tr("아무도 안 들어요"), "none")
                                               : statusLabel(tr("본편: %1").arg(main), "plain"),
                            row, 4);
            ++row;
            continue;
        }

        const bool carried = std::find(shuttle.moves.begin(), shuttle.moves.end(), machine.moveId)
                             != shuttle.moves.end();
        QCheckBox *check = new QCheckBox(tr("셔틀"));
        check->setChecked(carried);
        if (!detail.isValid()) {
            check->setEnabled(false);
            check->setToolTip(tr("먼저 셔틀 포켓몬을 골라요"));
        } else if (!carried && used >= 4) {
            check->setEnabled(false);
            check->setToolTip(tr("기술 칸 4개가 다 찼어요"));
        }
        const int moveId = machine.moveId;
        connect(check, &QCheckBox::toggled, this, [this, moveId](bool on) {
            const auto &moves = m_session->shuttle().moves;
            if (on) { // 첫 빈 칸에
                for (std::size_t i = 0; i < moves.size(); ++i)
                    if (moves[i] <= 0)
                        return m_session->setShuttleMove(int(i), moveId);
            } else {
                for (std::size_t i = 0; i < moves.size(); ++i)
                    if (moves[i] == moveId)
                        return m_session->setShuttleMove(int(i), 0);
            }
        });
        grid->addWidget(check, row, 3);

        if (carried && !carriers.isEmpty())
            grid->addWidget(
                    statusLabel(tr("셔틀이 들어요 — %1의 기술 칸을 비워도 돼요").arg(main), "ok"),
                    row, 4);
        else if (carried)
            grid->addWidget(statusLabel(tr("셔틀이 들어요"), "ok"), row, 4);
        else if (!carriers.isEmpty())
            grid->addWidget(statusLabel(tr("본편: %1").arg(main), "plain"), row, 4);
        else
            grid->addWidget(statusLabel(tr("아무도 안 들어요"), "none"), row, 4);
        ++row;
    }
    layout->addWidget(table);

    QLabel *note = new QLabel(tr("7번째 멤버예요 — 본편 6자리 분석(히트맵 · 문제)에는 들어가지 "
                                 "않아요"));
    note->setObjectName(QStringLiteral("squadNote"));
    layout->addWidget(note);
}
} // namespace com::yamada::studio
