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

#include "CreatureScript.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "GameObjectScript.h"
#include "InstanceScript.h"
#include "ScriptedCreature.h"
#include "SpellScript.h"
#include "SpellScriptLoader.h"
#include "blackwing_lair.h"

enum Say
{
    SAY_AGGRO               = 0,
    SAY_LEASH               = 1
};

enum Spells
{
    SPELL_CLEAVE            = 26350,
    SPELL_BLASTWAVE         = 23331,
    SPELL_MORTALSTRIKE      = 24573,
    SPELL_KNOCKBACK         = 25778,
    SPELL_SUPPRESSION_AURA  = 22247 // Suppression Device Spell
};

enum Events
{
    EVENT_CLEAVE = 1,
    EVENT_BLASTWAVE = 2,
    EVENT_MORTALSTRIKE = 3,
    EVENT_KNOCKBACK = 4,
    EVENT_CHECK = 5,
    EVENT_SPAWN_ADDS = 6, // New event for spawning adds
    // Suppression Device Events
    EVENT_SUPPRESSION_CAST = 7,
    EVENT_SUPPRESSION_RESET = 8
};

enum Actions
{
    ACTION_DEACTIVATE = 0,
    ACTION_DISARMED   = 1
};

class boss_broodlord : public CreatureScript
{
public:
    boss_broodlord() : CreatureScript("boss_broodlord") { }

    struct boss_broodlordAI : public BossAI
    {
        boss_broodlordAI(Creature* creature) : BossAI(creature, DATA_BROODLORD_LASHLAYER) { }

        EventMap customEvents;

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            Talk(SAY_AGGRO);

            events.ScheduleEvent(EVENT_CLEAVE, 8s);
            events.ScheduleEvent(EVENT_BLASTWAVE, 12s);
            events.ScheduleEvent(EVENT_MORTALSTRIKE, 20s);
            events.ScheduleEvent(EVENT_KNOCKBACK, 30s);
            events.ScheduleEvent(EVENT_SPAWN_ADDS, 50s);
            events.ScheduleEvent(EVENT_CHECK, 1s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            me->Yell("At last, the long nightmare is over...", LANG_UNIVERSAL);

            // Remove aura from Firemaw
            RemoveAuraFromFiremaw();

            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            for (auto const& playerPair : players)
            {
                Player* player = playerPair.GetSource();
                if (player)
                {
                    DistributeChallengeRewards(player, me, 1, false);
                }
            }

            std::list<GameObject*> _goList;
            GetGameObjectListWithEntryInGrid(_goList, me, GO_SUPPRESSION_DEVICE, 200.0f);
            for (auto const& go : _goList)
            {
                go->AI()->DoAction(ACTION_DEACTIVATE);
            }

            instance->SetBossState(DATA_BROODLORD_LASHLAYER, DONE);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
            {
                me->Yell("The strands of fate are severed!", LANG_UNIVERSAL);
            }
        }

        void Reset() override
        {
            BossAI::Reset();
            customEvents.Reset();
            customEvents.ScheduleEvent(1, 10s);  // Custom event to apply aura every 10 seconds
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
            {
                // Apply aura to Firemaw if Broodlord is alive and not in combat
                customEvents.Update(diff);
                if (customEvents.ExecuteEvent() == 1)
                {
                    if (!me->IsInCombat() && me->IsAlive())
                    {
                        ApplyAuraToFiremaw(819753);
                    }
                    customEvents.ScheduleEvent(1, 10s);  // Re-schedule the event
                }
                return;
            }

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE);
                    events.ScheduleEvent(EVENT_CLEAVE, 7s);
                    break;
                case EVENT_BLASTWAVE:
                    DoCastVictim(SPELL_BLASTWAVE);
                    events.ScheduleEvent(EVENT_BLASTWAVE, randtime(21s, 35s));
                    break;
                case EVENT_MORTALSTRIKE:
                    DoCastVictim(SPELL_MORTALSTRIKE);
                    events.ScheduleEvent(EVENT_MORTALSTRIKE, randtime(20s, 30s));
                    break;
                case EVENT_KNOCKBACK:
                    DoCastVictim(SPELL_KNOCKBACK);
                    if (DoGetThreat(me->GetVictim()))
                        DoModifyThreatByPercent(me->GetVictim(), -50);
                    events.ScheduleEvent(EVENT_KNOCKBACK, randtime(17s, 30s));
                    break;
                case EVENT_SPAWN_ADDS:
                    SpawnAdds();
                    events.ScheduleEvent(EVENT_SPAWN_ADDS, 50s);
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void SpawnAdds()
        {
            float x, y, z, o;
            me->GetPosition(x, y, z, o);
            float add_x, add_y, add_z;
            do
            {
                add_x = x + frand(-10, 10);
                add_y = y + frand(-10, 10);
                add_z = z;
            } while (!me->IsWithinLOS(add_x, add_y, add_z));

            if (Creature* add = me->SummonCreature(12463, add_x, add_y, add_z, o, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 180000))
            {
                if (Player* nearestPlayer = add->SelectNearestPlayer(100))
                {
                    add->AI()->AttackStart(nearestPlayer);
                }
            }
        }

        void ApplyAuraToFiremaw(uint32 spellId)
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, 11983, 100.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12457, 100.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12459, 100.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 13996, 100.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12461, 100.0f);
            for (Creature* creature : creatures)
            {
                creature->AddAura(spellId, creature);
            }
        }

        void RemoveAuraFromFiremaw()
        {
            std::list<Creature*> creatures;
            GetCreatureListWithEntryInGrid(creatures, me, 11983, 200.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12457, 200.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12459, 200.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 13996, 200.0f);
            GetCreatureListWithEntryInGrid(creatures, me, 12461, 200.0f);
            for (Creature* creature : creatures)
            {
                creature->RemoveAurasDueToSpell(819753);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackwingLairAI<boss_broodlordAI>(creature);
    }
};

class go_suppression_device : public GameObjectScript
{
    public:
        go_suppression_device() : GameObjectScript("go_suppression_device") { }

        void OnLootStateChanged(GameObject* go, uint32 state, Unit* /*unit*/) override
        {
            switch (state)
            {
                case GO_JUST_DEACTIVATED: // This case prevents the Gameobject despawn by Disarm Trap
                    go->SetLootState(GO_READY);
                    [[fallthrough]];
                case GO_ACTIVATED:
                    go->AI()->DoAction(ACTION_DISARMED);
                    break;
            }
        }

        struct go_suppression_deviceAI : public GameObjectAI
        {
            go_suppression_deviceAI(GameObject* go) : GameObjectAI(go), _instance(go->GetInstanceScript()), _active(true) { }

            void InitializeAI() override
            {
                if (_instance->GetBossState(DATA_BROODLORD_LASHLAYER) == DONE)
                {
                    Deactivate();
                    return;
                }

                _events.ScheduleEvent(EVENT_SUPPRESSION_CAST, 5s);
            }

            void UpdateAI(uint32 diff) override
            {
                _events.Update(diff);

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_SUPPRESSION_CAST:
                            if (me->GetGoState() == GO_STATE_READY)
                            {
                                me->CastSpell(nullptr, SPELL_SUPPRESSION_AURA);
                                me->SendCustomAnim(0);
                            }
                            _events.ScheduleEvent(EVENT_SUPPRESSION_CAST, 5s);
                            break;
                        case EVENT_SUPPRESSION_RESET:
                            Activate();
                            break;
                    }
                }
            }

            void DoAction(int32 action) override
            {
                if (action == ACTION_DEACTIVATE)
                {
                    _events.CancelEvent(EVENT_SUPPRESSION_RESET);
                }
                else if (action == ACTION_DISARMED)
                {
                    Deactivate();
                    _events.CancelEvent(EVENT_SUPPRESSION_CAST);

                    if (_instance->GetBossState(DATA_BROODLORD_LASHLAYER) != DONE)
                    {
                        _events.ScheduleEvent(EVENT_SUPPRESSION_RESET, 30s, 120s);
                    }
                }
            }

            void Activate()
            {
                if (_active)
                    return;
                _active = true;
                if (me->GetGoState() == GO_STATE_ACTIVE)
                    me->SetGoState(GO_STATE_READY);
                me->SetLootState(GO_READY);
                me->RemoveGameObjectFlag(GO_FLAG_NOT_SELECTABLE);
                _events.ScheduleEvent(EVENT_SUPPRESSION_CAST, 5s);
                me->Respawn();
            }

            void Deactivate()
            {
                if (!_active)
                    return;
                _active = false;
                me->SetGoState(GO_STATE_ACTIVE);
                me->SetGameObjectFlag(GO_FLAG_NOT_SELECTABLE);
                _events.CancelEvent(EVENT_SUPPRESSION_CAST);
            }

        private:
            InstanceScript* _instance;
            EventMap _events;
            bool _active;
        };

        GameObjectAI* GetAI(GameObject* go) const override
        {
            return new go_suppression_deviceAI(go);
        }
};

class spell_suppression_aura : public SpellScript
{
    PrepareSpellScript(spell_suppression_aura);

    void FilterTargets(std::list<WorldObject*>& targets)
    {
        targets.remove_if([&](WorldObject* target) -> bool
        {
            Unit* unit = target->ToUnit();
            return !unit || unit->HasStealthAura();
        });
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_suppression_aura::FilterTargets, EFFECT_ALL, TARGET_UNIT_DEST_AREA_ENEMY);
    }
};

void AddSC_boss_broodlord()
{
    new boss_broodlord();
    new go_suppression_device();
    RegisterSpellScript(spell_suppression_aura);
}
