#include "ui/squad/shuttledialog.h"

#include "data/repository/repository.h"
#include "data/sprites/spritecache.h"
#include "data/state/appstate.h"
#include "data/state/squadsession.h"
#include "ui/theme/theme.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

namespace com::yamada::studio {
namespace {
// 셔틀 포켓몬의 박스 아이콘(없으면 받기 시작하고, 도착하면 ready → rebuild가 다시 그린다)
QPixmap shuttleIcon(SpriteCache *icons, int pokemonId)
{
    const QString key = QString::number(pokemonId);
    const QString path = icons->path(key);
    if (path.isEmpty()) {
        icons->request(key);
        return {};
    }
    return QPixmap(path).scaled(40, 34, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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

    // ── 머리: 셔틀 포켓몬 + 비우기 + 기술 수 ────────────────────────────────
    QHBoxLayout *head = new QHBoxLayout;
    head->setSpacing(10);
    QPushButton *pick = new QPushButton(detail.isValid() ? detail.name.text(language)
                                                         : tr("+ 포켓몬 고르기"));
    pick->setObjectName(QStringLiteral("shuttlePickButton"));
    pick->setCursor(Qt::PointingHandCursor);
    if (detail.isValid()) {
        const QPixmap icon = shuttleIcon(m_icons, detail.pokemonId);
        if (!icon.isNull()) {
            pick->setIcon(icon);
            pick->setIconSize(icon.size());
        }
        pick->setToolTip(tr("눌러서 다른 포켓몬으로 바꿔요"));
    }
    connect(pick, &QPushButton::clicked, this, &ShuttleDialog::pickRequested);
    head->addWidget(pick);
    if (detail.isValid()) {
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
    if (machines.isEmpty()) {
        QLabel *none = new QLabel(tr("이 게임에는 비전머신이 없어요"));
        none->setObjectName(QStringLiteral("squadEmpty"));
        none->setAlignment(Qt::AlignCenter);
        none->setMinimumHeight(80);
        layout->addWidget(none);
    }
    // 셔틀이 배울 수 있는 비전머신(이 게임 기준)
    QSet<int> learnable;
    for (const MoveEntry &move : detail.machineMoves)
        if (move.hiddenMachine)
            learnable.insert(move.moveId);

    QGridLayout *grid = new QGridLayout;
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(6);
    grid->setColumnStretch(3, 1);
    int row = 0;
    for (const MoveEntry &machine : machines) {
        QLabel *number = new QLabel(
                QStringLiteral("HM%1").arg(machine.machineNumber, 2, 10, QLatin1Char('0')));
        number->setObjectName(QStringLiteral("squadCount"));
        grid->addWidget(number, row, 0);
        QLabel *name = new QLabel(machine.name.text(language));
        name->setFont(theme::font(theme::kFamilyBody, 14, QFont::ExtraBold));
        grid->addWidget(name, row, 1);

        const bool carried = std::find(shuttle.moves.begin(), shuttle.moves.end(), machine.moveId)
                             != shuttle.moves.end();
        QCheckBox *check = new QCheckBox(tr("셔틀"));
        check->setChecked(carried);
        if (!detail.isValid()) {
            check->setEnabled(false);
            check->setToolTip(tr("먼저 셔틀 포켓몬을 골라요"));
        } else if (!learnable.contains(machine.moveId)) {
            check->setEnabled(false);
            check->setToolTip(tr("%1이(가) 배울 수 없어요").arg(detail.name.text(language)));
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
        grid->addWidget(check, row, 2);

        // 본편 멤버 중 이 비전기술을 기술 칸에 둔 포켓몬
        QStringList carriers;
        for (int slot = 0; slot < 6; ++slot) {
            if (!m_session->detail(slot).isValid())
                continue;
            for (const auto &slotMove : m_session->slotMoves(slot))
                if (slotMove && slotMove->learnable && slotMove->move.moveId == machine.moveId)
                    carriers.append(m_session->detail(slot).name.text(language));
        }
        QLabel *status = new QLabel;
        status->setObjectName(QStringLiteral("squadResources"));
        if (carried && !carriers.isEmpty())
            status->setText(tr("셔틀이 들어요 — %1의 기술 칸을 비워도 돼요")
                                    .arg(carriers.join(QStringLiteral(" · "))));
        else if (carried)
            status->setText(tr("셔틀이 들어요"));
        else if (!carriers.isEmpty())
            status->setText(tr("본편: %1").arg(carriers.join(QStringLiteral(" · "))));
        else
            status->setText(tr("아무도 안 들어요"));
        grid->addWidget(status, row, 3);
        ++row;
    }
    layout->addLayout(grid);

    QLabel *note = new QLabel(tr("7번째 멤버예요 — 본편 6자리 분석(히트맵 · 문제)에는 들어가지 "
                                 "않아요"));
    note->setObjectName(QStringLiteral("squadNote"));
    layout->addWidget(note);
}
} // namespace com::yamada::studio
