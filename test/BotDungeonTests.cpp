#include "TestFramework.h"
#include "game/PlayerBots/BotDungeon.h"

// BotDungeon is pure geometry: no game objects needed.

TEST(BotDungeon_SpreadStepsDirectlyAway)
{
    // 3 yards apart on X, target 8: step 5 further out.
    float x = 0.0f, y = 0.0f;
    BotDungeon::SpreadDestination(0.0f, 0.0f, 3.0f, 0.0f, BotDungeon::SPREAD_TARGET_DIST, x, y);
    ASSERT_EQ(x, -5.0f);
    ASSERT_EQ(y, 0.0f);
}

TEST(BotDungeon_SpreadZeroDistanceFallsBackToPlusX)
{
    float x = 0.0f, y = 0.0f;
    BotDungeon::SpreadDestination(2.0f, 2.0f, 2.0f, 2.0f, BotDungeon::SPREAD_TARGET_DIST, x, y);
    ASSERT_EQ(x, 10.0f);
    ASSERT_EQ(y, 2.0f);
}

TEST(BotDungeon_SpreadRestoresTargetDistance)
{
    // 4 yards apart on Y, target 8: step 4, landing 8 away.
    float x = 0.0f, y = 0.0f;
    BotDungeon::SpreadDestination(0.0f, 0.0f, 0.0f, 4.0f, BotDungeon::SPREAD_TARGET_DIST, x, y);
    ASSERT_EQ(x, 0.0f);
    ASSERT_EQ(y, -4.0f);
}
