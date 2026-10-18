#include "TestFramework.h"
#include "game/PlayerBots/BotPersonality.h"

// BotPersonality is pure state transitions and arithmetic: no game objects needed.

TEST(BotPersonality_DefaultIsNormal)
{
    BotPersonality personality;
    ASSERT_TRUE(personality.skill == BOT_SKILL_NORMAL);
    ASSERT_EQ(personality.TickIntervalMs(), 1000u);
}

TEST(BotPersonality_FromString)
{
    ASSERT_TRUE(BotPersonality::FromString("casual") == BOT_SKILL_CASUAL);
    ASSERT_TRUE(BotPersonality::FromString("normal") == BOT_SKILL_NORMAL);
    ASSERT_TRUE(BotPersonality::FromString("sharp") == BOT_SKILL_SHARP);
    ASSERT_TRUE(BotPersonality::FromString("SHARP") == BOT_SKILL_INVALID);
    ASSERT_TRUE(BotPersonality::FromString("") == BOT_SKILL_INVALID);
    ASSERT_TRUE(BotPersonality::FromString(nullptr) == BOT_SKILL_INVALID);
}

TEST(BotPersonality_ToStringRoundTrip)
{
    ASSERT_EQ(BotPersonality::ToString(BOT_SKILL_CASUAL), "casual");
    ASSERT_EQ(BotPersonality::ToString(BOT_SKILL_NORMAL), "normal");
    ASSERT_EQ(BotPersonality::ToString(BOT_SKILL_SHARP), "sharp");
    ASSERT_EQ(BotPersonality::ToString(BOT_SKILL_INVALID), "unknown");
    ASSERT_TRUE(BotPersonality::FromString(BotPersonality::ToString(BOT_SKILL_SHARP)) == BOT_SKILL_SHARP);
}

TEST(BotPersonality_TickIntervalCapsSharpAtBaseline)
{
    BotPersonality personality;
    personality.Set(BOT_SKILL_CASUAL);
    ASSERT_EQ(personality.TickIntervalMs(), 1500u);

    personality.Set(BOT_SKILL_NORMAL);
    ASSERT_EQ(personality.TickIntervalMs(), 1000u);

    // Sharp never ticks faster than a human-paced baseline.
    personality.Set(BOT_SKILL_SHARP);
    ASSERT_EQ(personality.TickIntervalMs(), 1000u);
}

TEST(BotPersonality_InterruptEagerness)
{
    BotPersonality personality;
    personality.Set(BOT_SKILL_CASUAL);
    ASSERT_EQ(personality.InterruptAttemptPercent(), 40u);

    personality.Set(BOT_SKILL_NORMAL);
    ASSERT_EQ(personality.InterruptAttemptPercent(), 75u);

    personality.Set(BOT_SKILL_SHARP);
    ASSERT_EQ(personality.InterruptAttemptPercent(), 100u);
}

TEST(BotPersonality_DispelEagerness)
{
    BotPersonality personality;
    personality.Set(BOT_SKILL_CASUAL);
    ASSERT_EQ(personality.DispelAttemptPercent(), 25u);

    personality.Set(BOT_SKILL_NORMAL);
    ASSERT_EQ(personality.DispelAttemptPercent(), 60u);

    personality.Set(BOT_SKILL_SHARP);
    ASSERT_EQ(personality.DispelAttemptPercent(), 100u);
}

TEST(BotPersonality_RecoveryThreshold)
{
    BotPersonality personality;
    personality.Set(BOT_SKILL_CASUAL);
    ASSERT_EQ(personality.RecoveryThresholdPercent(), 60u);

    personality.Set(BOT_SKILL_NORMAL);
    ASSERT_EQ(personality.RecoveryThresholdPercent(), 100u);

    personality.Set(BOT_SKILL_SHARP);
    ASSERT_EQ(personality.RecoveryThresholdPercent(), 100u);
}

TEST(BotPersonality_WantsAttemptBoundaries)
{
    ASSERT_FALSE(BotPersonality::WantsAttempt(0u, 0u));
    ASSERT_FALSE(BotPersonality::WantsAttempt(75u, 75u));
    ASSERT_TRUE(BotPersonality::WantsAttempt(75u, 74u));
    ASSERT_TRUE(BotPersonality::WantsAttempt(100u, 99u));
    // Over-range percent clamps instead of always failing.
    ASSERT_TRUE(BotPersonality::WantsAttempt(1000u, 99u));
    ASSERT_FALSE(BotPersonality::WantsAttempt(1000u, 100u));
}
