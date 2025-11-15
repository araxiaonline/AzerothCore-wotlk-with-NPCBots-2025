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
    SPELL_SHADOWWORDPAIN                                   = 10894,
    SPELL_MANABURN                                         = 14033,
    SPELL_PSYCHICSCREAM                                    = 8122,
    SPELL_SHADOWSHIELD                                     = 22417,
    SPELL_FRENZY                                           = 8269,
    SPELL_BLOOD_BOIL                                       = 48721,
    SPELL_TELEPORT_VISUAL                                  = 100182,
};

struct TeleportPosition
{
    float x, y, z, orientation;
};

const TeleportPosition TeleportLocations[] =
{
    {359.717712f, -160.797531f, -64.949005f, 1.819360f},
    {347.160706f, -114.397751f, -64.948936f, 4.950481f},
    {372.198425f, -107.908203f, -64.949028f, 4.954361f},
    {386.720612f, -156.290421f, -64.949020f, 1.742074f},
    {310.598145f, -146.435532f, -70.386200f, 0.220587f}
};

class boss_high_interrogator_gerstahn : public CreatureScript
{
public:
    boss_high_interrogator_gerstahn() : CreatureScript("boss_high_interrogator_gerstahn") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_high_interrogator_gerstahnAI>(creature);
    }

    struct boss_high_interrogator_gerstahnAI : public ScriptedAI
    {
        boss_high_interrogator_gerstahnAI(Creature* creature) : ScriptedAI(creature) { }

        uint32 ShadowWordPain_Timer;
        uint32 ManaBurn_Timer;
        uint32 PsychicScream_Timer;
        uint32 ShadowShield_Timer;
        uint32 BloodBoil_Timer;
        uint32 Teleport_Timer;

        void Reset() override
        {
            ShadowWordPain_Timer = 4000;
            ManaBurn_Timer = 14000;
            PsychicScream_Timer = 32000;
            ShadowShield_Timer = 8000;
            BloodBoil_Timer = 15000;
            Teleport_Timer = 20000;
        }
        

        void JustEngagedWith(Unit* /*who*/) override
        {
            me->Yell("Ah, fresh subjects for interrogation. Your secrets will not remain hidden for long.", LANG_UNIVERSAL);
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->Yell("My questions... remain unanswered. The Dark Iron... will know of this...", LANG_UNIVERSAL);
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
                return;

            if (me->HealthBelowPct(30) && !me->HasAura(SPELL_FRENZY))
            {
                DoCast(me, SPELL_FRENZY, true);
            }
            // Teleport Player 
            if (Teleport_Timer <= diff)
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, true))
                {
                    DoCast(target, SPELL_TELEPORT_VISUAL, true);

                    const TeleportPosition& pos = TeleportLocations[urand(0, (sizeof(TeleportLocations) / sizeof(TeleportPosition)) - 1)];
                    target->NearTeleportTo(pos.x, pos.y, pos.z, pos.orientation);

                    me->Yell("You seem too comfortable. Let's change your perspective!", LANG_UNIVERSAL);

                    // Spawn 3 Dark Iron Slavers near the teleport location
                    for (int i = 0; i < 3; ++i)
                    {
                        // Randomize the spawn position around the teleport location within 3f radius
                        float angle = frand(0, M_PI * 2);
                        float dist = frand(0, 3.0f);
                        float x = pos.x + dist * cos(angle);
                        float y = pos.y + dist * sin(angle);

                        me->SummonCreature(5844, x, y, pos.z, 0, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 30000);
                    }
                }
                Teleport_Timer = 20000;
            }
            else Teleport_Timer -= diff;

            // Blood Boil Timer
            if (BloodBoil_Timer <= diff)
            {
                DoCast(me, SPELL_BLOOD_BOIL, true);
                BloodBoil_Timer = 12000; 
            }
            else BloodBoil_Timer -= diff;

            //ShadowWordPain_Timer
            if (ShadowWordPain_Timer <= diff)
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, false))
                    DoCast(target, SPELL_SHADOWWORDPAIN);
                ShadowWordPain_Timer = 7000;
            }
            else ShadowWordPain_Timer -= diff;

            //ManaBurn_Timer
            if (ManaBurn_Timer <= diff)
            {
                if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100, false))
                    DoCast(target, SPELL_MANABURN);
                ManaBurn_Timer = 10000;
            }
            else ManaBurn_Timer -= diff;

            //PsychicScream_Timer
            if (PsychicScream_Timer <= diff)
            {
                DoCastVictim(SPELL_PSYCHICSCREAM);
                PsychicScream_Timer = 30000;
            }
            else PsychicScream_Timer -= diff;

            //ShadowShield_Timer
            if (ShadowShield_Timer <= diff)
            {
                DoCast(me, SPELL_SHADOWSHIELD);
                ShadowShield_Timer = 25000;
            }
            else ShadowShield_Timer -= diff;

            DoMeleeAttackIfReady();
        }
    };
};

void AddSC_boss_high_interrogator_gerstahn()
{
    new boss_high_interrogator_gerstahn();
}
