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
    SPELL_SHADOWFLAME           = 22539,
    SPELL_WINGBUFFET            = 23339,
    SPELL_SHADOWOFEBONROC       = 23340,
    SPELL_CHARRED_EARTH         = 100272,
    SPELL_SUMMON_PLAYER         = 20279
};

enum Events
{
    EVENT_SHADOWFLAME           = 1,
    EVENT_WINGBUFFET            = 2,
    EVENT_SHADOWOFEBONROC       = 3,
    EVENT_CHARRED_EARTH         = 4
  //  EVENT_SUMMON_PLAYER         = 5
};

class boss_ebonroc : public CreatureScript
{
public:
    boss_ebonroc() : CreatureScript("boss_ebonroc") { }

    struct boss_ebonrocAI : public BossAI
    {
        boss_ebonrocAI(Creature* creature) : BossAI(creature, DATA_EBONROC) { }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != WAYPOINT_MOTION_TYPE || id != 12)
            {
                return;
            }

            me->GetMotionMaster()->MoveRandom(10.f);

            me->m_Events.AddEventAtOffset([this]()
            {
                me->GetMotionMaster()->Initialize();
            }, 15s);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);

            events.ScheduleEvent(EVENT_SHADOWFLAME, 18s);
            events.ScheduleEvent(EVENT_WINGBUFFET, 25s);
            events.ScheduleEvent(EVENT_SHADOWOFEBONROC, 8s, 10s);
            events.ScheduleEvent(EVENT_CHARRED_EARTH, urand(15000, 18000));
        //    events.ScheduleEvent(EVENT_SUMMON_PLAYER, urand(18000, 20000));
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
                        events.ScheduleEvent(EVENT_WINGBUFFET, 25s);
                        break;
                    case EVENT_SHADOWOFEBONROC:
                        DoCastVictim(SPELL_SHADOWOFEBONROC);
                        events.ScheduleEvent(EVENT_SHADOWOFEBONROC, 8s, 10s);
                        break;
                    case EVENT_CHARRED_EARTH:
                        CastSpellOnRandomTarget(SPELL_CHARRED_EARTH, 100.0f);
                        events.ScheduleEvent(EVENT_CHARRED_EARTH, urand(15000, 18000));
                        break;
                //    case EVENT_SUMMON_PLAYER:
                //        CastSpellOnRandomTarget(SPELL_SUMMON_PLAYER, 100.0f);
                //        events.ScheduleEvent(EVENT_SUMMON_PLAYER, urand(18000, 20000));
                //        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            DoMeleeAttackIfReady();
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

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackwingLairAI<boss_ebonrocAI>(creature);
    }
};

void AddSC_boss_ebonroc()
{
    new boss_ebonroc();
}
