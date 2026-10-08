#include "data/store/saveimport.h"

#include "core/save/saveformat.h"
#include "data/logging/logging.h"
#include "data/repository/repository.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QHash>

namespace com::yamada::studio::saveimport {
namespace {
// TODO(H6-CP5-1) 이 상수를 지운다 — 이제 세대는 세이브가 알려 준다(party->generation). 지우면 아래
// 세 곳이
//   컴파일 오류로 알려 준다: versionsOfGroup(세대를 인자로 받게), itemIdForGameIndex,
//   portable.generation
constexpr int kGeneration = 4;

QString tr(const char *text)
{
    return QCoreApplication::translate("com::yamada::studio::saveimport", text);
}

// 게임 묶음("heartgold-soulsilver")의 버전들({"heartgold", "soulsilver"}) — DB의 게임 표에서
QStringList versionsOfGroup(Repository &repository,
                            const QString &versionGroup) // TODO(H6-CP5-2) int generation 인자
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
    const QString suffix = QFileInfo(path).suffix().toLower();
    return suffix == QStringLiteral("sav") || suffix == QStringLiteral("dsv");
}

QString versionOfOriginGame(int originGame)
{
    static const QHash<int, QString> versions = {
            {7, QStringLiteral("heartgold")}, {8, QStringLiteral("soulsilver")},
            {10, QStringLiteral("diamond")},  {11, QStringLiteral("pearl")},
            {12, QStringLiteral("platinum")},
            // TODO(H6-CP5-3) 5세대: 20 white · 21 black · 22 white-2 · 23 black-2
    };
    return versions.value(originGame);
}

std::optional<squadfile::Portable> load(const QString &path, Repository &repository, QString *error)
{
    // ① 파일 → 바이트
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) // 부르는 쪽이 error를 안 넘길 수도 있다(기본값 nullptr)
            *error = tr("파일을 열 수 없어요: %1").arg(file.errorString());
        return std::nullopt;
    }

    // ② 바이트 → 파티: QByteArray를 core가 아는 save::Bytes(std::span)로 — 레이어 경계의
    // 변환(CLAUDE.md §4)
    const QByteArray raw = file.readAll();
    const save::Bytes bytes(reinterpret_cast<const std::uint8_t *>(raw.constData()),
                            std::size_t(raw.size()));
    const auto party = save::readParty(bytes); // raw가 살아 있는 동안만 bytes를 쓴다
    if (!party) {
        if (error)
            *error = tr("4세대(DP · Pt · HGSS) 세이브가 아니에요");
        return std::nullopt;
    }

    // ③ 버전 고르기: 묶음의 버전 중 파티 멤버의 출신 게임이 가장 많은 쪽. 아무도 안 맞으면(다른
    // 게임에서
    //   데려온 포켓몬뿐) 묶음의 첫 버전
    const QString versionGroup
            = QString::fromUtf8(party->versionGroup.data(), qsizetype(party->versionGroup.size()));
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

    // ④ 멤버 → SquadMember: 자리 순서를 그대로(세이브의 i번째 → 스쿼드의 i번째). 물건 · 성격은 게임
    //   번호라 DB id로 바꾸고, 종 · 기술 · 특성은 번호가 같다
    squadfile::Portable portable;
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

    // ⑤ 이름은 비워 둔다 — 스쿼드 화면이 기본 이름("소울실버 스쿼드")을 보인다
    portable.generation = kGeneration;
    portable.game = chosen;
    qCInfo(lcData).noquote() << QStringLiteral("save import: %1 · %2 members")
                                        .arg(portable.game)
                                        .arg(portable.squad.filled());

    return portable;
}
} // namespace com::yamada::studio::saveimport
