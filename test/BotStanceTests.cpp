#include "TestFramework.h"
#include "game/PlayerBots/BotStance.h"

// BotStance is pure state transitions and arithmetic: no game objects needed.

TEST(BotStance_DefaultFollowsClose)
{
    BotStance stance;
    ASSERT_FALSE(stance.IsHolding());
    ASSERT_FALSE(stance.IsPassive());
    ASSERT_EQ(stance.FollowMinDist(), 3.0f);
    ASSERT_EQ(stance.FollowMaxDist(), 6.0f);
    ASSERT_FALSE(stance.IsOutOfLeash(1000.0f, 1000.0f, 1000.0f));
    ASSERT_TRUE(BotStance::WantsTargetSource(false, BOT_TARGET_LEADER_VICTIM));
    ASSERT_TRUE(BotStance::WantsTargetSource(false, BOT_TARGET_PARTY_ATTACKER));
}

TEST(BotStance_FollowFarDistance)
{
    BotStance stance;
    stance.Follow(true);
    ASSERT_FALSE(stance.IsHolding());
    ASSERT_FALSE(stance.IsPassive());
    ASSERT_EQ(stance.FollowMinDist(), 12.0f);
    ASSERT_EQ(stance.FollowMaxDist(), 18.0f);

    stance.Follow(false);
    ASSERT_EQ(stance.FollowMinDist(), 3.0f);
    ASSERT_EQ(stance.FollowMaxDist(), 6.0f);
}

TEST(BotStance_HoldKeepsAggression)
{
    BotStance stance;
    stance.Hold(10.0f, 20.0f, 30.0f);
    ASSERT_TRUE(stance.IsHolding());
    ASSERT_FALSE(stance.IsPassive());
    ASSERT_TRUE(stance.IsAtAnchor(10.0f, 20.0f, 30.0f));
    ASSERT_TRUE(stance.IsAtAnchor(11.0f, 20.0f, 30.0f));
    ASSERT_FALSE(stance.IsAtAnchor(13.0f, 20.0f, 30.0f));
}

TEST(BotStance_LeashOnlyWhileHolding)
{
    BotStance stance;
    stance.Hold(0.0f, 0.0f, 0.0f);
    ASSERT_FALSE(stance.IsOutOfLeash(29.0f, 0.0f, 0.0f));
    ASSERT_TRUE(stance.IsOutOfLeash(31.0f, 0.0f, 0.0f));
    ASSERT_TRUE(stance.IsOutOfLeash(0.0f, 0.0f, 31.0f));

    stance.ResumeFollow();
    ASSERT_FALSE(stance.IsHolding());
    ASSERT_FALSE(stance.IsOutOfLeash(500.0f, 0.0f, 0.0f));
}

TEST(BotStance_StayThenAssist)
{
    BotStance stance;
    stance.Stay(5.0f, 5.0f, 5.0f);
    ASSERT_TRUE(stance.IsHolding());
    ASSERT_TRUE(stance.IsPassive());

    stance.Assist();
    ASSERT_FALSE(stance.IsPassive());
    ASSERT_TRUE(stance.IsHolding());
    ASSERT_TRUE(stance.IsAtAnchor(5.0f, 5.0f, 5.0f));

    stance.ResumeFollow();
    ASSERT_FALSE(stance.IsHolding());
    ASSERT_FALSE(stance.IsPassive());
}

TEST(BotStance_PassiveSkipsAssistSourcesOnly)
{
    ASSERT_FALSE(BotStance::WantsTargetSource(true, BOT_TARGET_LEADER_VICTIM));
    ASSERT_FALSE(BotStance::WantsTargetSource(true, BOT_TARGET_PARTY_ATTACKER));
    ASSERT_TRUE(BotStance::WantsTargetSource(true, BOT_TARGET_DUEL));
    ASSERT_TRUE(BotStance::WantsTargetSource(true, BOT_TARGET_FOCUS_MARK));
    ASSERT_TRUE(BotStance::WantsTargetSource(true, BOT_TARGET_SELF_ATTACKER));
    ASSERT_TRUE(BotStance::WantsTargetSource(true, BOT_TARGET_PET_ATTACKER));
}

TEST(BotStance_ClearHoldKeepsPassive)
{
    BotStance stance;
    stance.Stay(1.0f, 2.0f, 3.0f);
    stance.ClearHold();
    ASSERT_FALSE(stance.IsHolding());
    ASSERT_TRUE(stance.IsPassive());
}

TEST(BotStance_DistSq)
{
    ASSERT_EQ(BotStance::DistSq(0.0f, 0.0f, 0.0f, 3.0f, 4.0f, 0.0f), 25.0f);
    ASSERT_EQ(BotStance::DistSq(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f), 0.0f);
}
