#pragma once

#include <QString>

// SpriteCache::Kind::PokemonFront의 key를 만드는 규칙. 세대마다 정면 그림 폴더(PokéAPI/sprites의
// versions/…)가 다르고, 없는 세대(7 · 8)는 기본 그림(96×96)으로 간다. 도감 상세와 홈 세대 카드가
// 같은 규칙을 쓴다.
namespace com::yamada::studio::spritekeys {
QString front(int generation, int pokemonId);
} // namespace com::yamada::studio::spritekeys
