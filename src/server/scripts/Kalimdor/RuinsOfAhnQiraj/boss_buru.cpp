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

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "ruins_of_ahnqiraj.h"

enum Emotes
{
    EMOTE_TARGET                = 0
};

enum Spells
{
    SPELL_CREEPING_PLAGUE       = 20512,
    SPELL_DISMEMBER             = 96,
    SPELL_GATHERING_SPEED       = 1834,
    SPELL_FULL_SPEED            = 1557,
    SPELL_THORNS                = 25640,
    SPELL_BURU_TRANSFORM        = 24721,
    SPELL_SUMMON_HATCHLING      = 1881,
    SPELL_EXPLODE               = 19593,
    SPELL_EXPLODE_2             = 5255,
    SPELL_BURU_EGG_TRIGGER      = 26646
};

enum Events
{
    EVENT_DISMEMBER             = 1,
    EVENT_GATHERING_SPEED       = 2,
    EVENT_FULL_SPEED            = 3,
    EVENT_CREEPING_PLAGUE       = 4,
    EVENT_RESPAWN_EGG           = 5
};

enum Phases
{
    PHASE_EGG                   = 0,
    PHASE_TRANSFORM             = 1
};

enum Actions
{
    ACTION_EXPLODE              = 0
};

class boss_buru : public CreatureScript
{
public:
    boss_buru() : CreatureScript("boss_buru") { }

    struct boss_buruAI : public BossAI
    {
        boss_buruAI(Creature* creature) : BossAI(creature, DATA_BURU)
        {
        }

        void EnterEvadeMode() override
        {
            BossAI::EnterEvadeMode();

            for (ObjectGuid const& guid : Eggs)
                if (Creature* egg = me->GetMap()->GetCreature(guid))
                    egg->Respawn();

            Eggs.clear();
        }

        void EnterCombat(Unit* who) override
        {
            _EnterCombat();
            Talk(EMOTE_TARGET, who);
            DoCast(me, SPELL_THORNS);

            events.ScheduleEvent(EVENT_DISMEMBER, 5000);
            events.ScheduleEvent(EVENT_GATHERING_SPEED, 9000);
            events.ScheduleEvent(EVENT_FULL_SPEED, 60000);

            _phase = PHASE_EGG;
        }

        void DoAction(int32 action) override
        {
            if (action == ACTION_EXPLODE)
                if (_phase == PHASE_EGG)
                    Unit::DealDamage(me, me, 45000);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                ChaseNewVictim();
        }

        void ChaseNewVictim()
        {
            if (_phase != PHASE_EGG)
                return;

            me->RemoveAurasDueToSpell(SPELL_FULL_SPEED);
            me->RemoveAurasDueToSpell(SPELL_GATHERING_SPEED);
            events.ScheduleEvent(EVENT_GATHERING_SPEED, 9000);
            events.ScheduleEvent(EVENT_FULL_SPEED, 60000);

            if (Unit* victim = SelectTarget(SelectTargetMethod::Random, 0, 0.0f, true))
            {
                DoResetThreat();
                AttackStart(victim);
                Talk(EMOTE_TARGET, victim);
            }
        }

        void ManageRespawn(ObjectGuid EggGUID)
        {
            ChaseNewVictim();
            Eggs.push_back(EggGUID);
            events.ScheduleEvent(EVENT_RESPAWN_EGG, 100000);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_DISMEMBER:
                        DoCastVictim(SPELL_DISMEMBER);
                        events.ScheduleEvent(EVENT_DISMEMBER, 5000);
                        break;
                    case EVENT_GATHERING_SPEED:
                        DoCast(me, SPELL_GATHERING_SPEED);
                        events.ScheduleEvent(EVENT_GATHERING_SPEED, 9000);
                        break;
                    case EVENT_FULL_SPEED:
                        DoCast(me, SPELL_FULL_SPEED);
                        break;
                    case EVENT_CREEPING_PLAGUE:
                        DoCast(me, SPELL_CREEPING_PLAGUE);
                        events.ScheduleEvent(EVENT_CREEPING_PLAGUE, 6000);
                        break;
                    case EVENT_RESPAWN_EGG:
                        if (Creature* egg = me->GetMap()->GetCreature(*Eggs.begin()))
                        {
                            egg->Respawn();
                            Eggs.pop_front();
                        }
                        break;
                    default:
                        break;
                }
            }

            if (me->GetHealthPct() < 20.0f && _phase == PHASE_EGG)
            {
                DoCast(me, SPELL_BURU_TRANSFORM); // Enrage
                DoCast(me, SPELL_FULL_SPEED, true);
                me->RemoveAurasDueToSpell(SPELL_THORNS);
                _phase = PHASE_TRANSFORM;
            }

            DoMeleeAttackIfReady();
        }
    private:
        uint8 _phase;
        GuidList Eggs;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetRuinsOfAhnQirajAI<boss_buruAI>(creature);
    }
};

class npc_buru_egg : public CreatureScript
{
public:
    npc_buru_egg() : CreatureScript("npc_buru_egg") { }

    struct npc_buru_eggAI : public ScriptedAI
    {
        npc_buru_eggAI(Creature* creature) : ScriptedAI(creature)
        {
            _instance = me->GetInstanceScript();
            SetCombatMovement(false);
        }

        void EnterCombat(Unit* attacker) override
        {
            if (Creature* buru = me->GetMap()->GetCreature(_instance->GetGuidData(DATA_BURU)))
                if (!buru->IsInCombat())
                    buru->AI()->AttackStart(attacker);
        }

        void JustSummoned(Creature* who) override
        {
            if (who->GetEntry() == NPC_HATCHLING)
                if (Creature* buru = me->GetMap()->GetCreature(_instance->GetGuidData(DATA_BURU)))
                    if (Unit* target = buru->AI()->SelectTarget(SelectTargetMethod::Random))
                        who->AI()->AttackStart(target);
        }

        void JustDied(Unit* /*killer*/) override
        {
            DoCastAOE(SPELL_EXPLODE, true);
            DoCastAOE(SPELL_EXPLODE_2, true); // Unknown purpose
            DoCast(me, SPELL_SUMMON_HATCHLING, true);

            if (Creature* buru = me->GetMap()->GetCreature(_instance->GetGuidData(DATA_BURU)))
                if (boss_buru::boss_buruAI* buruAI = dynamic_cast<boss_buru::boss_buruAI*>(buru->AI()))
                    buruAI->ManageRespawn(me->GetGUID());
        }
    private:
        InstanceScript* _instance;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetRuinsOfAhnQirajAI<npc_buru_eggAI>(creature);
    }
};

class spell_egg_explosion : public SpellScriptLoader
{
public:
    spell_egg_explosion() : SpellScriptLoader("spell_egg_explosion") { }

    class spell_egg_explosion_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_egg_explosion_SpellScript);

        void HandleAfterCast()
        {
            if (Creature* buru = GetCaster()->FindNearestCreature(NPC_BURU, 5.f))
                buru->AI()->DoAction(ACTION_EXPLODE);
        }

        void HandleDummyHitTarget(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
                Unit::DealDamage(GetCaster(), target, -16 * GetCaster()->GetDistance(target) + 500);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_egg_explosion_SpellScript::HandleAfterCast);
            OnEffectHitTarget += SpellEffectFn(spell_egg_explosion_SpellScript::HandleDummyHitTarget, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_egg_explosion_SpellScript();
    }
};

void AddSC_boss_buru()
{
    new boss_buru();
    new npc_buru_egg();
    new spell_egg_explosion();
}
