#include "TestFramework.h"
#include "game/PlayerBots/BotHealer.h"

// BotHealer is pure triage arithmetic: no game objects needed.

TEST(BotHealer_FirstCandidateWinsSeededRace)
{
    // Candidates arrive strictly below 100%, so seeding best at 100%
    // lets the first admitted candidate always win.
    ASSERT_TRUE(BotHealer::WantsTriageTarget(99.0f, false, 100.0f, false));
    ASSERT_TRUE(BotHealer::WantsTriageTarget(10.0f, true, 100.0f, false));
}

TEST(BotHealer_LowerHealthWins)
{
    ASSERT_TRUE(BotHealer::WantsTriageTarget(20.0f, false, 60.0f, false));
    ASSERT_TRUE(BotHealer::WantsTriageTarget(20.0f, false, 60.0f, true));
    ASSERT_FALSE(BotHealer::WantsTriageTarget(80.0f, false, 60.0f, false));
    ASSERT_FALSE(BotHealer::WantsTriageTarget(80.0f, true, 60.0f, false));
}

TEST(BotHealer_TankBreaksHealthTie)
{
    ASSERT_TRUE(BotHealer::WantsTriageTarget(50.0f, true, 50.0f, false));
    ASSERT_FALSE(BotHealer::WantsTriageTarget(50.0f, false, 50.0f, true));
    ASSERT_FALSE(BotHealer::WantsTriageTarget(50.0f, false, 50.0f, false));
    ASSERT_FALSE(BotHealer::WantsTriageTarget(50.0f, true, 50.0f, true));
}

TEST(BotHealer_RezRankHealerFirst)
{
    ASSERT_EQ(BotHealer::RezRank(true, false), 0u);
    ASSERT_EQ(BotHealer::RezRank(true, true), 0u);
    ASSERT_EQ(BotHealer::RezRank(false, true), 1u);
    ASSERT_EQ(BotHealer::RezRank(false, false), 2u);
}

TEST(BotHealer_PickStaysStableForIdenticalInput)
{
    // Equal non-tanks keep the incumbent: no tick-to-tick flapping.
    ASSERT_FALSE(BotHealer::WantsTriageTarget(40.0f, false, 40.0f, false));
}
