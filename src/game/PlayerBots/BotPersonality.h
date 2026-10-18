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

#ifndef MANGOS_BOT_PERSONALITY_H
#define MANGOS_BOT_PERSONALITY_H

#include <cstring>

// Skill profile for combat bots: how fast a bot reacts and how eagerly it
// spends interrupts, dispels and field consumables (food/drink between
// pulls). Pure data and arithmetic, no game objects, so unit tests can drive
// it directly (same split as BotStance.h). CombatBotBaseAI embeds one and the
// `.partybot skill` command mutates it.
//
// Three profiles, reachable via `.partybot skill casual|normal|sharp`:
//   casual - relaxed dungeon companion: slower ticks, often lets enemy casts
//            and removable debuffs through, only eats/drinks when low.
//   normal - the default: human-paced ticks, usually interrupts and dispels,
//            tops up between pulls.
//   sharp  - legacy full eagerness: never deliberately skips an interrupt or
//            dispel it could land.
//
// Sharp means better decisions, not faster packets: its tick is capped at the
// same 1s human baseline as normal, so it never reacts faster than a player
// could. Battle bots share the base class and its interrupt conds, so they
// roll the normal profile too (they have no skill command yet).
enum BotSkill
{
    BOT_SKILL_CASUAL,
    BOT_SKILL_NORMAL,
    BOT_SKILL_SHARP,
    BOT_SKILL_INVALID,
};

struct BotPersonality
{
    BotPersonality() : skill(BOT_SKILL_NORMAL) {}

    void Set(BotSkill newSkill) { skill = newSkill; }

    static BotSkill FromString(char const* name)
    {
        if (!name)
            return BOT_SKILL_INVALID;
        if (std::strcmp(name, "casual") == 0)
            return BOT_SKILL_CASUAL;
        if (std::strcmp(name, "normal") == 0)
            return BOT_SKILL_NORMAL;
        if (std::strcmp(name, "sharp") == 0)
            return BOT_SKILL_SHARP;
        return BOT_SKILL_INVALID;
    }

    static char const* ToString(BotSkill value)
    {
        switch (value)
        {
            case BOT_SKILL_CASUAL: return "casual";
            case BOT_SKILL_NORMAL: return "normal";
            case BOT_SKILL_SHARP: return "sharp";
            default: return "unknown";
        }
    }

    // Milliseconds between AI ticks. Casual thinks slower; sharp is capped
    // at the normal baseline so skill never buys inhuman reaction speed.
    unsigned TickIntervalMs() const { return skill == BOT_SKILL_CASUAL ? 1500u : 1000u; }

    // Chance (0-100) to contest an enemy cast this tick when an interrupt is
    // otherwise ready. Normal usually tries, casual often lets it through,
    // sharp never deliberately skips.
    unsigned InterruptAttemptPercent() const
    {
        switch (skill)
        {
            case BOT_SKILL_CASUAL: return 40u;
            case BOT_SKILL_SHARP: return 100u;
            default: return 75u;
        }
    }

    // Chance (0-100) to attempt a dispel this tick when a removable aura is
    // otherwise found. Same shape as interrupts: casual lazy, sharp perfect.
    unsigned DispelAttemptPercent() const
    {
        switch (skill)
        {
            case BOT_SKILL_CASUAL: return 25u;
            case BOT_SKILL_SHARP: return 100u;
            default: return 60u;
        }
    }

    // Health/mana percent below which the bot eats and drinks between pulls.
    // Normal and sharp top up whenever not full (legacy behavior); casual
    // conserves consumables until actually low.
    unsigned RecoveryThresholdPercent() const { return skill == BOT_SKILL_CASUAL ? 60u : 100u; }

    // Pure gate shared by interrupt and dispel rolls: true when a 0-99 roll
    // falls inside the attempt chance. Percent is clamped so callers can pass
    // raw profile values without sanitizing first.
    static bool WantsAttempt(unsigned percent, unsigned roll0_99)
    {
        return roll0_99 < (percent > 100u ? 100u : percent);
    }

    BotSkill skill;
};

#endif
