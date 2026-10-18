#include "TestFramework.h"
#include "game/PlayerBots/BotObjective.h"

// BotObjective is pure role arithmetic: no game objects needed.

TEST(BotObjective_DefaultHasNoBaseRole)
{
    BotObjective objective;
    ASSERT_FALSE(objective.HasBaseRole());
    ASSERT_TRUE(objective.baseRole == BOT_OBJECTIVE_INVALID);
}

TEST(BotObjective_AssignBaseRoleSplitsTwoInFive)
{
    ASSERT_TRUE(BotObjective::AssignBaseRole(0u) == BOT_OBJECTIVE_DEFEND);
    ASSERT_TRUE(BotObjective::AssignBaseRole(1u) == BOT_OBJECTIVE_DEFEND);
    ASSERT_TRUE(BotObjective::AssignBaseRole(2u) == BOT_OBJECTIVE_ASSAULT);
    ASSERT_TRUE(BotObjective::AssignBaseRole(3u) == BOT_OBJECTIVE_ASSAULT);
    ASSERT_TRUE(BotObjective::AssignBaseRole(4u) == BOT_OBJECTIVE_ASSAULT);
    ASSERT_TRUE(BotObjective::AssignBaseRole(5u) == BOT_OBJECTIVE_DEFEND);

    BotObjective objective;
    objective.AssignBase(7u);
    ASSERT_TRUE(objective.HasBaseRole());
    ASSERT_TRUE(objective.baseRole == BOT_OBJECTIVE_ASSAULT);
    objective.Reset();
    ASSERT_FALSE(objective.HasBaseRole());
}

TEST(BotObjective_MapWsFlagState)
{
    ASSERT_TRUE(BotObjective::MapWsFlagState(0u) == BOT_FLAG_HOME);
    ASSERT_TRUE(BotObjective::MapWsFlagState(1u) == BOT_FLAG_GONE);
    ASSERT_TRUE(BotObjective::MapWsFlagState(2u) == BOT_FLAG_CARRIED);
    ASSERT_TRUE(BotObjective::MapWsFlagState(3u) == BOT_FLAG_DROPPED);
    ASSERT_TRUE(BotObjective::MapWsFlagState(9u) == BOT_FLAG_GONE);
}

TEST(BotObjective_FocusMirrorsRolePriority)
{
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_HOME, BOT_FLAG_HOME) == BOT_FOCUS_NONE);
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_CARRIED, BOT_FLAG_HOME) == BOT_FOCUS_OWN_KEEPER);
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_DROPPED, BOT_FLAG_CARRIED) == BOT_FOCUS_OWN_DROP);
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_HOME, BOT_FLAG_CARRIED) == BOT_FOCUS_ENEMY_KEEPER);
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_GONE, BOT_FLAG_DROPPED) == BOT_FOCUS_ENEMY_DROP);
    ASSERT_TRUE(BotObjective::FocusForState(BOT_FLAG_GONE, BOT_FLAG_GONE) == BOT_FOCUS_NONE);
}

TEST(BotObjective_RoleAgreesWithFocus)
{
    // Nearest bot converges exactly where the focus points, for every flag
    // combination. Keeps the two priority tables from drifting apart.
    BotFlagState states[] = { BOT_FLAG_HOME, BOT_FLAG_CARRIED, BOT_FLAG_DROPPED, BOT_FLAG_GONE };
    for (int o = 0; o < 4; ++o)
    {
        for (int e = 0; e < 4; ++e)
        {
            BotObjectiveRole role = BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, states[o], states[e], false, true);
            BotObjectiveFocus focus = BotObjective::FocusForState(states[o], states[e]);
            if (role == BOT_OBJECTIVE_CHASE)
                ASSERT_TRUE(focus == BOT_FOCUS_OWN_KEEPER);
            else if (role == BOT_OBJECTIVE_ESCORT)
                ASSERT_TRUE(focus == BOT_FOCUS_ENEMY_KEEPER);
            else if (role == BOT_OBJECTIVE_RECOVER)
                ASSERT_TRUE(focus == BOT_FOCUS_OWN_DROP || focus == BOT_FOCUS_ENEMY_DROP);
            else
                ASSERT_TRUE(focus == BOT_FOCUS_NONE || role == BOT_OBJECTIVE_ASSAULT || role == BOT_OBJECTIVE_DEFEND);
        }
    }
}

TEST(BotObjective_CalmGameKeepsBaseRole)
{
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_DEFEND, BOT_FLAG_HOME, BOT_FLAG_HOME, false, false) == BOT_OBJECTIVE_DEFEND);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_HOME, BOT_FLAG_HOME, false, true) == BOT_OBJECTIVE_ASSAULT);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_HOME, BOT_FLAG_HOME, true, false) == BOT_OBJECTIVE_ASSAULT);
}

TEST(BotObjective_OwnFlagAwayOutranksEverything)
{
    // Nearest chases the thief / recovers the drop.
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_DEFEND, BOT_FLAG_CARRIED, BOT_FLAG_HOME, false, true) == BOT_OBJECTIVE_CHASE);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_DROPPED, BOT_FLAG_HOME, false, true) == BOT_OBJECTIVE_RECOVER);
    // Everyone else holds their base role.
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_DEFEND, BOT_FLAG_CARRIED, BOT_FLAG_HOME, false, false) == BOT_OBJECTIVE_DEFEND);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_DROPPED, BOT_FLAG_CARRIED, false, false) == BOT_OBJECTIVE_ASSAULT);
    // Own flag away beats escorting, even for the nearest assaulter.
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_CARRIED, BOT_FLAG_CARRIED, false, true) == BOT_OBJECTIVE_CHASE);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_DROPPED, BOT_FLAG_DROPPED, false, true) == BOT_OBJECTIVE_RECOVER);
}

TEST(BotObjective_EscortAndEnemyRecover)
{
    // Teammate carries it: nearest assaulters escort, defenders hold.
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_HOME, BOT_FLAG_CARRIED, false, true) == BOT_OBJECTIVE_ESCORT);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_HOME, BOT_FLAG_CARRIED, false, false) == BOT_OBJECTIVE_ASSAULT);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_DEFEND, BOT_FLAG_HOME, BOT_FLAG_CARRIED, false, true) == BOT_OBJECTIVE_DEFEND);
    // Carrier dropped it: nearest anyone goes back for it.
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_DEFEND, BOT_FLAG_HOME, BOT_FLAG_DROPPED, false, true) == BOT_OBJECTIVE_RECOVER);
    ASSERT_TRUE(BotObjective::ReassignRole(BOT_OBJECTIVE_ASSAULT, BOT_FLAG_HOME, BOT_FLAG_DROPPED, false, false) == BOT_OBJECTIVE_ASSAULT);
}
