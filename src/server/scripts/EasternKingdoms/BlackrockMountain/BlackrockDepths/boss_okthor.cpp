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
#include "blackrock_depths.h"

enum Spells
{
    SPELL_ARCANE_BOLT       = 13748,
    SPELL_ARCANE_EXPLOSION  = 10201,
    SPELL_POLYMORPH         = 15534,
    SPELL_SLOW              = 19137,
    SPELL_ARCANE_BARRAGE    = 44425
};

enum Timers
{
    TIMER_ARCANE_BOLT        = 7000,
    TIMER_ARCANE_EXPLOSION  = 14000,
    TIMER_POLYMORPH         = 12000,
    TIMER_SLOW              = 15000,
    TIMER_ARCANE_BARRAGE    = 8000
};

class boss_okthor : public CreatureScript
{
public:
    boss_okthor() : CreatureScript("boss_okthor") {}

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_okthorAI>(creature);
    }

    struct boss_okthorAI : public BossAI
    {
        boss_okthorAI(Creature* creature) : BossAI(creature, DATA_OKTHOR) {}

        uint32 nextArcaneExplosionTime;

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(SPELL_ARCANE_BOLT, 0.2 * (int) TIMER_ARCANE_BOLT);
            events.ScheduleEvent(SPELL_ARCANE_EXPLOSION, 0.2 * (int) TIMER_ARCANE_EXPLOSION);
            events.ScheduleEvent(SPELL_POLYMORPH, 0.2 * (int) TIMER_POLYMORPH);
            events.ScheduleEvent(SPELL_SLOW, 500);
            events.ScheduleEvent(SPELL_ARCANE_BARRAGE, TIMER_ARCANE_BARRAGE);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (players.begin() != players.end())
            {
                uint32 baseRewardLevel = 1;
                bool isDungeon = me->GetMap()->IsDungeon();

                Player* player = players.begin()->GetSource();
                if (player)
                {
                    DistributeChallengeRewards(player, me, baseRewardLevel, isDungeon);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
            {
                return;
            }
            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
            {
                return;
            }
            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case SPELL_ARCANE_BOLT:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
                    {
                        DoCast(target, SPELL_ARCANE_BOLT);
                    }
                    events.ScheduleEvent(SPELL_ARCANE_BOLT, urand(TIMER_ARCANE_BOLT - 2000, TIMER_ARCANE_BOLT + 2000));
                    break;
                case SPELL_ARCANE_EXPLOSION:
                    DoCast(SPELL_ARCANE_EXPLOSION);
                    events.ScheduleEvent(SPELL_ARCANE_EXPLOSION, TIMER_ARCANE_EXPLOSION);
                    break;
                case SPELL_POLYMORPH:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
                    {
                        DoCast(target, SPELL_POLYMORPH, true);
                    }
                    events.ScheduleEvent(SPELL_POLYMORPH, urand(TIMER_POLYMORPH - 2000, TIMER_POLYMORPH + 2000));
                    break;
                case SPELL_SLOW:
                    if (me->GetDistance2d(me->GetVictim()) < 50.0f)
                    {
                        DoCast(SPELL_SLOW);
                    }
                    events.ScheduleEvent(SPELL_SLOW, TIMER_SLOW);
                    break;
                case SPELL_ARCANE_BARRAGE:
                    if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
                    {
                        DoCast(target, SPELL_ARCANE_BARRAGE, true);
                    }
                    events.ScheduleEvent(SPELL_ARCANE_BARRAGE, urand(SPELL_ARCANE_BARRAGE - 2000, SPELL_ARCANE_BARRAGE + 2000));
                    break;

                default:
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_okthor()
{
    new boss_okthor();
}
