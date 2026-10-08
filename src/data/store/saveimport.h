#pragma once

#include "data/store/squadfile.h"

#include <QString>

#include <optional>

// 게임 세이브(.sav · .dsv) → 스쿼드 (H2, ADR 0018). 가이드: docs/guides/h2-squad-sav-import.md
//
// core 파서(save::readParty)가 읽은 파티를 스쿼드 파일과 **같은 모양**(squadfile::Portable)으로
// 바꾼다. 그래서 화면은 .pks 불러오기와 같은 흐름(세대 · 게임 전환 → replaceSquad)을 그대로 쓴다.
//
//   파일(QByteArray) ─▶ save::Bytes(span) ─▶ save::readParty ─▶ ReadParty (게임 번호들)
//        ─▶ 번호 변환(Repository: 물건 · 성격 · 기본 폼) ─▶ Portable { 세대 4, 버전, Squad }
//
// 읽기 전용이다 — 세이브에 쓰지 않는다. 세이브에 있지만 스쿼드에 칸이 없는 값(레벨 · 개체값 ·
// 노력치)은 버린다(나중에 전투 분석에서 쓴다 — docs/data/battle-inputs.md).
namespace com::yamada::studio {
class Repository;
}

namespace com::yamada::studio::saveimport {
// 확장자로 고른다: .sav(melonDS · 실기 덤프) · .dsv(DeSmuME). 대소문자 무시
bool isSaveFile(const QString &path);

// 출신 게임 번호(PK4 0x5F) → PokéAPI version identifier. 모르는 값이면 빈 문자열
//   7 heartgold · 8 soulsilver · 10 diamond · 11 pearl · 12 platinum
QString versionOfOriginGame(int originGame);

// 실패하면 std::nullopt — error에 사람이 읽을 한 줄(파일을 못 연다 · 4세대 세이브가 아니다 …)
std::optional<squadfile::Portable> load(const QString &path, Repository &repository,
                                        QString *error = nullptr);
} // namespace com::yamada::studio::saveimport
