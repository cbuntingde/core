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

#ifndef MANGOS_BOT_STANCE_H
#define MANGOS_BOT_STANCE_H

// Leader-control stance for party bots: where to stand and how eager to
// fight. Pure data and arithmetic, no game objects, so unit tests can drive
// it directly (same split as BotTelemetry.h). PartyBotAI embeds one and the
// `.partybot follow|hold|stay|assist` commands mutate it.
//
// Two independent axes, so every combination is reachable:
//   follow [close|far] - trail the leader again (clears hold and passive)
//   hold               - anchor at the current spot, keep aggression
//   stay               - anchor AND go passive (self-defense plus explicit
//                        orders only)
//   assist             - leave passive mode, keep the anchor (so stay then
//                        assist holds position but helps again)
static float const BOT_FOLLOW_CLOSE_MIN_DIST = 3.0f;
static float const BOT_FOLLOW_CLOSE_MAX_DIST = 6.0f;
static float const BOT_FOLLOW_FAR_MIN_DIST = 12.0f;
static float const BOT_FOLLOW_FAR_MAX_DIST = 18.0f;

// Targets farther than this from a held bot's anchor are ignored, and a
// current victim dragged past it is dropped. Keeps held bots from chasing
// across the dungeon after the leader's pull.
static float const BOT_HOLD_LEASH_DIST = 30.0f;

// Closer than this to the anchor counts as back in position.
static float const BOT_HOLD_ANCHOR_TOLERANCE = 2.0f;

// One `.partybot` stance order. Kept here (no game types) so command
// handlers and the AI share the same vocabulary.
enum PartyBotStanceOrder
{
    PARTY_BOT_ORDER_FOLLOW,
    PARTY_BOT_ORDER_HOLD,
    PARTY_BOT_ORDER_STAY,
    PARTY_BOT_ORDER_ASSIST,
};

// followFar values for PARTY_BOT_ORDER_FOLLOW: keep the bot's distance,
// or switch to close/far.
static int const PARTY_BOT_FOLLOW_KEEP = -1;
static int const PARTY_BOT_FOLLOW_CLOSE = 0;
static int const PARTY_BOT_FOLLOW_FAR = 1;

// Where an auto-acquired target came from. Explicit leader orders (duel,
// focus marks, direct attack orders) and self/pet defense always apply;
// the assist-only sources are skipped while passive.
enum BotTargetSource
{
    BOT_TARGET_DUEL,
    BOT_TARGET_FOCUS_MARK,
    BOT_TARGET_LEADER_VICTIM,
    BOT_TARGET_PARTY_ATTACKER,
    BOT_TARGET_SELF_ATTACKER,
    BOT_TARGET_PET_ATTACKER,
};

struct BotStance
{
    BotStance() : holdPosition(false), passive(false), followFar(false),
        holdX(0.0f), holdY(0.0f), holdZ(0.0f) {}

    // Orders. Follow resumes everything; Assist only leaves passive mode.
    void Follow(bool useFar)
    {
        holdPosition = false;
        passive = false;
        followFar = useFar;
    }
    void ResumeFollow() { holdPosition = false; passive = false; }
    void ClearHold() { holdPosition = false; }
    void Hold(float x, float y, float z)
    {
        holdPosition = true;
        holdX = x; holdY = y; holdZ = z;
    }
    void Stay(float x, float y, float z) { Hold(x, y, z); passive = true; }
    void Assist() { passive = false; }

    bool IsHolding() const { return holdPosition; }
    bool IsPassive() const { return passive; }

    float FollowMinDist() const { return followFar ? BOT_FOLLOW_FAR_MIN_DIST : BOT_FOLLOW_CLOSE_MIN_DIST; }
    float FollowMaxDist() const { return followFar ? BOT_FOLLOW_FAR_MAX_DIST : BOT_FOLLOW_CLOSE_MAX_DIST; }

    static bool WantsTargetSource(bool passiveStance, BotTargetSource source)
    {
        switch (source)
        {
            case BOT_TARGET_LEADER_VICTIM:
            case BOT_TARGET_PARTY_ATTACKER:
                return !passiveStance;
            default:
                return true;
        }
    }

    static float DistSq(float ax, float ay, float az, float bx, float by, float bz)
    {
        float const dx = ax - bx;
        float const dy = ay - by;
        float const dz = az - bz;
        return dx * dx + dy * dy + dz * dz;
    }

    // True when holding and the point is past the anchor leash. Always
    // false when following, so call sites can check unconditionally.
    bool IsOutOfLeash(float x, float y, float z) const
    {
        if (!holdPosition)
            return false;
        return DistSq(x, y, z, holdX, holdY, holdZ) > BOT_HOLD_LEASH_DIST * BOT_HOLD_LEASH_DIST;
    }

    bool IsAtAnchor(float x, float y, float z) const
    {
        return DistSq(x, y, z, holdX, holdY, holdZ) <= BOT_HOLD_ANCHOR_TOLERANCE * BOT_HOLD_ANCHOR_TOLERANCE;
    }

    bool holdPosition;
    bool passive;
    bool followFar;
    float holdX;
    float holdY;
    float holdZ;
};

#endif
