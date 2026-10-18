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

#ifndef MANGOS_BOT_PERF_H
#define MANGOS_BOT_PERF_H

// Performance tiers for bot ticks. Pure data and arithmetic, no game
// objects, so unit tests can drive it directly (same split as BotStance.h).
// In combat and near the action every bot thinks full-rate; out of combat,
// bots far from their leader stretch their tick so a crowd of idle bots
// costs a fraction of the updates. Different intervals drift apart on
// their own, which also spreads the per-tick load without any scheduler.
struct BotPerf
{
    // Out-of-combat tick stride by leader distance: near the party everything
    // stays responsive, far away (across the zone) scans run at one third.
    static unsigned OutOfCombatStride(float leaderDistYd)
    {
        if (leaderDistYd < 30.0f)
            return 1u;
        if (leaderDistYd < 100.0f)
            return 2u;
        return 3u;
    }
};

#endif
