#pragma once

#include "data/text/localizedtext.h"

#include <QString>

// 이름 · 설명 보충 사전: PokéAPI에 한국어(또는 일본어) 값이 없는 것을 채운다.
//
// 값은 resources/data/names.json(qrc ":/data/names.json")에 있고, 처음 쓸 때 한 번 읽는다.
// PokéAPI 값이 이미 있는 언어 칸은 건드리지 않는다 — 사전은 "빈 칸 채우기"만 한다. 그래서
// PokéAPI가 나중에 번역을 더해도 충돌이 없고, 사전 항목은 그때 지워도 된다.
// 받은 DB를 다시 만들지 않고(재다운로드 없이) 실행 중에 덧입힌다.
// 장소 이름은 도로 번호 규칙이 있는 ui/dex/guidebook(place-names.json)이 맡는다.
namespace com::yamada::studio::namebook {
enum class Kind {
    Item,
    Move,
    Ability,
    Version,
    ItemEffect,
    MoveEffect,
    AbilityEffect,
};

// identifier(PokéAPI identifier: "devon-goods" · "shadow-rush" · "the-teal-mask-scarlet")의
// 사전 값으로 text의 빈 언어 칸을 채운다.
void fill(Kind kind, const QString &identifier, LocalizedText &text);
} // namespace com::yamada::studio::namebook
