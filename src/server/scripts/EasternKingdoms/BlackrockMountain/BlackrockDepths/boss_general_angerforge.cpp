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
    SPELL_MIGHTYBLOW                                       = 14099,
    SPELL_HAMSTRING                                        = 9080,
    SPELL_CLEAVE                                           = 20691
};

const Position anvilrageSpawnPositions[] = {
    {675.304199f, 40.174721f, -58.529747f, 3.095478f},
    {675.293152f, 34.244930f, -55.226852f, 3.095478f},
    {675.084961f, 29.733681f, -51.742348f, 3.095478f},
    {675.317749f, 22.587074f, -50.623539f, 3.095478f}
};

class boss_general_angerforge : public CreatureScript
{
public:
    boss_general_angerforge() : CreatureScript("boss_general_angerforge") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_general_angerforgeAI>(creature);
    }

    struct boss_general_angerforgeAI : public ScriptedAI
    {
        boss_general_angerforgeAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 MightyBlow_Timer;
        uint32 HamString_Timer;
        uint32 Cleave_Timer;
        uint32 Adds_Timer;
        bool Medics;

        void Reset() override
        {
            MightyBlow_Timer = 8000;
            HamString_Timer = 12000;
            Cleave_Timer = 16000;
            Adds_Timer = 0;
            Medics = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            me->Yell("Reservists! Get these interlopers out of here!", LANG_UNIVERSAL);
            for (const Position& pos : anvilrageSpawnPositions)
            {
                if (Creature* reservist = me->SummonCreature(8901, pos, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 69000))
                {
                    reservist->SetInCombatWithZone(); 
                }
            }
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
        
        void SummonAdds(Unit* victim)
        {
            if (Creature* SummonedAdd = DoSpawnCreature(8901, float(irand(-14, 14)), float(irand(-14, 14)), 0, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 120000))
                SummonedAdd->AI()->AttackStart(victim);
        }

        void SummonMedics(Unit* victim)
        {
            if (Creature* SummonedMedic = DoSpawnCreature(8894, float(irand(-9, 9)), float(irand(-9, 9)), 0, 0, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 120000))
                SummonedMedic->AI()->AttackStart(victim);
        }

        void UpdateAI(uint32 diff) override
        {
            //Return since we have no target
            if (!UpdateVictim())
                return;

            //MightyBlow_Timer
            if (MightyBlow_Timer <= diff)
            {
                DoCastVictim(SPELL_MIGHTYBLOW);
                MightyBlow_Timer = 18000;
            }
            else MightyBlow_Timer -= diff;

            //HamString_Timer
            if (HamString_Timer <= diff)
            {
                DoCastVictim(SPELL_HAMSTRING);
                HamString_Timer = 15000;
            }
            else HamString_Timer -= diff;

            //Cleave_Timer
            if (Cleave_Timer <= diff)
            {
                DoCastVictim(SPELL_CLEAVE);
                Cleave_Timer = 9000;
            }
            else Cleave_Timer -= diff;

            //Adds_Timer
            if (HealthBelowPct(21))
            {
                if (Adds_Timer <= diff)
                {
                    // summon 3 Adds every 25s
                    SummonAdds(me->GetVictim());
                    SummonAdds(me->GetVictim());
                    SummonAdds(me->GetVictim());

                    Adds_Timer = 25000;
                }
                else Adds_Timer -= diff;
            }

            //Summon Medics
            if (!Medics && HealthBelowPct(21))
            {
                SummonMedics(me->GetVictim());
                SummonMedics(me->GetVictim());
                Medics = true;
            }

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_general_angerforge()
{
    new boss_general_angerforge();
}
