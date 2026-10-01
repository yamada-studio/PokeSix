#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QLabel;
class QScrollArea;

namespace com::yamada::studio {
class AppState;
class EncounterList;
class EvolutionView;
class MatchupView;
class MoveList;
class PanelFrame;
class ProfileCard;
class SpriteCache;
class StatRadar;

// 도감 상세 화면 (로드맵 E1, 레퍼런스: 포딕 4세대 상세). 도감 목록에서 포켓몬을 누르면 열린다.
//
//   [← 목록]                                          기술 기준: 기라티나(Pt)
//   ┌ 그림 · 이름 ┐ ┌ 종족값(레이더) ┐ ┌ 획득법(야생 출현) ──────┐
//   └────────────┘ └───────┘ └──────────────────────────────┘
//   ┌ 타입 상성 (받을 때 · 줄 때) ────────────────────────────┐
//   ┌ 레벨업으로 익히는 기술 (Lv 1 = 하트비늘) ────────────────┐
//   ┌ 기술머신 · 비전머신 (번호 · 획득처) ─────────────────────┐
//
// 모든 값은 지금 세대 기준이다. 기술 · 기술머신은 그 세대의 대표 게임(Repository::
// representativeVersionGroup — 4세대 = 플래티넘) 기준. 세대 · 언어가 바뀌면 다시 그린다.
class DexDetailPage : public QWidget
{
    Q_OBJECT
public:
    DexDetailPage(Repository *repository, AppState *state, QWidget *parent = nullptr);

    void showPokemon(int pokemonId); // 그 포켓몬을 읽어 채운다

signals:
    void backRequested(); // [← 목록] · Esc · 지금 세대에 없는 포켓몬이 되었을 때

private:
    QWidget *buildContent();
    void reload();        // 같은 포켓몬을 지금 세대로 다시 읽는다
    void applyLanguage(); // 읽은 값을 지금 언어로 다시 그린다

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SpriteCache *m_fronts = nullptr;
    SpriteCache *m_icons = nullptr;        // 아이템 아이콘(하트비늘)
    SpriteCache *m_pokemonIcons = nullptr; // 포켓몬 박스 아이콘(진화 트리)
    PokemonDetail m_detail;
    TypeChart m_chart;

    QScrollArea *m_scroll = nullptr;
    QLabel *m_basis = nullptr;
    ProfileCard *m_profile = nullptr;
    StatRadar *m_stats = nullptr;
    PanelFrame *m_statsPanel = nullptr;
    EncounterList *m_encounters = nullptr;
    QLabel *m_evolutionLabel = nullptr;
    EvolutionView *m_evolution = nullptr;
    MatchupView *m_matchups = nullptr;
    MoveList *m_levelMoves = nullptr;
    MoveList *m_machineMoves = nullptr;
    PanelFrame *m_machinePanel = nullptr;
};
} // namespace com::yamada::studio
