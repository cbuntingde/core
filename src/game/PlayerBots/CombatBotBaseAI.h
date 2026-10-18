#ifndef MANGOS_COMBAT_BOT_BASE_H
#define MANGOS_COMBAT_BOT_BASE_H

#include "PlayerBotAI.h"
#include "BotRotation.h"
#include "SpellEntry.h"
#include "Player.h"

struct HealSpellCompare
{
    bool operator() (SpellEntry const* const lhs, SpellEntry const* const rhs) const
    {
        uint32 spell1dmg = 0;
        uint32 spell2dmg = 0;

        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            switch (lhs->Effect[i])
            {
                case SPELL_EFFECT_HEAL:
                    spell1dmg += lhs->EffectBasePoints[i];
                    break;
            }
        }
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            switch (rhs->Effect[i])
            {
                case SPELL_EFFECT_HEAL:
                    spell2dmg += rhs->EffectBasePoints[i];
                    break;
            }
        }

        return spell1dmg > spell2dmg;
    }
};

struct HealAuraCompare
{
    bool operator() (SpellEntry const* const lhs, SpellEntry const* const rhs) const
    {
        uint32 spell1dmg = 0;
        uint32 spell2dmg = 0;

        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            switch (lhs->Effect[i])
            {
                case SPELL_EFFECT_APPLY_AURA:
                case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                    if (lhs->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_HEAL)
                        spell1dmg += lhs->EffectBasePoints[i];
                    break;
            }
        }
        for (uint32 i = 0; i < MAX_SPELL_EFFECTS; i++)
        {
            switch (rhs->Effect[i])
            {
                case SPELL_EFFECT_APPLY_AURA:
                case SPELL_EFFECT_PERSISTENT_AREA_AURA:
                case SPELL_EFFECT_APPLY_AREA_AURA_PARTY:
                    if (rhs->EffectApplyAuraName[i] == SPELL_AURA_PERIODIC_HEAL)
                        spell2dmg += rhs->EffectBasePoints[i];
                    break;
            }
        }

        return spell1dmg > spell2dmg;
    }
};

class CombatBotBaseAI : public PlayerBotAI
{
public:

    CombatBotBaseAI() : PlayerBotAI(nullptr)
    {
        for (auto& ptr : m_spells.raw.spells)
            ptr = nullptr;
    }

    virtual void OnPacketReceived(WorldPacket const* packet) override;
    void SendBattlefieldPortPacket();
    void SendBattlemasterJoinPacket(uint8 battlegroundId);
    void SendAreaTriggerPacket(uint32 areaTriggerId);
    void ActivateNearbyAreaTrigger();

    void AutoAssignRole();
    void PopulateSpellData();
    void ResetSpellData();
    void AddAllSpellReagents();
    void SummonPetIfNeeded();
    void LearnArmorProficiencies();
    void LearnPremadeSpecForClass();
    void EquipPremadeGearTemplate();
    void EquipRandomGearInEmptySlots();
    void AutoEquipGear(uint32 option);
    void LearnRandomTalents();
    
    uint8 GetAttackersInRangeCount(float range) const;
    Unit* SelectAttackerDifferentFrom(Unit const* pExcept) const;
    Unit* SelectHealTarget(float selfHealPercent = 100.0f, float groupHealPercent = 100.0f) const;
    Unit* SelectPeriodicHealTarget(float selfHealPercent = 100.0f, float groupHealPercent = 100.0f) const;
    Player* SelectBuffTarget(SpellEntry const* pSpellEntry) const;
    Player* SelectBuffTarget(SpellEntry const* pSingleSpellEntry, SpellEntry const* pGroupSpellEntry, SpellEntry const*& pSelectedSpellEntry) const;
    Player* SelectDispelTarget(SpellEntry const* pSpellEntry) const;
    bool IsValidBuffTarget(Unit const* pTarget, SpellEntry const* pSpellEntry) const;
    bool IsValidHealTarget(Unit const* pTarget, float healthPercent = 100.0f) const;
    bool IsValidHostileTarget(Unit const* pTarget) const;
    bool IsValidDispelTarget(Unit const* pTarget, SpellEntry const* pSpellEntry) const;
    bool FindAndPreHealTarget();
    bool FindAndHealInjuredAlly(float selfHealPercent = 100.0f, float groupHealPercent = 100.0f);
    bool HealInjuredTarget(Unit* pTarget);
    bool HealInjuredTargetDirect(Unit* pTarget);
    bool HealInjuredTargetPeriodic(Unit* pTarget);
    template <class T>
    SpellEntry const* SelectMostEfficientHealingSpell(Unit const* pTarget, std::set<SpellEntry const*, T>& spellList) const;
    template <class T>
    SpellEntry const* SelectMostEfficientHealingSpell(Unit const* pTarget, int32 missingHealth, std::set<SpellEntry const*, T>& spellList) const;
    int32 GetIncomingdamage(Unit const* pTarget) const;
    bool AreOthersOnSameTarget(ObjectGuid guid, bool checkMelee = true, bool checkSpells = true) const;

    SpellCastResult DoCastSpell(Unit* pTarget, SpellEntry const* pSpellEntry);
    virtual bool CanTryToCastSpell(Unit const* pTarget, SpellEntry const* pSpellEntry) const;
    virtual bool CanUseCrowdControl(SpellEntry const* /*pSpellEntry*/, Unit const* /*pTarget*/) const { return true; }

    // Data-driven rotation framework. Rotations are ordered spell-priority
    // tables built from m_spells and evaluated by RunRotation; movement and
    // positioning stay inline in the per-class Update functions. Tables are
    // rebuilt whenever spell data is (re)populated, flagged by m_rotationDirty.
    // Returns true when a spell was cast and the caller should end its tick.
    bool RunRotation(Unit* pVictim, BotRotationTable const& rotation);
    void BuildMageRotation(BotRotationTable& opener, BotRotationTable& aoe, BotRotationTable& core);
    static bool MageCond_PyroblastOpener(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool MageCond_IceBlock(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool MageCond_MageAoE(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool MageCond_ConeOfCold(CombatBotBaseAI const* ai, Unit const* victim, Unit const* target, SpellEntry const* spell);
    static bool MageCond_Counterspell(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool MageCond_Blizzard(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool MageCond_Polymorph(CombatBotBaseAI const* ai, Unit const*, Unit const* target, SpellEntry const* spell);
    static bool MageCond_ArcanePower(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool MageCond_PresenceOfMind(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool MageCond_Scorch(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool MageCond_Evocation(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    // Paladin (Party) rotation tables: pre covers the attacker-gated
    // defensives after friend-protect; coreA/coreB bracket the inline
    // DivineFavor+HolyShock pair so the original cast order is preserved.
    // DivineShield (lone leading block), friend-protect (dynamic target),
    // the healer branch, Seal upkeep, movement and Freedom stay inline.
    void BuildPaladinRotation(BotRotationTable& pre, BotRotationTable& coreA, BotRotationTable& coreB);
    static bool PaladinCond_HolyShield(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool PaladinCond_TurnEvil(CombatBotBaseAI const* ai, Unit const*, Unit const* target, SpellEntry const*);
    static bool PaladinCond_Judgement(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool PaladinCond_HammerOfJustice(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool PaladinCond_HammerOfWrath(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool PaladinCond_Consecration(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool PaladinCond_Exorcism(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool PaladinCond_HolyWrath(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    // Shaman (Party) rotation: ManaTide (lone, role-independent) stays
    // inline; the table covers the non-healer DPS chain. Totems and the
    // healer branch stay inline.
    void BuildShamanRotation(BotRotationTable& core);
    static bool ShamanCond_ElementalMastery(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool ShamanCond_EarthShock(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool ShamanCond_FrostShock(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool ShamanCond_Purge(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const* spell);
    static bool ShamanCond_LightningBolt(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    // Hunter (Party) rotation: Volley (lone AoE), Auto Shot (ammo handling),
    // the first-attacker defensive block, melee strikes and movement stay
    // inline; the table covers the ranged shot chain.
    void BuildHunterRotation(BotRotationTable& core);
    static bool HunterCond_ConcussiveShot(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    // Priest (Party) rotation: self-shield, Fade/Shackle (first attacker),
    // InnerFocus (non-blocking) and the healer branch (dynamic targets) stay
    // inline; dpsPre/dpsPost bracket inline chasing and the wand fallback.
    void BuildPriestRotation(BotRotationTable& dpsPre, BotRotationTable& dpsPost);
    static bool PriestCond_Silence(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool PriestCond_PsychicScream(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool PriestCond_ManaBurn(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool PriestCond_MindFlay(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool PriestCond_HolyNova(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    // Warlock (Party) rotation: Banish (first attacker), Rain of Fire (lone
    // AoE), Demonic Sacrifice (pet target), chasing and the wand fallback
    // stay inline; coreA/coreB/coreC preserve the original cast order.
    void BuildWarlockRotation(BotRotationTable& coreA, BotRotationTable& coreB, BotRotationTable& coreC);
    static bool WarlockCond_DeathCoil(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarlockCond_Shadowburn(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarlockCond_SearingPain(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarlockCond_SiphonLife(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarlockCond_DrainLife(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarlockCond_Fear(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarlockCond_HowlOfTerror(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarlockCond_LifeTap(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    // Warrior (Party) rotation: stance dance (non-blocking), chasing, the
    // rage-gated Cleave/Heroic Strike pair and the no-victim Battle Shout
    // stay inline; coreA/coreB preserve the original cast order.
    void BuildWarriorRotation(BotRotationTable& coreA, BotRotationTable& coreB);
    static bool WarriorCond_Pummel(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_ShieldBash(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_Execute(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_LastStand(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_ConcussionBlow(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_ShieldBlock(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_ShieldWall(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_ShieldSlam(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_TankSpell(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_Hamstring(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_IntimidatingShout(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_Retaliation(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool WarriorCond_SweepingStrikes(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_Recklessness(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool WarriorCond_Disarm(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    // Rogue (Party) rotation: stealth opener (form-gated branches), Vanish
    // (movement side effect), random combo finishers, Blind (re-target side
    // effects), Adrenaline/ColdBlood/Sprint singles and Evasion/Blade Flurry
    // stay inline; the tables cover interrupts and combo builders.
    void BuildRogueRotation(BotRotationTable& interrupt, BotRotationTable& builders);
    static bool RogueCond_Interrupt(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    // Druid (Party) rotation: Barkskin, Hibernate, heals, Innervate, form
    // changes, stealth opener, Cower, range closers, Maul, Faerie Fire,
    // Insect Swarm, root-breaks and all movement stay inline; the tables
    // cover the per-form damage chains.
    void BuildDruidRotation(BotRotationTable& catFinish, BotRotationTable& catBuild,
        BotRotationTable& bearCore, BotRotationTable& bearAoE, BotRotationTable& caster);
    static bool DruidCond_ComboPoints(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool DruidCond_FrenziedRegeneration(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool DruidCond_BearAoE(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool DruidCond_Hurricane(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool DruidCond_Starfire(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    // Battle (BG) rotations. BG bots use different gates (flag awareness, no
    // group-heal role split) and different orders than Party bots, so each
    // Battle table is derived from BattleBotAI's own logic, reusing Party
    // conditions only where the gate is identical.
    void BuildBattlePaladinRotation(BotRotationTable& coreA, BotRotationTable& coreB);
    static bool BattlePaladinCond_Judgement(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool BattlePaladinCond_HammerOfJustice(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattlePaladinCond_HolyShield(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    void BuildBattleShamanRotation(BotRotationTable& core);
    static bool BattleShamanCond_LightningBolt(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    // Battle Mage: same engine as Party mage except the opener (Presence of
    // Mind aura only), Mana Shield (physical attackers only), the AoE gate
    // (attackers, not nearby enemies), no Blizzard, plus a self-dispel.
    // Polymorph intentionally targets the add (OTHER_ATTACKER): the old
    // inline code selected an add then cast on the victim, dropping the
    // target; the table casts on the selected add.
    void BuildBattleMageRotation(BotRotationTable& opener, BotRotationTable& aoe, BotRotationTable& core);
    static bool BattleMageCond_Pyroblast(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool BattleMageCond_ManaShield(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleMageCond_AoE(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    // Battle Priest reuses BuildPriestRotation for its identical DPS chain;
    // only its support prelude (heals/dispels for all, no Fade/Shackle) stays
    // inline and differs.
    // Battle Warlock: Shadow Ward over Banish/Rain of Fire, unconditional
    // Fear, caster-split curses over Curse of Agony; pet sacrifice, chasing
    // and wand stay inline.
    void BuildBattleWarlockRotation(BotRotationTable& coreA, BotRotationTable& coreB, BotRotationTable& coreC);
    static bool BattleWarlockCond_ShadowWard(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarlockCond_CurseOfTongues(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarlockCond_CurseOfExhaustion(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    // Battle Warrior: physical-gated shield wall, snare pair, rogue-only
    // Rend, caster-hunting cooldowns with Berserker Rage, victim-targeted
    // Whirlwind and a lone low-rage Heroic Strike; stance dance, chasing and
    // the no-victim Battle Shout stay inline.
    void BuildBattleWarriorRotation(BotRotationTable& coreA, BotRotationTable& coreB);
    static bool BattleWarriorCond_ShieldBlock(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_ShieldWall(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_PiercingHowl(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_Rend(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_IntimidatingShout(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_Retaliation(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleWarriorCond_Recklessness(CombatBotBaseAI const* ai, Unit const* victim, Unit const*, SpellEntry const*);
    // Battle Rogue reuses BuildRogueRotation (identical interrupt/builder
    // chains); its opener escape, finishers, Blind gate and Sprint differ and
    // stay inline.
    // Battle Druid: no Cower, melee-gated Demoralizing Roar, low-rage Swipe,
    // no Hurricane; form handling, heals and movement stay inline.
    void BuildBattleDruidRotation(BotRotationTable& catFinish, BotRotationTable& catBuild,
        BotRotationTable& bearCore, BotRotationTable& bearAoE, BotRotationTable& caster);
    static bool BattleDruidCond_DemoralizingRoar(CombatBotBaseAI const*, Unit const* victim, Unit const*, SpellEntry const*);
    static bool BattleDruidCond_Swipe(CombatBotBaseAI const* ai, Unit const*, Unit const*, SpellEntry const*);
    bool IsWearingShield(Player* pPlayer) const;
    bool IsInDuel() const;
    CombatBotRoles GetRole() const;

    void EquipOrUseNewItem();
    void AddItemToInventory(uint32 itemId, uint32 count = 1);
    void AddHunterAmmo();
    uint8 GetHighestHonorRankFromEquippedItems() const;
    void UpdateVisualHonorRankBasedOnItems();
    void BeginChasing(Unit* pVictim) const;
    bool SummonShamanTotems();
    SpellCastResult CastWeaponBuff(SpellEntry const* pSpellEntry, EquipmentSlots slot);
    bool UseTrinketEffects(bool onlyToBreakCC = false);
    bool UseItemEffect(Item* pItem, bool onlyToBreakCC = false);
    void BreakCrowdControlEffects();

    virtual void UpdateInCombatAI() = 0;
    virtual void UpdateOutOfCombatAI() = 0;
    virtual void UpdateInCombatAI_Paladin() = 0;
    virtual void UpdateOutOfCombatAI_Paladin() = 0;
    virtual void UpdateInCombatAI_Shaman() = 0;
    virtual void UpdateOutOfCombatAI_Shaman() = 0;
    virtual void UpdateInCombatAI_Hunter() = 0;
    virtual void UpdateOutOfCombatAI_Hunter() = 0;
    virtual void UpdateInCombatAI_Mage() = 0;
    virtual void UpdateOutOfCombatAI_Mage() = 0;
    virtual void UpdateInCombatAI_Priest() = 0;
    virtual void UpdateOutOfCombatAI_Priest() = 0;
    virtual void UpdateInCombatAI_Warlock() = 0;
    virtual void UpdateOutOfCombatAI_Warlock() = 0;
    virtual void UpdateInCombatAI_Warrior() = 0;
    virtual void UpdateOutOfCombatAI_Warrior() = 0;
    virtual void UpdateInCombatAI_Rogue() = 0;
    virtual void UpdateOutOfCombatAI_Rogue() = 0;
    virtual void UpdateInCombatAI_Druid() = 0;
    virtual void UpdateOutOfCombatAI_Druid() = 0;

    static bool IsPhysicalDamageClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_ROGUE:
            case CLASS_HUNTER:
            case CLASS_SHAMAN:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }
    static bool IsRangedDamageClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_HUNTER:
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
            case CLASS_MAGE:
            case CLASS_WARLOCK:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }
    static bool IsMeleeDamageClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_ROGUE:
            case CLASS_SHAMAN:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }
    static bool IsMeleeWeaponClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_ROGUE:
            case CLASS_SHAMAN:
                return true;
        }
        return false;
    }
    static bool IsShieldClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_SHAMAN:
                return true;
        }
        return false;
    }
    static bool IsTankClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_WARRIOR:
            case CLASS_PALADIN:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }
    static bool IsHealerClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_PALADIN:
            case CLASS_PRIEST:
            case CLASS_SHAMAN:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }
    static bool IsStealthClass(uint8 playerClass)
    {
        switch (playerClass)
        {
            case CLASS_ROGUE:
            case CLASS_DRUID:
                return true;
        }
        return false;
    }

    SpellEntry const* GetCrowdControlSpell() const
    {
        switch (me->GetClass())
        {
            case CLASS_PALADIN:
                return m_spells.paladin.pHammerOfJustice;
            case CLASS_MAGE:
                return m_spells.mage.pPolymorph;
            case CLASS_PRIEST:
                return m_spells.priest.pShackleUndead;
            case CLASS_WARLOCK:
                return m_spells.warlock.pBanish;
            case CLASS_ROGUE:
                return m_spells.rogue.pBlind;
            case CLASS_DRUID:
                return m_spells.druid.pHibernate;
        }
        return nullptr;
    }

    SpellEntry const* m_resurrectionSpell = nullptr;
    std::vector<SpellEntry const*> m_spellListTaunt;
    std::set<SpellEntry const*, HealAuraCompare> m_spellListPeriodicHeal;
    std::set<SpellEntry const*, HealSpellCompare> m_spellListDirectHeal;
    union
    {
        struct
        {
            SpellEntry const* spells[45];
        } raw;
        struct
        {
            SpellEntry const* pAura;
            SpellEntry const* pSeal;
            SpellEntry const* pBlessingBuff;
            SpellEntry const* pBlessingOfProtection;
            SpellEntry const* pBlessingOfFreedom;
            SpellEntry const* pBlessingOfSacrifice;
            SpellEntry const* pHammerOfJustice;
            SpellEntry const* pJudgement;
            SpellEntry const* pExorcism;
            SpellEntry const* pConsecration;
            SpellEntry const* pHammerOfWrath;
            SpellEntry const* pCleanse;
            SpellEntry const* pDivineShield;
            SpellEntry const* pLayOnHands;
            SpellEntry const* pRighteousFury;
            SpellEntry const* pHolyShock;
            SpellEntry const* pDivineFavor;
            SpellEntry const* pHolyWrath;
            SpellEntry const* pTurnEvil;
            SpellEntry const* pHolyShield;
        } paladin;
        struct
        {
            SpellEntry const* pLightningBolt;
            SpellEntry const* pChainLightning;
            SpellEntry const* pEarthShock;
            SpellEntry const* pFlameShock;
            SpellEntry const* pFrostShock;
            SpellEntry const* pPurge;
            SpellEntry const* pStormstrike;
            SpellEntry const* pElementalMastery;
            SpellEntry const* pLightningShield;
            SpellEntry const* pGhostWolf;
            SpellEntry const* pCureDisease;
            SpellEntry const* pCurePoison;
            SpellEntry const* pAirTotem;
            SpellEntry const* pEarthTotem;
            SpellEntry const* pFireTotem;
            SpellEntry const* pWaterTotem;
            SpellEntry const* pManaTideTotem;
            SpellEntry const* pWeaponBuff;
        } shaman;
        struct
        {
            SpellEntry const* pAspectOfTheCheetah;
            SpellEntry const* pAspectOfTheMonkey;
            SpellEntry const* pAspectOfTheHawk;
            SpellEntry const* pSerpentSting;
            SpellEntry const* pArcaneShot;
            SpellEntry const* pAimedShot;
            SpellEntry const* pMultiShot;
            SpellEntry const* pConcussiveShot;
            SpellEntry const* pWingClip;
            SpellEntry const* pHuntersMark;
            SpellEntry const* pMongooseBite;
            SpellEntry const* pRaptorStrike;
            SpellEntry const* pDisengage;
            SpellEntry const* pFeignDeath;
            SpellEntry const* pScareBeast;
            SpellEntry const* pVolley;
        } hunter;
        struct
        {
            SpellEntry const* pIceArmor;
            SpellEntry const* pArcaneIntellect;
            SpellEntry const* pArcaneBrilliance;
            SpellEntry const* pIceBarrier;
            SpellEntry const* pManaShield;
            SpellEntry const* pPolymorph;
            SpellEntry const* pFrostbolt;
            SpellEntry const* pFireBlast;
            SpellEntry const* pFireball;
            SpellEntry const* pArcaneExplosion;
            SpellEntry const* pFrostNova;
            SpellEntry const* pConeofCold;
            SpellEntry const* pBlink;
            SpellEntry const* pCounterspell;
            SpellEntry const* pPresenceOfMind;
            SpellEntry const* pArcanePower;
            SpellEntry const* pRemoveLesserCurse;
            SpellEntry const* pScorch;
            SpellEntry const* pPyroblast;
            SpellEntry const* pEvocation;
            SpellEntry const* pIceBlock;
            SpellEntry const* pBlizzard;
            SpellEntry const* pBlastWave;
            SpellEntry const* pCombustion;
        } mage;
        struct
        {
            SpellEntry const* pPowerWordFortitude;
            SpellEntry const* pDivineSpirit;
            SpellEntry const* pPrayerofSpirit;
            SpellEntry const* pPrayerofFortitude;
            SpellEntry const* pPrayerofShadowProtection;
            SpellEntry const* pInnerFire;
            SpellEntry const* pShadowProtection;
            SpellEntry const* pPowerWordShield;
            SpellEntry const* pHolyNova;
            SpellEntry const* pHolyFire;
            SpellEntry const* pMindBlast;
            SpellEntry const* pMindFlay;
            SpellEntry const* pShadowWordPain;
            SpellEntry const* pInnerFocus;
            SpellEntry const* pAbolishDisease;
            SpellEntry const* pDispelMagic;
            SpellEntry const* pManaBurn;
            SpellEntry const* pDevouringPlague;
            SpellEntry const* pPsychicScream;
            SpellEntry const* pShadowform;
            SpellEntry const* pVampiricEmbrace;
            SpellEntry const* pSilence;
            SpellEntry const* pFade;
            SpellEntry const* pShackleUndead;
            SpellEntry const* pSmite;
        } priest;
        struct
        {
            SpellEntry const* pDemonArmor;
            SpellEntry const* pDeathCoil;
            SpellEntry const* pDetectInvisibility;
            SpellEntry const* pShadowWard;
            SpellEntry const* pShadowBolt;
            SpellEntry const* pCorruption;
            SpellEntry const* pConflagrate;
            SpellEntry const* pShadowburn;
            SpellEntry const* pSearingPain;
            SpellEntry const* pImmolate;
            SpellEntry const* pRainOfFire;
            SpellEntry const* pDemonicSacrifice;
            SpellEntry const* pDrainLife;
            SpellEntry const* pSiphonLife;
            SpellEntry const* pBanish;
            SpellEntry const* pFear;
            SpellEntry const* pHowlofTerror;
            SpellEntry const* pCurseofAgony;
            SpellEntry const* pCurseofDoom;
            SpellEntry const* pCurseoftheElements;
            SpellEntry const* pCurseofShadow;
            SpellEntry const* pCurseofRecklessness;
            SpellEntry const* pCurseofTongues;
            SpellEntry const* pCurseofExhaustion;
            SpellEntry const* pLifeTap;
        } warlock;
        struct
        {
            SpellEntry const* pBattleStance;
            SpellEntry const* pBerserkerStance;
            SpellEntry const* pDefensiveStance;
            SpellEntry const* pCharge;
            SpellEntry const* pIntercept;
            SpellEntry const* pOverpower;
            SpellEntry const* pHeroicStrike;
            SpellEntry const* pCleave;
            SpellEntry const* pExecute;
            SpellEntry const* pMortalStrike;
            SpellEntry const* pBloodthirst;
            SpellEntry const* pBloodrage;
            SpellEntry const* pBerserkerRage;
            SpellEntry const* pRecklessness;
            SpellEntry const* pRetaliation;
            SpellEntry const* pDeathWish;
            SpellEntry const* pIntimidatingShout;
            SpellEntry const* pPummel;
            SpellEntry const* pRend;
            SpellEntry const* pDisarm;
            SpellEntry const* pWhirlwind;
            SpellEntry const* pBattleShout;
            SpellEntry const* pDemoralizingShout;
            SpellEntry const* pHamstring;
            SpellEntry const* pThunderClap;
            SpellEntry const* pSweepingStrikes;
            SpellEntry const* pLastStand;
            SpellEntry const* pShieldBlock;
            SpellEntry const* pShieldWall;
            SpellEntry const* pShieldBash;
            SpellEntry const* pShieldSlam;
            SpellEntry const* pSunderArmor;
            SpellEntry const* pConcussionBlow;
            SpellEntry const* pPiercingHowl;
        } warrior;
        struct
        {
            SpellEntry const* pSliceAndDice;
            SpellEntry const* pSinisterStrike;
            SpellEntry const* pAdrenalineRush;
            SpellEntry const* pEviscerate;
            SpellEntry const* pStealth;
            SpellEntry const* pGarrote;
            SpellEntry const* pAmbush;
            SpellEntry const* pCheapShot;
            SpellEntry const* pPremeditation;
            SpellEntry const* pBackstab;
            SpellEntry const* pHemorrhage;
            SpellEntry const* pGhostlyStrike;
            SpellEntry const* pGouge;
            SpellEntry const* pRupture;
            SpellEntry const* pExposeArmor;
            SpellEntry const* pKidneyShot;
            SpellEntry const* pColdBlood;
            SpellEntry const* pBladeFlurry;
            SpellEntry const* pVanish;
            SpellEntry const* pBlind;
            SpellEntry const* pPreparation;
            SpellEntry const* pEvasion;
            SpellEntry const* pRiposte;
            SpellEntry const* pKick;
            SpellEntry const* pSprint;
            SpellEntry const* pMainHandPoison;
            SpellEntry const* pOffHandPoison;
        } rogue;
        struct
        {
            SpellEntry const* pBearForm;
            SpellEntry const* pCatForm;
            SpellEntry const* pTravelForm;
            SpellEntry const* pAquaticForm;
            SpellEntry const* pMoonkinForm;
            SpellEntry const* pWrath;
            SpellEntry const* pMoonfire;
            SpellEntry const* pStarfire;
            SpellEntry const* pHurricane;
            SpellEntry const* pInsectSwarm;
            SpellEntry const* pBarkskin;
            SpellEntry const* pNaturesGrasp;
            SpellEntry const* pMarkoftheWild;
            SpellEntry const* pGiftoftheWild;
            SpellEntry const* pThorns;
            SpellEntry const* pRemoveCurse;
            SpellEntry const* pCurePoison;
            SpellEntry const* pAbolishPoison;
            SpellEntry const* pRebirth;
            SpellEntry const* pFaerieFire;
            SpellEntry const* pInnervate;
            SpellEntry const* pNaturesSwiftness;
            SpellEntry const* pEntanglingRoots;
            SpellEntry const* pHibernate;
            // Cat
            SpellEntry const* pProwl;
            SpellEntry const* pPounce;
            SpellEntry const* pRavage;
            SpellEntry const* pClaw;
            SpellEntry const* pShred;
            SpellEntry const* pRake;
            SpellEntry const* pRip;
            SpellEntry const* pFerociousBite;
            SpellEntry const* pTigersFury;
            SpellEntry const* pDash;
            SpellEntry const* pFaerieFireFeral;
            SpellEntry const* pCower;
            // Bear
            SpellEntry const* pGrowl;
            SpellEntry const* pChallengingRoar;
            SpellEntry const* pDemoralizingRoar;
            SpellEntry const* pEnrage;
            SpellEntry const* pFrenziedRegeneration;
            SpellEntry const* pSwipe;
            SpellEntry const* pMaul;
            SpellEntry const* pBash;
            SpellEntry const* pFeralCharge;
        } druid;
    } m_spells;

    bool m_initialized = false;
    // Set whenever spell data is reset or (re)populated; migrated rotations
    // rebuild their tables from m_spells on the next tick, then clear it.
    bool m_rotationDirty = true;
    bool m_isBuffing = false;
    bool m_preventCasting = false;
    bool m_receivedBgInvite = false;
    uint8 m_visualHonorRank = 0;
    CombatBotRoles m_role = ROLE_INVALID;
};

#endif
