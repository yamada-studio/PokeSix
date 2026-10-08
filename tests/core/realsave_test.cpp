// H1 완료 조건 — 본인 세이브로 확인. 세이브는 리포 밖에 두고 환경 변수로 알려 준다(없으면 건너뜀):
//   export POKESIX_SAVE_PT=$HOME/pokesix-saves/platinum.sav
//   export POKESIX_SAVE_HGSS=$HOME/pokesix-saves/soulsilver.sav
//   ctest --preset linux-debug -R RealSave --output-on-failure -V
// 읽은 파티를 찍으므로 -V(자세히)로 보면 게임 화면의 파티와 나란히 비교할 수 있다.
#include "core/save/partyreader.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using namespace com::yamada::studio::save;

namespace {
std::vector<std::uint8_t> loadFile(const char *path)
{
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void checkRealSave(const char *envName, std::string_view expectedGroup)
{
    const char *path = std::getenv(envName);
    if (path == nullptr || *path == '\0')
        GTEST_SKIP() << "set " << envName << " to your own save file (kept outside the repo)";

    const std::vector<std::uint8_t> bytes = loadFile(path);
    ASSERT_FALSE(bytes.empty()) << "cannot read " << path;
    const auto party = readParty(bytes);
    ASSERT_TRUE(party.has_value()) << "no party found in " << path;
    EXPECT_EQ(party->layout->versionGroup, expectedGroup);

    std::printf("  %s: slot 0x%05zX, save counter %u.%u (major.minor)\n", path, party->generalStart,
                party->footer.major, party->footer.minor);
    for (std::size_t i = 0; i < party->members.size(); ++i) {
        const ReadMember &m = party->members[i];
        std::printf("  %zu  #%d Lv.%d  moves %d %d %d %d  item %d  ability %d  nature %d%s\n",
                    i + 1, m.species, m.level, m.moves[0], m.moves[1], m.moves[2], m.moves[3],
                    m.heldItem, m.ability, m.nature, m.egg ? "  (egg)" : "");
        EXPECT_TRUE(m.checksumOk) << "member " << i + 1;
        EXPECT_GE(m.species, 1) << "member " << i + 1;
        EXPECT_LE(m.species, 493) << "member " << i + 1; // 4세대 전국도감 끝
        if (!m.egg) {
            EXPECT_GE(m.level, 1) << "member " << i + 1;
            EXPECT_LE(m.level, 100) << "member " << i + 1;
        }
    }
}
} // namespace

TEST(RealSave, Platinum)
{
    checkRealSave("POKESIX_SAVE_PT", "platinum");
}

TEST(RealSave, HeartGoldSoulSilver)
{
    checkRealSave("POKESIX_SAVE_HGSS", "heartgold-soulsilver");
}
