#pragma once

#include <QString>

// 게임 데이터 DB(pokesix.sqlite)가 어디 있고, 쓸 수 있는 상태인지.
namespace com::yamada::studio::gamedatabase {
// AppDataLocation/pokesix.sqlite (Linux: ~/.local/share/YamadaStudio/PokeSix/pokesix.sqlite)
QString defaultPath();

// 파일이 있고, 이 앱이 아는 스키마 버전(schema::kVersion)으로 만들어졌는가.
// false면 첫 실행(또는 스키마가 바뀐 새 버전)이다 → 인트로가 "데이터 받기"를 보여 준다.
bool isUsable(const QString &path);

// 변환이 만드는 임시 DB의 경로: dbPath + ".importing". 변환이 다 끝났을 때만 dbPath로 바꿔 넣는다.
QString pendingPath(const QString &dbPath);

// 임시 DB가 **이 빌드가 만들었을 것과 같은가** — 스키마 버전과 PokéAPI CSV
// 커밋(csvsource::kCommit)이 모두 같다. 둘 중 하나라도 다르면 옛 빌드가 남긴 것이라 쓰지 않는다.
bool matchesThisBuild(const QString &path);

// newFile을 dbPath 자리에 넣는다(옛 파일 삭제 → 이름 바꾸기). 실패하면 false — Windows에서는 다른
// 프로세스(예: 또 하나의 PokeSix 창)가 dbPath를 열어 두면 삭제도 이름 바꾸기도 막힌다. 그때
// newFile은 손대지 않고 남겨 둔다.
bool replaceWith(const QString &dbPath, const QString &newFile);

// 지난 변환이 끝났는데 바꿔 넣지 못한 채 남은 임시 DB가 있으면 지금 넣는다(앱 시작 때 · 다시 시도
// 때). 이 빌드와 맞지 않는 임시 DB는 지운다. 바꿔 넣었으면 true.
bool adoptPendingImport(const QString &dbPath);
} // namespace com::yamada::studio::gamedatabase
