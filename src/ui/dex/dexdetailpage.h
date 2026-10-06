#pragma once

#include "data/repository/repository.h"

#include <QWidget>

class QLabel;
class QScrollArea;

namespace com::yamada::studio {
class AppState;
class EncounterList;
class AbilityList;
class EvolutionView;
class DropdownButton;
class GameSelector;
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

    // 그 포켓몬을 읽어 채운다. version = 기준 게임(버전 "soulsilver". 비면 앱이 고른 게임)
    void showPokemon(int pokemonId, const QString &version = {});
    // 기준 게임 칩이 앱 전체의 게임(AppState)을 바꾸는가(도감 화면 — 기본). false면 이 화면
    // 안에서만 바꾼다(스쿼드 멤버 모달: 뒤의 스쿼드가 다른 버전으로 바뀌지 않게)
    void setFollowsAppGame(bool follows) { m_followsAppGame = follows; }

signals:
    void backRequested(); // [← 목록] · Esc · 지금 세대에 없는 포켓몬이 되었을 때

private:
    QWidget *buildContent();
    void reload();        // 같은 포켓몬을 지금 세대로 다시 읽는다
    void applyLanguage(); // 읽은 값을 지금 언어로 다시 그린다
    void showNaturePicker();
    void applyNature(); // 고른 성격 → 버튼 글자 · 레이더 ▲▼

    Repository *m_repository = nullptr;
    AppState *m_state = nullptr;
    SpriteCache *m_fronts = nullptr;
    SpriteCache *m_icons = nullptr;        // 아이템 아이콘(하트비늘)
    SpriteCache *m_pokemonIcons = nullptr; // 포켓몬 박스 아이콘(진화 트리)
    PokemonDetail m_detail;
    QString m_version; // 기준 게임(버전 — 진화 트리로 옮겨 가도 그대로). 비면 앱이 고른 게임
    bool m_followsAppGame = true;
    TypeChart m_chart;

    QScrollArea *m_scroll = nullptr;
    GameSelector *m_games = nullptr; // 기준 게임 칩 [BW][B2W2]
    QList<GameInfo> m_gameList;      // 지금 세대의 게임(칩)
    int m_gameGeneration = 0;        // m_gameList를 읽은 세대
    ProfileCard *m_profile = nullptr;
    StatRadar *m_stats = nullptr;
    PanelFrame *m_statsPanel = nullptr;
    DropdownButton *m_natureButton = nullptr; // 종족값 카드 머리 [성격 ▾]
    AbilityList *m_abilities = nullptr;
    QList<Nature> m_natures; // 처음 성격표를 열 때 읽는다
    int m_natureId = 0; // 고른 성격(포켓몬을 바꿔도 그대로 — 같은 성격으로 견줘 보게). 0 = 없음
    EncounterList *m_encounters = nullptr;
    QLabel *m_evolutionLabel = nullptr;
    EvolutionView *m_evolution = nullptr;
    MatchupView *m_matchups = nullptr;
    MoveList *m_levelMoves = nullptr;
    MoveList *m_machineMoves = nullptr;
    PanelFrame *m_machinePanel = nullptr;
    MoveList *m_tutorMoves = nullptr; // 가르침(그 게임의 NPC 튜터) — 없으면 창째 숨김
    PanelFrame *m_tutorPanel = nullptr;
    MoveList *m_eggMoves = nullptr; // 알 기술(부모에게서 물려받는다)
    PanelFrame *m_eggPanel = nullptr;
};
} // namespace com::yamada::studio
