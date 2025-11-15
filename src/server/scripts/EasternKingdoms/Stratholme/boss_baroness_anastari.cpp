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
#include "InstanceScript.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "TaskScheduler.h"
#include "stratholme.h"

enum Spells
{
    SPELL_BANSHEEWAIL = 16565,
    SPELL_BANSHEECURSE = 16867,
    SPELL_SILENCE = 18327,
    SPELL_SHADOW_BOLT_VOLLEY = 15245  
};

class boss_baroness_anastari : public CreatureScript
{
public:
    boss_baroness_anastari() : CreatureScript("boss_baroness_anastari") { }

    struct boss_baroness_anastariAI : public BossAI
    {
        boss_baroness_anastariAI(Creature* creature) : BossAI(creature, TYPE_ZIGGURAT1)
        {
        }

        void Reset() override
        {
            _scheduler.CancelAll();

            _scheduler.SetValidator([this]
                {
                    return !me->HasUnitState(UNIT_STATE_CASTING);
                });
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _scheduler.Schedule(1s, [this](TaskContext context) {
                DoCastVictim(SPELL_BANSHEEWAIL);
                context.Repeat(4s);
                })
                .Schedule(11s, [this](TaskContext context) {
                    DoCastVictim(SPELL_BANSHEECURSE);
                    context.Repeat(18s);
                    })
                    .Schedule(13s, [this](TaskContext context) {
                        DoCastVictim(SPELL_SILENCE);
                        context.Repeat(13s);
                        })
                        .Schedule(15s, [this](TaskContext context) {
                            DoCastAOE(SPELL_SHADOW_BOLT_VOLLEY, true);
                            context.Repeat(10s);
                            });
        }

        void JustDied(Unit* /*killer*/) override
        {
            instance->SetData(TYPE_ZIGGURAT1, IN_PROGRESS);
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
            {
                return;
            }

            _scheduler.Update(diff,
                std::bind(&ScriptedAI::DoMeleeAttackIfReady, this));
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetStratholmeAI<boss_baroness_anastariAI>(creature);
    }
};

void AddSC_boss_baroness_anastari()
{
    new boss_baroness_anastari;
}

