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
    SPELL_WHIRLWIND = 15589,
    SPELL_MORTALSTRIKE = 15708,
    SPELL_BLOODLUST = 21049,
    SPELL_SHOCKWAVE = 46968 
};

enum Timers
{
    TIMER_WHIRLWIND = 12000,
    TIMER_MORTAL = 18000,
    TIMER_BLOODLUST = 25000,
    TIMER_SHOCKWAVE = 16000 
};

class boss_gorosh_the_dervish : public CreatureScript
{
public:
    boss_gorosh_the_dervish() : CreatureScript("boss_gorosh_the_dervish") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_gorosh_the_dervishAI>(creature);
    }

    struct boss_gorosh_the_dervishAI : public BossAI
    {
        boss_gorosh_the_dervishAI(Creature* creature) : BossAI(creature, DATA_GOROSH) { }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(SPELL_WHIRLWIND, TIMER_WHIRLWIND);
            events.ScheduleEvent(SPELL_MORTALSTRIKE, TIMER_MORTAL);
            events.ScheduleEvent(SPELL_BLOODLUST, TIMER_BLOODLUST);
            events.ScheduleEvent(SPELL_SHOCKWAVE, TIMER_SHOCKWAVE); 
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
                case SPELL_WHIRLWIND:
                    DoCastVictim(SPELL_WHIRLWIND);
                    events.ScheduleEvent(SPELL_WHIRLWIND, TIMER_WHIRLWIND);
                    break;
                case SPELL_MORTALSTRIKE:
                    DoCastVictim(SPELL_MORTALSTRIKE);
                    events.ScheduleEvent(SPELL_MORTALSTRIKE, TIMER_MORTAL);
                    break;
                case SPELL_BLOODLUST:
                    DoCastSelf(SPELL_BLOODLUST);
                    events.ScheduleEvent(SPELL_BLOODLUST, TIMER_BLOODLUST);
                    break;
                case SPELL_SHOCKWAVE: 
                    DoCastAOE(SPELL_SHOCKWAVE);
                    events.ScheduleEvent(SPELL_SHOCKWAVE, TIMER_SHOCKWAVE);
                    break;
                default:
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_gorosh_the_dervish()
{
    new boss_gorosh_the_dervish();
}
