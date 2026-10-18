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

#ifndef MANGOS_BOT_ROTATION_H
#define MANGOS_BOT_ROTATION_H

#include <vector>

#include "SpellEntry.h"

class CombatBotBaseAI;
class Unit;

// Which unit a rotation step is checked against and cast on. Resolved at tick
// time: SELF is the bot itself, VICTIM is its current victim,
// OTHER_ATTACKER is another unit attacking the bot
// (SelectAttackerDifferentFrom); a step whose target resolves to null is
// skipped and evaluation continues with the next step.
enum BotRotationTarget
{
    BOT_TARGET_SELF,
    BOT_TARGET_VICTIM,
    BOT_TARGET_OTHER_ATTACKER,
};

// One entry of a spell-priority table. Steps run in order; the first step
// whose spell is known, whose condition holds, and which passes
// CanTryToCastSpell is cast, and the rotation stops for this tick.
// Movement and positioning stay inline in the per-class Update functions,
// never in tables.
struct BotRotationStep
{
    SpellEntry const* spell;            // resolved when the table is built; null entries are skipped
    BotRotationTarget checkTarget;      // unit passed to CanTryToCastSpell
    BotRotationTarget castTarget;       // unit passed to DoCastSpell
    // Optional extra gate beyond CanTryToCastSpell; null means always.
    // Arguments: (ai, victim, checkTarget, spell).
    bool (*condition)(CombatBotBaseAI const* ai, Unit const* victim, Unit const* target, SpellEntry const* spell);
};

using BotRotationTable = std::vector<BotRotationStep>;

#endif
