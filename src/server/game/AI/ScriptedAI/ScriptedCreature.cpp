/*
 * This file is part of the WarheadCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptedCreature.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GameTime.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "ObjectMgr.h"
#include "Spell.h"
#include "TemporarySummon.h"

// Spell summary for ScriptedAI::SelectSpell
struct TSpellSummary
{
    uint8 Targets;                                          // set of enum SelectTarget
    uint8 Effects;                                          // set of enum SelectEffect
} extern* SpellSummary;

void SummonList::DoZoneInCombat(uint32 entry)
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        Creature* summon = ObjectAccessor::GetCreature(*me, *i);
        ++i;
        if (summon && summon->IsAIEnabled
                && (!entry || summon->GetEntry() == entry))
        {
            summon->AI()->DoZoneInCombat();
        }
    }
}

void SummonList::DespawnEntry(uint32 entry)
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        Creature* summon = ObjectAccessor::GetCreature(*me, *i);
        if (!summon)
            i = storage_.erase(i);
        else if (summon->GetEntry() == entry)
        {
            i = storage_.erase(i);
            summon->DespawnOrUnsummon();
        }
        else
            ++i;
    }
}

void SummonList::DespawnAll()
{
    while (!storage_.empty())
    {
        Creature* summon = ObjectAccessor::GetCreature(*me, storage_.front());
        storage_.pop_front();
        if (summon)
            summon->DespawnOrUnsummon();
    }
}

void SummonList::RemoveNotExisting()
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        if (ObjectAccessor::GetCreature(*me, *i))
            ++i;
        else
            i = storage_.erase(i);
    }
}

bool SummonList::HasEntry(uint32 entry) const
{
    for (StorageType::const_iterator i = storage_.begin(); i != storage_.end(); ++i)
    {
        Creature* summon = ObjectAccessor::GetCreature(*me, *i);
        if (summon && summon->GetEntry() == entry)
            return true;
    }

    return false;
}

uint32 SummonList::GetEntryCount(uint32 entry) const
{
    uint32 count = 0;
    for (StorageType::const_iterator i = storage_.begin(); i != storage_.end(); ++i)
    {
        Creature* summon = ObjectAccessor::GetCreature(*me, *i);
        if (summon && summon->GetEntry() == entry)
            ++count;
    }

    return count;
}

void SummonList::Respawn()
{
    for (StorageType::iterator i = storage_.begin(); i != storage_.end();)
    {
        if (Creature* summon = ObjectAccessor::GetCreature(*me, *i))
        {
            summon->Respawn(true);
            ++i;
        }
        else
            i = storage_.erase(i);
    }
}

Creature* SummonList::GetCreatureWithEntry(uint32 entry) const
{
    for (StorageType::const_iterator i = storage_.begin(); i != storage_.end(); ++i)
    {
        if (Creature* summon = ObjectAccessor::GetCreature(*me, *i))
            if (summon->GetEntry() == entry)
                return summon;
    }

    return nullptr;
}

bool SummonList::IsAnyCreatureAlive() const
{
    for (auto const& guid : storage_)
    {
        if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
        {
            if (summon->IsAlive())
            {
                return true;
            }
        }
    }

    return false;
}

bool SummonList::IsAnyCreatureInCombat() const
{
    for (auto const& guid : storage_)
    {
        if (Creature* summon = ObjectAccessor::GetCreature(*me, guid))
        {
            if (summon->IsInCombat())
            {
                return true;
            }
        }
    }

    return false;
}

ScriptedAI::ScriptedAI(Creature* creature) : CreatureAI(creature),
    me(creature),
    IsFleeing(false),
    _evadeCheckCooldown(2500),
    _isCombatMovementAllowed(true)
{
    _isHeroic = me->GetMap()->IsHeroic();
    _difficulty = Difficulty(me->GetMap()->GetSpawnMode());
}

void ScriptedAI::AttackStartNoMove(Unit* who)
{
    if (!who)
        return;

    if (me->Attack(who, true))
        DoStartNoMovement(who);
}

void ScriptedAI::AttackStart(Unit* who)
{
    if (IsCombatMovementAllowed())
        CreatureAI::AttackStart(who);
    else
        AttackStartNoMove(who);
}

void ScriptedAI::UpdateAI(uint32 /*diff*/)
{
    //Check if we have a current target
    if (!UpdateVictim())
        return;

    DoMeleeAttackIfReady();
}

void ScriptedAI::DoStartMovement(Unit* victim, float distance, float angle)
{
    if (victim)
        me->GetMotionMaster()->MoveChase(victim, distance, angle);
}

void ScriptedAI::DoStartNoMovement(Unit* victim)
{
    if (!victim)
        return;

    me->GetMotionMaster()->MoveIdle();
}

void ScriptedAI::DoStopAttack()
{
    if (me->GetVictim())
        me->AttackStop();
}

void ScriptedAI::DoCastSpell(Unit* target, SpellInfo const* spellInfo, bool triggered)
{
    if (!target || me->IsNonMeleeSpellCast(false))
        return;

    me->StopMoving();
    me->CastSpell(target, spellInfo, triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE);
}

void ScriptedAI::DoPlaySoundToSet(WorldObject* source, uint32 soundId)
{
    if (!source)
        return;

    if (!sSoundEntriesStore.LookupEntry(soundId))
    {
        LOG_ERROR("entities.unit.ai", "TSCR: Invalid soundId {} used in DoPlaySoundToSet (Source: {})", soundId, source->GetGUID().ToString());
        return;
    }

    source->PlayDirectSound(soundId);
}

void ScriptedAI::DoPlayMusic(uint32 soundId, bool zone)
{
    ObjectList* targets = nullptr;

    if (me && me->FindMap())
    {
        Map::PlayerList const& players = me->GetMap()->GetPlayers();
        targets = new ObjectList();

        if (!players.IsEmpty())
        {
            for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                if (Player* player = i->GetSource())
                {
                    if (player->GetZoneId() == me->GetZoneId())
                    {
                        if (!zone)
                        {
                            if (player->GetAreaId() == me->GetAreaId())
                                targets->push_back(player);
                        }
                        else
                            targets->push_back(player);
                    }
                }
        }
    }

    if (targets)
    {
        for (ObjectList::const_iterator itr = targets->begin(); itr != targets->end(); ++itr)
        {
            (*itr)->SendPlayMusic(soundId, true);
        }

        delete targets;
    }
}

Creature* ScriptedAI::DoSpawnCreature(uint32 entry, float offsetX, float offsetY, float offsetZ, float angle, uint32 type, uint32 despawntime)
{
    return me->SummonCreature(entry, me->GetPositionX() + offsetX, me->GetPositionY() + offsetY, me->GetPositionZ() + offsetZ, angle, TempSummonType(type), despawntime);
}

SpellInfo const* ScriptedAI::SelectSpell(Unit* target, uint32 school, uint32 mechanic, SelectTargetType targets, uint32 powerCostMin, uint32 powerCostMax, float rangeMin, float rangeMax, SelectEffect effects)
{
    //No target so we can't cast
    if (!target)
        return nullptr;

    //Silenced so we can't cast
    if (me->HasUnitFlag(UNIT_FLAG_SILENCED))
        return nullptr;

    //Using the extended script system we first create a list of viable spells
    SpellInfo const* apSpell[MAX_CREATURE_SPELLS];
    memset(apSpell, 0, MAX_CREATURE_SPELLS * sizeof(SpellInfo*));

    uint32 spellCount = 0;

    SpellInfo const* tempSpell = nullptr;

    //Check if each spell is viable(set it to null if not)
    for (uint32 i = 0; i < MAX_CREATURE_SPELLS; i++)
    {
        tempSpell = sSpellMgr->GetSpellInfo(me->m_spells[i]);

        //This spell doesn't exist
        if (!tempSpell)
            continue;

        // Targets and Effects checked first as most used restrictions
        //Check the spell targets if specified
        if (targets && !(SpellSummary[me->m_spells[i]].Targets & (1 << (targets - 1))))
            continue;

        //Check the type of spell if we are looking for a specific spell type
        if (effects && !(SpellSummary[me->m_spells[i]].Effects & (1 << (effects - 1))))
            continue;

        //Check for school if specified
        if (school && (tempSpell->SchoolMask & school) == 0)
            continue;

        //Check for spell mechanic if specified
        if (mechanic && tempSpell->Mechanic != mechanic)
            continue;

        //Make sure that the spell uses the requested amount of power
        if (powerCostMin && tempSpell->ManaCost < powerCostMin)
            continue;

        if (powerCostMax && tempSpell->ManaCost > powerCostMax)
            continue;

        //Continue if we don't have the mana to actually cast this spell
        if (tempSpell->ManaCost > me->GetPower(Powers(tempSpell->PowerType)))
            continue;

        //Check if the spell meets our range requirements
        if (rangeMin && me->GetSpellMinRangeForTarget(target, tempSpell) < rangeMin)
            continue;
        if (rangeMax && me->GetSpellMaxRangeForTarget(target, tempSpell) > rangeMax)
            continue;

        //Check if our target is in range
        if (me->IsWithinDistInMap(target, float(me->GetSpellMinRangeForTarget(target, tempSpell))) || !me->IsWithinDistInMap(target, float(me->GetSpellMaxRangeForTarget(target, tempSpell))))
            continue;

        //All good so lets add it to the spell list
        apSpell[spellCount] = tempSpell;
        ++spellCount;
    }

    //We got our usable spells so now lets randomly pick one
    if (!spellCount)
        return nullptr;

    return apSpell[urand(0, spellCount - 1)];
}

void ScriptedAI::DoResetThreat()
{
    if (!me->CanHaveThreatList() || me->getThreatMgr().isThreatListEmpty())
    {
        LOG_ERROR("entities.unit.ai", "DoResetThreat called for creature that either cannot have threat list or has empty threat list (me entry = {})", me->GetEntry());
        return;
    }

    me->getThreatMgr().resetAllAggro();
}

float ScriptedAI::DoGetThreat(Unit* unit)
{
    if (!unit)
        return 0.0f;
    return me->getThreatMgr().getThreat(unit);
}

void ScriptedAI::DoModifyThreatPercent(Unit* unit, int32 pct)
{
    if (!unit)
        return;
    me->getThreatMgr().modifyThreatPercent(unit, pct);
}

void ScriptedAI::DoTeleportPlayer(Unit* unit, float x, float y, float z, float o)
{
    if (!unit)
        return;

    if (Player* player = unit->ToPlayer())
        player->TeleportTo(unit->GetMapId(), x, y, z, o, TELE_TO_NOT_LEAVE_COMBAT);
    else
        LOG_ERROR("entities.unit.ai", "Creature {} Tried to teleport non-player unit {} to x: {} y: {} z: {} o: {}. Aborted.",
            me->GetGUID(), unit->GetGUID(), x, y, z, o);
}

void ScriptedAI::DoTeleportAll(float x, float y, float z, float o)
{
    Map* map = me->GetMap();
    if (!map->IsDungeon())
        return;

    Map::PlayerList const& PlayerList = map->GetPlayers();
    for (Map::PlayerList::const_iterator itr = PlayerList.begin(); itr != PlayerList.end(); ++itr)
        if (Player* player = itr->GetSource())
            if (player->IsAlive())
                player->TeleportTo(me->GetMapId(), x, y, z, o, TELE_TO_NOT_LEAVE_COMBAT);
}

Unit* ScriptedAI::DoSelectLowestHpFriendly(float range, uint32 minHPDiff)
{
    Unit* unit = nullptr;
    Warhead::MostHPMissingInRange u_check(me, range, minHPDiff);
    Warhead::UnitLastSearcher<Warhead::MostHPMissingInRange> searcher(me, unit, u_check);
    Cell::VisitAllObjects(me, searcher, range);

    return unit;
}

std::list<Creature*> ScriptedAI::DoFindFriendlyCC(float range)
{
    std::list<Creature*> list;
    Warhead::FriendlyCCedInRange u_check(me, range);
    Warhead::CreatureListSearcher<Warhead::FriendlyCCedInRange> searcher(me, list, u_check);
    Cell::VisitAllObjects(me, searcher, range);
    return list;
}

std::list<Creature*> ScriptedAI::DoFindFriendlyMissingBuff(float range, uint32 uiSpellid)
{
    std::list<Creature*> list;
    Warhead::FriendlyMissingBuffInRange u_check(me, range, uiSpellid);
    Warhead::CreatureListSearcher<Warhead::FriendlyMissingBuffInRange> searcher(me, list, u_check);
    Cell::VisitAllObjects(me, searcher, range);
    return list;
}

Player* ScriptedAI::GetPlayerAtMinimumRange(float minimumRange)
{
    Player* player = nullptr;

    Warhead::PlayerAtMinimumRangeAway check(me, minimumRange);
    Warhead::PlayerSearcher<Warhead::PlayerAtMinimumRangeAway> searcher(me, player, check);

    Cell::VisitWorldObjects(me, searcher, minimumRange);

    return player;
}

void ScriptedAI::SetEquipmentSlots(bool loadDefault, int32 mainHand /*= EQUIP_NO_CHANGE*/, int32 offHand /*= EQUIP_NO_CHANGE*/, int32 ranged /*= EQUIP_NO_CHANGE*/)
{
    if (loadDefault)
    {
        me->LoadEquipment(me->GetOriginalEquipmentId(), true);
        return;
    }

    if (mainHand >= 0)
        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 0, uint32(mainHand));

    if (offHand >= 0)
        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 1, uint32(offHand));

    if (ranged >= 0)
        me->SetUInt32Value(UNIT_VIRTUAL_ITEM_SLOT_ID + 2, uint32(ranged));
}

void ScriptedAI::SetCombatMovement(bool allowMovement)
{
    _isCombatMovementAllowed = allowMovement;
}

enum eNPCs
{
    NPC_BROODLORD   = 12017,
    NPC_JAN_ALAI    = 23578,
    NPC_SARTHARION  = 28860,
    NPC_FREYA       = 32906,
};

bool ScriptedAI::EnterEvadeIfOutOfCombatArea()
{
    if (me->IsInEvadeMode() || !me->IsInCombat())
        return false;

    if (_evadeCheckCooldown == GameTime::GetGameTime().count())
        return false;

    _evadeCheckCooldown = GameTime::GetGameTime().count();

    if (!CheckEvadeIfOutOfCombatArea())
        return false;

    EnterEvadeMode();
    return true;
}

Player* ScriptedAI::SelectTargetFromPlayerList(float maxdist, uint32 excludeAura, bool mustBeInLOS) const
{
    Map::PlayerList const& pList = me->GetMap()->GetPlayers();
    std::vector<Player*> tList;
    for(Map::PlayerList::const_iterator itr = pList.begin(); itr != pList.end(); ++itr)
    {
        if (!me->IsWithinDistInMap(itr->GetSource(), maxdist, true, false) || !itr->GetSource()->IsAlive() || itr->GetSource()->IsGameMaster())
            continue;
        if (excludeAura && itr->GetSource()->HasAura(excludeAura))
            continue;
        if (mustBeInLOS && !me->IsWithinLOSInMap(itr->GetSource()))
            continue;
        tList.push_back(itr->GetSource());
    }
    if (!tList.empty())
        return tList[urand(0, tList.size() - 1)];
    else
        return nullptr;
}

// BossAI - for instanced bosses

BossAI::BossAI(Creature* creature, uint32 bossId) : ScriptedAI(creature),
    instance(creature->GetInstanceScript()),
    summons(creature),
    _boundary(instance ? instance->GetBossBoundary(bossId) : nullptr),
    _bossId(bossId)
{
}

void BossAI::_Reset()
{
    if (!me->IsAlive())
        return;

    me->ResetLootMode();
    events.Reset();
    summons.DespawnAll();
    if (instance)
        instance->SetBossState(_bossId, NOT_STARTED);
}

void BossAI::_JustDied()
{
    events.Reset();
    summons.DespawnAll();
    if (instance)
    {
        instance->SetBossState(_bossId, DONE);
        instance->SaveToDB();
    }
}

void BossAI::_EnterCombat()
{
    me->setActive(true);
    DoZoneInCombat();
    if (instance)
    {
        // bosses do not respawn, check only on enter combat
        if (!instance->CheckRequiredBosses(_bossId))
        {
            EnterEvadeMode();
            return;
        }
        instance->SetBossState(_bossId, IN_PROGRESS);
    }
}

void BossAI::TeleportCheaters()
{
    float x, y, z;
    me->GetPosition(x, y, z);

    ThreatContainer::StorageType threatList = me->getThreatMgr().getThreatList();
    for (ThreatContainer::StorageType::const_iterator itr = threatList.begin(); itr != threatList.end(); ++itr)
        if (Unit* target = (*itr)->getTarget())
            if (target->GetTypeId() == TYPEID_PLAYER && !CheckBoundary(target))
                target->NearTeleportTo(x, y, z, 0);
}

bool BossAI::CheckBoundary(Unit* who)
{
    if (!GetBoundary() || !who)
        return true;

    for (BossBoundaryMap::const_iterator itr = GetBoundary()->begin(); itr != GetBoundary()->end(); ++itr)
    {
        switch (itr->first)
        {
            case BOUNDARY_N:
                if (who->GetPositionX() > itr->second)
                    return false;
                break;
            case BOUNDARY_S:
                if (who->GetPositionX() < itr->second)
                    return false;
                break;
            case BOUNDARY_E:
                if (who->GetPositionY() < itr->second)
                    return false;
                break;
            case BOUNDARY_W:
                if (who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_NW:
                if (who->GetPositionX() + who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_SE:
                if (who->GetPositionX() + who->GetPositionY() < itr->second)
                    return false;
                break;
            case BOUNDARY_NE:
                if (who->GetPositionX() - who->GetPositionY() > itr->second)
                    return false;
                break;
            case BOUNDARY_SW:
                if (who->GetPositionX() - who->GetPositionY() < itr->second)
                    return false;
                break;
            default:
                break;
        }
    }

    return true;
}

void BossAI::JustSummoned(Creature* summon)
{
    summons.Summon(summon);
    if (me->IsInCombat())
        DoZoneInCombat(summon);
}

void BossAI::SummonedCreatureDespawn(Creature* summon)
{
    summons.Despawn(summon);
}

void BossAI::SummonedCreatureDespawnAll()
{
    summons.DespawnAll();
}

void BossAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
    {
        return;
    }

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
    {
        return;
    }

    while (uint32 const eventId = events.ExecuteEvent())
    {
        ExecuteEvent(eventId);
        if (me->HasUnitState(UNIT_STATE_CASTING))
        {
            return;
        }
    }

    DoMeleeAttackIfReady();
}

// WorldBossAI - for non-instanced bosses

WorldBossAI::WorldBossAI(Creature* creature) :
    ScriptedAI(creature),
    summons(creature)
{
}

void WorldBossAI::_Reset()
{
    if (!me->IsAlive())
        return;

    events.Reset();
    summons.DespawnAll();
}

void WorldBossAI::_JustDied()
{
    events.Reset();
    summons.DespawnAll();
}

void WorldBossAI::_EnterCombat()
{
    Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true);
    if (target)
        AttackStart(target);
}

void WorldBossAI::JustSummoned(Creature* summon)
{
    summons.Summon(summon);
    Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true);
    if (target)
        summon->AI()->AttackStart(target);
}

void WorldBossAI::SummonedCreatureDespawn(Creature* summon)
{
    summons.Despawn(summon);
}

void WorldBossAI::UpdateAI(uint32 diff)
{
    if (!UpdateVictim())
        return;

    events.Update(diff);

    if (me->HasUnitState(UNIT_STATE_CASTING))
        return;

    while (uint32 eventId = events.ExecuteEvent())
        ExecuteEvent(eventId);

    DoMeleeAttackIfReady();
}

// SD2 grid searchers.
Creature* GetClosestCreatureWithEntry(WorldObject* source, uint32 entry, float maxSearchRange, bool alive /*= true*/)
{
    return source->FindNearestCreature(entry, maxSearchRange, alive);
}

GameObject* GetClosestGameObjectWithEntry(WorldObject* source, uint32 entry, float maxSearchRange, bool onlySpawned /*= false*/)
{
    return source->FindNearestGameObject(entry, maxSearchRange, onlySpawned);
}

void GetCreatureListWithEntryInGrid(std::list<Creature*>& list, WorldObject* source, uint32 entry, float maxSearchRange)
{
    source->GetCreatureListWithEntryInGrid(list, entry, maxSearchRange);
}

void GetGameObjectListWithEntryInGrid(std::list<GameObject*>& list, WorldObject* source, uint32 entry, float maxSearchRange)
{
    source->GetGameObjectListWithEntryInGrid(list, entry, maxSearchRange);
}

 void GetDeadCreatureListInGrid(std::list<Creature*>& list, WorldObject* source, float maxSearchRange, bool alive /*= false*/)
{
    source->GetDeadCreatureListInGrid(list, maxSearchRange, alive);
}
