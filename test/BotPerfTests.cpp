#include "TestFramework.h"
#include "game/PlayerBots/BotPerf.h"

// BotPerf is pure arithmetic: no game objects needed.

TEST(BotPerf_StrideStaysFullRateNearParty)
{
    ASSERT_EQ(BotPerf::OutOfCombatStride(0.0f), 1u);
    ASSERT_EQ(BotPerf::OutOfCombatStride(29.9f), 1u);
}

TEST(BotPerf_StrideStepsDownWithDistance)
{
    ASSERT_EQ(BotPerf::OutOfCombatStride(30.0f), 2u);
    ASSERT_EQ(BotPerf::OutOfCombatStride(99.9f), 2u);
    ASSERT_EQ(BotPerf::OutOfCombatStride(100.0f), 3u);
    ASSERT_EQ(BotPerf::OutOfCombatStride(1000.0f), 3u);
}
