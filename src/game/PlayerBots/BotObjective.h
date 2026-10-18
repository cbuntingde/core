/*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef MANGOS_BOT_OBJECTIVE_H
#define MANGOS_BOT_OBJECTIVE_H

// Objective roles for battleground bots (Warsong Gulch first): what a bot is
// trying to achieve, separate from how it walks there. Pure data and
// arithmetic, no game objects, so unit tests can drive it directly (same
// split as BotStance.h). BattleBotAI embeds one and re-resolves the role
// every tick from live flag state; the waypoint network only supplies paths
// to the role's destination.
//
// Sticky base roles (defend/assault) come from the bot's guid at join time,
// so no roster scan is needed and teammates joining later change nothing.
// Flag events override per tick: chasing your stolen flag and recovering
// your dropped flag outrank everything (nothing can be captured while the
// own flag is away), then escorting your carrier and recovering a dropped
// enemy flag. The carrier itself keeps its base role here because movement
// short-circuits on the flag aura before role routing runs.
enum BotObjectiveRole
{
    BOT_OBJECTIVE_DEFEND,
    BOT_OBJECTIVE_ASSAULT,
    BOT_OBJECTIVE_ESCORT,
    BOT_OBJECTIVE_CHASE,
    BOT_OBJECTIVE_RECOVER,
    BOT_OBJECTIVE_INVALID,
};

// Where one flag is, mapped from the battleground's own state machine.
// GONE covers respawning/unknown: the room is the only sane destination.
enum BotFlagState
{
    BOT_FLAG_HOME,
    BOT_FLAG_CARRIED,
    BOT_FLAG_DROPPED,
    BOT_FLAG_GONE,
};

// Which live point the team converges on, in the same priority order as
// ReassignRole below: the own flag's thief or drop site first, then the
// teammate carrying the enemy flag or its drop site, else nothing.
enum BotObjectiveFocus
{
    BOT_FOCUS_NONE,
    BOT_FOCUS_OWN_KEEPER,
    BOT_FOCUS_OWN_DROP,
    BOT_FOCUS_ENEMY_KEEPER,
    BOT_FOCUS_ENEMY_DROP,
};

struct BotObjective
{
    BotObjective() : baseRole(BOT_OBJECTIVE_INVALID), node(5u) {}

    bool HasBaseRole() const { return baseRole != BOT_OBJECTIVE_INVALID; }
    void Reset() { baseRole = BOT_OBJECTIVE_INVALID; node = 5u; }

    // Sticky Arathi banner from a stable per-bot seed (Arathi always has 5
    // banners). No roster scan, stable across ticks and deaths: the bot
    // routes to its node and fights for it.
    static unsigned NodeFor(unsigned seed) { return seed % 5u; }
    void AssignNode(unsigned seed) { node = NodeFor(seed); }
    bool HasNode() const { return node < 5u; }

    // Sticky split from a stable per-bot seed (guid low part): 2 in 5
    // defend, the rest assault. Deterministic, so roles survive ticks,
    // deaths, and teammates joining later.
    static BotObjectiveRole AssignBaseRole(unsigned seed) { return (seed % 5u) < 2u ? BOT_OBJECTIVE_DEFEND : BOT_OBJECTIVE_ASSAULT; }
    void AssignBase(unsigned seed) { baseRole = AssignBaseRole(seed); }

    // Warsong flag states are 0 on base, 1 respawning, 2 on player,
    // 3 on ground; anything else means gone.
    static BotFlagState MapWsFlagState(unsigned state)
    {
        switch (state)
        {
            case 0: return BOT_FLAG_HOME;
            case 1: return BOT_FLAG_GONE;
            case 2: return BOT_FLAG_CARRIED;
            case 3: return BOT_FLAG_DROPPED;
            default: return BOT_FLAG_GONE;
        }
    }

    // The single contested point for proximity rolls, mirroring the
    // priority in ReassignRole. Game code resolves the focus to a position,
    // rolls nearest once, and passes it in below.
    static BotObjectiveFocus FocusForState(BotFlagState ownFlag, BotFlagState enemyFlag)
    {
        if (ownFlag == BOT_FLAG_CARRIED)
            return BOT_FOCUS_OWN_KEEPER;
        if (ownFlag == BOT_FLAG_DROPPED)
            return BOT_FOCUS_OWN_DROP;
        if (enemyFlag == BOT_FLAG_CARRIED)
            return BOT_FOCUS_ENEMY_KEEPER;
        if (enemyFlag == BOT_FLAG_DROPPED)
            return BOT_FOCUS_ENEMY_DROP;
        return BOT_FOCUS_NONE;
    }

    // Resolve this tick's role. amNearest means this bot is the closest
    // live teammate bot to the contested point (carrier or dropped flag),
    // computed game-side; it spreads chase/recover/escort across the team
    // instead of balling everyone up.
    static BotObjectiveRole ReassignRole(BotObjectiveRole base, BotFlagState ownFlag, BotFlagState enemyFlag, bool amCarrier, bool amNearest)
    {
        if (amCarrier)
            return base;
        // Own flag away outranks everything: chase a carrier, recover a drop.
        if (ownFlag == BOT_FLAG_CARRIED)
            return amNearest ? BOT_OBJECTIVE_CHASE : base;
        if (ownFlag == BOT_FLAG_DROPPED)
            return amNearest ? BOT_OBJECTIVE_RECOVER : base;
        // Teammate carries the enemy flag: nearest assaulters escort.
        if (enemyFlag == BOT_FLAG_CARRIED)
            return (base == BOT_OBJECTIVE_ASSAULT && amNearest) ? BOT_OBJECTIVE_ESCORT : base;
        // Our carrier dropped it: nearest anyone picks it back up.
        if (enemyFlag == BOT_FLAG_DROPPED)
            return amNearest ? BOT_OBJECTIVE_RECOVER : base;
        return base;
    }

    BotObjectiveRole baseRole;
    unsigned node;
};

#endif
