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
#include <vector>

enum Spells
{
    // Old fireblast value 15573
    SPELL_FIREBLAST         = 13342,
    SPELL_BURNING_SPIRIT    = 14744,
    SPELL_FIREBOLT_VOLLEY   = 15243,
};

enum AmbassadorEvents
{
    AGGRO_TEXT              = 0,
    EVENT_SPELL_FIREBLAST   = 1,
    EVENT_SUMMON_SPIRITS    = 2,
    EVENT_KILL_SPIRIT = 3,
    EVENT_FIREBOLT_VOLLEY = 4
};

const uint32 NPC_FIRE_SPIRIT = 9178;

const Position SummonPositions[7] =
{
    {1028.786987f, -224.787186f, -61.840500f, 3.617599f},
    {1045.144775f, -241.108292f, -61.967422f, 3.617599f},
    {1028.852905f, -257.484222f, -61.981380f, 3.617599f},
    {1012.461060f, -273.803406f, -61.994171f, 3.617599f},
    { 995.503052f, -257.563751f, -62.013153f, 3.617599f},
    { 979.358704f, -240.535309f, -61.983044f, 3.617599f},
    {1012.252747f, -206.696487f, -61.980618f, 3.617599f},
};

std::vector<int> gobjectDwarfRunesEntry { 170578, 170579, 170580, 170581, 170582, 170583, 170584 };

class boss_ambassador_flamelash : public CreatureScript
{
public:
    boss_ambassador_flamelash() : CreatureScript("boss_ambassador_flamelash") { }

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<boss_ambassador_flamelashAI>(creature);
    }

    struct boss_ambassador_flamelashAI : public BossAI
    {
        boss_ambassador_flamelashAI(Creature* creature) : BossAI(creature, BOSS_AMBASSADOR_FLAMELASH), summons(me) { }

        EventMap _events;
        SummonList summons;
        Position spawnPosition;

        // This will allow to find a valid position to spawn them
        std::vector<int> validPosition;
        bool foundValidPosition = false;

        void JustSummoned(Creature* cr) override { summons.Summon(cr); }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case EVENT_SUMMON_SPIRITS:
                    _events.ScheduleEvent(EVENT_SUMMON_SPIRITS, 12s, 14s);
                    break;
            }
        }

        void Reset() override
        {
            _events.Reset();
            summons.DespawnAll();
            TurnRunes(false);
            foundValidPosition = false;
            validPosition.clear();
        }

        void TurnRunes(bool mode)
        {
            // Active makes the runes burn, ready turns them off
            GOState state = mode ? GO_STATE_ACTIVE : GO_STATE_READY;

            for (int RuneEntry : gobjectDwarfRunesEntry)
                if (GameObject* dwarfRune = me->FindNearestGameObject(RuneEntry, 200.0f))
                    dwarfRune->SetGoState(state);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            spawnPosition = me->GetPosition();
            _events.ScheduleEvent(EVENT_SPELL_FIREBLAST, 2s);

            // Spawn 7 Embers initially
            for (int i = 0; i < 4; ++i)
                _events.ScheduleEvent(EVENT_SUMMON_SPIRITS, 4s);

            // Activate the runes (Start burning)
            TurnRunes(true);

            Talk(AGGRO_TEXT);

            _events.ScheduleEvent(EVENT_FIREBOLT_VOLLEY, 12s); 
        }

        void JustDied(Unit* /*killer*/) override
        {
            TurnRunes(false);
            _events.Reset();
            summons.DespawnAll();
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

        int getValidRandomPosition()
        {
            /* Generate a random position which
             * have not been used in 4 summonings.
             * Since we are calling the event whenever the Spirit
             * dies and not all at the time, we need to save at
             * least 4 positions until reseting the vector
            */

            // Searching a new position so reset this bool check
            foundValidPosition = false;
            int randomPosition;

            while (!foundValidPosition)
            {
                /* When we have summoned 4 creatures, reset the vector
                 * so we can summon new spirits in other positions.*/
                if (validPosition.size() == 4)
                    validPosition.clear();

                // The random ranges from the position 0 to the position 6
                randomPosition = urand(0, 6);

                // When we have an empty vector we can use any random position generated.
                if (validPosition.empty())
                    foundValidPosition = true;

                /* This check is done to avoid running the vector
                 * when it is empty. Because if it is empty, then any
                 * position can be used to summon Spirits.
                 */
                if (!foundValidPosition)
                {
                    // Check every position inside the vector
                    for (int pos : validPosition)
                    {
                        // If the random is different, we found a temporary true,
                        // until we find one that is equal, which means it has been used.
                        if (pos != randomPosition)
                            foundValidPosition = true;
                        else
                        {
                            foundValidPosition = false;
                            break;
                        }
                    }
                }
            }

            // We found a valid position. Save it and return it to summon.
            validPosition.emplace_back(randomPosition);
            return randomPosition;
        }

        void SummonSpirits()
        {
            // Make the Spirits chase Ambassador Flamelash
            me->SummonCreature(NPC_FIRE_SPIRIT, SummonPositions[getValidRandomPosition()], TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60 * IN_MILLISECONDS);
            _events.ScheduleEvent(EVENT_SUMMON_SPIRITS, 12s, 14s);
        }

        void UpdateAI(uint32 diff) override
        {
            if (!UpdateVictim())
                return;

            // Check if the boss has moved more than 32 yards from his spawn position
            if (me->GetDistance(spawnPosition) > 32.0f)
            {
                EnterEvadeMode(EVADE_REASON_OTHER); // Trigger a reset if the boss is too far from the spawn
                return; // Skip the rest of the update
            }

            _events.Update(diff);

            // Use a while loop to process each event in the queue
            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SPELL_FIREBLAST:
                    DoCastVictim(SPELL_FIREBLAST);
                    _events.ScheduleEvent(EVENT_SPELL_FIREBLAST, 7s);
                    break;
                case EVENT_SUMMON_SPIRITS:
                    SummonSpirits();
                    break;
                case EVENT_FIREBOLT_VOLLEY:
                    DoCast(SPELL_FIREBOLT_VOLLEY);
                    _events.ScheduleEvent(EVENT_FIREBOLT_VOLLEY, 12s);
                    break;
                default:
                    break;
                }
            }

            DoMeleeAttackIfReady();
        }
    };
};

class npc_burning_spirit : public CreatureScript
{
public:
    npc_burning_spirit() : CreatureScript("npc_burning_spirit") { }

    struct npc_burning_spiritAI : public ScriptedAI
    {
        npc_burning_spiritAI(Creature* creature) : ScriptedAI(creature), _moveCheckTimer(1000) {}

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (!summoner->IsCreature())
            {
                return;
            }

            _flamelasherGUID = summoner->GetGUID();
            me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(uint32 diff) override
        {
            if (_moveCheckTimer <= diff)
            {
                if (Creature* flamelasher = ObjectAccessor::GetCreature(*me, _flamelasherGUID))
                {
                    me->GetMotionMaster()->MovePoint(1, flamelasher->GetPosition());
                }
                _moveCheckTimer = 1000; // Check every second
            }
            else
            {
                _moveCheckTimer -= diff;
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == 1)
            {
                if (Creature* flamelasher = ObjectAccessor::GetCreature(*me, _flamelasherGUID))
                {
                    if (Aura* aura = flamelasher->GetAura(SPELL_BURNING_SPIRIT))
                    {
                        aura->ModStackAmount(1);
                    }
                    else
                    {
                        flamelasher->AddAura(SPELL_BURNING_SPIRIT, flamelasher);
                    }

                    int32 healAmount = CalculatePct(flamelasher->GetMaxHealth(), 3);

                    flamelasher->ModifyHealth(healAmount);

                    me->DespawnOrUnsummon();
                }
            }
        }


    private:
        uint32 _moveCheckTimer; 
        ObjectGuid _flamelasherGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetBlackrockDepthsAI<npc_burning_spiritAI>(creature);
    }
};


void AddSC_boss_ambassador_flamelash()
{
    new boss_ambassador_flamelash();
    new npc_burning_spirit();
}
