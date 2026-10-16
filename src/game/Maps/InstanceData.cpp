/*
 * Copyright (C) 2005-2011 MaNGOS <http://getmangos.com/>
 * Copyright (C) 2009-2011 MaNGOSZero <https://github.com/mangos/zero>
 * Copyright (C) 2011-2016 Nostalrius <https://nostalrius.org>
 * Copyright (C) 2016-2017 Elysium Project <https://github.com/elysium-project>
 *
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

#include "InstanceData.h"
#include "Database/DatabaseEnv.h"
#include "Map.h"

void InstanceData::SaveToDB()
{
    // no reason to save BGs/Arenas
    if (instance->IsBattleGround())
        return;

    if (!Save())
        return;

    std::string data = Save();

    static SqlStatementID updInstance;
    static SqlStatementID updWorld;

    if (instance->Instanceable())
    {
        SqlStatement stmt = CharacterDatabase.CreateStatement(updInstance, "UPDATE `instance` SET `data` = ? WHERE `id` = ?");
        stmt.addString(data);
        stmt.addUInt32(instance->GetInstanceId());
        stmt.Execute();
    }
    else
    {
        SqlStatement stmt = CharacterDatabase.CreateStatement(updWorld, "UPDATE `world` SET `data` = ? WHERE `map` = ?");
        stmt.addString(data);
        stmt.addUInt32(instance->GetId());
        stmt.Execute();
    }
}

bool InstanceData::CheckConditionCriteriaMeet(Player const* /*player*/, uint32 map_id, WorldObject const* source, uint32 instance_condition_id) const
{
    sLog.Out(LOG_BASIC, LOG_LVL_ERROR, "Condition system call InstanceData::CheckConditionCriteriaMeet but instance script for map %u not have implementation for player condition criteria with internal id %u for map %u",
                  instance->GetId(), instance_condition_id, map_id);
    return false;
}
