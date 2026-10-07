#include "data/state/squadsession.h"

#include "data/analysis/squadinput.h"
#include "data/logging/logging.h"
#include "data/state/appstate.h"
#include "data/store/squadstore.h"

#include <QMap>

namespace com::yamada::studio {
SquadSession::SquadSession(Repository *repository, SquadStore *store, AppState *state,
                           QObject *parent)
    : QObject(parent)
    , m_repository(repository)
    , m_store(store)
    , m_state(state)
{
    connect(m_state, &AppState::generationChanged, this, &SquadSession::reload);
    connect(m_state, &AppState::gameChanged, this, &SquadSession::reload);
    reload();
}

void SquadSession::reload()
{
    if (m_seeding)
        return; // 아래에서 앱 상태에 처음 버전을 써 넣는 중(gameChanged가 다시 부른다)
    m_generation = m_state->generation();
    m_chart = m_repository->typeChart(m_generation);
    m_games = m_repository->gamesForGeneration(m_generation);
    // 게임(버전): 앱이 고른 버전. 아직 안 골랐으면 스쿼드 저장소에 남은 마지막 게임(옛 파일은
    // 게임 묶음 identifier), 그것도 없으면 대표 게임
    QString wanted = m_state->game();
    if (wanted.isEmpty())
        wanted = m_store->currentGame(m_generation);
    m_version = m_repository->resolveVersion(m_generation, wanted);
    m_versionGroup = versionGroupOf(m_games, m_version);
    if (m_versionGroup.isEmpty())
        m_versionGroup = Repository::representativeVersionGroup(m_generation);
    if (m_state->game().isEmpty() && !m_version.isEmpty()) {
        // 앱이 아직 이 세대의 게임을 모른다 → 스쿼드가 이어받은 게임을 앱 전체의 게임으로(도감 ·
        // 아이템도 같은 버전으로 열리게)
        m_seeding = true;
        m_state->setGame(m_version);
        m_seeding = false;
    }
    // 스쿼드는 버전마다 따로. 버전으로 저장한 적이 없으면 게임 묶음 단위로 저장하던 때의 스쿼드를
    // 이어받는다(HG · SS 둘 다 같은 스쿼드에서 시작해 처음 고칠 때 갈라진다)
    m_squad = m_store->hasSquad(m_generation, m_version)
                      ? m_store->squad(m_generation, m_version)
                      : m_store->squad(m_generation, m_versionGroup);
    m_items.clear();
    m_itemsLoaded = false;
    m_committed = m_squad; // 되돌리기 기록은 스쿼드(세대 · 버전)마다 따로 — 바뀌면 비운다
    m_undoStack.clear();
    m_redoStack.clear();
    resolveAll();
    analyze();
    qCInfo(lcData) << "squad of" << m_version << "(generation" << m_generation << ") has"
                   << m_squad.filled() << "members";
    emit changed();
}

QString SquadSession::defaultName() const
{
    // 버전마다 스쿼드가 따로라 버전 이름으로("소울실버 스쿼드"). 이름을 모르면 세대로
    for (const GameInfo &game : m_games) {
        const qsizetype index = game.versions.indexOf(m_version);
        if (index >= 0 && index < game.versionNames.size()) {
            const QString name = game.versionNames.at(index).text(m_state->language());
            if (!name.isEmpty())
                return tr("%1 스쿼드").arg(name);
        }
    }
    return tr("%1세대 스쿼드").arg(m_generation);
}

const QList<Nature> &SquadSession::natures()
{
    if (m_natures.isEmpty())
        m_natures = m_repository->natures();
    return m_natures;
}

const QList<ItemRow> &SquadSession::items()
{
    if (!m_itemsLoaded) {
        m_items = m_repository->itemsForGeneration(m_generation);
        m_items.removeIf([this](const ItemRow &item) { return !item.existsIn(m_generation); });
        m_itemsLoaded = true;
    }
    return m_items;
}

void SquadSession::resolve(int slot)
{
    resolveMember(m_squad.members[std::size_t(slot)], m_details[std::size_t(slot)],
                  m_moves[std::size_t(slot)]);
}

void SquadSession::resolveAll()
{
    for (int slot = 0; slot < int(kSquadSize); ++slot)
        resolve(slot);
    resolveShuttle();
    for (const SquadMember &member : m_squad.members) { // nature() · item()이 찾을 목록
        if (member.natureId > 0)
            natures();
        if (member.itemId > 0)
            items();
    }
}

void SquadSession::resolveShuttle()
{
    resolveMember(m_squad.shuttle, m_shuttleDetail, m_shuttleMoves);
}

void SquadSession::resolveMember(const SquadMember &member, PokemonDetail &detail,
                                 std::array<std::optional<SlotMove>, 4> &moves)
{
    moves = {};
    detail = member.isEmpty()
                     ? PokemonDetail()
                     : m_repository->pokemonDetail(member.pokemonId, m_generation, versionGroup());
    if (detail.types.isEmpty()) { // 빈 자리 · 이 세대에 없는 포켓몬
        detail = PokemonDetail();
        return;
    }
    QHash<int, MoveEntry> learnable;
    for (const QList<MoveEntry> *list :
         {&detail.levelMoves, &detail.machineMoves, &detail.tutorMoves, &detail.eggMoves})
        for (const MoveEntry &move : *list)
            learnable.insert(move.moveId, move);
    QList<int> others; // 이 게임에서 배울 수 없는 기술(게임을 바꿨다) — 이름만이라도 보여 준다
    for (std::size_t i = 0; i < member.moves.size(); ++i) {
        const int id = member.moves[i];
        if (id <= 0)
            continue;
        if (learnable.contains(id))
            moves[i] = SlotMove {learnable.value(id), true};
        else
            others.append(id);
    }
    if (others.isEmpty())
        return;
    QHash<int, MoveEntry> found;
    for (const MoveEntry &move : m_repository->moves(others, m_generation))
        found.insert(move.moveId, move);
    for (std::size_t i = 0; i < member.moves.size(); ++i)
        if (found.contains(member.moves[i]))
            moves[i] = SlotMove {found.value(member.moves[i]), false};
}

void SquadSession::analyze()
{
    std::array<std::optional<ResolvedMember>, kSquadSize> members;
    for (std::size_t slot = 0; slot < kSquadSize; ++slot) {
        if (!m_details[slot].isValid())
            continue;
        ResolvedMember member;
        member.types = m_details[slot].types;
        for (const std::optional<SlotMove> &move : m_moves[slot])
            if (move && move->learnable)
                member.moves.append(move->move);
        members[slot] = std::move(member);
    }
    m_analysis = analyzeSquad(makeSquadInput(m_chart, members));
}

void SquadSession::commit()
{
    if (m_squad != m_committed) { // 편집 한 번 = 되돌리기 한 단계(무변화 커밋은 쌓지 않는다)
        constexpr qsizetype kHistoryLimit = 50;
        m_undoStack.append(m_committed);
        if (m_undoStack.size() > kHistoryLimit)
            m_undoStack.removeFirst();
        m_redoStack.clear();
        m_committed = m_squad;
    }
    m_store->setSquad(m_generation, m_version, m_squad);
    analyze();
    emit changed();
}

void SquadSession::undo()
{
    if (m_undoStack.isEmpty())
        return;
    m_redoStack.append(m_squad);
    m_squad = m_undoStack.takeLast();
    m_committed = m_squad; // commit이 이 되돌리기를 또 기록하지 않게
    resolveAll();
    commit();
}

void SquadSession::redo()
{
    if (m_redoStack.isEmpty())
        return;
    m_undoStack.append(m_squad);
    m_squad = m_redoStack.takeLast();
    m_committed = m_squad;
    resolveAll();
    commit();
}

void SquadSession::clearAll()
{
    Squad empty;
    empty.name = m_squad.name;
    if (m_squad == empty)
        return;
    m_squad = empty;
    resolveAll();
    commit(); // 기록에 쌓인다 → 실수로 비워도 undo로 돌아온다
}

QList<SquadSession::LearnableMove> SquadSession::learnableMoves(int slot) const
{
    const PokemonDetail &detail = m_details[std::size_t(slot)];
    // 순서: 레벨업(레벨 순) → 기술머신(번호 순) → 가르침 → 알. 같은 기술은 한 줄로 합친다
    QList<LearnableMove> rows;
    QHash<int, qsizetype> rowOf;
    auto row = [&](const MoveEntry &move) -> LearnableMove & {
        const auto it = rowOf.constFind(move.moveId);
        if (it != rowOf.constEnd())
            return rows[*it];
        rowOf.insert(move.moveId, rows.size());
        LearnableMove added;
        added.move = move;
        rows.append(added);
        return rows.last();
    };
    for (const MoveEntry &move : detail.levelMoves) {
        LearnableMove &r = row(move);
        if (r.level < 0)
            r.level = move.level; // 여러 레벨이면 가장 이른 것
    }
    for (const MoveEntry &move : detail.machineMoves)
        row(move).machine
                = QStringLiteral("%1%2")
                          .arg(move.hiddenMachine ? QStringLiteral("HM") : QStringLiteral("TM"))
                          .arg(move.machineNumber, 2, 10, QLatin1Char('0'));
    for (const MoveEntry &move : detail.tutorMoves)
        row(move).tutor = true;
    for (const MoveEntry &move : detail.eggMoves)
        row(move).egg = true;
    return rows;
}

const AbilityEntry *SquadSession::ability(int slot) const
{
    const int id = member(slot).abilityId;
    for (const AbilityEntry &a : m_details[std::size_t(slot)].abilities)
        if (a.abilityId == id)
            return &a;
    return nullptr;
}

const Nature *SquadSession::nature(int slot) const
{
    const int id = member(slot).natureId;
    for (const Nature &n : m_natures)
        if (n.id == id)
            return &n;
    return nullptr;
}

const ItemRow *SquadSession::item(int slot) const
{
    const int id = member(slot).itemId;
    for (const ItemRow &i : m_items)
        if (i.id == id)
            return &i;
    return nullptr;
}

void SquadSession::setName(const QString &name)
{
    if (m_squad.name == name)
        return;
    m_squad.name = name;
    commit();
}

void SquadSession::setVersion(const QString &version)
{
    if (m_version == version)
        return;
    m_store->setCurrentGame(m_generation, version);
    if (m_state->game() == version)
        reload(); // 앱 상태는 이미 그 버전(저장소만 달랐다)
    else
        m_state->setGame(version); // → gameChanged → reload: 그 버전의 스쿼드를 읽고 분석한다
}

void SquadSession::setPokemon(int slot, int pokemonId)
{
    SquadMember member;
    member.pokemonId = pokemonId;
    m_squad.members[std::size_t(slot)] = member;
    resolve(slot);
    // 특성은 첫 칸(일반 특성)으로 미리 고른다 — 1 · 2세대는 특성이 없어 비어 있다
    const PokemonDetail &detail = m_details[std::size_t(slot)];
    if (!detail.abilities.isEmpty())
        m_squad.members[std::size_t(slot)].abilityId = detail.abilities.first().abilityId;
    commit();
}

void SquadSession::clearSlot(int slot)
{
    m_squad.members[std::size_t(slot)] = SquadMember();
    resolve(slot);
    commit();
}

void SquadSession::swapSlots(int a, int b)
{
    if (a == b || a < 0 || b < 0 || a >= int(kSquadSize) || b >= int(kSquadSize))
        return;
    std::swap(m_squad.members[std::size_t(a)], m_squad.members[std::size_t(b)]);
    std::swap(m_details[std::size_t(a)], m_details[std::size_t(b)]);
    std::swap(m_moves[std::size_t(a)], m_moves[std::size_t(b)]);
    commit();
}

void SquadSession::moveSlot(int from, int to)
{
    if (from == to || from < 0 || to < 0 || from >= int(kSquadSize) || to >= int(kSquadSize))
        return;
    // 한 칸씩 이웃과 바꿔 가며 옮긴다 = 빼서 끼우기(rotate)
    const int step = from < to ? 1 : -1;
    for (int i = from; i != to; i += step) {
        std::swap(m_squad.members[std::size_t(i)], m_squad.members[std::size_t(i + step)]);
        std::swap(m_details[std::size_t(i)], m_details[std::size_t(i + step)]);
        std::swap(m_moves[std::size_t(i)], m_moves[std::size_t(i + step)]);
    }
    commit();
}

void SquadSession::setMove(int slot, int index, int moveId)
{
    SquadMember &member = m_squad.members[std::size_t(slot)];
    if (member.moves[std::size_t(index)] == moveId)
        return;
    // 같은 기술을 두 칸에 두지 않는다: 이미 다른 칸에 있으면 그 칸과 바꾼다
    for (int &other : member.moves)
        if (moveId > 0 && other == moveId)
            other = member.moves[std::size_t(index)];
    member.moves[std::size_t(index)] = moveId;
    resolve(slot);
    commit();
}

void SquadSession::replaceSquad(const Squad &squad)
{
    m_squad = squad;
    resolveAll();
    commit();
}

void SquadSession::setShuttlePokemon(int pokemonId)
{
    SquadMember member;
    member.pokemonId = pokemonId;
    m_squad.shuttle = member;
    resolveShuttle();
    commit();
}

void SquadSession::setShuttleMove(int index, int moveId)
{
    SquadMember &shuttle = m_squad.shuttle;
    if (shuttle.moves[std::size_t(index)] == moveId)
        return;
    for (int &other : shuttle.moves) // 같은 기술을 두 칸에 두지 않는다
        if (moveId > 0 && other == moveId)
            other = shuttle.moves[std::size_t(index)];
    shuttle.moves[std::size_t(index)] = moveId;
    resolveShuttle();
    commit();
}

void SquadSession::clearShuttle()
{
    if (m_squad.shuttle == SquadMember())
        return;
    m_squad.shuttle = SquadMember();
    resolveShuttle();
    commit();
}

void SquadSession::setMemo(int slot, const QString &memo)
{
    SquadMember &member = m_squad.members[std::size_t(slot)];
    const QString trimmed = memo.left(SquadMember::kMemoLength);
    if (member.memo == trimmed)
        return;
    member.memo = trimmed;
    m_store->setSquad(m_generation, m_version,
                      m_squad); // 분석과 상관없다 → 다시 그리지 않는다
}

void SquadSession::setAbility(int slot, int abilityId)
{
    m_squad.members[std::size_t(slot)].abilityId = abilityId;
    commit();
}

void SquadSession::setNature(int slot, int natureId)
{
    natures(); // nature()가 찾을 목록
    m_squad.members[std::size_t(slot)].natureId = natureId;
    commit();
}

void SquadSession::setItem(int slot, int itemId)
{
    items(); // item()이 찾을 목록
    m_squad.members[std::size_t(slot)].itemId = itemId;
    commit();
}
} // namespace com::yamada::studio
