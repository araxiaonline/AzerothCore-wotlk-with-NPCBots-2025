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
#include "CreatureTextMgr.h"
#include "GameObjectAI.h"
#include "GameObjectScript.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"
#include "ruins_of_ahnqiraj.h"

enum Spells
{
    SPELL_MORTAL_WOUND = 25646,
    SPELL_SAND_TRAP = 25648,
    SPELL_ENRAGE = 26527,
    SPELL_SUMMON_PLAYER = 26446,
    SPELL_WIDE_SLASH = 25814,
    SPELL_THRASH = 3391
};

enum Texts
{
    SAY_KURINNAXX_DEATH = 5 // Yell by 'Ossirian the Unscarred'
};

struct boss_kurinnaxx : public BossAI
{
    boss_kurinnaxx(Creature* creature) : BossAI(creature, DATA_KURINNAXX) {}

    void InitializeAI() override
    {
        me->m_CombatDistance = 50.0f;
    }

    void JustEngagedWith(Unit* who) override
    {
        BossAI::JustEngagedWith(who);

        scheduler.Schedule(8s, 10s, [this](TaskContext context)
            {
                DoCastVictim(SPELL_MORTAL_WOUND);
                context.Repeat(8s, 10s);
            }).Schedule(5s, 15s, [this](TaskContext context)
                {
                    CastSpellOnRandomTarget(SPELL_SAND_TRAP, 100.f);
                    context.Repeat(10s, 21s);
                }).Schedule(10s, 15s, [this](TaskContext context)
                    {
                        DoCastSelf(SPELL_WIDE_SLASH);
                        context.Repeat(12s, 15s);
                    }).Schedule(16s, [this](TaskContext context)
                        {
                            DoCastSelf(SPELL_THRASH);
                            context.Repeat(16s);
                        });

                    ScheduleHealthCheckEvent(30, [&]
                        {
                            DoCastSelf(SPELL_ENRAGE);
                        });
    }

    void JustDied(Unit* killer) override
    {
       
        if (killer)
        {
            killer->GetMap()->LoadGrid(-9502.80f, 2042.65f); // Ossirian grid
            killer->GetMap()->LoadGrid(-8538.17f, 1486.09f); // Andorov run path grid

            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                if (Creature* creature = player->SummonCreature(NPC_ANDOROV, -8538.177f, 1486.0956f, 32.39054f, 3.7638654f, TEMPSUMMON_CORPSE_DESPAWN, 0))
                {
                    creature->setActive(true);
                }
            }
        }

        if (Creature* ossirian = instance->GetCreature(DATA_OSSIRIAN))
        {
            ossirian->setActive(true);
            if (ossirian->GetAI())
                ossirian->AI()->Talk(SAY_KURINNAXX_DEATH);
        }
        BossAI::JustDied(killer);
        DoCastSelf(875167, true);
        Map::PlayerList const& players = me->GetMap()->GetPlayers();
        for (auto const& playerPair : players)
        {
            Player* player = playerPair.GetSource();
            if (player)
            {
                DistributeChallengeRewards(player, me, 10, false);
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
};

struct go_sand_trap : public GameObjectAI
{
    go_sand_trap(GameObject* go) : GameObjectAI(go) { }

    void Reset() override
    {
        _scheduler.Schedule(5s, [this](TaskContext /*context*/)
            {
                if (InstanceScript* instance = me->GetInstanceScript())
                    if (Creature* kurinnaxx = instance->GetCreature(DATA_KURINNAXX))
                        me->Use(kurinnaxx);
            });
    }

    void UpdateAI(uint32 const diff) override
    {
        _scheduler.Update(diff);
    }

protected:
    TaskScheduler _scheduler;
};

void AddSC_boss_kurinnaxx()
{
    RegisterRuinsOfAhnQirajCreatureAI(boss_kurinnaxx);
    RegisterGameObjectAI(go_sand_trap);
}
