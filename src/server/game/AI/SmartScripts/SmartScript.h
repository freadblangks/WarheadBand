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

#ifndef WARHEAD_SMARTSCRIPT_H
#define WARHEAD_SMARTSCRIPT_H

#include "Common.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GridNotifiers.h"
#include "SmartScriptMgr.h"
#include "Spell.h"
#include "Unit.h"

class WH_GAME_API SmartScript
{
public:
    SmartScript();
    ~SmartScript();

    void OnInitialize(WorldObject* obj, AreaTrigger const* at = nullptr);
    void GetScript();
    void FillScript(SmartAIEventList e, WorldObject* obj, AreaTrigger const* at);

    void ProcessEventsFor(SMART_EVENT e, Unit* unit = nullptr, uint32 var0 = 0, uint32 var1 = 0, bool bvar = false, SpellInfo const* spell = nullptr, GameObject* gob = nullptr);
    void ProcessEvent(SmartScriptHolder& e, Unit* unit = nullptr, uint32 var0 = 0, uint32 var1 = 0, bool bvar = false, SpellInfo const* spell = nullptr, GameObject* gob = nullptr);
    bool CheckTimer(SmartScriptHolder const& e) const;
    void RecalcTimer(SmartScriptHolder& e, uint32 min, uint32 max);
    void UpdateTimer(SmartScriptHolder& e, uint32 const diff);
    void InitTimer(SmartScriptHolder& e);
    void ProcessAction(SmartScriptHolder& e, Unit* unit = nullptr, uint32 var0 = 0, uint32 var1 = 0, bool bvar = false, SpellInfo const* spell = nullptr, GameObject* gob = nullptr);
    void ProcessTimedAction(SmartScriptHolder& e, uint32 const& min, uint32 const& max, Unit* unit = nullptr, uint32 var0 = 0, uint32 var1 = 0, bool bvar = false, SpellInfo const* spell = nullptr, GameObject* gob = nullptr);
    ObjectList* GetTargets(SmartScriptHolder const& e, Unit* invoker = nullptr);
    ObjectList* GetWorldObjectsInDist(float dist);
    void InstallTemplate(SmartScriptHolder const& e);
    SmartScriptHolder CreateSmartEvent(SMART_EVENT e, uint32 event_flags, uint32 event_param1, uint32 event_param2, uint32 event_param3, uint32 event_param4, uint32 event_param5, SMART_ACTION action, uint32 action_param1, uint32 action_param2, uint32 action_param3, uint32 action_param4, uint32 action_param5, uint32 action_param6, SMARTAI_TARGETS t, uint32 target_param1, uint32 target_param2, uint32 target_param3, uint32 target_param4, uint32 phaseMask);
    void AddEvent(SMART_EVENT e, uint32 event_flags, uint32 event_param1, uint32 event_param2, uint32 event_param3, uint32 event_param4, uint32 event_param5, SMART_ACTION action, uint32 action_param1, uint32 action_param2, uint32 action_param3, uint32 action_param4, uint32 action_param5, uint32 action_param6, SMARTAI_TARGETS t, uint32 target_param1, uint32 target_param2, uint32 target_param3, uint32 target_param4, uint32 phaseMask);
    void SetPathId(uint32 id) { mPathId = id; }
    uint32 GetPathId() const { return mPathId; }
    WorldObject* GetBaseObject()
    {
        WorldObject* obj = nullptr;
        if (me)
            obj = me;
        else if (go)
            obj = go;
        return obj;
    }

    bool IsUnit(WorldObject* obj)
    {
        return obj && obj->IsInWorld() && (obj->GetTypeId() == TYPEID_UNIT || obj->GetTypeId() == TYPEID_PLAYER);
    }

    bool IsPlayer(WorldObject* obj)
    {
        return obj && obj->IsInWorld() && obj->GetTypeId() == TYPEID_PLAYER;
    }

    bool IsCreature(WorldObject* obj)
    {
        return obj && obj->IsInWorld() && obj->GetTypeId() == TYPEID_UNIT;
    }

    bool IsGameObject(WorldObject* obj)
    {
        return obj && obj->IsInWorld() && obj->GetTypeId() == TYPEID_GAMEOBJECT;
    }

    void OnUpdate(const uint32 diff);
    void OnMoveInLineOfSight(Unit* who);

    Unit* DoSelectLowestHpFriendly(float range, uint32 MinHPDiff);
    void DoFindFriendlyCC(std::list<Creature*>& _list, float range);
    void DoFindFriendlyMissingBuff(std::list<Creature*>& list, float range, uint32 spellid);
    Unit* DoFindClosestFriendlyInRange(float range, bool playerOnly);

    void StoreTargetList(ObjectList* targets, uint32 id)
    {
        if (!targets)
            return;

        if (mTargetStorage->find(id) != mTargetStorage->end())
        {
            // check if already stored
            if ((*mTargetStorage)[id]->Equals(targets))
                return;

            delete (*mTargetStorage)[id];
        }

        (*mTargetStorage)[id] = new ObjectGuidList(targets, GetBaseObject());
    }

    bool IsSmart(Creature* c = nullptr)
    {
        bool smart = true;
        if (c && c->GetAIName() != "SmartAI")
            smart = false;

        if (!me || me->GetAIName() != "SmartAI")
            smart = false;

        if (!smart)
            LOG_ERROR("sql.sql", "SmartScript: Action target Creature(entry: {}) is not using SmartAI, action skipped to prevent crash.", c ? c->GetEntry() : (me ? me->GetEntry() : 0));

        return smart;
    }

    bool IsSmartGO(GameObject* g = nullptr)
    {
        bool smart = true;
        if (g && g->GetAIName() != "SmartGameObjectAI")
            smart = false;

        if (!go || go->GetAIName() != "SmartGameObjectAI")
            smart = false;
        if (!smart)
            LOG_ERROR("sql.sql", "SmartScript: Action target GameObject(entry: {}) is not using SmartGameObjectAI, action skipped to prevent crash.", g ? g->GetEntry() : (go ? go->GetEntry() : 0));

        return smart;
    }

    ObjectList* GetTargetList(uint32 id)
    {
        ObjectListMap::iterator itr = mTargetStorage->find(id);
        if (itr != mTargetStorage->end())
            return (*itr).second->GetObjectList();
        return nullptr;
    }

    void StoreCounter(uint32 id, uint32 value, uint32 reset, uint32 subtract)
    {
        CounterMap::iterator itr = mCounterList.find(id);
        if (itr != mCounterList.end())
        {
            if (!reset && !subtract)
            {
                itr->second += value;
            }
            else if (subtract)
            {
                itr->second -= value;
            }
            else
            {
                itr->second = value;
            }
        }
        else
        {
            mCounterList.insert(std::make_pair(id, value));
        }

        ProcessEventsFor(SMART_EVENT_COUNTER_SET, nullptr, id);
    }

    uint32 GetCounterValue(uint32 id)
    {
        CounterMap::iterator itr = mCounterList.find(id);
        if (itr != mCounterList.end())
            return itr->second;
        return 0;
    }

    GameObject* FindGameObjectNear(WorldObject* searchObject, ObjectGuid::LowType guid) const
    {
        auto bounds = searchObject->GetMap()->GetGameObjectBySpawnIdStore().equal_range(guid);
        if (bounds.first == bounds.second)
            return nullptr;

        return bounds.first->second;
    }

    Creature* FindCreatureNear(WorldObject* searchObject, ObjectGuid::LowType guid) const
    {
        auto bounds = searchObject->GetMap()->GetCreatureBySpawnIdStore().equal_range(guid);
        if (bounds.first == bounds.second)
            return nullptr;

        auto creatureItr = std::find_if(bounds.first, bounds.second, [](Map::CreatureBySpawnIdContainer::value_type const& pair)
        {
            return pair.second->IsAlive();
        });

        return creatureItr != bounds.second ? creatureItr->second : bounds.first->second;
    }

    ObjectListMap* mTargetStorage;

    void OnReset();
    void ResetBaseObject()
    {
        WorldObject* lookupRoot = me;
        if (!lookupRoot)
            lookupRoot = go;

        if (lookupRoot)
        {
            if (meOrigGUID)
            {
                if (Creature* m = ObjectAccessor::GetCreature(*lookupRoot, meOrigGUID))
                {
                    me = m;
                    go = nullptr;
                }
            }

            if (goOrigGUID)
            {
                if (GameObject* o = ObjectAccessor::GetGameObject(*lookupRoot, goOrigGUID))
                {
                    me = nullptr;
                    go = o;
                }
            }
        }

        goOrigGUID.Clear();
        meOrigGUID.Clear();
    }

    //TIMED_ACTIONLIST (script type 9 aka script9)
    void SetScript9(SmartScriptHolder& e, uint32 entry);
    Unit* GetLastInvoker(Unit* invoker = nullptr);
    ObjectGuid mLastInvoker;
    typedef std::unordered_map<uint32, uint32> CounterMap;
    CounterMap mCounterList;

    // Xinef: Fix Combat Movement
    void SetActualCombatDist(uint32 dist) { mActualCombatDist = dist; }
    void RestoreMaxCombatDist() { mActualCombatDist = mMaxCombatDist; }
    uint32 GetActualCombatDist() const { return mActualCombatDist; }
    uint32 GetMaxCombatDist() const { return mMaxCombatDist; }

    // Xinef: SmartCasterAI, replace above
    void SetCasterActualDist(float dist) { smartCasterActualDist = dist; }
    void RestoreCasterMaxDist() { smartCasterActualDist = smartCasterMaxDist; }
    Powers GetCasterPowerType() const { return smartCasterPowerType; }
    float GetCasterActualDist() const { return smartCasterActualDist; }
    float GetCasterMaxDist() const { return smartCasterMaxDist; }

    bool AllowPhaseReset() const { return _allowPhaseReset; }
    void SetPhaseReset(bool allow) { _allowPhaseReset = allow; }

private:
    void IncPhase(uint32 p)
    {
        // Xinef: protect phase from overflowing
        mEventPhase = std::min<uint32>(SMART_EVENT_PHASE_12, mEventPhase + p);
    }

    void DecPhase(uint32 p)
    {
        if (p >= mEventPhase)
            mEventPhase = 0;
        else
            mEventPhase -= p;
    }
    bool IsInPhase(uint32 p) const
    {
        if (mEventPhase == 0)
            return false;
        return (1 << (mEventPhase - 1)) & p;
    }
    void SetPhase(uint32 p = 0) { mEventPhase = p; }

    SmartAIEventList mEvents;
    SmartAIEventList mInstallEvents;
    SmartAIEventList mTimedActionList;
    bool isProcessingTimedActionList;
    Creature* me;
    ObjectGuid meOrigGUID;
    GameObject* go;
    ObjectGuid goOrigGUID;
    AreaTrigger const* trigger;
    SmartScriptType mScriptType;
    uint32 mEventPhase;

    std::unordered_map<int32, int32> mStoredDecimals;
    uint32 mPathId;
    SmartAIEventStoredList mStoredEvents;
    std::list<uint32> mRemIDs;

    uint32 mTextTimer;
    uint32 mLastTextID;
    uint32 mTalkerEntry;
    bool mUseTextTimer;

    // Xinef: Fix Combat Movement
    uint32 mActualCombatDist;
    uint32 mMaxCombatDist;

    // Xinef: SmartCasterAI, replace above in future
    uint32 smartCasterActualDist;
    uint32 smartCasterMaxDist;
    Powers smartCasterPowerType;

    // Xinef: misc
    bool _allowPhaseReset;

    SMARTAI_TEMPLATE mTemplate;
    void InstallEvents();

    void RemoveStoredEvent (uint32 id)
    {
        if (!mStoredEvents.empty())
        {
            for (SmartAIEventStoredList::iterator i = mStoredEvents.begin(); i != mStoredEvents.end(); ++i)
            {
                if (i->event_id == id)
                {
                    mStoredEvents.erase(i);
                    return;
                }
            }
        }
    }
    SmartScriptHolder FindLinkedEvent (uint32 link)
    {
        if (!mEvents.empty())
        {
            for (SmartAIEventList::iterator i = mEvents.begin(); i != mEvents.end(); ++i)
            {
                if (i->event_id == link)
                {
                    return (*i);
                }
            }
        }
        SmartScriptHolder s;
        return s;
    }
};

#endif
