#pragma once

#include <QString>

// 게임 데이터 DB(pokesix.sqlite)가 어디 있고, 쓸 수 있는 상태인지.
namespace com::yamada::studio::gamedatabase {
// AppDataLocation/pokesix.sqlite (Linux: ~/.local/share/YamadaStudio/PokeSix/pokesix.sqlite)
QString defaultPath();

// 파일이 있고, 이 앱이 아는 스키마 버전(schema::kVersion)으로 만들어졌는가.
// false면 첫 실행(또는 스키마가 바뀐 새 버전)이다 → 인트로가 "데이터 받기"를 보여 준다.
bool isUsable(const QString &path);
} // namespace com::yamada::studio::gamedatabase
