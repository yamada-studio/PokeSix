#pragma once

#include <map>
#include <span>
#include <string>
#include <vector>

namespace com::yamada::studio::resourceledger {
// 스쿼드가 기술 배치에 쓰는 소모 자원을 센다: 하트비늘(기술 떠올리기) · 기술머신 · NPC 가르침
// 비용. 한 게임에 하나뿐인 기술머신을 두 자리에 배치하면 모자람(shortfall)으로 경고한다.
//
// 배우는 방법(TeachPlan)과 기술머신의 입수 사정(MachineSupply)은 data/ui 계층이 멤버의 기술
// 목록(movereach가 매긴 하트비늘 표시)과 입수 사전으로 채워서 넘긴다 — core는 게임을 몰라도 된다.

struct Cost
{
    int amount = 0;
    std::string unit; // 입수 사전의 단위 그대로: "bp" · "coins" · "red-shard" · "money" …
};

// 기술 칸 하나를 어떻게 배우는가
enum class Means {
    LevelUp,  // 자력(기본 기술 · 레벨업) — 자원 없음
    Reminder, // 기술 떠올리기 — 하트비늘 1개
    Machine,  // 기술머신
    Tutor,    // NPC 가르침
    Other,    // 비전머신 · 알 기술 · 배울 수 없음 — 집계 밖
};

struct TeachPlan
{
    int slot = 0;
    Means means = Means::Other;
    std::string machine;    // Machine: 기술머신 아이템 identifier("tm26")
    std::vector<Cost> cost; // Tutor: 한 번 배우는 비용
};

// 기술머신 하나의 그 게임 입수 사정
struct MachineSupply
{
    bool known = false; // 입수 사전에 이 기술머신이 있다(모르면 경고도 비용도 내지 않는다)
    int copies = 0; // 한 번만 얻는 입수처 수(필드 · 숨겨진 · 받기 · 보상)
    bool repeatable = false;      // 반복 입수처(상점 · 교환 · 경품)가 있다
    std::vector<Cost> repeatCost; // 반복 입수 한 번의 비용
    bool postGameOnly = false;    // 모든 입수처가 엔딩 후 지역(HGSS의 관동)
};

struct MachineUse
{
    std::string machine;
    // Qt 쪽에서도 쓰는 헤더라 moc 키워드인 slots라는 이름은 피한다
    std::vector<int> members; // 이 기술머신이 필요한 슬롯(들어온 순서, 중복 없이)
    int bought = 0; // 반복 입수로 사야 하는 수 = max(0, 쓰임 − copies). 반복 입수가 있을 때만
    bool shortfall = false; // 쓰임 > copies 인데 반복 입수가 없다 → 경고
    MachineSupply supply;
};

struct Ledger
{
    int heartScales = 0;              // Reminder 칸 수
    std::vector<MachineUse> machines; // machine identifier 순
    std::vector<Cost> totals; // 단위별 합계(기술머신 구매 bought × 비용 + 가르침 비용), 단위 순
};

Ledger tally(std::span<const TeachPlan> plans,
             const std::map<std::string, MachineSupply> &supplies);
} // namespace com::yamada::studio::resourceledger
