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
#include "blackrock_spire.h"

enum Spells
{
    SPELL_FRENZY = 8269,
    SPELL_KNOCK_AWAY = 10101,
    SPELL_CLEAVE = 19632 // Added Cleave spell ID
};

enum Events
{
    EVENT_FRENZY = 1,
    EVENT_KNOCK_AWAY = 2,
    EVENT_CLEAVE = 3 // Added Cleave event
};

class boss_highlord_omokk : public CreatureScript
{
public:
    boss_highlord_omokk() : CreatureScript("boss_highlord_omokk") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<boss_highlordomokkAI>(creature);
    }

    struct boss_highlordomokkAI : public BossAI
    {
        boss_highlordomokkAI(Creature* creature) : BossAI(creature, DATA_HIGHLORD_OMOKK) { }

        void Reset() override
        {
            _Reset();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(EVENT_FRENZY, 20s); // Frenzy will be handled based on HP in UpdateAI
            events.ScheduleEvent(EVENT_KNOCK_AWAY, 18s);
            events.ScheduleEvent(EVENT_CLEAVE, 10s); // Schedule the first Cleave
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            Map::PlayerList const& players = me->GetMap()->GetPlayers();
            if (!players.IsEmpty())
            {
                uint32 baseRewardLevel = 1;
                bool isDungeon = me->GetMap()->IsDungeon();

                for (auto const& playerPair : players)
                {
                    if (Player* player = playerPair.GetSource())
                    {
                        DistributeChallengeRewards(player, me, baseRewardLevel, isDungeon);
                    }
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

            // Check for Frenzy condition based on HP
            if (me->HealthBelowPct(40) && !me->HasAura(SPELL_FRENZY))
            {
                DoCast(me, SPELL_FRENZY);
            }

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_KNOCK_AWAY:
                    DoCastVictim(SPELL_KNOCK_AWAY);
                    events.ScheduleEvent(EVENT_KNOCK_AWAY, 12s);
                    break;
                case EVENT_CLEAVE:
                    DoCastVictim(SPELL_CLEAVE);
                    events.ScheduleEvent(EVENT_CLEAVE, 8s);
                    break;
                default:
                    break;
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_highlordomokk()
{
    new boss_highlord_omokk();
}
