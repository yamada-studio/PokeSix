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
} // namespace

bool isSaveFile(const QString &path)
{
    // TODO(H2-CP4-1) QFileInfo(path).suffix()를 소문자로(toLower) 바꿔 "sav" 또는 "dsv"인가
    (void)path;
    return false;
}

QString versionOfOriginGame(int originGame)
{
    // TODO(H2-CP4-2) 헤더 주석의 다섯 쌍을 표로. 예) static const QHash<int, QString> 하나 +
    //   value(originGame) — 없는 키면 QHash::value가 빈 QString을 준다
    (void)originGame;
    return {};
}

std::optional<squadfile::Portable> load(const QString &path, Repository &repository, QString *error)
{
    // ① 파일 → 바이트. 실패하면 error에 tr("파일을 열 수 없어요: %1").arg(file.errorString())
    // TODO(H2-CP4-3) QFile file(path) · open(QIODevice::ReadOnly) · readAll()

    // ② 바이트 → 파티. Qt의 QByteArray를 core가 아는 save::Bytes(std::span)로 — 레이어 경계의 변환
    //   (CLAUDE.md §4). reinterpret_cast<const std::uint8_t *>(raw.constData()),
    //   std::size_t(raw.size()) 실패하면 tr("4세대(DP · Pt · HGSS) 세이브가 아니에요")
    // TODO(H2-CP4-4) const auto party = save::readParty(bytes);

    // ③ 버전 고르기: 그 게임 묶음(party->layout->versionGroup)의 버전들 중 파티 멤버 출신 게임
    //   (versionOfOriginGame(member.originGame))이 가장 많은 쪽. 하나도 안 맞으면(다른 게임에서
    //   데려온 포켓몬뿐) 묶음의 첫 버전. 묶음의 버전 목록은
    //   repository.gamesForGeneration(kGeneration)의 GameInfo 중 versionGroup이 같은 것의 versions
    // TODO(H2-CP4-5)

    // ④ 멤버 → SquadMember (같은 자리 순서 그대로)
    //   - 알이면 빈 자리로 둔다(싸우지 않는다)
    //   - pokemonId = repository.defaultPokemonId(species). form이 0이 아니면 아직 폼 표가 없으니
    //     qCWarning(lcData)로 남기고 기본 폼으로
    //   - moves · abilityId는 번호가 PokéAPI id와 같다 — 그대로
    //   - natureId = repository.natureIdForGameIndex(member.nature)
    //   - itemId = heldItem이 0이면 0, 아니면 repository.itemIdForGameIndex(kGeneration, heldItem)
    // TODO(H2-CP4-6)

    // ⑤ Portable { generation = kGeneration, game = 고른 버전, squad }. 이름은 비워 둔다(기본
    // 이름이 보인다)
    //   qCInfo(lcData)로 "save import: <게임> · <n> members" 한 줄
    // TODO(H2-CP4-7)
    (void)path;
    (void)repository;
    if (error)
        *error = tr("아직 구현 중이에요");
    (void)kGeneration;
    return std::nullopt;
}
} // namespace com::yamada::studio::saveimport
