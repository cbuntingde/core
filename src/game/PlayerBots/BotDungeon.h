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

#ifndef MANGOS_BOT_DUNGEON_H
#define MANGOS_BOT_DUNGEON_H

#include <cmath>

// Dungeon awareness, first slice: spread out. Pure geometry, no game
// objects, so unit tests can drive it directly (same split as BotStance.h).
// Ranged bots that stack on one spot eat every cleave together; when a
// groupmate stands too close, the bot steps away to a healthy distance.
// Tanks and melee are excluded game-side: they stack on the boss by design.
struct BotDungeon
{
    // Closer than this to a groupmate counts as stacked.
    static float constexpr SPREAD_TRIGGER_DIST = 5.0f;
    // Step away until this far apart.
    static float constexpr SPREAD_TARGET_DIST = 8.0f;

    // Destination that restores TARGET distance from a friend standing too
    // close: step directly away from them. Zero distance (same spot) falls
    // back to +X so the result is always defined.
    static void SpreadDestination(float meX, float meY, float friendX, float friendY, float range, float& outX, float& outY)
    {
        float const dx = meX - friendX;
        float const dy = meY - friendY;
        float const dist = std::sqrt(dx * dx + dy * dy);
        if (dist < 0.001f)
        {
            outX = meX + range;
            outY = meY;
            return;
        }
        float const step = range - dist;
        outX = meX + dx / dist * step;
        outY = meY + dy / dist * step;
    }
};

#endif
