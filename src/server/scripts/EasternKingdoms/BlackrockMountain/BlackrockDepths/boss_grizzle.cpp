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

enum Grizzle
{
    SPELL_GROUNDTREMOR = 6524,
    SPELL_FRENZY = 8269,
    SPELL_THUNDERSTORM = 51490, 
    EMOTE_FRENZY_KILL = 0
};

enum Timer
{
    TIMER_GROUNDTREMOR = 10000,
    TIMER_FRENZY = 15000,
    TIMER_THUNDERSTORM = 20000 
};

class boss_grizzle : public CreatureScript
{
public:
    boss_grizzle() : CreatureScript("boss_grizzle") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_grizzleAI>(creature);
    }

    struct boss_grizzleAI : public BossAI
    {
        boss_grizzleAI(Creature* creature) : BossAI(creature, DATA_GRIZZLE) {}

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(SPELL_GROUNDTREMOR, 0.2 * (int)TIMER_GROUNDTREMOR);
            events.ScheduleEvent(SPELL_FRENZY, 0.2 * (int)TIMER_FRENZY);
            events.ScheduleEvent(SPELL_THUNDERSTORM, 0.2 * (int)TIMER_THUNDERSTORM); 
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
            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case SPELL_GROUNDTREMOR:
                    if (me->GetDistance2d(me->GetVictim()) < 10.0f)
                        DoCastVictim(SPELL_GROUNDTREMOR);
                    events.ScheduleEvent(SPELL_GROUNDTREMOR, TIMER_GROUNDTREMOR);
                    break;
                case SPELL_FRENZY:
                    DoCastSelf(SPELL_FRENZY);
                    events.ScheduleEvent(SPELL_FRENZY, TIMER_FRENZY);
                    Talk(EMOTE_FRENZY_KILL);
                    break;
                case SPELL_THUNDERSTORM: 
                    DoCastSelf(SPELL_THUNDERSTORM, true);
                    events.ScheduleEvent(SPELL_THUNDERSTORM, TIMER_THUNDERSTORM);
                    break;
                default:
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_grizzle()
{
    new boss_grizzle();
}

