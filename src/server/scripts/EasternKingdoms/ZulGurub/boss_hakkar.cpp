/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
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

#include "AreaTriggerScript.h"
#include "CreatureScript.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "zulgurub.h"
#include <random>

/*
Name: Boss_Hakkar
%Complete: 95
Comment: Blood siphon spell buggy cause of Core Issue.
Category: Zul'Gurub
*/

enum Says
{
    SAY_AGGRO                       = 0,
    SAY_FLEEING                     = 1,
    SAY_MINION_DESTROY              = 2,
    SAY_PROTECT_ALTAR               = 3,
    SAY_PROTECT_GURUBASHI_EMPIRE    = 4,
    SAY_PLEDGE_ALLEGIANCE           = 5,
    SAY_WORLD_WILL_SUFFER           = 6,
    SAY_EVADE                       = 7,
};

enum Spells
{
    SPELL_BLOOD_SIPHON          = 24324,
    SPELL_BLOOD_SIPHON_HEAL     = 24322,
    SPELL_BLOOD_SIPHON_DAMAGE   = 24323,
    SPELL_CORRUPTED_BLOOD       = 24328,
    SPELL_CAUSE_INSANITY        = 24327,
    SPELL_ENRAGE                = 24318,
    SPELL_CHANNELED_IMMUNITY    = 232321,
    SPELL_VEIL_OF_SHADOW        = 887068,
    SPELL_SHADOW_DEATH          = 832379,
    SPELL_FEAR                  = 885782,
    // The Aspects of all High Priests spells
    SPELL_ASPECT_OF_JEKLIK      = 24687,
    SPELL_ASPECT_OF_VENOXIS     = 24688,
    SPELL_ASPECT_OF_MARLI       = 24686,
    SPELL_ASPECT_OF_THEKAL      = 24689,
    SPELL_ASPECT_OF_ARLOKK      = 24690,
    SPELL_POISONOUS_BLOOD       = 24321,
};

enum Events
{
    EVENT_CORRUPTED_BLOOD       = 1,
    EVENT_ENRAGE                = 2,
    // The Aspects of all High Priests events
    EVENT_ASPECT_OF_JEKLIK      = 3,
    EVENT_ASPECT_OF_VENOXIS     = 4,
    EVENT_ASPECT_OF_MARLI       = 5,
    EVENT_ASPECT_OF_THEKAL      = 6,
    EVENT_ASPECT_OF_ARLOKK      = 7,
    EVENT_HEALTH_CHECK          = 8,
    EVENT_RESPAWN_PILES         = 9,
    EVENT_VEIL_OF_SHADOW        = 10,
    EVENT_SHADOW_DEATH          = 11,
    EVENT_FEAR                  = 12,
};

class boss_hakkar : public CreatureScript
{
public:
    boss_hakkar() : CreatureScript("boss_hakkar") { }
    
    struct boss_hakkarAI : public BossAI
    {
        boss_hakkarAI(Creature* creature) : BossAI(creature, DATA_HAKKAR), hasDone75(false), hasDone50(false), hasDone25(false) {
            Initialize();
        }

        void Initialize() {
            voodooPileCount = 0;
        }
        
        bool CheckInRoom() override
        {
            if (me->GetPositionZ() < 52.f || me->GetPositionZ() > 57.28f)
            {
                BossAI::EnterEvadeMode(EVADE_REASON_BOUNDARY);
                return false;
            }
            return true;
        }

        void ApplyHakkarPowerStacks()
        {
            me->RemoveAurasDueToSpell(SPELL_HAKKAR_POWER);
            for (int i = DATA_JEKLIK; i < DATA_HAKKAR; i++)
                if (instance->GetBossState(i) != DONE)
                    DoCastSelf(SPELL_HAKKAR_POWER, true);
        }

        void Reset() override
        {
            _Reset();
            events.Reset();
            ApplyHakkarPowerStacks();
            hasDone75 = hasDone50 = hasDone25 = false;
            DespawnVoodooPiles();
            DespawnSpecificCreatures(817263);
            DespawnSpecificCreatures(817264);
            DespawnSpecificCreatures(810986);
            DoCastSelf(875167, true);
            Initialize();
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            DespawnVoodooPiles();
            DoCastSelf(875167, true);
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            for (auto const& playerPair : players)
            {
                Player* player = playerPair.GetSource();
                if (player)
                {
                    DistributeChallengeRewards(player, me, 1, false);
                }
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(EVENT_HEALTH_CHECK, 1s);
            events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, 25s);
            events.ScheduleEvent(EVENT_ENRAGE, 8min);
            events.ScheduleEvent(EVENT_VEIL_OF_SHADOW, 15s);
            events.ScheduleEvent(EVENT_SHADOW_DEATH, 10s);
            events.ScheduleEvent(EVENT_FEAR, 12s);
            if (instance->GetBossState(DATA_JEKLIK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_JEKLIK, 21s);
            if (instance->GetBossState(DATA_VENOXIS) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_VENOXIS, 14s);
            if (instance->GetBossState(DATA_MARLI) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_MARLI, 15s);
            if (instance->GetBossState(DATA_THEKAL) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_THEKAL, 10s);
            if (instance->GetBossState(DATA_ARLOKK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_ARLOKK, 18s);
            Talk(SAY_AGGRO);
        }

        void EnterEvadeMode(EvadeReason evadeReason) override
        {
            BossAI::EnterEvadeMode(evadeReason);
            ResetFromPassiveState();
            _Reset();
            Talk(SAY_EVADE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim() || !CheckInRoom())
                return;

            events.Update(diff);
            
            if (checkTimer <= diff) {
                bool foundAlive = false;
                std::list<Unit*> targets;
                Acore::AnyUnitInObjectRangeCheck check(me, 58.0f); 
                Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targets, check);
                Cell::VisitAllObjects(me, searcher, 58.0f); 

                for (Unit* unit : targets) {
                    if (unit->IsAlive() && (unit->GetTypeId() == TYPEID_PLAYER || (unit->GetTypeId() == TYPEID_UNIT && static_cast<Creature*>(unit)->IsNPCBot()))) {
                        foundAlive = true;
                        break;
                    }
                }

                if (!foundAlive) {
                    EnterEvadeMode(EVADE_REASON_NO_HOSTILES);
                    LOG_ERROR("scripts", "Hakkar resetting due to no hostiles within range.");
                }

                checkTimer = 5000; 
            }
            else {
                checkTimer -= diff;
            }


            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CORRUPTED_BLOOD:
                        DoCastVictim(SPELL_CORRUPTED_BLOOD, true);
                        events.ScheduleEvent(EVENT_CORRUPTED_BLOOD, 30s, 40s);
                        break;
                    case EVENT_ENRAGE:
                        if (!me->HasAura(SPELL_ENRAGE))
                            DoCastSelf(SPELL_ENRAGE);
                        events.ScheduleEvent(EVENT_ENRAGE, 90s);
                        break;
                    case EVENT_ASPECT_OF_JEKLIK:
                        DoCastVictim(SPELL_ASPECT_OF_JEKLIK, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_JEKLIK, 24s);
                        break;
                    case EVENT_ASPECT_OF_VENOXIS:
                        DoCastVictim(SPELL_ASPECT_OF_VENOXIS, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_VENOXIS, 16s, 18s);
                        break;
                    case EVENT_ASPECT_OF_MARLI:
                        if (Unit* victim = SelectTarget(SelectTargetMethod::MaxThreat, 0, 5.f, true))
                        {
                            DoCast(victim, SPELL_ASPECT_OF_MARLI, true);
                            me->GetThreatMgr().ModifyThreatByPercent(victim, -100.f);
                        }
                        events.ScheduleEvent(EVENT_ASPECT_OF_MARLI, 45s);
                        break;
                    case EVENT_ASPECT_OF_THEKAL:
                        DoCastVictim(SPELL_ASPECT_OF_THEKAL, true);
                        events.ScheduleEvent(EVENT_ASPECT_OF_THEKAL, 15s);
                        break;
                    case EVENT_ASPECT_OF_ARLOKK:
                        if (Unit* victim = SelectTarget(SelectTargetMethod::MaxThreat, 0, 5.f, true))
                        {
                            DoCast(victim, SPELL_ASPECT_OF_ARLOKK, true);
                            me->GetThreatMgr().ModifyThreatByPercent(victim, -100.f);
                        }
                        events.ScheduleEvent(EVENT_ASPECT_OF_ARLOKK, 10s, 15s);
                        break;
                    case EVENT_HEALTH_CHECK:
                        CheckHealthEvents();
                        events.ScheduleEvent(EVENT_HEALTH_CHECK, 1s);
                        break;
                    case EVENT_RESPAWN_PILES:
                        LOG_ERROR("scripts", "Attempting to spawn voodoo piles without trigger at health %u", me->GetHealthPct());
                        SpawnVoodooPiles();
                        break;
                    case EVENT_VEIL_OF_SHADOW:
                        DoCastVictim(SPELL_VEIL_OF_SHADOW, true);
                        events.ScheduleEvent(EVENT_VEIL_OF_SHADOW, 20s);
                        break;
                    case EVENT_SHADOW_DEATH:
                        DoCastSelf(SPELL_SHADOW_DEATH);
                        events.ScheduleEvent(EVENT_SHADOW_DEATH, 10s);
                        break;
                    case EVENT_FEAR:
                        CastSpellOnRandomTarget(SPELL_FEAR, 100.0f);
                        events.ScheduleEvent(EVENT_FEAR, 12s);
                        break;
                    default:
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void ResetFromPassiveState() {
           // LOG_ERROR("scripts", "Hakkar is resetting from passive state.");
            me->InterruptNonMeleeSpells(true);
            me->ClearUnitState(UNIT_STATE_CANNOT_AUTOATTACK);  
            me->SetReactState(REACT_AGGRESSIVE);
            me->RemoveAurasDueToSpell(SPELL_CHANNELED_IMMUNITY);
            me->GetMotionMaster()->Clear(false);
            me->GetMotionMaster()->MoveChase(me->GetVictim()); 
            isChanneling = false;
            DoMeleeAttackIfReady();
        }

        void CheckHealthEvents()
        {
          //  LOG_ERROR("scripts", "Hakkar current health: %u", me->GetHealthPct());

            if (!hasDone75 && me->HealthBelowPct(75)) {
                LOG_ERROR("scripts", "Triggering 75% health event");
                EnterPassiveState();
                hasDone75 = true;
                return;
            }
            if (!hasDone50 && me->HealthBelowPct(50)) {
                LOG_ERROR("scripts", "Triggering 50% health event");
                EnterPassiveState();
                hasDone50 = true;
                return;
            }
            if (!hasDone25 && me->HealthBelowPct(25)) {
                LOG_ERROR("scripts", "Triggering 25% health event");
                EnterPassiveState();
                hasDone25 = true;
                return;
            }
        }


        void EnterPassiveState()
        {
            me->InterruptNonMeleeSpells(false);
            me->AttackStop();
            me->SetReactState(REACT_PASSIVE);
            me->GetMotionMaster()->MovePoint(1, me->GetHomePosition());  // Use a unique point id, like 1

            isChanneling = true;  // Assume channeling will start after movement
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == 1) {
                // Hakkar has reached his home position, now perform the actions
                me->Yell("Behold the shadows that shield me! Your efforts are in vain, mortals!", LANG_UNIVERSAL);
                DoCast(me, SPELL_CHANNELED_IMMUNITY, true);
                SpawnVoodooPiles();
             //   events.ScheduleEvent(EVENT_RESPAWN_PILES, 5min);  // Schedule despawning of piles
            }
        }

        void DespawnVoodooPiles()
        {
            std::list<Creature*> piles;
            GetCreatureListWithEntryInGrid(piles, me, NPC_VOODOO_PILE, 100.0f);
            for (Creature* pile : piles) {
                pile->DespawnOrUnsummon();
            }
        }

        void SummonedCreatureDies(Creature* summoned, Unit* /*killer*/) override {
            if (summoned->GetEntry() == NPC_VOODOO_PILE) {
                VoodooPileDied();
            }
        }

        void VoodooPileDied() {
            if (--voodooPileCount <= 0 && isChanneling) {
                ResetFromPassiveState();
            }
        }

        void SpawnVoodooPiles() {
            static const std::vector<Position> allSpawnPositions = {
                {-11763.155273438f, -1680.8394775391f, 52.926322937012f, 1.4970208406448f},
                {-11764.12890625f, -1663.1394042969f, 52.934398651123f, 1.6032851934433f},
                {-11811.252929688f, -1664.75390625f, 52.93355178833f, 1.6093200445175f},
                {-11810.309570312f, -1682.7288818359f, 52.924865722656f, 4.8520903587341f},
                {-11819.540039062f, -1627.4089355469f, 52.783023834229f, 5.5547270774841f},
                {-11760.520507812f, -1633.8549804688f, 52.809814453125f, 1.4582473039627f},
                {-11780.124023438f, -1654.46484375f, 53.211692810059f, 2.2234199047089f},
                {-11791.803710938f, -1624.2508544922f, 54.721839904785f, 1.7439340353012f}
            };

            std::vector<Position> selectedPositions;
            std::sample(allSpawnPositions.begin(), allSpawnPositions.end(), std::back_inserter(selectedPositions),
                4, std::mt19937{ std::random_device{}() });

            voodooPileCount = 0; // Reset count before spawning new piles

            for (const auto& pos : selectedPositions) {
                if (TempSummon* pile = me->SummonCreature(NPC_VOODOO_PILE, pos, TEMPSUMMON_MANUAL_DESPAWN)) {
                    pile->SetReactState(REACT_PASSIVE);
                    voodooPileCount++;
                }
            }
        }

        void CastSpellOnRandomTarget(uint32 spellId, float range)
        {
            std::list<Unit*> targets;
            Acore::AnyUnitInObjectRangeCheck check(me, range);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targets, check);
            Cell::VisitAllObjects(me, searcher, range);

            targets.remove_if([this](Unit* unit) -> bool {
                return !unit->IsAlive() || !(unit->GetTypeId() == TYPEID_PLAYER || (unit->GetTypeId() == TYPEID_UNIT && static_cast<Creature*>(unit)->IsNPCBot()));
                });

            if (!targets.empty())
            {
                Unit* target = Acore::Containers::SelectRandomContainerElement(targets);
                DoCast(target, spellId);
            }
        }

        void DespawnSpecificCreatures(uint32 entry)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, entry, 100.0f);
            for (Creature* creature : creatures) {
                creature->DespawnOrUnsummon();
            }
        }

        void ScheduleAspectEvents()
        {
            // Rescheduling aspect events as previously defined
            if (instance->GetBossState(DATA_JEKLIK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_JEKLIK, 21s);
            if (instance->GetBossState(DATA_VENOXIS) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_VENOXIS, 14s);
            if (instance->GetBossState(DATA_MARLI) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_MARLI, 15s);
            if (instance->GetBossState(DATA_THEKAL) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_THEKAL, 10s);
            if (instance->GetBossState(DATA_ARLOKK) != DONE)
                events.ScheduleEvent(EVENT_ASPECT_OF_ARLOKK, 18s);
        }

    private:
        bool isChanneling;
        bool hasDone75; 
        bool hasDone50; 
        bool hasDone25;
        int voodooPileCount;
        uint32 checkTimer = 5000;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetZulGurubAI<boss_hakkarAI>(creature);
    }
};

class at_zulgurub_entrance_speech : public OnlyOnceAreaTriggerScript
{
public:
    at_zulgurub_entrance_speech() : OnlyOnceAreaTriggerScript("at_zulgurub_entrance_speech") {}

    bool _OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            // Instance map's enormous, Hakkar's GRID is not loaded by the time players enter.
            // Without this, the creature never says anything, because it doesn't load in time.
            player->GetMap()->LoadGrid(-11783.99f, -1655.27f);

            if (Creature* hakkar = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_HAKKAR)))
            {
                hakkar->setActive(true);
                if (hakkar->GetAI())
                {
                    hakkar->AI()->Talk(SAY_PROTECT_GURUBASHI_EMPIRE);
                }
            }
        }

        return false;
    }
};

class at_zulgurub_bridge_speech : public OnlyOnceAreaTriggerScript
{
public:
    at_zulgurub_bridge_speech() : OnlyOnceAreaTriggerScript("at_zulgurub_bridge_speech") {}

    bool _OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            // Instance map's enormous, Hakkar's GRID is not loaded by the time players enter.
           // Without this, the creature never says anything, because it doesn't load in time.
            player->GetMap()->LoadGrid(-11783.99f, -1655.27f);

            if (Creature* hakkar = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_HAKKAR)))
            {
                if (hakkar->GetAI())
                {
                    hakkar->AI()->Talk(SAY_PROTECT_ALTAR);
                }
            }
        }

        return false;
    }
};

class at_zulgurub_temple_speech : public OnlyOnceAreaTriggerScript
{
public:
    at_zulgurub_temple_speech() : OnlyOnceAreaTriggerScript("at_zulgurub_temple_speech") {}

    bool _OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            // Instance map's enormous, Hakkar's GRID is not loaded by the time players enter.
           // Without this, the creature never says anything, because it doesn't load in time.
            player->GetMap()->LoadGrid(-11783.99f, -1655.27f);

            if (Creature* hakkar = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_HAKKAR)))
            {
                if (hakkar->GetAI())
                {
                    hakkar->AI()->Talk(SAY_MINION_DESTROY);
                }
            }
        }

        return false;
    }
};

class at_zulgurub_bloodfire_pit_speech : public OnlyOnceAreaTriggerScript
{
public:
    at_zulgurub_bloodfire_pit_speech() : OnlyOnceAreaTriggerScript("at_zulgurub_bloodfire_pit_speech") {}

    bool _OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            // Instance map's enormous, Hakkar's GRID is not loaded by the time players enter.
           // Without this, the creature never says anything, because it doesn't load in time.
            player->GetMap()->LoadGrid(-11783.99f, -1655.27f);

            if (Creature* hakkar = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_HAKKAR)))
            {
                if (hakkar->GetAI())
                {
                    hakkar->AI()->Talk(SAY_PLEDGE_ALLEGIANCE, player);
                }
            }
        }

        return false;
    }
};

class at_zulgurub_edge_of_madness_speech : public OnlyOnceAreaTriggerScript
{
public:
    at_zulgurub_edge_of_madness_speech() : OnlyOnceAreaTriggerScript("at_zulgurub_edge_of_madness_speech") {}

    bool _OnTrigger(Player* player, const AreaTrigger* /*at*/) override
    {
        if (InstanceScript* instance = player->GetInstanceScript())
        {
            // Instance map's enormous, Hakkar's GRID is not loaded by the time players enter.
           // Without this, the creature never says anything, because it doesn't load in time.
            player->GetMap()->LoadGrid(-11783.99f, -1655.27f);

            if (Creature* hakkar = ObjectAccessor::GetCreature(*player, instance->GetGuidData(DATA_HAKKAR)))
            {
                if (hakkar->GetAI())
                {
                    hakkar->AI()->Talk(SAY_WORLD_WILL_SUFFER, player);
                }
            }
        }

        return false;
    }
};

class spell_blood_siphon : public SpellScript
{
    PrepareSpellScript(spell_blood_siphon);

    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_BLOOD_SIPHON_DAMAGE, SPELL_BLOOD_SIPHON_HEAL });
    }

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        // Max. 20 targets
        if (!targets.empty())
        {
            Acore::Containers::RandomResize(targets, 20);
        }
    }

    void HandleScriptEffect(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            if (Player* player = GetHitPlayer())
            {
                player->CastSpell(caster, player->HasAura(SPELL_POISONOUS_BLOOD) ? SPELL_BLOOD_SIPHON_DAMAGE : SPELL_BLOOD_SIPHON_HEAL, true);
            }
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_blood_siphon::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_blood_siphon::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
    }
};

class spell_hakkar_power_down : public SpellScript
{
    PrepareSpellScript(spell_hakkar_power_down);

    void HandleOnHit()
    {
        if (Unit* caster = GetCaster())
            if (caster->HasAura(SPELL_HAKKAR_POWER))
                caster->RemoveAuraFromStack(SPELL_HAKKAR_POWER);
    }

    void Register() override
    {
        OnHit += SpellHitFn(spell_hakkar_power_down::HandleOnHit);
    }
};

void AddSC_boss_hakkar()
{
    new boss_hakkar();
    new at_zulgurub_entrance_speech();
    new at_zulgurub_bridge_speech();
    new at_zulgurub_temple_speech();
    new at_zulgurub_bloodfire_pit_speech();
    new at_zulgurub_edge_of_madness_speech();
    RegisterSpellScript(spell_blood_siphon);
    RegisterSpellScript(spell_hakkar_power_down);
}
