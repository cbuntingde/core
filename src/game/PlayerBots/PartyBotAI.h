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

#ifndef MANGOS_PARTYBOTAI_H
#define MANGOS_PARTYBOTAI_H

#include "BotStance.h"
#include "CombatBotBaseAI.h"
#include "Group.h"
#include "ObjectAccessor.h"

class PartyBotAI : public CombatBotBaseAI
{
public:

    PartyBotAI(Player* pLeader, Player* pClone, CombatBotRoles role, uint8 race, uint8 class_, uint8 level, uint32 mapId, uint32 instanceId, float x, float y, float z, float o)
        : CombatBotBaseAI(), m_race(race), m_class(class_), m_level(level), m_mapId(mapId), m_instanceId(instanceId), m_x(x), m_y(y), m_z(z), m_o(o)
    {
        m_role = role;
        m_leaderGuid = pLeader->GetObjectGuid();
        m_cloneGuid = pClone ? pClone->GetObjectGuid() : ObjectGuid();
        m_updateTimer.Reset(2000);
    }
    PartyBotAI(Player* pLeader, uint32 mapId, uint32 instanceId, float x, float y, float z, float o)
        : CombatBotBaseAI(), m_mapId(mapId), m_instanceId(instanceId), m_x(x), m_y(y), m_z(z), m_o(o)
    {
        m_role = ROLE_INVALID;
        m_leaderGuid = pLeader->GetObjectGuid();
        m_updateTimer.Reset(2000);
    }

    bool OnSessionLoaded(PlayerBotEntry* entry, WorldSession* sess) final;
    void OnPlayerLogin() final;
    void UpdateAI(uint32 const diff) final;
    void OnPacketReceived(WorldPacket const* packet) final;

    void CloneFromPlayer(Player const* pPlayer);
    void AddToPlayerGroup();

    bool CanTryToCastSpell(Unit const* pTarget, SpellEntry const* pSpellEntry) const final;
    Player* GetPartyLeader() const;
    bool AttackStart(Unit* pVictim);
    Unit* SelectAttackTarget(Player* pLeader) const;
    Unit* SelectPartyAttackTarget() const;
    Player* SelectResurrectionTarget() const;
    Player* SelectShieldTarget() const;
    Unit* GetMarkedTarget(RaidTargetIcon mark) const;
    bool CanUseCrowdControl(SpellEntry const* pSpellEntry, Unit const* pTarget) const override;
    bool DrinkAndEat();
    bool HasSafeRecoveryWindow() const;
    bool ShouldAutoRevive() const;
    bool IsValidDistancingTarget(Unit* pTarget, Unit* pEnemy);
    Unit* GetDistancingTarget(Unit* pEnemy);
    bool RunAwayFromTarget(Unit* pEnemy);
    bool SpreadFromGroupmates();
    bool CrowdControlMarkedTargets();
    bool AcceptsTarget(Unit const* pTarget, BotTargetSource source) const;
    void ReturnToHoldAnchor();
    void DropOutOfLeashVictim();
    bool EnterCombatDruidForm();
    bool ShouldEnterStealth() const;
    bool EnterStealthIfNeeded(SpellEntry const* pStealthSpell);

    bool CheckForDispelTargets();
    void UpdateInCombatAI() final;
    void UpdateOutOfCombatAI() final;
    void UpdateInCombatAI_Paladin() final;
    void UpdateOutOfCombatAI_Paladin() final;
    void UpdateInCombatAI_Shaman() final;
    void UpdateOutOfCombatAI_Shaman() final;
    void UpdateInCombatAI_Hunter() final;
    void UpdateOutOfCombatAI_Hunter() final;
    void UpdateInCombatAI_Mage() final;
    void UpdateOutOfCombatAI_Mage() final;
    void UpdateInCombatAI_Priest() final;
    void UpdateOutOfCombatAI_Priest() final;
    void UpdateInCombatAI_Warlock() final;
    void UpdateOutOfCombatAI_Warlock() final;
    void UpdateInCombatAI_Warrior() final;
    void UpdateOutOfCombatAI_Warrior() final;
    void UpdateInCombatAI_Rogue() final;
    void UpdateOutOfCombatAI_Rogue() final;
    void UpdateInCombatAI_Druid() final;
    void UpdateOutOfCombatAI_Druid() final;

    std::vector<RaidTargetIcon> m_marksToCC;
    BotStance m_stance;
    std::vector<RaidTargetIcon> m_marksToFocus;
    ShortTimeTracker m_updateTimer;
    ObjectGuid m_leaderGuid;
    ObjectGuid m_cloneGuid;
    uint8 m_race = 0;
    uint8 m_class = 0;
    uint8 m_level = 0;
    uint32 m_mapId = 0;
    uint32 m_instanceId = 0;
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_z = 0.0f;
    float m_o = 0.0f;
    bool m_resetSpellData = false;
    // Data-driven mage rotation (see BuildMageRotation); movement stays inline.
    BotRotationTable m_rotationMageOpener;
    BotRotationTable m_rotationMageAoE;
    BotRotationTable m_rotationMageCore;
    // Data-driven paladin rotation (see BuildPaladinRotation). Friend-protect
    // heals (dynamic lowest-HP target), the healer branch, Seal upkeep, the
    // DivineFavor+HolyShock pair, movement, Blessing of Freedom and the
    // sub-30% self-heal stay inline.
    BotRotationTable m_rotationPaladinPre;
    BotRotationTable m_rotationPaladinCoreA;
    BotRotationTable m_rotationPaladinCoreB;
    // Data-driven shaman rotation (see BuildShamanRotation). ManaTide,
    // totems, the healer branch and the sub-20% self-heal stay inline.
    BotRotationTable m_rotationShamanCore;
    // Data-driven hunter rotation (see BuildHunterRotation). Volley, Auto
    // Shot, the first-attacker defensive block, melee strikes, aspects and
    // movement stay inline.
    BotRotationTable m_rotationHunterCore;
    // Data-driven priest DPS rotation (see BuildPriestRotation). Defensives,
    // InnerFocus and the healer branch stay inline; the tables bracket
    // chasing and the wand fallback.
    BotRotationTable m_rotationPriestDPSPre;
    BotRotationTable m_rotationPriestDPSPost;
    // Data-driven warlock rotation (see BuildWarlockRotation). Banish, Rain
    // of Fire, Demonic Sacrifice, chasing and the wand fallback stay inline.
    BotRotationTable m_rotationWarlockCoreA;
    BotRotationTable m_rotationWarlockCoreB;
    BotRotationTable m_rotationWarlockCoreC;
    // Data-driven warrior rotation (see BuildWarriorRotation). Stance dance,
    // chasing, the rage-gated Cleave/Heroic Strike pair and the no-victim
    // Battle Shout stay inline.
    BotRotationTable m_rotationWarriorCoreA;
    BotRotationTable m_rotationWarriorCoreB;
    // Data-driven rogue rotation (see BuildRogueRotation). Openers, Vanish,
    // finishers, Blind, singles and movement stay inline.
    BotRotationTable m_rotationRogueInterrupt;
    BotRotationTable m_rotationRogueBuilders;
    // Data-driven druid rotation (see BuildDruidRotation). Form handling,
    // heals, stealth opener, closers and movement stay inline.
    BotRotationTable m_rotationDruidCatFinish;
    BotRotationTable m_rotationDruidCatBuild;
    BotRotationTable m_rotationDruidBearCore;
    BotRotationTable m_rotationDruidBearAoE;
    BotRotationTable m_rotationDruidCaster;
};

#endif
