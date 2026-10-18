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

#ifndef MANGOS_BOT_HEALER_H
#define MANGOS_BOT_HEALER_H

// Triage order for healers: who gets the heal or dispel first. Pure data and
// arithmetic, no game objects, so unit tests can drive it directly (same
// split as BotStance.h). CombatBotBaseAI applies it in SelectHealTarget
// (lowest health wins, self included in the same race instead of always
// first) and SelectDispelTarget (dispel the dying first, tanks break ties).
//
// One rule serves both: threshold checks stay at the call sites (heal
// percent gates, dispel validity), this only orders candidates that already
// passed them. Candidates arrive strictly below 100%, so seeding the race
// with 100% lets the first candidate always win.
struct BotHealer
{
    // True when a candidate should replace the current best pick. Lower
    // health percent wins; on exactly equal health a tank replaces a
    // non-tank. Anything else keeps the incumbent, which keeps the pick
    // stable tick to tick for identical input.
    // Resurrection order: the healer first, then the tank, then the rest.
    // The group recovers fastest when its healer stands up first.
    static unsigned RezRank(bool isHealer, bool isTank)
    {
        return isHealer ? 0u : (isTank ? 1u : 2u);
    }

    static bool WantsTriageTarget(float healthPercent, bool isTank, float bestHealth, bool bestIsTank)
    {
        if (healthPercent < bestHealth)
            return true;
        if (healthPercent == bestHealth && isTank && !bestIsTank)
            return true;
        return false;
    }
};

#endif
