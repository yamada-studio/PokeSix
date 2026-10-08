#include "data/store/saveimport.h"

#include "core/save/partyreader.h"
#include "data/logging/logging.h"
#include "data/repository/repository.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHash>

namespace com::yamada::studio::saveimport {
namespace {
// 지금은 4세대 표(kGen4Layouts)만 있다. 5세대(H6)가 오면 SaveLayout에 세대를 넣는다
constexpr int kGeneration = 4;

QString tr(const char *text)
{
    return QCoreApplication::translate("com::yamada::studio::saveimport", text);
}

// 게임 묶음("heartgold-soulsilver")의 버전들({"heartgold", "soulsilver"}) — DB의 게임 표에서
QStringList versionsOfGroup(Repository &repository, const QString &versionGroup)
{
    for (const GameInfo &game : repository.gamesForGeneration(kGeneration)) {
        if (game.versionGroup == versionGroup)
            return game.versions;
    }
    return {};
}
} // namespace

bool isSaveFile(const QString &path)
{
    // TODO(H2-CP4-1) QFileInfo(path).suffix()를 소문자로(toLower) 바꿔 "sav" 또는 "dsv"인가
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("sav") || suffix == QStringLiteral("dsv");
}

QString versionOfOriginGame(int originGame)
{
    // TODO(H2-CP4-2) 헤더 주석의 다섯 쌍을 표로. 예) static const QHash<int, QString> 하나 +
    //   value(originGame) — 없는 키면 QHash::value가 빈 QString을 준다
    //   7 heartgold · 8 soulsilver · 10 diamond · 11 pearl · 12 platinum
    static const QHash<int, QString> versions = {
            {7, QStringLiteral("heartgold")}, {8, QStringLiteral("soulsilver")},
            {10, QStringLiteral("diamond")},  {11, QStringLiteral("pearl")},
            {12, QStringLiteral("platinum")},
    };
    return versions.value(originGame);
}

std::optional<squadfile::Portable> load(const QString &path, Repository &repository, QString *error)
{
    // ① 파일 → 바이트. 실패하면 error에 tr("파일을 열 수 없어요: %1").arg(file.errorString())
    // TODO(H2-CP4-3) QFile file(path) · open(QIODevice::ReadOnly) · readAll()
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) // 부르는 쪽이 error를 안 넘길 수도 있다(기본값 nullptr)
            *error = tr("파일을 열 수 없어요: %1").arg(file.errorString());
        return std::nullopt;
    }

    // ② 바이트 → 파티. Qt의 QByteArray를 core가 아는 save::Bytes(std::span)로 — 레이어 경계의 변환
    //   (CLAUDE.md §4). reinterpret_cast<const std::uint8_t *>(raw.constData()),
    //   std::size_t(raw.size()) 실패하면 tr("4세대(DP · Pt · HGSS) 세이브가 아니에요")
    // TODO(H2-CP4-4) const auto party = save::readParty(bytes);
    const QByteArray raw = file.readAll();
    const save::Bytes bytes(reinterpret_cast<const std::uint8_t *>(raw.constData()),
                            std::size_t(raw.size()));
    const auto party = save::readParty(bytes); // raw가 살아 있는 동안만 bytes를 쓴다
    if (!party) {
        if (error)
            *error = tr("4세대(DP · Pt · HGSS) 세이브가 아니에요");
        return std::nullopt;
    }

    // ③ 버전 고르기: 그 게임 묶음(party->layout->versionGroup)의 버전들 중 파티 멤버 출신 게임
    //   (versionOfOriginGame(member.originGame))이 가장 많은 쪽. 하나도 안 맞으면(다른 게임에서
    //   데려온 포켓몬뿐) 묶음의 첫 버전. 묶음의 버전 목록은
    //   repository.gamesForGeneration(kGeneration)의 GameInfo 중 versionGroup이 같은 것의 versions

    const QString versionGroup = QString::fromUtf8(party->layout->versionGroup.data(),
                                                   qsizetype(party->layout->versionGroup.size()));
    const QStringList versions = versionsOfGroup(repository, versionGroup);
    QHash<QString, int> votes; // 버전 → 그 버전 출신 멤버 수
    for (const save::ReadMember &member : party->members) {
        const QString version = versionOfOriginGame(member.originGame);
        if (versions.contains(version))
            ++votes[version];
    }
    QString chosen
            = versions.isEmpty() ? QString() : versions.constFirst(); // 아무도 안 맞으면 첫 버전
    int best = 0;
    for (const QString &version : versions) {
        if (votes.value(version) > best) {
            best = votes.value(version);
            chosen = version;
        }
    }

    // ④ 멤버 → SquadMember (같은 자리 순서 그대로)
    //   - 알이면 빈 자리로 둔다(싸우지 않는다)
    //   - pokemonId = repository.defaultPokemonId(species). form이 0이 아니면 아직 폼 표가 없으니
    //     qCWarning(lcData)로 남기고 기본 폼으로
    //   - moves · abilityId는 번호가 PokéAPI id와 같다 — 그대로
    //   - natureId = repository.natureIdForGameIndex(member.nature)
    //   - itemId = heldItem이 0이면 0, 아니면 repository.itemIdForGameIndex(kGeneration, heldItem)
    squadfile::Portable portable;
    // 자리 순서를 그대로: 세이브의 i번째 → 스쿼드의 i번째 (파티는 최대 6, 스쿼드도 6자리)
    for (std::size_t i = 0; i < party->members.size() && i < portable.squad.members.size(); ++i) {
        const save::ReadMember &member = party->members[i];
        if (member.egg)
            continue; // 알은 싸우지 않는다 — 그 자리는 빈 자리로 남는다
        if (member.form != 0)
            qCWarning(lcData) << "save import: species" << member.species << "form" << member.form
                              << "has no form table yet — using the default form";
        SquadMember &out = portable.squad.members[i];
        out.pokemonId = repository.defaultPokemonId(member.species);
        out.moves = member.moves; // 둘 다 std::array<int, 4>, 번호도 PokéAPI id와 같다
        out.abilityId = member.ability;
        out.natureId = repository.natureIdForGameIndex(member.nature);
        out.itemId = member.heldItem == 0
                             ? 0
                             : repository.itemIdForGameIndex(kGeneration, member.heldItem);
    }

    // ⑤ Portable { generation = kGeneration, game = 고른 버전, squad }. 이름은 비워 둔다(기본
    // 이름이 보인다)
    //   qCInfo(lcData)로 "save import: <게임> · <n> members" 한 줄
    portable.generation = kGeneration;
    portable.game = chosen;
    // TODO(H2-CP4-7)
    qCInfo(lcData).noquote() << QStringLiteral("save import: %1 · %2 members")
                                        .arg(portable.game)
                                        .arg(portable.squad.filled());

    return portable;
}
} // namespace com::yamada::studio::saveimport
