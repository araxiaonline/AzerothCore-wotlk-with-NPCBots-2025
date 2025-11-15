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
#include "ScriptedCreature.h"
#include "blackwing_lair.h"

enum Spells
{
    SPELL_SHADOWFLAME       = 22539,
    SPELL_WINGBUFFET        = 23339,
    SPELL_FLAMEBUFFET       = 23341,
    SPELL_CHAIN_LIGHTNING   = 25439,
};

enum Events
{
    EVENT_SHADOWFLAME = 1,
    EVENT_WINGBUFFET = 2,
    EVENT_FLAMEBUFFET = 3,
    EVENT_CHAIN_LIGHTNING = 4,
    EVENT_SPAWN_WHELPS = 5  
};

const uint32 NPC_CORRUPTED_RED_WHELP = 14022; 


class boss_firemaw : public CreatureScript
{
public:
    boss_firemaw() : CreatureScript("boss_firemaw") { }

    struct boss_firemawAI : public BossAI
    {
        boss_firemawAI(Creature* creature) : BossAI(creature, DATA_FIREMAW) { }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);

            events.ScheduleEvent(EVENT_SHADOWFLAME, 18s);
            events.ScheduleEvent(EVENT_WINGBUFFET, 30s);
            events.ScheduleEvent(EVENT_FLAMEBUFFET, 5s);
            events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand(15000, 18000));
            events.ScheduleEvent(EVENT_SPAWN_WHELPS, 15s);
        }


        void JustDied(Unit* /*killer*/) override
        {
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

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_SHADOWFLAME:
                        DoCastVictim(SPELL_SHADOWFLAME);
                        events.ScheduleEvent(EVENT_SHADOWFLAME, 15s, 25s);
                        break;
                    case EVENT_WINGBUFFET:
                        DoCastVictim(SPELL_WINGBUFFET);
                        if (DoGetThreat(me->GetVictim()))
                            DoModifyThreatByPercent(me->GetVictim(), -75);
                        events.ScheduleEvent(EVENT_WINGBUFFET, 30s);
                        break;
                    case EVENT_FLAMEBUFFET:
                        DoCastVictim(SPELL_FLAMEBUFFET);
                        events.ScheduleEvent(EVENT_FLAMEBUFFET, 5s);
                        break;
                    case EVENT_CHAIN_LIGHTNING:
                        CastSpellOnRandomTarget(SPELL_CHAIN_LIGHTNING, 100.0f, true);
                        events.ScheduleEvent(EVENT_CHAIN_LIGHTNING, urand(15000, 18000));
                        break;
                    case EVENT_SPAWN_WHELPS:
                        SpawnWhelps();
                        events.ScheduleEvent(EVENT_SPAWN_WHELPS, 15s); // Reschedule the event
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
        }

        void CastSpellOnRandomTarget(uint32 spellId, float range, bool triggered)
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
                DoCast(target, spellId, triggered);
            }
        }
        void SpawnWhelps()
        {
            float angleIncrement = 2 * M_PI / 5; 
            float baseAngle = frand(0, 2 * M_PI); 

            for (int i = 0; i < urand(3, 5); ++i)
            {
                float angle = baseAngle + i * angleIncrement;
                float x = me->GetPositionX() + 15.0f * cos(angle);
                float y = me->GetPositionY() + 15.0f * sin(angle);
                float z = me->GetPositionZ();

                if (Creature* whelp = me->SummonCreature(NPC_CORRUPTED_RED_WHELP, x, y, z, 0, TEMPSUMMON_CORPSE_DESPAWN, 60000))
                {
                    whelp->SetInCombatWithZone(); 
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackwingLairAI<boss_firemawAI>(creature);
    }
};

void AddSC_boss_firemaw()
{
    new boss_firemaw();
}
