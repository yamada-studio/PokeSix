#pragma once

#include "data/store/squad.h"

#include <QString>

#include <optional>

// 스쿼드 내보내기 파일(.pks) 읽기 · 쓰기. 내용은 JSON — squads.json의 스쿼드 한 개
// 모양(squadToJson)에 세대 · 게임(버전)을 붙인 것이라, 다른 PC의 PokeSix로 그대로 옮길 수 있다.
//   { "pokesix-squad": 1, "generation": 4, "game": "heartgold", "squad": { … } }
// 확장자만 우리 것(.pks)이고 형식 검사는 안의 "pokesix-squad" 키로 한다(.json이었던 초기
// 내보내기 파일도 읽힌다).
namespace com::yamada::studio::squadfile {
inline constexpr int kFormatVersion = 1;

struct Portable
{
    int generation = 0;
    QString game; // 버전 identifier("heartgold")
    Squad squad;
};

// 실패하면 false — error에 사람이 읽을 한 줄(파일을 못 연다 …)
bool save(const QString &path, const Portable &portable, QString *error = nullptr);
// 실패하면 std::nullopt — PokeSix 스쿼드 파일이 아니거나 깨진 JSON
std::optional<Portable> load(const QString &path, QString *error = nullptr);

// 저장 대화 상자의 기본 파일 이름: "pokesix-squad-4-heartgold.pks"
QString fileName(const Portable &portable);

inline constexpr char kExtension[] = ".pks";
} // namespace com::yamada::studio::squadfile
