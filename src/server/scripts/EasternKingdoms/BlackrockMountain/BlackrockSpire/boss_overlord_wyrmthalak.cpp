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
    SPELL_BLASTWAVE = 13018,
    SPELL_SHOUT = 23511,
    SPELL_CLEAVE = 20691,
    SPELL_KNOCKAWAY = 20686,
    SPELL_FLAMESTRIKE = 10216,
    SPELL_SCORCH = 10207,
    SPELL_LAVA_LASH = 820354,
    SPELL_TELEPORT_VISUAL = 100182
};

enum Events
{
    EVENT_BLAST_WAVE = 1,
    EVENT_SHOUT = 2,
    EVENT_CLEAVE = 3,
    EVENT_KNOCK_AWAY = 4,
    EVENT_FLAMESTRIKE = 5,
    EVENT_SCORCH = 6,
    EVENT_LAVA_LASH = 7,
    EVENT_TELEPORT = 8
};

enum Adds
{
    NPC_SPIRESTONE_WARLORD          = 9216,
    NPC_SMOLDERTHORN_BERSERKER      = 9268
};

constexpr uint32 CALL_HELP = 0;

const Position SummonLocation1 = {-49.43f, -455.82f, 77.82f, 4.61f};
const Position SummonLocation2 = {-58.48f, -456.29f, 77.82f, 4.613f};

class boss_overlord_wyrmthalak : public CreatureScript
{
public:
    boss_overlord_wyrmthalak() : CreatureScript("boss_overlord_wyrmthalak") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockSpireAI<boss_overlordwyrmthalakAI>(creature);
    }

    struct boss_overlordwyrmthalakAI : public BossAI
    {
        boss_overlordwyrmthalakAI(Creature* creature) : BossAI(creature, DATA_OVERLORD_WYRMTHALAK) { }

        bool Summoned;

        void Reset() override
        {
            events.Reset();
            _Reset();
            Summoned = false;
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _JustEngagedWith();
            events.ScheduleEvent(EVENT_BLAST_WAVE, 25 * IN_MILLISECONDS); // 20 seconds
            events.ScheduleEvent(EVENT_SHOUT, 2 * IN_MILLISECONDS); // 2 seconds
            events.ScheduleEvent(EVENT_CLEAVE, urand(4 * IN_MILLISECONDS, 8 * IN_MILLISECONDS));
            events.ScheduleEvent(EVENT_KNOCK_AWAY, 12 * IN_MILLISECONDS); // 12 seconds
            events.ScheduleEvent(EVENT_FLAMESTRIKE, 15 * IN_MILLISECONDS); // 15 seconds
            events.ScheduleEvent(EVENT_SCORCH, 10 * IN_MILLISECONDS); // 10 seconds
            events.ScheduleEvent(EVENT_LAVA_LASH, 8 * IN_MILLISECONDS); // 8 seconds
            events.ScheduleEvent(EVENT_TELEPORT, 6 * IN_MILLISECONDS); // 1 second
        }

        void JustDied(Unit* /*killer*/) override
        {
            instance->SetBossState(DATA_OVERLORD_WYRMTHALAK, DONE);
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

            if (!Summoned && HealthBelowPct(51))
            {
                Talk(CALL_HELP);
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 150, true))
                {
                    if (Creature* warlord = me->SummonCreature(NPC_SPIRESTONE_WARLORD, SummonLocation1, TEMPSUMMON_TIMED_DESPAWN, 300 * IN_MILLISECONDS))
                        warlord->AI()->AttackStart(target);
                    if (Creature* berserker = me->SummonCreature(NPC_SMOLDERTHORN_BERSERKER, SummonLocation2, TEMPSUMMON_TIMED_DESPAWN, 300 * IN_MILLISECONDS))
                        berserker->AI()->AttackStart(target);
                    Summoned = true;
                }
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_BLAST_WAVE:
                        DoCastVictim(SPELL_BLASTWAVE);
                        events.ScheduleEvent(EVENT_BLAST_WAVE, 25 * IN_MILLISECONDS); // 20 seconds 
                        break;
                    case EVENT_SHOUT:
                        DoCastVictim(SPELL_SHOUT);
                        events.ScheduleEvent(EVENT_SHOUT, 10 * IN_MILLISECONDS); // 10 seconds 
                        break;
                    case EVENT_CLEAVE:
                        DoCastVictim(SPELL_CLEAVE);
                        events.ScheduleEvent(EVENT_CLEAVE, 7 * IN_MILLISECONDS); // 7 seconds 
                        break;
                    case EVENT_KNOCK_AWAY:
                        DoCastVictim(SPELL_KNOCKAWAY);
                        events.ScheduleEvent(EVENT_KNOCK_AWAY, 14 * IN_MILLISECONDS); // 14 seconds
                        break;
                    case EVENT_FLAMESTRIKE:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 50.0f, false)) 
                        {
                            float x, y, z;
                            target->GetPosition(x, y, z);
                            me->CastSpell(x, y, z, SPELL_FLAMESTRIKE, true);
                        }
                        events.ScheduleEvent(EVENT_FLAMESTRIKE, urand(10 * IN_MILLISECONDS, 20 * IN_MILLISECONDS)); // 10-20 seconds
                        break;
                    case EVENT_SCORCH:
                        DoCastRandomTarget(SPELL_SCORCH, 0, 35.0f, false, true);
                        events.ScheduleEvent(EVENT_SCORCH, urand(8 * IN_MILLISECONDS, 12 * IN_MILLISECONDS)); // 8-12 seconds
                        break;
                    case EVENT_LAVA_LASH:
                        DoCastVictim(SPELL_LAVA_LASH, true);
                        events.ScheduleEvent(EVENT_LAVA_LASH, urand(10 * IN_MILLISECONDS, 15 * IN_MILLISECONDS));  // 10-15 seconds
                        break;
                    case EVENT_TELEPORT:
                    {
                        const Position TeleportLocation = { -52.944023132324f, -405.62515258789f, 77.759742736816f, 4.6943874359131f };

                        me->Yell("There shall be  no escape!", LANG_UNIVERSAL);

                        std::list<Unit*> targets;
                        Acore::AnyUnitInObjectRangeCheck checker(me, 60.0f); // Adjust range as needed
                        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> searcher(me, targets, checker);
                        Cell::VisitAllObjects(me, searcher, 60.0f); // Adjust range as needed
                        for (Unit* target : targets)
                        {
                            if (target->IsInCombat() && target->IsAlive())
                            {
                                target->CastSpell(target, SPELL_TELEPORT_VISUAL, true);
                                target->NearTeleportTo(TeleportLocation.GetPositionX() + frand(-3.0f, 3.0f), TeleportLocation.GetPositionY() + frand(-3.0f, 3.0f), TeleportLocation.GetPositionZ(), TeleportLocation.GetOrientation());
                            }
                        }

                        me->CastSpell(me, SPELL_TELEPORT_VISUAL, true);
                        me->NearTeleportTo(TeleportLocation.GetPositionX(), TeleportLocation.GetPositionY(), TeleportLocation.GetPositionZ(), TeleportLocation.GetOrientation());
                        
                        me->CastSpell(me, 833020, false);

                        events.ScheduleEvent(EVENT_TELEPORT, urand(38 * IN_MILLISECONDS, 50 * IN_MILLISECONDS)); 
                        break;
                    }
                }
            }
            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_overlordwyrmthalak()
{
    new boss_overlord_wyrmthalak();
}
